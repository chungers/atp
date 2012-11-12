#ifndef ATP_SERVICE_LOG_READER_ZMQ_H_
#define ATP_SERVICE_LOG_READER_ZMQ_H_

#include <boost/thread.hpp>

#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"


using namespace std;

namespace atp {
namespace log_reader {

const static string DATA_END("DATA_END");

void DispatchEvents(LogReader& logReader,
                    const std::string& endpoint,
                    const time_duration& duration);

boost::thread* DispatchEventsInThread(LogReader& logReader,
                                      const std::string& endpoint,
                                      const time_duration& duration);


} // log_reader
} // atp

#endif //ATP_SERVICE_LOG_READER_ZMQ_H_
