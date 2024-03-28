# Implementing a Network Stack for xv6-riscv 

## Project Overview
[[中文版]](README_ZH.md)
This project aims to implement a simple network stack for the xv6-riscv operating system, enabling basic network communication on QEMU.

I hope to write a detailed tutorial on the network protocol stack to enhance my understanding of networking, and it would be great if it could also help others who are learning.

Disclaimer: I am also a beginner learner, and any errors are subject to correction. The English part is translated by AI, and some code is completed by Copilot. If there are any issues, please raise an issue or contact me, thank you.

## Project Goals

- To implement basic network protocols on xv6-riscv, including but not limited to TCP/IP and UDP.
- To ensure the stability and performance of the network stack to support various network applications.
- To write a tutorial on the implementation of the network protocol stack starting from the link layer.

## Basic Introduction

### Operating System

- **xv6-riscv**: An open-source RISC-V operating system for educational and research purposes.

### Network Protocol Stack

- **Application Layer**
  - HTTP
  - DNS
- **Transport Layer**
  - TCP (net_tx_tcp, net_rx_tcp)
  - UDP (net_tx_udp, net_rx_udp)
- **Network Layer**
  - IPv4 (net_tx_ip, net_rx_ip)
  - IPv6
- **Link Layer**
  - ARP (net_tx_arp, net_rx_arp)
  - Ethernet (net_tx_eth, net_rx_eth)

### Hardware Drivers

- **QEMU Virtio-net**: Virtio was developed as a standardized open interface for virtual machines (VMs) to access simplified devices, such as block devices and network adapters. Virtio-net is a virtual Ethernet card and is the most complex device supported by virtio to date.

## Feature Implementation

- [X] Implemented vitio-net-mmio send and receive drivers (one queue each for sending and receiving)
- [X] Implemented Ethernet send and receive functions
- [X] Implemented ARP response send and receive functions
- [X] Implemented UDP send and receive functions
- [X] Implemented the connect system call to obtain a socket and fd
- [X] Implemented write, read, and close system calls for socket adaptation
- [ ] Implement send and receive buffer functions
- [ ] Implement multi-CPU concurrency
- [ ] Implement TCP send and receive functions
- [ ] Implement DNS resolution functionality
- [ ] Implement website file download
- [ ] Implement HTTP/HTTPS

## Documentation Directory

1. [Creating My World](docs/new_world_in_minecraft.md) (todo)
2. Enjoying the Elegant Lines of Networking (todo)
3. Starting with Ethernet (todo)
4. Reaching the Throne of IP (todo)
5. The Secrets of UDP (todo)
6. Catching the Socket (todo)

## Tool Recommendations

I use vscode for development in QEMU within WSL2. The .vscode folder contains some vscode configurations. For specific usage, I recommend [&#34;Starting to Use Vscode for Debugging XV6 from Scratch&#34;](https://zhuanlan.zhihu.com/p/501901665).
