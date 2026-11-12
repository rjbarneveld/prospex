projectFolder="$1"
DEM="../$projectFolder/dtm10_filled.tif"

for name in texture landuse vegetation channel lakes groundwaterLevel
do
    f="../$projectFolder/${name}.tif"
    tmp="../$projectFolder/${name}_masked.tif"
    gdal_calc.py \
        -A "$DEM" \
        -B "$f" \
        --calc="where(A==-9999,-9999,B)" \
        --NoDataValue=-9999 \
        --outfile="$tmp"
    mv $tmp $f
done
