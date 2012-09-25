/** @file
    Macros and declarations for terminal oriented ioctls and
    I/O discipline.

    Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1988, 1989, 1993, 1994
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)termios.h 8.3 (Berkeley) 3/28/94
    NetBSD: termios.h,v 1.29 2005/12/11 12:25:21 christos Exp
**/
#ifndef _SYS_TERMIOS_H_
#define _SYS_TERMIOS_H_

#include <sys/ansi.h>
#include <sys/featuretest.h>

/*  Special Control Characters
 *
 * Index into c_cc[] character array.
 */
typedef enum {
/* Name     Enabled by */
  VTABLEN,  /* OXTABS - Length between TAB stops. */
  VEOF,     /* ICANON */
  VEOL,     /* ICANON */
  VERASE,   /* ICANON */
  VKILL,    /* ICANON */
  VINTR,    /* ISIG */
  VQUIT,    /* ISIG */
  VMIN,     /* !ICANON */
  VTIME,    /* !ICANON */

  /* Extensions from BSD and POSIX -- Not yet implemented for UEFI */
  VWERASE,   /* IEXTEN, ICANON -- Erase the WORD to the left of the cursor */
  VREPRINT,  /* IEXTEN, ICANON -- Re-draw the current line (input buffer) */
  VLNEXT,    /* IEXTEN, ICANON -- Input the next character literally */
  VDISCARD,  /* IEXTEN -- Toggle.  Discards output display until toggled. */

  /* NCCS must always be the last member of this enum. */
  NCCS      /* Number of control characters in c_cc[]  */
} CCC_INDEX;

#define _POSIX_VDISABLE ((unsigned char)'\377')

#define CCEQ(val, c)  (c == val ? val != _POSIX_VDISABLE : 0)

/*
 * Input flags - software input processing
  c_iflag
*/
#define   INLCR     0x0001  /* map NL into CR */
#define   IGNCR     0x0002  /* ignore CR */
#define   ICRNL     0x0004  /* map CR to NL (ala CRMOD) */
#define   IGNSPEC   0x0008  /* Ignore function, control, and other non-printing special keys. */
#ifdef  HAVE_DA_SERIAL
  #define ISTRIP    0x0010  /* strip 8th bit off chars */
  #define IGNBRK    0x0020  /* ignore BREAK condition */
  #define BRKINT    0x0040  /* map BREAK to SIGINTR */
  #define IRESRV1   0x0080
  #define IGNPAR    0x0100  /* ignore (discard) parity errors */
  #define PARMRK    0x0200  /* mark parity and framing errors */
  #define INPCK     0x0400  /* enable checking of parity errors */
  #define IXON      0x0800  /* enable output flow control */
  #define IXOFF     0x1000  /* enable input flow control */
  #define IXANY     0x2000  /* any char will restart after stop */
#endif  /* HAVE_DA_SERIAL */

/*
 * Output flags - software output processing
  c_oflag
 */
#define OPOST     0x0001  /* enable following output processing */
#define ONLCR     0x0002  /* map NL to CR-NL (ala CRMOD) */
#define OXTABS    0x0004  /* expand tabs to spaces */
#define ONOEOT    0x0008  /* discard EOT's (^D) on output */
#define OCRNL     0x0010  /* map CR to NL */
#define ONOCR     0x0020  /* discard CR's when on column 0 */
#define ONLRET    0x0040  /* move to column 0 on CR */
#define OCTRL     0x0080  /* Map control characters to the sequence ^C */

/*
 * Control flags - hardware control of terminal
  c_cflag
 */
#ifdef  HAVE_DA_SERIAL
  #define CIGNORE   0x0001      /* ignore control flags */
  #define CSIZE     0x0300      /* character size mask */
  #define     CS5       0x0000      /* 5 bits (pseudo) */
  #define     CS6       0x0100      /* 6 bits */
  #define     CS7       0x0200      /* 7 bits */
  #define     CS8       0x0300      /* 8 bits */
  #define CSTOPB    0x0400      /* send 2 stop bits, else 1 */
  #define CREAD     0x0800      /* enable receiver */
  #define PARENB    0x1000      /* parity enable */
  #define PARODD    0x2000      /* odd parity, else even */
  #define HUPCL     0x4000      /* hang up on last close */
  #define CLOCAL    0x8000      /* ignore modem status lines */
#endif


/*
 * "Local" flags - dumping ground for other state
 *
 * Warning: some flags in this structure begin with
 * the letter "I" and look like they belong in the
 * input flag.
 */
#define ECHO      0x0001    /* enable echoing */
#define ECHOE     0x0002    /* visually erase chars */
#define ECHOK     0x0004    /* echo NL after line kill */
#define ECHONL    0x0008    /* echo NL even if ECHO is off */
#define ISIG      0x0010    /* enable signals INTR, QUIT, [D]SUSP */
#define ICANON    0x0020    /* canonicalize input lines */
#define IEXTEN    0x0040    /* enable Extensions */
#define SKIP_1    0x0100    /* Currently unused */
#define TOSTOP    0x0200    /* stop background jobs on output */
#define PENDIN    0x0400    /* re-echo input buffer at next read */
#define NOFLSH    0x0800    /* don't flush output on signal */
#define FLUSHO    0x1000    /* output being flushed (state) */

typedef INT8    cc_t;
typedef UINT16  tcflag_t;
typedef UINT32  speed_t;

struct termios {
  INT32     c_ispeed;   /* input speed    - Use a signed type instead of speed_t */
  INT32     c_ospeed;   /* output speed   - to ease integer promotion when used. */
  tcflag_t  c_iflag;    /* input flags */
  tcflag_t  c_oflag;    /* output flags */
  tcflag_t  c_cflag;    /* control flags */
  tcflag_t  c_lflag;    /* local flags */
  cc_t      c_cc[NCCS]; /* control chars */
};

/*
 * Commands passed to tcsetattr() for setting the termios structure.
 */
#define TCSANOW     0       /* make change immediate */
#define TCSADRAIN   1       /* drain output, then change */
#define TCSAFLUSH   2       /* drain output, flush input */
#define TCSASOFT    0x10    /* flag - don't alter h.w. state */

/*
 * Standard speeds
 */
#define B0            0
#define B50          50
#define B75          75
#define B110        110
#define B134        134
#define B150        150
#define B200        200
#define B300        300
#define B600        600
#define B1200      1200
#define B1800      1800
#define B2400      2400
#define B4800      4800
#define B9600      9600
#define B19200    19200
#define B38400    38400

// Extended speed definitions
#define B7200      7200
#define B14400    14400
#define B28800    28800
#define B57600    57600
#define B76800    76800
#define B115200  115200
#define B230400  230400
#define B460800  460800
#define B921600  921600

#define TCIFLUSH  1
#define TCOFLUSH  2
#define TCIOFLUSH 3
#define TCOOFF    1
#define TCOON     2
#define TCIOFF    3
#define TCION     4

#include <sys/EfiCdefs.h>

__BEGIN_DECLS
speed_t cfgetispeed (const struct termios *);
speed_t cfgetospeed (const struct termios *);
int     cfsetispeed (struct termios *, speed_t);
int     cfsetospeed (struct termios *, speed_t);
int     tcgetattr   (int, struct termios *);
int     tcsetattr   (int, int, const struct termios *);
int     tcdrain     (int);
int     tcflow      (int, int);
int     tcflush     (int, int);
//int     tcsendbreak (int, int);
//pid_t   tcgetsid    (int);


//void    cfmakeraw   (struct termios *);
//int     cfsetspeed  (struct termios *, speed_t);
__END_DECLS

/*  Input values for UEFI Keyboard Scan Codes.

    The UEFI Keyboard Scan Codes are mapped into the upper range of the Unicode
    Private Use Area so that the characters can be inserted into the input stream
    and treated the same as any other character.

    These values are only used for input.  If these codes are output to the
    console, or another interactive I/O device, the behavior will depend upon
    the current locale and UEFI character set loaded.
*/
typedef enum {
  TtySpecKeyMin = 0xF7F0,
  /* This area is reserved for use by internal I/O software.
      At least 4 values must exist between TtySpecKeyMin and TtyFunKeyMin.
  */
  TtyFunKeyMin  = 0xF7FA,
  TtyKeyEject   = 0xF7FA,
  TtyRecovery,         TtyToggleDisplay,    TtyHibernate,
  TtySuspend,          TtyBrightnessDown,   TtyBrightnessUp,
  TtyVolumeDown = 0xF87F,
  TtyVolumeUp,         TtyMute,
  TtyF24        = 0xF88D,
  TtyF23,              TtyF22,              TtyF21,              TtyF20,
  TtyF19,              TtyF18,              TtyF17,              TtyF16,
  TtyF15,              TtyF14,              TtyF13,
  TtyEscape     = 0xF8E9,
  TtyF12,              TtyF11,              TtyF10,              TtyF9,
  TtyF8,               TtyF7,               TtyF6,               TtyF5,
  TtyF4,               TtyF3,               TtyF2,               TtyF1,
  TtyPageDown,         TtyPageUp,           TtyDelete,           TtyInsert,
  TtyEnd,              TtyHome,             TtyLeftArrow,        TtyRightArrow,
  TtyDownArrow,
  TtyUpArrow    = 0xF8FF,
  TtyFunKeyMax  = 0xF900
} TtyFunKey;

// Non-UEFI character definitions
#define CHAR_EOT    0x0004        /* End of Text (EOT) character */

#endif /* !_SYS_TERMIOS_H_ */
