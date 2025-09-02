#!/bin/bash
# chmod +x makeRun_cup04.sh
projectFolder='iolierfossen'
dempath=/run/user/1000/gvfs/smb-share\:server\=int.nibio.no\,share\=databank/Forvaltning/hoydemodeller/dtm/cron_corrected_by_dtm1_median_dtm10/dtm10_corrected_from_dtm1_norge.vrt
# echo $vdnr
 vdnr=001.K4A #Lierfossen
 sh ./getClimate02.sh $vdnr $projectFolder

 user='sl_robr'
 pw='sl_rbor'
 host='dbmain.int.nibio.no'
 db='sl'
 
 gdal_translate -a_nodata 0 ../$projectFolder/dtm10_.tif ../$projectFolder/dtm10.tif
 rm ../$projectFolder/dtm10_.tif
 conda activate richdem
 python3 fillDEM.py
 conda deactivate
 rm ../$projectFolder/dtm10.tif
 
#create bounding boxex
# ogrinfo -so io/loc_id.gpkg sql_statement | awk '$1 ~/^ *Extent/' | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' > corners.txt
 ogr2ogr -a_srs EPSG:25833 -t_srs EPSG:4258 -f "GPKG" ../$projectFolder/nbfelt/nbfelt_4258.gpkg ../$projectFolder/nbfelt/nbfelt_lierfossen_25833.gpkg
 ogrinfo ../$projectFolder/nbfelt/*.gpkg -sql "ALTER TABLE nbfelt_lierfossen_25833 RENAME TO nbfelt"
 ogrinfo -so ../$projectFolder/nbfelt/nbfelt_lierfossen_25833.gpkg nbfelt | awk '$1 ~/^ *Extent/' | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' > corners.txt
 xmin=`sed '1q;d' corners.txt`
 ymin=`sed '2q;d' corners.txt`
 xmax=`sed '3q;d' corners.txt`
 ymax=`sed '4q;d' corners.txt`
 rm corners.txt

 ogrinfo -so ../$projectFolder/nbfelt/nbfelt_4258.gpkg nbfelt | awk '$1 ~/^ *Extent/' | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' > corners_ll.txt
 xllmin=`sed '1q;d' corners_ll.txt`
 yllmin=`sed '2q;d' corners_ll.txt`
 xllmax=`sed '3q;d' corners_ll.txt`
 yllmax=`sed '4q;d' corners_ll.txt`
 rm corners_ll.txt
 rm ../$projectFolder/nbfelt/nbfelt_4258.gpkg
#download shapefiles land use, texture, water and field (property) boundaries
 pgsql2shp -f "../$projectFolder/texture" -h $host -u $user -P $pw $db "SELECT tekstur1,tekstur2,geo FROM org_jordsmonn.jm_harmonisert_flate as a where a.geo && ST_MakeEnvelope('$xllmin','$yllmin','$xllmax','$yllmax',4258)"
 pgsql2shp -f "../$projectFolder/ar5" -h $host -u $user -P $pw $db "SELECT artype,geo FROM org_ar5.ar5_flate as a where a.geo && ST_MakeEnvelope('$xllmin','$yllmin','$xllmax','$yllmax',4258)"
 pgsql2shp -f "../$projectFolder/elv" -h $host -u $user -P $pw $db "SELECT objectid,objtype,geo33 FROM (select objectid,geo33,objtype from grl_sk.fkb_vann_20250820_linje where objtype IN ('ElvBekk','Elvekant')) as a where a.geo33 && ST_MakeEnvelope('$xmin','$ymin','$xmax','$ymax',25833)"
 pgsql2shp -f "../$projectFolder/vann" -h $host -u $user -P $pw $db "SELECT objectid,geo33z FROM grl_sk.fkb_vann_20211209_flate as a where a.geo33z && ST_MakeEnvelope('$xmin','$ymin','$xmax','$ymax',4258)"
 innsjoe='Innsjø'
 #pgsql2shp -f "../$projectFolder/lakes25833" -h $host -u $user -P $pw $db "SELECT objectid,geo33z FROM grl_sk.fkb_vann_20250820_flate as a where a.objtype = '$innsjoe' AND a.arealm2 > 5000 AND a.geo33z && ST_MakeEnvelope('$xmin','$ymin','$xmax','$ymax',25833)"
 
#CREATE AND MERGE ADJACENT LAKES
 psql  --command="CREATE TABLE sl_robr.tmp_lakes AS SELECT * from (SELECT objectid,geo33z FROM grl_sk.fkb_vann_20250820_flate as a where a.objtype = '$innsjoe' AND a.arealm2 > 5000 AND a.geo33z && ST_MakeEnvelope('$xmin','$ymin','$xmax','$ymax',25833))" postgresql://${user}:${pw}@${host}/${db}
 psql  --command="CREATE TABLE sl_robr.merged_lakes AS WITH clusters AS (SELECT unnest(ST_ClusterIntersecting(geo33z)) AS cluster_geom FROM sl_robr.tmp_lakes) SELECT row_number() OVER () AS id,ST_Union(cluster_geom) AS geom FROM clusters GROUP BY cluster_geom;" postgresql://${user}:${pw}@${host}/${db}
 psql  --command="DROP TABLE sl_robr.tmp_lakes;" postgresql://${user}:${pw}@${host}/${db}
 pgsql2shp -f "../$projectFolder/lakes25833" -h $host -u $user -P $pw $db "SELECT * FROM sl_robr.merged_lakes"
 psql  --command="DROP TABLE sl_robr.merged_lakes;" postgresql://${user}:${pw}@${host}/${db}

 pgsql2shp -f "../$projectFolder/property25833" -h $host -u $user -P $pw $db "SELECT gardsnummer,objectid,geo FROM grl_matrikkel_teig.teig_matrikkelnummmer_2021_flate as a where a.geo && ST_MakeEnvelope('$xmin','$ymin','$xmax','$ymax',25833)" 
 
 ogr2ogr -a_srs EPSG:4258 -t_srs EPSG:25833 -f "ESRI Shapefile" ../$projectFolder/texture25833.shp ../$projectFolder/texture.shp; rm ../$projectFolder/texture.*
 ogr2ogr -a_srs EPSG:4258 -t_srs EPSG:25833 -f "ESRI Shapefile" ../$projectFolder/ar525833.shp ../$projectFolder/ar5.shp; rm ../$projectFolder/ar5.*
 ogr2ogr -a_srs EPSG:4258 -t_srs EPSG:25833 -f "ESRI Shapefile" ../$projectFolder/vann25833.shp ../$projectFolder/vann.shp; rm ../$projectFolder/vann.*
 ogr2ogr -a_srs EPSG:4258 -t_srs EPSG:25833 -f "ESRI Shapefile" ../$projectFolder/elv25833.shp ../$projectFolder/elv.shp; rm ../$projectFolder/elv.*
 
 gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/texture.tif --NoDataValue=0;
 gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/landuse.tif --NoDataValue=0
 gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/vegetation.tif --NoDataValue=0
 gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/river.tif --NoDataValue=0
 gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/lakes.tif --NoDataValue=0
 gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/property.tif --NoDataValue=0;
 gdal_calc.py --calc="0" -A ../$projectFolder/dtm10_filled.tif --outfile=../$projectFolder/measures.tif --NoDataValue=0;
 
 gdal_rasterize -a tekstur1 ../$projectFolder/texture25833.shp ../$projectFolder/texture.tif; rm ../$projectFolder/texture25833.*
 gdal_rasterize -a artype ../$projectFolder/ar525833.shp ../$projectFolder/landuse.tif; rm ../$projectFolder/ar525833.*	
 gdal_rasterize -a OBJECTID ../$projectFolder/elv2.shp ../$projectFolder/river.tif; rm ../$projectFolder/elv25833.*
 gdal_rasterize -a OBJECTID ../$projectFolder/vann25833.shp ../$projectFolder/vann.tif; rm ../$projectFolder/vann25833.*
 gdal_rasterize -a ID ../$projectFolder/lakes25833.shp ../$projectFolder/lakes.tif; rm ../$projectFolder/lakes25833.*
 gdal_rasterize -a OBJECTID ../$projectFolder/property25833.shp ../$projectFolder/property.tif; rm ../$projectFolder/property25833.*
#assign vegetation classes for vegetation growth
 gdal_calc.py -A ../$projectFolder/landuse.tif --calc="(A==30)*2 + ((A>20)&(A<30))*6" --outfile=../$projectFolder/vegetation.tif
 gdal_calc.py -A ../$projectFolder/river.tif --calc="A/A" --outfile=../$projectFolder/river.tif
 ./lakeStats ../$projectFolder
 ./distToWater ../$projectFolder
 ./groundwater ../$projectFolder
 
  bash select60.sh $xmin $ymin $xmax $ymax $xllmin $yllmin $xllmax $yllmax
  !BUILD STH TO READ TABLE AND CONNECT MEASURES TO CHANGES IN PARAMETERS
 #pprocess selected maps
 #for i in io/erosion*.tif;
 # do gdal_calc.py -A $i --calc="A" --outfile=${i/.tif/_skuterud.tif} --NoDataValue=-9999 --overwrite;
 #done
done < regine_1_utvalg.csv
