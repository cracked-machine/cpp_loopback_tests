FROM debian:trixie

ARG USER=user
ENV DEBIAN_FRONTEND=noninteractive

# misc tools
RUN apt update -y && apt install -y \
    clangd \
    clang-format \
    sudo \
    locales \
    doxygen \
    git \
    build-essential \
    make \
    pkg-config \
    cmake \
    ninja-build \
    yq \
    jq \
    python3-scapy \
    tcpdump

# optional cross compile to windows    
RUN apt update -y && apt install -y \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64 

# POCO dependencies
RUN apt update -y && apt install -y \
    libpoco-dev

# Boost dependencies
RUN apt update -y && apt install -y \
    libboost-all-dev

# AF_XDP dependencies
RUN apt update -y && apt install -y \
    libbpf-dev libxdp-dev

# DPDK dependencies
RUN apt update -y && apt install -y \
     dpdk dpdk-dev libnuma-dev

RUN locale-gen en_GB.UTF-8 && update-locale

RUN useradd -m $USER && echo "$USER:$USER" | chpasswd && adduser $USER sudo
RUN echo "user            ALL = (ALL) NOPASSWD: ALL" >> /etc/sudoers

RUN printf "alias ll='ls -la'" >> /home/$USER/.bashrc

CMD /bin/bash