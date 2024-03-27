这个文件是关于virtio规范的详细技术文档，涉及到虚拟化环境中的设备通信和配置。由于文件内容非常长，我将为您提供文件中几个关键部分的中文翻译。请注意，这是一个技术性很强的文档，翻译可能需要根据上下文进行适当调整。

---

### 4.1.5.4 设备配置更改的通知

一些virtio PCI设备可以改变设备配置状态，这反映在设备的设备特定配置区域中。在这种情况下：
- 如果MSI-X功能被禁用：
  1. 为设备的ISR状态字段设置第二个低位。
  2. 发送适当的PCI中断给设备。
- 如果MSI-X功能被启用：
  1. 如果config_msix_vector不是NO_VECTOR，请求设备的适当MSI-X中断消息，config_msix_vector设置MSI-X表条目号。
一个中断可能同时表示一个或多个virtqueue已被使用，以及配置空间已更改。

### 4.1.5.5 驱动程序处理中断

驱动程序中断处理程序通常：
- 如果MSI-X功能被禁用：
  - 读取ISR状态字段，这将将其重置为零。
  - 如果低位被设置：检查设备的所有virtqueues，看是否有设备通过操作需要服务的进展。
  - 如果第二个低位被设置：重新检查配置空间看有什么变化。
- 如果MSI-X功能被启用：
  - 查看映射到该MSI-X向量的设备的所有virtqueues，看是否有设备通过操作需要服务的进展。
  - 如果MSI-X向量等于config_msix_vector，重新检查配置空间看有什么变化。

### 4.2 MMIO上的virtio

没有PCI支持的虚拟环境（嵌入式设备模型中的常见情况）可能会使用简单的内存映射设备（“virtio-mmio”）代替PCI设备。

内存映射的virtio设备行为基于PCI设备规范。因此，包括设备初始化、队列配置和缓冲区传输在内的大多数操作几乎相同。现有差异在以下部分中描述。

### 4.2.1 MMIO设备发现

与PCI不同，MMIO没有提供通用的设备发现机制。对于每个设备，客户操作系统需要知道寄存器和中断的位置。对于使用扁平设备树的系统，建议的绑定示例如下所示：

```plaintext
// 示例：virtio_block设备在0x1e000处占用512字节，中断42。
virtio_block@1e000 {
    compatible = "virtio,mmio";
    reg = <0x1e000 0x200>;
    interrupts = <42>;
}
```

### 4.2.2 MMIO设备寄存器布局

MMIO virtio设备提供了一组内存映射的控制寄存器，后面跟着设备特定的配置空间，如表4.1所描述。

所有寄存器值都以小端模式组织。

表4.1：MMIO设备寄存器布局

名称 | 偏移量 | 方向 | 功能描述
--- | --- | --- | ---
MagicValue | 0x000 | R | 魔术值0x74726976（小端等价于“virt”字符串）。
Version | 0x004 | R | 设备版本号0x2。注意：传统设备（见4.2.4传统接口）使用0x1。
DeviceID | 0x008 | R | Virtio子系统设备ID。有关可能的值，请参见第5节设备类型。值为零（0x0）用于定义一个系统内存映射，其中包含静态、众所周知地址的占位符设备，根据用户需求分配功能。
VendorID | 0x00c | R | Virtio子系统供应商ID。
DeviceFeatures | 0x010 | R | 表示设备支持的功能的标志。从该寄存器读取返回32个连续的标志位，最低有效位取决于最后写入DeviceFeaturesSel的值。访问该寄存器返回DeviceFeaturesSel * 32到(DeviceFeaturesSel * 32)+31的位，例如，如果DeviceFeaturesSel设置为0，则返回功能位0到31，如果DeviceFeaturesSel设置为1，则返回功能位32到63。另见2.2功能位。
DeviceFeaturesSel | 0x014 | W | 设备（主机）功能字选择。写入该寄存器选择一组32个设备功能位，可通过从DeviceFeatures读取。
DriverFeatures | 0x020 | W | 表示设备功能由驱动程序理解和激活的标志。写入该寄存器设置32个连续的标志位，最低有效位取决于最后写入DriverFeaturesSel的值。访问该寄存器设置DriverFeaturesSel * 32到(DriverFeaturesSel * 32) + 31的位，例如，如果DriverFeaturesSel设置为0，则返回功能位0到31，如果DriverFeaturesSel设置为1，则返回功能位32到63。另见2.2功能位。
DriverFeaturesSel | 0x024 | W | 激活（客户）功能字选择。写入该寄存器选择一组32个已激活的功能位，可通过写入DriverFeatures访问。
QueueSel | 0x030 | W | 虚拟队列索引。写入该寄存器选择以下操作适用的虚拟队列：QueueNumMax、QueueNum、QueueReady、QueueDescLow、QueueDescHigh、QueueDriverlLow、QueueDriverHigh、QueueDeviceLow、QueueDeviceHigh和QueueReset。第一个队列的索引号为零（0x0）。
QueueNumMax | 0x034 | R | 最大虚拟队列大小。从该寄存器读取返回设备准备处理的队列的最大大小（元素数量）或零（0x0），如果队列不可用。这适用于通过写入QueueSel选择的队列。
QueueNum | 0x038 | W | 虚拟队列大小。写入该寄存器通知设备驱动程序将使用的队列大小。这适用于通过写入QueueSel选择的队列。
QueueReady | 0x044 | RW | 虚拟队列就绪位。向该寄存器写入一（0x1）通知设备可以从该虚拟队列执行请求。读取该寄存器返回最后写入的值。读和写访问均适用于通过写入QueueSel选择的队列。
QueueNotify | 0x050 | W | 队列通知器。向该寄存器写入值通知设备在队列中有新的缓冲区需要处理。当未协商VIRTIO_F_NOTIFICATION_DATA时，写入的值是队列索引。当协商了VIRTIO_F_NOTIFICATION_DATA时，通知数据值具有以下格式：`plaintext le32 {vqn : 16;next_off : 15;next_wrap : 1;};`另见2.9驱动程序通知。
InterruptStatus | 0x60 | R | 中断状态。从该寄存器读取返回引起设备中断的事件的位掩码。可能的事件包括：已使用的缓冲区通知 - 位0 - 中断是因为设备在至少一个活动的虚拟队列中使用了缓冲区。配置更改通知 - 位1 - 中断是因为设备的配置已更改。
InterruptACK | 0x064 | W | 中断确认。向该寄存器写入与InterruptStatus中定义的位掩码相应的值，通知设备已处理引起中断的事件。
Status | 0x070 | RW | 设备状态。从该寄存器读取返回当前设备状态标志。向该寄存器写入非零值设置状态标志，表示驱动程序进度。向该寄存器写入零（0x0）触发设备重置。另见4.2.3.1设备初始化。
QueueDescLow | 0x080 | QueueDescHigh | 0x084 | W | 虚拟队列的描述符区域64位长物理地址。将这些寄存器的值（地址的低32位写入QueueDescLow，高32位写入QueueDescHigh）写入，通知设备选择了QueueSel寄存器的队列的描述符区域的位置。
QueueDriverLow | 0x090 | QueueDriverHigh | 0x094 | W | 虚拟队列的驱动程序区域64位长物理地址。将这些寄存器的值（地址的低32位写入QueueDriverLow，高32位写入QueueDriverHigh）写入，通知设备选择了QueueSel寄存器的队列的驱动程序区域的位置。
QueueDeviceLow | 0x0a0 | QueueDeviceHigh | 0x0a4 | W | 虚拟队列的设备区域64位长物理地址。将这些寄存器的值（地址的低32位写入QueueDeviceLow，高32位写入QueueDeviceHigh）写入，通知设备选择了QueueSel寄存器的队列的设备区域的位置。
SHMSel | 0x0ac | W | 共享内存ID。向该寄存器写入选择共享内存区域2.10，以下操作适用于SHMLenLow、SHMLenHigh、SHMBaseLow和SHMBaseHigh。
SHMLenLow | 0x0b0 | SHMLenHigh | 0x0b4