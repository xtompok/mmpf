#!/bin/sh

cd $(dirname $0)/jrcache
NAME=`date +"jr_%Y-%m-%d_%H-%M.zip"`
wget -O $NAME http://data.pid.cz/PID_GTFS.zip
unzip -o $NAME


FILES="agency calendar calendar_dates routes shapes stops trips stop_times"

for f in $FILES
do
	echo "$f..."
	psql gtfs_praha -c "TRUNCATE TABLE gtfs_$f;"
	cat $f.txt | iconv -f utf-8 -t utf-8 -c |  psql gtfs_praha -c "COPY gtfs_$f FROM STDIN WITH CSV DELIMITER ',' HEADER;"
	psql gtfs_praha -c "ANALYZE gtfs_$f;"
done
cd ..
psql gtfs_praha < GTFS_shapes.sql

./prepare.py
