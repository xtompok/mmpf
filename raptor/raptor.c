#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "data.pb-c.h"
#include "raptor.h"

char * prt_time(uint32_t time){
	char * str;
	str = malloc(9);
	if (time==UINT32_MAX){
		sprintf(str,"--:--:--");
	}else{
		sprintf(str,"%2d:%02d:%02d",time/3600, (time%3600)/60, time%60);
	}
	return str;
}

uint8_t is_valid_trip(Validity * valid, time_t time){
	if (valid->start > time){
		return 0;
	}
	if (valid->end < time){
		return 0;
	}
	int days;
	int index;
	int bitindex;
	days = (time-valid->start)/(24*3600);
	index = days/8;
	bitindex = days%8;
	return valid->bitmap.data[index]&(1<<bitindex);
}


struct stop * gen_stops(Timetable * tt){
	struct stop * stops;
	stops = calloc(tt->n_stops,sizeof(struct stop));
	for (int i=0;i<tt->n_stops;i++){
		stops[i].id = tt->stops[i]->id;
		stops[i].nroutes = tt->stops[i]->nroutes;
		stops[i].ntransfers = tt->stops[i]->ntransfers;
		stops[i].name = malloc(strlen(tt->stops[i]->name)+1);
		strcpy(stops[i].name,tt->stops[i]->name);
	}	
	return stops;
}

void print_route_trips_validity(Timetable * tt, Route * rt, time_t time){
	for (int i=0;i<rt->ntrips;i++){
		printf("%s at %s on %s - ",
			tt->stops[tt->route_stops[rt->stopsidx]]->name,
			prt_time(tt->stop_times[rt->tripsidx+i*rt->nstops]->departure),
			rt->name);

		if (is_valid_trip(tt->validities[tt->trip_validity[rt->servicesidx+i]],time)){
			printf("valid (%d)\n",tt->trip_validity[rt->servicesidx+i]);
		} else {
			printf("invalid (%d)\n",tt->trip_validity[rt->servicesidx+i]);
		}
	}
	
}

int * gen_trips_lut(Timetable * tt,Route * rt,time_t time){
	int * lut;
	unsigned int lutidx;
	
	lut = malloc(rt->ntrips*sizeof(int));
	lutidx = 0;
	for (int i=0;i<rt->ntrips;i++){
		if (is_valid_trip(tt->validities[tt->trip_validity[rt->servicesidx+i]],time)){
			lut[i]=lutidx;
			lutidx++;	
		} else {
			lut[i]=-1;
		}
	}
	return lut;
}

struct timetable * gen_tt_for_date(Timetable * pbtt, time_t date, struct timetable * old_tt){
	struct timetable * tt;
	tt = calloc(1,sizeof(struct timetable));
	if (old_tt == NULL){
		tt->stops=gen_stops(pbtt);
		tt->nstops=pbtt->n_stops;
	}else{
		tt->stops=old_tt->stops;
		tt->nstops=old_tt->nstops;
	}
	tt->routes = calloc(pbtt->n_routes,sizeof(struct route));
	tt->st_times = calloc(pbtt->n_stop_times,sizeof(struct st_time));
	tt->rt_stops = calloc(pbtt->n_route_stops,sizeof(struct stop *));
	tt->st_routes = calloc(pbtt->n_stop_routes,sizeof(struct route *));
	
	int * route_lut;
	route_lut = calloc(pbtt->n_routes,sizeof(int));
	int nst_times;
	int nrt_stops;
	tt->nroutes = 0;
	nst_times = 0;
	nrt_stops = 0;

	for (int ridx=0;ridx < pbtt->n_routes; ridx++){
		Route * pbr;
		pbr = pbtt->routes[ridx];
		int32_t * lut;
		lut = gen_trips_lut(pbtt,pbr,date);
		int32_t ntrips;
		ntrips=0;
		for (int i=0;i<pbr->ntrips;i++){
			if (lut[i]!= -1)
				ntrips++;
		}
		if (ntrips == 0){
			dprintf("No valid trips\n");
			route_lut[ridx]=-1;
			free(lut);
			continue;
		}
		
		struct route * r;
		r = tt->routes+tt->nroutes;
		dprintf("Routes: %d, ridx: %d\n",tt->nroutes,ridx);
		route_lut[ridx]=tt->nroutes;
		tt->nroutes++;
		r->id = pbr->id;
		r->nstops = pbr->nstops;
		r->stops = tt->rt_stops + nrt_stops ;
		for (int i=0;i<pbr->nstops;i++){
			r->stops[i] = tt->stops+pbtt->stops[pbtt->route_stops[pbr->stopsidx+i]]->id;
			nrt_stops++;
		}
		r->ntrips = ntrips;
		r->trips = tt->st_times+nst_times;
		for (int i=0;i<pbr->ntrips;i++){
			if (lut[i]== -1){
				continue;
			}
			for (int s=0;s<r->nstops;s++){
				struct st_time * st;
				st = tt->st_times+nst_times;
				StopTime * pbst;
				pbst = pbtt->stop_times[pbr->tripsidx+i*pbr->nstops+s];
				nst_times++;
				st->arrival = pbst->arrival;
				st->departure = pbst->departure;
			}
		}
		r->name = malloc(strlen(pbr->name)+1);
		strcpy(r->name,pbr->name);
		dprintf("Processing route %s\n",r->name);


		free(lut);
	}
	
	int st_routesidx;
	int stopidx;
	st_routesidx=0;
	stopidx = 0;
	tt->stops[0].routes = tt->st_routes;
	for (int sr=0;sr < pbtt->n_stop_routes;sr++){
		uint32_t pbridx;
		pbridx = pbtt->stop_routes[sr];
		if (route_lut[pbridx] == -1){
			continue;
		}
		tt->st_routes[st_routesidx] = tt->routes+route_lut[pbridx];
		st_routesidx++;
		if (sr > pbtt->stops[stopidx]->routeidx+pbtt->stops[stopidx]->nroutes){
			tt->stops[stopidx].nroutes = tt->st_routes+st_routesidx - tt->stops[stopidx].routes;
			stopidx++;
			tt->stops[stopidx].routes = tt->st_routes+st_routesidx;
		}
	tt->stops[stopidx].nroutes = tt->st_routes+st_routesidx - tt->stops[stopidx].routes;
	
	}
/*	for (int s=0;s<pbtt->n_stops;s++){
		tt->stops[s].routes=tt->st_routes+st_routesidx;
		for (int r=0;r<pbtt->n_routes;r++){
			
			if (route_lut[r] == -1){
				continue;
			}
			tt->st_routes[st_routesidx]=tt->routes+route_lut[r];
			st_routesidx++;
		}
		tt->stops[s].nroutes=(tt->st_routes+st_routesidx)-(tt->stops[s].routes);
		
	}
*/
	return tt;

}


void print_validity(Validity * val){
	char * week[] = {"N","P","U","S","C","P","S"};
	struct tm starttm;
	struct tm endtm;
	char wday;
	starttm = *(localtime(&(val->start)));
	wday = starttm.tm_wday;
	endtm = *(localtime(&(val->end)));
	printf("Start: %s",asctime(&starttm));	
	printf("End: %s",asctime(&endtm));	
	for (int i = 0; i<val->bitmap.len*8; i++){
		printf("%s",week[wday]);
		wday++;
		wday%=7;
	}
	printf("\n");
	for (int i = 0; i<val->bitmap.len*8; i++){
		printf("%d",!!(val->bitmap.data[i/8]&(1<<(i%8))));
	}
	printf("\n");


}

char valid_time(uint32_t time){
	return !(time==UINT32_MAX);		
}

/*void filter_trips_by_date(Timetable * tt,uint64_t date){
	for (int i=0;i<tt->n_routes;i++){
		Route * r;
		r = tt->routes[i];
	} 	
}*/

// Return relative index of stop on route in stop_times or UINT32_MAX if not found
uint32_t get_stopidx(Timetable * tt, uint32_t stop, uint32_t routeidx){
	Route * route;
	route = tt->routes[routeidx];
	uint32_t stopidx;
	stopidx = UINT32_MAX;
	for (int i=route->stopsidx;i<route->stopsidx+route->nstops;i++){
		if (tt->route_stops[i]==stop){
			stopidx = i-route->stopsidx;
			break;
			}
	}
	return stopidx;

}

uint32_t get_trip_from_arrival(Timetable * tt,uint32_t stop,uint32_t arr, uint32_t routeidx){
	Route * route;
	route = tt->routes[routeidx];
	uint32_t stopidx;
	stopidx = get_stopidx(tt,stop,routeidx);

	if (stopidx == UINT32_MAX){
		printf("Stop not found");
		return UINT32_MAX;	
	}

	for (int i=route->tripsidx+stopidx;
		i<route->tripsidx+(route->ntrips*route->nstops);
		i+=route->nstops){
		
		if (tt->stop_times[i]->arrival == arr){
			return i-stopidx;
		}
		
	}
	return UINT32_MAX;
}

uint32_t get_departure(Timetable * tt,uint32_t from,uint32_t to, uint32_t arr, uint32_t routeidx){
	uint32_t tostopidx;
	tostopidx = get_stopidx(tt,to,routeidx);

	uint32_t fromstopidx;
	fromstopidx = get_stopidx(tt,from,routeidx);

	if (tostopidx == UINT32_MAX || fromstopidx == UINT32_MAX ){
		printf("Stop not found");
		return UINT32_MAX;	
	}

	uint32_t tripidx;
	tripidx = get_trip_from_arrival(tt,to,arr,routeidx);
	if (tripidx == UINT32_MAX){
		printf("Trip not found");
		return UINT32_MAX;	
	}
	
	return tt->stop_times[tripidx+fromstopidx]->departure;

	
}

struct stop_arrival get_arrival(struct mem_data * md,uint32_t stop, uint32_t round){
	struct stop_arrival sa;
	sa.from = md->s_arr[stop].from[round];
	sa.fdep = md->s_arr[stop].fdep[round];
	sa.time = md->s_arr[stop].time[round];
	sa.route = md->s_arr[stop].route[round];
	return sa;
}

Timetable * load_timetable(char * filename){
	FILE * ttfile;
	unsigned char * ttbuf;
	size_t ttbuf_sz;
	ttfile = fopen(filename,"r");
	fseek(ttfile,0L,SEEK_END);
	ttbuf_sz = ftell(ttfile);
	ttbuf = malloc(ttbuf_sz);
	if (!ttbuf){
		printf("Allocation failed\n");
		return NULL;	
	}
	rewind(ttfile);
	fread(ttbuf,ttbuf_sz,1,ttfile);

	return timetable__unpack(NULL,ttbuf_sz,ttbuf);
}
void print_tt_stats(Timetable * tt){
	printf("Stations: %lu\n",tt->n_stops);
	printf("Routes: %lu\n",tt->n_routes);
	printf("Trips: %lu\n",tt->n_trip_validity);	
	int ntrips;
	ntrips = 0;
#ifdef DEBUG
	printf("Services:");
	for (int r=0;r<tt->n_routes;r++){
		Route * route;
		route = tt->routes[r];
		printf("\nRoute: %s\n",route->name);
		printf("Route id:%d\n",route->id);
		printf("Servicesidx: %d\n",route->servicesidx);
		ntrips += route->ntrips;
		for (int t=0;t<(route->nstops*route->ntrips);t+=route->nstops){
			printf("%d ",tt->trip_validity[route->servicesidx+t/route->nstops]);
		}
	}
#endif 
	printf("\n");
	printf("Trips: %d\n",ntrips);
}

void print_trips(struct timetable * tt){
	for (int ridx = 0;ridx<tt->nroutes;ridx++){
		struct route * r;
		r = tt->routes+ridx;
		printf("Route: %s\n",r->name);
		for (int sidx=0;sidx < r->nstops;sidx++){
			printf("%30s	",r->stops[sidx]->name);
			for (int tidx=0;tidx < r->ntrips;tidx++){
				printf("%s ",prt_time(r->trips[tidx*r->nstops+sidx].departure));		
			}	
			printf("\n");
		}	
	}

}

struct mem_data * init_mem_data(Timetable * tt){
	struct mem_data * md;
	md = malloc(sizeof(struct mem_data));
	if (!md){
		return NULL;
	}
	md->s_arr = calloc(tt->n_stops,sizeof(struct stop_arrivals));
	dprintf("Allocating %d kB for memdata\n",tt->n_stops*sizeof(struct stop_arrivals)/1024);
	if (!md->s_arr){
		return NULL;
	}
	return md;
}

int time2sec(time_t time){
	struct tm start;
	start.tm_year=2017;
	start.tm_mon=0;
	start.tm_mday=1;
	start.tm_hour=0;
	start.tm_min = 0;
	start.tm_sec = 0;
	
	time_t start_time;
	start_time = mktime(&start);
	
	return ((int)floor(difftime(time,start_time)))%(24*3600);
}

time_t curr_day_timestamp(void){
	time_t curr_time;
	curr_time=time(NULL);
	curr_time-=curr_time%(24*3600);
	return curr_time;
		
}

void search_con(Timetable * tt,struct mem_data * md,uint32_t from,uint32_t to,time_t time){	
	time_t date;
	date = time - (time%(24*3600));
	time %= 24*3600;
	struct timetable * newtt;
	newtt = gen_tt_for_date(tt,date,NULL); 
	for (int s=0;s<newtt->nstops;s++){
		for (int k=0;k<MAX_TRANSFERS;k++){
			md->s_arr[s].time[k] = UINT32_MAX;
		}
	}

	md->s_arr[from].time[0] = time;
	
	for (int round=1;round < MAX_TRANSFERS;round++){
		dprintf("===========Round %d============\n",round);
		//Copy times from last round
		for (int s=0;s<newtt->nstops;s++){
			md->s_arr[s].time[round] = md->s_arr[s].time[round-1];
			md->s_arr[s].from[round] = md->s_arr[s].from[round-1];
			md->s_arr[s].fdep[round] = md->s_arr[s].fdep[round-1];
			md->s_arr[s].route[round] = md->s_arr[s].route[round-1];
					
		}
		
		// Update times with transfers
		for (int r=0;r<newtt->nroutes;r++){	
			struct route * rt;
			struct stop ** rs;
			struct st_time * st;
			
			rt = newtt->routes+r;
			rs = rt->stops;
			st = rt->trips;

			int trip = -1;
			int trip_from = -1;
			uint32_t trip_fdep = UINT32_MAX;

			dprintf("--Route %s (id: %d)\n",rt->name,rt->id);
			for (int s=0; s<rt->nstops; s++){
				struct stop_arrivals * curst;
				curst = &(md->s_arr[rs[s]->id]); 
				dprintf("At: %s on %s\n",rs[s]->name,prt_time(curst->time[round-1]));
				// Trip undefined
				if (trip==-1){
					if (curst->time[round-1] == UINT32_MAX){
						dprintf(".");
						continue;
					}
					for (int t=0; t<rt->ntrips*rt->nstops; t+=rt->nstops){
						// TODO: Does not consider trips through midnight
						if (st[s+t].departure <= curst->time[round-1])
							continue;
						trip = t;
						trip_from = rs[s]->id;
						trip_fdep = st[s+t].departure;
#ifdef DEBUG
						printf("Found trip %d on route %s at %s on %s\n",trip,rt->name,rs[s]->name,prt_time(st[s+t].departure));
						if (t>0){
							printf("Previous trip at %s\n",prt_time(st[s+t-rt->nstops].departure));
						}
#endif
						break;
					}

				// Trip found
				}else{
#ifdef DEBUG
					printf("Current minimal time: %s, current trip: %s",
						prt_time(curst->time[round]),
						prt_time(st[s+trip].arrival));
					if (trip>0){
						printf(", prev trip: %s\n",prt_time(st[s+trip-rt->nstops].arrival));
					}else{
						printf("\n");
					}
#endif
					if (curst->time[round] > st[s+trip].arrival){
						curst->time[round] = st[s+trip].arrival;
						curst->from[round] = trip_from;
						curst->fdep[round] = trip_fdep;
						curst->route[round] = newtt->routes[r].id;
						dprintf("Updated time at %s to %s\n",rs[s]->name,prt_time(curst->time[round]));
					}
					while ((trip > 0) && (curst->time[round-1] < st[s+trip-rt->nstops].departure)){
						trip -= rt->nstops;
						dprintf("Trip decreased to %s\n",prt_time(st[s+trip].arrival));
					}
					
				}



			}
		}
	}		
}

void print_results(Timetable * tt, struct mem_data * md, int from, int to, int time){
#if DEBUG 
	for (int s=0;s<tt->n_stops;s++){
		for (int k=0;k<MAX_TRANSFERS;k++){
			if (md->s_arr[s].time[k] == UINT32_MAX)
				continue;
			uint32_t from;
			uint32_t arr;
			uint32_t route;
			from = md->s_arr[s].from[k];
			arr = md->s_arr[s].time[k];
			route = md->s_arr[s].route[k];

			printf("Station: %s(%d), time: %s, round %d, from %s, time: %s\n",
				tt->stops[s]->name,s,
				prt_time(arr),k,
				tt->stops[from]->name,
				prt_time(get_departure(tt,from,s,arr,route)));
		}
	
	}
#endif


	printf("From: %s at %s\n",tt->stops[from]->name,prt_time(time));
	for (int k=0;k<MAX_TRANSFERS;k++){
		if (valid_time(md->s_arr[to].time[k])){
			printf("To %s at %s by %s\n",
				tt->stops[to]->name,
				prt_time(md->s_arr[to].time[k]),
				tt->routes[md->s_arr[to].route[k]]->name);
		} else {
			printf("Can't get to %s with %d transfers\n",
				tt->stops[to]->name,k);
		}
	}

	uint32_t stops_buf[MAX_TRANSFERS];
	uint32_t stop;
	int round;
	int count;
	stop = to;
	stops_buf[MAX_TRANSFERS-1]=stop;
	round = MAX_TRANSFERS-2;
	count = 0;
	while (stop != from){
		if (!valid_time(md->s_arr[stop].time[round])){
			printf("Connection not found\n");
			return;
		}
		stop = md->s_arr[stop].from[round];
		stops_buf[round] = stop;
		round--;
		count++;
	}
	for (int i=round+2;i<MAX_TRANSFERS;i++){
		uint32_t s;
		s = stops_buf[i];
		struct stop_arrival sa;
		sa = get_arrival(md,s,i);
		printf("%s (%s) --> %s (%s) by %s\n",
			tt->stops[sa.from]->name,
			prt_time(sa.fdep),
			tt->stops[s]->name,
			prt_time(sa.time),
			tt->routes[sa.route]->name);	
	}

	
	


}
void check_trips(Timetable * tt){
	for (int r=0;r<tt->n_routes;r++){
		StopTime ** st;
		int ntrips;
		int nstops;

		st = tt->stop_times + tt->routes[r]->tripsidx;
		ntrips = tt->routes[r]->ntrips;
		nstops = tt->routes[r]->nstops;

		int lasttrip = 0;

		for (int t=0;t <ntrips*nstops; t+=nstops){
			if (st[t]->departure < lasttrip){
				printf("Wrong order: Route %d trip %d time %d last %d\n",r,t/nstops,st[t]->departure,lasttrip);
			}
			lasttrip = st[t]->departure;
		}
	}	

}

int main(int argc, char ** argv){
	
	Timetable * timetable;
	timetable = load_timetable("tt.bin");
	if (!timetable){
		printf("Failed to load timetable\n");	
		return 1;
	}
	print_tt_stats(timetable);
	printf("Timetable loaded. Stops: %lu, routes: %lu\n",timetable->n_stops,timetable->n_routes);
	check_trips(timetable);

//	for (int i=0; i<timetable->n_stop_times;i++){
//		printf("%d %d\n",timetable->stop_times[i]->arrival, timetable->stop_times[i]->departure);
//	}
	
	struct mem_data * md;
	md = init_mem_data(timetable);
	if (!md){
		printf("Failed to init in-memory data");
		return 1;	
	}

	
	uint32_t from;
	uint32_t to;
	uint32_t time;

	if (argc < 4){
		printf("Usage: %s from to time\n",argv[0]);
		return 1;
	}

	from = atoi(argv[1]);
	to = atoi(argv[2]);
	time = atoi(argv[3]);

	printf("Searching from %d to %d at %lu\n",from,to,time+curr_day_timestamp());

	search_con(timetable,md,from,to,time+curr_day_timestamp());	
	print_results(timetable,md,from,to,time);

	return 0;
}
