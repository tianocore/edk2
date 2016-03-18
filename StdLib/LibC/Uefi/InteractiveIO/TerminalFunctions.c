/** @file
    "Terminal" Control functions for Interactive IO.

    Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/BaseMemoryLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <sys/termios.h>
#include  <Device/IIO.h>
#include  <MainData.h>

/** Get input baud rate.

    Extracts the input baud rate from the termios structure pointed to by the
    pTermios argument.

    @param[in]  pTermios  A pointer to the termios structure from which to extract
                          the input baud rate.

    @return The value of the input speed is returned exactly as it is contained
            in the termios structure, without interpretation.
**/
speed_t
cfgetispeed (
  const struct termios *pTermios
  )
{
  return pTermios->c_ispeed;
}

/** Get output baud rate.

    Extracts the output baud rate from the termios structure pointed to by the
    pTermios argument.

    @param[in]  pTermios  A pointer to the termios structure from which to extract
                          the output baud rate.

    @return The value of the output speed is returned exactly as it is contained
            in the termios structure, without interpretation.
**/
speed_t
cfgetospeed (
  const struct termios *pTermios
  )
{
  return pTermios->c_ospeed;
}

/** Set input baud rate.

    Replaces the input baud rate, in the termios structure pointed to by the
    pTermios argument, with the value of NewSpeed.

    @param[out]   pTermios  A pointer to the termios structure into which to set
                            the input baud rate.
    @param[in]    NewSpeed  The new input baud rate.

    @retval 0     The operation completed successfully.
    @retval -1    An error occured and errno is set to indicate the error.
                    * EINVAL - The value of NewSpeed is outside the range of
                      possible speed values as specified in <sys/termios.h>.
**/
int
cfsetispeed (
  struct termios *pTermios,
  speed_t         NewSpeed
  )
{
  int RetVal;

  if(NewSpeed < B921600) {
    pTermios->c_ispeed = NewSpeed;
    RetVal = 0;
  }
  else {
    RetVal = -1;
    errno = EINVAL;
  }
  return RetVal;
}

/** Set output baud rate.

    Replaces the output baud rate, in the termios structure pointed to by the
    pTermios argument, with the value of NewSpeed.

    @param[out]   pTermios  A pointer to the termios structure into which to set
                            the output baud rate.
    @param[in]    NewSpeed  The new output baud rate.

    @retval 0     The operation completed successfully.
    @retval -1    An error occured and errno is set to indicate the error.
                    * EINVAL - The value of NewSpeed is outside the range of
                      possible speed values as specified in <sys/termios.h>.
**/
int
cfsetospeed (
  struct termios *pTermios,
  speed_t         NewSpeed
  )
{
  int RetVal;

  if(NewSpeed < B921600) {
    pTermios->c_ospeed = NewSpeed;
    RetVal = 0;
  }
  else {
    RetVal = -1;
    errno = EINVAL;
  }
  return RetVal;
}

/** Get the parameters associated with an interactive IO device.

    Get the parameters associated with the device referred to by
    fd and store them into the termios structure referenced by pTermios.

    @param[in]    fd        The file descriptor for an open interactive IO device.
    @param[out]   pTermios  A pointer to a termios structure into which to store
                            attributes of the interactive IO device.

    @retval 0     The operation completed successfully.
    @retval -1    An error occured and errno is set to indicate the error.
                    * EBADF - The fd argument is not a valid file descriptor.
                    * ENOTTY - The file associated with fd is not an interactive IO device.
**/
int
tcgetattr (
  int             fd,
  struct termios *pTermios
  )
{
  cIIO             *IIO;
  int               RetVal;
  struct __filedes *filp;
  struct termios   *Termio;

  RetVal = 0;
  if(ValidateFD( fd, VALID_OPEN)) {
    filp = &gMD->fdarray[fd];

    if((filp->f_iflags & _S_ITTY) != 0) {
      // fd is for a TTY or "Interactive IO" device
      IIO = (cIIO *)filp->devdata;
      Termio = &IIO->Termio;
      (void)CopyMem((void *)pTermios, (const void *)Termio, sizeof(struct termios));
    }
    else {
      errno   = ENOTTY;
      RetVal  = -1;
    }
  }
  else {
    errno   = EBADF;
    RetVal  = -1;
  }
  return RetVal;
}

/** Set the parameters associated with an interactive IO device.

    Set the parameters associated with the device referred to by
    fd to the values in the termios structure referenced by pTermios.

    Behavior is modified by the value of the OptAct parameter:
      * TCSANOW: The change shall occur immediately.
      * TCSADRAIN: The change shall occur after all output written to fd is
        transmitted. This action should be used when changing parameters which
        affect output.
      * TCSAFLUSH: The change shall occur after all output written to fd is
        transmitted, and all input so far received but not read shall be
        discarded before the change is made.

    @param[in]  fd        The file descriptor for an open interactive IO device.
    @param[in]  OptAct    Currently has no effect.
    @param[in]  pTermios  A pointer to a termios structure into which to retrieve
                          attributes to set in the interactive IO device.

    @retval 0     The operation completed successfully.
    @retval -1    An error occured and errno is set to indicate the error.
                    * EBADF - The fd argument is not a valid file descriptor.
                    * ENOTTY - The file associated with fd is not an interactive IO device.
**/
int
tcsetattr (
  int                   fd,
  int                   OptAct,   // Currently ignored
  const struct termios *pTermios
  )
{
  cIIO             *IIO;
  int               RetVal;
  struct __filedes *filp;
  struct termios   *Termio;

  RetVal = 0;
  if(ValidateFD( fd, VALID_OPEN)) {
    filp = &gMD->fdarray[fd];

    if((filp->f_iflags & _S_ITTY) != 0) {
      // fd is for a TTY or "Interactive IO" device
      IIO = (cIIO *)filp->devdata;
      Termio = &IIO->Termio;
      (void)CopyMem((void *)Termio, (const void *)pTermios, sizeof(struct termios));
    }
    else {
      errno   = ENOTTY;
      RetVal  = -1;
    }
  }
  else {
    errno   = EBADF;
    RetVal  = -1;
  }
  return RetVal;
}

/** Transmit pending output.

    Function is not yet implemented for UEFI.

    @param[in]  fd        Ignored

    @retval -1    This function is not yet supported.  errno is set to ENOTSUP.
**/
int
tcdrain (int fd)
{
  errno = ENOTSUP;
  return -1;
}

/** Suspend or restart the transmission or reception of data.

    This function will suspend or resume transmission or reception of data on
    the file referred to by fd, depending on the value of Action.

    Function is not yet implemented for UEFI.

    @param[in]  fd        Ignored
    @param[in]  Action    Ignored

    @retval -1    This function is not yet supported.  errno is set to ENOTSUP.
**/
int
tcflow (
  int fd,
  int Action)
{
  errno = ENOTSUP;
  return -1;
}

/** Discard non-transmitted output data, non-read input data, or both.

    Function is not yet implemented for UEFI.

    @param[in]  fd              Ignored
    @param[in]  QueueSelector   Ignored

    @retval -1    This function is not yet supported.  errno is set to ENOTSUP.
**/
int
tcflush (
  int fd,
  int QueueSelector)
{
  errno = ENOTSUP;
  return -1;
}

