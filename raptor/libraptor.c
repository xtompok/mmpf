#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <err.h>

#include "data.pb-c.h"
#include "libraptor.h"
#include "util.h"



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


struct stop * gen_stops(Timetable * tt){
	struct stop * stops;
	stops = calloc(tt->n_stops,sizeof(struct stop));
	for (int i=0;i<tt->n_stops;i++){
		stops[i].pbstop = tt->stops[i];
		stops[i].nroutes = tt->stops[i]->nroutes;
		stops[i].ntransfers = tt->stops[i]->ntransfers;
		stops[i].name = malloc(strlen(tt->stops[i]->name)+1);
		strcpy(stops[i].name,tt->stops[i]->name);
	}	
	return stops;
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
void free_tt(struct timetable * tt){
	for (int stopidx=0; stopidx < tt->nstops; stopidx++){
		free(tt->stops[stopidx].name);
	}
	for (int ridx=0;ridx < tt->nroutes;ridx++){
		free(tt->routes[ridx].name);
	}
	free(tt->routes);
	free(tt->st_times);
	free(tt->rt_stops);
	free(tt->st_routes);	
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

/*	for (int sidx=0;sidx < pbtt->n_stops;sidx++){
		printf("Stop: %s\n",pbtt->stops[sidx]->name);
		for (int ridx=pbtt->stops[sidx]->routeidx;ridx < pbtt->stops[sidx]->routeidx+pbtt->stops[sidx]->nroutes;ridx++){
			printf("%s, ",pbtt->routes[pbtt->stop_routes[ridx]]->name);
		}
		printf("\n");
	}*/
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
		//printf("Route: %s, ridx: %d ,Trips: %d\n",pbr->name,ridx,ntrips); 
		if (ntrips == 0){
			dprintf("No valid trips for route %s\n",pbr->name);
			route_lut[ridx]=-1;
			free(lut);
			continue;
		}
		
		struct route * r;
		r = tt->routes+tt->nroutes;
		//dprintf("Routes: %d, ridx: %d\n",tt->nroutes,ridx);
		route_lut[ridx]=tt->nroutes;
		tt->nroutes++;
		r->pbroute = pbtt->routes[ridx];
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
		//dprintf("Processing route %s\n",r->name);


		free(lut);
	}
	
	int st_routesidx;
	int stopidx;
	st_routesidx=0;
	stopidx = 0;
	tt->stops[0].routes = tt->st_routes;
//	printf("New stop routes\n");
//	printf("%p\n",tt);
	for (int sridx=0;sridx < pbtt->n_stop_routes;sridx++){
		uint32_t pbridx;
		pbridx = pbtt->stop_routes[sridx];

		if (pbtt->stops[stopidx]->nroutes==0){
			tt->stops[stopidx].routes = NULL;
			tt->stops[stopidx].nroutes = 0;
			stopidx++;
			continue;
		}
		
		// We are switching to the routes for the next stop
		if (sridx >= pbtt->stops[stopidx]->routeidx + pbtt->stops[stopidx]->nroutes){
			tt->stops[stopidx].nroutes = tt->st_routes+st_routesidx - tt->stops[stopidx].routes;
			stopidx++;
//			printf("\nStop: %s\n",tt->stops[stopidx].name);
			tt->stops[stopidx].routes = tt->st_routes+st_routesidx;
		}
		// Route has no trips at given date
		if (route_lut[pbridx] == -1){
			continue;
		}
		
		// Append route to the list
		tt->st_routes[st_routesidx] = tt->routes+route_lut[pbridx];
//		printf("%s, ",tt->st_routes[st_routesidx]->name);
		st_routesidx++;

	}
	tt->stops[stopidx].nroutes = tt->st_routes+st_routesidx - tt->stops[stopidx].routes;
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
	free(route_lut);
	return tt;

}




struct mem_data * create_mem_data(Timetable * tt){
	struct mem_data * md;
	md = malloc(sizeof(struct mem_data));
	if (!md){
		return NULL;
	}
	md->s_arr = calloc(tt->n_stops,sizeof(struct stop_arrivals));
	if (!md->s_arr){
		return NULL;
	}
	return md;
}

void clear_mem_data(struct mem_data * md,int nstops){
	for (int i=0;i<nstops;i++){
		for (int j=0;j<MAX_TRANSFERS;j++){
			md->s_arr[i].time[j] = UINT32_MAX;
			md->s_arr[i].from[j] = UINT32_MAX;
			md->s_arr[i].fdep[j] = UINT32_MAX;
			md->s_arr[i].route[j] = UINT32_MAX;
		}	
	}
}



void search_con(Timetable * tt,
	struct mem_data * md,
	uint32_t from,
	uint32_t to,
	time_t time,
	int max_rounds){

	time_t date;
	date = time - (time%(24*3600));
	time %= 24*3600;
	struct timetable * newtt;
	newtt = gen_tt_for_date(tt,date,NULL); 

	md->s_arr[from].time[0] = time;
	
	for (int round=1;round < max_rounds;round++){
		//dprintf("===========Round %d============\n",round);
		//Copy times from last round
		for (int s=0;s<newtt->nstops;s++){
			md->s_arr[s].time[round] = md->s_arr[s].time[round-1];
			md->s_arr[s].from[round] = md->s_arr[s].from[round-1];
			md->s_arr[s].fdep[round] = md->s_arr[s].fdep[round-1];
			md->s_arr[s].route[round] = md->s_arr[s].route[round-1];
					
		}

		for (int t=0;t<tt->n_transfers;t++){
					
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

			//dprintf("--Route %s (id: %d)\n",rt->name,rt->id);
			for (int s=0; s<rt->nstops; s++){
				struct stop_arrivals * curst;
				curst = &(md->s_arr[rs[s]->pbstop->id]); 
				//dprintf("At: %s on %s\n",rs[s]->name,prt_time(curst->time[round-1]));
				// Trip undefined
				if (trip==-1){
					if (curst->time[round-1] == UINT32_MAX){
						//dprintf(".");
						continue;
					}
					for (int t=0; t<rt->ntrips*rt->nstops; t+=rt->nstops){
						// TODO: Does not consider trips through midnight
						if (st[s+t].departure <= curst->time[round-1])
							continue;
						trip = t;
						trip_from = rs[s]->pbstop->id;
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
						curst->route[round] = newtt->routes[r].pbroute->id;
						//dprintf("Updated time at %s to %s\n",rs[s]->name,prt_time(curst->time[round]));
					}
					while ((trip > 0) && (curst->time[round-1] < st[s+trip-rt->nstops].departure)){
						trip -= rt->nstops;
						//dprintf("Trip decreased to %s\n",prt_time(st[s+trip].arrival));
					}
					
				}



			}
		}
	}		
}

void free_conns(struct stop_conns * conns){
	for (int ridx = 0;ridx < conns->n_routes;ridx++){
		free(conns->routes[ridx].stops);
	}
	free(conns->routes);
	free(conns);
}

struct stop_conns * search_stop_conns(struct timetable * newtt, uint32_t from, time_t time){
	time_t date;
	date = time - (time%(24*3600));
	time %= 24*3600;

	char * timestr;
	timestr = prt_time(time);
	printf("Stop: %s at: %s\n",newtt->stops[from].name,timestr);
	free(timestr);
	
	struct stop_conns * conns;
	conns = malloc(sizeof(struct stop_conns));
	conns->n_routes = newtt->stops[from].nroutes;
	conns->routes = calloc(sizeof(struct stop_route),conns->n_routes);

	int nroutes;
	nroutes = 0;

	for (int rIdx=0;rIdx<conns->n_routes;rIdx++){
		struct route * r;
		r = newtt->stops[from].routes[rIdx];
		printf("Route: %s\n",r->name);
			
	}

	for (int rIdx=0;rIdx<conns->n_routes;rIdx++)
	{
		struct route * r;
		r = newtt->stops[from].routes[rIdx];
		printf("Route: %s\n",r->name);
		int fidx=0;
		printf("Search from: %s\n",r->stops[0]->name);
		printf("Stop: %d\n",from);
		while (r->stops[fidx]->pbstop->id != from){	
			printf("Stop: %s(%d)\n",r->stops[fidx]->name,r->stops[fidx]->pbstop->id);
			fidx++;
			if (fidx >= r->nstops){
				err(1,"Error, stop not found");
			}
			
		}
		// Can't get on on the last stop
		if (fidx == r->nstops-1){
			continue;
		}

		int t=0;
		unsigned char no_trips = 0;
		while (r->trips[fidx+t].departure < time){
			t+=r->nstops;	
			if ((t/(r->nstops)) >= r->ntrips){
				no_trips = 1;
				break;
			}
		}
		if (no_trips){
			continue;		
		}

		conns->routes[nroutes].departure = r->trips[fidx+t].departure;
		conns->routes[nroutes].pbroute = r->pbroute;
		conns->routes[nroutes].n_stops = r->nstops-fidx-1;
		conns->routes[nroutes].stops = calloc(sizeof(struct stop_arr),conns->routes[nroutes].n_stops);
		struct stop_arr * arrs;
		arrs = conns->routes[nroutes].stops;
		char * timestr;
		timestr = prt_time(conns->routes[nroutes].departure);
		printf("Departure at %s\n",timestr);
		free(timestr);
		for (int sidx=0;sidx < (conns->routes[nroutes].n_stops);sidx++){
			arrs[sidx].to = r->stops[fidx+sidx+1]->pbstop;
			arrs[sidx].arrival = r->trips[fidx+t+sidx+1].arrival;
			timestr = prt_time(arrs[sidx].arrival);
			printf("At %s on %s\n",arrs[sidx].to->name,timestr);
			free(timestr);
		}
		nroutes++;
			


		
	}
	conns->n_routes = nroutes;

	return conns;
}
