library(lubridate)
q<-read.csv("q_jova.csv",sep="\t",header=T,na.strings="NA")
oq<-read.csv("QE_0_skuterud_baseline.csv",sep=" ",header=T)
q$date<-as.Date(q$date,format="%d.%m.%Y")
oq$date<-as.Date(oq$date,format="%Y-%m-%d")

startNSE=as.Date("2014-01-01")
endNSE=as.Date("2018-12-31")
set <- as.data.frame(((subset(oq,(date>=startNSE&date<=endNSE))$q_overland + subset(oq,(date>=startNSE&date<=endNSE))$q_drain + subset(oq,(date>=startNSE&date<=endNSE))$q_lake)*1000)/(24*3600))
names(set)<-"model"
set$observed <- subset(q,(date>=startNSE&date<=endNSE))$q_ls
set$date <- subset(q,(date>=startNSE&date<=endNSE))$date
set$yearmonth<-as.Date(paste(year(set$date),month(set$date),"15",sep="-"))
av_model <- mean(set$model)
av_observed <- mean(set$observed)
denominator <- sum((set$observed-av_observed)**2)
enumerator <- sum((set$observed-set_model)**2)
NSE_daily <- 1 - enumerator/denominator

monthly<-aggregate(.~yearmonth,set,sum)
