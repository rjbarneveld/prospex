library(dplyr)
df<-na.omit(read.csv("current_gnrArea.csv",header=T))
df<-df[with(df, order(gardsnummer, area)),]
df<-as.data.frame(df %>% group_by(gardsnummer) %>% mutate(cum_sum = cumsum(area)))
areaSums<-aggregate(area~gardsnummer,df,sum)
names(areaSums)[2]<-"tot"
df<-merge(df,areaSums,by="gardsnummer")
df$pct<-as.integer(100*df$cum_sum/df$tot)
df$selected<-ifelse(df$pct<65,1,0)
write.table(file="selectionArea65.csv",df[,c("fid","selected")],quote=F,row.names=F,sep=",")
