#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "data.pb-c.h"
#define MAX_TRANSFERS 8

struct stop_arrivals{
	uint32_t time[MAX_TRANSFERS];
	uint32_t from[MAX_TRANSFERS];	
	uint32_t route[MAX_TRANSFERS];
};

struct mem_data {
	struct stop_arrivals * s_arr;	
};

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

struct mem_data * init_mem_data(Timetable * tt){
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

void search_con(Timetable * tt,struct mem_data * md,uint32_t from,uint32_t to,uint32_t time){	
	for (int s=0;s<tt->n_stops;s++){
		for (int k=0;k<MAX_TRANSFERS;k++){
			md->s_arr[s].time[k] = UINT32_MAX;
		}
	}

	md->s_arr[from].time[0] = time;
	
	for (int round=1;round < MAX_TRANSFERS;round++){
		printf("===========Round %d============\n",round);
		//Copy times from last round
		for (int s=0;s<tt->n_stops;s++){
			md->s_arr[s].time[round] = md->s_arr[s].time[round-1];
			md->s_arr[s].from[round] = md->s_arr[s].from[round-1];
			md->s_arr[s].route[round] = md->s_arr[s].route[round-1];
					
		}
		
		// Update times with transfers
		for (int r=0;r<tt->n_routes;r++){	
			uint32_t * rs;
			StopTime ** st;
			int nstops;
			int ntrips;

			rs = tt->route_stops + tt->routes[r]->stopsidx;
			st = tt->stop_times + tt->routes[r]->tripsidx;
			nstops = tt->routes[r]->nstops;
			ntrips = tt->routes[r]->ntrips;

			int trip = -1;
			int trip_from = -1;

			printf("--Route %s\n",tt->routes[r]->name);
			for (int s=0; s<nstops; s++){
				struct stop_arrivals * curst;
				curst = &(md->s_arr[rs[s]]);
				printf("At: %s on %s\n",tt->stops[rs[s]]->name,prt_time(curst->time[round-1]));
				// Trip undefined
				if (trip==-1){
					if (curst->time[round-1] == UINT32_MAX){
						printf(".");
						continue;
					}
					for (int t=0; t<ntrips*nstops; t+=nstops){
						if (st[s+t]->departure <= curst->time[round-1])
							continue;
						trip = t;
						trip_from = rs[s];
						printf("Found trip %d on route %s at %s on %s\n",trip,tt->routes[r]->name,tt->stops[rs[s]]->name,prt_time(st[s+t]->departure));
						if (t>0){
							printf("Previous trip at %s\n",prt_time(st[s+t-nstops]->departure));
						}
						break;
					}

				// Trip found
				}else{
					printf("Current time: %s, trip: %s",
						prt_time(curst->time[round]),
						prt_time(st[s+trip]->arrival));
					if (trip>0){
						printf(", prev trip: %s\n",prt_time(st[s+trip-nstops]->arrival));
					}else{
						printf("\n");
					}
					if (curst->time[round] > st[s+trip]->arrival){
						curst->time[round] = st[s+trip]->arrival;
						curst->from[round] = trip_from;
						curst->route[round] = r;
						//printf("Updated time at %s to %d\n",tt->stops[s]->name,curst->time[round]);
					}
					while ((trip > 0) && (curst->time[round-1] < st[s+trip-nstops]->departure)){
						trip -= nstops;
						printf("Trip decreased to %d\n",trip);
					}
					
				}



			}
		}
	}		
}

void print_results(Timetable * tt, struct mem_data * md, int from, int to, int time){
	for (int s=0;s<tt->n_stops;s++){
		
		for (int k=0;k<MAX_TRANSFERS;k++){
			if (md->s_arr[s].time[k] == UINT32_MAX)
				continue;
			printf("Station: %s(%d), time: %s, round %d, from %d\n",tt->stops[s]->name,s,prt_time(md->s_arr[s].time[k]),k,md->s_arr[s].from[k]);
		}
	
	}


	printf("From: %s at %s\n",tt->stops[from]->name,prt_time(time));
	for (int k=0;k<MAX_TRANSFERS;k++){
		printf("To %s at %s by %s\n",
			tt->stops[to]->name,
			prt_time(md->s_arr[to].time[k]),
			tt->routes[md->s_arr[to].route[k]]->name);
	}

	uint32_t stops_buf[MAX_TRANSFERS];
	uint32_t stop;
	int round;
	int count;
	stop = to;
	round = MAX_TRANSFERS-1;
	count = 0;
	while (stop != from){
		stop = md->s_arr[stop].from[round];
		stops_buf[round] = stop;
		round--;
		count++;
	}
	for (int i=round+1;i<MAX_TRANSFERS;i++){
		uint32_t s;
		s = stops_buf[i];
		printf("Stop: %s, time: %s by: %s\n",tt->stops[s]->name,prt_time(md->s_arr[s].time[i]),tt->routes[md->s_arr[s].route[i]]->name);	
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
	printf("Timetable loaded. Stops: %d, routes: %d\n",timetable->n_stops,timetable->n_routes);
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

	printf("Searching from %d to %d at %d\n",from,to,time);

	search_con(timetable,md,from,to,time);	
	print_results(timetable,md,from,to,time);

	return 0;
}
