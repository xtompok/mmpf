 DROP TABLE IF EXISTS GTFS_AGENCY CASCADE;
 DROP TABLE IF EXISTS GTFS_ROUTES CASCADE;
 DROP TABLE IF EXISTS GTFS_STOPS CASCADE;
 DROP TABLE IF EXISTS GTFS_TRIPS CASCADE;

 DROP TABLE IF EXISTS GTFS_STOP_TIMES CASCADE;
 DROP TABLE IF EXISTS GTFS_SHAPES CASCADE;
 DROP TABLE IF EXISTS GTFS_CALENDAR CASCADE;
 DROP TABLE IF EXISTS GTFS_CALENDAR_DATES CASCADE;

 --DROP VIEW IF EXISTS tram_stops;

 /*
 agency_id,agency_name,agency_url,agency_timezone,agency_lang,agency_phone
 MTA NYCT,MTA New York City Transit, http://www.mta.info,America/New_York,en,718-330-1234
 */

 CREATE TABLE GTFS_AGENCY
 (
 agency_id VARCHAR(10),
 agency_name VARCHAR(50),
 agency_url VARCHAR(100),
 agency_timezone VARCHAR(50),
 agency_lang VARCHAR(2),
 agency_phone VARCHAR(30)
 );

 /*
 routes.txt
 route_id,agency_id,route_short_name,route_long_name,route_desc,route_type,route_url,route_color,route_text_color
 1,MTA NYCT,1,Broadway - 7 Avenue Local,"Trains operate between 242 St in the Bronx and South Ferry in Manhattan, most times",1,http://www.mta.info/nyct/service/pdf/t1cur.pdf,EE352E,
 */

 CREATE TABLE GTFS_ROUTES
 (
 route_id VARCHAR(10),
 agency_id NUMERIC(3),
 route_short_name VARCHAR(50),
 route_long_name VARCHAR(100),
 route_type NUMERIC(3),
 route_url VARCHAR(100),
 route_color VARCHAR(8),
 route_text_color VARCHAR(8),
 is_night NUMERIC(1)
 );

 CREATE INDEX ON GTFS_ROUTES(route_id);
 /*
 stops.txt
 stop_id,stop_code,stop_name,stop_desc,stop_lat,stop_lon,zone_id,stop_url,location_type,parent_station
 101,,Van Cortlandt Park - 242 St,,40.889248,-73.898583,,,1,
 */

 CREATE TABLE GTFS_STOPS
 ( stop_id VARCHAR(15),
 stop_name VARCHAR(100),
 stop_lat NUMERIC(38,8),
 stop_lon NUMERIC(38,8),
 zone_id VARCHAR(3),
 stop_url VARCHAR(100),
 location_type NUMERIC(2),
 parent_station VARCHAR(15),
 wheelchair_boarding NUMERIC(1),
 level_id VARCHAR(10),
 platform_code VARCHAR(10)
 );

 /*
 trips.txt
 route_id,service_id,trip_id,trip_headsign,direction_id,block_id,shape_id
 1,A20130803WKD,A20130803WKD_000800_1..S03R,SOUTH FERRY,1,,1..S03R
 */
 CREATE TABLE GTFS_TRIPS
 (
 route_id VARCHAR(10),
 service_id VARCHAR(20),
 trip_id VARCHAR(20),
 trip_headsign VARCHAR(50),
 trip_short_name VARCHAR(50),
 direction_id NUMERIC(1),
 block_id VARCHAR(20),
 shape_id VARCHAR(10),
 wheelchair_accessible NUMERIC(1),
 bikes_allowed NUMERIC(1),
 exceptional NUMERIC(10),
 trip_operation_type VARCHAR(10)
 );

 CREATE INDEX ON GTFS_TRIPS(trip_id);
 CREATE INDEX ON GTFS_TRIPS(route_id);


 /*
 stop_times.txt
 trip_id,arrival_time,departure_time,stop_id,stop_sequence,stop_headsign,pickup_type,drop_off_type,shape_dist_traveled
 A20130803WKD_000800_1..S03R,00:08:00,00:08:00,101S,1,,0,0,
 */

 CREATE TABLE GTFS_STOP_TIMES
 (
 trip_id VARCHAR(20),
 arrival_time VARCHAR(8),
 departure_time VARCHAR(8),
 stop_id VARCHAR(15),
 stop_sequence NUMERIC(10),
 stop_headsign VARCHAR(50),
 pickup_type NUMERIC(1),
 drop_off_type NUMERIC(1),
 shape_dist_traveled DOUBLE PRECISION
 );

 CREATE INDEX ON GTFS_STOP_TIMES(trip_id);

 /*
 shapes.txt
 shape_id,shape_pt_lat,shape_pt_lon,shape_pt_sequence,shape_dist_traveled
 4..N06R,40.668897,-73.932942,0,
 */

 CREATE TABLE GTFS_SHAPES
 (
 shape_id VARCHAR(10),
 shape_pt_lat NUMERIC,
 shape_pt_lon NUMERIC,
 shape_pt_sequence NUMERIC(6),
 shape_dist_traveled DOUBLE PRECISION
 );

 /*
 calendar_dates.txt
 service_id,date,exception_type
 */

 CREATE TABLE GTFS_CALENDAR_DATES
 (
 service_id VARCHAR(20),
 exception_date DATE,
 exception_type VARCHAR(10)
 );

 /*
 calendar.txt
 service_id,monday,tuesday,wednesday,thursday,friday,saturday,sunday,start_date,end_date
 A20130803WKD,1,1,1,1,1,0,0,20130803,20141231
 */

 CREATE TABLE GTFS_CALENDAR
 (
 service_id VARCHAR(20),
 monday NUMERIC(1),
 tuesday NUMERIC(1),
 wednesday NUMERIC(1),
 thursday NUMERIC(1),
 friday NUMERIC(1),
 saturday NUMERIC(1),
 sunday NUMERIC(1),
 start_date DATE,
 end_date DATE
 );

CREATE UNIQUE INDEX ON gtfs_stops(stop_id);
CREATE UNIQUE INDEX ON gtfs_routes(route_id);
CREATE UNIQUE INDEX ON gtfs_trips(trip_id);

CREATE INDEX ON gtfs_stop_times(trip_id);
CREATE INDEX ON gtfs_stop_times(stop_id);
CREATE INDEX ON gtfs_trips(route_id);
CREATE INDEX ON gtfs_trips(shape_id);
CREATE INDEX ON gtfs_shapes(shape_id);

--CREATE VIEW tram_stops AS 
--SELECT DISTINCT s.stop_id,s.stop_name 
--FROM gtfs_routes AS r 
--INNER JOIN gtfs_trips AS t ON r.route_id = t.route_id 
--INNER JOIN gtfs_stop_times AS st ON st.trip_id = t.trip_id 
--INNER JOIN gtfs_stops AS s ON s.stop_id = st.stop_id 
--WHERE route_short_name SIMILAR TO '[0-9]{1,2}';

