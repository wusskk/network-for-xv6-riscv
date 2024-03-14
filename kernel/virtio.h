//
// virtio device definitions.
// for both the mmio interface, and virtio descriptors.
// only tested with qemu.
//
// the virtio spec:
// https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf
//

// virtio mmio control registers, mapped starting at 0x10001000.
// from qemu virtio_mmio.h
#define VIRTIO_MMIO_MAGIC_VALUE		0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION		0x004 // version; should be 2
#define VIRTIO_MMIO_DEVICE_ID		0x008 // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID		0x00c // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES	0x010
#define VIRTIO_MMIO_DRIVER_FEATURES	0x020
#define VIRTIO_MMIO_QUEUE_SEL		0x030 // select queue, write-only
#define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034 // max size of current queue, read-only
#define VIRTIO_MMIO_QUEUE_NUM		0x038 // size of current queue, write-only
#define VIRTIO_MMIO_QUEUE_READY		0x044 // ready bit
#define VIRTIO_MMIO_QUEUE_NOTIFY	0x050 // write-only
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060 // read-only
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064 // write-only
#define VIRTIO_MMIO_STATUS		0x070 // read/write
#define VIRTIO_MMIO_QUEUE_DESC_LOW	0x080 // physical address for descriptor table, write-only
#define VIRTIO_MMIO_QUEUE_DESC_HIGH	0x084
#define VIRTIO_MMIO_DRIVER_DESC_LOW	0x090 // physical address for available ring, write-only
#define VIRTIO_MMIO_DRIVER_DESC_HIGH	0x094
#define VIRTIO_MMIO_DEVICE_DESC_LOW	0x0a0 // physical address for used ring, write-only
#define VIRTIO_MMIO_DEVICE_DESC_HIGH	0x0a4

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	1
#define VIRTIO_CONFIG_S_DRIVER		2
#define VIRTIO_CONFIG_S_DRIVER_OK	4
#define VIRTIO_CONFIG_S_FEATURES_OK	8

// device feature bits
#define VIRTIO_BLK_F_RO              5	/* Disk is read-only */
#define VIRTIO_BLK_F_SCSI            7	/* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE     11	/* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ             12	/* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT         27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX     29

// net device feature bits
#define VIRTIO_NET_F_CSUM (0)                // Device handles packets with partial checksum.
#define VIRTIO_NET_F_GUEST_CSUM (1)          // Driver handles packets with partial checksum.
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS (2) // Control channel offloads reconfiguration support.
#define VIRTIO_NET_F_MTU (3)                 // Device maximum MTU reporting is supported
#define VIRTIO_NET_F_MAC (5)                 // Device has given MAC address
#define VIRTIO_NET_F_GUEST_TSO4 (7)          // Driver can receive TSOv4.
#define VIRTIO_NET_F_GUEST_TSO6 (8)          // Driver can receive TSOv6.
#define VIRTIO_NET_F_GUEST_ECN (9)           // Driver can receive TSO with ECN.
#define VIRTIO_NET_F_GUEST_UFO (10)          // Driver can receive UFO.
#define VIRTIO_NET_F_HOST_TSO4 (11)          // Device can receive TSOv4.
#define VIRTIO_NET_F_HOST_TSO6 (12)          // Device can receive TSOv6.
#define VIRTIO_NET_F_HOST_ECN (13)           // Device can receive TSO with ECN.
#define VIRTIO_NET_F_HOST_UFO (14)           // Device can receive UFO.
#define VIRTIO_NET_F_MRG_RXBUF (15)          // Driver can merge receive buffers.
#define VIRTIO_NET_F_STATUS (16)             // Configuration status field is available.
#define VIRTIO_NET_F_CTRL_VQ (17)            // Control channel is available.
#define VIRTIO_NET_F_CTRL_RX (18)            // Control channel RX mode support.
#define VIRTIO_NET_F_CTRL_VLAN (19)          // Control channel VLAN filtering.
#define VIRTIO_NET_F_GUEST_ANNOUNCE (21)     // Driver can send gratuitous packets.
#define VIRTIO_NET_F_MQ (22)                 // Device supports multiqueue with automatic receive steering.
#define VIRTIO_NET_F_CTRL_MAC_ADDR (23)      // Set MAC address through control channel.
#define VIRTIO_NET_F_HOST_USO (56)           // Device can receive USO packets.
#define VIRTIO_NET_F_HASH_REPORT (57)        // Device can report per-packet hash value and a type of calculated hash.
#define VIRTIO_NET_F_GUEST_HDRLEN (59)       // Driver can provide the exact hdr_len value.
#define VIRTIO_NET_F_RSS (60)                // Device supports RSS (receive-side scaling)
#define VIRTIO_NET_F_RSC_EXT (61)            // Device can process duplicated ACKs and report number of coalesced segments and duplicated ACKs.
#define VIRTIO_NET_F_STANDBY (62)            // Device may act as a standby for a primary device with the same MAC address.
#define VIRTIO_NET_F_SPEED_DUPLEX (63)       // Device reports speed and duplex.

// this many virtio descriptors.
// must be a power of two.
#define NUM 8

// a single descriptor, from the spec.
struct virtq_desc {
  uint64 addr;
  uint32 len;
  uint16 flags;
  uint16 next;
};
#define VRING_DESC_F_NEXT  1 // chained with another descriptor
#define VRING_DESC_F_WRITE 2 // device writes (vs read)

// the (entire) avail ring, from the spec.
struct virtq_avail {
  uint16 flags; // always zero
  uint16 idx;   // driver will write ring[idx] next
  uint16 ring[NUM]; // descriptor numbers of chain heads
  uint16 unused;
};

// one entry in the "used" ring, with which the
// device tells the driver about completed requests.
struct virtq_used_elem {
  uint32 id;   // index of start of completed descriptor chain
  uint32 len;
};

struct virtq_used {
  uint16 flags; // always zero
  uint16 idx;   // device increments when it adds a ring[] entry
  struct virtq_used_elem ring[NUM];
};

// these are specific to virtio block devices, e.g. disks,
// described in Section 5.2 of the spec.

#define VIRTIO_BLK_T_IN  0 // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

// the format of the first descriptor in a disk request.
// to be followed by two more descriptors containing
// the block, and a one-byte status.
struct virtio_blk_req {
  uint32 type; // VIRTIO_BLK_T_IN or ..._OUT
  uint32 reserved;
  uint64 sector;
};
