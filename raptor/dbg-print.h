#ifndef DBG_PRINT_H
#define DBG_PRINT_H

#include "data.pb-c.h"
#include "structs.h"
#include <stdint.h>

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

// Print results of the searching a connection
void print_results(Timetable * tt, struct mem_data * md, int from, int to, int time);


// Convert timestamp to human readable string (allocated on heap).
char * prt_time(uint32_t time);

#endif
