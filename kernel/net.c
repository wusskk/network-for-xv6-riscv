#include "types.h"
#include "riscv.h"
#include "net.h"
#include "defs.h"

static uint32 local_ip = MAKE_IP_ADDR(10, 0, 2, 2);  //10.0.2.2    
uint8 local_mac[6] = {0};               // qemu's idea of the guest IP//{ 0x02, 0xca, 0xfe, 0xf0, 0x0d, 0x02 };
static uint8 broadcast_mac[ETHADDR_LEN] = {0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF};

char *
mbuf_put(struct mbuf *m, int len)
{
  char *tmp;
  if(len + m->len > MBUF_SIZE)
    return 0;

  m->len = m->len + len;
  m->head = m->head - len;
  tmp = m->head;

  return tmp;
}

char *
mbuf_get(struct mbuf *m, int len)
{
  char *tmp;
  if(len > m->len)
    return 0;

  m->len = m->len - len;
  tmp = m->head;
  m->head = m->head + len;

  return tmp;
}

void
mbuf_free(struct mbuf *m)
{
  kfree(m);
  m = 0;
}

struct mbuf *
mbuf_alloc()
{
  struct mbuf *m = (struct mbuf *)kalloc();
  if(m == 0) {
    return 0;
  }

  memset(m, 0, sizeof(struct mbuf));
  m->head = m->buf + MBUF_SIZE;
  m->len = 0;

  return m;
}

// This code is lifted from FreeBSD's ping.c, and is copyright by the Regents
// of the University of California.
static unsigned short
in_cksum(const unsigned char *addr, int len)
{
  int nleft = len;
  const unsigned short *w = (const unsigned short *)addr;
  unsigned int sum = 0;
  unsigned short answer = 0;

  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while(nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  /* mop up an odd byte, if necessary */
  if(nleft == 1) {
    *(unsigned char *)(&answer) = *(const unsigned char *)w;
    sum += answer;
  }

  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum & 0xffff) + (sum >> 16);
  sum += (sum >> 16);
  /* guaranteed now that the lower 16 bits of sum are correct */

  answer = ~sum; /* truncate to 16 bits */
  return answer;
}

static int
net_tx_eth(struct mbuf *m, int ethtype)
{
  char *tmp = mbuf_put(m, sizeof(struct eth));
  if(tmp == 0) {
    printf("error in put eth header: buf is too big!");
    return -1;
  }

  struct eth *eth_hdr = (struct eth *)tmp;
  memmove(eth_hdr->shost, local_mac, ETHADDR_LEN);
  // In a real networking stack, dhost would be set to the address discovered
  // through ARP. Because we don't support enough of the ARP protocol, set it
  // to broadcast instead.
  memmove(eth_hdr->dhost, broadcast_mac, ETHADDR_LEN);
  eth_hdr->type = htons(ethtype);

  virtio_net_send((uint8 *)m->head, m->len, 0);

  return 0;
}

static int
net_tx_ip(struct mbuf *m, uint8 ptype, uint32 ra)
{
  char *tmp = mbuf_put(m, sizeof(struct ip));
  if(tmp == 0) {
    printf("error in put ip header: buf is too big!");
    return -1;
  }

  struct ip *ip_hdr = (struct ip *)tmp;
  ip_hdr->ip_vhl = (4 << 4) | (20 >> 2);
  // ip_hdr->ip_off = 0;
  ip_hdr->ip_p = ptype;
  ip_hdr->ip_src = htonl(local_ip);
  ip_hdr->ip_dst = htonl(ra);
  ip_hdr->ip_len = htons(m->len);
  ip_hdr->ip_ttl = 100;
  ip_hdr->ip_sum = in_cksum((unsigned char *)ip_hdr, sizeof(*ip_hdr));

  // now on to the ethernet layer
  return net_tx_eth(m, ETHTYPE_IP);
}

int
net_tx_udp(struct mbuf *m, uint32 ra, uint16 lp, uint16 rp)
{
  char *tmp = mbuf_put(m, sizeof(struct udp));
  if(tmp == 0) {
    printf("error in put udp header: buf is too big!");
    return -1;
  }

  struct udp *udp_hdr = (struct udp *)tmp;
  udp_hdr->dport = htons(rp);
  udp_hdr->sport = htons(lp);
  udp_hdr->ulen = htons(m->len);
  udp_hdr->sum = 0;

  return net_tx_ip(m, IPPROTO_UDP, ra);
}

int
net_tx_tcp(struct mbuf *m, uint32 ra, uint16 lp, uint16 rp, struct tcpcb* tcb)
{
  char* tmp = mbuf_put(m, sizeof(struct tcp));
  if (tmp == 0) {
      printf("error in put tcp header: buf is too big!");
      return -1;
  }

  struct tcp* tcp_hdr = (struct tcp*)tmp;
  tcp_hdr->dport = htons(rp);
  tcp_hdr->sport = htons(lp);
  tcp_hdr->off = sizeof(struct tcp) / 4;
  tcp_hdr->win = htons(MBUF_SIZE - sizeof(struct eth) - sizeof(struct ip));

  char* psd_tmp = mbuf_put(m, sizeof(struct psd));
  if (psd_tmp == 0) {
      printf("error in put tcp pseudo header: buf is too big!");
      return -1;
  }
  struct psd* psd_hdr = (struct psd*)psd_tmp;
  psd_hdr->sip = htonl(local_ip);
  psd_hdr->dip = htonl(ra);
  psd_hdr->zero = 0;
  psd_hdr->prot = IPPROTO_TCP;
  psd_hdr->len = htons(m->len - sizeof(struct psd));


  if(tcb->state == CLOSED){
    tcp_hdr->seqn = htonl(100);
    tcp_hdr->syn =1;
    // tcb->state = SYN_SENT;
  }else if(tcb->state == SYN_SENT){
    tcp_hdr->seqn = htonl(101);
    tcp_hdr->ackn = htonl(tcb->ackn + 1);
    tcp_hdr->ack = 1;
    tcb->state = ESTABLISHED;
  }else if(tcb->state == ESTABLISHED){
    tcp_hdr->seqn = htonl(tcb->seqn);
    tcp_hdr->ackn = htonl(tcb->ackn + 1);
    tcp_hdr->ack = 1;
  }

  tcp_hdr->sum = in_cksum((unsigned char *)psd_hdr, (sizeof(struct psd)+sizeof(struct tcp)));
  memset(psd_hdr, 0, sizeof(struct psd));
  mbuf_get(m, sizeof(struct psd));
  return net_tx_ip(m, IPPROTO_TCP, ra);
}

static void
net_rx_udp(struct mbuf *m, uint16 len, uint32 ra)
{
  struct udp *udp_hdr = (struct udp *)mbuf_get(m, sizeof(struct udp));
  if(udp_hdr == 0) {
    printf("error in get udp header: buf is too small!");
    return;
  }

  if(ntohs(udp_hdr->ulen) != len) {
    printf("UDP length does not match IP packet length\n");
    mbuf_free(m);
    return;
  }

  uint16 rport = ntohs(udp_hdr->sport);
  uint16 lport = ntohs(udp_hdr->dport);

  sockrecvudp(m, ra, lport, rport);
}

static void
net_rx_tcp(struct mbuf *m, uint16 len, uint32 ra)
{
  struct tcp *tcp_hdr = (struct tcp *)mbuf_get(m, sizeof(struct tcp));
  if(tcp_hdr == 0) {
    printf("error in get udp header: buf is too small!");
    return;
  }

  // if(ntohs(tcp_hdr->ulen) != len) {
  //   printf("TCP length does not match IP packet length\n");
  //   mbuf_free(m);
  //   return;
  // }
  uint32 seqn = ntohl(tcp_hdr->seqn); 
  uint32 ackn = ntohl(tcp_hdr->ackn); 

  uint state = ESTABLISHED;
  if(tcp_hdr->syn == 1 && tcp_hdr->ack == 1) 
    state = SYN_SENT;
  // }else if(tcp_hdr->ack == 1){
  //   state = ESTABLISHED;
  // }else if()


  uint16 rport = ntohs(tcp_hdr->sport);
  uint16 lport = ntohs(tcp_hdr->dport);

  sockrecvtcp(m, ra, lport, rport, state, seqn, ackn);
}

static void
net_rx_ip(struct mbuf *m)
{
  struct ip *ip_hdr = (struct ip *)mbuf_get(m, sizeof(struct ip));
  if(ip_hdr == 0) {
    printf("error in get ip header: buf is too small!");
    return;
  }

  if(ip_hdr->ip_vhl != ((4 << 4) | (20 >> 2))) {
    printf("Unknown IP version/length: %x\n", ip_hdr->ip_vhl);
    mbuf_free(m);
    return;
  }
  if(in_cksum((unsigned char *)ip_hdr, sizeof(*ip_hdr)) != 0) {
    printf("Bad IP checksum\n");
    mbuf_free(m);
    return;
  }
  if(ip_hdr->ip_dst != htonl(local_ip)) {
    printf("IP packet not for us\n");
    mbuf_free(m);
    return;
  }
  if(ip_hdr->ip_off != 0) {
    printf("IP fragmentation not supported\n");
    mbuf_free(m);
    return;
  }
  if(ip_hdr->ip_p == IPPROTO_UDP) {
    net_rx_udp(m, (ntohs(ip_hdr->ip_len) - sizeof(*ip_hdr)), ntohl(ip_hdr->ip_src));
  } else if(ip_hdr->ip_p == IPPROTO_TCP) {
    net_rx_tcp(m, (ntohs(ip_hdr->ip_len) - sizeof(*ip_hdr)), ntohl(ip_hdr->ip_src));
  } else {
    printf("Unknown IP protocol: %x\n", ip_hdr->ip_p);
    mbuf_free(m);
  }
}

/*
ARP报文
硬件类型（HTYPE）：如以太网（0x0001）、分组无线网。
协议类型（PTYPE）：如网际协议(IP)（0x0800）、IPv6（0x86DD）。
硬件地址长度（HLEN）：每种硬件地址的字节长度，一般为6（以太网）。
协议地址长度（PLEN）：每种协议地址的字节长度，一般为4（IPv4）。
操作码：1为ARP请求，2为ARP应答，3为RARP请求，4为RARP应答。
源硬件地址（Sender Hardware Address，简称SHA）：n个字节，n由硬件地址长度得到，一般为发送方MAC地址。
源协议地址（Sender Protocol Address，简称SPA）：m个字节，m由协议地址长度得到，一般为发送方IP地址。
目标硬件地址（Target Hardware Address，简称THA）：n个字节，n由硬件地址长度得到，一般为目标MAC地址。
目标协议地址（Target Protocol Address，简称TPA）：m个字节，m由协议地址长度得到，一般为目标IP地址。
*/

static void
net_rx_arp(struct mbuf *m)
{
  struct arp *arp_hdr = (struct arp *)mbuf_get(m, sizeof(struct arp));
  if(arp_hdr == 0) {
    printf("error in get arp header: buf is too small!");
    return;
  }

  if(ntohs(arp_hdr->hrd) != ARP_HRD_ETHER ||
     ntohs(arp_hdr->pro) != ETHTYPE_IP ||
     arp_hdr->hln != ETHADDR_LEN ||
     arp_hdr->pln != sizeof(uint32)) {
    printf("ARP format not recognized\n");
    mbuf_free(m);
    return;
  }

  if(ntohs(arp_hdr->op) != ARP_OP_REQUEST) {
    printf("ARP op not a request\n");
    mbuf_free(m);
    return;
  }

  if(ntohl(arp_hdr->tip) != local_ip) {
    printf("ARP request not for us\n");
    mbuf_free(m);
    return;
  }

  struct mbuf *newm = mbuf_alloc();
  char *tmp = mbuf_put(newm, sizeof(struct arp));
  if(tmp == 0) {
    printf("error in put arp header: buf is too big!");
    mbuf_free(newm);
    mbuf_free(m);
    return;
  }

  struct arp *new_arp_hdr = (struct arp *)tmp;
  new_arp_hdr->hrd = htons(ARP_HRD_ETHER);
  new_arp_hdr->pro = htons(ETHTYPE_IP);
  new_arp_hdr->hln = ETHADDR_LEN;
  new_arp_hdr->pln = sizeof(uint32);
  new_arp_hdr->op = htons(ARP_OP_REPLY);

  memmove(new_arp_hdr->sha, local_mac, ETHADDR_LEN);
  new_arp_hdr->sip = htonl(local_ip);
  memmove(new_arp_hdr->tha, arp_hdr->sha, ETHADDR_LEN);
  new_arp_hdr->tip = arp_hdr->sip;

  net_tx_eth(newm, ETHTYPE_ARP);
  mbuf_free(m);
}

void
net_rx_eth(struct mbuf *m)
{
  struct eth *eth_hdr = (struct eth *)mbuf_get(m, sizeof(struct eth));
  if(eth_hdr == 0) {
    printf("error in get eth header: buf is too small!");
    return;
  }

  uint16 ethtype = ntohs(eth_hdr->type);
  if(ethtype == ETHTYPE_IP) {
    net_rx_ip(m);
  } else if(ethtype == ETHTYPE_ARP) {
    net_rx_arp(m);
  } else {
    printf("Unknown ethertype: %x\n", ethtype);
    mbuf_free(m);
  }
}