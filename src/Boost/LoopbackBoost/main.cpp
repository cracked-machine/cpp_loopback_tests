#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <pcap/pcap.h>
#include <queue>
#include <vector>

namespace po = boost::program_options;

// Thread-safe packet queue
class PacketQueue
{
public:
  void push( const std::vector<u_char> &pkt, const struct pcap_pkthdr &hdr )
  {
    boost::unique_lock<boost::mutex> lock( mutex_ );
    queue_.push( { hdr, pkt } );
    cond_.notify_one();
  }

  bool pop( std::vector<u_char> &pkt, struct pcap_pkthdr &hdr )
  {
    boost::unique_lock<boost::mutex> lock( mutex_ );
    while ( queue_.empty() && running_ )
      cond_.wait( lock );
    if ( !running_ && queue_.empty() ) return false;
    auto entry = queue_.front();
    queue_.pop();
    hdr = entry.first;
    pkt = entry.second;
    return true;
  }

  void stop()
  {
    boost::unique_lock<boost::mutex> lock( mutex_ );
    running_ = false;
    cond_.notify_all();
  }

private:
  std::queue<std::pair<struct pcap_pkthdr, std::vector<u_char>>> queue_;
  boost::mutex mutex_;
  boost::condition_variable cond_;
  bool running_ = true;
};

// Ingress thread
class IngressWorker
{
public:
  IngressWorker( pcap_t *handle, PacketQueue &queue )
      : handle_( handle ),
        queue_( queue )
  {
  }

  void operator()()
  {
    const u_char *pkt;
    struct pcap_pkthdr *hdr;
    while ( true )
    {
      int ret = pcap_next_ex( handle_, &hdr, &pkt );
      if ( ret == 1 )
      {
        std::vector<u_char> copy( pkt, pkt + hdr->caplen );
        queue_.push( copy, *hdr );
      }
      else if ( ret == -2 )
        break;
      else if ( ret == -1 )
      {
        std::cerr << "Ingress error: " << pcap_geterr( handle_ ) << std::endl;
        break;
      }
    }
    queue_.stop();
  }

private:
  pcap_t *handle_;
  PacketQueue &queue_;
};

// Egress thread
class EgressWorker
{
public:
  EgressWorker( pcap_t *handle, pcap_dumper_t *dumper, PacketQueue &queue )
      : handle_( handle ),
        dumper_( dumper ),
        queue_( queue )
  {
  }

  void operator()()
  {
    std::vector<u_char> pkt;
    struct pcap_pkthdr hdr;
    while ( queue_.pop( pkt, hdr ) )
    {
      if ( dumper_ ) { pcap_dump( (u_char *)dumper_, &hdr, pkt.data() ); }
      else if ( handle_ )
      {
        if ( pcap_sendpacket( handle_, pkt.data(), pkt.size() ) != 0 )
        {
          std::cerr << "Egress send error: " << pcap_geterr( handle_ ) << std::endl;
        }
      }
    }
  }

private:
  pcap_t *handle_;
  pcap_dumper_t *dumper_;
  PacketQueue &queue_;
};

// Helper to detect PCAP file by extension
bool isPcapFile( const std::string &s ) { return s.find( ".pcap" ) != std::string::npos; }

int main( int argc, char **argv )
{
  std::string ingress, egress;
  int snaplen = 65535;

  // --- CLI ---
  po::options_description desc( "Loopback Boost App Options" );
  desc.add_options()( "help,h", "show help" )(
      "ingress,i", po::value<std::string>( &ingress )->required(), "ingress file or device" )(
      "egress,e", po::value<std::string>( &egress )->required(), "egress file or device" )(
      "snaplen,s", po::value<int>( &snaplen )->default_value( 65535 ), "snapshot length" );

  po::variables_map vm;
  try
  {
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    if ( vm.count( "help" ) )
    {
      std::cout << desc << std::endl;
      return 0;
    }
    po::notify( vm );
  }
  catch ( const std::exception &ex )
  {
    std::cerr << ex.what() << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  char errbuf[PCAP_ERRBUF_SIZE];

  // --- Open ingress ---
  pcap_t *ingressHandle = nullptr;
  if ( isPcapFile( ingress ) ) { ingressHandle = pcap_open_offline( ingress.c_str(), errbuf ); }
  else { ingressHandle = pcap_open_live( ingress.c_str(), snaplen, 1, 1000, errbuf ); }
  if ( !ingressHandle )
  {
    std::cerr << "Cannot open ingress: " << errbuf << std::endl;
    return 1;
  }

  // --- Open egress ---
  pcap_dumper_t *dumper = nullptr;
  pcap_t *egressHandle = nullptr;
  if ( isPcapFile( egress ) )
  {
    dumper = pcap_dump_open( ingressHandle, egress.c_str() );
    if ( !dumper )
    {
      std::cerr << "Cannot open egress file: " << pcap_geterr( ingressHandle ) << std::endl;
      pcap_close( ingressHandle );
      return 1;
    }
  }
  else
  {
    egressHandle = pcap_open_live( egress.c_str(), snaplen, 1, 1000, errbuf );
    if ( !egressHandle )
    {
      std::cerr << "Cannot open egress device: " << errbuf << std::endl;
      pcap_close( ingressHandle );
      return 1;
    }
  }

  // --- Packet queue & threads ---
  PacketQueue queue;
  boost::thread ingressThread( IngressWorker( ingressHandle, queue ) );
  boost::thread egressThread( EgressWorker( egressHandle, dumper, queue ) );

  ingressThread.join();
  egressThread.join();

  if ( dumper ) pcap_dump_close( dumper );
  if ( egressHandle ) pcap_close( egressHandle );
  if ( ingressHandle ) pcap_close( ingressHandle );

  std::cout << "Loopback completed." << std::endl;
  return 0;
}
