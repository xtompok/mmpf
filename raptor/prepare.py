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

pbtt = data_pb2.Timetable()

#Stops
cur.execute("SELECT * FROM gtfs_stops")
stops = cur.fetchall()
stoplut = {}
stopilut = {}
pbstops = pbtt.stops
for (newid,stop) in enumerate (stops):
	stoplut[stop["stop_id"]]=newid
	stopilut[newid]=stop["stop_id"]
	pbstop = pbstops.add()
	pbstop.name = stop["stop_name"] 
	pbstop.id = newid
	

# Routes

class Subroute(object):
	maxid = 0
	stop_seqs = {}

	def __init__(self,stimes,name=None):
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
			self.new(pbtimes,stops,name)
			self.stop_seqs[stops] = self
			self.valid = True

		
	def new(self,pbtimes,stops,name):
		self.pbroute = data_pb2.Route()
		self.pbroute.id = Subroute.maxid
		self.pbroute.ntrips = 1
		self.pbroute.nstops = len(stops)
		if name:
			self.pbroute.name = name

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
		self.triptimes.sort(key=lambda x:x[0].departure)
		for triptime in self.triptimes:
			trips.extend(triptime)
		routes.extend([self.pbroute])

	
		
cur.execute("SELECT * FROM gtfs_routes ORDER BY route_short_name;")
routes = cur.fetchall()

subroutes = []

for route in routes:
	print(route["route_short_name"])
	cur.execute("SELECT * FROM gtfs_trips WHERE route_id = %s ORDER BY trip_id;",(route["route_id"],))
	trips = cur.fetchall()

	for trip in trips:
		cur.execute("SELECT * FROM gtfs_stop_times WHERE trip_id = %s ORDER BY stop_sequence",(trip["trip_id"],))
		stimes = cur.fetchall()
		sr = Subroute(stimes,route["route_short_name"])
		if sr.valid:
			subroutes.append(sr)


pbtimes = pbtt.stop_times
pbroutes = pbtt.routes
pbrstops = pbtt.route_stops

for sr in subroutes:
	sr.flush(pbrstops,pbtimes,pbroutes)

pbsroutes = pbtt.stop_routes
pbtransfers = pbtt.transfers
sroutes = {}

for route in pbroutes:
	for stop in pbrstops[route.stopsidx:route.stopsidx+route.nstops]:
		if stop in sroutes:
			sroutes[stop].append(route.id)
		else:
			sroutes[stop] = [route.id]

routecnt = 0
for stop in pbstops:
	routes = sroutes.get(stop.id,[])
	stop.nroutes=len(routes)
	stop.routeidx=routecnt
	pbsroutes.extend(routes)
	routecnt += len(routes)

	stop.ntransfers = 0
	stop.transferidx = 0


with open("tt.bin","wb") as ttfile:
	ttfile.write(pbtt.SerializeToString())
