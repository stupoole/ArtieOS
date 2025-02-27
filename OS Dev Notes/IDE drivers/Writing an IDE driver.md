# This is a WIP, tutorial type document.

Much of the information required to read an ISO filesystem from a CD-ROM device using DMA is obfuscated or difficult to find. Here, I am trying to create one concise document which contains the information (or links to it) which was scattered among many sources. For example, SCSI command packets and their appropriate controls are not freely available in full, and several manuals are required to implement DMA, IDE and SCSI all together. Similarly, I simply couldn't find some of the information I needed, so this document hopes to bridge those gaps and fill it in with some results of trial, error and me banging my head against metaphorical walls. This was written using Obsidian (note taking app) with the intention for the user to read it in the same app. A PDF version will be exported periodically.

## CD ROM IDE BusMastering DMA

https://archive.org/details/spc2r20/page/n45/mode/2up

An IDE controller is an intermediary between a storage device and CPU/memory. An Integrated Drive Electronics (IDE) controller performs translation of standard commands into communication for specific storage device types. This means that using an IDE controller for use with a SATA device is much the same as when using a PATA device, and the user does not need to know the difference.

For each PCI IDE Controller device, there are two IDE channels on board (named primary and secondary) supporting two drives each (named Main and Alt in this documentation, replacing Master and Slave). That's a total of 4 drives per IDE controller. Simultaneous read/write is not handled in this tutorial, but is possible (if the controllers and attached devices support it).

BusMastering (BM) Direct Memory Access (DMA) if a form of communication between memory a device. In this configuration, a DMA controller (a PCI device) is given control of the bus, and enables data to be transferred between a device controlled by the DMA controller and memory, independent of the CPU. There are also two channels on an IDE BM DMA controller, one for each IDE controller. These are names Primary and Secondary and correspond to the IDE channels mentioned above. !INSERT IDEMS100.pdf!

If an IDE controller supports BM DMA then

## Useful Links:

https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf

file: ATA-ATAPI-6.pdf

http://users.utcluj.ro/~baruch/media/siee/labor/ATA-Interface.pdf

http://users.utcluj.ro/~baruch/media/siee/labor/ATA-Interface.pdf

https://wiki.osdev.org/ATAPI

https://wiki.osdev.org/ATA_PIO_Mode

https://wiki.osdev.org/ATA/ATAPI_using_DMA  

https://people.freebsd.org/~imp/asiabsdcon2015/works/d2161r5-ATAATAPI_Command_Set_-_3.pdf

http://web.archive.org/web/20221119212108/https://node1.123dok.com/dt01pdf/123dok_us/001/139/1139315.pdf.pdf?X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=7PKKQ3DUV8RG19BL%2F20221119%2F%2Fs3%2Faws4_request&X-Amz-Date=20221119T211954Z&X-Amz-SignedHeaders=host&X-Amz-Expires=600&X-Amz-Signature=c4154afd7a42db51d05db14370b8e885c3354c14cb66986bffcfd1a9a0869ee2

file: idems100.pdf

## Process overview:

There are a few overarching steps for setting up and using IDE BM DMA:

1.

*

*Discovery
**:  Discover devices and find the necessary ports or memory addresses to communicate with the PCI controller, BusMaster controller and IDE controller.

2.

*

*Configuration
**: Configure each element to be ready for Bus Mastering. This also requires preparing physical (memory) region(s) for use with DMA and handling of interrupts (if used).

3.

*

*Initiation
** Tell the drive how much data to transfer and which way before telling it to start. Tell the BM controller which way data is meant to flow and tell it to start/stop.

These steps are broken down further in the next few steps each with their own overview before the details. Many interrupts will be generated unless configured not to. Handling these interrupts is covered in its own section.

## Detection

<!-- todo: references for
 PCI classes
 OSdev PIO tutorial
BAR cases
Legacy port source
-->

- Discover PCI devices to detect if there is an IDE controller connected via PCI (Class 0x1; Subclass 0x1 in PCI Config reg2 bits 24-31 and 16-32 respectively).
  - If not PCI, sorry this tutorial does not help much. This means that there cannot be DMA or busmastering. The PIO tutorials on OSdev.net should be sufficient.
- Read the PCI configuration space registers to discover the base address registers (BAR#0-5) and the ProgIF (programming interface) byte. The contents of these determine how to communicate with the IDE controller. Use the PCI config space registers to control the PCI status and PCI command registers.
- Take note of the interrupt pin/line values from PCI config space as well.
- If ProgIF bit7 is set, (PCI Config reg2 bit15) then the IDE controller supports BM. If not, sorry this tutorial does not help much. You must use PIO mode.

https://wiki.osdev.org/PCI_IDE_Controller#Detecting_a_PCI_IDE_Controller
https://en.wikipedia.org/wiki/PCI_configuration_space#Bus_enumeration

Each PCI base address register (BAR) contains the upper 28 bits of a base physical address or base IO port address in its upper 28 bits. The lower 4 bits describe the region type, where bit0 specifies whether the contained value is a physical memory address (bit0 = 0) or an IO port (bit0 = 1). In practice this means that

```
if (BARX & 0x1)  addr = BARX & 0xFFF0 // should be cast to 16 bit I/O port
else addr = BARX & 0xFFFFFFF0 // addr is 32bit physical address
```

- If BAR0-3 are all zeros, then the IDE controller requires reading and writing to its register using legacy ports (Primary: 0x1f0 - 0x1f8, Secondary: 0x170 - 0x178). IRQ lines 14 and 15 are used for the BM interrupts, shared with the IDE interrupts. In this case, IDE controller interrupts also set the BM interrupt pin, which is annoying.
- If BAR 0-3 are populated, then the BAR0 contains the memory location for the Base Primary Main Registers (replacing port 0x1f0). BAR1 contains the memory location for the Primary Alt register (Alt Status at offset 0x0 and Alt Drive Select at 0x1. Not to be confused with Main/Alt drive). BAR2 and BAR3 contain the corresponding values for the Secondary IDE controller. In this case, any BM controller interrupts are sent to the interrupt line/pin as described in the PCI config space registers.
- BAR4 contains the base address/port for the BM controller register

NOTE: IDE controllers use 8 bit ports for communication (except when using PIO, in which case the base ports must be access as 16 bits wide). This means that BAR0 + 7 is the location of the Primary Status register if BAR0 is non-zero, for example. Otherwise, this is accessed using I/O port 0x1f0 + 7

## Configuration

At this point you should know where the PCI configuration space starts, where/how to access IDE controller registers and where/how to access the BM registers. Now we must know what to do with them.

The mains configuration steps are:

1. Select the drive
2. Identify whether the device supports PACKET commands (is ATAPI not ATA).
3. Detect the capabilities of the device (max DMA mode, packet size, sector and block size, capacity)
4. select the right DMA mode
5. enable busmastering in the PCI configuration space for the PCI controller
6. tell the busmaster controller what memory region(s) to use for transfers

Step 6 can be performed once for each device during configuration, or can be performed before each read/write call, with the memory being freed in between. How memory and physical regions are handled is up to the user. The simplest case is to set and forget but this may be seen as wasteful, especially if wanting to read large amounts of data at once.

### Select a drive

Write the correct data to the drive select IDE command register (offset 0x6. 0xE0 for main drive and 0xF0 for alt drive. LBA bit is always set.)

Selecting the drive can take time (~400ns) and so a delay should be introduced here. There are various methods such as reading the alt_status port 15 times. If precise timing is set up, a short sleep may be more suitable (using TSC for example).

### Check signature for ATA or ATAPI

After a reset, the IDE command registers will be set to a signature to differentiate between an ATA device and an ATAPI device. This tells the user whether to use the ATA IDENTIFY command (0xEC) or the ATAPI IDENTIFY command (0xA1) to read data. Either way the output data is accessed using PIO reads.

To initiate a reset:

1. write 0x04 (reset cmd) to the control register (0x3F6 for main drive and 0x3F7 for alt drive).
2. Wait at least 400ns.
3. Write 0 to the same control register.
4. Poll the alt_status register until the bsy bit clears (bit 7).

Immediately after the reset, read from the relevant registers and compare the values of each with the signatures given below:

| Name         | Reg Offset | ATA  | ATAPI |
|--------------|------------|------|-------|
| Sector count | 0x02       | 0x01 | 0x01  |
| LBA low      | 0x03       | 0x01 | 0x01  |
| LBA mid      | 0x04       | 0x00 | 0x14  |
| LBA high     | 0x05       | 0x00 | 0xEB  |

*

*Table x
**: ATA and ATAPI device signatures.

If the signatures do not match, then something has gone wrong. It might help to check the alt status error and device_fault bits, and if one is set, read the error register to get more information. It might be that the reset didn't happen properly. Maybe try playing with timings in this case.

### Detect capabilities

To detect the capabilities of the drive, the user simply issue an ATA IDENTIFY DEVICE command (0xEC) or an ATAPI INDENITIFY PACKET DEVICE command (0xA1) to the device. This is followed by reading 256 words from the data register (offset 0x00 to 0x01) as 16 bit reads.

Normally when performing a non-SCSI command, parameters for these commands are provided by writing to the IDE command registers before writing the command code to the command register. In this case, apart from the device select register (which is used to choose which drive to identify), they can all be zero or left untouched.

In the following steps, the ERR, DRQ and BSY bits are bits 0, 3 and 7, respectively, of the alt (or main) status register.

1) write the appropriate command code to the IDE command register (offset 0x7)
2) wait until the BSY bit is cleared
3) wait until the DRQ bit is set
4) assert that ERR is not set
5) use inw to read 256 words of identity data (512 bytes)

Below are some resulting fields from the INDENTIFY PACKET DEVICE command. Not all 256 words will be detailed here, but they are available in the ATAPI6 draft. Some important words are:

Word 0:
TODO: Serial number etc. My code should also print these strings. word 63: MW DMA modes word 88: UDMA modes

### Select the right DMA mode

### Configure the BM controller

## Initiation

At this point the system should be ready to request data from a storage device and to initiate a DMA transfer. If you chose to remake the PRDT for each transfer/set of transfers, set up the region and write to the BM PRDT register before these steps and free the memory region and clear the PRDT register after these steps:

1. Make sure the BM controller is ready by clearing the interrupt and error bits in the BM status register. This is done by writing ones to each of these bits of the register to toggle it, unlike most other registers. Writing zeros does nothing when writing to the BM status register.
2. Write zeros to IDE command register offsets 1 to 5 (inclusive) before sending the PACKET command. The drive select register should be set for the correct drive already.
3. Write the PACKET command (0xA0) to IDE command register (offset 0x7) to start a SCSI Packet transfer.
4. Wait for an interrupt/poll until bsy clears and DRQ is set. This indicates that the drive is ready to receive the packet via PIO interface
5. Write a SCSI packet containing appropriate data as (either 6 or 8) 16 bit words. This packet is 12 or 16 bytes for ATAPI even if using a SCSI Packet of fewer bytes (extend with zeros). The size is determined in the configuration steps.
6. Set the BM command register's start/stop bit (bit 0) to 1. If transferring data from the drive to memory, this register should be set to 0x9 (8 + 1) or 0x1 for writing to the drive.
7. Wait until the transfer is complete which will emit an interrupt and the drive will reset it's bsy and drq bits to of the IDE status register to 0.
8. Copy from (if reading from device) or clear (if copying to the device) memory region(s)

The structure of a scsi read (10) packet is given here and more possible packets are provided in TODO: section link

| byte | word | field                     | value                 | notes                  |
|------|------|---------------------------|-----------------------|------------------------|
| 0    | 0    | opcode                    | 0x28                  | READ (10)              |
| 1    | 0    | features                  | 0                     | RDPROT DPO FUA RARC    |
| 2    | 1    | LBA                       | lba>>24 & 0xff        | Most Significant Byte  |
| 3    | 1    | LBA                       | lba>>16 & 0xff        | (This is the start LBA |
| 4    | 2    | LBA                       | lba>>8 & 0xff         | of the transfer)       |
| 5    | 2    | LBA                       | lba & 0xff            | Least Significant Byte |
| 6    | 3    | group number              | 0                     | bottom 5 bits          |
| 7    | 3    | transfer length (sectors) | sec_count << 8 & 0xff | Most Significant Byte  |
| 8    | 4    | transfer length (sectors) | sec_count & 0xff      | Least Significant Byte |
| 9    | 4    | control                   | 0                     | dev specific           |
| 10   | 5    |                           | 0                     |                        |
| 11   | 5    |                           | 0                     |                        |

FOR LATER: reserve and configure the physical (memory) region(s) used for DMA and create a physical region descriptor table (PRDT) containing and physical region descriptor (PRD) entry for each configured physical region.

### DMA MODES

There are three main levels of DMA: SW DMA, multiword DMA (MW DMA) and ultra DMA (UDMA). For the user, the differences are not important. It is just important to know that the mode you select is supported by the drive (TODO: and the other hardware? Does the BM/PCI data show this?). If you're interested, see wikipedia: https://en.wikipedia.org/wiki/UDMA and https://en.wikipedia.org/wiki/WDMA_(computer). UDMA is faster than MW DMA so just use the highest UDMA mode supported if both types are supported, and use the highest MW DMA mode if not. There are UDMA modes 0 to 7 and MW DMA modes 0 to 4. SW DMA ranges 0 to 2 but is very legacy, and will not be discussed here. In all cases the largest number is the fastest. Some of these modes are defined by standards that are not ATA.

### Selecting DMA modes:

This requires sending a feature select command. This is an ATAPI command which uses IDE register values as input parameters:
TODO: write what the registers mean and what other options are for these.

### Writing to the DMA Status Register

The bits in the DMA Status Register are toggled by writing a one to them. That means, for example, that in order to CLEAR the interrupt bit you must write to the status register with this bit set. You do NOT write a 0 to clear the bit.

The DMA Command Register does not behave like this. It is written like normal.

### Communication

#### ATA, ATAPI and Packets

These are SCSI packets. For ATA devices, there is no PACKET command. The parameters for an ATA command are stored in the IDE controller registers. For ATAPI devices, the ATAPI commands use the IDE controller registers as well, but when a PACKET is sent, most of these registers are ignored, only the drive select is used (TODO: does the drive select and a register to request a specific number of bytes from a command?). The parameters for a command are sent as a SCSI command packet.

As part of the drive config steps, you should read the IDENTIFY data or the IDENTIFY PACKET DEVICE data (512 bytes as 256 words). This data contains the device configuration information such as the DMA modes possible on this device, as well as the expected number of bytes to be sent after calling PACKET command.

#### How to PIO transfer

TODO: the steps taken and the expected state after each step

#### How to DMA transfer

TODO: the steps taken and the expected state after each step

#### Highlighting the Difference

TODO: e.g. The DMA transfer sets bsy then sets drq without resetting busy, and doesn't emit an interrupt upon completion

#### Polling vs Interrupts

##    

# Registers

## PCI

### Access

Each register in PCI config space is 32-bits wide and must be accessed as 32-bit writes and reads. They are memory locations and so must be access using pointers. An example in C:

```
u32 PCI_read_reg(u32* PCI_base, u32 register_id)
{
    return *(PCI_base + register_id*4);
}

u32 PCI_write_reg(u32* PCI_base, u32 register_id, u32 value)
{
    *(PCI_base + (register_id*4)) = value;
    return PCI_read_reg(PCI_base, register_id);
}
```

PCI IDE controllers have header type 0 and therefore take on the following form.
https://wiki.osdev.org/PCI
TODO PCI

### PCI IDE Status

### PCI IDE Commnad

### PCI IDE ProgIF

### PCI IDE BARs

#### BAR0 - Primary Base

#### BAR1 - Primary Alt Base

#### BAR2 - Secondary Base

#### BAR3 - Secondary Alt base

#### BAR4 - BM controller base

## IDE Controller

### ATA Command Block registers

### ATAPI Command Block registers

### ATA/ATAPI alt registers

## BusMaster Controller

### BM Status

### BM Command

### BM PRDT