/** @file
  Interface of IPsec printing debug information.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecImpl.h"
#include "IpSecDebug.h"

//
// The print title for IKEv1 variety phase.
//
CHAR8 *mStateStr[] = {
  "IKEv1_MAIN_1",
  "IKEv1_MAIN_2",
  "IKEv1_MAIN_3",
  "IKEv1_MAIN_ESTABLISHED",
  "IKEv1_QUICK_1",
  "IKEv1_QUICK_2",
  "IKEv1_QUICK_ESTABLISHED"
};
//
// The print title for IKEv1 variety Exchagne.
//
CHAR8 *mExchangeStr[] = {
  "IKEv1 Main Exchange",
  "IKEv1 Info Exchange",
  "IKEv1 Quick Exchange",
  "IKEv1 Unknown Exchange"
};

//
// The print title for IKEv1 variety Payload.
//
CHAR8 *mPayloadStr[] = {
  "IKEv1 None Payload",
  "IKEv1 SA Payload",
  "IKEv1 Proposal Payload",
  "IKEv1 Transform Payload",
  "IKEv1 KE Payload",
  "IKEv1 ID Payload",
  "IKEv1 Certificate Payload",
  "IKEv1 Certificate Request Payload",
  "IKEv1 Hash Payload",
  "IKEv1 Signature Payload",
  "IKEv1 Nonce Payload",
  "IKEv1 Notify Payload",
  "IKEv1 Delete Payload",
  "IKEv1 Vendor Payload"
};

/**
  Print the IP address.

  @param[in]  Level     Debug print error level. Pass to DEBUG().
  @param[in]  Ip        Point to a specified IP address.
  @param[in]  IpVersion The IP Version.

**/
VOID
IpSecDumpAddress (
  IN UINTN               Level,
  IN EFI_IP_ADDRESS      *Ip,
  IN UINT8               IpVersion
  )
{
  if (IpVersion == IP_VERSION_6) {
    DEBUG (
      (Level,
      "%x%x:%x%x:%x%x:%x%x",
      Ip->v6.Addr[0],
      Ip->v6.Addr[1],
      Ip->v6.Addr[2],
      Ip->v6.Addr[3],
      Ip->v6.Addr[4],
      Ip->v6.Addr[5],
      Ip->v6.Addr[6],
      Ip->v6.Addr[7])
      );
    DEBUG (
      (Level,
      ":%x%x:%x%x:%x%x:%x%x\n",
      Ip->v6.Addr[8],
      Ip->v6.Addr[9],
      Ip->v6.Addr[10],
      Ip->v6.Addr[11],
      Ip->v6.Addr[12],
      Ip->v6.Addr[13],
      Ip->v6.Addr[14],
      Ip->v6.Addr[15])
      );
  } else {
    DEBUG (
      (Level,
      "%d.%d.%d.%d\n",
      Ip->v4.Addr[0],
      Ip->v4.Addr[1],
      Ip->v4.Addr[2],
      Ip->v4.Addr[3])
      );
  }

}

/**
  Print IKEv1 Current states.

  @param[in]  Previous    The Previous state of IKEv1.
  @param[in]  Current     The current state of IKEv1.

**/
VOID
IpSecDumpState (
  IN UINT32              Previous,
  IN UINT32              Current
  )
{
  if (Previous == Current) {
    DEBUG ((DEBUG_INFO, "\n****Current state is %a\n", mStateStr[Previous]));
  } else {
    DEBUG ((DEBUG_INFO, "\n****Change state from %a to %a\n", mStateStr[Previous], mStateStr[Current]));
  }

}

/**
  Print the buffer in form of Hex.

  @param[in]  Title       The strings to be printed before the data of the buffer.
  @param[in]  Data        Points to buffer to be printed.
  @param[in]  DataSize    The size of the buffer to be printed.

**/
VOID
IpSecDumpBuf (
  IN CHAR8                 *Title,
  IN UINT8                 *Data,
  IN UINTN                 DataSize
  )
{
  UINTN Index;
  UINTN DataIndex;
  UINTN BytesRemaining;
  UINTN BytesToPrint;

  DataIndex       = 0;
  BytesRemaining  = DataSize;

  DEBUG ((DEBUG_INFO, "==%a %d bytes==\n", Title, DataSize));

  while (BytesRemaining > 0) {

    BytesToPrint = (BytesRemaining > IPSEC_DEBUG_BYTE_PER_LINE) ? IPSEC_DEBUG_BYTE_PER_LINE : BytesRemaining;

    for (Index = 0; Index < BytesToPrint; Index++) {
      DEBUG ((DEBUG_INFO, " 0x%02x,", Data[DataIndex++]));
    }

    DEBUG ((DEBUG_INFO, "\n"));
    BytesRemaining -= BytesToPrint;
  }

}
