/** @file
  Implementation of the signal and raise functions as declared in <signal.h>.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Library/UefiLib.h>

#include  <LibConfig.h>
#include  <errno.h>
#include  <signal.h>
#include  <MainData.h>

/** The signal function associates a "signal handler" with a signal number.

    The signal function chooses one of three ways in which receipt of the
    signal number, sig, is to be subsequently handled. If the value of func
    is SIG_DFL, default handling for that signal will occur. If the value of
    func is SIG_IGN, the signal will be ignored.  Otherwise, func shall point
    to a function to be called when that signal occurs. An invocation of such a
    function because of a signal, or (recursively) of any further functions
    called by that invocation (other than functions in the standard library),
    is called a signal handler.

    At program startup, the equivalent of signal(sig, SIG_IGN); may be executed
    for some signals selected in an implementation-defined manner; the
    equivalent of signal(sig, SIG_DFL); is executed for all other signals
    defined by the implementation.

    @return   If the request can be honored, the signal function returns the
              value of func for the most recent successful call to signal for
              the specified signal sig. Otherwise, a value of SIG_ERR is
              returned and a positive value is stored in errno.
 */
__sighandler_t *
signal(int sig, __sighandler_t *func)
{
  __sighandler_t *OldHandler;

  if (sig < 0 || sig >= SIG_LAST) {
    errno = EINVAL;
    return SIG_ERR;
  }
  OldHandler = gMD->sigarray[sig];
  gMD->sigarray[sig] = func;

  return OldHandler;
}

static
void
_defaultSignalHandler( int sig )
{
  Print(L"\nCaught signal %d.\n", sig);
}

/** Send a signal.

    The raise function carries out the actions described for signal, above,
    for the signal sig.

    If a signal handler is called, the raise function shall not return until
    after the signal handler does.

    @return   The raise function returns zero if successful,
              nonzero if unsuccessful.
**/
int
raise( int sig)
{
  __sighandler_t *Handler;

  if (sig < 0 || sig >= SIG_LAST) {
    return EINVAL;
  }
  Handler = gMD->sigarray[sig];

  if(Handler == SIG_DFL) {
    _defaultSignalHandler( sig );
  }
  else if( Handler != SIG_IGN) {
    Handler( sig );
  }
  return 0;
}
