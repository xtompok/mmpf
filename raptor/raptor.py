#!/usr/bin/python3

import sys
import csv
import os
import re
import subprocess
import data_pb2
from datetime import datetime
from ctypes import Structure,c_uint32,c_uint64,POINTER,c_void_p,c_char_p,c_int
from ctypes import cdll

MAX_TRANSFERS=8

class stop_arrivals(Structure):
	_fields_ = [("time",c_uint32 * MAX_TRANSFERS),
		("from",c_uint32 * MAX_TRANSFERS),
		("fdep",c_uint32 * MAX_TRANSFERS),
		("route",c_uint32 * MAX_TRANSFERS)]

class mem_data(Structure):
	_fields_ = [("s_arr",POINTER(stop_arrivals))]

raptor = cdll.LoadLibrary("./libraptor.so")

load_timetable = raptor.load_timetable
load_timetable.argtypes = [c_char_p]
load_timetable.restype = c_void_p

search_con = raptor.search_con
search_con.argtypes = [c_void_p,POINTER(mem_data),c_uint32,c_uint32,c_uint64,c_int]

create_mem_data = raptor.create_mem_data
create_mem_data.argtypes = [c_void_p]
create_mem_data.restype = POINTER(mem_data)

clear_mem_data = raptor.clear_mem_data
clear_mem_data.argtypes = [POINTER(mem_data),c_int]

def re_from_name(name):
	parts = name.split(" ")
	return "[^ ]*( |$)".join(parts)

if len(sys.argv) < 3:
	print("Usage: {} from to [time]".format(sys.argv[0]))
	sys.exit(1)

def strtime(time):
	if time != 4294967295:
		return "{:2}:{:02}:{:02}".format(int(time/3600)%24,int(time/60)%60,time%60)
	else:
		return "--:--:--"

def load_routes(filename):
	routes = data_pb2.Routes()
	with open(filename,"rb") as routefile:
		routes.ParseFromString(routefile.read())
	return routes.routes

stops = []
with open("stopslut.csv") as slutfile:
	reader = csv.DictReader(slutfile)
	stops = [s for s in reader]


fstops = list(filter(lambda x:re.match(re_from_name(sys.argv[1]),x["name"]), stops))
tstops = list(filter(lambda x:re.match(re_from_name(sys.argv[2]),x["name"]), stops))


print("From stops: {}".format(fstops))
print("To stops: {}".format(tstops))

fromtime = datetime.now()
if len(sys.argv) == 4:
	(ahour,amin) = map(int,sys.argv[3].split("."))
	fromtime = fromtime.replace(hour=ahour,minute=amin,second = 0)
timestamp = int(fromtime.timestamp())


routes = load_routes("routes.bin")

tt = load_timetable(c_char_p(b"tt.bin"))
print("Loaded")
md = create_mem_data(tt)
clear_mem_data(md,len(stops))
print("Initialized")
search_con(tt,md,int(fstops[0]["raptor_id"]),int(tstops[0]["raptor_id"]),timestamp,7)
print("Found")

print(md.contents.s_arr[int(tstops[0]["raptor_id"])].route[7])
print(md.contents.s_arr[int(tstops[0]["raptor_id"])].time[7])
print(type(md))

for s in stops:
	for r in range(8):
		if md.contents.s_arr[int(s["raptor_id"])].time[r] != 2**32-1 and s != fstops[0]:
			print(s["name"],
				routes[md.contents.s_arr[int(s["raptor_id"])].route[r]].name,
				strtime(md.contents.s_arr[int(s["raptor_id"])].time[r]))
