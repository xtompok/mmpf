#!/usr/bin/python3

import sys
import csv
import psycopg2
import psycopg2.extras
import data_pb2
from datetime import date
import calendar 
import datetime
import math

class Subroute(object):
	maxid = 0
	stop_seqs = {}

	def __init__(self,stimes,service_id,name=None):
		stops = tuple(map(lambda x:stoplut[x["stop_id"]], stimes))
		service_id = int(service_id)
		pbtimes = []
		for stop in stimes:
			pbtime = data_pb2.StopTime()
			pbtime.arrival = time2sec(stop["arrival_time"])
			pbtime.departure = time2sec(stop["departure_time"])
			pbtimes.append(pbtime) 

		if stops in self.stop_seqs:
			self.stop_seqs[stops].append(pbtimes,service_id)
			self.valid = False
		else:
			self.new(pbtimes,service_id,stops,name)
			self.stop_seqs[stops] = self
			self.valid = True

		
	def new(self,pbtimes,service_id,stops,name):
		self.pbroute = data_pb2.Route()
		self.pbroute.id = Subroute.maxid
		self.pbroute.ntrips = 1
		self.pbroute.nstops = len(stops)
		if name:
			self.pbroute.name = name

		Subroute.maxid += 1

		self.stops = stops
		self.triptimes = [pbtimes]
		self.services = [service_id]

	def append(self,pbtimes,service_id):
		self.triptimes.append(pbtimes)
		self.services.append(service_id)
		self.pbroute.ntrips += 1
	
	def flush(self,rstops,trips,routes,services):
		self.pbroute.stopsidx = len(rstops)
		self.pbroute.tripsidx = len(trips)
		self.pbroute.servicesidx = len(services)
		rstops.extend(self.stops)
		trips_services = [(trip,service) for trip,service in zip(self.triptimes,self.services)]
		trips_services.sort(key=lambda x:x[0][0].departure)
		for triptime,service_id in trips_services:
			trips.extend(triptime)
			services.extend([service_id])
		routes.extend([self.pbroute])

def cal2validity(val,week,exc):
	weekdays = ["monday","tuesday","wednesday","thursday","friday","saturday","sunday"]
	val.start = calendar.timegm(week["start_date"].timetuple())
	val.end = calendar.timegm(week["end_date"].timetuple())
	oneday = datetime.timedelta(days=1)
	curday = week["start_date"]
	days = []
	while curday <= week["end_date"]:
		days.append(int(week[weekdays[curday.weekday()]]))
		curday += oneday
	# TODO: Handle exceptions
	#print(exc)
	val.bitmap = list2bitstring(days)


def list2bitstring(l):
	bs = []
	while len(l)%8 != 0:
		l.append(0)
	for i in range(int(len(l)/8)):
		a = l[8*i+0] | l[8*i+1]<<1 | l[8*i+2]<<2 | l[8*i+3]<<3 | \
			l[8*i+4]<<4 | l[8*i+5]<<5 | l[8*i+6]<<6 | l[8*i+7]<<7
		bs.append(a)
	return bytes(bs)

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

# Stops
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

#Validities
cur.execute("SELECT * FROM gtfs_calendar ORDER BY service_id::INT;")
services = cur.fetchall()
pbvalidities = pbtt.validities
servlut = {}
servilut = {}
for (newid,service) in enumerate(services):
	servlut[service["service_id"]]=newid
	servilut[newid]=service["service_id"]
	cur.execute("SELECT * FROM gtfs_calendar_dates WHERE service_id = %s;",(service["service_id"],))
	exceptions = cur.fetchall()
	cal2validity(pbvalidities.add(),service,exceptions)
print("Service LUT",servlut)
	

# Routes		
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
		#print("Trip {},serivce_id {}".format(trip["trip_id"],trip["service_id"]));
		sr = Subroute(stimes,servlut[trip["service_id"]],route["route_short_name"])
		if sr.valid:
			subroutes.append(sr)


pbtimes = pbtt.stop_times
pbroutes = pbtt.routes
pbrstops = pbtt.route_stops
pbvals = pbtt.trip_validity

for sr in subroutes:
	sr.flush(pbrstops,pbtimes,pbroutes,pbvals)

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
