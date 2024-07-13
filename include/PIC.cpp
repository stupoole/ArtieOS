//
// Created by artypoole on 04/07/24.
//

#include "PIC.h"


#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

u8 mask1;
u8 mask2;

PIC::PIC()
{
    /* Normally, IRQs 0 to 7 are mapped to entries 8 to 15. This
    *  is a problem in protected mode, because IDT entry 8 is a
    *  Double Fault! Without remapping, every time IRQ0 fires,
    *  you get a Double Fault Exception, which is NOT actually
    *  what's happening. We send commands to the Programmable
    *  Interrupt Controller (PICs - also called the 8259's) in
    *  order to make IRQ0 to 15 be remapped to IDT entries 32 to
    *  47 */
    mask1 = inb(PIC1_DATA); // save masks
    mask2 = inb(PIC2_DATA);

    outb(PIC1, 0x11); // initialisation sequence
    outb(PIC2, 0x11);
    outb(PIC1_DATA, 0x20); // offset = 32
    outb(PIC2_DATA, 0x28); // offset = 32 + 8
    outb(PIC1_DATA, 0x04); // tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC2_DATA, 0x02); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    outb(PIC1_DATA, 0x01); // 8086 mode
    outb(PIC2_DATA, 0x01); // 8086 mode
    outb(PIC1_DATA, 0x0); //
    outb(PIC2_DATA, 0x0);
    outb(PIC1_DATA, 0xFF); // Output mask - disable pic
    outb(PIC2_DATA, 0xFF); // Output mask - disable pic
}

void PIC::disable()
{
    auto& log = Serial::get();
    log.write("Disabled PIC");
    mask1 = inb(PIC1_DATA); // save masks
    mask2 = inb(PIC2_DATA);
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void PIC::enable()
{
    auto& log = Serial::get();
    log.write("Renabled PIC");
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void PIC::enableIRQ(const u8 i)
{
    auto& log = Serial::get();
    log.log("Enabling IRQ", i);


    if (i < 8)
    {
        const u8 old_mask1 = inb(PIC1_DATA);
        const u8 byte = 0x1 << i;
        mask1 = old_mask1 & byte;
        log.write(byte, true);
        log.newLine();
    }
    else if (i<16)
    {
        const u8 old_mask2 = inb(PIC2_DATA);
        const u8 byte = 0x1 << (i - 8);
        mask2 = old_mask2 & byte;
        log.write(byte, true);
        log.newLine();
    }
    else return;

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void PIC::enableAll()
{
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}

