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
#include "libraptor.h"

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
