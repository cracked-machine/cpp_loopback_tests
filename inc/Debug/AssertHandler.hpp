// @file This file is for debug assertion handling and stack trace logging on windows builds

#ifndef DEBUG_ASSERT_HANDLER_HPP
#define DEBUG_ASSERT_HANDLER_HPP

#include <csignal>
#include <cstdlib>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <spdlog/spdlog.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace Debug {

void stack_trace();

[[noreturn]] inline void
assert_handler( const char *condition, const char *message, const char *file, const int line )
{
  SPDLOG_CRITICAL( "\n" );
  SPDLOG_CRITICAL( "=== Assertion Failed ===" );
  SPDLOG_CRITICAL( "Condition: {}", condition );
  SPDLOG_CRITICAL( "Message: {}", message );
  SPDLOG_CRITICAL( "Location: {}:{}", file, line );
  SPDLOG_CRITICAL( "========================" );

#ifdef _WIN32
  // Break into debugger if attached (Windows)
  if ( IsDebuggerPresent() ) { DebugBreak(); }
#elif defined( __unix__ )
  // Break into debugger if attached (Unix)
  if ( std::getenv( "UNDER_GDB" ) || std::getenv( "UNDER_LLDB" ) ) { raise( SIGTRAP ); }
#endif

  Debug::stack_trace();

  std::abort();
}

} // namespace Debug

// Define our custom assertion macro before including any EnTT headers
#define ENTT_ASSERT( condition, msg )                                                              \
  do                                                                                               \
  {                                                                                                \
    if ( !( condition ) ) { ::Debug::assert_handler( #condition, msg, __FILE__, __LINE__ ); }      \
  } while ( 0 )

#endif // DEBUG_ASSERT_HANDLER_HPP
