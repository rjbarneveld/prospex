#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: get_raster_extent.sh raster.tif" >&2
  exit 1
fi

raster="$1"

gdalinfo -json "$raster" | python3 -c '
import sys, json

j = json.load(sys.stdin)

# Projected/native raster coordinates
cc = j["cornerCoordinates"]

xs = [
    cc["upperLeft"][0],
    cc["upperRight"][0],
    cc["lowerRight"][0],
    cc["lowerLeft"][0]
]

ys = [
    cc["upperLeft"][1],
    cc["upperRight"][1],
    cc["lowerRight"][1],
    cc["lowerLeft"][1]
]

xmin = min(xs)
xmax = max(xs)
ymin = min(ys)
ymax = max(ys)

# WGS84 lon/lat coordinates
coords = j["wgs84Extent"]["coordinates"][0]

lons = [p[0] for p in coords]
lats = [p[1] for p in coords]

xllmin = min(lons)
xllmax = max(lons)
yllmin = min(lats)
yllmax = max(lats)

print(xmin, ymin, xmax, ymax, xllmin, xllmax, yllmin, yllmax)
'
