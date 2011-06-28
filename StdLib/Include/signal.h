/** @file
  The header <signal.h> declares a type and two functions and defines several
  macros, for handling various signals (conditions that may be reported during
  program execution).

  The UEFI implementation of <signal.h> maps signals onto the UEFI
  event mechanism.

  An implementation need not generate any of these signals, except as a result
  of explicit calls to the raise function. Additional signals and pointers to
  undeclarable functions, with macro definitions beginning, respectively, with
  the letters SIG and an uppercase letter or with SIG_ and an uppercase letter
  may also be specified by the implementation. The complete set of signals,
  their semantics, and their default handling is implementation-defined; all
  signal numbers shall be positive.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _SIGNAL_H
#define _SIGNAL_H
#include  <sys/EfiCdefs.h>
#include  <sys/signal.h>

/*  The type sig_atomic_t is the (possibly volatile-qualified) integer type of
    an object that can be accessed as an atomic entity, even in the presence
    of asynchronous interrupts.

    This, possibly machine specific, type is defined in <machine/signal.h>.
*/

/** The following three macros expand to constant expressions with distinct
    values that have type compatible with the second argument to, and the
    return value of, the signal function, and whose values compare unequal to
    the address of any declarable function.
**/
#define SIG_IGN   ((__sighandler_t *) 0)
#define SIG_DFL   ((__sighandler_t *) 1)
#define SIG_ERR   ((__sighandler_t *) 3)

/** The following members expand to positive integer constant expressions with
    type int and distinct values that are the signal numbers, each
    corresponding to the specified condition.
    Many existing programs expect these to be macros.
**/
#define SIGINT     __SigInt     ///< receipt of an interactive attention signal
#define SIGILL     __SigIll     ///< detection of an invalid function image, such as an invalid instruction
#define SIGABRT    __SigAbrt    ///< abnormal termination, such as is initiated by the abort function
#define SIGFPE     __SigFpe     ///< an erroneous arithmetic operation, such as zero divide or an operation resulting in overflow
#define SIGSEGV    __SigSegv    ///< an invalid access to storage
#define SIGTERM    __SigTerm    ///< a termination request sent to the program
#define SIGBREAK   __SigBreak   ///< added for Python
#define SIG_LAST   __Sig_Last   ///< One more than the largest signal number

__BEGIN_DECLS

/*  For historical reasons; programs expect signal to be declared
    in <sys/signal.h>.  The function is documented in <sys/signal.h>.

    The function is declared in the C Standard as:<BR>
      void (*signal(int sig, void (*func)(int)))(int);
*/

/** Send a signal.

    The raise function carries out the actions described for signal,
    in <sys/signal.h>, for the signal sig. If a signal handler is called, the
    raise function shall not return until after the signal handler does.

    @return   The raise function returns zero if successful,
              nonzero if unsuccessful.
**/
int raise(int sig);

__END_DECLS

#endif  /* _SIGNAL_H */
