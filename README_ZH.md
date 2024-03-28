# xv6-riscv 实现网络堆栈

## 项目概述

本项目旨在为 xv6-riscv 操作系统实现一个简单的网络堆栈，使得xv6能够在qemu上进行基本的网络通信。

我希望写一份详细的网络协议栈的教程，能够帮我完善对网络的理解，如果能帮助到其他学习的人就更好了。

免责说明：本人也是一位初级学习者，如有错误，不吝赐教。英文部分由AI翻译，部分代码由copilot完成，如有问题，请提issue或者联系我，谢谢。

## 项目目标

- 在 xv6-riscv 上实现基本的网络协议，包括但不限于 TCP/IP 和 UDP。
- 确保网络堆栈的稳定性和性能，以支持多种网络应用。
- 编写从链路层开始的网络协议栈实现教程

## 基本介绍

### 操作系统

- **xv6-riscv**: 一个开源的 RISC-V 操作系统，用于教学和研究目的。

### 网络协议栈

- **应用层**
  - HTTP
  - DNS
- **传输层**
  - TCP(net_tx_tcp, net_rx_tcp)
  - UDP(net_tx_udp, net_rx_udp)
- **网络层**
  - IPv4(net_tx_ip, net_rx_ip)
  - IPv6
- **链路层**
  - ARP(net_tx_arp, net_rx_arp)
  - 以太网(net_tx_eth, net_rx_eth)

### 硬件驱动

- **QEMU Virtio-net**: Virtio 被开发为虚拟机 （VM） 的标准化开放接口，用于访问简化的设备，例如块设备和网络适配器。Virtio-net是一个虚拟以太网卡，是virtio迄今为止支持的最复杂的设备。

## 特性实现

- [X] 实现vitio-net-mmio的接收发送驱动(发送和接收各一个队列)
- [X] 实现以太网接收发送功能
- [X] 实现ARP的回答的接收发送功能
- [X] 实现UDP的接收发送功能
- [X] 实现connnect系统调用以获取socket和fd
- [X] 实现write，read和close系统调用对socket的适配
- [ ] 实现发送接收的缓冲区功能
- [ ] 实现多CPU的并发
- [ ] 实现TCP的接收发送功能
- [ ] 实现DNS解析功能
- [ ] 实现网站文件下载
- [ ] 实现HTTP/HTTPS

## 文档目录

1. 创建我的世界(todo)
2. 爽摸网络优美的线条(todo)
3. 从以太网开始(todo)
4. 到达IP的王座(todo)
5. UDP的秘密(todo)
6. 抓到socket(todo)

## 工具推荐

我使用vscode在wsl2的qemu中开发，.vscode文件夹中有部分vscode配置，具体使用方式推荐[《从零开始使用Vscode调试XV6》](https://zhuanlan.zhihu.com/p/501901665)。
