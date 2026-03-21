#include "mouse.h"
#include "../../drivers/screen/terminal.h"
#include "../../kernel/io/io.h"
#include "../../libc/stdio/stdio.h"

#define PS2_DATA 0x60
#define PS2_STATUS 0x64 // read
#define PS2_COMMAND 0x64 // write

#define PS2_OUTPUT_FULL 0x01 // a byte is waiting in PS2_DATA
#define PS2_INPUT_FULL 0x02  // the controller hasnt taken the last byte yet

#define PS2_ENABLE_AUX 0xA8
#define PS2_READ_CONFIG 0x20
#define PS2_WRITE_CONFIG 0x60
#define PS2_TO_AUX 0xD4 // next data byte goes to the mouse

#define CONFIG_AUX_IRQ 0x02
#define CONFIG_AUX_CLOCK_OFF 0x20

#define MOUSE_DEFAULTS 0xF6
#define MOUSE_ENABLE 0xF4
#define MOUSE_ACK 0xFA

// first byte of a packet: buttons, then always set, then the sign and
// overflow bits of both movements
#define PACKET_ALWAYS_SET 0x08
#define PACKET_X_SIGN 0x10
#define PACKET_Y_SIGN 0x20
#define PACKET_OVERFLOW 0xC0

#define PIC_SLAVE_DATA 0xA1
#define IRQ12_MASK (1 << 4) // irq12 is irq4 of the slave pic

// spins a bounded time, a missing controller shouldnt hang the boot
#define PS2_TIMEOUT 100000

#define MOUSE_EVENTS_SIZE 64

// vmmouse: the absolute pointer of vmware, qemu has it too. it is reached
// through the vmware backdoor, an in on its port with the magic in eax
#define VMWARE_MAGIC 0x564D5868
#define VMWARE_PORT 0x5658
#define VMWARE_GETVERSION 10
#define VMMOUSE_DATA 39
#define VMMOUSE_STATUS 40
#define VMMOUSE_COMMAND 41
#define VMMOUSE_ENABLE 0x45414552
#define VMMOUSE_REQUEST_ABSOLUTE 0x53424152
#define VMMOUSE_VERSION_ID 0x3442554A
#define VMMOUSE_STATUS_ERROR 0xFFFF0000
#define VMMOUSE_PACKET_WORDS 4
#define VMMOUSE_LEFT 0x20
#define VMMOUSE_RIGHT 0x10
#define VMMOUSE_MIDDLE 0x08

static uint8_t vmmouse = 0;

static volatile struct mouse_event events[MOUSE_EVENTS_SIZE];
static volatile uint32_t events_head = 0;
static volatile uint32_t events_tail = 0;

static uint8_t packet[3];
static uint8_t packet_len = 0;

static int
ps2_wait_write (void)
{
  for (uint32_t i = 0; i < PS2_TIMEOUT; i++)
    {
      if (!(insb (PS2_STATUS) & PS2_INPUT_FULL))
        {
          return 1;
        }
    }
  return 0;
}

static int
ps2_wait_read (void)
{
  for (uint32_t i = 0; i < PS2_TIMEOUT; i++)
    {
      if (insb (PS2_STATUS) & PS2_OUTPUT_FULL)
        {
          return 1;
        }
    }
  return 0;
}

// sends a command to the mouse and reads its ack
static int
mouse_command (uint8_t command)
{
  if (!ps2_wait_write ())
    {
      return 0;
    }
  outb (PS2_COMMAND, PS2_TO_AUX);
  if (!ps2_wait_write ())
    {
      return 0;
    }
  outb (PS2_DATA, command);
  if (!ps2_wait_read ())
    {
      return 0;
    }
  return insb (PS2_DATA) == MOUSE_ACK;
}

struct backdoor
{
  uint32_t eax, ebx, ecx, edx;
};

static struct backdoor
vmware_call (uint32_t command, uint32_t arg)
{
  struct backdoor r;
  asm volatile ("inl %%dx, %%eax"
                : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
                : "a"(VMWARE_MAGIC), "b"(arg), "c"(command),
                  "d"(VMWARE_PORT));
  return r;
}

// switches the vmmouse on if there is one. the ps/2 mouse keeps raising
// irq12 for every event, the position is read from the vmmouse then
static uint8_t
vmmouse_init (void)
{
  if (vmware_call (VMWARE_GETVERSION, ~VMWARE_MAGIC).ebx != VMWARE_MAGIC)
    {
      return 0;
    }
  vmware_call (VMMOUSE_COMMAND, VMMOUSE_ENABLE);
  uint32_t status = vmware_call (VMMOUSE_STATUS, 0).eax;
  if ((status & VMMOUSE_STATUS_ERROR) == VMMOUSE_STATUS_ERROR
      || (status & 0xFFFF) == 0
      || vmware_call (VMMOUSE_DATA, 1).eax != VMMOUSE_VERSION_ID)
    {
      return 0;
    }
  vmware_call (VMMOUSE_COMMAND, VMMOUSE_REQUEST_ABSOLUTE);
  return 1;
}

void
mouse_init (uint8_t verbose)
{
  int ok = ps2_wait_write ();
  outb (PS2_COMMAND, PS2_ENABLE_AUX);
  // the configuration byte decides which ports raise irqs
  ok = ok && ps2_wait_write ();
  outb (PS2_COMMAND, PS2_READ_CONFIG);
  ok = ok && ps2_wait_read ();
  uint8_t config = insb (PS2_DATA);
  config = (config | CONFIG_AUX_IRQ) & ~CONFIG_AUX_CLOCK_OFF;
  ok = ok && ps2_wait_write ();
  outb (PS2_COMMAND, PS2_WRITE_CONFIG);
  ok = ok && ps2_wait_write ();
  outb (PS2_DATA, config);
  ok = ok && mouse_command (MOUSE_DEFAULTS) && mouse_command (MOUSE_ENABLE);
  vmmouse = ok && vmmouse_init ();
  outb (PIC_SLAVE_DATA, insb (PIC_SLAVE_DATA) & ~IRQ12_MASK);
  if (verbose)
    {
      printf (vmmouse ? "ps/2 mouse init <absolute>" : "ps/2 mouse init");
      if (ok)
        {
          init_OK ();
        }
      else
        {
          init_ER ();
        }
    }
}

static void
event_push (int16_t dx, int16_t dy, uint8_t buttons, uint8_t absolute)
{
  uint32_t next = (events_head + 1) % MOUSE_EVENTS_SIZE;
  // a full queue loses its oldest event, nobody is reading them anyway
  if (next == events_tail)
    {
      events_tail = (events_tail + 1) % MOUSE_EVENTS_SIZE;
    }
  events[events_head].dx = dx;
  events[events_head].dy = dy;
  events[events_head].buttons = buttons;
  events[events_head].absolute = absolute;
  events_head = next;
}

// takes the packets queued in the vmmouse. positions go from 0 to 0xFFFF
// across the screen
static void
vmmouse_read (void)
{
  for (;;)
    {
      uint32_t status = vmware_call (VMMOUSE_STATUS, 0).eax;
      if ((status & VMMOUSE_STATUS_ERROR) == VMMOUSE_STATUS_ERROR)
        {
          // it lost track, starting it again clears its queue
          vmmouse_init ();
          return;
        }
      if ((status & 0xFFFF) < VMMOUSE_PACKET_WORDS)
        {
          return;
        }
      struct backdoor p = vmware_call (VMMOUSE_DATA, VMMOUSE_PACKET_WORDS);
      uint8_t buttons = ((p.eax & VMMOUSE_LEFT) ? MOUSE_LEFT : 0)
                        | ((p.eax & VMMOUSE_RIGHT) ? MOUSE_RIGHT : 0)
                        | ((p.eax & VMMOUSE_MIDDLE) ? MOUSE_MIDDLE : 0);
      event_push ((p.ebx & 0xFFFF) >> 1, (p.ecx & 0xFFFF) >> 1, buttons, 1);
    }
}

void
mouse_handle_interrupt (void)
{
  uint8_t data = insb (PS2_DATA);
  // a lost byte would shift every packet after it, the first byte always
  // has bit 3 set so wait for one that does. the ack of the enable command
  // can come in late, after the setup stopped reading
  if (packet_len == 0 && (!(data & PACKET_ALWAYS_SET) || data == MOUSE_ACK))
    {
      return;
    }
  packet[packet_len++] = data;
  if (packet_len < 3)
    {
      return;
    }
  packet_len = 0;
  // with the vmmouse on, the packets only raise irq12 and the vmmouse has
  // the position
  if (vmmouse)
    {
      vmmouse_read ();
      return;
    }
  uint8_t flags = packet[0];
  if (flags & PACKET_OVERFLOW)
    {
      return;
    }
  // 9 bit two's complement, the sign bits are in the first byte
  int16_t dx = packet[1] - ((flags & PACKET_X_SIGN) ? 256 : 0);
  int16_t dy = packet[2] - ((flags & PACKET_Y_SIGN) ? 256 : 0);
  // the mouse counts up going away from the user, the screen counts down
  event_push (dx, -dy, flags & 0x07, 0);
}

int32_t
mouse_get_event (struct mouse_event *out)
{
  if (events_tail == events_head)
    {
      return -1;
    }
  out->dx = events[events_tail].dx;
  out->dy = events[events_tail].dy;
  out->buttons = events[events_tail].buttons;
  out->absolute = events[events_tail].absolute;
  out->reserved[0] = out->reserved[1] = 0;
  events_tail = (events_tail + 1) % MOUSE_EVENTS_SIZE;
  return 0;
}

void
mouse_flush_events (void)
{
  events_tail = events_head;
}
