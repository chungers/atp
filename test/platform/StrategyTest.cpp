

#include <string>

#include <boost/bind.hpp>

#include <gtest/gtest.h>
#include <glog/logging.h>


#include "platform/strategy.hpp"


using std::string;


using namespace atp::platform;
using namespace proto::indicator;
using namespace proto::platform;



TEST(StrategyTest, UsageSyntax1)
{
  Strategy strategy;
  strategy.mutable_id()->set_name("test");
  strategy.mutable_id()->set_variant("instance1");

  strategy.set_comments("A simple strategy");

  Signal* signal = strategy.add_signal();
  signal->mutable_id()->MergeFrom(strategy.id());
  signal->mutable_id()->set_source("AAPL.STK");
  signal->mutable_id()->set_event("BID");

  signal->set_duration_micros(10^6 * 60);
  signal->set_interval_micros(10^6);
  signal->set_use_ohlc(false);
  signal->set_std_out(true);

  Signal_Indicator* indicator = signal->add_indicator();
  indicator->set_name("EMA5");
  indicator->mutable_config()->set_type(IndicatorConfig::MA);
  indicator->mutable_config()->mutable_ma()->set_type(MAConfig::EXPONENTIAL);
  indicator->mutable_config()->mutable_ma()->set_period(5);

  Signal_Indicator* indicator2 = signal->add_indicator();
  indicator2->MergeFrom(*indicator);
  indicator2->set_name("EMA20");
  indicator2->mutable_config()->mutable_ma()->set_period(20);

  Signal* signal2 = strategy.add_signal();
  signal2->MergeFrom(*signal); // get everything from signal
  signal2->mutable_id()->set_event("ASK"); // and override the event for ASK

  Signal* signal3 = strategy.add_signal();
  signal3->MergeFrom(*signal); // get everything from signal
  signal3->mutable_id()->set_event("LAST");

  signal3->set_use_ohlc(true); // use ohlc
  // all the indicators use the ohlc close as the source
  for (int i = 0; i < signal3->indicator_size(); ++i) {
    signal3->mutable_indicator(i)->set_ohlc_source(Signal_Indicator::CLOSE);
  }


  EXPECT_TRUE(strategy.IsInitialized());

  EXPECT_EQ("AAPL.STK", strategy.signal(0).id().source());
  EXPECT_EQ("BID", strategy.signal(0).id().event());
  EXPECT_EQ("EMA5", strategy.signal(0).indicator(0).name());
  EXPECT_EQ("EMA20", strategy.signal(0).indicator(1).name());

  EXPECT_EQ("AAPL.STK", strategy.signal(1).id().source());
  EXPECT_EQ("ASK", strategy.signal(1).id().event());
  EXPECT_EQ("EMA5", strategy.signal(1).indicator(0).name());
  EXPECT_EQ("EMA20", strategy.signal(1).indicator(1).name());

  EXPECT_EQ("AAPL.STK", strategy.signal(2).id().source());
  EXPECT_EQ("LAST", strategy.signal(2).id().event());
  EXPECT_EQ("EMA5", strategy.signal(2).indicator(0).name());
  EXPECT_EQ("EMA20", strategy.signal(2).indicator(1).name());
  EXPECT_TRUE(strategy.signal(2).use_ohlc());
  EXPECT_EQ(Signal_Indicator::CLOSE, strategy.signal(2).indicator(0).ohlc_source());
  EXPECT_EQ(Signal_Indicator::CLOSE, strategy.signal(2).indicator(1).ohlc_source());
}


