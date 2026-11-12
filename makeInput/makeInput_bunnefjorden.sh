#!/bin/bash
# chmod +x makeRun_cup04.sh
projectFolder='iobunnefjorden'
dempath=/run/user/1000/gvfs/smb-share\:server\=int.nibio.no\,share\=databank/Forvaltning/hoydemodeller/dtm/cron_corrected_by_dtm1_median_dtm10/dtm10_corrected_from_dtm1_norge.vrt
# echo $vdnr
 vdnr=005.40
 sh ./getClimate02.sh $vdnr $projectFolder
 
 user='sl_robr'
 pw='sl_rbor'
 host='dbmain.int.nibio.no'
 db='sl'
 
 ogr2ogr -f PostgreSQL "PG:host=$host dbname=$db user=$user password=$pw" ../$projectFolder/bunnefjorden_4258.gpkg -nln prosj_hydrotwin.bunnefjorden_4258 -lco GEOMETRY_NAME=geom -nlt PROMOTE_TO_MULTI -progress
 ogr2ogr ../$projectFolder/bunnefjorden_25832_buffer.gpkg ../$projectFolder/bunnefjorden_25832.gpkg -dialect SQLite -sql "SELECT vassdragnr, ST_Buffer(geom,100) AS geo FROM regine"
 gdalwarp -crop_to_cutline -cutline ../$projectFolder/bunnefjorden_25832_dissolved_buffer.gpkg $dempath ../$projectFolder/dtm10.tif
 gdal_calc.py --calc="(A>0)*A" -A ../$projectFolder/dtm10.tif --outfile=../$projectFolder/dtm10.tif --NoDataValue=0 --overwrite;
 conda activate richdem
 python3 fillDEM.py $projectFolder
 conda deactivate
# rm ../$projectFolder/dtm10.tif
 
#in db04
psql  --command="CREATE table prosj_hydrotwin.bunnefjorden_tekstur as SELECT a.figurid, a.tekstur1, a.tekstur2, a.geo FROM org_jordsmonn.jm_harmonisert_flate AS a WHERE EXISTS (SELECT 1 FROM prosj_hydrotwin.bunnefjorden_4258 AS b WHERE ST_Intersects(a.geo, b.geom))" postgresql://${user}:${pw}@${host}/${db}
psql  --command="create table prosj_hydrotwin.bunnefjorden_ar5 as SELECT a.sl_sdeid, a.artype, a.geo FROM org_ar5.ar5_flate AS a WHERE EXISTS (SELECT 1 FROM prosj_hydrotwin.bunnefjorden_4258 AS b WHERE ST_Intersects(a.geo, b.geom))" postgresql://${user}:${pw}@${host}/${db}

ogr2ogr -f GPKG "../$projectFolder/texture.gpkg"   "PG:host=$host dbname=$db user=$user password=$pw" -sql "SELECT a.tekstur1, st_transform(a.geo, 25833) FROM prosj_hydrotwin.bunnefjorden_texture as a" -nln texture -progress
ogr2ogr -f GPKG "../$projectFolder/landuse.gpkg"   "PG:host=$host dbname=$db user=$user password=$pw" -sql "SELECT a.artype, st_transform(a.geo, 25833) FROM prosj_hydrotwin.bunnefjorden_ar5 as a" -nln landuse -progress

#in db04
#lakes
psql  --command="CREATE TABLE prosj_hydrotwin.tmp_lakes AS SELECT a.objectid, a.geo33z FROM grl_sk.fkb_vann_20250820_flate AS a WHERE a.objtype = 'Innsjø' AND a.arealm2 > 5000 AND EXISTS (SELECT 1 FROM prosj_hydrotwin.bunnefjorden_4258 AS b WHERE ST_Intersects(a.geo33z,ST_Transform(b.geom, 25833)));" postgresql://${user}:${pw}@${host}/${db}
psql  --command="CREATE TABLE prosj_hydrotwin.merged_lakes AS WITH clusters AS (SELECT unnest(ST_ClusterIntersecting(geo33z)) AS cluster_geom FROM prosj_hydrotwin.tmp_lakes) SELECT row_number() OVER () AS id,ST_Union(cluster_geom) AS geom FROM clusters GROUP BY cluster_geom;" postgresql://${user}:${pw}@${host}/${db}
psql  --command="DROP TABLE prosj_hydrotwin.tmp_lakes" postgresql://${user}:${pw}@${host}/${db}
ogr2ogr -f GPKG "../$projectFolder/lakes.gpkg" "PG:host=$host dbname=$db user=$user password=$pw" -sql "SELECT * FROM prosj_hydrotwin.merged_lakes"

#property
psql  --command="CREATE TABLE prosj_hydrotwin.bunnefjorden_property AS SELECT a.gardsnummer, a.objectid,a.geo FROM grl_matrikkel_teig.teig_matrikkelnummmer_2021_flate AS a WHERE EXISTS (SELECT 1 FROM prosj_hydrotwin.bunnefjorden_25833 AS b WHERE ST_Intersects(a.geo,b.geom))" postgresql://${user}:${pw}@${host}/${db}
ogr2ogr -f GPKG "../$projectFolder/property.gpkg" "PG:host=$host dbname=$db user=$user password=$pw" -sql "SELECT * FROM prosj_hydrotwin.bunnefjorden_property"

gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/texture.tif --NoDataValue=0;
gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/landuse.tif --NoDataValue=0
gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/vegetation.tif --NoDataValue=0
gdal_calc.py --calc="A>5000" -A ../$projectFolder/flacc.tif --overwrite --outfile=../$projectFolder/river.tif
gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/lakes.tif --NoDataValue=0
gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/property.tif --NoDataValue=0;

gdal_rasterize -a tekstur1 ../$projectFolder/texture.gpkg ../$projectFolder/texture.tif
gdal_rasterize -a artype ../$projectFolder/landuse.gpkg ../$projectFolder/landuse.tif

#fill in missing texture
python3 fill_missing_texture.py ../$projectFolder/landuse.tif ../$projectFolder/texture.tif ../$projectFolder/texture_filled.tif

gdal_rasterize -a ID ../$projectFolder/lakes.gpkg ../$projectFolder/lakes.tif;
gdal_rasterize -a OBJECTID ../$projectFolder/property.gpkg ../$projectFolder/property.tif;
gdal_calc.py -A ../$projectFolder/landuse.tif --calc="(A==30)*2 + ((A>20)&(A<30))*6" --outfile=../$projectFolder/vegetation.tif
gdal_calc.py -A ../$projectFolder/river.tif --calc="A/A" --outfile=../$projectFolder/river.tif

./lakeStats ../$projectFolder
  gdal_calc.py -A ../$projectFolder/lakes.tif --calc="A*(A/A)" --outfile=../$projectFolder/lakes.tif
 ./distToWater_fast ../$projectFolder
 ./groundwater_reference ../$projectFolder
 ./getChannel ../$projectFolder/dtm10_filled.tif ../$projectFolder/channel.tif 1000000
 ./drainRoute ../$projectFolder
 bash select60.sh $xmin $ymin $xmax $ymax $xllmin $yllmin $xllmax $yllmax
 
 #HARMONISE NULL DATA VALUES
# ./mask_geotiffs.sh ../$projectFolder/dtm10_filled.tif
# ./harmoniseNull.sh $projectFolder
 #python3 harmoniseNull.py "../$projectFolder" --files '*.tif'
python3 harmonise_tifs.py "../$projectFolder" "../$projectFolder/dtm10.tif"
 #gdal_calc.py   -A ioskut2/dtm10_filled__.tif   --outfile=ioskut2/dtm10_filled_fixed.tif   --calc="where(A<0,0,A)"   --NoDataValue=0   --type=Float32

 !BUILD STH TO READ TABLE AND CONNECT MEASURES TO CHANGES IN PARAMETERS
 #pprocess selected maps
 #for i in io/erosion*.tif;
 # do gdal_calc.py -A $i --calc="A" --outfile=${i/.tif/_skuterud.tif} --NoDataValue=-9999 --overwrite;
 #done
