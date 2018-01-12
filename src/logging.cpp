#include <boost/log/core/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup.hpp>

#include <paxos/logging.hpp>


namespace paxos
{


BOOST_LOG_GLOBAL_LOGGER_INIT(global_logger, boost::log::sources::severity_logger_mt)
{
    boost::log::sources::severity_logger_mt<LogLevel> logger;

    boost::log::add_file_log
    (
        boost::log::keywords::file_name = "paxos.log",
        boost::log::keywords::rotation_size = LogFileRotationSize,
        boost::log::keywords::format =
        (
            boost::log::expressions::stream
                << boost::log::expressions::format_date_time<boost::posix_time::ptime>
                    ("TimeStamp", "[%Y-%m-%d %H:%M:%S:%f]: ")
                << boost::log::expressions::message
        )
    );
    boost::log::add_console_log
    (
        std::cout,
        boost::log::keywords::format =
        (
            boost::log::expressions::stream
                << boost::log::expressions::format_date_time<boost::posix_time::ptime>
                    ("TimeStamp", "[%Y-%m-%d %H:%M:%S:%f]: ")
                << boost::log::expressions::message
        )
    );
    boost::log::core::get()->set_filter
    (
        boost::log::expressions::attr<LogLevel>("Severity") >= LogLevel::Info
    );

    boost::log::add_common_attributes();

    return logger;
}


void
DisableLogging()
{
    boost::log::core::get()->set_logging_enabled(false);
}


}
