
#include <iostream>
#include <glog/logging.h>

#include "utils.hpp"
#include "ib/tick_types.hpp"
#include "ib/TickerMap.hpp"
#include "varz/varz.hpp"
#include "ApiImpl.hpp"

DEFINE_VARZ_int64(api_events, 0, "");
DEFINE_VARZ_int64(api_requests, 0, "");
DEFINE_VARZ_int64(api_last_ts, 0, "");
DEFINE_VARZ_int64(api_event_interval_micros, 0, "");
DEFINE_VARZ_int64(api_event_marketdata_process_micros, 0, "");


// Macro for writing field value.
#define __f__(m) "," << #m << '=' << m

#define LOG_EVENT                                          \
  boost::uint64_t t = now(); VARZ_api_events++;            \
  VARZ_api_event_interval_micros = t - VARZ_api_last_ts;   \
  VARZ_api_last_ts = t; EWRAPPER_LOGGER                 \
  << "cid=" << connection_id_                           \
  << ",ts_utc=" << t                                    \
  << ",event=" << __func__                              \

// Macro for recording certain event handling latencies
#define TIME_EVENT_MARKETDATA \
  VARZ_api_event_marketdata_process_micros = now() - t;


// Macro for tick type enum to string conversion
#define __tick_type_enum(m) ",field=" << TickTypeNames[m]

namespace ib {
namespace internal {

LoggingEWrapper::LoggingEWrapper() : TimeTracking() {
  VARZ_api_last_ts = now();
}

LoggingEWrapper::~LoggingEWrapper() {
}

void LoggingEWrapper::set_connection_id(const unsigned int id)
{
  connection_id_ = id;
  EWRAPPER_LOGGER << "Connection id updated to " << id;
}

int LoggingEWrapper::get_connection_id() {
  return connection_id_;
}

using ib::internal::PrintContract;

////////////////////////////////////////////////////////////////////////////////
// EWrapper Methods
//
void LoggingEWrapper::tickPrice(TickerId tickerId, TickType field,
                                double price, int canAutoExecute) {
  LOG_EVENT
      << __f__(tickerId)
      << __tick_type_enum(field)
      << __f__(price)
      << __f__(canAutoExecute);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::tickSize(TickerId tickerId, TickType field, int size) {
  LOG_EVENT
      << __f__(tickerId)
      << __tick_type_enum(field)
      << __f__(size);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::tickOptionComputation(
    TickerId tickerId, TickType tickType,
    double impliedVol,
    double delta, double optPrice, double pvDividend,
    double gamma, double vega,
    double theta, double undPrice) {
  LOG_EVENT
      << __f__(tickerId)
      << __tick_type_enum(tickType)
      << __f__(impliedVol)
      << __f__(delta)
      << __f__(optPrice)
      << __f__(pvDividend)
      << __f__(gamma)
      << __f__(vega)
      << __f__(theta)
      << __f__(undPrice);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::tickGeneric(
    TickerId tickerId, TickType tickType, double value) {
  LOG_EVENT
      << __f__(tickerId)
      << __tick_type_enum(tickType)
      << __f__(value);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::tickString(TickerId tickerId, TickType tickType,
                                 const IBString& value) {
  LOG_EVENT
      << __f__(tickerId)
      << __tick_type_enum(tickType)
      << __f__(value);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::tickEFP(TickerId tickerId, TickType tickType,
                              double basisPoints,
                              const IBString& formattedBasisPoints,
                              double totalDividends, int holdDays,
                              const IBString& futureExpiry,
                              double dividendImpact,
                              double dividendsToExpiry) {
  LOG_EVENT
      << __f__(tickerId)
      << __tick_type_enum(tickType)
      << __f__(basisPoints)
      << __f__(formattedBasisPoints)
      << __f__(totalDividends)
      << __f__(holdDays)
      << __f__(futureExpiry)
      << __f__(dividendImpact)
      << __f__(dividendsToExpiry);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::openOrder(OrderId orderId, const Contract& contract,
                                const Order& order, const OrderState& state) {
  LOG_EVENT
      << __f__(orderId)
      << __f__(&contract)
      << __f__(&order)
      << __f__(&state);
}
void LoggingEWrapper::openOrderEnd() {
  LOG_EVENT;
}
void LoggingEWrapper::winError(const IBString &str, int lastError) {
  LOG_EVENT
      << __f__(str)
      << __f__(lastError);
}
void LoggingEWrapper::connectionClosed() {
  LOG_EVENT;
}
void LoggingEWrapper::updateAccountValue(const IBString& key,
                                         const IBString& val,
                                         const IBString& currency,
                                         const IBString& accountName) {
  LOG_EVENT
      << __f__(key)
      << __f__(val)
      << __f__(currency)
      << __f__(accountName);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::updatePortfolio(const Contract& contract, int position,
                                      double marketPrice, double marketValue,
                                      double averageCost,
                                      double unrealizedPNL, double realizedPNL,
                                      const IBString& accountName) {
  LOG_EVENT
      << __f__(&contract)
      << __f__(position)
      << __f__(marketPrice)
      << __f__(marketValue)
      << __f__(averageCost)
      << __f__(unrealizedPNL)
      << __f__(realizedPNL)
      << __f__(accountName);
}
void LoggingEWrapper::updateAccountTime(const IBString& timeStamp) {
  LOG_EVENT
      << __f__(timeStamp);
}
void LoggingEWrapper::accountDownloadEnd(const IBString& accountName) {
  LOG_EVENT
      << __f__(accountName);
}
void LoggingEWrapper::contractDetails(int reqId,
                                      const ContractDetails& contractDetails) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(&contractDetails)
      << PrintContract(contractDetails.summary);
}
void LoggingEWrapper::bondContractDetails(
    int reqId, const ContractDetails& contractDetails) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(&contractDetails)
      << PrintContract(contractDetails.summary);
}
void LoggingEWrapper::contractDetailsEnd(int reqId) {
  LOG_EVENT
      << __f__(reqId);
}
void LoggingEWrapper::execDetails(int reqId, const Contract& contract,
                                  const Execution& execution) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(&contract)
      << __f__(&execution);
}
void LoggingEWrapper::execDetailsEnd(int reqId) {
  LOG_EVENT
      << __f__(reqId);
}
void LoggingEWrapper::updateMktDepth(TickerId id, int position,
                                     int operation, int side,
                                     double price, int size) {
  LOG_EVENT
      << __f__(id)
      << __f__(position)
      << __f__(operation)
      << __f__(side)
      << __f__(price)
      << __f__(size);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::updateMktDepthL2(TickerId id, int position,
                                       IBString marketMaker, int operation,
                                       int side, double price, int size) {
  LOG_EVENT
      << __f__(id)
      << __f__(position)
      << __f__(marketMaker)
      << __f__(operation)
      << __f__(side)
      << __f__(price)
      << __f__(size);
  TIME_EVENT_MARKETDATA;
}
void LoggingEWrapper::updateNewsBulletin(int msgId, int msgType,
                                         const IBString& newsMessage,
                                         const IBString& originExch) {
  LOG_EVENT
      << __f__(msgId)
      << __f__(msgType)
      << __f__(newsMessage)
      << __f__(originExch);
}
void LoggingEWrapper::managedAccounts(const IBString& accountsList) {
  LOG_EVENT
      << __f__(accountsList);
}
void LoggingEWrapper::receiveFA(faDataType pFaDataType, const IBString& cxml) {
  LOG_EVENT
      << __f__(pFaDataType)
      << __f__(cxml);
}
void LoggingEWrapper::historicalData(TickerId reqId, const IBString& date,
                                     double open, double high,
                                     double low, double close, int volume,
                                     int barCount, double WAP, int hasGaps) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(date)
      << __f__(open)
      << __f__(high)
      << __f__(low)
      << __f__(close)
      << __f__(volume)
      << __f__(barCount)
      << __f__(WAP)
      << __f__(hasGaps);
}
void LoggingEWrapper::scannerParameters(const IBString &xml) {
  LOG_EVENT
      << __f__(xml);
}
void LoggingEWrapper::scannerData(int reqId, int rank,
                                  const ContractDetails &contractDetails,
                                  const IBString &distance,
                                  const IBString &benchmark,
                                  const IBString &projection,
                                  const IBString &legsStr) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(rank)
      << __f__(&contractDetails)
      << __f__(distance)
      << __f__(benchmark)
      << __f__(projection)
      << __f__(legsStr);
}
void LoggingEWrapper::scannerDataEnd(int reqId) {
  LOG_EVENT
      << __f__(reqId);
}
void LoggingEWrapper::realtimeBar(TickerId reqId, long time,
                                  double open, double high,
                                  double low, double close,
                                  long volume, double wap, int count) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(time)
      << __f__(open)
      << __f__(high)
      << __f__(low)
      << __f__(close)
      << __f__(volume)
      << __f__(wap)
      << __f__(count);
}
void LoggingEWrapper::fundamentalData(TickerId reqId, const IBString& data) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(data);
}
void LoggingEWrapper::deltaNeutralValidation(int reqId,
                                             const UnderComp& underComp) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(&underComp);
}
void LoggingEWrapper::tickSnapshotEnd(int reqId) {
  LOG_EVENT
      << __f__(reqId);
}
void LoggingEWrapper::marketDataType(TickerId reqId, int marketDataType) {
  LOG_EVENT
      << __f__(reqId)
      << __f__(marketDataType);
}
void LoggingEWrapper::orderStatus(OrderId orderId, const IBString &status,
                                  int filled,  int remaining,
                                  double avgFillPrice,
                                  int permId, int parentId,
                                  double lastFillPrice, int clientId,
                                  const IBString& whyHeld) {
  LOG_EVENT
      << __f__(orderId)
      << __f__(status)
      << __f__(filled)
      << __f__(remaining)
      << __f__(avgFillPrice)
      << __f__(permId)
      << __f__(parentId)
      << __f__(lastFillPrice)
      << __f__(clientId)
      << __f__(whyHeld);
}
void LoggingEWrapper::nextValidId(OrderId orderId) {
  LOG_EVENT
      << __f__(orderId);
}
void LoggingEWrapper::currentTime(long time) {
  LOG_EVENT
      << __f__(time);
}
void LoggingEWrapper::error(const int id, const int errorCode,
                            const IBString errorString) {
  LOG_EVENT
      << __f__(id)
      << __f__(errorCode)
      << __f__(errorString);
}

#define LOG_START                                       \
  VARZ_api_requests++; ECLIENT_LOGGER                    \
  << "cid=" << connection_id_                           \
  << ",ts=" << (call_start_ = now_micros())             \
  << ",ts_utc=" << utc_micros().total_microseconds()    \
  << ",action=" << __func__

#define LOG_END                                         \
  ECLIENT_LOGGER                                        \
  << "cid=" << connection_id_                           \
  << ",ts=" << (call_start_)                            \
  << ",action=" << __func__                             \
  << ",elapsed=" << (now_micros() - call_start_)        \


// Exclusive lock when writing on the socket.
// If using boost::asio locking shouldn't be necessary.
typedef boost::unique_lock<boost::mutex> write_lock;

#define NO_LOCK

#ifdef NO_LOCK
#define LOCK // no-op
#else
#define LOCK write_lock lock(socket_write_mutex_);
#endif

//////////////////////////////////////////////////////////////

using ib::internal::PrintContract;

LoggingEClientSocket::LoggingEClientSocket(
    EWrapper* e_wrapper)
    : EClientSocketBase(e_wrapper)
    , connection_id_(-1) {
}

LoggingEClientSocket::~LoggingEClientSocket() {
}

void LoggingEClientSocket::set_connection_id(const unsigned int id)
{
  connection_id_ = id;;
}

const unsigned int LoggingEClientSocket::get_connection_id()
{
  return connection_id_;
}

int LoggingEClientSocket::serverVersion() {
  LOG_START;
  int v = EClientSocketBase::serverVersion();
  LOG_END;
  return v;
}
IBString LoggingEClientSocket::TwsConnectionTime() {
  LOG_START;
  IBString v = EClientSocketBase::TwsConnectionTime();
  LOG_END;
  return v;
}
void LoggingEClientSocket::reqMktData(TickerId id, const Contract &contract,
                                      const IBString& genericTicks,
                                      bool snapshot) {
  LOG_START
      << __f__(id)
      << __f__(&contract)
      << __f__(genericTicks)
      << __f__(snapshot)
      << PrintContract(contract);
  LOCK
      EClientSocketBase::reqMktData(id, contract, genericTicks, snapshot);
  LOG_END;
}
void LoggingEClientSocket::cancelMktData(TickerId id) {
  LOG_START;
  LOCK
      EClientSocketBase::cancelMktData(id);
  LOG_END;
}
void LoggingEClientSocket::placeOrder(OrderId id, const Contract &contract,
                                      const Order &order) {
  LOG_START
      << __f__(id)
      << __f__(&contract)
      << __f__(&order);
  LOCK
      EClientSocketBase::placeOrder(id, contract, order);
  LOG_END;
}
void LoggingEClientSocket::cancelOrder(OrderId id) {
  LOG_START
      << __f__(id);
  LOCK
      EClientSocketBase::cancelOrder(id);
  LOG_END;
}
void LoggingEClientSocket::reqOpenOrders() {
  LOG_START;
  LOCK
      EClientSocketBase::reqOpenOrders();
  LOG_END;
}
void LoggingEClientSocket::reqAccountUpdates(bool subscribe,
                                             const IBString& acctCode) {
  LOG_START
      << __f__(subscribe)
      << __f__(acctCode);
  LOCK
      EClientSocketBase::reqAccountUpdates(subscribe, acctCode);
  LOG_END;
}
void LoggingEClientSocket::reqExecutions(int reqId,
                                         const ExecutionFilter& filter) {
  LOG_START
      << __f__(reqId)
      << __f__(&filter);
  LOCK
      EClientSocketBase::reqExecutions(reqId, filter);
  LOG_END;
}
void LoggingEClientSocket::reqIds(int numIds) {
  LOG_START
      << __f__(numIds);
  LOCK
      EClientSocketBase::reqIds(numIds);
  LOG_END;
}
void LoggingEClientSocket::reqContractDetails(int reqId,
                                              const Contract &contract) {
  LOG_START
      << __f__(reqId)
      << __f__(&contract)
      << PrintContract(contract);
  LOCK
      EClientSocketBase::reqContractDetails(reqId, contract);
  LOG_END;
}
void LoggingEClientSocket::reqMktDepth(TickerId id,
                                       const Contract &contract, int numRows) {
  LOG_START
      << __f__(id)
      << __f__(&contract)
      << __f__(numRows)
      << PrintContract(contract);

  LOCK
      EClientSocketBase::reqMktDepth(id, contract, numRows);
  LOG_END;
}
void LoggingEClientSocket::cancelMktDepth(TickerId id) {
  LOG_START
      << __f__(id);
  LOCK
      EClientSocketBase::cancelMktDepth(id);
  LOG_END;
}
void LoggingEClientSocket::reqNewsBulletins(bool allMsgs) {
  LOG_START
      << __f__(allMsgs);
  LOCK
      EClientSocketBase::reqNewsBulletins(allMsgs);
  LOG_END;
}
void LoggingEClientSocket::cancelNewsBulletins() {
  LOG_START;
  LOCK
      EClientSocketBase::cancelNewsBulletins();
  LOG_END;
}
void LoggingEClientSocket::setServerLogLevel(int level) {
  LOG_START
      << __f__(level);
  LOCK
      EClientSocketBase::setServerLogLevel(level);
  LOG_END;
}
void LoggingEClientSocket::reqAutoOpenOrders(bool bAutoBind) {
  LOG_START
      << __f__(bAutoBind);
  LOCK
      EClientSocketBase::reqAutoOpenOrders(bAutoBind);
  LOG_END;
}
void LoggingEClientSocket::reqAllOpenOrders() {
  LOG_START;
  LOCK
      EClientSocketBase::reqAllOpenOrders();
  LOG_END;
}
void LoggingEClientSocket::reqManagedAccts() {
  LOG_START;
  LOCK
      EClientSocketBase::reqManagedAccts();
  LOG_END;
}
void LoggingEClientSocket::requestFA(faDataType pFaDataType) {
  LOG_START
      << __f__(pFaDataType);
  LOCK
      EClientSocketBase::requestFA(pFaDataType);
  LOG_END;
}
void LoggingEClientSocket::replaceFA(faDataType pFaDataType,
                                     const IBString& cxml) {
  LOG_START
      << __f__(pFaDataType)
      << __f__(cxml);
  LOCK
      EClientSocketBase::replaceFA(pFaDataType, cxml);
  LOG_END;
}
void LoggingEClientSocket::reqHistoricalData(
    TickerId id, const Contract &contract,
    const IBString &endDateTime,
    const IBString &durationStr, const IBString &barSizeSetting,
    const IBString &whatToShow, int useRTH, int formatDate) {
  LOG_START
      << __f__(id)
      << __f__(&contract)
      << __f__(endDateTime)
      << __f__(durationStr)
      << __f__(barSizeSetting)
      << __f__(whatToShow)
      << __f__(useRTH)
      << __f__(formatDate)
      << PrintContract(contract);
  LOCK
      EClientSocketBase::reqHistoricalData(
          id, contract, endDateTime,
          durationStr, barSizeSetting,
          whatToShow, useRTH, formatDate);
  LOG_END;
}
void LoggingEClientSocket::exerciseOptions(
    TickerId id, const Contract &contract,
    int exerciseAction, int exerciseQuantity,
    const IBString &account, int override) {
  LOG_START
      << __f__(id)
      << __f__(&contract)
      << __f__(exerciseAction)
      << __f__(exerciseQuantity)
      << __f__(account)
      << __f__(override);
  LOCK
      EClientSocketBase::exerciseOptions(
          id, contract,
          exerciseAction, exerciseQuantity,
          account, override);
  LOG_END;
}
void LoggingEClientSocket::cancelHistoricalData(TickerId tickerId ) {
  LOG_START
      << __f__(tickerId);
  LOCK
      EClientSocketBase::cancelHistoricalData(tickerId);
  LOG_END;
}
void LoggingEClientSocket::reqRealTimeBars(TickerId id,
                                           const Contract &contract,
                                           int barSize,
                                           const IBString &whatToShow,
                                           bool useRTH) {
  LOG_START
      << __f__(id)
      << __f__(&contract)
      << __f__(barSize)
      << __f__(whatToShow)
      << __f__(useRTH)
      << PrintContract(contract);
  LOCK
      EClientSocketBase::reqRealTimeBars(id, contract, barSize,
                                         whatToShow, useRTH);
  LOG_END;
}
void LoggingEClientSocket::cancelRealTimeBars(TickerId tickerId) {
  LOG_START
      << __f__(tickerId);
  LOCK
      EClientSocketBase::cancelRealTimeBars(tickerId);
  LOG_END;
}
void LoggingEClientSocket::cancelScannerSubscription(int tickerId) {
  LOG_START
      << __f__(tickerId);
  LOCK
      EClientSocketBase::cancelScannerSubscription(tickerId);
  LOG_END;
}
void LoggingEClientSocket::reqScannerParameters() {
  LOG_START;
  LOCK
      EClientSocketBase::reqScannerParameters();
  LOG_END;
}
void LoggingEClientSocket::reqScannerSubscription(
    int tickerId, const ScannerSubscription &subscription) {
  LOG_START
      << __f__(tickerId)
      << __f__(&subscription);
  LOCK
      EClientSocketBase::reqScannerSubscription(tickerId, subscription);
  LOG_END;
}
void LoggingEClientSocket::reqCurrentTime() {
  LOG_START;
  LOCK
      EClientSocketBase::reqCurrentTime();
  LOG_END;
}
void LoggingEClientSocket::reqFundamentalData(
    TickerId reqId, const Contract& contract, const IBString& reportType) {
  LOG_START
      << __f__(reqId)
      << __f__(&contract)
      << __f__(reportType);
  LOCK
      EClientSocketBase::reqFundamentalData(reqId, contract, reportType);
  LOG_END;
}
void LoggingEClientSocket::cancelFundamentalData(TickerId reqId) {
  LOG_START
      << __f__(reqId);
  LOCK
      EClientSocketBase::cancelFundamentalData(reqId);
  LOG_END;
}
void LoggingEClientSocket::calculateImpliedVolatility(
    TickerId reqId, const Contract &contract,
    double optionPrice, double underPrice) {
  LOG_START
      << __f__(reqId)
      << __f__(&contract)
      << __f__(optionPrice)
      << __f__(underPrice);
  LOCK
      EClientSocketBase::calculateImpliedVolatility(reqId, contract,
                                                    optionPrice, underPrice);
  LOG_END;
}
void LoggingEClientSocket::calculateOptionPrice(TickerId reqId,
                                                const Contract &contract,
                                                double volatility,
                                                double underPrice) {
  LOG_START
      << __f__(reqId)
      << __f__(&contract)
      << __f__(volatility)
      << __f__(underPrice);
  LOCK
      EClientSocketBase::calculateOptionPrice(reqId, contract,
                                              volatility, underPrice);
  LOG_END;
}
void LoggingEClientSocket::cancelCalculateImpliedVolatility(TickerId reqId) {
  LOG_START
      << __f__(reqId);
  LOCK
      EClientSocketBase::cancelCalculateImpliedVolatility(reqId);
  LOG_END;
}
void LoggingEClientSocket::cancelCalculateOptionPrice(TickerId reqId) {
  LOG_START
      << __f__(reqId);
  LOCK
      EClientSocketBase::cancelCalculateOptionPrice(reqId);
  LOG_END;
}
void LoggingEClientSocket::reqGlobalCancel() {
  LOG_START;
  LOCK
      EClientSocketBase::reqGlobalCancel();
  LOG_END;
}
void LoggingEClientSocket::reqMarketDataType(int marketDataType) {
  LOG_START
      << __f__(marketDataType);
  LOCK
      EClientSocketBase::reqMarketDataType(marketDataType);
  LOG_END;
}


};  // namespace adapter
};  // namespace ib
