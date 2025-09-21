// dpdk_pcap_loop.hpp
#pragma once

extern "C" {
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
}

#include <stdexcept>
#include <string>
#include <vector>

class DpdkEnv
{
public:
  DpdkEnv( int argc, char **argv )
  {
    if ( rte_eal_init( argc, argv ) < 0 ) { throw std::runtime_error( "Failed to init DPDK EAL" ); }
  }
};

class DpdkPort
{
  uint16_t port_id;

public:
  DpdkPort( uint16_t id )
      : port_id( id )
  {
    if ( !rte_eth_dev_is_valid_port( port_id ) ) { throw std::runtime_error( "Invalid port id" ); }
  }

  void start( uint16_t nb_rxq = 1, uint16_t nb_txq = 1 )
  {
    rte_eth_dev_configure( port_id, nb_rxq, nb_txq, nullptr );
    for ( int q = 0; q < nb_rxq; q++ )
    {
      rte_eth_rx_queue_setup(
          port_id,
          q,
          128,
          rte_eth_dev_socket_id( port_id ),
          nullptr,
          rte_pktmbuf_pool_create(
              "RX_POOL", 1024, 32, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id() ) );
    }
    for ( int q = 0; q < nb_txq; q++ )
    {
      rte_eth_tx_queue_setup( port_id, q, 128, rte_eth_dev_socket_id( port_id ), nullptr );
    }
    rte_eth_dev_start( port_id );
  }

  rte_mbuf *recv( uint16_t q = 0 )
  {
    rte_mbuf *buf = nullptr;
    uint16_t n = rte_eth_rx_burst( port_id, q, &buf, 1 );
    return n ? buf : nullptr;
  }

  void send( rte_mbuf *pkt, uint16_t q = 0 ) { rte_eth_tx_burst( port_id, q, &pkt, 1 ); }
};
