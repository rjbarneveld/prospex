#!/usr/bin/env python3
"""Standardise NoData and apply a DEM validity mask to all TIFFs in a folder.
Requires GDAL Python bindings and NumPy. No resampling or cropping occurs.
Example: python3 harmonise_tifs.py ../project ../project/masked_dem.tif
Default: replace input TIFFs in place after writing each corrected file.
Use --output DIRECTORY to write separate copies instead.
All bands are processed. Float64 outputs preserve ordinary integer/float data;
64-bit integer and complex inputs are rejected to avoid silent loss.
"""
import argparse
from pathlib import Path
import sys
import numpy as np
from osgeo import gdal, osr

gdal.UseExceptions()


def invalid(a, nodata):
    bad = ~np.isfinite(a)
    if nodata is not None:
        bad |= np.isnan(a) if np.isnan(nodata) else (a == nodata)
    return bad


def check_grid(ds, ref, path):
    if (ds.RasterXSize, ds.RasterYSize) != (ref.RasterXSize, ref.RasterYSize):
        raise ValueError(f'{path}: dimensions differ from mask DEM')
    a, b = ds.GetGeoTransform(can_return_null=True), ref.GetGeoTransform(can_return_null=True)
    if a is None or b is None or not np.allclose(a, b, rtol=0, atol=1e-7):
        raise ValueError(f'{path}: grid differs from mask DEM; align it first')
    s, r = ds.GetSpatialRef(), ref.GetSpatialRef()
    if s is None or r is None:
        raise ValueError(f'{path}: missing CRS on input or mask DEM')
    s.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)
    r.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)
    if not s.IsSame(r):
        raise ValueError(f'{path}: CRS differs from mask DEM')
    for i in range(1, ds.RasterCount + 1):
        t = gdal.GetDataTypeName(ds.GetRasterBand(i).DataType)
        if t.startswith('C') or t in ('Int64', 'UInt64'):
            raise ValueError(f'{path}: unsupported type {t}')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('folder', type=Path, help='Folder containing TIFFs (not recursive)')
    p.add_argument('dem', type=Path, help='Mask DEM: valid elevations inside, NoData outside')
    p.add_argument('--nodata', type=float, default=-9999, help='Common output NoData (default: -9999)')
    p.add_argument('--output', type=Path, help='Write separate copies here; default overwrites input TIFFs')
    args = p.parse_args()
    if not np.isfinite(args.nodata):
        p.error('--nodata must be finite')
    folder, dem_path = args.folder.resolve(), args.dem.resolve()
    if not folder.is_dir():
        p.error('Input folder does not exist')
    files = sorted(f for f in folder.iterdir() if f.is_file() and f.suffix.lower() in ('.tif', '.tiff'))
    if not files:
        p.error('No TIFF files found')
    out = (args.output or folder).resolve()
    inplace = out == folder
    if not inplace and any((out / f.name).resolve() == dem_path for f in files):
        p.error('Output must not overwrite inputs or mask DEM')
    ref = gdal.Open(str(dem_path))
    if ref is None or ref.RasterCount != 1:
        p.error('Mask DEM must be a readable single-band raster')
    rb = ref.GetRasterBand(1)
    rnd = rb.GetNoDataValue()
    if rnd is None and rb.GetMaskFlags() & gdal.GMF_ALL_VALID:
        p.error('Mask DEM has neither declared NoData nor a validity mask')
    # Validate all grids and destinations before processing.
    for f in files:
        ds = gdal.Open(str(f))
        check_grid(ds, ref, f)
        ds = None
        if not inplace and (out / f.name).exists():
            p.error(f'Output already exists: {out / f.name}; choose a fresh output folder')
    out.mkdir(parents=True, exist_ok=True)
    for f in files:
        src = gdal.Open(str(f))
        target = out / f.name
        temporary = out / (f.name + '.partial.tif')
        dst = None
        total = 0
        try:
            dst = gdal.GetDriverByName('GTiff').Create(str(temporary), src.RasterXSize,
                src.RasterYSize, src.RasterCount, gdal.GDT_Float64,
                options=['COMPRESS=LZW', 'TILED=YES', 'BIGTIFF=IF_SAFER'])
            dst.SetGeoTransform(src.GetGeoTransform())
            dst.SetProjection(src.GetProjection())
            for i in range(1, src.RasterCount + 1):
                sb, db = src.GetRasterBand(i), dst.GetRasterBand(i)
                db.SetNoDataValue(args.nodata)
                db.SetDescription(sb.GetDescription())
                for y in range(0, src.RasterYSize, 256):
                    h = min(256, src.RasterYSize-y)
                    for x in range(0, src.RasterXSize, 512):
                        w = min(512, src.RasterXSize-x)
                        m = rb.ReadAsArray(x,y,w,h)
                        outside = invalid(m,rnd) | (rb.GetMaskBand().ReadAsArray(x,y,w,h) == 0)
                        a = sb.ReadAsArray(x,y,w,h).astype(np.float64)
                        bad = outside | invalid(a,sb.GetNoDataValue()) | (sb.GetMaskBand().ReadAsArray(x,y,w,h) == 0)
                        if np.any((~bad) & (a == args.nodata)):
                            raise ValueError(f'{f}: valid data equals output NoData; select another --nodata')
                        a[bad] = args.nodata
                        db.WriteArray(a,x,y)
                        total += int(bad.sum())
            dst.FlushCache()
            db = None
            dst = None
            src = None
            temporary.replace(target)
            for suffix in ('.msk', '.aux.xml', '.ovr'):
                Path(str(target) + suffix).unlink(missing_ok=True)
            print(f'{f.name}: {total} band-pixels masked -> {target}', flush=True)
        except Exception:
            db = None
            dst = None
            temporary.unlink(missing_ok=True)
            raise
        finally:
            src = None
    print(f'Done: {len(files)} TIFFs. Common NoData={args.nodata}. Grid and extent preserved.')


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        sys.exit(1)
