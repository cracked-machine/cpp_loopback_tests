#include <errno.h>
#include <linux/if_link.h>
#include <linux/if_xdp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <xdp/xsk.h>

int main()
{
  printf( "Hello, AF_XDP!\n" );

  int sock = socket( AF_XDP, SOCK_RAW, 0 );
  if ( sock < 0 )
  {
    perror( "socket(AF_XDP)" );
    return 1;
  }

  printf( "AF_XDP socket created successfully (fd=%d)\n", sock );

  close( sock );
  return 0;
}
