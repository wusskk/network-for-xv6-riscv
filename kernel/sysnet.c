#include "types.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "net.h"
#include "param.h"
#include "fs.h"
#include "file.h"
#include "proc.h"



struct sock {
  struct sock *next;
  uint32 raddr; // the remote IPv4 address
  uint16 lport; // the local UDP port number
  uint16 rport; // the remote UDP port number
  enum tlp { UDP, TCP } type; // transpot layer protocol udp or tcp
  struct spinlock lock;
  struct mbuf tx;
  struct mbuf rx;
  struct tcpcb tcb;
};

static struct spinlock lock =
{
    .name = "slock",
    .cpu = 0,
    .locked = 0,
};
static struct sock sockets = {0};

static struct sock*
get_sock(uint32 ra, uint16 lp, uint16 rp)
{
  acquire(&lock);
  struct sock *tmp = sockets.next;
  while(tmp != 0) {
    if(tmp->lport == lp && tmp->raddr == ra && tmp->rport == rp) {
      break;
    }
    tmp = tmp->next;
  }
  release(&lock);
  return tmp;
}

int
sockalloc(struct file **file, uint32 ra, uint16 lp, uint16 rp, uint16 type)
{
  struct sock *sock;

  *file = filealloc();
  if(*file == 0)
    return -1;
  sock = (struct sock *)kalloc();
  if(sock == 0) {
    fileclose(*file);
    return -1;
  }
  memset(sock, 0, sizeof(struct sock));

  sock->raddr = ra;
  sock->lport = lp;
  sock->rport = rp;
  sock->type = type;
  if(type == TCP){
    sock->tcb.state = CLOSED;
  }
  initlock(&sock->lock, "rxlcok");
  (*file)->type = FD_SOCK;
  (*file)->sock = sock;
  (*file)->writable = 1;
  (*file)->readable = 1;

  // add socket to sockets
  acquire(&lock);
  struct sock *tmp = &sockets;
  while(tmp->next != 0)
    tmp = tmp->next;
  tmp->next = sock;
  release(&lock);

  return 0;
}

void
sockclose(struct sock *sock)
{
  acquire(&lock);
  // del socket to sockets
  struct sock *tmp = &sockets;
  struct sock *tmpn;
  while(tmp->next != 0) {
    tmpn = tmp->next;
    if(tmpn->lport == sock->lport && tmpn->raddr == sock->raddr && tmpn->rport == sock->rport && tmpn->type == sock->type) {
      tmp->next = sock->next;
      break;
    }
    tmp = tmp->next;
  }


  release(&lock);

  kfree((char *)sock);
}

int
sockread(struct sock *sock, uint64 addr, int n)
{
  struct proc *pr = myproc();
  struct mbuf *m = &sock->rx;

  acquire(&sock->lock);
  while(m->len == 0 && !pr->killed) {
    sleep(m, &sock->lock);
  }

  if(pr->killed) {
    release(&sock->lock);
    return -1;
  }

  if(n > m->len)
    n = m->len;

  if(copyout(pr->pagetable, addr, m->head, n) == -1) {
    memset(m, 0, sizeof(struct mbuf));
    release(&sock->lock);
    return -1;
  }
  memset(m, 0, sizeof(struct mbuf));
  release(&sock->lock);
  return n;
}

// n is len, addr is the address of data
int
sockwrite(struct sock *sock, uint64 addr, int n)
{
  struct proc *pr = myproc();
  struct mbuf *m = &sock->tx;
  memset(&sock->tx, 0, sizeof(struct mbuf));

  m->next = 0;
  m->head = m->buf + MBUF_SIZE;
  m->len = 0;

  char *tmp = mbuf_put(m, n);
  if(tmp == 0) {
    printf("error in put data: buf is too big!");
    return -1;
  }

  if(copyin(pr->pagetable, tmp, addr, n) == -1) {
    return -1;
  }

  if(sock->type == UDP)
    return net_tx_udp(m, sock->raddr, sock->lport, sock->rport);
  else if(sock->type == TCP)
    return net_tx_tcp(m, sock->raddr, sock->lport, sock->rport, &sock->tcb);
  else
    return -1;
}


int sockconnect(struct sock* sock) {
  struct mbuf* m = &sock->tx;
  memset(m, 0, sizeof(struct mbuf));
  m->next = 0;
  m->head = m->buf + MBUF_SIZE;
  m->len = 0;
  
  // Send SYN packet
  if(net_tx_tcp(m, sock->raddr, sock->lport, sock->rport, &sock->tcb) == -1) {
    return -1;
  }
  
  // Wait for SYN-ACK packet
  acquire(&sock->lock);
  while(sock->tcb.state != SYN_SENT) {
    sleep(&sock->rx, &sock->lock);
  }
  release(&sock->lock);

  // Send ACK packet
  memset(m, 0, sizeof(struct mbuf));
  m->next = 0;
  m->head = m->buf + MBUF_SIZE;
  m->len = 0;
  if(net_tx_tcp(m, sock->raddr, sock->lport, sock->rport, &sock->tcb) == -1) {
    return -1;
  }
  return 0;
}
// {
//   struct mbuf* m = &sock->rx;
//   memset(&sock->tx, 0, sizeof(struct mbuf));
//   m->next = 0;
//   m->head = m->buf + MBUF_SIZE;
//   m->len = 0;

//   char *tmp = mbuf_put(m, sizeof(struct tcp));
//   if(tmp == 0) {
//     printf("error in put data: buf is too big!");
//     return -1;
//   }
//   struct tcp* tcp_hdr = (struct tcp*)tmp;
//   tcp_hdr->sport = htons(sock->lport);
//   tcp_hdr->dport = htons(sock->rport);
//   tcp_hdr->syn = 1;
//   tcp_hdr->seqn = 100;
  

//   return 0;
// }

void
sockrecvudp(struct mbuf *m, uint32 ra, uint16 lp, uint16 rp)
{
  struct sock *sock = 0;
  sock = get_sock(ra, lp, rp);

  if(sock == 0) {
    printf("no such socket\n");
    return;
  }

  acquire(&sock->lock);
  memmove(&sock->rx, m, sizeof(struct mbuf));
  wakeup(&sock->rx);
  release(&sock->lock);
}

void
sockrecvtcp(struct mbuf *m, uint32 ra, uint16 lp, uint16 rp, uint state, uint32 seqn, uint32 ackn)
{
  struct sock *sock = 0;
  sock = get_sock(ra, lp, rp);

  if(sock == 0) {
    printf("no such socket\n");
    return;
  }

  acquire(&sock->lock);

  sock->tcb.ackn = seqn;
  sock->tcb.seqn = ackn;
  if(state == SYN_SENT) {
    sock->tcb.state = SYN_SENT;
  }

  memmove(&sock->rx, m, sizeof(struct mbuf));
  wakeup(&sock->rx);
  release(&sock->lock);
}