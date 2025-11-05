/** @file
  Header file for for USB network common driver

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DRIVER_BINDING_H_
#define DRIVER_BINDING_H_

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiUsbLib.h>
#include <Protocol/UsbIo.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/UsbEthernetProtocol.h>

#define NETWORK_COMMON_DRIVER_VERSION    1
#define NETWORK_COMMON_POLLING_INTERVAL  0x10
#define RX_BUFFER_COUNT                  32
#define TX_BUFFER_COUNT                  32
#define MEMORY_REQUIRE                   0

#define UNDI_DEV_SIGNATURE  SIGNATURE_32('u','n','d','i')
#define UNDI_DEV_FROM_THIS(a)  CR(a, NIC_DEVICE, NiiProtocol, UNDI_DEV_SIGNATURE)
#define UNDI_DEV_FROM_NIC(a)   CR(a, NIC_DEVICE, NicInfo, UNDI_DEV_SIGNATURE)

#pragma pack(1)
typedef struct {
  UINT8     DestAddr[PXE_HWADDR_LEN_ETHER];
  UINT8     SrcAddr[PXE_HWADDR_LEN_ETHER];
  UINT16    Protocol;
} ETHERNET_HEADER;
#pragma pack()

typedef struct {
  UINTN                                        Signature;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL    NiiProtocol;
  EFI_HANDLE                                   DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL                     *BaseDevPath;
  EFI_DEVICE_PATH_PROTOCOL                     *DevPath;
  NIC_DATA                                     NicInfo;
  VOID                                         *ReceiveBuffer;
} NIC_DEVICE;

typedef VOID (*API_FUNC)(
  PXE_CDB *,
  NIC_DATA *
  );

extern PXE_SW_UNDI                   *gPxe;
extern NIC_DEVICE                    *gLanDeviceList[MAX_LAN_INTERFACE];
extern EFI_COMPONENT_NAME2_PROTOCOL  gNetworkCommonComponentName2;

EFI_STATUS
EFIAPI
NetworkCommonSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
NetworkCommonDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
NetworkCommonDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

VOID
PxeStructInit (
  OUT PXE_SW_UNDI  *PxeSw
  );

VOID
UpdateNicNum (
  IN      NIC_DATA     *Nic,
  IN OUT  PXE_SW_UNDI  *PxeSw
  );

EFI_STATUS
EFIAPI
UndiApiEntry (
  IN  UINT64  Cdb
  );

UINTN
MapIt (
  IN NIC_DATA  *Nic,
  IN UINT64    MemAddr,
  IN UINT32    Size,
  IN UINT32    Direction,
  OUT UINT64   MappedAddr
  );

VOID
UnMapIt (
  IN NIC_DATA  *Nic,
  IN UINT64    MemAddr,
  IN UINT32    Size,
  IN UINT32    Direction,
  IN UINT64    MappedAddr
  );

VOID
UndiGetState (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiStart (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiStop (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiGetInitInfo (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiGetConfigInfo (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiInitialize (
  IN      PXE_CDB   *Cdb,
  IN OUT  NIC_DATA  *Nic
  );

VOID
UndiReset (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiShutdown (
  IN      PXE_CDB   *Cdb,
  IN OUT  NIC_DATA  *Nic
  );

VOID
UndiInterruptEnable (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiReceiveFilter (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiStationAddress (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiStatistics (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiMcastIp2Mac (
  IN OUT  PXE_CDB   *Cdb,
  IN      NIC_DATA  *Nic
  );

VOID
UndiNvData (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiGetStatus (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiFillHeader (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiTransmit (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

VOID
UndiReceive (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

UINT16
Initialize (
  IN      PXE_CDB   *Cdb,
  IN OUT  NIC_DATA  *Nic
  );

UINT16
Transmit (
  IN      PXE_CDB   *Cdb,
  IN OUT  NIC_DATA  *Nic,
  IN      UINT64    CpbAddr,
  IN      UINT16    OpFlags
  );

UINT16
Receive (
  IN PXE_CDB       *Cdb,
  IN OUT NIC_DATA  *Nic,
  IN UINT64        CpbAddr,
  IN OUT UINT64    DbAddr
  );

UINT16
SetFilter (
  IN  NIC_DATA  *Nic,
  IN  UINT16    SetFilter,
  IN  UINT64    CpbAddr,
  IN  UINT32    CpbSize
  );

UINT16
Statistics (
  IN NIC_DATA  *Nic,
  IN UINT64    DbAddr,
  IN UINT16    DbSize
  );

#endif
