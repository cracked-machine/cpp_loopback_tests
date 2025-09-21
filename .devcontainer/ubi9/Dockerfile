FROM redhat/ubi9:9.6-1758184894

ARG USER=user

RUN dnf install -y \
    sudo \
    shadow-utils \
    bash \
    passwd \
    iproute \
    gcc \
    make \
    cmake \
    kernel-headers \
    git gcc clang make cmake bash sudo shadow-utils iproute

RUN git clone https://github.com/libbpf/libbpf.git /tmp/libbpf \
    && cd /tmp/libbpf/src \
    && make \
    && make install

RUN git clone https://github.com/xdp-project/xdp-tools.git /tmp/xdp-tools \
    && cd /tmp/xdp-tools \
    && make \
    && make install


# Create user and set password
RUN useradd -m -s /bin/bash $USER \
    && echo "$USER:$USER" | chpasswd \
    && usermod -aG wheel $USER \
    && echo "%wheel ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Create mount points for hugepages and BPFFS
RUN mkdir -p /mnt/huge /sys/fs/bpf

# Set working directory
WORKDIR /home/$USER

# Switch to non-root user
USER $USER
ENV HOME /home/$USER

# Copy your AF_XDP binary into the container
# Adjust the source path as needed
COPY build-x86_64-linux-gnu/bin/LoopbackAFXDP /usr/local/bin/LoopbackAFXDP

# Expose hugepages and BPFFS mount points to container at runtime
# These must be bind-mounted when running:
# docker run --mount type=bind,source=/mnt/huge,target=/mnt/huge ...
# docker run --mount type=bind,source=/sys/fs/bpf,target=/sys/fs/bpf ...

# Default command
CMD ["/bin/bash"]