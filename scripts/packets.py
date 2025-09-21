#!/usr/bin/env python3

from scapy.all import IP, TCP, Ether, wrpcap

# Output PCAP filename
pcap_file = "input.pcap"

# Parameters
src_ip = "192.168.0.1"
dst_ip = "192.168.0.2"
src_port = 12345
dst_port = 80
num_packets = 10
payload = b"HelloTCP"  # example payload

# Starting TCP sequence number
seq_num = 1000

packets = []

for i in range(num_packets):
    ip = IP(src=src_ip, dst=dst_ip)
    tcp = TCP(
        sport=src_port,
        dport=dst_port,
        seq=seq_num,
        ack=0,
        flags="PA"  # PSH + ACK
    )
    pkt = Ether() / ip / tcp / payload
    packets.append(pkt)

    # Increment sequence number: seq += TCP payload length
    seq_num += len(payload)

# Write packets to PCAP file
wrpcap(pcap_file, packets)
print(f"{num_packets} TCP packets written to {pcap_file}")
