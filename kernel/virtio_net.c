#include "types.h"
#include "riscv.h"
#include "memlayout.h"
#include "virtio.h"
#include "defs.h"
#include "spinlock.h"
#include "net.h"

#define R(r) ((volatile uint32*)(VIRTIO1 + (r)))
#define DATA_LEN 1514

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
}__attribute__((packed));

struct virtio_net_rx_hdr {
    uint8 flags;
    uint8 gso_type;
    uint16 hdr_len;
    uint16 gso_size;
    uint16 csum_start;
    uint16 csum_offset;
    uint8 data[DATA_LEN]; 
}__attribute__((packed));

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
}__attribute__((packed));

struct virtq {
    // a set (not a ring) of DMA descriptors, with which the
    // driver tells the device where to read and write individual
    // disk operations. there are NUM descriptors.
    // most commands consist of a "chain" (a linked list) of a couple of
    // these descriptors.
    struct virtq_desc desc[NUM];

    // a ring in which the driver writes descriptor numbers
    // that the driver would like the device to process.  it only
    // includes the head descriptor of each chain. the ring has
    // NUM elements.
    struct virtq_avail avail;

    // a ring in which the device writes descriptor numbers that
    // the device has finished processing (just the head of each chain).
    // there are NUM used ring entries.
    struct virtq_used used;

    uint16 used_idx;
};

static struct net {
    struct virtq tx;
    struct virtq rx;
    struct virtio_net_hdr tx_head[NUM];
    struct virtio_net_rx_hdr rx_head[NUM];
    struct spinlock vnet_lock;
} net = {0};

static inline void
fill_desc(struct virtq_desc* desc, uint64 addr, uint32 len, uint16 flags, uint16 next) 
{
    volatile struct virtq_desc* pt = (volatile struct virtq_desc*)desc;
	pt->addr = addr;
	pt->len = len;
	pt->flags = flags;
	pt->next = next;
	__sync_synchronize();
}

static inline void
fill_avail(struct virtq_avail* avail, uint16 idx)
{
    volatile struct virtq_avail* pt = (volatile struct virtq_avail*)avail;
    pt->ring[avail->idx % NUM] = idx;
    pt->idx++;
    __sync_synchronize();
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


    // set queue size.
    *R(VIRTIO_MMIO_QUEUE_NUM) = NUM;

    if (qnum == 0) {
        for (int i = 0; i < NUM; i++) {
        fill_desc(&queue->desc[i], (uint64)&net.rx_head[i],
                sizeof(struct virtio_net_rx_hdr), VRING_DESC_F_WRITE, 0);
        fill_avail(&queue->avail, i);
        }
    }
    
    // write physical addresses.
    *R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)queue->desc;
    *R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)queue->desc >> 32;
    *R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)&queue->avail;
    *R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)&queue->avail >> 32;
    *R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)&queue->used;
    *R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)&queue->used >> 32;

    // queue is ready.
    *R(VIRTIO_MMIO_QUEUE_READY) = 0x1;
    printf("2. queue %d STATUS: %d\n", qnum, *R(VIRTIO_MMIO_QUEUE_READY));
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
    features = VIRTIO_NET_F_MTU | VIRTIO_NET_F_MAC;
    *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

    // tell device that feature negotiation is complete.
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    // re-read status to ensure FEATURES_OK is set.
    status = *R(VIRTIO_MMIO_STATUS);
    if (!(status & VIRTIO_CONFIG_S_FEATURES_OK))
        panic("virtio net FEATURES_OK unset");

    // initialize queue 0 and 1.
    init_virtq(&net.rx, 0);
    init_virtq(&net.tx, 1);

    // set driver ok
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    virtio_net_cfg(R(VIRTIO_MMIO_CONFIG));
    printf("VIRTIO_MMIO_INTERRUPT_STATUS: %x\n", *R(VIRTIO_MMIO_INTERRUPT_STATUS));
}

void virtio_net_send(uint8* buf, uint32 len, int flag) 
{   
    acquire(&net.vnet_lock);
    // printf("Now in send\n");
    // printf("VIRTIO_MMIO_STATUS: %x\n", *R(VIRTIO_MMIO_STATUS));

    // printf("send: ");
    // for (int i = 0; i < len; i++) {
    //     printf("%x ", buf[i]);
    // }
    // printf("\n");

    int idx[2];
    idx[0] = net.tx.used_idx++ % NUM;
    idx[1] = net.tx.used_idx++ % NUM;

    fill_desc(&net.tx.desc[idx[0]], (uint64)&net.tx_head[idx[0]], sizeof(struct virtio_net_hdr), VRING_DESC_F_NEXT, idx[1]);

    fill_desc(&net.tx.desc[idx[1]], (uint64)buf, len, flag, 0);

    __sync_synchronize();

    fill_avail(&net.tx.avail, idx[0]);

    __sync_synchronize();

    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 1; 

    uint16* pt_idx = &net.tx.used.idx;
    uint16* pt_used_idx = &net.tx.used_idx;

    while(*pt_idx == *pt_used_idx) ;
    net.tx.used_idx += 1;
    
    // printf("Now out send\n");
    release(&net.vnet_lock);
}


void 
virtio_net_recv() 
{   
    // printf("Now in receive\n");

    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

    __sync_synchronize();

    // // used_idx 和 idx 比较，确定是否有新数据传入
    // uint16* pt_idx = &net.rx.used.idx;
    // uint16* pt_used_idx = &net.rx.used_idx;

    while (net.rx.used.idx != net.rx.used_idx) {
        acquire(&net.vnet_lock);
        // 如果有新数据传入，used_idx指向ring中新存放的数据
        struct virtq_used_elem pkt = net.rx.used.ring[net.rx.used_idx % NUM]; //未协商VIRTIO_NET_F_MRG_RXBUF，只有一个描述符

        // struct virtq_desc data_desc = net.rx.desc[pkt.id];
        
        uint8* data = net.rx_head[pkt.id].data;// + 10;
        uint data_len = pkt.len - 10;
  
        // buf = (uint8*)memmove((void*)buf, (void*)data, data_len);
        struct mbuf* m = mbuf_alloc();
        char* tmp = mbuf_put(m, data_len);
        if (tmp == 0) {
            printf("error in get data: buf is too big!");
            return;
        }
        fill_avail(&net.rx.avail, pkt.id);
        net.rx.used_idx += 1;

        memmove(tmp, data, data_len);   
        

        // printf("receive %d bytes: ", data_len);
        // for (int i = 0; i < data_len ; i++) {
        //     printf("%x ", buf[i]);
        // }
        // printf("\n");

        __sync_synchronize();

        *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;
        release(&net.vnet_lock);
        net_rx_eth(m);
    }
    // printf("Now out receive\n");
}

void 
virtio_net_intr(void) 
{
    virtio_net_recv();
}

void 
virtio_test(void)
{   
    #define LEN 42
    uint8 buf[LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02, 0xca, 0xfe, 0xf0, 0x0d, 0x01, 0x08, 0x06, 0x00, 0x01,
                0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x02, 0xca, 0xfe, 0xf0, 0x0d, 0x01, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xac, 0x1c, 0x2b, 0xd8};
    virtio_net_send(buf, LEN, 0);
}
