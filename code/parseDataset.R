library(lubridate)
library(dplyr)

dfSurveys <- read.csv("dataset/demsurveyMergedCodedDisID.csv")
dfSurveys <- select(dfSurveys, matches("egoid")|matches("completed")|matches("euthanasia")|matches("fssocsec")|matches("fswelfare")|matches("jobguar")|matches("marijuana")|matches("toomucheqrights"))
for(i in 1:nrow(dfSurveys)) {
  if(nchar(dfSurveys[i,]$completed_1) > 0) {
    dfSurveys[i, ]$completed_1 <- as.character(as.POSIXct(dfSurveys[i, ]$completed_1, format="%m/%d/%Y %H:%M"))
  }
}

write.csv(dfSurveys, "dfSurveysBeforeDict.csv", row.names = F)

#euthanasia
i <- 8
euthanasia_choices <- unique(x=c(unique(dfSurveys[,i]), unique(dfSurveys[,i+1]), unique(dfSurveys[,i+2]), unique(dfSurveys[,i+3]), unique(dfSurveys[,i+4]), unique(dfSurveys[,i+5])))

euthanasia_dict <- c(
  "No"=0, 
  "Yes"=1, 
  "Not sure"=NA
)

for(j in i:(i+5)) {
  dfSurveys[,j] <- euthanasia_dict[dfSurveys[,j]]
}

#fsocsec
i <- 14
fsocsec_choices <- unique(x=c(unique(dfSurveys[,i]), unique(dfSurveys[,i+1]), unique(dfSurveys[,i+2]), unique(dfSurveys[,i+3]), unique(dfSurveys[,i+4]), unique(dfSurveys[,i+5])))

fsocsec_dict <- c(
  "Decrease"=0, 
  "Increase"=1,
  "Be kept the same"=NA,
  "Not sure"=NA
)

for(j in i:(i+5)) {
  dfSurveys[,j] <- fsocsec_dict[dfSurveys[,j]]
}

#fswelware
i <- 20
fswelware_choices <- unique(x=c(unique(dfSurveys[,i]), unique(dfSurveys[,i+1]), unique(dfSurveys[,i+2]), unique(dfSurveys[,i+3]), unique(dfSurveys[,i+4]), unique(dfSurveys[,i+5])))

fswelware_dict <- c(
  "Decrease"=0, 
  "Increase"=1,
  "Be kept the same"=NA,
  "Not sure"=NA
)

for(j in i:(i+5)) {
  dfSurveys[,j] <- fswelware_dict[dfSurveys[,j]]
}

#jobguar
i <- 26
jobguar_choices <- unique(x=c(unique(dfSurveys[,i]), unique(dfSurveys[,i+1]), unique(dfSurveys[,i+2]), unique(dfSurveys[,i+3]), unique(dfSurveys[,i+4]), unique(dfSurveys[,i+5])))

jobguar_dict <- c(
  "Govt. See to Job/Living"=0,
  "2"=0,
  "3"=0,
  "4"=NA,
  "5"=1,
  "6"=1,
  "Each Perfon on Own"=1
)

for(j in i:(i+5)) {
  dfSurveys[,j] <- jobguar_dict[dfSurveys[,j]]
}

#marijuana
i <- 32
marijuana_choices <- unique(x=c(unique(dfSurveys[,i]), unique(dfSurveys[,i+1]), unique(dfSurveys[,i+2]), unique(dfSurveys[,i+3]), unique(dfSurveys[,i+4]), unique(dfSurveys[,i+5])))

marijuana_dict <- c(
  "Not Legal"=0, 
  "Legal"=1,
  "Not Sure"=NA
)

for(j in i:(i+5)) {
  dfSurveys[,j] <- marijuana_dict[dfSurveys[,j]]
}

#toomucheqrights
i <- 38
toomucheqrights_choices <- unique(x=c(unique(dfSurveys[,i]), unique(dfSurveys[,i+1]), unique(dfSurveys[,i+2]), unique(dfSurveys[,i+3]), unique(dfSurveys[,i+4]), unique(dfSurveys[,i+5])))

toomucheqrights_dict <- c(
  "Disagree"=0,
  "Strongly disagree"=0,
  "Agree"=1,
  "Strongly agree"=1,
  "Neither agree nor disagree"=NA
)

for(j in i:(i+5)) {
  dfSurveys[,j] <- toomucheqrights_dict[dfSurveys[,j]]
}

write.csv(dfSurveys, "dfSurveysAfterDict.csv", row.names = F)

