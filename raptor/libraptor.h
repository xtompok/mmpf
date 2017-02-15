#ifndef LIBRAPTOR_H
#define LIBRAPTOR_H

#include "data.pb-c.h"
#include "structs.h"
#include "dbg-print.h"
#include "util.h"
#include <stdint.h>


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

/******************/

// Search a connection
void search_con(Timetable * tt,struct mem_data * md,uint32_t from,uint32_t to,time_t time);


#endif
