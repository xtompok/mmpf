#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <string.h>

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
		stops[i].id = tt->stops[i]->id;
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
			//dprintf("No valid trips\n");
			route_lut[ridx]=-1;
			free(lut);
			continue;
		}
		
		struct route * r;
		r = tt->routes+tt->nroutes;
		//dprintf("Routes: %d, ridx: %d\n",tt->nroutes,ridx);
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
		//dprintf("Processing route %s\n",r->name);


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




struct mem_data * init_mem_data(Timetable * tt){
	struct mem_data * md;
	md = malloc(sizeof(struct mem_data));
	if (!md){
		return NULL;
	}
	md->s_arr = calloc(tt->n_stops,sizeof(struct stop_arrivals));
	//dprintf("Allocating %d kB for memdata\n",tt->n_stops*sizeof(struct stop_arrivals)/1024);
	if (!md->s_arr){
		return NULL;
	}
	return md;
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
		//dprintf("===========Round %d============\n",round);
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

			//dprintf("--Route %s (id: %d)\n",rt->name,rt->id);
			for (int s=0; s<rt->nstops; s++){
				struct stop_arrivals * curst;
				curst = &(md->s_arr[rs[s]->id]); 
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

