#!/usr/bin/env python3

import argparse
import sys

import numpy as np
import rasterio
from scipy.ndimage import distance_transform_edt


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Fill missing texture values in agricultural land-use cells "
            "(classes 20–23) using the nearest agricultural cell with "
            "a valid texture value."
        )
    )
    parser.add_argument("landuse", help="Land-use GeoTIFF")
    parser.add_argument("texture", help="Texture GeoTIFF")
    parser.add_argument("output", help="Output texture GeoTIFF")
    parser.add_argument(
        "--zero-is-nodata",
        action="store_true",
        help="Also treat texture value 0 as missing.",
    )
    args = parser.parse_args()

    with rasterio.open(args.landuse) as lu_src, rasterio.open(args.texture) as tx_src:
        if lu_src.width != tx_src.width or lu_src.height != tx_src.height:
            raise ValueError("Land-use and texture rasters have different dimensions.")

        if lu_src.transform != tx_src.transform:
            raise ValueError("Land-use and texture rasters have different transforms.")

        if lu_src.crs != tx_src.crs:
            raise ValueError("Land-use and texture rasters have different CRS definitions.")

        landuse = lu_src.read(1)
        texture = tx_src.read(1)
        texture_nodata = tx_src.nodata
        profile = tx_src.profile.copy()

    agricultural = (landuse >= 20) & (landuse <= 23)

    if texture_nodata is None:
        if np.issubdtype(texture.dtype, np.floating):
            missing_texture = np.isnan(texture)
            output_nodata = -9999.0
        else:
            raise ValueError(
                "Texture raster has no NoData value. "
                "Set one first or modify the script with the missing value."
            )
    else:
        if np.isnan(texture_nodata):
            missing_texture = np.isnan(texture)
        else:
            missing_texture = texture == texture_nodata

        output_nodata = texture_nodata

    if np.issubdtype(texture.dtype, np.floating):
        missing_texture |= np.isnan(texture)

    if args.zero_is_nodata:
        missing_texture |= texture == 0

    # Cells that need filling.
    target = agricultural & missing_texture

    # Valid donors must themselves be agricultural cells.
    donor = agricultural & ~missing_texture

    n_target = int(np.count_nonzero(target))
    n_donor = int(np.count_nonzero(donor))

    print(f"Agricultural cells requiring texture: {n_target}")
    print(f"Valid agricultural donor cells:       {n_donor}")

    if n_target == 0:
        print("Nothing to fill.")
        filled = texture.copy()
    else:
        if n_donor == 0:
            raise ValueError("No valid agricultural texture cells were found.")

        # distance_transform_edt finds the nearest zero cell.
        # Donor cells are therefore encoded as zero.
        non_donor = ~donor

        pixel_width = abs(profile["transform"].a)
        pixel_height = abs(profile["transform"].e)

        _, nearest_indices = distance_transform_edt(
            non_donor,
            sampling=(pixel_height, pixel_width),
            return_indices=True,
        )

        nearest_y = nearest_indices[0]
        nearest_x = nearest_indices[1]

        filled = texture.copy()
        filled[target] = texture[
            nearest_y[target],
            nearest_x[target],
        ]

    profile.update(
        count=1,
        nodata=output_nodata,
        compress="deflate",
        predictor=2,
        tiled=False,
    )

    with rasterio.open(args.output, "w", **profile) as dst:
        dst.write(filled, 1)

    remaining = agricultural & (
        np.isnan(filled)
        if np.issubdtype(filled.dtype, np.floating)
        else np.zeros(filled.shape, dtype=bool)
    )

    if output_nodata is not None:
        if np.isnan(output_nodata):
            remaining |= agricultural & np.isnan(filled)
        else:
            remaining |= agricultural & (filled == output_nodata)

    if args.zero_is_nodata:
        remaining |= agricultural & (filled == 0)

    print(f"Cells filled:                         {n_target}")
    print(
        "Agricultural cells still missing:    "
        f"{int(np.count_nonzero(remaining))}"
    )
    print(f"Written: {args.output}")

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)
