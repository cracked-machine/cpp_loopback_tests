#!/bin/bash
set -e

# 1️⃣ Create a hugepages mount
sudo mkdir -p /mnt/huge || true
sudo mount -t hugetlbfs nodev /mnt/huge

# Optional: reserve hugepages (example: 512 pages of 2MB)
echo 512 | sudo tee /proc/sys/vm/nr_hugepages

# 2️⃣ Ensure BPFFS is mounted
sudo mkdir -p /sys/fs/bpf
sudo mount -t bpf bpf /sys/fs/bpf


docker run -it --rm \
    --privileged \
    --network=host \
    --mount type=bind,source=/mnt/huge,target=/mnt/huge \
    --mount type=bind,source=/sys/fs/bpf,target=/sys/fs/bpf \
    --mount type=bind,source=$(pwd)/build-x86_64-linux-gnu/bin/LoopbackAFXDP,target=/usr/local/bin/LoopbackAFXDP \
    debian:trixie-slim \
    bash -c "apt update && apt install -y libbpf-dev libxdp-dev iproute2 && /usr/local/bin/LoopbackAFXDP --umem-dir /mnt/huge"

