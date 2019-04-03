/** @file
Serial conole output and string formating.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "memory_options.h"
#include "general_definitions.h"

// Resource programmed to PCI bridge, 1MB bound alignment is needed.
// The default value is overwritten by MRC parameter, assuming code
// relocated to eSRAM.
uint32_t UartMmioBase = 0;

// Serial port registers based on SerialPortLib.c
#define R_UART_BAUD_THR       0
#define R_UART_LSR            20

#define   B_UART_LSR_RXRDY    BIT0
#define   B_UART_LSR_TXRDY    BIT5
#define   B_UART_LSR_TEMT     BIT6

// Print mask see DPF and D_Xxxx
#define DPF_MASK  DpfPrintMask

// Select class of messages enabled for printing
uint32_t DpfPrintMask =
    D_ERROR |
    D_INFO |
    // D_REGRD |
    // D_REGWR |
    // D_FCALL |
    // D_TRN |
    0;

#ifdef NDEBUG
// Don't generate debug code
void dpf( uint32_t mask, char_t* bla, ...)
{
  return;
}

uint8_t mgetc(void)
{
  return 0;
}

uint8_t mgetch(void)
{
  return 0;
}

#else

#ifdef SIM
// Use Vpi console in simulation environment
#include <vpi_user.h>

void dpf( uint32_t mask, char_t* bla, ...)
{
  va_list va;

  if( 0 == (mask & DPF_MASK)) return;

  va_start( va, bla);
  vpi_vprintf( bla, va);
  va_end(va);
}

#else

#ifdef EMU
// Use standard console in windows environment
#include <stdio.h>
#endif

// Read character from serial port
uint8_t mgetc(void)
{
#ifdef EMU

  // Emulation in Windows environment uses console
  getchar();

#else
  uint8_t c;

  while ((*(volatile uint8_t*) (UartMmioBase + R_UART_LSR) & B_UART_LSR_RXRDY) == 0);
  c = *(volatile uint8_t*) (UartMmioBase + R_UART_BAUD_THR);

  return c;
#endif
}


uint8_t mgetch(void)
{
#ifdef EMU
  return 0;
#else
  uint8_t c = 0;

  if((*(volatile uint8_t*) (UartMmioBase + R_UART_LSR) & B_UART_LSR_RXRDY) != 0)
  {
    c = *(volatile uint8_t*) (UartMmioBase + R_UART_BAUD_THR);
  }

  return c;
#endif
}

// Print single character
static void printc(
    uint8_t c)
{
#ifdef EMU

  // Emulation in Windows environment uses console output
  putchar(c);

#else

  //
  // Use MMIO access to serial port on PCI
  //   while( 0 == (0x20 & inp(0x3f8 + 5)));
  //   outp(0x3f8 + 0, c);
  //
  while (0
      == (B_UART_LSR_TEMT & *((volatile uint8_t*) (UartMmioBase + R_UART_LSR))))
    ;
  *((volatile uint8_t*) (UartMmioBase + R_UART_BAUD_THR)) = c;
#endif
}

// Print 0 terminated string on serial console
static void printstr(
    char_t *str)
{
  while (*str)
  {
    printc(*str++);
  }
}
// Print 64bit number as hex string on serial console
// the width parameters allows skipping leading zeros
static void printhexx(
    uint64_t val,
    uint32_t width)
{
  uint32_t i;
  uint8_t c;
  uint8_t empty = 1;

  // 64bit number has 16 characters in hex representation
  for (i = 16; i > 0; i--)
  {
    c = *(((uint8_t *)&val) + ((i - 1) >> 1));
    if (((i - 1) & 1) != 0)
      c = c >> 4;
    c = c & 0x0F;

    if (c > 9)
      c += 'A' - 10;
    else
      c += '0';

    if (c != '0')
    {
      // end of leading zeros
      empty = 0;
    }

    // don't print leading zero
    if (!empty || i <= width)
    {
      printc(c);
    }
  }
}
// Print 32bit number as hex string on serial console
// the width parameters allows skipping leading zeros
static void printhex(
    uint32_t val,
    uint32_t width)
{
  uint32_t i;
  uint8_t c;
  uint8_t empty = 1;

  // 32bit number has 8 characters in hex representation
  for (i = 8; i > 0; i--)
  {
    c = (uint8_t) ((val >> 28) & 0x0F);
    if (c > 9)
      c += 'A' - 10;
    else
      c += '0';

    val = val << 4;

    if (c != '0')
    {
      // end of leading zeros
      empty = 0;
    }

    // don't print leading zero
    if (!empty || i <= width)
    {
      printc(c);
    }
  }
}
// Print 32bit number as decimal string on serial console
// the width parameters allows skipping leading zeros
static void printdec(
    uint32_t val,
    uint32_t width)
{
  uint32_t i;
  uint8_t c = 0;
  uint8_t empty = 1;

  // Ten digits is enough for 32bit number in decimal
  uint8_t buf[10];

  for (i = 0; i < sizeof(buf); i++)
  {
    c = (uint8_t) (val % 10);
    buf[i] = c + '0';
    val = val / 10;
  }

  while (i > 0)
  {
    c = buf[--i];

    if (c != '0')
    {
      // end of leading zeros
      empty = 0;
    }

    // don't print leading zero
    if (!empty || i < width)
    {
      printc(c);
    }
  }
}

// Consume numeric substring leading the given string
// Return pointer to the first non-numeric character
// Buffer reference by width is updated with number
// converted from the numeric substring.
static char_t *getwidth(
    char_t *bla,
    uint32_t *width)
{
  uint32_t val = 0;

  while (*bla >= '0' && *bla <= '9')
  {
    val = val * 10 + *bla - '0';
    bla += 1;
  }

  if (val > 0)
  {
    *width = val;
  }
  return bla;
}

// Consume print format designator from the head of given string
// Return pointer to first character after format designator
// input fmt
// ----- ---
//  s   -> s
//  d   -> d
//  X   -> X
//  llX -> L
static char_t *getformat(
    char_t *bla,
    uint8_t *fmt)
{
  if (bla[0] == 's')
  {
    bla += 1;
    *fmt = 's';
  }
  else if (bla[0] == 'd')
  {
    bla += 1;
    *fmt = 'd';
  }
  else if (bla[0] == 'X' || bla[0] == 'x')
  {
    bla += 1;
    *fmt = 'X';
  }
  else if (bla[0] == 'l' && bla[1] == 'l' && bla[2] == 'X')
  {
    bla += 3;
    *fmt = 'L';
  }

  return bla;
}

// Simplified implementation of standard printf function
// The output is directed to serial console. Only selected
// class of messages is printed (mask has to match DpfPrintMask)
// Supported print formats: %[n]s,%[n]d,%[n]X,,%[n]llX
// The width is ignored for %s format.
void dpf(
    uint32_t mask,
    char_t* bla,
    ...)
{
  uint32_t* arg = (uint32_t*) (&bla + 1);

  // Check UART MMIO base configured
  if (0 == UartMmioBase)
    return;

  // Check event not masked
  if (0 == (mask & DPF_MASK))
    return;

  for (;;)
  {
    uint8_t x = *bla++;
    if (x == 0)
      break;

    if (x == '\n')
    {
      printc('\r');
      printc('\n');
    }
    else if (x == '%')
    {
      uint8_t fmt = 0;
      uint32_t width = 1;

      bla = getwidth(bla, &width);
      bla = getformat(bla, &fmt);

      // Print value
      if (fmt == 'd')
      {
        printdec(*arg, width);
        arg += 1;
      }
      else if (fmt == 'X')
      {
        printhex(*arg, width);
        arg += 1;
      }
      else if (fmt == 'L')
      {
        printhexx(*(uint64_t*) arg, width);
        arg += 2;
      }
      else if (fmt == 's')
      {
        printstr(*(char**) arg);
        arg += 1;
      }
    }
    else
    {
      printc(x);
    }
  }
}

#endif  //SIM
#endif  //NDEBUG
