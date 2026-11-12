#!/usr/bin/env python3
"""Harmonise model NoData and DEM mask without resampling.
Requires numpy and rasterio. Outputs go to PROJECT/harmonised by default.
Example: python3 harmoniseNull.py ../project
Use --files '*.tif' to select all project TIFFs (quote the wildcard).
The DEM is always included. Inputs are never overwritten.
"""
import argparse
from pathlib import Path
import numpy as np
import rasterio


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('project', type=Path, help='Actual project directory')
    p.add_argument('--dem', default='dtm10_filled.tif')
    p.add_argument('--files', nargs='+', default=['texture.tif', 'landuse.tif',
                   'vegetation.tif', 'channel.tif', 'lakes.tif', 'groundwaterLevel.tif'])
    p.add_argument('--nodata', type=float, default=-9999)
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    if not np.isfinite(args.nodata):
        p.error('--nodata must be finite')
    root = args.project.resolve()
    dem_path = root / args.dem
    paths = [dem_path]
    for pattern in args.files:
        matches = sorted(root.glob(pattern))
        if not matches:
            p.error(f'No files match {pattern}')
        paths.extend(matches)
    paths = list(dict.fromkeys(x.resolve() for x in paths))
    out = (args.output or root / 'harmonised').resolve()
    if len({x.name for x in paths}) != len(paths):
        p.error('Input filenames must be unique')
    if any((out / x.name).resolve() == x for x in paths):
        p.error('Output directory must differ from input directory')
    out.mkdir(parents=True, exist_ok=True)
    if any((out / x.name).exists() for x in paths):
        p.error('Output files already exist; choose a fresh --output directory')
    with rasterio.open(dem_path) as dem:
        # Check every grid before writing anything. Never silently resample.
        for path in paths:
            with rasterio.open(path) as src:
                if (src.width, src.height, src.crs) != (dem.width, dem.height, dem.crs) or not np.allclose(tuple(src.transform), tuple(dem.transform), rtol=0, atol=1e-7):
                    raise ValueError(f'{path}: grid/CRS differs from DEM')
        for path in paths:
            target = out / path.name
            tmp = out / (path.stem + '.partial.tif')
            try:
                with rasterio.open(path) as src:
                    profile = src.profile.copy()
                    # Float64 accommodates negative NoData for byte rasters
                    # and preserves all ordinary model integer/float values.
                    if any(np.dtype(d).kind in 'iu' and np.dtype(d).itemsize == 8 for d in src.dtypes):
                        raise ValueError('64-bit integer inputs require an explicit conversion policy')
                    profile.update(driver='GTiff', dtype='float64', nodata=args.nodata,
                                   compress='lzw', BIGTIFF='IF_SAFER')
                    changed = 0
                    with rasterio.open(tmp, 'w', **profile) as dst:
                        for _, w in dem.block_windows(1):
                            d = dem.read(1, window=w, masked=True)
                            outside = np.ma.getmaskarray(d) | ~np.isfinite(d.data)
                            for band in range(1, src.count + 1):
                                a = src.read(band, window=w, masked=True)
                                values = a.data.astype('float64')
                                bad = outside | np.ma.getmaskarray(a) | ~np.isfinite(values)
                                changed += int(bad.sum())
                                values[bad] = args.nodata
                                dst.write(values, band, window=w)
                tmp.replace(target)
                print(f'{path.name}: {changed} band-pixels set to NoData -> {target}')
            except Exception:
                tmp.unlink(missing_ok=True)
                raise
    print(f'Done. All outputs use NoData={args.nodata}; extent and grid preserved.')

if __name__ == '__main__':
    main()
