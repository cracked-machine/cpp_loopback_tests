#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

constexpr uint16_t NB_MBUF = 8192;
constexpr uint16_t BURST_SIZE = 32;

// Thread-safe queue
class PacketQueue
{
public:
  void push( struct rte_mbuf *pkt )
  {
    std::unique_lock<std::mutex> lock( mutex_ );
    queue_.push( pkt );
    cv_.notify_one();
  }

  struct rte_mbuf *pop()
  {
    std::unique_lock<std::mutex> lock( mutex_ );
    cv_.wait( lock, [this] { return !queue_.empty(); } );
    struct rte_mbuf *pkt = queue_.front();
    queue_.pop();
    return pkt;
  }

private:
  std::queue<struct rte_mbuf *> queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

// Initialize a port
bool init_port( uint16_t port_id, struct rte_mempool *mbuf_pool )
{
  struct rte_eth_conf port_conf{};
  port_conf.rxmode.max_lro_pkt_size = RTE_ETHER_MAX_LEN; // modern DPDK

  if ( rte_eth_dev_configure( port_id, 1, 1, &port_conf ) != 0 ) return false;

  if ( rte_eth_rx_queue_setup(
           port_id, 0, 1024, rte_eth_dev_socket_id( port_id ), nullptr, mbuf_pool ) != 0 )
    return false;

  if ( rte_eth_tx_queue_setup( port_id, 0, 1024, rte_eth_dev_socket_id( port_id ), nullptr ) != 0 )
    return false;

  return rte_eth_dev_start( port_id ) == 0;
}

// Ingress thread
void ingress_thread( uint16_t port_id, PacketQueue &queue )
{
  struct rte_mbuf *bufs[BURST_SIZE];

  while ( true )
  {
    uint16_t nb_rx = rte_eth_rx_burst( port_id, 0, bufs, BURST_SIZE );
    for ( uint16_t i = 0; i < nb_rx; ++i )
    {
      queue.push( bufs[i] );
    }
  }
}

// Egress thread
void egress_thread( uint16_t port_id, PacketQueue &queue )
{
  struct rte_mbuf *bufs[BURST_SIZE];

  while ( true )
  {
    for ( uint16_t i = 0; i < BURST_SIZE; ++i )
      bufs[i] = queue.pop();

    uint16_t nb_tx = rte_eth_tx_burst( port_id, 0, bufs, BURST_SIZE );

    for ( uint16_t i = nb_tx; i < BURST_SIZE; ++i )
      rte_pktmbuf_free( bufs[i] );
  }
}

int main( int argc, char *argv[] )
{
  if ( rte_eal_init( argc, argv ) < 0 )
  {
    std::cerr << "Failed to init EAL" << std::endl;
    return 1;
  }

  struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
      "MBUF_POOL", NB_MBUF, 0, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id() );
  if ( !mbuf_pool )
  {
    std::cerr << "Failed to create mempool" << std::endl;
    return 1;
  }

  uint16_t ingress_port = 0;
  uint16_t egress_port = 1;

  if ( !init_port( ingress_port, mbuf_pool ) || !init_port( egress_port, mbuf_pool ) )
  {
    std::cerr << "Failed to init ports" << std::endl;
    return 1;
  }

  PacketQueue queue;

  std::thread ingress( ingress_thread, ingress_port, std::ref( queue ) );
  std::thread egress( egress_thread, egress_port, std::ref( queue ) );

  ingress.join();
  egress.join();

  return 0;
}
