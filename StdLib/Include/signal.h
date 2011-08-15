/** @file
  This file declares a type and two functions and defines several
  macros, for handling various signals (conditions that may be reported during
  program execution).

    For historical reasons; programs expect signal to be declared
    in <sys/signal.h>.  The signal function is documented in <sys/signal.h>.

    The signal function is declared in the C Standard as:<BR>
    void (*signal(int sig, void (*func)(int)))(int);

    The EDK II implementation of the library or base firmware does not generate
    any of these signals, except as a result of explicit calls to the raise function.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

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

/** @{
    The following three macros expand to constant expressions with distinct
    values that have type compatible with the second argument to, and the
    return value of, the signal function, and whose values compare unequal to
    the address of any declarable function.
**/
#define SIG_IGN   ((__sighandler_t *) 0)
#define SIG_DFL   ((__sighandler_t *) 1)
#define SIG_ERR   ((__sighandler_t *) 3)
/*@}*/

/** @{
    The following macros expand to positive integer constant expressions with
    type int and distinct values that are the signal numbers, each
    corresponding to the specified condition.
    The C95 specification requires these to be macros.
**/
#define SIGINT     __SigInt     ///< receipt of an interactive attention signal
#define SIGILL     __SigIll     ///< detection of an invalid function image, such as an invalid instruction
#define SIGABRT    __SigAbrt    ///< abnormal termination, such as is initiated by the abort function
#define SIGFPE     __SigFpe     ///< an erroneous arithmetic operation, such as zero divide or an operation resulting in overflow
#define SIGSEGV    __SigSegv    ///< an invalid access to storage
#define SIGTERM    __SigTerm    ///< a termination request sent to the program
#define SIGBREAK   __SigBreak   ///< added for Python
#define SIGALRM    __SigAlrm    ///< Added for Posix timer functions
#define SIGVTALRM  __SigVtAlrm  ///< Added for Posix timer functions
#define SIGPROF    __SigProf    ///< Added for Posix timer functions
#define SIGUSR1    __SigUsr1    ///< Added for Posix timer functions
#define SIGUSR2    __SigUsr2    ///< Added for Posix timer functions
#define SIGWINCH   __SigWinch   ///< Added for Posix timer functions
#define SIGPIPE    __SigPipe    ///< Added for Posix timer functions
#define SIGQUIT    __SigQuit    ///< Added for Posix timer functions
#define SIG_LAST   __Sig_Last   ///< One more than the largest signal number
/*@}*/

__BEGIN_DECLS

/** Send a signal.

    The raise function carries out the actions described for signal,
    in <sys/signal.h>, for the signal sig. If a signal handler is called, the
    raise function does not return until after the signal handler does.

    @return   The raise function returns zero if successful,
              or nonzero if unsuccessful.
**/
int raise(int sig);

__END_DECLS

#endif  /* _SIGNAL_H */
