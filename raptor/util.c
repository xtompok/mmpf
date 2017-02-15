#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "data.pb-c.h"
#include "util.h"


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

char valid_time(uint32_t time){
	return !(time==UINT32_MAX);		
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

struct stop_arrival get_arrival(struct mem_data * md,uint32_t stop, uint32_t round){
	struct stop_arrival sa;
	sa.from = md->s_arr[stop].from[round];
	sa.fdep = md->s_arr[stop].fdep[round];
	sa.time = md->s_arr[stop].time[round];
	sa.route = md->s_arr[stop].route[round];
	return sa;
}
