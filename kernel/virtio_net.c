#include "types.h"
#include "riscv.h"
#include "memlayout.h"
#include "virtio.h"
#include "defs.h"
// #include "buf.h"
#include "spinlock.h"

#define R(r) ((volatile uint32*)(VIRTIO1 + (r)))

struct virtio_net_hdr {
#define VIRTIO_NET_HDR_F_NEEDS_CSUM 1
#define VIRTIO_NET_HDR_F_DATA_VALID 2
#define VIRTIO_NET_HDR_F_RSC_INFO 4
    uint8 flags;
#define VIRTIO_NET_HDR_GSO_NONE 0
#define VIRTIO_NET_HDR_GSO_TCPV4 1
#define VIRTIO_NET_HDR_GSO_UDP 3
#define VIRTIO_NET_HDR_GSO_TCPV6 4
#define VIRTIO_NET_HDR_GSO_UDP_L4 5
#define VIRTIO_NET_HDR_GSO_ECN 0x80
    uint8 gso_type;
    uint16 hdr_len;
    uint16 gso_size;
    uint16 csum_start;
    uint16 csum_offset;
    uint16 num_buffers;
};

struct virtio_net_rx_hdr {
    uint8 flags;
    uint8 gso_type;
    uint16 hdr_len;
    uint16 gso_size;
    uint16 csum_start;
    uint16 csum_offset;
    uint16 num_buffers;
    uint8 data[1514]; 
};

struct virtio_net_config {
	uint8 mac[6];
	uint16 status;
	uint16 max_virtqueue_pairs;
	uint16 mtu;
	uint32 speed;
	uint8 duplex;
	uint8 rss_max_key_size;
	uint16 rss_max_indirection_table_length;
	uint32 supported_hash_types;
} __attribute__((packed));

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

    char free[NUM];

    uint16 used_idx;

    struct virtio_net_hdr head[NUM];
};

static struct net {
    struct virtq tx;
    struct virtq rx;
    uint64 buf[1526];
    struct spinlock vnet_lock;
} net;

static inline void
fill_desc(struct virtq_desc* desc, uint64 addr, uint32 len, uint16 flags, uint16 next) 
{
    desc->addr = addr;
    desc->len = len;
    desc->flags = flags;
    desc->next = next;
}

static inline void
fill_avail(struct virtq_avail* avail, uint16 idx)
{
    avail->ring[avail->idx % NUM] = idx;
    avail->idx++;
}

void virtio_net_cfg(volatile uint32* cfg_addr)
{
    struct virtio_net_config *cfg = (struct virtio_net_config *)cfg_addr;

    printf("mac: %x:%x:%x:%x:%x:%x\n", cfg->mac[0], cfg->mac[1], cfg->mac[2],
            cfg->mac[3], cfg->mac[4], cfg->mac[5]);
    printf("status: %d\n", cfg->status);
    printf("max_virtqueue_pairs: %d\n", cfg->max_virtqueue_pairs);
    printf("mtu: %d\n", cfg->mtu);
    printf("speed: %d\n", cfg->speed);
    printf("duplex: %d\n", cfg->duplex);
    printf("rss_max_key_size: %d\n", cfg->rss_max_key_size);
    printf("rss_max_indirection_table_length: %d\n", cfg->rss_max_indirection_table_length);
    printf("supported_hash_types: %d\n", cfg->supported_hash_types);
}

static void init_virtq(struct virtq* queue, int qnum) {
    // initialize queue 0
    *R(VIRTIO_MMIO_QUEUE_SEL) = qnum;
    printf("1. queue %d STATUS: %d\n", qnum, *R(VIRTIO_MMIO_QUEUE_READY));
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

    if (qnum == 1) {
        for (int i = 0; i < NUM; i++) {
        fill_desc(net.rx.desc + i, (uint64)&net.buf[i],
                1526, VRING_DESC_F_WRITE, 0);
        fill_avail(net.rx.avail, i);
        }
    }

    // queue is ready.
    *R(VIRTIO_MMIO_QUEUE_READY) = 0x1;
    printf("2. queue %d STATUS: %d\n", qnum, *R(VIRTIO_MMIO_QUEUE_READY));
    for(int i=0; i<NUM; i++)
        queue->free[i] = 1;
}

void 
virtio_net_init(void) 
{
    initlock(&net.vnet_lock, "virtio_net");

    uint32 status = 0;
    printf("VIRTIO_MMIO_DEVICE_ID: %x\n", *R(VIRTIO_MMIO_DEVICE_ID));
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
    printf("feature is %x\n", features);
    features = VIRTIO_NET_F_MTU |  VIRTIO_NET_F_STATUS | VIRTIO_F_VERSION_1 |VIRTIO_F_RING_RESET;
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

    virtio_net_cfg(R(VIRTIO_MMIO_CONFIG));
}

static void
free_desc(int i, int qnum)
{   
    struct virtq queue;
    if (qnum == 0) 
        queue = net.tx;
    else
        queue = net.rx;
  if(i >= NUM)
    panic("free_desc 1");
  if(queue.free[i])
    panic("free_desc 2");
  queue.desc[i].addr = 0;
  queue.desc[i].len = 0;
  queue.desc[i].flags = 0;
  queue.desc[i].next = 0;
  queue.free[i] = 1;
  wakeup(&queue.free[0]);
}

static void
free_chain(int i, int qnum)
{   
    struct virtq queue;
    if (qnum == 0) 
        queue = net.tx;
    else
        queue = net.rx;
    while(1){
        int flag = queue.desc[i].flags;
        int nxt = queue.desc[i].next;
        free_desc(i, qnum);
        if(flag & VRING_DESC_F_NEXT)
            i = nxt;
        else
            break;
    }
}

// find a free descriptor, mark it non-free, return its index.
static int
alloc_desc(int qnum) {
    char *free;
    if (qnum == 0) 
        free = net.tx.free;
    else
        free = net.rx.free;

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
                free_desc(idx[j], qnum);
            return -1;
        }
    }
    return 0;
}


void virtio_net_send(uint8* buf, uint32 len, int flag) 
{   
    acquire(&net.vnet_lock);
    printf("Now in send\n");
    printf("VIRTIO_MMIO_STATUS: %x\n", *R(VIRTIO_MMIO_STATUS));

    printf("send: ");
    for (int i = 0; i < len; i++) {
        printf("%d ", buf[i]);
    }
    printf("\n");

    int idx[2];
    while (1) {
        if (alloc2_desc(idx, 0) == 0) {
            break;
        }
        sleep(&net.tx.free[0], &net.vnet_lock);
    }



    fill_desc(&net.tx.desc[idx[0]], (uint64)&net.tx.head[idx[0]], sizeof(struct virtio_net_hdr), VRING_DESC_F_NEXT, idx[1]);

    fill_desc(&net.tx.desc[idx[1]], (uint64)buf, len, flag, 0);

    __sync_synchronize();

    fill_avail(net.tx.avail, idx[0]);

    __sync_synchronize();

    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; 

    uint16* pt_idx = &net.tx.used->idx;
    uint16* pt_used_idx = &net.tx.used_idx;

    while(*pt_idx == *pt_used_idx) ;
    net.tx.used_idx += 1;
    
    free_chain(idx[0], 0);
    printf("Now out send\n");

    release(&net.vnet_lock);
}


void 
virtio_net_rece(uint8* buf) 
{
    acquire(&net.vnet_lock);
    printf("Now in receive\n");

    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

  __sync_synchronize();

    // used_idx 和 idx 比较，确定是否有新数据传入
    uint16* pt_idx = &net.rx.used->idx;
    uint16* pt_used_idx = &net.rx.used_idx;

    while (*pt_idx != *pt_used_idx) {
        // 如果有新数据传入，used_idx指向ring中新存放的数据
        struct virtq_used_elem data = net.rx.used->ring[net.rx.used_idx % NUM]; //未协商VIRTIO_NET_F_MRG_RXBUF，只有一个描述符

        struct virtq_desc data_desc = net.rx.desc[data.id];
        
        buf = ((struct virtio_net_rx_hdr*)data_desc.addr)->data;
        printf("receive: ");
        int data_len = data.len -sizeof(struct virtio_net_hdr);
        for (int i = 0; i < data_len ; i++) {
            printf("%d ", buf[i]);
        }
        printf("\n");

        fill_avail(net.rx.avail, data.id);
        net.rx.used_idx += 1;
    }
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 1;
    printf("Now out receive\n");

    release(&net.vnet_lock);
}

void 
virtio_net_intr(void) 
{
    uint8* buf = 0;
    virtio_net_rece(buf);
}

void 
virtio_test(void)
{   
    #define LEN 10
    uint8 buf[LEN] = {0};
    for (int i = 0; i < LEN; i++) {
        buf[i] = i;
    }
    virtio_net_send(buf, LEN, 0);
}