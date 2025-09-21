#include <iostream>
#include <rte_eal.h>
#include <rte_version.h>

int main( int argc, char **argv )
{
  // Initialize the Environment Abstraction Layer (EAL)
  if ( rte_eal_init( argc, argv ) < 0 )
  {
    std::cerr << "Failed to initialize DPDK EAL" << std::endl;
    return 1;
  }

  std::cout << "Hello, DPDK! Version: " << rte_version() << std::endl;
  return 0;
}
