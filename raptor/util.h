#ifndef UTIL_H
#define UTIL_H

#include "structs.h"

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

//Checks if the departure time of each trip of the route from the first stop increases 
void check_trips(Timetable * tt);

// Returns stop_arrival structure for the given stop and the given round of RAPTOR algorithm
struct stop_arrival get_arrival(struct mem_data * md,uint32_t stop, uint32_t round);

#endif

