/** @file
  IP4 option support functions.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip4Impl.h"


/**
  Validate the IP4 option format for both the packets we received
  and will transmit.

  @param[in]  Option            The first byte of the option
  @param[in]  OptionLen         The length of the whole option
  @param[in]  Rcvd              The option is from the packet we received if TRUE,
                                otherwise the option we wants to transmit.

  @retval TRUE     The option is properly formatted
  @retval FALSE    The option is mal-formated

**/
BOOLEAN
Ip4OptionIsValid (
  IN UINT8                  *Option,
  IN UINT32                 OptionLen,
  IN BOOLEAN                Rcvd
  )
{
  UINT32                    Cur;
  UINT32                    Len;
  UINT32                    Point;

  Cur       = 0;

  while (Cur < OptionLen) {
    switch (Option[Cur]) {
    case IP4_OPTION_NOP:
      Cur++;
      break;

    case IP4_OPTION_EOP:
      Cur = OptionLen;
      break;

    case IP4_OPTION_LSRR:
    case IP4_OPTION_SSRR:
    case IP4_OPTION_RR:
      Len   = Option[Cur + 1];
      Point = Option[Cur + 2];

      //
      // SRR/RR options are formatted as |Type|Len|Point|Ip1|Ip2|...
      //
      if ((OptionLen - Cur < Len) || (Len < 3) || ((Len - 3) % 4 != 0)) {
        return FALSE;
      }

      if ((Point > Len + 1) || (Point % 4 != 0)) {
        return FALSE;
      }

      //
      // The Point must point pass the last entry if the packet is received
      // by us. It must point to 4 if the packet is to be sent by us for
      // source route option.
      //
      if ((Option[Cur] != IP4_OPTION_RR) &&
          ((Rcvd && (Point != Len + 1)) || (!Rcvd && (Point != 4)))) {

        return FALSE;
      }

      Cur += Len;
      break;

    default:
      Len = Option[Cur + 1];

      if ((OptionLen - Cur < Len) || (Len < 2)) {
        return FALSE;
      }

      Cur = Cur + Len;
      break;
    }

  }

  return TRUE;
}


/**
  Copy the option from the original option to buffer. It
  handles the details such as:
  1. whether copy the single IP4 option to the first/non-first
     fragments.
  2. Pad the options copied over to aligned to 4 bytes.

  @param[in]       Option            The original option to copy from
  @param[in]       OptionLen         The length of the original option
  @param[in]       FirstFragment     Whether it is the first fragment
  @param[in, out]  Buf               The buffer to copy options to. NULL
  @param[in, out]  BufLen            The length of the buffer

  @retval EFI_SUCCESS           The options are copied over
  @retval EFI_BUFFER_TOO_SMALL  Buf is NULL or BufLen provided is too small.

**/
EFI_STATUS
Ip4CopyOption (
  IN     UINT8              *Option,
  IN     UINT32             OptionLen,
  IN     BOOLEAN            FirstFragment,
  IN OUT UINT8              *Buf,           OPTIONAL
  IN OUT UINT32             *BufLen
  )
{
  UINT8                     OptBuf[40];
  UINT32                    Cur;
  UINT32                    Next;
  UINT8                     Type;
  UINT32                    Len;

  ASSERT ((BufLen != NULL) && (OptionLen <= 40));

  Cur   = 0;
  Next  = 0;

  while (Cur < OptionLen) {
    Type  = Option[Cur];
    Len   = Option[Cur + 1];

    if (Type == IP4_OPTION_NOP) {
      //
      // Keep the padding, in case that the sender wants to align
      // the option, say, to 4 bytes
      //
      OptBuf[Next] = IP4_OPTION_NOP;
      Next++;
      Cur++;

    } else if (Type == IP4_OPTION_EOP) {
      //
      // Don't append the EOP to avoid including only a EOP option
      //
      break;

    } else {
      //
      // don't copy options that is only valid for the first fragment
      //
      if (FirstFragment || (Type & IP4_OPTION_COPY_MASK) != 0) {
        CopyMem (OptBuf + Next, Option + Cur, Len);
        Next += Len;
      }

      Cur += Len;
    }
  }

  //
  // Don't append an EOP only option.
  //
  if (Next == 0) {
    *BufLen = 0;
    return EFI_SUCCESS;
  }

  //
  // Append an EOP if the end of option doesn't coincide with the
  // end of the IP header, that is, isn't aligned to 4 bytes..
  //
  if ((Next % 4) != 0) {
    OptBuf[Next] = IP4_OPTION_EOP;
    Next++;
  }

  //
  // Head length is in the unit of 4 bytes. Now, Len is the
  // acutal option length to appear in the IP header.
  //
  Len = ((Next + 3) &~0x03);

  //
  // If the buffer is too small, set the BufLen then return
  //
  if ((Buf == NULL) || (*BufLen < Len)) {
    *BufLen = Len;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Copy the option to the Buf, zero the buffer first to pad
  // the options with NOP to align to 4 bytes.
  //
  ZeroMem (Buf, Len);
  CopyMem (Buf, OptBuf, Next);
  *BufLen = Len;
  return EFI_SUCCESS;
}
