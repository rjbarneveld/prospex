#!/bin/bash
#GET 60% OF AGRICULTURAL AREA
user='sl_robr'
pw='sl_rbor'
host='dbmain.int.nibio.no'
db='sl'
 
psql --command="create table current_teig as SELECT gardsnummer,objectid,geo FROM grl_matrikkel_teig.teig_matrikkelnummmer_2021_flate as a where a.geo && ST_MakeEnvelope('$1','$2','$3','$4',25833);"  postgresql://${user}:${pw}@${host}/${db}
psql --command="create table current_agric as SELECT sl_sdeid,ST_TRANSFORM(geo,25833) FROM org_ar5.ar5_flate as a where artype in (21,22,23) AND a.geo && ST_MakeEnvelope('$5','$6','$7','$8',4258);"  postgresql://${user}:${pw}@${host}/${db}

psql --command="CREATE TABLE all_lines AS SELECT St_ExteriorRing(geo) AS geo FROM current_teig UNION ALL SELECT St_ExteriorRing(st_transform) AS the_geom FROM current_agric;" postgresql://${user}:${pw}@${host}/${db}
psql --command="CREATE TABLE noded_lines AS SELECT St_Union(geo) AS geo FROM all_lines;" postgresql://${user}:${pw}@${host}/${db}
psql --command="CREATE TABLE new_polys (id serial PRIMARY KEY, the_geom geometry);" postgresql://${user}:${pw}@${host}/${db}
psql --command="INSERT INTO new_polys (the_geom) SELECT geom AS the_geom FROM St_Dump((SELECT St_Polygonize(geo) AS geo FROM noded_lines));" postgresql://${user}:${pw}@${host}/${db}
psql --command="CREATE TABLE new_polys_pip AS SELECT id, ST_PointOnSurface(the_geom) AS the_geom FROM new_polys;" postgresql://${user}:${pw}@${host}/${db}
psql --command="CREATE TABLE pip_with_attributes AS SELECT a.id, f.sl_sdeid, c.gardsnummer FROM new_polys_pip a LEFT JOIN current_agric f ON St_Within(a.the_geom, f.st_transform) LEFT JOIN current_teig c ON St_Within(a.the_geom, c.geo);" postgresql://${user}:${pw}@${host}/${db}
psql --command="CREATE TABLE new_fields AS SELECT * FROM new_polys a LEFT JOIN pip_with_attributes b USING (id);" postgresql://${user}:${pw}@${host}/${db}
psql --command="DELETE FROM new_fields WHERE sl_sdeid IS NULL;" postgresql://${user}:${pw}@${host}/${db}
psql --command="ALTER TABLE new_fields ADD column area numeric;" postgresql://${user}:${pw}@${host}/${db}
psql --command="UPDATE new_fields SET area = st_area(the_geom);" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE current_agric;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE current_teig;" postgresql://${user}:${pw}@${host}/${db}
psql --command="ALTER TABLE new_fields ADD COLUMN fid SERIAL PRIMARY KEY;" postgresql://${user}:${pw}@${host}/${db}
psql --command="\copy (SELECT gardsnummer,fid,area FROM sl_robr.new_fields) TO 'current_gnrArea.csv' CSV HEADER;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE all_lines;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE noded_lines;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE new_polys;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE new_polys_pip;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE pip_with_attributes;" postgresql://${user}:${pw}@${host}/${db}

Rscript selectAgricArea.R
rm current_gnrArea.csv
psql --command="create table selection65 (fid integer, selected integer);" postgresql://${user}:${pw}@${host}/${db}
psql --command="\COPY selection65(fid,selected) FROM 'selectionArea65.csv' DELIMITER ',' CSV HEADER;" postgresql://${user}:${pw}@${host}/${db}
rm selectionArea65.csv

psql --command="CREATE TABLE selectedfields as select the_geom,selected from new_fields as nf inner join selection65 as s on nf.fid = s.fid;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE new_fields;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DROP TABLE selection65;" postgresql://${user}:${pw}@${host}/${db}
psql --command="DELETE from selectedfields where selected = 0;" postgresql://${user}:${pw}@${host}/${db}

pgsql2shp -f "io/selected65" -h dbmain.int.nibio.no -u $user -P sl_rbor sl "SELECT the_geom,selected FROM sl_robr.selectedfields"
psql --command="DROP TABLE selectedfields;" postgresql://${user}:${pw}@${host}/${db}

gdal_calc.py --calc="0" -A io/dtm10_filled.tif --outfile=io/selected65.tif
gdal_rasterize -a SELECTED io/selected65.shp  io/selected65.tif
rm io/selected65.cpg
rm io/selected65.dbf
rm io/selected65.prj
rm io/selected65.shp
rm io/selected65.shx
