# Loopback examples


You can tail the pcap output file using

```
sudo tcpdump -n -r <file.pcap> -U
```

# Using C++ POCO Library

1. Reading input `.pcacp` file and looping back to output `.pcap` file:

    ```
    ./build-x86_64-linux-gnu/bin/LoopbackPOCO --ingress input.pcap --egress output.pcap
    ```

2. Reading network device and looping back output to `.pcap` file:

    ```
    ./build-x86_64-linux-gnu/bin/LoopbackPOCO --ingress eth0 --egress out.pcap
    ```

3. Reading from network device and looping back output to network device:

    ```
    ./build-x86_64-linux-gnu/bin/LoopbackPOCO --ingress input.pcap --egress eth1
    ```
