// #include "Debug/AssertHandler.hpp" // Include this first for custom assertions

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <Logging/BasicLogController.hpp>

int main()
{
  // Logging: make sure errors - exceptions and failed asserts - go to log file
  fclose( stderr );

  using Logger = Logging::BasicLogController;
  std::unique_ptr<Logger> logger{ std::make_unique<Logger>( "logger", "log.txt" ) };

  SPDLOG_INFO( "Init" );
}