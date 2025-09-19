#ifndef __LOGGING_BASICLOGGER_HPP__
#define __LOGGING_BASICLOGGER_HPP__

#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// Usage:
//
// #include <BasicLogController.hpp>
// std::unique_ptr<Test::Logging::BasicLogController> logger{
//      std::make_unique<Test::Logging::BasicLogController>("logger", "log.txt")
// };

namespace Logging {

//! @brief A non-synchronous log controller using SPDLog
//
//  Follows the Model–view–controller pattern:
//
// 1 Internal Model:
// - `spdlog::logger`
//
// 3 External Views:
// - `spdlog::sinks::stdout_color_sink_mt`
// - `spdlog::sinks::basic_file_sink_mt`
// - `spdlog::sinks::callback_sink_mt`
//
class BasicLogController
{
public:
  BasicLogController( std::string log_name, std::string log_path )
      : m_log_name( log_name ),
        m_log_path( log_path )
  {
    m_console_sink->set_level( spdlog::level::trace );
    // m_console_sink->set_pattern("[%c] [%^%l%$] %s:%v");
    m_console_sink->set_pattern( "%s:%# - %v" );

    m_file_sink->set_level( spdlog::level::trace );
    // m_file_sink->set_pattern("[%c] [%^%l%$] %s:%v");
    m_file_sink->set_pattern( "%s:%# - %v" );

    spdlog::set_default_logger( m_logger );
    spdlog::flush_on( spdlog::level::trace );
  }

private:
  std::string m_log_name{};
  std::string m_log_path{};

  // sink for Logging to stdout
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_console_sink{
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };

  // sink for Logging to file
  std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_file_sink{
      std::make_shared<spdlog::sinks::basic_file_sink_mt>( m_log_path, true ) };

  std::shared_ptr<spdlog::sinks::callback_sink_mt> m_callback_sink{
      std::make_shared<spdlog::sinks::callback_sink_mt>( []( const spdlog::details::log_msg &msg ) {
        // std::cout << "BasicLogController Callback!!!" << "\n";
      } ) };

  // initialise spdlog::logger with sinks
  std::shared_ptr<spdlog::logger> m_logger{
      std::make_shared<spdlog::logger>( spdlog::logger( m_log_name,
                                                        {
                                                            m_file_sink,
                                                            m_console_sink,
                                                            m_callback_sink // DISABLE CALLBACK HERE
                                                        } ) ) };
};

} // namespace Logging

#endif // __LOGGING_BASICLOGGER_HPP__