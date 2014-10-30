/** @file
  NonCanonical Interactive Input Function.

  The functions assume that isatty() is TRUE at the time they are called.
  If _S_IWTTY is set, the device returns WIDE characters.

  Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <LibConfig.h>

#include  <sys/syslimits.h>
#include  <sys/termios.h>
#include  <Containers/Fifo.h>
#include  <Device/IIO.h>

/** Perform a noncanonical read of input.

    @param[in]    filp        Pointer to a file descriptor structure.
    @param[in]    BufferSize  Maximum number of bytes to return.

    @retval    -1   An error has occurred.  Reason in errno.
    @retval    -1   No data returned.  None was ready.
    @retval    >0   The number of elements returned
**/
ssize_t
IIO_NonCanonRead (
  struct __filedes *filp
  )
{
  cIIO           *This;
  cFIFO          *InBuf;
  struct termios *Termio;
  ssize_t         NumRead;
  cc_t            tioMin;
  cc_t            tioTime;
  UINT32          InputType;
  wchar_t         InChar;     // Intermediate character buffer

  NumRead = -1;
  InChar  = 0;      // Initialize so compilers don't complain.
  This    = filp->devdata;
  Termio  = &This->Termio;
  InBuf   = This->InBuf;
  tioMin  = Termio->c_cc[VMIN];
  tioTime = Termio->c_cc[VTIME];

  if(tioMin >= MAX_INPUT) {
    tioMin = MAX_INPUT;
  }
  /*  There are four types of processing that may be done, based on
      the values of tioMin and tioTime.
          Min   Time    Type
          ---   ----    ----
           0      0       0   Return buffer contents or 1 new char
           0     >0       1   Return 0 or 1 character depending on timeout
          >0      0       2   Buffer Min chars. Return BufferSize chars.
          >0     >0       3   Return up to Min chars. Unless the inter-byte timer expires.

    Currently, only type 0 is implemented.
  */
  InputType = 0;
  if(tioMin   != 0)     InputType = 2;
  if(tioTime  != 0)   ++InputType;
  //switch(InputType) {
  //  case 0:
      if(InBuf->IsEmpty(InBuf)) {
        NumRead = filp->f_ops->fo_read(filp, &filp->f_offset, sizeof(wchar_t), &InChar);
        if(NumRead > 0) {
          (void) InBuf->Write(InBuf, &InChar, 1);  // Buffer the character
        }
      }
  //    break;
  //  case 1:
  //    break;
  //  case 2:
  //    break;
  //  case 3:
  //    break;
  //}
  return NumRead;
}
