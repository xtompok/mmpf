#ifndef STRUCTS_H
#define STRUCTS_H

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
	Route * pbroute;
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
	Stop * pbstop;
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
	struct stop from;
	struct stop to;
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


//*********************************/
// Structures for osmwalk search **/
//*********************************/

struct stop_arr{
	Stop * to;
	uint64_t arrival;
};

struct stop_route{
	uint64_t departure;
	Route * pbroute;
	uint32_t n_stops;
	struct stop_arr * stops;
};

struct stop_conns{
	int n_routes;
	struct stop_route * routes;
};


#endif
