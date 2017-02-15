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
#include "util.h"

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
