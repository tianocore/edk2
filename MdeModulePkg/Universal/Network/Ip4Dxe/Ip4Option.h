/** @file
  IP4 option support routines.
  
Copyright (c) 2005 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP4_OPTION_H__
#define __EFI_IP4_OPTION_H__

#define IP4_OPTION_EOP        0
#define IP4_OPTION_NOP        1
#define IP4_OPTION_LSRR       131  // Loss source and record routing,   10000011
#define IP4_OPTION_SSRR       137  // Strict source and record routing, 10001001
#define IP4_OPTION_RR         7    // Record routing, 00000111

#define IP4_OPTION_COPY_MASK  0x80

/**
  Validate the IP4 option format for both the packets we received
  and will transmit. It will compute the ICMP error message fields
  if the option is mal-formated. But this information isn't used.

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
  );

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
  );
#endif
