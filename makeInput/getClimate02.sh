#!/bin/bash
#while read var;
#do
var=$1
folder=$2
echo $var
grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2012_.csv > climate12.csv
awk '{for(i=NF-729;i<=NF;i++) printf $i" "; print ""}' climate12.csv > climate12_.csv
sed -i 's/"//g' climate12_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate12_.csv | xargs -n 1 > rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate12_.csv | xargs -n 1 > tm.csv
rm climate12*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2013_.csv > climate13.csv
awk '{for(i=NF-729;i<=NF;i++) printf $i" "; print ""}' climate13.csv > climate13_.csv
sed -i 's/"//g' climate13_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate13_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate13_.csv | xargs -n 1 >> tm.csv
rm climate13*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2014_.csv > climate14.csv
awk '{for(i=NF-729;i<=NF;i++) printf $i" "; print ""}' climate14.csv > climate14_.csv
sed -i 's/"//g' climate14_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate14_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate14_.csv | xargs -n 1 >> tm.csv
rm climate14*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2015_.csv > climate15.csv
awk '{for(i=NF-729;i<=NF;i++) printf $i" "; print ""}' climate15.csv > climate15_.csv
sed -i 's/"//g' climate15_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate15_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate15_.csv | xargs -n 1 >> tm.csv
rm climate15*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2016_.csv > climate16.csv
awk '{for(i=NF-729;i<=NF;i++) printf $i" "; print ""}' climate16.csv > climate16_.csv
sed -i 's/"//g' climate16_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate16_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate16_.csv | xargs -n 1 >> tm.csv
rm climate16*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2017_.csv > climate17.csv
awk '{for(i=NF-729;i<=NF;i++) printf $i" "; print ""}' climate17.csv > climate17_.csv
sed -i 's/"//g' climate17_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate17_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate17_.csv | xargs -n 1 >> tm.csv
rm climate17*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2018_.csv > climate18.csv
awk '{for(i=NF-729;i<=NF;i++) printf $i" "; print ""}' climate18.csv > climate18_.csv
sed -i 's/"//g' climate18_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate18_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate18_.csv | xargs -n 1 >> tm.csv
rm climate18*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2019_.csv > climate19.csv
sed -i 's/\b[[:alpha:]]*'-'[[:digit:]]*[[:alpha:]]*\b/"GZ 0"/g' climate19.csv
awk '{for(i=NF-728;i<=NF;i++) printf $i" "; print ""}' climate19.csv > climate19_.csv
sed -i 's/"//g' climate19_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate19_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate19_.csv | xargs -n 1 >> tm.csv
rm climate19*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2020_.csv > climate20.csv
awk '{for(i=NF-730;i<=NF;i++) printf $i" "; print ""}' climate20.csv > climate20_.csv
sed -i 's/"//g' climate20_.csv
awk '{for(i=1;i<=366;i++) printf $i" "; print ""}' climate20_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-365;i<=NF;i++) printf $i" "; print ""}' climate20_.csv | xargs -n 1 >> tm.csv
rm climate20*.csv

grep "$var" ~/nibio/Gully2020/model/climate/regine_c_2021_.csv > climate21.csv
awk '{for(i=NF-729;i<=NF;i++) printf $i" "; print ""}' climate21.csv > climate21_.csv
sed -i 's/"//g' climate21_.csv
awk '{for(i=1;i<=365;i++) printf $i" "; print ""}' climate21_.csv | xargs -n 1 >> rr.csv
awk '{for(i=NF-364;i<=NF;i++) printf $i" "; print ""}' climate21_.csv | xargs -n 1 >> tm.csv
rm climate21*.csv

paste dates.csv tm.csv rr.csv > ../$2/climate.csv
rm tm.csv
rm rr.csv

