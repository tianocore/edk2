/** @file
     Implementation of converting an multicast IP address to multicast HW MAC
     address.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Snp.h"

/**
  Call undi to convert an multicast IP address to a MAC address.

  @param  Snp   Pointer to snp driver structure.
  @param  IPv6  Flag to indicate if this is an ipv6 address.
  @param  IP    Multicast IP address.
  @param  MAC   Pointer to hold the return MAC address.

  @retval EFI_SUCCESS           The multicast IP address was mapped to the
                                multicast HW MAC address.
  @retval EFI_INVALID_PARAMETER Invalid UNDI command.
  @retval EFI_UNSUPPORTED       Command is not supported by UNDI.
  @retval EFI_DEVICE_ERROR      Fail to execute UNDI command.

**/
EFI_STATUS
PxeIp2Mac (
  IN SNP_DRIVER           *Snp,
  IN BOOLEAN              IPv6,
  IN EFI_IP_ADDRESS       *IP,
  IN OUT EFI_MAC_ADDRESS  *MAC
  )
{
  PXE_CPB_MCAST_IP_TO_MAC  *Cpb;
  PXE_DB_MCAST_IP_TO_MAC   *Db;

  if (Snp->Cdb == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Snp->Cdb is NULL\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Cpb               = Snp->Cpb;
  Db                = Snp->Db;
  Snp->Cdb->OpCode  = PXE_OPCODE_MCAST_IP_TO_MAC;
  Snp->Cdb->OpFlags = (UINT16)(IPv6 ? PXE_OPFLAGS_MCAST_IPV6_TO_MAC : PXE_OPFLAGS_MCAST_IPV4_TO_MAC);
  Snp->Cdb->CPBsize = (UINT16)sizeof (PXE_CPB_MCAST_IP_TO_MAC);
  Snp->Cdb->DBsize  = (UINT16)sizeof (PXE_DB_MCAST_IP_TO_MAC);

  Snp->Cdb->CPBaddr = (UINT64)(UINTN)Cpb;
  Snp->Cdb->DBaddr  = (UINT64)(UINTN)Db;

  Snp->Cdb->StatCode  = PXE_STATCODE_INITIALIZE;
  Snp->Cdb->StatFlags = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb->IFnum     = Snp->IfNum;
  Snp->Cdb->Control   = PXE_CONTROL_LAST_CDB_IN_LIST;

  CopyMem (&Cpb->IP, IP, sizeof (PXE_IP_ADDR));

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((DEBUG_NET, "\nSnp->undi.mcast_ip_to_mac()  "));

  (*Snp->IssueUndi32Command)((UINT64)(UINTN)Snp->Cdb);

  switch (Snp->Cdb->StatCode) {
    case PXE_STATCODE_SUCCESS:
      break;

    case PXE_STATCODE_INVALID_CPB:
      return EFI_INVALID_PARAMETER;

    case PXE_STATCODE_UNSUPPORTED:
      DEBUG (
        (DEBUG_NET,
         "\nSnp->undi.mcast_ip_to_mac()  %xh:%xh\n",
         Snp->Cdb->StatFlags,
         Snp->Cdb->StatCode)
        );
      return EFI_UNSUPPORTED;

    default:
      //
      // UNDI command failed.  Return EFI_DEVICE_ERROR
      // to caller.
      //
      DEBUG (
        (DEBUG_NET,
         "\nSnp->undi.mcast_ip_to_mac()  %xh:%xh\n",
         Snp->Cdb->StatFlags,
         Snp->Cdb->StatCode)
        );

      return EFI_DEVICE_ERROR;
  }

  CopyMem (MAC, &Db->MAC, sizeof (PXE_MAC_ADDR));
  return EFI_SUCCESS;
}

/**
  Converts a multicast IP address to a multicast HW MAC address.

  This function converts a multicast IP address to a multicast HW MAC address
  for all packet transactions. If the mapping is accepted, then EFI_SUCCESS will
  be returned.

  @param This A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param IPv6 Set to TRUE if the multicast IP address is IPv6 [RFC 2460].
              Set to FALSE if the multicast IP address is IPv4 [RFC 791].
  @param IP   The multicast IP address that is to be converted to a multicast
              HW MAC address.
  @param MAC  The multicast HW MAC address that is to be generated from IP.

  @retval EFI_SUCCESS           The multicast IP address was mapped to the
                                multicast HW MAC address.
  @retval EFI_NOT_STARTED       The Simple Network Protocol interface has not
                                been started by calling Start().
  @retval EFI_INVALID_PARAMETER IP is NULL.
  @retval EFI_INVALID_PARAMETER MAC is NULL.
  @retval EFI_INVALID_PARAMETER IP does not point to a valid IPv4 or IPv6
                                multicast address.
  @retval EFI_DEVICE_ERROR      The Simple Network Protocol interface has not
                                been initialized by calling Initialize().
  @retval EFI_UNSUPPORTED       IPv6 is TRUE and the implementation does not
                                support IPv6 multicast to MAC address conversion.

**/
EFI_STATUS
EFIAPI
SnpUndi32McastIpToMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      IPv6,
  IN EFI_IP_ADDRESS               *IP,
  OUT EFI_MAC_ADDRESS             *MAC
  )
{
  SNP_DRIVER  *Snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  //
  // Get pointer to SNP driver instance for *this.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((IP == NULL) || (MAC == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode.State) {
    case EfiSimpleNetworkInitialized:
      break;

    case EfiSimpleNetworkStopped:
      Status = EFI_NOT_STARTED;
      goto ON_EXIT;

    default:
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
  }

  Status = PxeIp2Mac (Snp, IPv6, IP, MAC);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
