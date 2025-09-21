#include <boost/lexical_cast.hpp>
#include <iostream>

int main()
{
  int num = 42;
  std::string text = boost::lexical_cast<std::string>( num );
  std::cout << "Hello, Boost! Number: " << text << std::endl;
  return 0;
}
