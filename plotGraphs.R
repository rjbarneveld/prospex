library(ggplot2)
library(lubridate)
oqsource<-read.csv("iolierfossen/oQ.csv",sep=" ",header=T)
oq<-read.csv("iolierfossen/QE0.csv",sep=" ",header=T)
q<-read.csv("iolierfossen/1.200.0-Vannføring-dogn-v1.csv",sep=";",header=T,dec=",")
lake<-read.csv("iolierfossen/lakesOut.csv",header=T,sep=" ")

q$date<-as.Date(q$Tidspunkt,format="%Y-%m-%d")
oqsource$date<-as.Date(oqsource$date,format="%Y-%m-%d")
oq$date<-as.Date(oq$date,format="%Y-%m-%d")
lake$date<-as.Date(lake$date,format="%Y-%m-%d")

startDate="2012-01-01"
endDate="2013-12-31"
ggoqsource <- ggplot() + geom_area(data=oqsource,aes(x=as.Date(date),y=Q_m3d/(24*3600),fill=source),color="black",alpha=.5) + geom_line(data=subset(q,year(date)<=2022),aes(x=date,y=Vannføring),color="red") + xlim(as.Date(startDate),as.Date(endDate)) 
ggoq <- ggplot() + geom_area(data=oq,aes(x=as.Date(date),y=(q_overland + q_drain + q_lake)/(24*3600)),color="black") + geom_line(data=subset(q,year(date)<=2022),aes(x=date,y=Vannføring),color="red") + xlim(as.Date(startDate),as.Date(endDate)) 
#LAKE LEVEL
gglevel <- ggplot(lake) + geom_line(aes(x=date,y=level,group=lake,color=as.factor(lake)))
