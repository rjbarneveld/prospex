library(ggplot2)
df<-read.csv("io_aarungen/QE0.csv",sep=" ",header=T)
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland))
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=q_overland))
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland))
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=q_overland))
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=q_drain))
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=q_drain/(50000000)))
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=q_drain/(50000)))
library(lubridate)
aggregate(q_drain~year(as.Date(date)),sum,df)
aggregate(q_drain~year(as.Date(date)),df,sum)
aggregate(q_drain~year(as.Date(date)),df,sum)/1000000
aggregate(q_drain~year(as.Date(date)),df,sum)/50000
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
aggregate(q_drain~year(as.Date(date)),df,sum)/1000000
aggregate(q_drain~year(as.Date(date)),df,sum)/50000
aggregate(q_overland~year(as.Date(date)),df,sum)/50000
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=q_drain))
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=q_overland))
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
head(df)
df
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
head(df)
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland/50000))
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland/5000000))
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland/5000000))
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland/5000000))
aggregate(e_overland~year(as.Date(date)),df,sum)/50000000
aggregate(e_overland~year(as.Date(date)),df,sum)/50000
aggregate(e_overland~year(as.Date(date)),df,sum)/500000
aggregate(e_overland~year(as.Date(date)),df,sum)/50000
aggregate(e_overland~year(as.Date(date)),df,sum)/50
aggregate(e_overland~year(as.Date(date)),df,sum)/50000000
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
aggregate(e_overland~year(as.Date(date)),df,sum)/50000000
aggregate(e_drain~year(as.Date(date)),df,sum)/50000000
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_drain/5000000))
aggregate(e_overland~year(as.Date(date)),df,sum)/500000000
aggregate(e_overland~year(as.Date(date)),df,sum)/5000000000
aggregate(e_overland~year(as.Date(date)),df,sum)/5000000
aggregate(e_overland~year(as.Date(date)),df,sum)/500000000
aggregate(e_overland~year(as.Date(date)),df,sum)
aggregate(e_gully~year(as.Date(date)),df,sum)/500000000
aggregate(e_gully~year(as.Date(date)),df,sum)/50000000
aggregate(e_gully~year(as.Date(date)),df,sum)/50000000000
aggregate(e_gully~year(as.Date(date)),df,sum)/5000000
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_drain/50000000))
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland/50000000))
sum(subset(df$e_overland,year(as.Date(date))))
sum(subset(df$e_overland,year(as.Date(df$date))))
sum(subset(df$e_overland,year(as.Date(df$date))==2012))
head(df)$e_overland
sum(subset(df$e_overland,year(as.Date(df$date))==2012))
sum(subset(df$e_overland,year(as.Date(df$date))==2012))/50000000
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
df
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland/50000000),color="red") + geom_line(aes(y=e_drain/50000000),color="blue")
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=e_overland/50000000),color="red") + geom_line(aes(y=e_drain/50000000),color="blue")
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=100*e_overland/50000000),color="red") + geom_line(aes(y=e_drain/50000000),color="blue")
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
ggplot(df,aes(x=as.Date(date))) + geom_line(aes(y=100*e_overland/50000000),color="red") + geom_line(aes(y=e_drain/50000000),color="blue")
sum(subset(df$e_overland,year(as.Date(df$date))==2012))
aggregate(e_overland~year(as.Date(date)),df,sum)/50000000
aggregate(e_overland~year(as.Date(date)),df,sum)/50000000*100
df1<-read.csv("io_aarungen/QE_1_aarungen_till1_river0.csv",sep=" ",header=T)
head(df1)$e_overland
head(df)$e_overland
aggregate(e_overland~year(as.Date(date)),df1,sum)/50000000*100
aggregate(e_overland~year(as.Date(date)),df,sum)/50000000*100
df<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
aggregate(e_overland~year(as.Date(date)),df,sum)/50000000*100
df0<-read.csv("io_aarungen/QE_0_aarungen_baseline_river0.csv",sep=" ",header=T)
df1<-read.csv("io_aarungen/QE_1_aarungen_till1_river0.csv",sep=" ",header=T)
df2<-read.csv("io_aarungen/QE_1_aarungen_till2_river0.csv",sep=" ",header=T)
df2<-read.csv("io_aarungen/QE_2_aarungen_till2_river0.csv",sep=" ",header=T)
df3<-read.csv("io_aarungen/QE_2_aarungen_till3_river0.csv",sep=" ",header=T)
df3<-read.csv("io_aarungen/QE_3_aarungen_till3_river0.csv",sep=" ",header=T)
df4<-read.csv("io_aarungen/QE_4_aarungen_till4_river0.csv",sep=" ",header=T)
agg0<-aggregate((e_gully + e_drain + e_overland)~year(as.Date(date)),df0,sum)
agg1<-aggregate((e_gully + e_drain + e_overland)~year(as.Date(date)),df1,sum)
agg2<-aggregate((e_gully + e_drain + e_overland)~year(as.Date(date)),df2,sum)
agg3<-aggregate((e_gully + e_drain + e_overland)~year(as.Date(date)),df3,sum)
agg4<-aggregate((e_gully + e_drain + e_overland)~year(as.Date(date)),df4,sum)
names(agg0)
names(agg0)[1]
names(agg0)<-c("dato","sedimenttilførsel")
names(agg1)<-c("dato","sedimenttilførsel")
names(agg2)<-c("dato","sedimenttilførsel")
names(agg3)<-c("dato","sedimenttilførsel")
names(agg4)<-c("dato","sedimenttilførsel")
agg0$scenario<-"høstpløying, vårkorn"
agg1$scenario<-"overvintring i stubb"
agg2$scenario<-"direktesåing"
agg3$scenario<-"strip till"
agg4$scenario<-"redusert jordarbeiding (harving)"
annual<-rbind(agg0,agg1)
annual<-rbind(annual,agg2)
annual<-rbind(annual,agg3)
annual<-rbind(annual,agg4)
daily0<-df0[,c("date","e_gully","e_overland","e_drain")]
daily1<-df1[,c("date","e_gully","e_overland","e_drain")]
daily2<-df2[,c("date","e_gully","e_overland","e_drain")]
daily3<-df3[,c("date","e_gully","e_overland","e_drain")]
daily4<-df4[,c("date","e_gully","e_overland","e_drain")]
daily0$scenario<-"høstpløying, vårkorn"
daily1$scenario<-"overvintring i stubb"
daily2$scenario<-"direktesåing"
daily3$scenario<-"strip till"
daily4$scenario<-"redusert jordarbeiding (harving)"
daily<-rbind(daily0,daily0)
daily<-rbind(daily0,daily1)
daily<-rbind(daily,dayly2)
daily<-rbind(daily,daily2)
daily<-rbind(daily,daily3)
daily<-rbind(daily,daily4)
ggplot(daily) + geom_line(aes(x=as.Date(date),y=e_gully,group=scenario,colour=scenario))
ggplot(annual) + geom_line(aes(x=as.Date(date),y=e_gully,group=scenario,colour=scenario))
head(annual)
ggplot(annual) + geom_line(aes(x=dato,y=e_gully,group=scenario,colour=scenario))
ggplot(annual) + geom_line(aes(x=dato,y=sedimenttilførsel,group=scenario,colour=scenario))
daily$e_tot<-daily$e_overland + daily$e_drain + daily$e_gully
ggplot(daily) + geom_line(aes(x=date,y=e_tot,group=scenario,colour=scenario))
ggplot(daily) + geom_line(aes(x=date,y=e_tot,group=scenario,colour=scenario)) + geom_smooth()
ggplot(daily) + geom_smooth(aes(x=date,y=e_tot,group=scenario,colour=scenario))
ggplot(daily) + geom_smooth(aes(x=as.Date(date),y=e_tot,group=scenario,colour=scenario))
ls()
head(df1)
aggq0<-aggregate((q_drain + q_overland)~year(as.Date(date)),df0,sum)
aggq1<-aggregate((q_drain + q_overland)~year(as.Date(date)),df1,sum)
aggq2<-aggregate((q_drain + q_overland)~year(as.Date(date)),df2,sum)
aggq3<-aggregate((q_drain + q_overland)~year(as.Date(date)),df3,sum)
aggq4<-aggregate((q_drain + q_overland)~year(as.Date(date)),df4,sum)
head(aggq0)
names(aggq1)<-c("dato","avrenning")
names(aggq2)<-c("dato","avrenning")
names(aggq3)<-c("dato","avrenning")
names(aggq4)<-c("dato","avrenning")
names(aggq0)<-c("dato","avrenning")
aggq0$scenario<-"høstpløying, vårkorn"
aggq1$scenario<-"overvintring i stubb"
aggq2$scenario<-"direktesåing"
aggq3$scenario<-"strip till"
aggq4$scenario<-"redusert jordarbeiding (harving)"
annual<-rbind(aggq0,aggq1)
annual<-rbind(annual,agg4)
annual<-rbind(agg0,agg1)
annual<-rbind(annual,agg2)
annual<-rbind(annual,agg3)
annual<-rbind(annual,agg4)
annualq<-rbind(aggq0,aggq1)
annualq<-rbind(annual1,aggq2)
annualq<-rbind(annualq,aggq2)
annualq<-rbind(annualq,aggq3)
annualq<-rbind(annualq,aggq4)
dailyq0<-df0[,c("date","q_overland","q_drain")]
dailyq1<-df1[,c("date","q_overland","q_drain")]
dailyq2<-df2[,c("date","q_overland","q_drain")]
dailyq3<-df3[,c("date","q_overland","q_drain")]
dailyq4<-df4[,c("date","q_overland","q_drain")]
dailyq<-rbind(dailyq0,dailyq1)
dailyq<-rbind(dailyq,dailyq2)
dailyq<-rbind(dailyq,dailyq3)
dailyq<-rbind(dailyq,dailyq4)
dailyq0$scenario<-"høstpløying, vårkorn"d
dailyq0$scenario<-"høstpløying, vårkorn"
dailyq1$scenario<-"overvintring i stubb"
dailyq2$scenario<-"direktesåing"
dailyq3$scenario<-"strip till"
daily4q$scenario<-"redusert jordarbeiding (harving)"
dailyq4$scenario<-"redusert jordarbeiding (harving)"
dailyq<-rbind(dailyq0,dailyq1)
dailyq<-rbind(dailyq,dailyq2)
dailyq<-rbind(dailyq,dailyq3)
dailyq<-rbind(dailyq,dailyq4)
ggplot(dailyq) + geom_line(aes(x=as.Date(date)),y=q_overland + q_drain,colour=scenario,group=scenario))
ggplot(dailyq) + geom_line(aes(x=as.Date(date),y=q_overland + q_drain,colour=scenario,group=scenario))
ggplot(dailyq) + geom_smooth(aes(x=as.Date(date),y=q_overland + q_drain,colour=scenario,group=scenario))
names(annualq)
names(annualq)[1]
names(annualq)[1]<-"date"
ggplot(annualq) + geom_line(aes(x=as.Date(date),y=avrenning,colour=scenario,group=scenario))
ggplot(annualq) + geom_line(aes(x=as.Date(date),y=avrenning,colour=scenario,group=scenario),size=1)
ggplot(annualq) + geom_line(aes(x=as.Date(date),y=avrenning,colour=scenario,group=scenario),size=2)
df1<-read.csv("io_aarungen/QE_1_aarungen_till1_river0.csv",sep=" ",header=T)
agg1<-aggregate((e_gully + e_drain + e_overland)~year(as.Date(date)),df1,sum)
names(agg1)<-c("dato","sedimenttilførsel")
agg1$scenario<-"overvintring i stubb"
annual<-rbind(agg0,agg1)
annual<-rbind(annual,agg2)
annual<-rbind(annual,agg3)
annual<-rbind(annual,agg4)
daily1<-df1[,c("date","e_gully","e_overland","e_drain")]
ggplot(annual) + geom_line(aes(x=as.Date(date),y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(annual) + geom_line(aes(x=as.Date(dato),y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(annualq) + geom_line(aes(x=as.Date(date),y=avrenning,colour=scenario,group=scenario),size=1)
ggplot(annual) + geom_line(aes(x=as.Date(dato),y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(annualq) + geom_line(aes(x=as.Date(date),y=avrenning,colour=scenario,group=scenario),size=1)
ggplot(annual) + geom_line(aes(x=as.Date(dato),y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(annualq) + geom_line(aes(x=as.Date(date),y=avrenning,colour=scenario,group=scenario),size=1)
ggplot(annual) + geom_line(aes(x=as.Date(dato),y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(annualq) + geom_line(aes(x=as.Date(date),y=avrenning,colour=scenario,group=scenario),size=1)
ggplot(annual) + geom_line(aes(x=as.Date(dato),y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(annual) + geom_line(aes(x=dato,y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(subset(annual,scenario!="høstpløying, vårkorn") + geom_line(aes(x=dato,y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
)
ggplot(subset(annual,scenario!="høstpløying, vårkorn")) + geom_line(aes(x=dato,y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(subset(annual,scenario!="overvintring i stubb")) + geom_line(aes(x=dato,y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(subset(annual,(scenario!="overvintring i stubb"&dato>2012))) + geom_line(aes(x=dato,y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(subset(annual,(scenario!="overvintring i stubb"&dato>2012))) + geom_col(aes(x=dato,y=sedimenttilførsel,colour=scenario,group=scenario),size=1)
ggplot(subset(annual,(scenario!="overvintring i stubb"&dato>2012))) + geom_col(aes(x=dato,y=sedimenttilførsel,fill=scenario,group=scenario),size=1)
ggplot(subset(annual,(scenario!="overvintring i stubb"&dato>2012))) + geom_bar(aes(x=dato,y=sedimenttilførsel,fill=scenario,group=scenario),size=1)
ggplot(subset(annual,(scenario!="overvintring i stubb"&dato>2012))) + geom_col(aes(x=dato,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge")
head(annualq)
ggplot(subset(annual,(scenario!="overvintring i stubb"&dato>2012))) + geom_col(aes(x=date,y=avrenning,fill=scenario,group=scenario),size=1,position="dodge")
ggplot(subset(annualq,(scenario!="overvintring i stubb"&dato>2012))) + geom_col(aes(x=date,y=avrenning,fill=scenario,group=scenario),size=1,position="dodge")
ggplot(subset(annualq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning,fill=scenario,group=scenario),size=1,position="dodge")
aggq0<-aggregate((q_drain + q_overland)~year(as.Date(date)),df0,max)
aggq1<-aggregate((q_drain + q_overland)~year(as.Date(date)),df1,max)
aggq2<-aggregate((q_drain + q_overland)~year(as.Date(date)),df2,max)
aggq3<-aggregate((q_drain + q_overland)~year(as.Date(date)),df3,max)
aggq4<-aggregate((q_drain + q_overland)~year(as.Date(date)),df4,max)
aggq0$scenario<-"høstpløying, vårkorn"
aggq1$scenario<-"overvintring i stubb"
aggq2$scenario<-"direktesåing"
aggq3$scenario<-"strip till"
aggq4$scenario<-"redusert jordarbeiding (harving)"
head(aggq0)
names(aggq0)<-c("date","avrenning","scenario"
)
names(aggq1)<-c("date","avrenning","scenario")
names(aggq2)<-c("date","avrenning","scenario")
names(aggq3)<-c("date","avrenning","scenario")
names(aggq4)<-c("date","avrenning","scenario")
annualMaxq<-rbind(aggq0,aggq1)
annualMaxq<-rbind(annualMaxq,aggq2)
annualMaxq<-rbind(annualMaxq,aggq3)
annualMaxq<-rbind(annualMaxq,aggq4)
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning,fill=scenario,group=scenario),size=1,position="dodge")
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning(50000000,fill=scenario,group=scenario),size=1,position="dodge")
)
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning(50000000,fill=scenario,group=scenario),size=1,position="dodge")
savehistor("makeGraphs.R")
savehistory("makeGraphs.R")
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning,fill=scenario,group=scenario),size=1,position="dodge")
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning/50000000,fill=scenario,group=scenario),size=1,position="dodge")
head(df0)
max(df0$q_stream)
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning/50000,fill=scenario,group=scenario),size=1,position="dodge")
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning/5000,fill=scenario,group=scenario),size=1,position="dodge")
climate<-read.csv("io_aarungen/climate.csv",header=T,sep=" ")
head(climate)
climate<-read.csv("io_aarungen/climate.csv",header=T,sep="\t")
head(climate)
climate<-read.csv("io_aarungen/climate.csv",header=F,sep="\t")
names(climate)<-c("index","year","month","day","T","rr")
head(climate)
climate$date<-as.Date(paste(climate$year,climate$month,climate$day,sep"-"))
climate$date<-as.Date(paste(climate$year,climate$month,climate$day,sep="-"))
head(climate)
annualRR<-aggregate(rr~year(date),climate,sum)
names(annualRR)
names(annualRR)[1]<-"date"
names(annualRR)[2]<-"nedbær"
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning/50000,fill=scenario,group=scenario),size=1,position="dodge")
head(annualMaxq)
merge(annualMaxq,annualRR,by="date")
annualMaxq<-merge(annualMaxq,annualRR,by="date")
annualq<-merge(annualq,annualRR,by="date")
annual<-merge(annual,annualRR,by="date")
head(annual)
names(annual)
names(annual)[1]
names(annual)[1]<-"date"
annual<-merge(annual,annualRR,by="date")
head(annualMaxq)
annualq$runoffratio<-(annualq$avrenning/48000000)/annualq$nedbær
head(annualMaxq)
head(annualq)
annualq$runoffratio<-(annualq$avrenning/480000)/annualq$nedbær
head(annualq)
annualq$runoffratio<-(annualq$avrenning/4800000)/annualq$nedbær
head(annualq)
annualq$runoffratio<-(annualq$avrenning/48000)/annualq$nedbær
head(annualq)
annualMaxq$runoffratio<-(annualMaxq$avrenning/4800000)/annualMaxq$nedbær
head(annualMaxq)
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=runoffratio,fill=scenario,group=scenario),size=1,position="dodge")
ggplot(subset(annualMaxq,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=runoffratio,fill=scenario,group=scenario),color="black",size=1,position="dodge")
savehistory("plotGraphs.R")
aggMaxOverland0<-aggregate((q_overland)~year(as.Date(date)),df0,max)
aggMaxOverland1<-aggregate((q_overland)~year(as.Date(date)),df1,max)
aggMaxOverland2<-aggregate((q_overland)~year(as.Date(date)),df2,max)
aggMaxOverland3<-aggregate((q_overland)~year(as.Date(date)),df3,max)
aggMaxOverland4<-aggregate((q_overland)~year(as.Date(date)),df4,max)

aggMaxOverland0$scenario<-"høstpløying, vårkorn"
aggMaxOverland1$scenario<-"overvintring i stubb"
aggMaxOverland2$scenario<-"direktesåing"
aggMaxOverland3$scenario<-"strip till"
aggMaxOverland4$scenario<-"redusert jordarbeiding (harving)"

names(aggMaxOverland0)<-c("date","overflateavrenning","scenario")
names(aggMaxOverland1)<-c("date","overflateavrenning","scenario")
names(aggMaxOverland2)<-c("date","overflateavrenning","scenario")
names(aggMaxOverland3)<-c("date","overflateavrenning","scenario")
names(aggMaxOverland4)<-c("date","overflateavrenning","scenario")

annualMaxOverland<-rbind(aggMaxOverland0,aggMaxOverland1)
annualMaxOverland<-rbind(aggMaxOverland,aggMaxOverland2)
annualMaxOverland<-rbind(annualMaxOverland,aggMaxOverland2)
annualMaxOverland<-rbind(annualMaxOverland,aggMaxOverland3)
annualMaxOverland<-rbind(annualMaxOverland,aggMaxOverland4)
head(annualMaxOverland)
ggplot(subset(annualMaxOverland,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=runoffratio,fill=scenario,group=scenario),color="black",size=1,position="dodge")
annualMaxOverland<-merge(annualMaxOverland,annualRR,by="date")
annualMaxq$runoffratio<-(annualMaxq$avrenning/4800000)/annualMaxq$nedbær
ggplot(subset(annualMaxOverland,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=runoffratio,fill=scenario,group=scenario),color="black",size=1,position="dodge")
annualMaxOverland$runoffratio<-(annualMaxOverland$avrenning/4800000)/annualMaxOverland$nedbær
head(annualMaxOverland)
annualMaxOverland$runoffratio<-(annualMaxOverland$overflateavrenning/4800000)/annualMaxOverland$nedbær
ggplot(subset(annualMaxOverland,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=runoffratio,fill=scenario,group=scenario),color="black",size=1,position="dodge")
head(annualMaxOveraldn)
head(annualMaxOverland)
ggplot(subset(annualMaxOverland,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=overflateavrenning,fill=scenario,group=scenario),color="black",size=1,position="dodge")
ggplot(subset(annual,(scenario!="overvintring i stubb"&dato>2012))) + geom_col(aes(x=date,y=avrenning,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=avrenning,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_segment(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=int(date),y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=integer(date),y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=as.integer(date),y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=as.integer(date),y=sedimenttilførsel/500,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=as.integer(date),y=sedimenttilførsel/500,fill=scenario,group=scenario),size=1,position="dodge",color="black") + ylabel("Sedimenttilførsel (kg/daa)")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=as.integer(date),y=sedimenttilførsel/500,fill=scenario,group=scenario),size=1,position="dodge",color="black") + ylab("Sedimenttilførsel (kg/daa)")
typeof(annual$date)
annual$date<-as.integer(annual$date)
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black")
typeof(annual$date)
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black")
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) 
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw()
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18)
ggplot(subset(annual,(scenario!="overvintring i  stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18)
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18)
ggplot(subset(annual,(scenario!="overvintring i  stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18)
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=sedimenttilførsel,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18)
head(annual)
annual$reference<-ifelse(date==2012,89220,ifelse(date==))
subset(annual,scenario=="høstpløying, vårkorn")
annual$reference<-ifelse(date==2012,89220,ifelse(date==2013,20714,""))
annual$reference<-ifelse(annual$date==2012,89220,ifelse(annual$date==2013,20714,""))
subset(annual,scenario=="høstpløying, vårkorn")
annual$reference<-ifelse(annual$date==2012,89220,ifelse(annual$date==2013,20714,ifelse(annual$date==2014,31240,ifelse(annual$date==2015,25799,ifelse(annual$date==2016,14587,ifelse(annual$date==2017,28844,ifelse(annual$date==2018,31517,"")))))))
subset(annual,scenario=="høstpløying, vårkorn")
annual$reference<-ifelse(annual$date==2012,89220,ifelse(annual$date==2013,20714,ifelse(annual$date==2014,31240,ifelse(annual$date==2015,25799,ifelse(annual$date==2016,14587,ifelse(annual$date==2017,28844,ifelse(annual$date==2018,31517,ifelse(annual$date==2019,43033,ifelse(annual$date==2020,36085,ifelse(annual$date==2021,23064,""))))))))))
annual$reduksjon<-(annual$reference-annual$sedimenttilførsel)/annual$reference
annual$reduksjon<-(annual$reference-annual$sedimenttilførsel)
annual$reduksjon<-(annual$reference-1)
head(annual)
annual$reference*2
typeof(annual$reference)
annual$reference<-ifelse(annual$date==2012,89220,ifelse(annual$date==2013,20714,ifelse(annual$date==2014,31240,ifelse(annual$date==2015,25799,ifelse(annual$date==2016,14587,ifelse(annual$date==2017,28844,ifelse(annual$date==2018,31517,ifelse(annual$date==2019,43033,ifelse(annual$date==2020,36085,ifelse(annual$date==2021,23064,0))))))))))
typeof(annual$reference)
annual$reduksjon<-(annual$reference-annual$sedimenttilførsel)/annual$reference
ggplot(subset(annual,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=reduksjon,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18) + ylab("reduksjon sedimenttilførsel")
ggplot(subset(annual,(scenario!="overvintring i stubb"scenario!="høstpløying, vårkorn"&date>2012))) + geom_col(aes(x=date,y=reduksjon,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18) + ylab("reduksjon sedimenttilførsel")
ggplot(subset(annual,(scenario!="overvintring i stubb"&scenario!="høstpløying, vårkorn"&date>2012))) + geom_col(aes(x=date,y=reduksjon,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18) + ylab("reduksjon sedimenttilførsel")
png("reduksjonAarlig.png",1200,500)
ggplot(subset(annual,(scenario!="overvintring i stubb"&scenario!="høstpløying, vårkorn"&date>2012))) + geom_col(aes(x=date,y=reduksjon,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 18) + ylab("reduksjon sedimenttilførsel")
dev.off()
png("reduksjonAarlig.png",1200,500)
ggplot(subset(annual,(scenario!="overvintring i stubb"&scenario!="høstpløying, vårkorn"&date>2012))) + geom_col(aes(x=date,y=reduksjon,fill=scenario,group=scenario),size=1,position="dodge",color="black") + scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon sedimenttilførsel")
dev.off()
reduksjon<-read.csv("io_aarungen/reduksjon.csv",sep="\t",header=T)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon))
reduksjon<-read.csv("io_aarungen/reduksjon.csv",sep="\t",header=T)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon))
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=posisjon,fill=posisjon))
ggplot(reduksjon) + geom_col(aes(x=måned,y=reduksjon,group=posisjon,fill=posisjon))
ggplot(reduksjon) + geom_col(aes(x=måned,y=reduksjon,group=posisjon,fill=posisjon))+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon sedimenttilførsel")
ggplot(reduksjon) + geom_col(aes(x=måned,y=reduksjon,group=posisjon,fill=posisjon))+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
ggplot(reduksjon) + geom_col(aes(x=måned,y=reduksjon,group=posisjon,fill=posisjon),position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
ggplot(reduksjon) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
ggplot(subset(reduksjon,posisjon="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
head(reduksjon)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
ggplot(subset(reduksjon,posisjon=="bunn")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
ggplot(subset(reduksjon,posisjon=="midt")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
ggplot(subset(reduksjon,posisjon=="midt")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black";position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
ggplot(subset(reduksjon,posisjon=="midt")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 24) + ylab("reduksjon jordtap")
ggplot(subset(reduksjon,posisjon=="midt")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap")
png("reduksjon_topp.png",450,450)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap")
dev.off()
png("reduksjon_topp.png",600,600)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap")
dev.off()
png("reduksjon_topp.png",600,600)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
dev.off()
png("reduksjon_topp.png",600,800)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
dev.off()
png("reduksjon_topp.png",800,600)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
dev.off()
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(2012,2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
head(reduksjon)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c("jan","feb","mar","apr","mai","jun","jul","aug","sep","okt","nov","des")) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_discrete(name = " ", breaks = c("jan","feb","mar","apr","mai","jun","jul","aug","sep","okt","nov","des")) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=as.factor(måned),y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_discrete(name = " ", breaks = c("jan","feb","mar","apr","mai","jun","jul","aug","sep","okt","nov","des")) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none")
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none") + ylim(0,1.5)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none") + ylim(0,1.5) + title("asdf")
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none") + ylim(0,1.5) + labs(title("topp"))
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 32) + ylab("reduksjon jordtap") + guides(fill="none") + ylim(0,1.5) + labs(title="topp")
png("reduksjon_topp.png",1200,600)
ggplot(subset(reduksjon,posisjon=="topp")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 42) + ylab("reduksjon jordtap") + guides(fill="none") + ylim(0,1.5) + labs(title="topp")
dev.off()
png("reduksjon_midt.png",1200,600)
ggplot(subset(reduksjon,posisjon=="midt")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 42) + ylab("reduksjon jordtap") + guides(fill="none") + ylim(0,1.5) + labs(title="topp")
dev.off()
png("reduksjon_bunn.png",1200,600)
ggplot(subset(reduksjon,posisjon=="bunn")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 42) + ylab("reduksjon jordtap") + guides(fill="none") + ylim(0,1.5) + labs(title="bunn")
dev.off()
png("reduksjon_midt.png",1200,600)
ggplot(subset(reduksjon,posisjon=="midt")) + geom_col(aes(x=måned,y=reduksjon,group=scenario,fill=scenario),color="black",position="dodge")+ scale_x_continuous(name = " ", breaks = c(1,2,3,4,5,6,7,8,9,10,11,12)) + theme_bw(base_size = 42) + ylab("reduksjon jordtap") + guides(fill="none") + ylim(0,1.5) + labs(title="midt")
dev.off()
ggplot(subset(annualMaxOverland,(scenario!="overvintring i stubb"&date>2012))) + geom_col(aes(x=date,y=overflateavrenning,fill=scenario,group=scenario),color="black",size=1,position="dodge")
savehistory("plotGraphs.R")
