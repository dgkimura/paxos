#ifndef __LOGGING_HPP_INCLUDED__
#define __LOGGING_HPP_INCLUDED__


#include <boost/log/common.hpp>


namespace paxos
{


enum class LogLevel
{
    Info,
    Warning,
    Error
};

#define LOG(level) BOOST_LOG_SEV(global_logger::get(), level)

BOOST_LOG_GLOBAL_LOGGER(global_logger, boost::log::sources::severity_logger_mt<LogLevel>)

const int LogFileRotationSize = 10 * 1024 * 1024;


void DisableLogging();


}


#endif
