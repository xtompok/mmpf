DROP TABLE IF EXISTS tmp_shapes;
CREATE TABLE tmp_shapes AS
(
	SELECT pts.id AS shape_id,ST_Makeline(pts.pos) AS shape FROM
	(
		SELECT shape_id AS id, ST_Makepoint(shape_pt_lon,shape_pt_lat) AS pos 
		FROM gtfs_shapes AS shp
		ORDER BY shape_pt_sequence
	) AS pts
	GROUP BY shape_id
);
DROP TABLE IF EXISTS shapes;
CREATE TABLE shapes AS(
	SELECT DISTINCT r.route_id, r.route_short_name, ts.shape_id, ts.shape FROM tmp_shapes AS ts
	INNER JOIN gtfs_trips AS t ON t.shape_id = ts.shape_id
	INNER JOIN gtfs_routes AS r ON r.route_id = t.route_id
);
