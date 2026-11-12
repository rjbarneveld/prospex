#!/usr/bin/env python3
"""Fill depressions and compute D8 accumulation; preserve input NoData/mask.
Usage: python3 fillDEM.py PROJECT_FOLDER
As in the original script, PROJECT_FOLDER is relative to the parent directory.
Requires richdem, rasterio and numpy.
"""
import argparse
from pathlib import Path
import numpy as np
import rasterio
import richdem as rd


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path)
    args = parser.parse_args()
    folder = Path('..') / args.folder
    with rasterio.open(folder / 'dtm10.tif') as src:
        if src.count != 1:
            raise ValueError('Expected a single-band DEM')
        nodata = src.nodata
        if nodata is None:
            raise ValueError('Input DEM has no declared NoData value')
        data = src.read(1, masked=True)
        values = data.data.astype('float64')
        invalid = np.ma.getmaskarray(data) | ~np.isfinite(values)
        if invalid.all():
            raise ValueError('DEM has no valid cells')
        profile = src.profile.copy()

    # RichDEM uses a finite internal sentinel; restore original NoData on export.
    internal_nodata = -np.finfo(np.float64).max
    if np.any(values[~invalid] == internal_nodata):
        raise ValueError('Valid DEM value collides with internal NoData sentinel')
    values[invalid] = internal_nodata
    dem = rd.rdarray(values, no_data=internal_nodata)
    filled = rd.FillDepressions(dem, epsilon=True, in_place=False)
    accumulation = rd.FlowAccumulation(filled, method='D8')
    profile.update(driver='GTiff', count=1, dtype='float64', nodata=nodata,
                   compress='lzw', BIGTIFF='IF_SAFER')
    for name, result in [('dtm10_filled.tif', filled), ('flacc.tif', accumulation)]:
        output = np.asarray(result, dtype='float64').copy()
        if np.any((~invalid) & ((~np.isfinite(output)) | (output == internal_nodata))):
            raise ValueError(f'{name}: invalid computed values inside valid DEM area')
        output[invalid] = nodata
        with rasterio.open(folder / name, 'w', **profile) as dst:
            dst.write(output, 1)
        print(f'{folder / name}: NoData={nodata}; original mask/grid preserved')


if __name__ == '__main__':
    main()
