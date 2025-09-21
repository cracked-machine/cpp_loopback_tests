// ./build-x86_64-linux-gnu/bin/LoopbackPOCO --ingress input.pcap --egress output.pcap
// ./build-x86_64-linux-gnu/bin/LoopbackPOCO --ingress eth0 --egress out.pcap
// ./build-x86_64-linux-gnu/bin/LoopbackPOCO --ingress input.pcap --egress eth1

// You can tail the pcap output file using
// sudo tcpdump -n -r <file.pcap> -U

#include <Poco/Condition.h>
#include <Poco/Mutex.h>
#include <Poco/Runnable.h>
#include <Poco/Thread.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/HelpFormatter.h>
#include <atomic>
#include <iostream>
#include <pcap/pcap.h>
#include <queue>
#include <vector>

using namespace Poco::Util;

// Thread-safe packet queue
class PacketQueue
{
public:
  void push( const std::vector<u_char> &pkt, const struct pcap_pkthdr &hdr )
  {
    Poco::Mutex::ScopedLock lock( _mutex );
    _queue.push( { hdr, pkt } );
    _cond.signal();
  }

  bool pop( std::vector<u_char> &pkt, struct pcap_pkthdr &hdr )
  {
    Poco::Mutex::ScopedLock lock( _mutex );
    while ( _queue.empty() && _running )
    {
      _cond.wait( _mutex );
    }
    if ( !_running && _queue.empty() ) return false;
    auto entry = _queue.front();
    _queue.pop();
    hdr = entry.first;
    pkt = entry.second;
    return true;
  }

  void stop()
  {
    Poco::Mutex::ScopedLock lock( _mutex );
    _running = false;
    _cond.broadcast();
  }

private:
  std::queue<std::pair<struct pcap_pkthdr, std::vector<u_char>>> _queue;
  Poco::Mutex _mutex;
  Poco::Condition _cond;
  bool _running = true;
};

// Ingress thread: reads from pcap_t (live or file)
class IngressWorker : public Poco::Runnable
{
public:
  IngressWorker( pcap_t *handle, PacketQueue &q )
      : _handle( handle ),
        _queue( q )
  {
  }

  void run() override
  {
    const u_char *pkt;
    struct pcap_pkthdr *hdr;
    while ( true )
    {
      int ret = pcap_next_ex( _handle, &hdr, &pkt );
      if ( ret == 1 )
      {
        std::vector<u_char> copy( pkt, pkt + hdr->caplen );
        _queue.push( copy, *hdr );
      }
      else if ( ret == -2 )
      {
        break; // EOF (file)
      }
      else if ( ret == -1 )
      {
        std::cerr << "Ingress error: " << pcap_geterr( _handle ) << std::endl;
        break;
      }
    }
    _queue.stop();
  }

private:
  pcap_t *_handle;
  PacketQueue &_queue;
};

// Egress thread: dequeues and writes out
class EgressWorker : public Poco::Runnable
{
public:
  EgressWorker( pcap_t *handle, pcap_dumper_t *dumper, PacketQueue &q )
      : _handle( handle ),
        _dumper( dumper ),
        _queue( q )
  {
  }

  void run() override
  {
    std::vector<u_char> pkt;
    struct pcap_pkthdr hdr;
    while ( _queue.pop( pkt, hdr ) )
    {
      if ( _dumper ) { pcap_dump( (u_char *)_dumper, &hdr, pkt.data() ); }
      else
      {
        if ( pcap_sendpacket( _handle, pkt.data(), pkt.size() ) != 0 )
        {
          std::cerr << "Egress send error: " << pcap_geterr( _handle ) << std::endl;
        }
      }
    }
  }

private:
  pcap_t *_handle;        // used if live device
  pcap_dumper_t *_dumper; // used if writing to PCAP file
  PacketQueue &_queue;
};

class LoopbackApp : public Application
{
public:
  LoopbackApp()
      : _helpRequested( false )
  {
  }

protected:
  void defineOptions( OptionSet &options ) override
  {
    Application::defineOptions( options );
    options.addOption( Option( "help", "h", "show help" ).repeatable( false ) );
    options.addOption(
        Option( "ingress", "i", "ingress source (pcap file or device)" ).argument( "file|dev" ) );
    options.addOption(
        Option( "egress", "e", "egress sink (pcap file or device)" ).argument( "file|dev" ) );
    options.addOption(
        Option( "snaplen", "s", "snapshot length" ).argument( "n" ).required( false ) );
  }

  void handleOption( const std::string &name, const std::string &value ) override
  {
    if ( name == "help" )
      _helpRequested = true;
    else if ( name == "ingress" )
      _ingress = value;
    else if ( name == "egress" )
      _egress = value;
    else if ( name == "snaplen" )
      _snaplen = std::stoi( value );
  }

  int main( const std::vector<std::string> & ) override
  {
    if ( _helpRequested || _ingress.empty() || _egress.empty() )
    {
      HelpFormatter fmt( options() );
      fmt.setCommand( commandName() );
      fmt.setUsage( "OPTIONS" );
      fmt.setHeader( "Loopback app: ingress → egress" );
      fmt.format( std::cout );
      return EXIT_OK;
    }

    char errbuf[PCAP_ERRBUF_SIZE];

    // --- Open ingress ---
    pcap_t *ingress = nullptr;
    if ( isPcapFile( _ingress ) ) { ingress = pcap_open_offline( _ingress.c_str(), errbuf ); }
    else { ingress = pcap_open_live( _ingress.c_str(), _snaplen, 1, 1000, errbuf ); }
    if ( !ingress )
    {
      std::cerr << "Cannot open ingress: " << errbuf << std::endl;
      return EXIT_SOFTWARE;
    }

    // --- Open egress ---
    pcap_dumper_t *dumper = nullptr;
    pcap_t *egressHandle = nullptr;
    if ( isPcapFile( _egress ) )
    {
      dumper = pcap_dump_open( ingress, _egress.c_str() ); // reuse linktype
      if ( !dumper )
      {
        std::cerr << "Cannot open egress pcap: " << pcap_geterr( ingress ) << std::endl;
        pcap_close( ingress );
        return EXIT_SOFTWARE;
      }
    }
    else
    {
      egressHandle = pcap_open_live( _egress.c_str(), _snaplen, 1, 1000, errbuf );
      if ( !egressHandle )
      {
        std::cerr << "Cannot open egress device: " << errbuf << std::endl;
        pcap_close( ingress );
        return EXIT_SOFTWARE;
      }
    }

    // --- Start workers ---
    PacketQueue queue;
    IngressWorker ingressWorker( ingress, queue );
    EgressWorker egressWorker( egressHandle, dumper, queue );

    Poco::Thread t1, t2;
    t1.start( ingressWorker );
    t2.start( egressWorker );

    t1.join();
    t2.join();

    if ( dumper ) pcap_dump_close( dumper );
    if ( egressHandle ) pcap_close( egressHandle );
    if ( ingress ) pcap_close( ingress );

    std::cout << "Loopback finished." << std::endl;
    return EXIT_OK;
  }

private:
  bool _helpRequested;
  std::string _ingress;
  std::string _egress;
  int _snaplen = 65535;

  bool isPcapFile( const std::string &s ) { return s.find( ".pcap" ) != std::string::npos; }
};

POCO_APP_MAIN( LoopbackApp )
