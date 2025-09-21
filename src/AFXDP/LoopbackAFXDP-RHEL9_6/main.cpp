#include <linux/if_link.h>
#include <linux/if_xdp.h>
#include <bpf/libbpf.h>
#include <xdp/xsk.h>
#include <unistd.h>
#include <sys/resource.h>

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>

constexpr int FRAME_SIZE = 2048;
constexpr int NUM_FRAMES = 4096;
constexpr int BATCH_SIZE = 64;

struct Packet {
    size_t len;
    char data[FRAME_SIZE];
};

// Thread-safe queue
class PacketQueue {
public:
    void push(Packet pkt) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(pkt);
        cv_.notify_one();
    }

    Packet pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]{ return !queue_.empty(); });
        Packet pkt = queue_.front();
        queue_.pop();
        return pkt;
    }

private:
    std::queue<Packet> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

// Setup ulimit for locked memory (required for XDP)
bool set_memlock_rlimit() {
    struct rlimit rlim{RLIM_INFINITY, RLIM_INFINITY};
    return setrlimit(RLIMIT_MEMLOCK, &rlim) == 0;
}

// Simple UMEM wrapper
struct UMEM {
    struct xsk_umem* umem = nullptr;
    void* buffer = nullptr;
    size_t size = FRAME_SIZE * NUM_FRAMES;
};

// Simple XDP socket wrapper
struct XDP_Socket {
    struct xsk_socket* xsk = nullptr;
    std::string ifname;
    uint32_t queue_id = 0;
};

// Populate fill ring
void populate_fill_ring(XDP_Socket& xsk, UMEM& umem) {
    struct xsk_ring_prod* fr = xsk_umem__fill_ring(umem.umem);
    uint32_t idx;
    for (uint32_t i = 0; i < NUM_FRAMES; ++i) {
        xsk_ring_prod__reserve(fr, 1, &idx);
        *xsk_ring_prod__fill_addr(fr, idx) = i * FRAME_SIZE;
        xsk_ring_prod__submit(fr, 1);
    }
}

// Ingress thread
void ingress_thread(XDP_Socket& xsk, UMEM& umem, PacketQueue& queue) {
    struct xsk_ring_cons* rx = xsk_socket__rx_ring(xsk.xsk);
    struct xsk_ring_prod* fq = xsk_umem__fill_ring(umem.umem);

    struct xdp_desc descs[BATCH_SIZE];
    uint32_t idx[BATCH_SIZE];

    while (true) {
        int nb = xsk_ring_cons__peek(rx, BATCH_SIZE, &idx[0]);
        for (int i = 0; i < nb; ++i) {
            struct xdp_desc* d = xsk_ring_cons__rx_desc(rx, idx[i]);
            Packet pkt;
            pkt.len = d->len;
            std::memcpy(pkt.data, (char*)umem.buffer + d->addr, d->len);
            queue.push(pkt);

            // Refill
            xsk_ring_prod__reserve(fq, 1, &idx[i]);
            *xsk_ring_prod__fill_addr(fq, idx[i]) = d->addr;
            xsk_ring_prod__submit(fq, 1);
        }
        xsk_ring_cons__release(rx, nb);
    }
}

// Egress thread
void egress_thread(XDP_Socket& xsk, PacketQueue& queue) {
    struct xsk_ring_prod* tx = xsk_socket__tx_ring(xsk.xsk);

    uint32_t idx[BATCH_SIZE];
    Packet pkts[BATCH_SIZE];

    while (true) {
        for (int i = 0; i < BATCH_SIZE; ++i)
            pkts[i] = queue.pop();

        int nb = xsk_ring_prod__reserve(tx, BATCH_SIZE, &idx[0]);
        for (int i = 0; i < nb; ++i) {
            // In real mode you'd copy data to UMEM; here we just simulate
            (void)pkts[i];
        }
        xsk_ring_prod__submit(tx, nb);
    }
}

int main(int argc, char* argv[]) {
    if (!set_memlock_rlimit()) {
        std::cerr << "Failed to set RLIMIT_MEMLOCK" << std::endl;
        return 1;
    }

    UMEM umem;
    umem.buffer = aligned_alloc(FRAME_SIZE, umem.size);

    // For real NIC use argv[1], for simulation hardcode "veth0"
    XDP_Socket xsk;
    xsk.ifname = "veth0";
    xsk.queue_id = 0;

    // Create UMEM
    struct xsk_umem_config cfg{};
    cfg.frame_size = FRAME_SIZE;
    cfg.frame_headroom = 0;
    cfg.fill_size = 4096;
    cfg.comp_size = 4096;

    if (xsk_umem__create(&umem.umem, umem.buffer, umem.size, nullptr, nullptr, &cfg)) {
        std::cerr << "Failed to create UMEM" << std::endl;
        return 1;
    }

    // Create XSK socket
    struct xsk_socket_config scfg{};
    scfg.rx_size = 4096;
    scfg.tx_size = 4096;
    scfg.libbpf_flags = 0;
    scfg.xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST;

    if (xsk_socket__create(&xsk.xsk, xsk.ifname.c_str(), xsk.queue_id, umem.umem, nullptr, nullptr, &scfg)) {
        std::cerr << "Failed to create XSK socket" << std::endl;
        return 1;
    }

    populate_fill_ring(xsk, umem);

    PacketQueue queue;
    std::thread ingress(ingress_thread, std::ref(xsk), std::ref(umem), std::ref(queue));
    std::thread egress(egress_thread, std::ref(xsk), std::ref(queue));

    ingress.join();
    egress.join();

    xsk_socket__delete(xsk.xsk);
    xsk_umem__delete(umem.umem);
    free(umem.buffer);

    return 0;
}
