
import argparse
from pathlib import Path
import richdem as rd

parser = argparse.ArgumentParser(description="Fill DEM depressions with RichDEM")
parser.add_argument("folder", type=Path, help="Folder containing the input DEM")
args = parser.parse_args()

# --- Build paths ---
folder = args.folder
input_file = ".." / folder / "dtm10.tif"              # keep same filename
output_file = ".." / folder / "dtm10_filled.tif"      # same folder, new name

# --- Process DEM ---
dem = rd.LoadGDAL(str(input_file))
dem_filled = rd.FillDepressions(dem, epsilon=True, in_place=False)
rd.SaveGDAL(str(output_file), dem_filled)
