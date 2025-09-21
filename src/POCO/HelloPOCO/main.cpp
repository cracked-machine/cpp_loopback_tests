#include "Poco/DateTime.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Timespan.h"
#include <iostream>

int main()
{
  Poco::DateTime now;
  std::string formatted = Poco::DateTimeFormatter::format( now, "%Y-%m-%d %H:%M:%S" );
  std::cout << "Hello, POCO! Current time: " << formatted << std::endl;
  return 0;
}
