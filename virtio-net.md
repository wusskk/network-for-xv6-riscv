### 5.1 网络设备

virtio网络设备是一个虚拟的以太网网卡，是目前virtio支持的设备中最为复杂的。它支持通过新增特性来增强功能，这一点在其发展历程中表现得尤为明显。空的缓冲区被放置在一个virtqueue中用于接收数据包，而外发的数据包则被排队到另一个virtqueue中进行传输。第三个命令队列用于控制高级过滤特性。

#### 5.1.1 设备ID

- 设备ID为1。

#### 5.1.2 Virtqueues

- 0 receiveq1
- 1 transmitq1
- ...（如果未协商VIRTIO_NET_F_MQ或VIRTIO_NET_F_RSS，则N=1；否则N由max_virtqueue_pairs设置）
- controlq仅在设置了VIRTIO_NET_F_CTRL_VQ时存在。

#### 5.1.3 特性位

- VIRTIO_NET_F_CSUM (0) 设备处理带有部分校验和的数据包。这种“校验和卸载”是现代网卡上的常见特性。
- VIRTIO_NET_F_GUEST_CSUM (1) 驱动程序处理带有部分校验和的数据包。
- VIRTIO_NET_F_CTRL_GUEST_OFFLOADS (2) 控制通道卸载重新配置支持。
- VIRTIO_NET_F_MTU(3) 支持设备最大MTU报告。如果设备提供，则设备会通知驱动程序其最大MTU的值。如果协商成功，驱动程序将使用mtu作为最大MTU值。
- VIRTIO_NET_F_MAC (5) 设备已提供MAC地址。
- VIRTIO_NET_F_GUEST_TSO4 (7) 驱动程序可以接收TSOv4。
- VIRTIO_NET_F_GUEST_TSO6 (8) 驱动程序可以接收TSOv6。
- VIRTIO_NET_F_GUEST_ECN (9) 驱动程序可以接收带有ECN的TSO。
- VIRTIO_NET_F_GUEST_UFO (10) 驱动程序可以接收UFO。
- VIRTIO_NET_F_HOST_TSO4 (11) 设备可以接收TSOv4。
- VIRTIO_NET_F_HOST_TSO6 (12) 设备可以接收TSOv6。
- VIRTIO_NET_F_HOST_ECN (13) 设备可以接收带有ECN的TSO。
- VIRTIO_NET_F_HOST_UFO (14) 设备可以接收UFO。
- VIRTIO_NET_F_MRG_RXBUF (15) 驱动程序可以合并接收缓冲区。
- VIRTIO_NET_F_STATUS (16) 可用配置状态字段。
- VIRTIO_NET_F_CTRL_VQ (17) 可用控制通道。
- VIRTIO_NET_F_CTRL_RX (18) 控制通道RX模式支持。
- VIRTIO_NET_F_CTRL_VLAN (19) 控制通道VLAN过滤。
- VIRTIO_NET_F_GUEST_ANNOUNCE(21) 驱动程序可以发送无目的数据包。
- VIRTIO_NET_F_MQ(22) 设备支持多队列并具有自动接收引导功能。
- VIRTIO_NET_F_CTRL_MAC_ADDR(23) 通过控制通道设置MAC地址。
- VIRTIO_NET_F_HOST_USO (56) 设备可以接收USO数据包。与UFO（分割数据包）不同，USO在每个较小的数据包都有UDP头的情况下，将大型UDP数据包分割成几个段。
- VIRTIO_NET_F_HASH_REPORT(57) 设备可以报告每数据包的哈希值及其计算哈希的类型。
- VIRTIO_NET_F_GUEST_HDRLEN(59) 驱动程序可以提供确切的hdr_len值。设备从知道确切的头部长度中受益。
- VIRTIO_NET_F_RSS(60) 设备支持RSS（接收端扩展）并使用Toeplitz哈希计算和可配置的哈希参数进行接收引导。
- VIRTIO_NET_F_RSC_EXT(61) 设备可以处理重复的ACK并报告合并段的数量和重复的ACK数量。
- VIRTIO_NET_F_STANDBY(62) 设备可以作为具有相同MAC地址的主设备的备用设备。
- VIRTIO_NET_F_SPEED_DUPLEX(63) 设备报告速度和双工模式。

#### 5.1.3.1 特性位要求

一些网络特性位需要其他网络特性位（见2.2.1）：

- VIRTIO_NET_F_GUEST_TSO4 需要 VIRTIO_NET_F_GUEST_CSUM。
- VIRTIO_NET_F_GUEST_TSO6 需要 VIRTIO_NET_F_GUEST_CSUM。
- VIRTIO_NET_F_GUEST_ECN 需要 VIRTIO_NET_F_GUEST_TSO4 或 VIRTIO_NET_F_GUEST_TSO6。
- VIRTIO_NET_F_GUEST_UFO 需要 VIRTIO_NET_F_GUEST_CSUM。
- VIRTIO_NET_F_HOST_TSO4 需要 VIRTIO_NET_F_CSUM。
- VIRTIO_NET_F_HOST_TSO6 需要 VIRTIO_NET_F_CSUM。
- VIRTIO_NET_F_HOST_ECN 需要 VIRTIO_NET_F_HOST_TSO4 或 VIRTIO_NET_F_HOST_TSO6。
- VIRTIO_NET_F_HOST_UFO 需要 VIRTIO_NET_F_CSUM。
- VIRTIO_NET_F_HOST_USO 需要 VIRTIO_NET_F_CSUM。
- VIRTIO_NET_F_CTRL_RX 需要 VIRTIO_NET_F_CTRL_VQ。
- VIRTIO_NET_F_CTRL_VLAN 需要 VIRTIO_NET_F_CTRL_VQ。
- VIRTIO_NET_F_GUEST_ANNOUNCE 需要 VIRTIO_NET_F_CTRL_VQ。
- VIRTIO_NET_F_MQ 需要 VIRTIO_NET_F_CTRL_VQ。
- VIRTIO_NET_F_CTRL_MAC_ADDR 需要 VIRTIO_NET_F_CTRL_VQ。
- VIRTIO_NET_F_RSC_EXT 需要 VIRTIO_NET_F_HOST_TSO4 或 VIRTIO_NET_F_HOST_TSO6。
- VIRTIO_NET_F_RSS 需要 VIRTIO_NET_F_CTRL_VQ。

#### 5.1.3.2 传统接口：特性位

- VIRTIO_NET_F_GSO (6) 设备处理带有任何GSO类型的数据包。这原本是用来指示分段卸载支持的，但经过进一步调查发现需要多个位。
- VIRTIO_NET_F_GUEST_RSC4 (41) 设备合并TCPIP v4数据包。这是由虚拟化软件开发商用补丁实现的，用于认证目的，当前的Windows驱动程序依赖于它。如果virtio-net设备报告此特性，则它将无法工作。
- VIRTIO_NET_F_GUEST_RSC6 (42) 设备合并TCPIP v6数据包。与VIRTIO_NET_F_GUEST_RSC4类似。


#### 5.1.4 设备配置布局

设备配置字段如下，对于驱动程序来说是只读的。MAC地址字段总是存在的（尽管仅在设置了VIRTIO_NET_F_MAC时才有效），状态字段仅在设置了VIRTIO_NET_F_STATUS时存在。当前为驱动程序定义了两个只读位（用于状态字段）：VIRTIO_NET_S_LINK_UP 和 VIRTIO_NET_S_ANNOUNCE。

```c
#define VIRTIO_NET_S_LINK_UP 1
#define VIRTIO_NET_S_ANNOUNCE 2
```

以下驱动程序只读字段 `max_virtqueue_pairs` 仅在设置了VIRTIO_NET_F_MQ或VIRTIO_NET_F_RSS时存在。此字段指定了在至少协商了这些特性之一后，可以配置的每个发送和接收virtqueues（分别是receiveq1...receiveqN和transmitq1...transmitqN）的最大数量。

以下驱动程序只读字段 `mtu` 仅在设置了VIRTIO_NET_F_MTU时存在。此字段指定了驱动程序使用的MTU的最大值。

以下两个字段，速度和双工，仅在设置了VIRTIO_NET_F_SPEED_DUPLEX时存在。

速度字段包含设备速度，单位为1 Mbit每秒，0到0x7fffffff，或者0xffffffff表示速度未知。

双工字段的值为0x01表示全双工，0x00表示半双工，0xff表示双工状态未知。

速度和双工都可能改变，因此驱动程序在接收到配置更改通知后预期将重新读取这些值。

```c
struct virtio_net_config {
    u8 mac[6];
    le16 status;
    le16 max_virtqueue_pairs;
    le16 mtu;
    le32 speed;
    u8 duplex;
    u8 rss_max_key_size;
    le16 rss_max_indirection_table_length;
    le32 supported_hash_types;
};
```

以下字段 `rss_max_key_size` 仅在设置了VIRTIO_NET_F_RSS或VIRTIO_NET_F_HASH_REPORT时存在。它指定了RSS密钥的最大支持长度（以字节为单位）。

以下字段 `rss_max_indirection_table_length` 仅在设置了VIRTIO_NET_F_RSS时存在。它指定了RSS间接表中16位条目的最大数量。

下一个字段 `supported_hash_types` 仅在设备支持哈希计算时存在，即如果设置了VIRTIO_NET_F_RSS或VIRTIO_NET_F_HASH_REPORT。

字段 `supported_hash_types` 包含支持的哈希类型的位掩码。有关支持的哈希类型的详细信息，请参见5.1.6.4.3.1。


#### 5.1.4.1 设备要求：设备配置布局

设备必须将 `max_virtqueue_pairs` 设置为1到0x8000之间的值（包含两端），如果它提供了VIRTIO_NET_F_MQ特性。
设备必须将 `mtu` 设置为68到65535之间的值（包含两端），如果它提供了VIRTIO_NET_F_MTU特性。
设备应当将 `mtu` 设置为至少1280，如果它提供了VIRTIO_NET_F_MTU特性。
设备不得在设置了 `mtu` 之后修改它。
设备不得传递超过 `mtu`（加上低级以太网头长度）大小的接收数据包，这些数据包在VIRTIO_NET_F_MTU成功协商后，使用gso_type NONE或ECN且未进行分割。
设备必须转发传输数据包，大小最多为 `mtu`（加上低级以太网头长度），在VIRTIO_NET_F_MTU成功协商后，不进行分割。
设备必须将 `rss_max_key_size` 设置为至少40，如果它提供了VIRTIO_NET_F_RSS或VIRTIO_NET_F_HASH_REPORT特性。
设备必须将 `rss_max_indirection_table_length` 设置为至少128，如果它提供了VIRTIO_NET_F_RSS特性。
如果驱动程序协商了VIRTIO_NET_F_STANDBY特性，设备可以作为具有相同MAC地址的主设备的备用设备。
如果协商了VIRTIO_NET_F_SPEED_DUPLEX特性，速度必须包含设备速度，单位为1 Mbit每秒，0到0x7fffffff，或者0xffffffff表示速度未知。
如果协商了VIRTIO_NET_F_SPEED_DUPLEX特性，双工模式必须具有0x00表示全双工，0x01表示半双工，或者0xff表示双工状态未知的值。
如果同时协商了VIRTIO_NET_F_SPEED_DUPLEX和VIRTIO_NET_F_STATUS特性，设备在状态中设置了VIRTIO_NET_S_LINK_UP时，不应改变速度和双工字段。

#### 5.1.4.2 驱动程序要求：设备配置布局

驱动程序应当协商VIRTIO_NET_F_MAC特性，如果设备提供了它。如果驱动程序协商了VIRTIO_NET_F_MAC特性，驱动程序必须将物理地址设置为MAC。否则，它应当使用本地管理的MAC地址（参见IEEE 802，“9.2 48位通用局域网MAC地址”）。
如果驱动程序没有协商VIRTIO_NET_F_STATUS特性，它应当假设链路是活动的，否则它应当从状态的最低位读取链路状态。
驱动程序应当协商VIRTIO_NET_F_MTU特性，如果设备提供了它。
如果驱动程序协商了VIRTIO_NET_F_MTU特性，它必须提供足够的接收缓冲区，以接收至少一个大小为 `mtu`（加上低级以太网头长度）的接收数据包，使用gso_type NONE或ECN。
如果驱动程序协商了VIRTIO_NET_F_MTU特性，它不得传输大小超过 `mtu`（加上低级以太网头长度）的数据包，使用gso_type NONE或ECN。
驱动程序应当协商VIRTIO_NET_F_STANDBY特性，如果设备提供了它。
如果协商了VIRTIO_NET_F_SPEED_DUPLEX特性，驱动程序必须将速度值高于0x7fffffff的任何值以及双工模式值不匹配0x00或0x01的任何值视为未知值。
如果协商了VIRTIO_NET_F_SPEED_DUPLEX特性，驱动程序在配置更改通知后应当重新读取速度和双工模式。

#### 5.1.4.3 传统接口：设备配置布局

当使用传统接口时，过渡设备和驱动程序必须根据客户端的原生端序格式化 `struct virtio_net_config` 中的状态和 `max_virtqueue_pairs`，而不是（在不使用传统接口时不一定）小端序。
当使用传统接口时，MAC是驱动程序可写的，这提供了一种方式让驱动程序在不协商VIRTIO_NET_F_CTRL_MAC_ADDR的情况下更新MAC。
传统接口下，`virtio_net_hdr` 结构体中的字段按照客户端的原生端序格式化，而不是小端序。

#### 5.1.5 设备初始化

驱动程序将执行典型的初始化例程，如下所示：

1. 识别并初始化接收和传输virtqueues，每种类型最多N个。如果协商了VIRTIO_NET_F_MQ特性，则N=max_virtqueue_pairs，否则识别N=1。
2. 如果协商了VIRTIO_NET_F_CTRL_VQ特性，识别控制virtqueue。
3. 用缓冲区填充接收队列：参见5.1.6.3。
4. 即使有VIRTIO_NET_F_MQ，默认只使用receiveq1、transmitq1和controlq。驱动程序将发送VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET命令，指定要使用的传输和接收队列的数量。
5. 如果设置了VIRTIO_NET_F_MAC特性，配置空间中的MAC条目指示网络卡的“物理”地址，否则驱动程序通常会生成一个随机的本地MAC地址。
6. 如果协商了VIRTIO_NET_F_STATUS特性，链路状态来自状态的最低位。否则，驱动程序假设它是活动的。
7. 一个高性能的驱动程序将通过协商VIRTIO_NET_F_CSUM特性来指示它将生成无校验和的数据包。
8. 如果协商了该特性，驱动程序可以使用TCP分段或UDP分段/碎片卸载，通过协商VIRTIO_NET_F_HOST_TSO4（IPv4 TCP）、VIRTIO_NET_F_HOST_TSO6（IPv6 TCP）、VIRTIO_NET_F_HOST_UFO（UDP碎片化）和VIRTIO_NET_F_HOST_USO（UDP分段）特性。相反的特性也可用：驱动程序可以通过协商这些特性来节省虚拟设备的一些工作。
9. 一个真正最小的驱动程序将只接受VIRTIO_NET_F_MAC，忽略其他所有特性。

#### 5.1.6 设备操作

通过将数据包放置在transmitq1...transmitqN中进行传输，接收数据包的缓冲区放置在receiveq1...receiveqN中。在每种情况下，数据包前都有一个头部：

```c
struct virtio_net_hdr {
    #define VIRTIO_NET_HDR_F_NEEDS_CSUM 1
    #define VIRTIO_NET_HDR_F_DATA_VALID 2
    #define VIRTIO_NET_HDR_F_RSC_INFO 4
    u8 flags;
    #define VIRTIO_NET_HDR_GSO_NONE 0
    #define VIRTIO_NET_HDR_GSO_TCPV4 1
    #define VIRTIO_NET_HDR_GSO_UDP 3
    #define VIRTIO_NET_HDR_GSO_TCPV6 4
    #define VIRTIO_NET_HDR_GSO_UDP_L4 5
    #define VIRTIO_NET_HDR_GSO_ECN 0x80
    u8 gso_type;
    le16 hdr_len;
    le16 gso_size;
    le16 csum_start;
    le16 csum_offset;
    le16 num_buffers;
    le32 hash_value; /* 仅当协商了VIRTIO_NET_F_HASH_REPORT时 */
    le16 hash_report; /* 仅当协商了VIRTIO_NET_F_HASH_REPORT时 */
    le16 padding_reserved; /* 仅当协商了VIRTIO_NET_F_HASH_REPORT时 */
};
```

controlq用于控制设备特性，如过滤。

#### 5.1.6.1 传统接口：设备操作

当使用传统接口时，过渡设备和驱动程序必须按照客户端的原生端序格式化 `struct virtio_net_hdr` 中的字段，而不是（在不使用传统接口时不一定）小端序。

传统接口下的驱动程序只在协商了 VIRTIO_NET_F_MRG_RXBUF 特性时才在 `struct virtio_net_hdr` 中呈现 `num_buffers`。如果没有协商该特性，该结构体将短2个字节。

当使用传统接口时，驱动程序应忽略传输队列和 controlq 队列的已使用长度。

注意：历史上，一些设备即使没有实际写入数据，也会将总描述符长度放在那里。

#### 5.1.6.2 数据包传输

传输单个数据包是简单的，但根据驱动程序协商的不同特性而有所不同。

1. 驱动程序可以发送一个完全校验和的数据包。在这种情况下，flags 将为零，gso_type 将为 VIRTIO_NET_HDR_GSO_NONE。
2. 如果驱动程序协商了 VIRTIO_NET_F_CSUM，它可以跳过校验和数据包的计算：
   - flags 设置了 VIRTIO_NET_HDR_F_NEEDS_CSUM，
   - csum_start 设置为数据包内开始校验和的偏移量，
   - csum_offset 表示从 csum_start 之后多少字节放置新的（16位一补数）校验和，由设备完成。
   - 数据包中的 TCP 校验和字段设置为 TCP 伪头的总和，以便用 TCP 标头和主体的一补数校验和替换它将得到正确的结果。
   注意：例如，考虑一个部分校验和的 TCP（IPv4）数据包。它将有一个14字节的以太网标头和20字节的 IP 标头，接着是 TCP 标头（TCP 校验和字段在该标头的16字节处）。csum_start 将是 14+20 = 34（TCP 校验和包括标头），csum_offset 将是 16。
3. 如果驱动程序协商了 VIRTIO_NET_F_HOST_TSO4、TSO6、USO 或 UFO，并且数据包需要 TCP 分段、UDP 分段或碎片化，那么 gso_type 设置为 VIRTIO_NET_HDR_GSO_TCPV4、TCPV6、UDP_L4 或 UDP。（否则，它设置为 VIRTIO_NET_HDR_GSO_NONE）。在这种情况下，可以传输大于1514字节的数据包：元数据指示如何复制数据包标头以将其切割成更小的数据包。其他 gso 字段设置为：
   - 如果协商了 VIRTIO_NET_F_GUEST_HDRLEN 特性，hdr_len 指示需要复制的标头长度。这是从数据包开始到传输有效负载开始的字节数。否则，如果未协商 VIRTIO_NET_F_GUEST_HDRLEN 特性，hdr_len 是对设备的一个提示，表明需要保留多少标头以复制到每个数据包中，通常设置为标头的长度，包括传输标头1。
   注意：一些设备从知道确切的标头长度中受益。
   - gso_size 是每个数据包的最大大小，超出标头（即 MSS）。
   - 如果驱动程序协商了 VIRTIO_NET_F_HOST_ECN 特性，gso_type 中的 VIRTIO_NET_HDR_GSO_ECN 位表示 TCP 数据包设置了 ECN 位2。
1由于各种实现中的 bug，该字段不能作为传输标头大小的保证。2这种情况未被一些较旧的硬件处理，因此在协议中特别指出。
4. num_buffers 设置为零。此字段在传输数据包中未使用。
5. 头部和数据包作为传输队列的一个输出描述符添加，然后设备被通知有新的条目（参见 5.1.5 设备初始化）。

#### 5.1.6.2.1 驱动程序要求：数据包传输

驱动程序必须将 num_buffers 设置为零。
如果未协商 VIRTIO_NET_F_CSUM，驱动程序必须将 flags 设置为零，并应当提供一个完全校验和的数据包给设备。
如果协商了 VIRTIO_NET_F_HOST_TSO4，驱动程序可以设置 gso_type 为 VIRTIO_NET_HDR_GSO_TCPV4 以请求 TCPv4 分段，否则驱动程序不得设置 gso_type 为 VIRTIO_NET_HDR_GSO_TCPV4。
如果协商了 VIRTIO_NET_F_HOST_TSO6，驱动程序可以设置 gso_type 为 VIRTIO_NET_HDR_GSO_TCPV6 以请求 TCPv6 分段，否则驱动程序不得设置 gso_type 为 VIRTIO_NET_HDR_GSO_TCPV6。
如果协商了 VIRTIO_NET_F_HOST_UFO，驱动程序可以设置 gso_type 为 VIRTIO_NET_HDR_GSO_UDP 以请求 UDP 分段，否则驱动程序不得设置 gso_type 为 VIRTIO_NET_HDR_GSO_UDP。
如果协商了 VIRTIO_NET_F_HOST_USO，驱动程序可以设置 gso_type 为 VIRTIO_NET_HDR_GSO_UDP_L4 以请求 UDP 分段，否则驱动程序不得设置 gso_type 为 VIRTIO_NET_HDR_GSO_UDP_L4。
驱动程序不应当发送需要分段卸载的 TCP 数据包，除非协商了 VIRTIO_NET_F_HOST_ECN 特性，在这种情况下，驱动程序必须在 gso_type 中设置 VIRTIO_NET_HDR_GSO_ECN 位。
如果协商了 VIRTIO_NET_F_CSUM 特性，驱动程序可以设置 flags 中的 VIRTIO_NET_HDR_F_NEEDS_CSUM 位，如果是这样：
   1. 驱动程序必须验证数据包校验和在 csum_offset 偏移量从 csum_start 以及所有先前的偏移量；
   2. 驱动程序必须将数据包中存储的校验和设置为 TCP/UDP 伪头；
   3. 驱动程序必须设置 csum_start 和 csum_offset，以便从 csum_start 开始计算一补数校验和直到数据包末尾，并将结果存储在从 csum_start 开始的 csum_offset 偏移量处，这将导致一个完全校验和的数据包；
如果未协商 VIRTIO_NET_F_HOST_TSO4、TSO6、USO 或 UFO 选项，驱动程序必须将 gso_type 设置为 VIRTIO_NET_HDR_GSO_NONE。
如果 gso_type 与 VIRTIO_NET_HDR_GSO_NONE 不同，则驱动程序必须同时设置 VIRTIO_NET_HDR_F_NEEDS_CSUM 位在 flags 中，并且必须设置 gso_size 以指示所需的 MSS。
如果协商了 VIRTIO_NET_F_HOST_TSO4、TSO6、USO 或 UFO 选项：
   - 如果协商了 VIRTIO_NET_F_GUEST_HDRLEN 特性，并且 gso_type 与 VIRTIO_NET_HDR_GSO_NONE 不同，驱动程序必须将 hdr_len 设置为等于标头长度的值，包括传输标头。
   - 如果未协商 VIRTIO_NET_F_GUEST_HDRLEN 特性，或者 gso_type 是 VIRTIO_NET_HDR_GSO_NONE，驱动程序应将 hdr_len 设置为不小于标头长度的值，包括传输标头。
驱动程序应当接受 VIRTIO_NET_F_GUEST_HDRLEN 特性，如果它被提供了，并且如果它能够提供确切的标头长度。
驱动程序不得设置 flags 中的 VIRTIO_NET_HDR_F_DATA_VALID 和 VIRTIO_NET_HDR_F_RSC_INFO 位。

#### 5.1.6.2.2 设备要求：数据包传输

设备必须忽略它不认识的标志位。

如果未设置标志中的 VIRTIO_NET_HDR_F_NEEDS_CSUM 位，设备不得使用 csum_start 和 csum_offset。

如果协商了 VIRTIO_NET_F_HOST_TSO4、TSO6、USO 或 UFO 选项之一：

- 如果协商了 VIRTIO_NET_F_GUEST_HDRLEN 特性，并且 gso_type 与 VIRTIO_NET_HDR_GSO_NONE 不同，设备可以使用 hdr_len 作为传输标头大小。
  注意：应谨慎实现，以防止恶意驱动程序通过设置错误的 hdr_len 攻击设备。
- 如果未协商 VIRTIO_NET_F_GUEST_HDRLEN 特性，或者 gso_type 是 VIRTIO_NET_HDR_GSO_NONE，设备可以将 hdr_len 仅作为传输标头大小的提示。设备不得依赖 hdr_len 是正确的。
  注意：这是由于实现中的各种错误。

如果未设置 VIRTIO_NET_HDR_F_NEEDS_CSUM，设备不得依赖数据包校验和是正确的。

#### 5.1.6.2.3 数据包传输中断

通常，驱动程序会抑制传输 virtqueue 中断，并在后续数据包的传输路径中检查已使用的包。

在这种中断处理程序中，正常的行为是从 virtqueue 中检索已使用的缓冲区，并释放相应的标头和数据包。

#### 5.1.6.3 设置接收缓冲区

通常，最好尽可能保持接收 virtqueue 充满：如果它耗尽了，网络性能将受到影响。

如果使用了 VIRTIO_NET_F_GUEST_TSO4、VIRTIO_NET_F_GUEST_TSO6 或 VIRTIO_NET_F_GUEST_UFO 特性，最大的传入数据包将长达 65550 字节（TCP 或 UDP 数据包的最大大小，加上 14 字节的以太网标头），否则为 1514 字节。12 字节的 struct virtio_net_hdr 将被添加到这个长度之前，使得总长度为 65562 或 1526 字节。

#### 5.1.6.3.1 驱动程序要求：设置接收缓冲区

- 如果未协商 VIRTIO_NET_F_MRG_RXBUF：
  - 如果协商了 VIRTIO_NET_F_GUEST_TSO4、VIRTIO_NET_F_GUEST_TSO6 或 VIRTIO_NET_F_GUEST_UFO，驱动程序应以至少 65562 字节的缓冲区填充接收队列。
  - 否则，驱动程序应以至少 1526 字节的缓冲区填充接收队列。
- 如果协商了 VIRTIO_NET_F_MRG_RXBUF，每个缓冲区必须至少是 struct virtio_net_hdr 的大小。
  注意：显然，每个缓冲区可以跨越多个描述符元素。

如果协商了 VIRTIO_NET_F_MQ，将使用的每个 receiveq1...receiveqN 都应填充接收缓冲区。

#### 5.1.6.3.2 设备要求：设置接收缓冲区

设备必须将 num_buffers 设置为接收数据包使用的描述符数量。

如果未协商 VIRTIO_NET_F_MRG_RXBUF，设备必须仅使用单个描述符。

注意：这意味着如果未协商 VIRTIO_NET_F_MRG_RXBUF，num_buffers 总是 1。

#### 5.1.6.4 接收数据包处理

当一个数据包被复制到接收队列的缓冲区中时，最佳做法是禁用进一步的已用缓冲区通知，处理数据包直到找不到更多，然后重新启用它们。

处理传入数据包涉及：

1. num_buffers 指示数据包跨越的描述符数量（包括这一个）：如果未协商 VIRTIO_NET_F_MRG_RXBUF，则总是1。这允许接收大数据包而无需分配大缓冲区：不适合单个缓冲区的数据包可以流向下一个缓冲区，依此类推。在这种情况下，至少会有 num_buffers 个已用的缓冲区在 virtqueue 中，设备将它们链接在一起形成单个数据包，类似于它将数据包存储在单个跨越多个描述符的大缓冲区中。其他缓冲区不会以 struct virtio_net_hdr 开始。

2. 如果 num_buffers 是一，那么整个数据包将包含在此缓冲区内，紧随 struct virtio_net_hdr 之后。

3. 如果协商了 VIRTIO_NET_F_GUEST_CSUM 特性，flags 中的 VIRTIO_NET_HDR_F_DATA_VALID 位可以被设置：如果是这样，设备已经验证了数据包校验和。在多层封装协议的情况下，已经验证了一个层级的校验和。

此外，VIRTIO_NET_F_GUEST_CSUM、TSO4、TSO6、UDP 和 ECN 特性启用了接收校验和、大接收卸载和 ECN 支持，这些是传输校验和、传输分段卸载和 ECN 特性的输入等效特性，如 5.1.6.2 所述：

1. 如果协商了 VIRTIO_NET_F_GUEST_TSO4、TSO6 或 UFO 选项，那么 gso_type 可能是除 VIRTIO_NET_HDR_GSO_NONE 之外的其他值，gso_size 字段指示所需的 MSS（参见数据包传输第2点）。
2. 如果协商了 VIRTIO_NET_F_RSC_EXT 选项（这暗示了 VIRTIO_NET_F_GUEST_TSO4、TSO6 之一），设备还处理重复的 ACK 段，在 csum_start 字段中报告合并的 TCP 段数量，在 csum_offset 字段中报告重复的 ACK 段数量，并在 flags 中设置 VIRTIO_NET_HDR_F_RSC_INFO 位。
3. 如果协商了 VIRTIO_NET_F_GUEST_CSUM 特性，flags 中的 VIRTIO_NET_HDR_F_NEEDS_CSUM 位可以被设置：如果是这样，设备必须验证数据包校验和在 csum_offset 偏移量从 csum_start 以及所有先前的偏移量；设备必须将数据包中存储的校验和设置为 TCP/UDP 伪头；设备必须设置 csum_start 和 csum_offset，以便从 csum_start 开始计算一补数校验和直到数据包末尾，并将结果存储在从 csum_start 开始的 csum_offset 偏移量处，这将导致一个完全校验和的数据包。

如果适用，设备为传入数据包计算每包哈希，如 5.1.6.4.3 所述。

如果适用，设备报告传入数据包的哈希信息，如 5.1.6.4.4 所述。

#### 5.1.6.4.1 设备要求：接收数据包处理

如果未协商 VIRTIO_NET_F_MRG_RXBUF，设备必须将 num_buffers 设置为 1。

如果协商了 VIRTIO_NET_F_MRG_RXBUF，设备必须将 num_buffers 设置为指示数据包（包括标头）跨越的缓冲区数量。

如果接收数据包跨越多个缓冲区，设备必须使用除最后一个之外的所有缓冲区（即第一个 num_buffers - 1 个缓冲区）完全填充到驱动程序提供的每个缓冲区的全长。

设备必须使用单个接收数据包的所有缓冲区，以便驱动程序观察到至少 num_buffers 个已使用的缓冲区。

如果未协商 VIRTIO_NET_F_GUEST_CSUM，设备必须将 flags 设置为零，并应向驱动程序提供一个完全校验和的数据包。

如果未协商 VIRTIO_NET_F_GUEST_TSO4，设备不得将 gso_type 设置为 VIRTIO_NET_HDR_GSO_TCPV4。

#### 5.1.6.4.2 驱动程序要求：处理传入数据包

驱动程序必须忽略它不认识的标志位。

如果未设置 VIRTIO_NET_HDR_F_NEEDS_CSUM 位或如果设置了 VIRTIO_NET_HDR_F_RSC_INFO 位标志，则驱动程序不得使用 csum_start 和 csum_offset。

如果协商了 VIRTIO_NET_F_GUEST_TSO4、TSO6 或 UFO 选项，驱动程序可以将 hdr_len 仅用作传输标头大小的提示。驱动程序不得依赖 hdr_len 是正确的。

注意：这是由于实现中的各种错误。

如果未设置 VIRTIO_NET_HDR_F_NEEDS_CSUM 或 VIRTIO_NET_HDR_F_DATA_VALID，则驱动程序不得依赖数据包校验和是正确的。

#### 5.1.6.4.3 传入数据包的哈希计算

设备在以下情况下尝试为每个数据包计算哈希：

- 协商了 VIRTIO_NET_F_RSS 特性。设备使用哈希来确定将传入数据包放置在哪个接收 virtqueue。
- 协商了 VIRTIO_NET_F_HASH_REPORT 特性。设备报告数据包的哈希值和哈希类型。

如果协商了 VIRTIO_NET_F_RSS 特性：

- 设备使用 virtio_net_rss_config 结构中的 hash_types 作为“启用哈希类型”的位掩码。
- 设备使用 virtio_net_rss_config 结构中的 hash_key_data 和 hash_key_length 定义的密钥（参见 5.1.6.5.7.1）。

如果未协商 VIRTIO_NET_F_RSS 特性：

- 设备使用 virtio_net_hash_config 结构中的 hash_types 作为“启用哈希类型”的位掩码。
- 设备使用 virtio_net_hash_config 结构中的 hash_key_data 和 hash_key_length 定义的密钥（参见 5.1.6.5.6.4）。

注意：如果设备提供 VIRTIO_NET_F_HASH_REPORT，即使它只支持一对 virtqueues，它也必须支持至少一个 VIRTIO_NET_CTRL_MQ 类的命令来配置报告的哈希参数：

- 如果设备提供 VIRTIO_NET_F_RSS，它必须支持 VIRTIO_NET_CTRL_MQ_RSS_CONFIG 命令，根据 5.1.6.5.7.1。
- 否则，设备必须支持 VIRTIO_NET_CTRL_MQ_HASH_CONFIG 命令，根据 5.1.6.5.6.4。

#### 5.1.6.4.3.1 支持/启用的哈希类型

适用于 IPv4 数据包的哈希类型：

```c
#define VIRTIO_NET_HASH_TYPE_IPv4 (1 << 0)
#define VIRTIO_NET_HASH_TYPE_TCPv4 (1 << 1)
#define VIRTIO_NET_HASH_TYPE_UDPv4 (1 << 2)
```

适用于没有扩展头的 IPv6 数据包的哈希类型：

```c
#define VIRTIO_NET_HASH_TYPE_IPv6 (1 << 3)
#define VIRTIO_NET_HASH_TYPE_TCPv6 (1 << 4)
#define VIRTIO_NET_HASH_TYPE_UDPv6 (1 << 5)
```

适用于具有扩展头的 IPv6 数据包的哈希类型：

```c
#define VIRTIO_NET_HASH_TYPE_IP_EX (1 << 6)
#define VIRTIO_NET_HASH_TYPE_TCP_EX (1 << 7)
#define VIRTIO_NET_HASH_TYPE_UDP_EX (1 << 8)
```

#### 5.1.6.4.3.2 IPv4 数据包

设备根据“启用哈希类型”的位掩码计算 IPv4 数据包的哈希，如下所示：

- 如果设置了 VIRTIO_NET_HASH_TYPE_TCPv4 并且数据包具有 TCP 标头，则哈希计算以下字段：
  - 源 IP 地址
  - 目的 IP 地址
  - 源 TCP 端口
  - 目的 TCP 端口
- 否则，如果设置了 VIRTIO_NET_HASH_TYPE_UDPv4 并且数据包具有 UDP 标头，则哈希计算以下字段：
  - 源 IP 地址
  - 目的 IP 地址
  - 源 UDP 端口
  - 目的 UDP 端口
- 否则，如果设置了 VIRTIO_NET_HASH_TYPE_IPv4，则哈希计算以下字段：
  - 源 IP 地址
  - 目的 IP 地址
- 否则，设备不计算哈希。

#### 5.1.6.4.3.3 没有扩展头的 IPv6 数据包

设备根据“启用哈希类型”的位掩码计算没有扩展头的 IPv6 数据包的哈希，如下所示：

- 如果设置了 VIRTIO_NET_HASH_TYPE_TCPv6 并且数据包具有 TCPv6 标头，则哈希计算以下字段：
  - 源 IPv6 地址
  - 目的 IPv6 地址
  - 源 TCP 端口
  - 目的 TCP 端口
- 否则，如果设置了 VIRTIO_NET_HASH_TYPE_UDPv6 并且数据包具有 UDPv6 标头，则哈希计算以下字段：
  - 源 IPv6 地址
  - 目的 IPv6 地址
  - 源 UDP 端口
  - 目的 UDP 端口
- 否则，如果设置了 VIRTIO_NET_HASH_TYPE_IPv6，则哈希计算以下字段：
  - 源 IPv6 地址
  - 目的 IPv6 地址
- 否则，设备不计算哈希。

#### 5.1.6.4.3.4 具有扩展头的 IPv6 数据包

设备根据“启用哈希类型”的位掩码计算具有扩展头的 IPv6 数据包的哈希，如下所示：

- 如果设置了 VIRTIO_NET_HASH_TYPE_TCP_EX 并且数据包具有 TCPv6 标头，则哈希计算以下字段：
  - IPv6 目的选项头中的 home 地址选项中的地址。如果扩展头不存在，则使用源 IPv6 地址。
  - 相关扩展头中的 Routing-Header-Type-2 包含的 IPv6 地址。如果扩展头不存在，则使用目的 IPv6 地址。
  - 源 TCP 端口
  - 目的 TCP 端口
- 否则，如果设置了 VIRTIO_NET_HASH_TYPE_UDP_EX 并且数据包具有 UDPv6 标头，则哈希计算以下字段：
  - IPv6 目的选项头中的 home 地址选项中的地址。如果扩展头不存在，则使用源 IPv6 地址。
  - 相关扩展头中的 Routing-Header-Type-2 包含的 IPv6 地址。如果扩展头不存在，则使用目的 IPv6 地址。
  - 源 UDP 端口
  - 目的 UDP 端口
- 否则，如果设置了 VIRTIO_NET_HASH_TYPE_IP_EX，则哈希计算以下字段：
  - IPv6 目的选项头中的 home 地址选项中的地址。如果扩展头不存在，则使用源 IPv6 地址。
  - 相关扩展头中的 Routing-Header-Type-2 包含的 IPv6 地址。如果扩展头不存在，则使用目的 IPv6 地址。
- 否则，跳过 IPv6 扩展头，并按照没有扩展头的 IPv6 数据包的定义计算哈希（参见 5.1.6.4.3.3）。

#### 5.1.6.4.4 传入数据包的哈希报告

如果协商了 VIRTIO_NET_F_HASH_REPORT 并且设备已经为数据包计算了哈希，设备将使用 hash_report 填充计算哈希的报告类型，使用 hash_value 填充计算出的哈希值。

如果协商了 VIRTIO_NET_F_HASH_REPORT 但由于某些原因未计算哈希，设备将把 hash_report 设置为 VIRTIO_NET_HASH_REPORT_NONE。

设备可以在 hash_report 中报告的可能值如下。它们对应于在 5.1.6.4.3.1 中定义的支持的哈希类型，如下所示：

```c
VIRTIO_NET_HASH_TYPE_XXX = 1 << (VIRTIO_NET_HASH_REPORT_XXX - 1)
#define VIRTIO_NET_HASH_REPORT_NONE 0
#define VIRTIO_NET_HASH_REPORT_IPv4 1
#define VIRTIO_NET_HASH_REPORT_TCPv4 2
#define VIRTIO_NET_HASH_REPORT_UDPv4 3
#define VIRTIO_NET_HASH_REPORT_IPv6 4
#define VIRTIO_NET_HASH_REPORT_TCPv6 5
#define VIRTIO_NET_HASH_REPORT_UDPv6 6
#define VIRTIO_NET_HASH_REPORT_IPv6_EX 7
#define VIRTIO_NET_HASH_REPORT_TCPv6_EX 8
#define VIRTIO_NET_HASH_REPORT_UDPv6_EX 9
```

#### 5.1.6.5 控制 Virtqueue

如果驱动程序协商了VIRTIO_NET_F_CTRL_VQ特性，它可以使用控制Virtqueue发送命令来操作设备上的一些特性，这些特性不容易映射到配置空间中。所有命令都采用以下形式：

```c
struct virtio_net_ctrl {
    u8 class;
    u8 command;
    u8 command_specific_data[];
    u8 ack;
};
```

/* ack 值 */
#define VIRTIO_NET_OK 0
#define VIRTIO_NET_ERR 1

类、命令和命令特定数据由驱动程序设置，设备设置ack字节。除了发出诊断外，如果ack不是VIRTIO_NET_OK，它几乎无法做其他事情。

#### 5.1.6.5.1 数据包接收过滤

如果驱动程序协商了VIRTIO_NET_F_CTRL_RX和VIRTIO_NET_F_CTRL_RX_EXTRA特性，它可以发送控制命令来处理混杂模式、多播、单播和广播接收。

注意：一般来说，这些命令都是尽力而为的：不想要的数据包仍然可能到达。

#define VIRTIO_NET_CTRL_RX 0
#define VIRTIO_NET_CTRL_RX_PROMISC 0
#define VIRTIO_NET_CTRL_RX_ALLMULTI 1
#define VIRTIO_NET_CTRL_RX_ALLUNI 2
#define VIRTIO_NET_CTRL_RX_NOMULTI 3
#define VIRTIO_NET_CTRL_RX_NOUNI 4
#define VIRTIO_NET_CTRL_RX_NOBCAST 5

#### 5.1.6.5.1.1 设备要求：数据包接收过滤

如果设备协商了VIRTIO_NET_F_CTRL_RX特性，设备必须支持以下VIRTIO_NET_CTRL_RX类命令：

- VIRTIO_NET_CTRL_RX_PROMISC切换混杂模式的开启和关闭。命令特定数据是一个字节，包含0（关闭）或1（开启）。如果混杂模式开启，设备应该接收所有传入的数据包。即使开启了其他由VIRTIO_NET_CTRL_RX类命令设置的模式，这也应该生效。
- VIRTIO_NET_CTRL_RX_ALLMULTI切换所有多播接收的开启和关闭。命令特定数据是一个字节，包含0（关闭）或1（开启）。当所有多播接收开启时，设备应该允许所有传入的多播数据包。

如果设备协商了VIRTIO_NET_F_CTRL_RX_EXTRA特性，设备必须支持以下VIRTIO_NET_CTRL_RX类命令：

- VIRTIO_NET_CTRL_RX_ALLUNI切换所有单播接收的开启和关闭。命令特定数据是一个字节，包含0（关闭）或1（开启）。当所有单播接收开启时，设备应该允许所有传入的单播数据包。
- VIRTIO_NET_CTRL_RX_NOMULTI抑制多播接收。命令特定数据是一个字节，包含0（允许多播接收）或1（抑制多播接收）。当多播接收被抑制时，设备不应该向驱动程序发送多播数据包。即使VIRTIO_NET_CTRL_RX_ALLMULTI开启，这也应该生效。此过滤器不应用于广播数据包。
- VIRTIO_NET_CTRL_RX_NOUNI抑制单播接收。命令特定数据是一个字节，包含0（允许单播接收）或1（抑制单播接收）。当单播接收被抑制时，设备不应该向驱动程序发送单播数据包。即使VIRTIO_NET_CTRL_RX_ALLUNI开启，这也应该生效。
- VIRTIO_NET_CTRL_RX_NOBCAST抑制广播接收。命令特定数据是一个字节，包含0（允许广播接收）或1（抑制广播接收）。当广播接收被抑制时，设备不应该向驱动程序发送广播数据包。即使VIRTIO_NET_CTRL_RX_ALLMULTI开启，这也应该生效。

#### 5.1.6.5.1.2 驱动程序要求：数据包接收过滤

如果驱动程序没有协商VIRTIO_NET_F_CTRL_RX特性，它不得发出VIRTIO_NET_CTRL_RX_PROMISC或VIRTIO_NET_CTRL_RX_ALLMULTI命令。

如果驱动程序没有协商VIRTIO_NET_F_CTRL_RX_EXTRA特性，它不得发出VIRTIO_NET_CTRL_RX_ALLUNI、VIRTIO_NET_CTRL_RX_NOMULTI、VIRTIO_NET_CTRL_RX_NOUNI或VIRTIO_NET_CTRL_RX_NOBCAST命令。

#### 5.1.6.5.2 设置 MAC 地址过滤

如果驱动程序协商了VIRTIO_NET_F_CTRL_RX特性，它可以发送控制命令来进行MAC地址过滤。

```c
struct virtio_net_ctrl_mac {
    le32 entries;
    u8 macs[entries][6];
};
```

#define VIRTIO_NET_CTRL_MAC 1
#define VIRTIO_NET_CTRL_MAC_TABLE_SET 0
#define VIRTIO_NET_CTRL_MAC_ADDR_SET 1

设备可以根据任意数量的目的MAC地址过滤传入的数据包。这个表通过使用VIRTIO_NET_CTRL_MAC类和VIRTIO_NET_CTRL_MAC_TABLE_SET命令设置。命令特定数据是两个可变长度的6字节MAC地址表（如struct virtio_net_ctrl_mac中所述）。第一个表包含单播地址，第二个表包含多播地址。

VIRTIO_NET_CTRL_MAC_ADDR_SET命令用于设置默认MAC地址，该地址被rx过滤接受（如果协商了VIRTIO_NET_F_MAC，这将反映在配置空间中的mac中）。

VIRTIO_NET_CTRL_MAC_ADDR_SET的命令特定数据是6字节的MAC地址。

#### 5.1.6.5.2.1 设备要求：设置 MAC 地址过滤

设备在重置时必须有一个空的MAC过滤表。

设备必须在消耗VIRTIO_NET_CTRL_MAC_TABLE_SET命令之前更新MAC过滤表。

如果协商了VIRTIO_NET_F_MAC，设备必须在消耗VIRTIO_NET_CTRL_MAC_ADDR_SET命令之前更新配置空间中的mac。

设备应该丢弃目的MAC既不匹配mac（或使用VIRTIO_NET_CTRL_MAC_ADDR_SET设置的地址）也不匹配MAC过滤表的传入数据包。

#### 5.1.6.5.2.2 驱动程序要求：设置 MAC 地址过滤

如果未协商VIRTIO_NET_F_CTRL_RX，驱动程序不得发出VIRTIO_NET_CTRL_MAC类命令。

如果协商了VIRTIO_NET_F_CTRL_RX，驱动程序应该发出VIRTIO_NET_CTRL_MAC_ADDR_SET来设置默认的mac，如果它与mac不同。

驱动程序必须在VIRTIO_NET_CTRL_MAC_TABLE_SET命令之后跟随一个le32数字，然后是该数字的非多播MAC地址数量，然后是另一个le32数字，然后是该数字的多播地址数量。任何一个数字可以是0。

#### 5.1.6.5.2.3 传统接口：设置 MAC 地址过滤

当使用传统接口时，过渡设备和驱动程序必须根据客户端的原生端序格式化struct virtio_net_ctrl_mac中的条目，而不是（在不使用传统接口时不一定）小端序。

传统驱动程序如果没有协商VIRTIO_NET_F_CTRL_MAC_ADDR，会在配置空间中更改mac，当NIC接收传入数据包时。这些驱动程序总是从第一个字节到最后一个字节写入mac值，因此，在检测到此类驱动程序后，过渡设备可以延迟MAC更新，或者可以延迟处理传入数据包，直到驱动程序在配置空间中写入mac的最后一个字节。

#### 5.1.6.5.3 VLAN 过滤

如果驱动程序协商了VIRTIO_NET_F_CTRL_VLAN特性，它可以控制设备中的VLAN过滤表。

注意：与基于MAC地址的过滤类似，VLAN过滤也是尽力而为的：不想要的数据包仍然可能到达。

#define VIRTIO_NET_CTRL_VLAN 2
#define VIRTIO_NET_CTRL_VLAN_ADD 0
#define VIRTIO_NET_CTRL_VLAN_DEL 1

VIRTIO_NET_CTRL_VLAN_ADD和VIRTIO_NET_CTRL_VLAN_DEL命令都采用小端16位VLAN ID作为命令特定数据。

#### 5.1.6.5.3.1 传统接口：VLAN 过滤

当使用传统接口时，过渡设备和驱动程序必须根据客户端的原生端序格式化VLAN ID，而不是（在不使用传统接口时不一定）小端序。

#### 5.1.6.5.4 无代价的数据包发送

如果驱动程序协商了VIRTIO_NET_F_GUEST_ANNOUNCE（依赖于VIRTIO_NET_F_CTRL_VQ），设备可以要求驱动程序发送无代价的数据包；这通常是在客户机物理迁移后完成的，并且需要在新的网络链路上宣布其存在。（由于虚拟化主机没有关于客户机网络配置的知识（例如，标记的VLAN），因此以这种方式促使客户机是最简单的）

#define VIRTIO_NET_CTRL_ANNOUNCE 3
#define VIRTIO_NET_CTRL_ANNOUNCE_ACK 0

驱动程序在注意到设备配置状态字段的变化时检查设备配置状态字段中的VIRTIO_NET_S_ANNOUNCE位。命令VIRTIO_NET_CTRL_ANNOUNCE_ACK用于表示驱动程序已接收到通知，设备在收到命令缓冲区后清除状态中的VIRTIO_NET_S_ANNOUNCE位。

处理此通知涉及：

1. 发送无代价的数据包（例如ARP）或标记有待发送的无代价数据包，并让延迟的例程发送它们。
2.通过控制Virtqueue发送VIRTIO_NET_CTRL_ANNOUNCE命令。

#### 5.1.6.5.4.1 驱动程序要求：发送无代价的数据包

如果驱动程序协商了VIRTIO_NET_F_GUEST_ANNOUNCE，它应该在看到状态中的VIRTIO_NET_S_ANNOUNCE位后通知网络对等体其新位置。驱动程序必须在命令队列上发送一个带有类VIRTIO_NET_CTRL_ANNOUNCE和命令VIRTIO_NET_CTRL_ANNOUNCE_ACK的命令。

#### 5.1.6.5.4.2 设备要求：发送无代价的数据包

如果协商了VIRTIO_NET_F_GUEST_ANNOUNCE，设备在收到带有类VIRTIO_NET_CTRL_ANNOUNCE和命令VIRTIO_NET_CTRL_ANNOUNCE_ACK的命令缓冲区之前，必须清除状态中的VIRTIO_NET_S_ANNOUNCE位，然后将其标记为已使用。

#### 5.1.6.5.5 多队列模式下的设备操作

本规范定义了设备可能实现的以下模式，用于与多个传输/接收Virtqueues操作：

- 自动接收导向，如5.1.6.5.6所定义。如果设备支持此模式，它提供VIRTIO_NET_F_MQ特性位。
- 接收端扩展，如5.1.6.5.7.3所定义。如果设备支持此模式，它提供VIRTIO_NET_F_RSS特性位。

设备可以支持这些特性中的一个或两个。驱动程序可以协商设备支持的任何一组特性。

多队列默认情况下是禁用的。

驱动程序通过发送使用类VIRTIO_NET_CTRL_MQ的命令来启用多队列。命令选择多队列操作的模式，如下所示：

#define VIRTIO_NET_CTRL_MQ 4
#define VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET 0（用于自动接收导向）
#define VIRTIO_NET_CTRL_MQ_RSS_CONFIG 1（用于可配置的接收导向）
#define VIRTIO_NET_CTRL_MQ_HASH_CONFIG 2（用于可配置的哈希计算）

如果协商了多个多队列模式，最终设备配置由驱动程序发送的最后一个命令定义。

#### 5.1.6.5.6 自动接收导向的多队列模式

如果驱动程序协商了VIRTIO_NET_F_MQ特性位（依赖于VIRTIO_NET_F_CTRL_VQ），它可以在多个transmitq1...transmitqN之一上传输外发数据包，并要求设备根据数据包流量将传入数据包排队到多个receiveq1...receiveqN之一。

驱动程序通过发送VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET命令来启用多队列，指定要使用的传输和接收队列的数量，最多为max_virtqueue_pairs；随后，transmitq1...transmitqn和receiveq1...receiveqn（其中n=virtqueue_pairs）可以使用。

```c
struct virtio_net_ctrl_mq_pairs_set {
    le16 virtqueue_pairs;
};
```

#define VIRTIO_NET_CTRL_MQ_VQ_PAIRS_MIN 1
#define VIRTIO_NET_CTRL_MQ_VQ_PAIRS_MAX 0x8000

当通过VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET命令启用多队列时，设备必须使用基于数据包流量的自动接收导向。接收导向分类器的编程是隐含的。在驱动程序传输了一个流量的数据包到transmitqX之后，设备应该使得该流量的传入数据包被导向到receiveqX。对于单向协议，或者还没有传输数据包的情况下，设备可以将数据包导向到指定的receiveq1...receiveqn中的随机队列。

通过将VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET命令的virtqueue_pairs设置为1（这是默认值）并等待设备使用命令缓冲区来禁用多队列。

#### 5.1.6.5.6.1 驱动程序要求：多队列模式中的自动接收导向

驱动程序必须在启用它们之前使用VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET命令配置virtqueues。

驱动程序不得请求0或大于设备配置空间中max_virtqueue_pairs的virtqueue_pairs。

驱动程序必须仅在任何transmitq1上排队数据包，然后才能将VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET命令放入可用环中。

驱动程序一旦将VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET命令放入可用环中，就不得在大于virtqueue_pairs的传输队列上排队数据包。

#### 5.1.6.5.6.2 设备要求：多队列模式中的自动接收导向

在重置初始化后，设备必须仅在receiveq1上排队数据包。

设备一旦将VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET命令放入已使用缓冲区，就不得在大于virtqueue_pairs的接收队列上排队数据包。

#### 5.1.6.5.6.3 传统接口：多队列模式中的自动接收导向

当使用传统接口时，过渡设备和驱动程序必须根据客户端的原生端序格式化virtqueue_pairs，而不是（在不使用传统接口时不一定）小端序。

#### 5.1.6.5.6.4 哈希计算

如果协商了VIRTIO_NET_F_HASH_REPORT并且设备使用自动接收导向，设备必须支持一个命令来配置哈希计算参数。

驱动程序按以下方式提供哈希计算参数：

类VIRTIO_NET_CTRL_MQ，命令VIRTIO_NET_CTRL_MQ_HASH_CONFIG。

命令特定数据具有以下格式：

```c
struct virtio_net_hash_config {
    le32 hash_types;
    le16 reserved[4];
    u8 hash_key_length;
    u8 hash_key_data[hash_key_length];
};
```

字段hash_types包含允许的哈希类型的位掩码，如5.1.6.4.3.1所定义。最初，设备禁用所有哈希类型，仅报告VIRTIO_NET_HASH_REPORT_NONE。

字段reserved必须包含零。它被定义为使结构与virtio_net_rss_config结构的布局相匹配，如5.1.6.5.7.1所定义。

字段hash_key_length和hash_key_data定义在哈希计算中使用的密钥。

#### 5.1.6.5.7 接收端扩展（RSS）

如果设备支持RSS接收端扩展，并提供Toeplitz哈希计算和可配置参数，则提供VIRTIO_NET_F_RSS特性。

驱动程序通过读取设备配置来查询设备的RSS能力，如5.1.4所定义。

#### 5.1.6.5.7.1 设置RSS参数

驱动程序使用以下格式发送VIRTIO_NET_CTRL_MQ_RSS_CONFIG命令：

```c
struct virtio_net_rss_config {
    le32 hash_types;
    le16 indirection_table_mask;
    le16 unclassified_queue;
    le16 indirection_table[indirection_table_length];
    le16 max_tx_vq;
    u8 hash_key_length;
    u8 hash_key_data[hash_key_length];
};
```

字段hash_types包含允许的哈希类型的位掩码，如5.1.6.4.3.1所定义。

字段indirection_table_mask是应用于计算出的哈希的掩码，以产生indirection_table数组中的索引。indirection_table中的条目数量为（indirection_table_mask + 1）。

字段unclassified_queue包含接收Virtqueue的0-based索引，用于放置未分类的数据包。索引0对应于receiveq1。

字段indirection_table包含接收Virtqueues的索引数组。索引0对应于receiveq1。

驱动程序设置max_tx_vq以通知设备它可以使用的传输Virtqueues数量（transmitq1...transmitq max_tx_vq）。

字段hash_key_length和hash_key_data定义在哈希计算中使用的密钥。

#### 5.1.6.5.7.2 驱动程序要求：设置RSS参数

如果未协商VIRTIO_NET_F_RSS特性，驱动程序不得发送VIRTIO_NET_CTRL_MQ_RSS_CONFIG命令。

驱动程序必须仅使用已启用队列的索引填充indirection_table数组。索引0对应于receiveq1。

indirection_table中的条目数量（indirection_table_mask + 1）必须为2的幂。

驱动程序必须使用小于设备报告的rss_max_indirection_table_length的indirection_table_mask值。

驱动程序不得设置设备不支持的任何VIRTIO_NET_HASH_TYPE_标志。

#### 5.1.6.5.7.3 设备要求：RSS处理

设备必须按以下方式确定网络数据包的目标队列：

- 计算数据包的哈希，如5.1.6.4.3所定义。
- 如果设备未为特定数据包计算哈希，则设备将数据包定向到virtio_net_rss_config结构的unclassified_queue指定的receiveq（值0对应于receiveq1）。
- 应用indirection_table_mask到计算出的哈希，并使用结果作为indirection表中的索引，以获取目标receiveq的0-based编号（值0对应于receiveq1）。

#### 5.1.6.5.8 来宾卸载状态配置

如果驱动程序协商了VIRTIO_NET_F_CTRL_GUEST_OFFLOADS特性，它可以发送控制命令来动态配置卸载状态。

#### 5.1.6.5.8.1 设置卸载状态

为了配置卸载状态，使用以下布局结构和定义：

```c
le64 offloads;
#define VIRTIO_NET_F_GUEST_CSUM 1
#define VIRTIO_NET_F_GUEST_TSO4 7
#define VIRTIO_NET_F_GUEST_TSO6 8
#define VIRTIO_NET_F_GUEST_ECN 9
#define VIRTIO_NET_F_GUEST_UFO 10
#define VIRTIO_NET_CTRL_GUEST_OFFLOADS 5
#define VIRTIO_NET_CTRL_GUEST_OFFLOADS_SET 0
```

类VIRTIO_NET_CTRL_GUEST_OFFLOADS有一个命令：VIRTIO_NET_CTRL_GUEST_OFFLOADS_SET应用新的卸载配置。

作为命令数据传递的le64值是一个位掩码，设置的位定义了要启用的卸载，清除的位定义了要禁用的卸载。

#### 5.1.6.5.8.2 驱动程序要求：设置卸载状态

驱动程序不得启用未协商适当特性的卸载。

#### 5.1.6.5.8.3 传统接口：设置卸载状态

在使用传统接口时，过渡设备和驱动程序必须按照客户端的原生端序格式化offloads，而不是（在不使用传统接口时不一定）小端序。

#### 5.1.6.6 传统接口：框架要求

在使用传统接口时，未协商VIRTIO_F_ANY_LAYOUT的过渡驱动程序必须在传输和接收时使用单个描述符来处理struct virtio_net_hdr，网络数据在接下来的描述符中。

此外，在使用控制Virtqueue（参见5.1.6.5）时，未协商VIRTIO_F_ANY_LAYOUT的过渡驱动程序必须：

- 对于所有命令，使用包含前两个字段（类和命令）的单个2字节描述符。
- 对于除VIRTIO_NET_CTRL_MAC_TABLE_SET之外的所有命令，使用包含命令特定数据的单个描述符，无填充。
- 对于VIRTIO_NET_CTRL_MAC_TABLE_SET命令，使用恰好两个描述符包含命令特定数据，无填充：第一个描述符必须包括unicast地址的virtio_net_ctrl_mac表结构体，无填充，第二个描述符必须包括multicast地址的virtio_net_ctrl_mac表结构体，无填充。
- 对于所有命令，使用单个1字节描述符用于ack字段。

### 5.2 块设备

virtio块设备是一个简单的虚拟块设备（例如磁盘）。读取和写入请求（以及其他特殊请求）被放置在其队列中，并且由设备进行服务（可能是无序的）。

#### 5.2.1 设备ID

- 块设备的ID为2。

#### 5.2.2 Virtqueues

- 0 requestq1
- ...（如果未协商VIRTIO_BLK_F_MQ，则N=1；否则N由num_queues设置）

#### 5.2.3 特性位

- VIRTIO_BLK_F_SIZE_MAX (1) 任何单个段的最大大小在size_max中指定。
- VIRTIO_BLK_F_SEG_MAX (2) 请求中的最大段数在seg_max中指定。
- VIRTIO_BLK_F_GEOMETRY (4) 以geometry中指定的磁盘样式几何形状。
