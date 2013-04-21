#ifndef IBAPI966_API_ECLIENT_MOCK_H_
#define IBAPI966_API_ECLIENT_MOCK_H_

#include <Shared/EClient.h>

///
/// No-op implementation of the EClient.h interface for testing
///
class EClientMock : public EClient
{
 public:
  EClientMock(EWrapper& ewrapper) : ewrapper_(ewrapper) {}
  ~EClientMock() {}

 protected:
  EWrapper* getWrapper()
  {
    return &ewrapper_;
  }

 public:


  virtual bool eConnect( const char *host, unsigned int port, int clientId=0)
  { return true; }

  virtual void eDisconnect(){}
  virtual int serverVersion()
  { return 0; }

  virtual IBString TwsConnectionTime()
  { return ""; }

  virtual void reqMktData( TickerId id, const Contract &contract,
                           const IBString& genericTicks, bool snapshot){}
  virtual void cancelMktData( TickerId id){}
  virtual void placeOrder( OrderId id, const Contract &contract, const Order &order){}
  virtual void cancelOrder( OrderId id){}
  virtual void reqOpenOrders(){}
  virtual void reqAccountUpdates(bool subscribe, const IBString& acctCode){}
  virtual void reqExecutions(int reqId, const ExecutionFilter& filter){}
  virtual void reqIds( int numIds){}
  virtual bool checkMessages(){ return true; }
  virtual void reqContractDetails( int reqId, const Contract &contract){}
  virtual void reqMktDepth( TickerId id, const Contract &contract, int numRows){}
  virtual void cancelMktDepth( TickerId id){}
  virtual void reqNewsBulletins( bool allMsgs){}
  virtual void cancelNewsBulletins(){}
  virtual void setServerLogLevel(int level){}
  virtual void reqAutoOpenOrders(bool bAutoBind){}
  virtual void reqAllOpenOrders(){}
  virtual void reqManagedAccts(){}
  virtual void requestFA(faDataType pFaDataType){}
  virtual void replaceFA(faDataType pFaDataType, const IBString& cxml){}
  virtual void reqHistoricalData( TickerId id, const Contract &contract,
                                  const IBString &endDateTime, const IBString &durationStr, const IBString &barSizeSetting,
                                  const IBString &whatToShow, int useRTH, int formatDate){}
  virtual void exerciseOptions( TickerId id, const Contract &contract,
                                int exerciseAction, int exerciseQuantity, const IBString &account, int override){}
  virtual void cancelHistoricalData( TickerId tickerId ){}
  virtual void reqRealTimeBars( TickerId id, const Contract &contract, int barSize,
                                const IBString &whatToShow, bool useRTH){}
  virtual void cancelRealTimeBars( TickerId tickerId){}
  virtual void cancelScannerSubscription( int tickerId){}
  virtual void reqScannerParameters(){}
  virtual void reqScannerSubscription( int tickerId, const ScannerSubscription &subscription){}
  virtual void reqCurrentTime(){}
  virtual void reqFundamentalData( TickerId reqId, const Contract&, const IBString& reportType){}
  virtual void cancelFundamentalData( TickerId reqId){}
  virtual void calculateImpliedVolatility( TickerId reqId, const Contract &contract, double optionPrice, double underPrice){}
  virtual void calculateOptionPrice( TickerId reqId, const Contract &contract, double volatility, double underPrice){}
  virtual void cancelCalculateImpliedVolatility( TickerId reqId){}
  virtual void cancelCalculateOptionPrice( TickerId reqId){}
  virtual void reqGlobalCancel(){}
  virtual void reqMarketDataType( int marketDataType){}

 private:
  EWrapper& ewrapper_;
};

#endif //IBAPI966_API_ECLIENT_MOCK_H_
