#include "types.h"
#include "riscv.h"
#include "memlayout.h"
#include "virtio.h"
#include "defs.h"
// #include "buf.h"
#include "spinlock.h"

#define R(r) ((volatile uint32*)(VIRTIO1 + (r)))

struct virtq {
    // a set (not a ring) of DMA descriptors, with which the
    // driver tells the device where to read and write individual
    // disk operations. there are NUM descriptors.
    // most commands consist of a "chain" (a linked list) of a couple of
    // these descriptors.
    struct virtq_desc* desc;

    // a ring in which the driver writes descriptor numbers
    // that the driver would like the device to process.  it only
    // includes the head descriptor of each chain. the ring has
    // NUM elements.
    struct virtq_avail* avail;

    // a ring in which the device writes descriptor numbers that
    // the device has finished processing (just the head of each chain).
    // there are NUM used ring entries.
    struct virtq_used* used;
};

static struct net {
    struct virtq tx;
    struct virtq rx;
    int tx_free[NUM];
    int rx_free[NUM];

    struct spinlock vnet_lock;
} net;

static inline void init_virtq(struct virtq* queue, int qnum) {
    // initialize queue 0
    *R(VIRTIO_MMIO_QUEUE_SEL) = qnum;

    // ensure queue is not in use.
    if (*R(VIRTIO_MMIO_QUEUE_READY))
        panic("virtio net should not be ready");

    uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0)
        panic("virtio net has no queue");
    if (max < NUM)
        panic("virtio net max queue too short");

    queue->desc = kalloc();
    queue->avail = kalloc();
    queue->used = kalloc();

    if (!queue->desc || !queue->avail || !queue->used)
        panic("virtio disk kalloc");
    memset(queue->desc, 0, PGSIZE);
    memset(queue->avail, 0, PGSIZE);
    memset(queue->used, 0, PGSIZE);
    // set queue size.
    *R(VIRTIO_MMIO_QUEUE_NUM) = NUM;

    // write physical addresses.
    *R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)queue->desc;
    *R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)queue->desc >> 32;
    *R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)queue->avail;
    *R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)queue->avail >> 32;
    *R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)queue->used;
    *R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)queue->used >> 32;

    // queue is ready.
    *R(VIRTIO_MMIO_QUEUE_READY) = 0x1;
}

void 
virtio_net_init(void) 
{
    initlock(&net.vnet_lock, "virtio_net");

    uint32 status = 0;
    
    // initialize the virtio device
    if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 || 
    *R(VIRTIO_MMIO_VERSION) != 2 || 
    *R(VIRTIO_MMIO_DEVICE_ID) != 1 || 
    *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551) {
        panic("could not find virtio net");
    }

    // reset device
    *R(VIRTIO_MMIO_STATUS) = status;

    // set ACKNOWLEDGE status bit
    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS) = status;

    // set DRIVER status bit
    status |= VIRTIO_CONFIG_S_DRIVER;
    *R(VIRTIO_MMIO_STATUS) = status;

    // negotiate features
    uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
    // features &= ~(1 << VIRTIO_NET_F_MAC);
    // features &= ~(1 << VIRTIO_NET_F_STATUS);
    features = VIRTIO_NET_F_MTU | VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS;
    *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

    // tell device that feature negotiation is complete.
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    // re-read status to ensure FEATURES_OK is set.
    status = *R(VIRTIO_MMIO_STATUS);
    if (!(status & VIRTIO_CONFIG_S_FEATURES_OK))
        panic("virtio net FEATURES_OK unset");

    // initialize queue 0 and 1.
    init_virtq(&net.tx, 0);
    init_virtq(&net.rx, 1);

    // set driver ok
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = status;
}

// find a free descriptor, mark it non-free, return its index.
static int
alloc_desc(int qnum) {
    int *free;
    if (qnum == 0) 
        free = net.tx_free;
    else
        free = net.rx_free;

    for (int i = 0; i < NUM; i++) {
        if (free[i]) {
            free[i] = 0;
            return i;
        }
    }
    return -1;
}

// allocate three descriptors (they need not be contiguous).
// disk transfers always use three descriptors.
static int
alloc2_desc(int* idx, int qnum) {
    for (int i = 0; i < 2; i++) {
        idx[i] = alloc_desc(qnum);
        if (idx[i] < 0) {
            for (int j = 0; j < i; j++)
                free_desc(idx[j]);
            return -1;
        }
    }
    return 0;
}

void virtio_net_send(uint64* b, uint32 len, int flag) 
{
    acquire(&net.vnet_lock);

    int idx[3];
    while (1) {
        if (alloc2_desc(idx, 0) == 0) {
            break;
        }
        sleep(&net.tx_free[0], &net.vnet_lock);
    }


}
