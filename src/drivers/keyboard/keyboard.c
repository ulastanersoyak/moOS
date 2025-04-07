#include "keyboard.h"
#include "../../kernel/io/io.h"

#include <stddef.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_BUFFER_SIZE 128

// scancode set 1 make codes
#define SC_BACKSPACE 0x0E
#define SC_LCTRL 0x1D
#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_CAPSLOCK 0x3A
#define SC_EXTENDED 0xE0
#define SC_RELEASED 0x80

// us layout, indexed by scancode. 0 means the key has no char
static const char keymap[] = {
  0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
  '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
  'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
  'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
  'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ',
};

static const char keymap_shift[] = {
  0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
  'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',  'Z',
  'X',  'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ',
};

static volatile char buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t buffer_head = 0; // next slot to write
static volatile uint32_t buffer_tail = 0; // next slot to read

static uint8_t shift_held = 0;
static uint8_t ctrl_held = 0;
static uint8_t caps_lock = 0;
static uint8_t extended = 0;
static volatile uint8_t ctrl_c_pressed = 0;

static void
buffer_push (char c)
{
  uint32_t next = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
  // drop the char when buffer is full
  if (next == buffer_tail)
    {
      return;
    }
  buffer[buffer_head] = c;
  buffer_head = next;
}

static char
translate (uint8_t scancode)
{
  if (scancode >= sizeof (keymap))
    {
      return 0;
    }
  char c = keymap[scancode];
  uint8_t upper = shift_held;
  // caps lock only affects letters
  if (c >= 'a' && c <= 'z')
    {
      upper ^= caps_lock;
    }
  return upper ? keymap_shift[scancode] : c;
}

void
keyboard_handle_interrupt (void)
{
  // scancode has to be read or the keyboard controller wont send new ones
  uint8_t scancode = insb (KEYBOARD_DATA_PORT);
  if (scancode == SC_EXTENDED)
    {
      extended = 1;
      return;
    }
  uint8_t released = scancode & SC_RELEASED;
  scancode &= ~SC_RELEASED;

  if (extended)
    {
      extended = 0;
      // right ctrl shares the left ctrl scancode
      if (scancode == SC_LCTRL)
        {
          ctrl_held = !released;
          return;
        }
      if (released)
        {
          return;
        }
      switch (scancode)
        {
        case 0x48:
          buffer_push (KEY_UP);
          break;
        case 0x50:
          buffer_push (KEY_DOWN);
          break;
        case 0x4B:
          buffer_push (KEY_LEFT);
          break;
        case 0x4D:
          buffer_push (KEY_RIGHT);
          break;
        case 0x47:
          buffer_push (KEY_HOME);
          break;
        case 0x4F:
          buffer_push (KEY_END);
          break;
        case 0x53:
          buffer_push (KEY_DELETE);
          break;
        }
      return;
    }

  switch (scancode)
    {
    case SC_LSHIFT:
    case SC_RSHIFT:
      shift_held = !released;
      return;
    case SC_LCTRL:
      ctrl_held = !released;
      return;
    case SC_CAPSLOCK:
      if (!released)
        {
          caps_lock = !caps_lock;
        }
      return;
    }

  if (released)
    {
      return;
    }

  char c = translate (scancode);
  if (!c)
    {
      return;
    }
  if (ctrl_held && (c == 'c' || c == 'C'))
    {
      ctrl_c_pressed = 1;
      c = KEY_CTRL_C;
    }
  else if (ctrl_held && (c == 'd' || c == 'D'))
    {
      // end of input for programs reading the terminal
      c = KEY_CTRL_D;
    }
  buffer_push (c);
}

int32_t
keyboard_getchar (void)
{
  if (buffer_tail == buffer_head)
    {
      return -1;
    }
  char c = buffer[buffer_tail];
  buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
  return (unsigned char)c;
}

uint8_t
keyboard_take_ctrl_c (void)
{
  uint8_t pressed = ctrl_c_pressed;
  ctrl_c_pressed = 0;
  return pressed;
}

void
keyboard_flush (void)
{
  buffer_tail = buffer_head;
  ctrl_c_pressed = 0;
}
