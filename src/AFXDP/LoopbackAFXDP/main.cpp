#include <condition_variable>
#include <iostream>
#include <linux/if_xdp.h>
#include <mutex>
#include <net/if.h>
#include <queue>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <xdp/xsk.h>

#define NUM_FRAMES 4096
#define FRAME_SIZE 2048
#define BATCH_SIZE 64

#ifndef XDP_FLAGS_UPDATE_IF_NOEXIST
#define XDP_FLAGS_UPDATE_IF_NOEXIST 0
#endif

// Thread-safe packet queue
struct Packet
{
  uint64_t addr;
  uint32_t len;
};

class PacketQueue
{
public:
  void push( Packet pkt )
  {
    std::unique_lock<std::mutex> lock( mutex );
    queue.push( pkt );
    cond.notify_one();
  }

  bool pop( Packet &pkt )
  {
    std::unique_lock<std::mutex> lock( mutex );
    while ( queue.empty() && running )
      cond.wait( lock );
    if ( !running && queue.empty() ) return false;
    pkt = queue.front();
    queue.pop();
    return true;
  }

  void stop()
  {
    std::unique_lock<std::mutex> lock( mutex );
    running = false;
    cond.notify_all();
  }

private:
  std::queue<Packet> queue;
  std::mutex mutex;
  std::condition_variable cond;
  bool running = true;
};

// UMEM wrapper
struct UMEM
{
  struct xsk_umem *umem;
  void *area;
  size_t size;
  struct xsk_ring_prod *fq; // Fill queue
  struct xsk_ring_cons *cq; // Completion queue
};

// AF_XDP socket wrapper
struct XDP_Socket
{
  struct xsk_socket *xsk;
  struct xsk_umem *umem;
  struct xsk_ring_cons *rx;
  struct xsk_ring_prod *tx;
  int ifindex;
  uint32_t queue_id;
};

// Setup UMEM
bool setup_umem( UMEM &umem )
{
  size_t size = NUM_FRAMES * FRAME_SIZE;
  void *area = mmap(
      nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0 );
  if ( area == MAP_FAILED ) return false;

  struct xsk_umem_config cfg = {};
  cfg.fill_size = NUM_FRAMES;
  cfg.comp_size = NUM_FRAMES;
  cfg.frame_size = FRAME_SIZE;
  cfg.frame_headroom = 0;

  struct xsk_umem *xu;
  umem.fq = new xsk_ring_prod;
  umem.cq = new xsk_ring_cons;

  if ( xsk_umem__create( &xu, area, size, umem.fq, umem.cq, &cfg ) )
  {
    munmap( area, size );
    delete umem.fq;
    delete umem.cq;
    return false;
  }

  umem.umem = xu;
  umem.area = area;
  umem.size = size;
  return true;
}

// Setup AF_XDP socket (classic API)
bool setup_xdp_socket( XDP_Socket &xsk, const char *ifname, UMEM &umem )
{
  xsk.ifindex = if_nametoindex( ifname );
  xsk.queue_id = 0;

  struct xsk_socket_config cfg = {};
  cfg.rx_size = NUM_FRAMES;
  cfg.tx_size = NUM_FRAMES;
  cfg.libbpf_flags = 0;
  cfg.xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST;

  xsk.rx = new xsk_ring_cons;
  xsk.tx = new xsk_ring_prod;

  if ( xsk_socket__create( &xsk.xsk, ifname, xsk.queue_id, umem.umem, xsk.rx, xsk.tx, &cfg ) )
  {
    std::cerr << "Failed to create XSK socket on " << ifname << "\n";
    delete xsk.rx;
    delete xsk.tx;
    return false;
  }

  xsk.umem = umem.umem;
  return true;
}

// Fill UMEM fill queue
void populate_fill_ring( UMEM &umem )
{
  uint32_t idx;
  for ( uint32_t i = 0; i < NUM_FRAMES; ++i )
  {
    if ( xsk_ring_prod__reserve( umem.fq, 1, &idx ) )
    {
      *xsk_ring_prod__fill_addr( umem.fq, idx ) = i * FRAME_SIZE;
    }
  }
  xsk_ring_prod__submit( umem.fq, NUM_FRAMES );
}

// Ingress thread: RX -> queue
void ingress_thread( XDP_Socket &xsk, UMEM &umem, PacketQueue &queue )
{
  uint32_t idxs[BATCH_SIZE];

  while ( true )
  {
    uint32_t n = xsk_ring_cons__peek( xsk.rx, BATCH_SIZE, idxs );
    for ( uint32_t i = 0; i < n; ++i )
    {
      const struct xdp_desc *desc = xsk_ring_cons__rx_desc( xsk.rx, idxs[i] );
      queue.push( Packet{ desc->addr, desc->len } );

      // Return frame to fill queue
      uint32_t fidx;
      if ( xsk_ring_prod__reserve( umem.fq, 1, &fidx ) )
      {
        *xsk_ring_prod__fill_addr( umem.fq, fidx ) = desc->addr;
        xsk_ring_prod__submit( umem.fq, 1 );
      }
    }
    xsk_ring_cons__release( xsk.rx, n );
  }
}

// Egress thread: queue -> TX
void egress_thread( XDP_Socket &xsk, PacketQueue &queue )
{
  uint32_t idx;
  Packet pkt;

  while ( queue.pop( pkt ) )
  {
    if ( xsk_ring_prod__reserve( xsk.tx, 1, &idx ) )
    {
      xsk_ring_prod__tx_desc( xsk.tx, idx )->addr = pkt.addr;
      xsk_ring_prod__tx_desc( xsk.tx, idx )->len = pkt.len;
      xsk_ring_prod__submit( xsk.tx, 1 );
    }
  }
}

int main( int argc, char **argv )
{
  if ( argc != 3 )
  {
    std::cerr << "Usage: " << argv[0] << " <ingress-if> <egress-if>\n";
    return 1;
  }

  UMEM umem{};
  if ( !setup_umem( umem ) )
  {
    std::cerr << "UMEM setup failed\n";
    return 1;
  }

  XDP_Socket xsk_ing{}, xsk_eg{};
  if ( !setup_xdp_socket( xsk_ing, argv[1], umem ) ) return 1;
  if ( !setup_xdp_socket( xsk_eg, argv[2], umem ) ) return 1;

  populate_fill_ring( umem );

  PacketQueue queue;
  std::thread t_rx( ingress_thread, std::ref( xsk_ing ), std::ref( umem ), std::ref( queue ) );
  std::thread t_tx( egress_thread, std::ref( xsk_eg ), std::ref( queue ) );

  t_rx.join();
  t_tx.join();

  xsk_socket__delete( xsk_ing.xsk );
  xsk_socket__delete( xsk_eg.xsk );
  xsk_umem__delete( umem.umem );
  munmap( umem.area, umem.size );
  delete umem.fq;
  delete umem.cq;

  return 0;
}
