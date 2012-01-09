source("include/ContractDb.class.R")
source("config.R")


cdb <- new.ContractDb()
connect(cdb)
load(cdb, CONFIG$symbols)
load(cdb, CONFIG$indexes, index=TRUE)
disconnect(cdb)

message("Found contractDetails, count = ", length(cdb$.Data))

contractDbFile <- CONFIG$contractDb
rm(CONFIG)
save.image(contractDbFile)
message("ContractDb stored in ", contractDbFile)




