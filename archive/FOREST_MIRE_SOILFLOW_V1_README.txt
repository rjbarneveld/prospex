FOREST/MIRE SOIL FLOW V1
========================

Purpose
-------
A standalone daily subsurface module for forest (land-use 30) and mire
(land-use 60). It does not use drainRoute and runs whether dynamicG is 0 or 1.

Files
-----
1. soil_waterbalance_daily_v2_forest_mire_handoff.inc
   This is the earlier daily-v2 soil balance with one minimal change:
   lower-profile drainage is not removed from forest/mire cells. ET, rainfall,
   EHD storage and EHD -> SD percolation are unchanged.

2. forest_mire_soilflow_v1.inc
   Moves forest/mire water through SD and a persistent saturated store.

Daily order
-----------
#include "include/soil_waterbalance_daily_v2_forest_mire_handoff.inc"

if(dynamicG == 1)
    #include "include/lateral_subsurface_v2_no_forest_mire.inc"  // or add guards

#include "include/forest_mire_soilflow_v1.inc"
#include "include/flacc_DAG_v2.inc"

IMPORTANT WHEN dynamicG == 1
----------------------------
The general update_soil_layers and lateral_subsurface routines must skip
land-use 30 and 60, because forest_mire_soilflow_v1 owns those cells.
At the beginning of their land-cell loops add:

    if (landuse[x][y] == 30 || landuse[x][y] == 60)
        continue;

Also exclude forest/mire as receiving land targets in the general lateral
subsurface routine. Channels and lakes remain valid sinks.

Concept
-------
EHD is unchanged and is not routed laterally in v1.

EHD -> SD:
    handled by the ordinary soil-water balance.

SD mobile water above field capacity:
    small vertical fraction -> saturated store;
    large lateral fraction -> neighbouring forest/mire, adjacent channel/lake.

Saturated store:
    persistent between days;
    local head reconstructed from storage and specific yield;
    daily dG recalculated from local heads;
    routes to lower forest/mire heads and adjacent channels/lakes;
    above-surface storage becomes dsc exfiltration.

No drainRoute references exist in the forest/mire module.

Initial parameters
------------------
fmSpecificYield              = 0.20
fmResponseDepthM             = 2.0 m
fmVerticalMultiplier         = 0.10
fmUnsatLateralMultiplier     = 10.0
fmSaturatedLateralMultiplier = 10.0

Diagnostic
----------
forest_mire_soilflow_<scenario>_<gid>.csv

The mass_residual_m3 column should remain near zero. Check annual trends in
sat_store_m3, mean_dtg_m, sat_channel_m3 and exfiltration_m3 before calibration.
