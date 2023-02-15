/** @file
  This file contains code for UNDI command based on UEFI specification.

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DriverBinding.h"

// API table, defined in UEFI specification
API_FUNC  gUndiApiTable[] =3D {
  UndiGetState,
  UndiStart,
  UndiStop,
  UndiGetInitInfo,
  UndiGetConfigInfo,
  UndiInitialize,
  UndiReset,
  UndiShutdown,
  UndiInterruptEnable,
  UndiReceiveFilter,
  UndiStationAddress,
  UndiStatistics,
  UndiMcastIp2Mac,
  UndiNvData,
  UndiGetStatus,
  UndiFillHeader,
  UndiTransmit,
  UndiReceive
};

/**
   Callback function for enable Rate Limiter

   @param[in] Event           Event whose notification function is being invoked
   @param[in] Context         Pointer to the notification function's context

**/
VOID
EFIAPI
UndiRateLimiterCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  NIC_DATA  *Nic =3D Context;

  if (Nic->RateLimitingCreditCount < Nic->RateLimitingCredit) {
    Nic->RateLimitingCreditCount++;
  }
}

/**
  This command is used to determine the operational state of the UNDI.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiGetState (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_GET_STATE) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) ||
      (Cdb->CPBaddr !=3D PXE_CPBADDR_NOT_USED) ||
      (Cdb->DBsize !=3D PXE_DBSIZE_NOT_USED) ||
      (Cdb->DBaddr !=3D PXE_DBADDR_NOT_USED) ||
      (Cdb->OpFlags !=3D PXE_OPFLAGS_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  Cdb->StatFlags =3D Cdb->StatFlags | Nic->State;

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiGetState !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiGetState (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  This command is used to change the UNDI operational state from stopped to started.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiStart (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  PXE_CPB_START_31  *Cpb;
  EFI_STATUS        Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_START) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D sizeof (PXE_CPB_START_31)) ||
      (Cdb->DBsize !=3D PXE_DBSIZE_NOT_USED) ||
      (Cdb->DBaddr !=3D PXE_DBADDR_NOT_USED) ||
      (Cdb->OpFlags !=3D PXE_OPFLAGS_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_STOPPED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_ALREADY_STARTED;
    return;
  }

  Cpb =3D (PXE_CPB_START_31 *)(UINTN)Cdb->CPBaddr;

  Nic->PxeStart.Delay     =3D Cpb->Delay;
  Nic->PxeStart.Virt2Phys =3D Cpb->Virt2Phys;
  Nic->PxeStart.Block     =3D Cpb->Block;
  Nic->PxeStart.Map_Mem   =3D 0;
  Nic->PxeStart.UnMap_Mem =3D 0;
  Nic->PxeStart.Sync_Mem  =3D Cpb->Sync_Mem;
  Nic->PxeStart.Unique_ID =3D Cpb->Unique_ID;

  if (Nic->RateLimitingEnable =3D=3D TRUE) {
    Status =3D gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    UndiRateLimiterCallback,
                    Nic,
                    &Nic->RateLimiter
                    );
    if (EFI_ERROR (Status)) {
      goto ErrorCreateEvent;
    }

    Status =3D gBS->SetTimer (
                    Nic->RateLimiter,
                    TimerPeriodic,
                    PcdGet32 (RateLimitingFactor) * 10000
                    );
    if (EFI_ERROR (Status)) {
      goto ErrorSetTimer;
    }
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiStart !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiStart (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      goto ErrorUndiStart;
    }
  }

  Nic->State     =3D PXE_STATFLAGS_GET_STATE_STARTED;
  Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
  Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  return;

ErrorUndiStart:
  gBS->SetTimer (&Nic->RateLimiter, TimerCancel, 0);
ErrorSetTimer:
  gBS->CloseEvent (&Nic->RateLimiter);
ErrorCreateEvent:
  Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
  Cdb->StatCode  =3D PXE_STATCODE_DEVICE_FAILURE;
}

/**
  This command is used to change the UNDI operational state from started to stopped.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiStop (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_STOP) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) ||
      (Cdb->CPBaddr !=3D PXE_CPBADDR_NOT_USED) ||
      (Cdb->DBsize !=3D PXE_DBSIZE_NOT_USED) ||
      (Cdb->DBaddr !=3D PXE_DBADDR_NOT_USED) ||
      (Cdb->OpFlags !=3D PXE_OPFLAGS_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State =3D=3D PXE_STATFLAGS_GET_STATE_STOPPED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_NOT_STARTED;
    return;
  }

  if (Nic->State =3D=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_NOT_SHUTDOWN;
    return;
  }

  Nic->PxeStart.Delay     =3D 0;
  Nic->PxeStart.Virt2Phys =3D 0;
  Nic->PxeStart.Block     =3D 0;
  Nic->PxeStart.Map_Mem   =3D 0;
  Nic->PxeStart.UnMap_Mem =3D 0;
  Nic->PxeStart.Sync_Mem  =3D 0;
  Nic->State              =3D PXE_STATFLAGS_GET_STATE_STOPPED;

  if (Nic->RateLimitingEnable =3D=3D TRUE) {
    gBS->SetTimer (&Nic->RateLimiter, TimerCancel, 0);
    gBS->CloseEvent (&Nic->RateLimiter);
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiStop !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiStop (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  This command is used to retrieve initialization information that is
  needed by drivers and applications to initialized UNDI.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiGetInitInfo (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  PXE_DB_GET_INIT_INFO  *Db;
  EFI_STATUS            Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_GET_INIT_INFO) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) ||
      (Cdb->CPBaddr !=3D PXE_CPBADDR_NOT_USED) ||
      (Cdb->DBsize !=3D sizeof (PXE_DB_GET_INIT_INFO)) ||
      (Cdb->OpFlags !=3D PXE_OPFLAGS_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State =3D=3D PXE_STATFLAGS_GET_STATE_STOPPED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_NOT_STARTED;
    return;
  }

  Db =3D (PXE_DB_GET_INIT_INFO *)(UINTN)Cdb->DBaddr;

  Db->MemoryRequired         =3D MEMORY_REQUIRE;
  Db->FrameDataLen           =3D PXE_MAX_TXRX_UNIT_ETHER;
  Db->LinkSpeeds[0]          =3D 10;
  Db->LinkSpeeds[1]          =3D 100;
  Db->LinkSpeeds[2]          =3D 1000;
  Db->LinkSpeeds[3]          =3D 0;
  Db->MediaHeaderLen         =3D PXE_MAC_HEADER_LEN_ETHER;
  Db->HWaddrLen              =3D PXE_HWADDR_LEN_ETHER;
  Db->MCastFilterCnt         =3D MAX_MCAST_ADDRESS_CNT;
  Db->TxBufCnt               =3D Nic->PxeInit.TxBufCnt;
  Db->TxBufSize              =3D Nic->PxeInit.TxBufSize;
  Db->RxBufCnt               =3D Nic->PxeInit.RxBufCnt;
  Db->RxBufSize              =3D Nic->PxeInit.RxBufSize;
  Db->IFtype                 =3D PXE_IFTYPE_ETHERNET;
  Db->SupportedDuplexModes   =3D PXE_DUPLEX_DEFAULT;
  Db->SupportedLoopBackModes =3D LOOPBACK_NORMAL;

  Cdb->StatFlags |=3D (PXE_STATFLAGS_CABLE_DETECT_SUPPORTED |
                     PXE_STATFLAGS_GET_STATUS_NO_MEDIA_SUPPORTED);

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiGetInitInfo !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiGetInitInfo (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  This command is used to retrieve configuration information about
  the NIC being controlled by the UNDI.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiGetConfigInfo (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  PXE_DB_GET_CONFIG_INFO  *Db;
  EFI_STATUS              Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_GET_CONFIG_INFO) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) ||
      (Cdb->CPBaddr !=3D PXE_CPBADDR_NOT_USED) ||
      (Cdb->DBsize !=3D sizeof (PXE_DB_GET_CONFIG_INFO)) ||
      (Cdb->OpFlags !=3D PXE_OPFLAGS_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State =3D=3D PXE_STATFLAGS_GET_STATE_STOPPED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_NOT_STARTED;
    return;
  }

  Db =3D (PXE_DB_GET_CONFIG_INFO *)(UINTN)Cdb->DBaddr;

  Db->pci.BusType =3D PXE_BUSTYPE_USB;

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiGetConfigInfo !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiGetConfigInfo (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  This command resets the network adapter and initializes UNDI using
  the parameters supplied in the CPB.

  @param[in]      Cdb  A pointer to the command descriptor block.
  @param[in, out] Nic  A pointer to the Network interface controller data.

**/
VOID
UndiInitialize (
  IN      PXE_CDB   *Cdb,
  IN OUT  NIC_DATA  *Nic
  )
{
  PXE_CPB_INITIALIZE  *Cpb;
  PXE_DB_INITIALIZE   *Db;
  EFI_STATUS          Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_INITIALIZE) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D sizeof (PXE_CPB_INITIALIZE)))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
  }

  if (Nic->State =3D=3D PXE_STATFLAGS_GET_STATE_STOPPED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_NOT_STARTED;
    return;
  }

  if ((Cdb->OpFlags !=3D PXE_OPFLAGS_INITIALIZE_DETECT_CABLE) &&
      (Cdb->OpFlags !=3D PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  }

  if (Nic->State =3D=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_ALREADY_INITIALIZED;
    return;
  }

  Cpb =3D (PXE_CPB_INITIALIZE *)(UINTN)Cdb->CPBaddr;
  Db  =3D (PXE_DB_INITIALIZE *)(UINTN)Cdb->DBaddr;

  Nic->PxeInit.LinkSpeed    =3D Cpb->LinkSpeed;
  Nic->PxeInit.DuplexMode   =3D Cpb->DuplexMode;
  Nic->PxeInit.LoopBackMode =3D Cpb->LoopBackMode;
  Nic->PxeInit.MemoryAddr   =3D Cpb->MemoryAddr;
  Nic->PxeInit.MemoryLength =3D Cpb->MemoryLength;
  Nic->PxeInit.TxBufCnt     =3D TX_BUFFER_COUNT;
  Nic->PxeInit.TxBufSize    =3D Nic->MaxSegmentSize;
  Nic->PxeInit.RxBufCnt     =3D RX_BUFFER_COUNT;
  Nic->PxeInit.RxBufSize    =3D Nic->MaxSegmentSize;

  Cdb->StatCode =3D Initialize (Cdb, Nic);

  Db->MemoryUsed =3D MEMORY_REQUIRE;
  Db->TxBufCnt   =3D Nic->PxeInit.TxBufCnt;
  Db->TxBufSize  =3D Nic->PxeInit.TxBufSize;
  Db->RxBufCnt   =3D Nic->PxeInit.RxBufCnt;
  Db->RxBufSize  =3D Nic->PxeInit.RxBufSize;

  Nic->RxFilter    =3D PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST;
  Nic->CanTransmit =3D FALSE;

  if (Cdb->OpFlags =3D=3D PXE_OPFLAGS_INITIALIZE_DETECT_CABLE) {
    if ((Nic->Request.Request =3D=3D USB_CDC_NETWORK_CONNECTION) && (Nic->Request.Value =3D=3D NETWORK_DISCONNECT)) {
      Nic->CableDetect =3D 0;
    } else if ((Nic->Request.Request =3D=3D USB_CDC_NETWORK_CONNECTION) && (Nic->Request.Value =3D=3D NETWORK_CONNECTED)) {
      Nic->CableDetect =3D 1;
    }

    if (Nic->CableDetect =3D=3D 0) {
      Cdb->StatFlags |=3D PXE_STATFLAGS_INITIALIZED_NO_MEDIA;
    }
  }

  if (Cdb->StatCode !=3D PXE_STATCODE_SUCCESS) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
  } else {
    Nic->State =3D PXE_STATFLAGS_GET_STATE_INITIALIZED;
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiInitialize !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiInitialize (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  Initialize Network interface controller data.

  @param[in]      Cdb     A pointer to the command descriptor block.
  @param[in, out] Nic     A pointer to the Network interface controller data.

  @retval Status  A value of Pxe statcode.

**/
UINT16
Initialize (
  IN      PXE_CDB   *Cdb,
  IN OUT  NIC_DATA  *Nic
  )
{
  UINTN       Status;
  UINT32      Index;
  EFI_STATUS  EfiStatus;

  Status =3D MapIt (
             Nic,
             Nic->PxeInit.MemoryAddr,
             Nic->PxeInit.MemoryLength,
             TO_AND_FROM_DEVICE,
             (UINT64)(UINTN)&Nic->MappedAddr
             );

  if (Status !=3D 0) {
    return (UINT16)Status;
  }

  for (Index =3D 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
    Nic->PermNodeAddress[Index] =3D Nic->MacAddr.Addr[Index];
  }

  for (Index =3D 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
    Nic->CurrentNodeAddress[Index] =3D Nic->PermNodeAddress[Index];
  }

  for (Index =3D 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
    Nic->BroadcastNodeAddress[Index] =3D 0xFF;
  }

  for (Index =3D PXE_HWADDR_LEN_ETHER; Index < PXE_MAC_LENGTH; Index++) {
    Nic->CurrentNodeAddress[Index]   =3D 0;
    Nic->PermNodeAddress[Index]      =3D 0;
    Nic->BroadcastNodeAddress[Index] =3D 0;
  }

  if (Nic->UsbEth->UsbEthInitialize !=3D NULL) {
    EfiStatus =3D Nic->UsbEth->UsbEthInitialize (Cdb, Nic);
    if (EFI_ERROR (EfiStatus)) {
      return PXE_STATFLAGS_COMMAND_FAILED;
    }
  }

  return (UINT16)Status;
}

/**
  This command resets the network adapter and reinitializes the UNDI
  with the same parameters provided in the Initialize command.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiReset (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_RESET) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) ||
      (Cdb->CPBaddr !=3D PXE_CPBADDR_NOT_USED) ||
      (Cdb->DBsize !=3D PXE_DBSIZE_NOT_USED) ||
      (Cdb->DBaddr !=3D PXE_DBADDR_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    return;
  }

  if ((Cdb->OpFlags !=3D PXE_OPFLAGS_NOT_USED) &&
      (Cdb->OpFlags !=3D PXE_OPFLAGS_RESET_DISABLE_INTERRUPTS) &&
      (Cdb->OpFlags !=3D PXE_OPFLAGS_RESET_DISABLE_FILTERS))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  }

  if ((Cdb->OpFlags & PXE_OPFLAGS_RESET_DISABLE_FILTERS) =3D=3D 0) {
    Nic->RxFilter =3D PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST;
  }

  if ((Cdb->OpFlags & PXE_OPFLAGS_RESET_DISABLE_INTERRUPTS) !=3D 0) {
    Nic->InterrupOpFlag =3D 0;
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiReset !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiReset (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  The Shutdown command resets the network adapter and leaves it in a
  safe state for another driver to initialize.

  @param[in]      Cdb  A pointer to the command descriptor block.
  @param[in, out] Nic  A pointer to the Network interface controller data.

**/
VOID
UndiShutdown (
  IN      PXE_CDB   *Cdb,
  IN OUT  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_SHUTDOWN) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) ||
      (Cdb->CPBaddr !=3D PXE_CPBADDR_NOT_USED) ||
      (Cdb->DBsize !=3D PXE_DBSIZE_NOT_USED) ||
      (Cdb->DBaddr !=3D PXE_DBADDR_NOT_USED) ||
      (Cdb->OpFlags !=3D PXE_OPFLAGS_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    return;
  }

  Nic->CanTransmit =3D FALSE;

  Nic->State =3D PXE_STATFLAGS_GET_STATE_STARTED;

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiShutdown !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiShutdown (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  The Interrupt Enables command can be used to read and/or change
  the current external interrupt enable settings.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiInterruptEnable (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  Cdb->StatCode =3D PXE_STATCODE_UNSUPPORTED;
  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiInterruptEnable !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiInterruptEnable (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    } else {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
      Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
    }
  }
}

/**
  This command is used to read and change receive filters and,
  if supported, read and change the multicast MAC address filter list.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiReceiveFilter (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  UINT16                  NewFilter;
  PXE_DB_RECEIVE_FILTERS  *Db;
  EFI_STATUS              Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_RECEIVE_FILTERS) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    return;
  }

  NewFilter =3D (UINT16)(Cdb->OpFlags & 0x1F);

  switch (Cdb->OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_OPMASK) {
    case PXE_OPFLAGS_RECEIVE_FILTER_READ:
      if (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) {
        Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
        Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
      }

      if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) =3D=3D 0) {
        if ((Cdb->DBsize !=3D 0)) {
          Db =3D (PXE_DB_RECEIVE_FILTERS *)(UINTN)Cdb->DBaddr;
          CopyMem (Db, &Nic->McastList, Nic->McastCount);
        }
      }

      break;

    case PXE_OPFLAGS_RECEIVE_FILTER_ENABLE:
      if (NewFilter =3D=3D 0) {
        Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
      }

      if (Cdb->CPBsize !=3D 0) {
        if (((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) =3D=3D 0) ||
            ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) !=3D 0) ||
            ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) !=3D 0) ||
            ((Cdb->CPBsize % sizeof (PXE_MAC_ADDR)) !=3D 0))
        {
          Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
        }
      }

      if ((Cdb->OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) !=3D 0) {
        if (((Cdb->OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) !=3D 0) ||
            ((Cdb->OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) !=3D 0))
        {
          Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
        }

        if ((Cdb->CPBsize =3D=3D 0)) {
          Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
        }
      }

      Cdb->StatCode =3D SetFilter (Nic, NewFilter, Cdb->CPBaddr, Cdb->CPBsize);
      break;

    case PXE_OPFLAGS_RECEIVE_FILTER_DISABLE:
      if (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) {
        Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
        Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
      }

      break;

    default:
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
      Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
  }

  Cdb->StatFlags =3D (PXE_STATFLAGS)(Cdb->StatFlags | Nic->RxFilter);

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiReceiveFilter !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiReceiveFilter (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  Set PXE receive filter.

  @param[in]  Nic         A pointer to the Network interface controller data.
  @param[in]  SetFilter   PXE receive filter
  @param[in]  CpbAddr     Command Parameter Block Address
  @param[in]  CpbSize     Command Parameter Block Size

**/
UINT16
SetFilter (
  IN  NIC_DATA  *Nic,
  IN  UINT16    SetFilter,
  IN  UINT64    CpbAddr,
  IN  UINT32    CpbSize
  )
{
  EFI_STATUS                   Status;
  UINT8                        *McastList;
  UINT8                        Count;
  UINT8                        Index1;
  UINT8                        Index2;
  PXE_CPB_RECEIVE_FILTERS      *Cpb;
  USB_ETHERNET_FUN_DESCRIPTOR  UsbEthFunDescriptor;

  Count =3D 0;
  Cpb   =3D (PXE_CPB_RECEIVE_FILTERS *)(UINTN)CpbAddr;

  // The Cpb could be NULL.(ref:PXE_CPBADDR_NOT_USED)
  Nic->RxFilter =3D (UINT8)SetFilter;

  if (((SetFilter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) !=3D 0) || (Cpb !=3D NULL)) {
    if (Cpb !=3D NULL) {
      Nic->McastCount =3D (UINT8)(CpbSize / PXE_MAC_LENGTH);
      CopyMem (&Nic->McastList, Cpb, Nic->McastCount);
    }

    Nic->UsbEth->UsbEthFunDescriptor (Nic->UsbEth, &UsbEthFunDescriptor);
    if ((UsbEthFunDescriptor.NumberMcFilters << 1) =3D=3D 0) {
      Nic->RxFilter |=3D PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;
      Nic->UsbEth->SetUsbEthPacketFilter (Nic->UsbEth, Nic->RxFilter);
    } else {
      Status =3D gBS->AllocatePool (EfiBootServicesData, Nic->McastCount * 6, (VOID **)&McastList);
      if (EFI_ERROR (Status)) {
        return PXE_STATCODE_INVALID_PARAMETER;
      }

      if (Cpb !=3D NULL) {
        for (Index1 =3D 0; Index1 < Nic->McastCount; Index1++) {
          for (Index2 =3D 0; Index2 < 6; Index2++) {
            McastList[Count++] =3D Cpb->MCastList[Index1][Index2];
          }
        }
      }

      Nic->RxFilter |=3D PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
      if (Cpb !=3D NULL) {
        Nic->UsbEth->SetUsbEthMcastFilter (Nic->UsbEth, Nic->McastCount, McastList);
      }

      Nic->UsbEth->SetUsbEthPacketFilter (Nic->UsbEth, Nic->RxFilter);
      FreePool (McastList);
    }
  }

  return PXE_STATCODE_SUCCESS;
}

/**
  This command is used to get current station and broadcast MAC addresses
  and, if supported, to change the current station MAC address.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiStationAddress (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  PXE_CPB_STATION_ADDRESS  *Cpb;
  PXE_DB_STATION_ADDRESS   *Db;
  UINT16                   Index;
  EFI_STATUS               Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_STATION_ADDRESS) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->DBsize !=3D sizeof (PXE_DB_STATION_ADDRESS)))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    return;
  }

  if (Cdb->OpFlags =3D=3D PXE_OPFLAGS_STATION_ADDRESS_RESET) {
    if (CompareMem (&Nic->CurrentNodeAddress[0], &Nic->PermNodeAddress[0], PXE_MAC_LENGTH) !=3D 0) {
      for (Index =3D 0; Index < PXE_MAC_LENGTH; Index++) {
        Nic->CurrentNodeAddress[Index] =3D Nic->PermNodeAddress[Index];
      }
    }
  }

  if (Cdb->CPBaddr !=3D 0) {
    Cpb =3D (PXE_CPB_STATION_ADDRESS *)(UINTN)Cdb->CPBaddr;
    for (Index =3D 0; Index < PXE_MAC_LENGTH; Index++) {
      Nic->CurrentNodeAddress[Index] =3D Cpb->StationAddr[Index];
    }
  }

  if (Cdb->DBaddr !=3D 0) {
    Db =3D (PXE_DB_STATION_ADDRESS *)(UINTN)Cdb->DBaddr;
    for (Index =3D 0; Index < PXE_MAC_LENGTH; Index++) {
      Db->StationAddr[Index]   =3D Nic->CurrentNodeAddress[Index];
      Db->BroadcastAddr[Index] =3D Nic->BroadcastNodeAddress[Index];
      Db->PermanentAddr[Index] =3D Nic->PermNodeAddress[Index];
    }
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiStationAddress !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiStationAddress (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  This command is used to read and clear the NIC traffic statistics.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiStatistics (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_STATISTICS) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) ||
      (Cdb->CPBaddr !=3D PXE_CPBADDR_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    return;
  }

  if ((Cdb->OpFlags !=3D PXE_OPFLAGS_STATISTICS_RESET) &&
      (Cdb->OpFlags !=3D PXE_OPFLAGS_STATISTICS_READ))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  }

  Cdb->StatCode =3D Statistics (Nic, Cdb->DBaddr, Cdb->DBsize);

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiStatistics !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiStatistics (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  Return data for DB data.

  @param[in]  Nic      A pointer to the Network interface controller data.
  @param[in]  DbAddr   Data Block Address.
  @param[in]  DbSize   Data Block Size.

**/
UINT16
Statistics (
  IN NIC_DATA  *Nic,
  IN UINT64    DbAddr,
  IN UINT16    DbSize
  )
{
  PXE_DB_STATISTICS  *DbStatistic;
  EFI_STATUS         Status;

  DbStatistic =3D (PXE_DB_STATISTICS *)(UINTN)DbAddr;

  if (DbSize =3D=3D 0) {
    return PXE_STATCODE_SUCCESS;
  }

  DbStatistic->Supported  =3D 0x802;
  DbStatistic->Data[0x01] =3D Nic->RxFrame;
  DbStatistic->Data[0x0B] =3D Nic->TxFrame;

  if (Nic->UsbEth->UsbEthStatistics !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthStatistics (Nic, DbAddr, DbSize);
    if (EFI_ERROR (Status)) {
      return PXE_STATFLAGS_COMMAND_FAILED;
    }
  }

  return PXE_STATCODE_SUCCESS;
}

/**
  Translate a multicast IPv4 or IPv6 address to a multicast MAC address.

  @param[in, out] Cdb  A pointer to the command descriptor block.
  @param[in]      Nic  A pointer to the Network interface controller data.

**/
VOID
UndiMcastIp2Mac (
  IN OUT  PXE_CDB   *Cdb,
  IN      NIC_DATA  *Nic
  )
{
  PXE_CPB_MCAST_IP_TO_MAC  *Cpb;
  PXE_DB_MCAST_IP_TO_MAC   *Db;
  UINT8                    *Tmp;
  EFI_STATUS               Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_MCAST_IP_TO_MAC) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D sizeof (PXE_CPB_MCAST_IP_TO_MAC)) ||
      (Cdb->DBsize !=3D sizeof (PXE_DB_MCAST_IP_TO_MAC)))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    return;
  }

  Cpb =3D (PXE_CPB_MCAST_IP_TO_MAC *)(UINTN)Cdb->CPBaddr;
  Db  =3D (PXE_DB_MCAST_IP_TO_MAC *)(UINTN)Cdb->DBaddr;

  if ((Cdb->OpFlags & PXE_OPFLAGS_MCAST_IPV6_TO_MAC) !=3D 0) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_UNSUPPORTED;
    return;
  }

  Tmp =3D (UINT8 *)(&Cpb->IP.IPv4);

  if ((Tmp[0] & 0xF0) !=3D 0xE0) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CPB;
  }

  Db->MAC[0] =3D 0x01;
  Db->MAC[1] =3D 0x00;
  Db->MAC[2] =3D 0x5E;
  Db->MAC[3] =3D Tmp[1] & 0x7F;
  Db->MAC[4] =3D Tmp[2];
  Db->MAC[5] =3D Tmp[3];

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiMcastIp2Mac !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiMcastIp2Mac (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  This command is used to read and write (if supported by NIC H/W)
  nonvolatile storage on the NIC.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiNvData (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  Cdb->StatCode =3D PXE_STATCODE_UNSUPPORTED;
  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiNvData !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiNvData (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    } else {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
      Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
    }
  }
}

/**
  This command returns the current interrupt status and/or the
  transmitted buffer addresses and the current media status.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiGetStatus (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  PXE_DB_GET_STATUS  *Db;
  PXE_DB_GET_STATUS  TmpGetStatus;
  UINT16             NumEntries;
  UINTN              Index;
  EFI_STATUS         Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_GET_STATUS) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D PXE_CPBSIZE_NOT_USED) ||
      (Cdb->CPBaddr !=3D PXE_CPBADDR_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    return;
  }

  TmpGetStatus.RxFrameLen =3D 0;
  TmpGetStatus.reserved   =3D 0;
  Db                      =3D (PXE_DB_GET_STATUS *)(UINTN)Cdb->DBaddr;

  if ((Cdb->DBsize > 0) && (Cdb->DBsize < sizeof (UINT32) * 2)) {
    CopyMem (Db, &TmpGetStatus, Cdb->DBsize);
  } else {
    CopyMem (Db, &TmpGetStatus, sizeof (UINT32) * 2);
  }

  if ((Cdb->OpFlags & PXE_OPFLAGS_GET_TRANSMITTED_BUFFERS) !=3D 0) {
    if (Cdb->DBsize =3D=3D 0) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
      Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
      return;
    }

    NumEntries  =3D Cdb->DBsize - sizeof (UINT64);
    Cdb->DBsize =3D sizeof (UINT32) * 2;

    for (Index =3D 0; NumEntries >=3D sizeof (UINT64); Index++, NumEntries -=3D sizeof (UINT64)) {
      if (Nic->TxBufferCount > 0) {
        Nic->TxBufferCount--;
        Db->TxBuffer[Index] =3D Nic->MediaHeader[Nic->TxBufferCount];
      }
    }
  }

  if ((Cdb->OpFlags & PXE_OPFLAGS_GET_INTERRUPT_STATUS) !=3D 0) {
    if (Nic->ReceiveStatus !=3D 0) {
      Cdb->StatFlags |=3D PXE_STATFLAGS_GET_STATUS_RECEIVE;
    }
  }

  if ((Nic->Request.Request =3D=3D USB_CDC_NETWORK_CONNECTION) && (Nic->Request.Value =3D=3D NETWORK_DISCONNECT)) {
    Nic->CableDetect =3D 0;
  } else if ((Nic->Request.Request =3D=3D USB_CDC_NETWORK_CONNECTION) && (Nic->Request.Value =3D=3D NETWORK_CONNECTED)) {
    Nic->CableDetect =3D 1;
  }

  if ((Cdb->OpFlags & PXE_OPFLAGS_GET_MEDIA_STATUS) !=3D 0) {
    if (Nic->CableDetect =3D=3D 0) {
      Cdb->StatFlags |=3D PXE_STATFLAGS_GET_STATUS_NO_MEDIA;
    }
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiGetStatus !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiGetStatus (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  This command is used to fill the media header(s) in transmit packet(s).

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiFillHeader (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  PXE_CPB_FILL_HEADER             *CpbFillHeader;
  PXE_CPB_FILL_HEADER_FRAGMENTED  *CpbFill;
  EthernetHeader                  *MacHeader;
  UINTN                           Index;
  EFI_STATUS                      Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_FILL_HEADER) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D sizeof (PXE_CPB_FILL_HEADER_FRAGMENTED)) ||
      (Cdb->DBsize !=3D PXE_DBSIZE_NOT_USED) ||
      (Cdb->DBaddr !=3D PXE_DBADDR_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    return;
  }

  if (Cdb->CPBsize =3D=3D PXE_CPBSIZE_NOT_USED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  }

  if ((Cdb->OpFlags & PXE_OPFLAGS_FILL_HEADER_FRAGMENTED) !=3D 0) {
    CpbFill =3D (PXE_CPB_FILL_HEADER_FRAGMENTED *)(UINTN)Cdb->CPBaddr;

    if ((CpbFill->FragCnt =3D=3D 0) || (CpbFill->FragDesc[0].FragLen < PXE_MAC_HEADER_LEN_ETHER)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
      Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
      return;
    }

    MacHeader           =3D (EthernetHeader *)(UINTN)CpbFill->FragDesc[0].FragAddr;
    MacHeader->Protocol =3D CpbFill->Protocol;

    for (Index =3D 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
      MacHeader->DestAddr[Index] =3D CpbFill->DestAddr[Index];
      MacHeader->SrcAddr[Index]  =3D CpbFill->SrcAddr[Index];
    }
  } else {
    CpbFillHeader =3D (PXE_CPB_FILL_HEADER *)(UINTN)Cdb->CPBaddr;

    MacHeader           =3D (EthernetHeader *)(UINTN)CpbFillHeader->MediaHeader;
    MacHeader->Protocol =3D CpbFillHeader->Protocol;

    for (Index =3D 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
      MacHeader->DestAddr[Index] =3D CpbFillHeader->DestAddr[Index];
      MacHeader->SrcAddr[Index]  =3D CpbFillHeader->SrcAddr[Index];
    }
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiFillHeader !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiFillHeader (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }
  }
}

/**
  The Transmit command is used to place a packet into the transmit queue.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiTransmit (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_TRANSMIT) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D sizeof (PXE_CPB_TRANSMIT)) ||
      (Cdb->DBsize !=3D PXE_DBSIZE_NOT_USED) ||
      (Cdb->DBaddr !=3D PXE_DBADDR_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    return;
  }

  if (Cdb->CPBsize =3D=3D PXE_CPBSIZE_NOT_USED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiTransmit !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiTransmit (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }

    return;
  }

  Cdb->StatCode =3D Transmit (Cdb, Nic, Cdb->CPBaddr, Cdb->OpFlags);

  if (Cdb->StatCode !=3D PXE_STATCODE_SUCCESS) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
  }
}

/**
  Use USB Ethernet Protocol Bulk out command to transmit data.

  @param[in]      Cdb      A pointer to the command descriptor block.
  @param[in, out] Nic      A pointer to the Network interface controller data.
  @param[in]      CpbAddr  Command Parameter Block Address.
  @param[in]      OpFlags  Operation Flags.

**/
UINT16
Transmit (
  IN      PXE_CDB   *Cdb,
  IN OUT  NIC_DATA  *Nic,
  IN      UINT64    CpbAddr,
  IN      UINT16    OpFlags
  )
{
  EFI_STATUS        Status;
  PXE_CPB_TRANSMIT  *Cpb;
  UINT64            BulkOutData;
  UINTN             DataLength;
  UINTN             TransmitLength;
  UINTN             Map;
  UINT32            Counter;
  UINT16            StatCode;

  BulkOutData =3D 0;
  Counter     =3D 0;
  Cpb         =3D (PXE_CPB_TRANSMIT *)(UINTN)CpbAddr;

  if (Nic->CanTransmit) {
    return PXE_STATCODE_BUSY;
  }

  Nic->CanTransmit =3D TRUE;

  if ((OpFlags & PXE_OPFLAGS_TRANSMIT_FRAGMENTED) !=3D 0) {
    return PXE_STATCODE_INVALID_PARAMETER;
  }

  Map =3D MapIt (
          Nic,
          Cpb->FrameAddr,
          Cpb->DataLen + (UINT32)Cpb->MediaheaderLen,
          TO_DEVICE,
          (UINT64)(UINTN)&BulkOutData
          );

  if (Map !=3D 0) {
    Nic->CanTransmit =3D FALSE;
    return PXE_STATCODE_INVALID_PARAMETER;
  }

  if (Nic->TxBufferCount < MAX_XMIT_BUFFERS) {
    Nic->MediaHeader[Nic->TxBufferCount] =3D Cpb->FrameAddr;
    Nic->TxBufferCount++;
  }

  DataLength =3D (UINTN)(Cpb->DataLen + (UINT32)Cpb->MediaheaderLen);

  while (1) {
    if (Counter >=3D 3) {
      StatCode =3D PXE_STATCODE_BUSY;
      break;
    }

    TransmitLength =3D DataLength;

    Status =3D Nic->UsbEth->UsbEthTransmit (Cdb, Nic->UsbEth, (VOID *)(UINTN)BulkOutData, &TransmitLength);
    if (EFI_ERROR (Status)) {
      StatCode =3D  PXE_STATFLAGS_COMMAND_FAILED;
    }

    if (Status =3D=3D EFI_INVALID_PARAMETER) {
      StatCode =3D PXE_STATCODE_INVALID_PARAMETER;
      break;
    }

    if (Status =3D=3D EFI_DEVICE_ERROR) {
      StatCode =3D PXE_STATCODE_DEVICE_FAILURE;
      break;
    }

    if (!EFI_ERROR (Status)) {
      Nic->TxFrame++;
      StatCode =3D PXE_STATCODE_SUCCESS;
      break;
    }

    Counter++;
  }

  UnMapIt (
    Nic,
    Cpb->FrameAddr,
    Cpb->DataLen + (UINT32)Cpb->MediaheaderLen,
    TO_DEVICE,
    BulkOutData
    );

  Nic->CanTransmit =3D FALSE;

  return StatCode;
}

/**
  When the network adapter has received a frame, this command is used
  to copy the frame into driver/application storage.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

**/
VOID
UndiReceive (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  if ((Cdb->OpCode !=3D PXE_OPCODE_RECEIVE) ||
      (Cdb->StatCode !=3D PXE_STATCODE_INITIALIZE) ||
      (Cdb->StatFlags !=3D PXE_STATFLAGS_INITIALIZE) ||
      (Cdb->IFnum >=3D (gPxe->IFcnt | gPxe->IFcntExt << 8)) ||
      (Cdb->CPBsize !=3D sizeof (PXE_CPB_RECEIVE)) ||
      (Cdb->DBsize !=3D sizeof (PXE_DB_RECEIVE)) ||
      (Cdb->OpFlags !=3D PXE_OPFLAGS_NOT_USED))
  {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_INVALID_CDB;
    return;
  } else {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_COMPLETE;
    Cdb->StatCode  =3D PXE_STATCODE_SUCCESS;
  }

  if (Nic->State !=3D PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    Cdb->StatCode  =3D PXE_STATCODE_NOT_INITIALIZED;
    return;
  }

  if (Nic->UsbEth->UsbEthUndi.UsbEthUndiReceive !=3D NULL) {
    Status =3D Nic->UsbEth->UsbEthUndi.UsbEthUndiReceive (Cdb, Nic);
    if (EFI_ERROR (Status)) {
      Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
    }

    return;
  }

  Cdb->StatCode =3D Receive (Cdb, Nic, Cdb->CPBaddr, Cdb->DBaddr);

  if (Cdb->StatCode !=3D PXE_STATCODE_SUCCESS) {
    Cdb->StatFlags =3D PXE_STATFLAGS_COMMAND_FAILED;
  }
}

/**
  Use USB Ethernet Protocol Bulk in command to receive data.

  @param[in]      Cdb      A pointer to the command descriptor block.
  @param[in, out] Nic      A pointer to the Network interface controller data.
  @param[in]      CpbAddr  Command Parameter Block Address.
  @param[in, out] DbAddr   Data Block Address.

**/
UINT16
Receive (
  IN PXE_CDB       *Cdb,
  IN OUT NIC_DATA  *Nic,
  IN UINT64        CpbAddr,
  IN OUT UINT64    DbAddr
  )
{
  EFI_STATUS       Status;
  UINTN            Index;
  PXE_FRAME_TYPE   FrameType;
  PXE_CPB_RECEIVE  *Cpb;
  PXE_DB_RECEIVE   *Db;
  NIC_DEVICE       *NicDevice;
  UINT8            *BulkInData;
  UINTN            DataLength;
  EthernetHeader   *Header;
  EFI_TPL          OriginalTpl;

  FrameType  =3D PXE_FRAME_TYPE_NONE;
  NicDevice  =3D UNDI_DEV_FROM_NIC (Nic);
  BulkInData =3D NicDevice->ReceiveBuffer;
  DataLength =3D (UINTN)Nic->MaxSegmentSize;
  Cpb        =3D (PXE_CPB_RECEIVE *)(UINTN)CpbAddr;
  Db         =3D (PXE_DB_RECEIVE *)(UINTN)DbAddr;

  if (!BulkInData) {
    return PXE_STATCODE_INVALID_PARAMETER;
  }

  if ((Nic->RateLimitingCreditCount =3D=3D 0) && (Nic->RateLimitingEnable =3D=3D TRUE)) {
    return PXE_STATCODE_NO_DATA;
  }

  Status =3D Nic->UsbEth->UsbEthReceive (Cdb, Nic->UsbEth, (VOID *)BulkInData, &DataLength);
  if (EFI_ERROR (Status)) {
    Nic->ReceiveStatus =3D 0;
    if (Nic->RateLimitingEnable =3D=3D TRUE) {
      OriginalTpl =3D gBS->RaiseTPL (TPL_NOTIFY);
      if (Nic->RateLimitingCreditCount !=3D 0) {
        Nic->RateLimitingCreditCount--;
      }

      gBS->RestoreTPL (OriginalTpl);
    }

    return PXE_STATCODE_NO_DATA;
  }

  Nic->RxFrame++;

  if (DataLength !=3D 0) {
    if (DataLength > Cpb->BufferLen) {
      DataLength =3D (UINTN)Cpb->BufferLen;
    }

    CopyMem ((UINT8 *)(UINTN)Cpb->BufferAddr, (UINT8 *)BulkInData, DataLength);

    Header =3D (EthernetHeader *)BulkInData;

    Db->FrameLen       =3D (UINT32)DataLength;
    Db->MediaHeaderLen =3D PXE_MAC_HEADER_LEN_ETHER;

    for (Index =3D 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
      if (Header->DestAddr[Index] !=3D Nic->CurrentNodeAddress[Index]) {
        break;
      }
    }

    if (Index >=3D PXE_HWADDR_LEN_ETHER) {
      FrameType =3D PXE_FRAME_TYPE_UNICAST;
    } else {
      for (Index =3D 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
        if (Header->DestAddr[Index] !=3D Nic->BroadcastNodeAddress[Index]) {
          break;
        }
      }

      if (Index >=3D PXE_HWADDR_LEN_ETHER) {
        FrameType =3D PXE_FRAME_TYPE_BROADCAST;
      } else {
        if ((Header->DestAddr[0] & 1) =3D=3D 1) {
          FrameType =3D PXE_FRAME_TYPE_FILTERED_MULTICAST;
        } else {
          FrameType =3D PXE_FRAME_TYPE_PROMISCUOUS;
        }
      }
    }

    Db->Type     =3D FrameType;
    Db->Protocol =3D Header->Protocol;

    for (Index =3D 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
      Db->SrcAddr[Index]  =3D Header->SrcAddr[Index];
      Db->DestAddr[Index] =3D Header->DestAddr[Index];
    }
  }

  if (FrameType =3D=3D PXE_FRAME_TYPE_NONE) {
    Nic->ReceiveStatus =3D 0;
  } else {
    Nic->ReceiveStatus =3D 1;
  }

  return PXE_STATCODE_SUCCESS;
}

/**
  Fill out PXE SW UNDI structure.

  @param[out]  PxeSw      A pointer to the PXE SW UNDI structure.

**/
VOID
PxeStructInit (
  OUT PXE_SW_UNDI  *PxeSw
  )
{
  PxeSw->Signature =3D PXE_ROMID_SIGNATURE;
  PxeSw->Len       =3D (UINT8)sizeof (PXE_SW_UNDI);
  PxeSw->Fudge     =3D 0;
  PxeSw->IFcnt     =3D 0;
  PxeSw->IFcntExt  =3D 0;
  PxeSw->Rev       =3D PXE_ROMID_REV;
  PxeSw->MajorVer  =3D PXE_ROMID_MAJORVER;
  PxeSw->MinorVer  =3D PXE_ROMID_MINORVER;
  PxeSw->reserved1 =3D 0;

  PxeSw->Implementation =3D PXE_ROMID_IMP_SW_VIRT_ADDR |
                          PXE_ROMID_IMP_FRAG_SUPPORTED |
                          PXE_ROMID_IMP_CMD_LINK_SUPPORTED |
                          PXE_ROMID_IMP_STATION_ADDR_SETTABLE |
                          PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED |
                          PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED |
                          PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED |
                          PXE_ROMID_IMP_FILTERED_MULTICAST_RX_SUPPORTED;

  PxeSw->EntryPoint   =3D (UINT64)(UINTN)UndiApiEntry;
  PxeSw->reserved2[0] =3D 0;
  PxeSw->reserved2[1] =3D 0;
  PxeSw->reserved2[2] =3D 0;
  PxeSw->BusCnt       =3D 1;
  PxeSw->BusType[0]   =3D PXE_BUSTYPE_USB;
  PxeSw->Fudge        =3D PxeSw->Fudge - CalculateSum8 ((VOID *)PxeSw, PxeSw->Len);
}

/**
  Update NIC number.

  @param[in]      Nic       A pointer to the Network interface controller data.
  @param[in, out] PxeSw     A pointer to the PXE SW UNDI structure.

**/
VOID
UpdateNicNum (
  IN      NIC_DATA     *Nic,
  IN OUT  PXE_SW_UNDI  *PxeSw
  )
{
  UINT16  NicNum;

  NicNum =3D (PxeSw->IFcnt | PxeSw->IFcntExt << 8);

  if (Nic =3D=3D NULL) {
    if (NicNum > 0) {
      NicNum--;
    }

    PxeSw->IFcnt    =3D (UINT8)(NicNum & 0xFF);          // Get lower byte
    PxeSw->IFcntExt =3D (UINT8)((NicNum & 0xFF00) >> 8); // Get upper byte
    PxeSw->Fudge    =3D (UINT8)(PxeSw->Fudge - CalculateSum8 ((VOID *)PxeSw, PxeSw->Len));
    return;
  }

  NicNum++;

  PxeSw->IFcnt    =3D (UINT8)(NicNum & 0xFF);          // Get lower byte
  PxeSw->IFcntExt =3D (UINT8)((NicNum & 0xFF00) >> 8); // Get upper byte
  PxeSw->Fudge    =3D (UINT8)(PxeSw->Fudge - CalculateSum8 ((VOID *)PxeSw, PxeSw->Len));
}

/**
  UNDI API table entry.

  @param[in]  Cdb  A pointer to the command descriptor block.

**/
EFI_STATUS
EFIAPI
UndiApiEntry (
  IN  UINT64  Cdb
  )
{
  PXE_CDB   *CdbPtr;
  NIC_DATA  *Nic;

  if (Cdb =3D=3D 0) {
    return EFI_INVALID_PARAMETER;
  }

  CdbPtr =3D (PXE_CDB *)(UINTN)Cdb;
  Nic    =3D &(gLanDeviceList[CdbPtr->IFnum]->NicInfo);
  gUndiApiTable[CdbPtr->OpCode](CdbPtr, Nic);
  return EFI_SUCCESS;
}

/**
  Map virtual memory address for DMA. This field can be set to
  zero if there is no mapping service.

  @param[in]  Nic           A pointer to the Network interface controller data.
  @param[in]  MemAddr       Virtual address to be mapped.
  @param[in]  Size          Size of memory to be mapped.
  @param[in]  Direction     Direction of data flow for this memory's usage:
                            cpu->device, device->cpu or both ways.
  @param[out] MappedAddr    Pointer to return the mapped device address.

**/
UINTN
MapIt (
  IN NIC_DATA  *Nic,
  IN UINT64    MemAddr,
  IN UINT32    Size,
  IN UINT32    Direction,
  OUT UINT64   MappedAddr
  )
{
  UINT64  *PhyAddr;

  PhyAddr =3D (UINT64 *)(UINTN)MappedAddr;

  if (Nic->PxeStart.Map_Mem =3D=3D 0) {
    *PhyAddr =3D MemAddr;
  } else {
    ((void (*)(UINT64, UINT64, UINT32, UINT32, UINT64))(UINTN) Nic->PxeStart.Map_Mem)(
  Nic->PxeStart.Unique_ID,
  MemAddr,
  Size,
  Direction,
  MappedAddr
  );
  }

  return PXE_STATCODE_SUCCESS;
}

/**
  Un-map previously mapped virtual memory address. This field can be set
  to zero only if the Map_Mem() service is also set to zero.

  @param[in]  Nic           A pointer to the Network interface controller data.
  @param[in]  MemAddr       Virtual address to be mapped.
  @param[in]  Size          Size of memory to be mapped.
  @param[in]  Direction     Direction of data flow for this memory's usage:
                            cpu->device, device->cpu or both ways.
  @param[in]  MappedAddr    Pointer to return the mapped device address.

**/
VOID
UnMapIt (
  IN NIC_DATA  *Nic,
  IN UINT64    MemAddr,
  IN UINT32    Size,
  IN UINT32    Direction,
  IN UINT64    MappedAddr
  )
{
  if (Nic->PxeStart.UnMap_Mem !=3D 0) {
    ((void (*)(UINT64, UINT64, UINT32, UINT32, UINT64))(UINTN) Nic->PxeStart.UnMap_Mem)(
  Nic->PxeStart.Unique_ID,
  MemAddr,
  Size,
  Direction,
  MappedAddr
  );
  }

  return;
}
