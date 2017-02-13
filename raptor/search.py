#!/usr/bin/python3

import sys
import csv
import os
import re
import subprocess
from datetime import datetime

def re_from_name(name):
	parts = name.split(" ")
	return "[^ ]*( |$)".join(parts)

if len(sys.argv) < 3:
	print("Usage: {} from to [time]".format(sys.argv[0]))
	sys.exit(1)


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
seconds = (fromtime - fromtime.replace(hour=0, minute=0, second=0, microsecond=0)).seconds

subprocess.run(["./raptor",fstops[0]["raptor_id"],tstops[0]["raptor_id"],str(seconds)])




