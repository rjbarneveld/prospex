#!/usr/bin/env bash
set -euo pipefail

# Mask all GeoTIFFs in the DEM folder with the DEM NoData mask.
# Files are overwritten in place via a temporary file.
# Existing values inside the DEM domain are left unchanged.
# Multi-band rasters are supported.
#
# Usage:
#   ./mask_geotiffs_inplace.sh /path/to/dem.tif [nodata]
#
# Example:
#   ./mask_geotiffs_inplace.sh ../ioskut2/dtm10_filled.tif -9999

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: $0 /path/to/dem.tif [nodata]" >&2
    exit 1
fi

DEM=$(readlink -f "$1")
NODATA="${2:--9999}"
FOLDER=$(dirname "$DEM")

if [[ ! -f "$DEM" ]]; then
    echo "ERROR: DEM not found: $DEM" >&2
    exit 1
fi

python_with_gdal=""
for candidate in python3 /usr/bin/python3; do
    if command -v "$candidate" >/dev/null 2>&1 && \
       "$candidate" -c 'from osgeo import gdal; import numpy' >/dev/null 2>&1; then
        python_with_gdal=$(command -v "$candidate")
        break
    fi
done

if [[ -z "$python_with_gdal" ]]; then
    echo "ERROR: no Python interpreter with GDAL and NumPy found." >&2
    exit 1
fi

"$python_with_gdal" - "$DEM" "$NODATA" <<'PY'
import math
import os
import sys
from pathlib import Path

import numpy as np
from osgeo import gdal

gdal.UseExceptions()

dem_path = Path(sys.argv[1]).resolve()
nodata_out = float(sys.argv[2])
folder = dem_path.parent

if not math.isfinite(nodata_out):
    raise SystemExit("Output NoData must be finite.")

# Read DEM and build one authoritative mask.
dem_ds = gdal.Open(str(dem_path), gdal.GA_ReadOnly)
if dem_ds is None:
    raise SystemExit(f"Could not open DEM: {dem_path}")

dem_band = dem_ds.GetRasterBand(1)
dem = dem_band.ReadAsArray().astype(np.float64)
dem_nodata = dem_band.GetNoDataValue()

mask = ~np.isfinite(dem)
if dem_nodata is not None:
    if math.isnan(float(dem_nodata)):
        mask |= np.isnan(dem)
    else:
        mask |= dem == float(dem_nodata)

# Also respect GDAL's mask band if one exists.
mask_band = dem_band.GetMaskBand()
if mask_band is not None:
    mask_values = mask_band.ReadAsArray()
    if mask_values is not None:
        mask |= mask_values == 0

width = dem_ds.RasterXSize
height = dem_ds.RasterYSize
gt = dem_ds.GetGeoTransform()
projection = dem_ds.GetProjection()

driver = gdal.GetDriverByName("GTiff")

files = sorted({
    p.resolve()
    for pattern in ("*.tif", "*.tiff", "*.TIF", "*.TIFF")
    for p in folder.glob(pattern)
    if p.is_file()
})

for src_path in files:
    # The DEM already defines the mask; don't rewrite it.
    if src_path == dem_path:
        print(f"[SKIP] {src_path.name} (reference DEM)")
        continue

    src = gdal.Open(str(src_path), gdal.GA_ReadOnly)
    if src is None:
        raise RuntimeError(f"Could not open {src_path}")

    if src.RasterXSize != width or src.RasterYSize != height:
        raise RuntimeError(f"{src_path.name}: grid size differs from DEM")
    if src.GetGeoTransform() != gt:
        raise RuntimeError(f"{src_path.name}: geotransform differs from DEM")
    if src.GetProjection() != projection:
        raise RuntimeError(f"{src_path.name}: CRS differs from DEM")

    tmp = src_path.with_name(f".{src_path.name}.masktmp.tif")
    if tmp.exists():
        tmp.unlink()

    # Float32 safely represents -9999 and matches the model's usual raster inputs.
    dst = driver.Create(
        str(tmp), width, height, src.RasterCount, gdal.GDT_Float32,
        options=["TILED=YES", "COMPRESS=DEFLATE", "PREDICTOR=3", "BIGTIFF=IF_SAFER"]
    )
    if dst is None:
        raise RuntimeError(f"Could not create temporary file for {src_path.name}")

    dst.SetGeoTransform(gt)
    dst.SetProjection(projection)
    if src.GetMetadata():
        dst.SetMetadata(src.GetMetadata())

    for b in range(1, src.RasterCount + 1):
        sb = src.GetRasterBand(b)
        db = dst.GetRasterBand(b)

        values = sb.ReadAsArray().astype(np.float32)
        values[mask] = np.float32(nodata_out)

        db.WriteArray(values)
        db.SetNoDataValue(nodata_out)

        if sb.GetDescription():
            db.SetDescription(sb.GetDescription())
        if sb.GetUnitType():
            db.SetUnitType(sb.GetUnitType())

        db.FlushCache()

    dst.FlushCache()
    dst = None
    src = None

    os.replace(tmp, src_path)
    print(f"[MASKED] {src_path.name} ({int(mask.sum())} cells set to {nodata_out:g})")

dem_ds = None
print("[DONE] All matching GeoTIFFs masked in place.")
PY

