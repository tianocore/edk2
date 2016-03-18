/** @file

    Implementation and Platform specific portion of <signal.h>.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _SYS_SIGNAL_H
#define _SYS_SIGNAL_H
#include  <sys/EfiCdefs.h>
#include  <machine/signal.h>

/** The actual (default) signal numbers are assigned using an anonymous enum
    so that the compiler can do the work of assigning values.  This helps
    ensure that the developer should never have to renumber the signals or
    figure out what number to assign to a new signal.

    Properly constructed programs will NEVER depend upon signal numbers being
    in a particular order or having a particular value.  All that is guaranteed
    is that each signal number is distinct, positive, and non-zero.
**/
enum {
  __SigInt    = 1,
  __SigIll,
  __SigAbrt,
  __SigFpe,
  __SigSegv,
  __SigTerm,
  __SigBreak,
  __SigAlrm,
  __SigVtAlrm,
  __SigProf,
  __SigUsr1,
  __SigUsr2,
  __SigWinch,
  __SigPipe,
  __SigQuit,
  __Sig_Last
};

/** The type of a signal handler function. **/
typedef void __sighandler_t(int);

__BEGIN_DECLS
/** The signal function associates a "signal handler" with a signal number.

    For historical reasons; programs expect signal to be declared
    in <sys/signal.h>.

    @param[in]  sig       Signal number that function is to be associated with.
    @param[in]  function  The "handler" function to be associated with signal sig.

    @return     If the request can be honored, the signal function returns the
                value of func for the most recent successful call to signal for
                the specified signal sig. Otherwise, a value of SIG_ERR is
                returned and a positive value is stored in errno.
 */
__sighandler_t  *signal(int sig, __sighandler_t *func);

__END_DECLS

#endif    /* _SYS_SIGNAL_H */
