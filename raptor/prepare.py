#!/usr/bin/python3

import sys
import csv
import psycopg2
import psycopg2.extras
import data_pb2


def time2sec(time):
	(h,m,s) = time.split(":")
	return int(h)*3600+int(m)*60+int(s)

# Connect to the database
try:
	conn = psycopg2.connect('dbname=gtfs_praha')
except:
	print("Unable to connect to database \"gtfs_praha\"")
	sys.exit(1)
cur = conn.cursor(cursor_factory = psycopg2.extras.DictCursor)


#Stops
cur.execute("SELECT * FROM gtfs_stops")
stops = cur.fetchall()
stoplut = {}
stopilut = {}
pbstops = data_pb2.Stops()
for (newid,stop) in enumerate (stops):
	stoplut[stop["stop_id"]]=newid
	stopilut[newid]=stop["stop_id"]
	pbstop = pbstops.stops.add()
	pbstop.name = stop["stop_name"] 
	pbstop.id = newid
	

# Routes

class Subroute(object):
	maxid = 0
	stop_seqs = {}

	def __init__(self,stimes):
		stops = tuple(map(lambda x:stoplut[x["stop_id"]], stimes))
		pbtimes = []
		for stop in stimes:
			pbtime = data_pb2.StopTime()
			pbtime.arrival = time2sec(stop["arrival_time"])
			pbtime.departure = time2sec(stop["departure_time"])
			pbtimes.append(pbtime) 

		if stops in self.stop_seqs:
			self.stop_seqs[stops].append(pbtimes)
			self.valid = False
		else:
			self.new(pbtimes,stops)
			self.stop_seqs[stops] = self
			self.valid = True

		
	def new(self,pbtimes,stops):
		self.pbroute = data_pb2.Route()
		self.pbroute.id = Subroute.maxid
		self.pbroute.ntrips = 1
		self.pbroute.nstops = len(stops)

		Subroute.maxid += 1

		self.stops = stops
		self.triptimes = [pbtimes]

		
	
	def append(self,pbtimes):
		self.triptimes.append(pbtimes)
		self.pbroute.ntrips += 1
	
	def flush(self,rstops,trips,routes):
		self.pbroute.stopsidx = len(rstops)
		self.pbroute.tripsidx = len(trips)
		rstops.extend(self.stops)
		for triptime in self.triptimes:
			trips.extend(triptime)
		routes.extend([self.pbroute])

	
		
cur.execute("SELECT * FROM gtfs_routes;")
routes = cur.fetchall()

subroutes = []

for route in routes[:6]:
	print(route["route_short_name"])
	cur.execute("SELECT * FROM gtfs_trips WHERE route_id = %s;",(route["route_id"],))
	trips = cur.fetchall()

	for trip in trips:
		cur.execute("SELECT * FROM gtfs_stop_times WHERE trip_id = %s ORDER BY stop_sequence",(trip["trip_id"],))
		stimes = cur.fetchall()
		sr = Subroute(stimes)
		if sr.valid:
			subroutes.append(sr)


pbtimes = data_pb2.StopTimes()
pbroutes = data_pb2.Routes()
pbrstops = data_pb2.RouteStops()

for sr in subroutes:
	sr.flush(pbrstops.stop_ids,pbtimes.stop_times,pbroutes.routes)

pbsroutes = data_pb2.StopRoutes()
pbtransfers = data_pb2.Transfers()
sroutes = {}

for route in pbroutes.routes:
	for stop in pbrstops.stop_ids[route.stopsidx:route.stopsidx+route.nstops]:
		if stop in sroutes:
			sroutes[stop].append(route.id)
		else:
			sroutes[stop] = [route.id]

routecnt = 0
for stop in pbstops.stops:
	routes = sroutes.get(stop.id,[])
	stop.nroutes=len(routes)
	stop.routeidx=routecnt
	pbsroutes.route_ids.extend(routes)
	routecnt += len(routes)

	stop.ntransfers = 0
	stop.transferidx = 0

pbtt = data_pb2.Timetable()
pbtt.routes.CopyFrom(pbroutes)
pbtt.stop_times.CopyFrom(pbtimes)
pbtt.route_stops.CopyFrom(pbrstops)
pbtt.stops.CopyFrom(pbstops)
pbtt.transfers.CopyFrom(data_pb2.Transfers())
pbtt.stop_routes.CopyFrom(pbsroutes)

with open("tt.bin","wb") as ttfile:
	ttfile.write(pbtt.SerializeToString())
print(pbroutes)
print(pbstops)
