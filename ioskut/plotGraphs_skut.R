library(ggplot2)
library(lubridate)
oqsource<-read.csv("oQ.csv",sep=" ",header=T)
oq<-read.csv("QE0.csv",sep=" ",header=T)
q<-read.csv("q_jova.csv",sep="\t",header=T,na.strings="NA")
lake<-read.csv("lakesOut.csv",header=T,sep=" ")
lupoints<-read.csv("lu_points_timeseries0.csv",sep=" ",header=T)

q$date<-as.Date(q$date,format="%d.%m.%Y")
oqsource$date<-as.Date(oqsource$date,format="%Y-%m-%d")
oq$date<-as.Date(oq$date,format="%Y-%m-%d")
lake$date<-as.Date(lake$date,format="%Y-%m-%d")
lupoints$date<-as.Date(lupoints$date,format="%Y-%m-%d")

startDate=as.Date("2016-01-01")
endDate=as.Date("2018-12-31")
ggoqsource <- ggplot() + geom_area(data=oqsource,aes(x=as.Date(date),y=(Q_m3d*1000)/(24*3600),fill=source),color="black",alpha=.5) + geom_line(data=subset(q,year(date)<=2022),aes(x=date,y=q_ls),color="red") + xlim(startDate,endDate)
ggoq <- ggplot() + geom_area(data=oq,aes(x=as.Date(date),y=(1000*(q_overland + q_drain + q_lake))/(24*3600)),color="black") + geom_line(data=subset(q,year(date)<=2022),aes(x=date,y=q_ls),color="red") + xlim(startDate,endDate)
gglupoints <- ggplot(subset(lupoints,parameter=="infil")) + geom_line(aes(date,value,group=land_use,color=land_use))
#LAKE LEVEL
gglevel <- ggplot(lake) + geom_line(aes(x=date,y=level,group=lake,color=as.factor(lake)))

startNSE=as.Date("2014-01-01")
endNSE=max(oq$date)
set_model <- ((subset(oq,(date>=startNSE&date<=endNSE))$q_overland + subset(oq,(date>=startNSE&date<=endNSE))$q_drain + subset(oq,(date>=startNSE&date<=endNSE))$q_lake)*1000)/(24*3600)
set_observed <- subset(q,(date>=startNSE&date<=endNSE))$q_ls
av_model <- mean(set_model)
av_observed <- mean(set_observed)
denominator <- sum((set_observed-av_observed)**2)
enumerator <- sum((set_observed-set_model)**2)
NSE <- 1 - enumerator/denominator
