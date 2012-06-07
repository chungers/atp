#ifndef R_IB_EM_H
#define R_IB_EM_H

#include <Rcpp.h>


/*
 * IBrokers order object:
> sort(names(o))
 [1] "account"                       "action"
 [3] "algoParams"                    "algoStrategy"
 [5] "allOrNone"                     "auctionStrategy"
 [7] "auxPrice"                      "basisPoints"
 [9] "basisPointsType"               "blockOrder"
[11] "clearingAccount"               "clearingIntent"
[13] "clientId"                      "continuousUpdate"
[15] "delta"                         "deltaNeutralAuxPrice"
[17] "deltaNeutralOrderType"         "designatedLocation"
[19] "discretionaryAmt"              "displaySize"
[21] "eTradeOnly"                    "faGroup"
[23] "faMethod"                      "faPercentage"
[25] "faProfile"                     "firmQuoteOnly"
[27] "goodAfterTime"                 "goodTillDate"
[29] "hidden"                        "lmtPrice"
[31] "minQty"                        "nbboPriceCap"
[33] "notHeld"                       "ocaGroup"
[35] "ocaType"                       "openClose"
[37] "orderId"                       "orderRef"
[39] "orderType"                     "origin"
[41] "outsideRTH"                    "overridePercentageConstraints"
[43] "parentId"                      "percentOffset"
[45] "permId"                        "referencePriceType"
[47] "rule80A"                       "scaleInitLevelSize"
[49] "scalePriceIncrement"           "scaleSubsLevelSize"
[51] "settlingFirm"                  "shortSaleSlot"
[53] "startingPrice"                 "stockRangeLower"
[55] "stockRangeUpper"               "stockRefPrice"
[57] "sweepToFill"                   "tif"
[59] "totalQuantity"                 "trailStopPrice"
[61] "transmit"                      "triggerMethod"
[63] "volatility"                    "volatilityType"
[65] "whatIf"
*/
RcppExport SEXP api_place_order(SEXP connectionHandle,
                                SEXP orderList,
                                SEXP contractList);

RcppExport SEXP api_cancel_order(SEXP connectionHandle,
                                 SEXP orderId);

#endif // R_IB_EM_H
