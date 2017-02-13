#ifndef RAPTOR_H
#define RAPTOR_H

#include "data.pb-c.h"
#include <stdint.h>

#define MAX_TRANSFERS 8

//***************************************************************/
// Internal structures for representing timetable at given day **/
//***************************************************************/

/* message Route{
	required uint32 id = 1;
	required uint32 nstops = 2;
	required uint32 ntrips = 3;
	required uint32 stopsidx = 4;
	required uint32 tripsidx = 5;
	required uint32 servicesidx = 6;
	optional string name = 7;
}*/
struct route{
	uint32_t id;
	uint32_t nstops;
	uint32_t ntrips;
	struct stop ** stops;
	struct st_time * trips;
	char * name;
};
/* message StopTime{
	required int32 arrival = 1;
	required int32 departure = 2;
}*/

struct st_time{
	int32_t arrival;
	int32_t departure;	
};
/* 
message Stop{
	required uint32 id = 1;
	required uint32 nroutes = 2;
	required uint32 ntransfers = 3;
	required uint32 routeidx = 4;
	required uint32 transferidx = 5;
	optional string name = 6;
} */
struct stop{
	uint32_t id;
	uint32_t nroutes;	
	uint32_t ntransfers;
	struct route ** routes;
	struct transfer * transfers;
	char * name;
};

/*
message Transfer{
	required uint32 from = 1;
	required uint32 to = 2;
	required float time = 3; 
}*/

struct transfer{
	uint32_t from;
	uint32_t to;
	double time;
};

/*
message Validity {
	required uint64 start = 1;
	required uint64 end = 2;
	required bytes bitmap = 3;
};

// Not needed

*/
/*message Timetable {
//	required Date from = 1;
	repeated Route routes = 2;
	repeated StopTime stop_times = 3;
	repeated uint32 trip_validity = 4;
	repeated Validity validities = 5;
	repeated uint32 route_stops = 6;
	repeated Stop stops = 7;
	repeated Transfer transfers = 8;
	repeated uint32 stop_routes = 9;	
}*/

struct timetable {
	int nroutes;
	struct route * routes;
	struct st_time * st_times;
	struct transfer * transfers;	
	int nstops;
	struct stop * stops;
	struct stop ** rt_stops;
	struct route ** st_routes;
};


//*********************************/
// Structures for current search **/
//*********************************/

struct stop_arrivals{
	uint32_t time[MAX_TRANSFERS];
	uint32_t from[MAX_TRANSFERS];	
	uint32_t fdep[MAX_TRANSFERS];
	uint32_t route[MAX_TRANSFERS];
};
struct stop_arrival{
	uint32_t time;
	uint32_t from;
	uint32_t fdep;
	uint32_t route;	
};

struct mem_data {
	struct stop_arrivals * s_arr;	
};

//**********************/
// Printing functions **/
//**********************/

// Prints if each trip of given route is valid at given time
void print_route_trips_validity(Timetable * tt, Route * rt, time_t time);

// Prints given validity structure
void print_validity(Validity * val);

// Print some statistics about timetable
void print_tt_stats(Timetable * tt);

// Prints all trips of the timetable 
void print_trips(struct timetable * tt);

// Debug printf
#ifdef DEBUG
#define dprintf(format, ...) printf(format, __VA_ARGS__)
#else
#define dprintf(format,...) 
#endif

/*********************/
/* Printing results **/
/*********************/

// Returns relative index in the route_stops of the given stop on the given route
// Returns UINT32_MAX if the stop was not found.
uint32_t get_stopidx(Timetable * tt, uint32_t stop, uint32_t routeidx);

// Returns absolute index of the start of the trip in the stop_times determined by
// - stop id
// - time of arrival at given stop
// - route index
uint32_t get_trip_from_arrival(Timetable * tt,uint32_t stop,uint32_t arr, uint32_t routeidx);

// Returns departure of the trip determined by
// - from - station for calculation of departure
// - to - station with known arrival
// - arr - time of arrival at given to station
// - routeidx - index of the route
uint32_t get_departure(Timetable * tt,uint32_t from,uint32_t to, uint32_t arr, uint32_t routeidx);

// Returns stop_arrival structure for the given stop and the given round of RAPTOR algorithm
struct stop_arrival get_arrival(struct mem_data * md,uint32_t stop, uint32_t round);

// Print results of the searching a connection
void print_results(Timetable * tt, struct mem_data * md, int from, int to, int time);

//********************************/
// Preparing internal structure **/
//********************************/

// Load PBF timetable from the given filename
Timetable * load_timetable(char * filename);

// Create internal stops structure from PBF
struct stop * gen_stops(Timetable * tt);

// Generate lookup table for valid trips of given route at given time
// Semantic: id in PBF trips -> index in valid trips * -1 if trip is invalid
int * gen_trips_lut(Timetable * tt,Route * rt,time_t time);

// Create internal timetable structure from given PBF timetable and date
// If some structure was already created, it is possible to give pointer to it and function
// will recycle date-independent parts to save memory
struct timetable * gen_tt_for_date(Timetable * pbtt, time_t date, struct timetable * old_tt);

// Initialize memory data required for searching a connection
struct mem_data * init_mem_data(Timetable * tt);

//***************************/
// Simple useful functions **/
//***************************/

// Returns true if given time is valid
char valid_time(uint32_t time);

// Is trip (by given validity) valid at given time?
uint8_t is_valid_trip(Validity * valid, time_t time);

// Convert time_t to seconds since midnight
int time2sec(time_t time);

// Get UNIX timestamp of the midnight of the current day
time_t curr_day_timestamp(void);

// Convert timestamp to human readable string (allocated on heap).
char * prt_time(uint32_t time);

/******************/

// Search a connection
void search_con(Timetable * tt,struct mem_data * md,uint32_t from,uint32_t to,time_t time);

//Checks if the departure time of each trip of the route from the first stop increases 
void check_trips(Timetable * tt);

#endif
