/** @file
  EFI internal structures for the EFI UNDI driver.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UNDI_32_H_
#define _UNDI_32_H_

#include <Uefi.h>

#include <Guid/EventGroup.h>
#include <Protocol/PciIo.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/DevicePath.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>

#include <IndustryStandard/Pci.h>


#include "E100b.h"

extern EFI_DRIVER_BINDING_PROTOCOL  gUndiDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gUndiComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gUndiComponentName2;

#define MAX_NIC_INTERFACES 16

#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION_31 0x00010001
#define PXE_ROMID_MINORVER_31 0x10
#define PXE_STATFLAGS_DB_WRITE_TRUNCATED  0x2000

//
// UNDI_CALL_TABLE.state can have the following values
//
#define DONT_CHECK -1
#define ANY_STATE -1
#define MUST_BE_STARTED 1
#define MUST_BE_INITIALIZED 2

#define UNDI_DEV_SIGNATURE   SIGNATURE_32('u','n','d','i')
#define UNDI_DEV_FROM_THIS(a) CR(a, UNDI32_DEV, NIIProtocol_31, UNDI_DEV_SIGNATURE)
#define UNDI_DEV_FROM_NIC(a) CR(a, UNDI32_DEV, NicInfo, UNDI_DEV_SIGNATURE)

typedef struct {
  UINTN                                     Signature;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL NIIProtocol_31;
  EFI_HANDLE                                DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL                  *Undi32BaseDevPath;
  EFI_DEVICE_PATH_PROTOCOL                  *Undi32DevPath;
  NIC_DATA_INSTANCE                         NicInfo;
} UNDI32_DEV;

typedef struct {
  UINT16 cpbsize;
  UINT16 dbsize;
  UINT16 opflags;
  UINT16 state;
  VOID (*api_ptr)();
} UNDI_CALL_TABLE;

typedef VOID (*ptr)(VOID);
typedef VOID (*bsptr_30)(UINTN);
typedef VOID (*virtphys_30)(UINT64, UINT64);
typedef VOID (*block_30)(UINT32);
typedef VOID (*mem_io_30)(UINT8, UINT8, UINT64, UINT64);

typedef VOID (*bsptr)(UINT64, UINTN);
typedef VOID (*virtphys)(UINT64, UINT64, UINT64);
typedef VOID (*block)(UINT64, UINT32);
typedef VOID (*mem_io)(UINT64, UINT8, UINT8, UINT64, UINT64);

typedef VOID (*map_mem)(UINT64, UINT64, UINT32, UINT32, UINT64);
typedef VOID (*unmap_mem)(UINT64, UINT64, UINT32, UINT32, UINT64);
typedef VOID (*sync_mem)(UINT64, UINT64, UINT32, UINT32, UINT64);

extern UNDI_CALL_TABLE  api_table[];
extern PXE_SW_UNDI      *pxe_31;  // !pxe structure for 3.1 drivers
extern UNDI32_DEV       *UNDI32DeviceList[MAX_NIC_INTERFACES];

//
// functions defined in e100b.c
//
UINT8 InByte (NIC_DATA_INSTANCE *AdapterInfo, UINT32 Port);
UINT16 InWord (NIC_DATA_INSTANCE *AdapterInfo, UINT32 Port);
UINT32 InLong (NIC_DATA_INSTANCE *AdapterInfo, UINT32 Port);
VOID  OutByte (NIC_DATA_INSTANCE *AdapterInfo, UINT8 Data, UINT32 Port);
VOID  OutWord (NIC_DATA_INSTANCE *AdapterInfo, UINT16 Data, UINT32 Port);
VOID  OutLong (NIC_DATA_INSTANCE *AdapterInfo, UINT32 Data, UINT32 Port);

UINTN E100bInit (NIC_DATA_INSTANCE *AdapterInfo);
UINTN E100bReset (NIC_DATA_INSTANCE *AdapterInfo, INT32 OpFlags);
UINTN E100bShutdown (NIC_DATA_INSTANCE *AdapterInfo);
UINTN E100bTransmit (NIC_DATA_INSTANCE *AdapterInfo, UINT64 cpb, UINT16 opflags);
UINTN E100bReceive (NIC_DATA_INSTANCE *AdapterInfo, UINT64 cpb, UINT64 db);
UINTN E100bSetfilter (NIC_DATA_INSTANCE *AdapterInfo, UINT16 New_filter,
                      UINT64 cpb, UINT32 cpbsize);
UINTN E100bStatistics(NIC_DATA_INSTANCE *AdapterInfo, UINT64 db, UINT16 dbsize);
UINT8 E100bSetupIAAddr (NIC_DATA_INSTANCE *AdapterInfo);
UINT8 E100bSetInterruptState (NIC_DATA_INSTANCE *AdapterInfo);

UINT8 E100bGetEepromAddrLen (NIC_DATA_INSTANCE *AdapterInfo);
UINT16 E100bReadEeprom (NIC_DATA_INSTANCE *AdapterInfo, INT32 Location, UINT8 address_len);
INT16 E100bReadEepromAndStationAddress (NIC_DATA_INSTANCE *AdapterInfo);

UINT16 next(UINT16);
UINT8 SetupCBlink (NIC_DATA_INSTANCE *AdapterInfo);
VOID SetFreeCB (NIC_DATA_INSTANCE *AdapterInfo,TxCB *);
TxCB *GetFreeCB (NIC_DATA_INSTANCE *AdapterInfo);
UINT16 CheckCBList (NIC_DATA_INSTANCE *AdapterInfo);

UINT8 SelectiveReset (NIC_DATA_INSTANCE *AdapterInfo);
UINT16 InitializeChip (NIC_DATA_INSTANCE *AdapterInfo);
UINT8 SetupReceiveQueues (NIC_DATA_INSTANCE *AdapterInfo);
VOID  Recycle_RFD (NIC_DATA_INSTANCE *AdapterInfo, UINT16);
VOID XmitWaitForCompletion (NIC_DATA_INSTANCE *AdapterInfo);
INT8 CommandWaitForCompletion (TxCB *cmd_ptr, NIC_DATA_INSTANCE *AdapterInfo);

BOOLEAN PhyDetect (NIC_DATA_INSTANCE *AdapterInfo);
VOID PhyReset (NIC_DATA_INSTANCE *AdapterInfo);
VOID
MdiWrite (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT8 RegAddress,
  IN UINT8 PhyAddress,
  IN UINT16 DataValue
  );

VOID
MdiRead(
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT8 RegAddress,
  IN UINT8 PhyAddress,
  IN OUT UINT16 *DataValue
  );

BOOLEAN SetupPhy (NIC_DATA_INSTANCE *AdapterInfo);
VOID FindPhySpeedAndDpx (NIC_DATA_INSTANCE *AdapterInfo, UINT32 PhyId);



//
// functions defined in init.c
//
EFI_STATUS
InstallConfigTable (
  IN VOID
  );

EFI_STATUS
EFIAPI
InitializeUNDIDriver (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

VOID
UNDI_notify_virtual (
  EFI_EVENT event,
  VOID      *context
  );

VOID
EFIAPI
UndiNotifyExitBs (
  EFI_EVENT Event,
  VOID      *Context
  );

EFI_STATUS
EFIAPI
UndiDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UndiDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UndiDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

EFI_STATUS
AppendMac2DevPath (
  IN OUT  EFI_DEVICE_PATH_PROTOCOL **DevPtr,
  IN      EFI_DEVICE_PATH_PROTOCOL *BaseDevPtr,
  IN      NIC_DATA_INSTANCE        *AdapterInfo
  );

VOID
TmpDelay (
  IN UINT64 UnqId,
  IN UINTN MicroSeconds
  );

VOID
TmpMemIo (
  IN UINT64 UnqId,
  IN UINT8 ReadWrite,
  IN UINT8 Len,
  IN UINT64 Port,
  IN UINT64 BufAddr
  );

//
// functions defined in decode.c
//
VOID
UNDI_GetState (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Start (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Stop (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_GetInitInfo (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_GetConfigInfo (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Initialize (
  IN  PXE_CDB       *CdbPtr,
  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Reset (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Shutdown (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Interrupt (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_RecFilter (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_StnAddr (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Statistics (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_ip2mac (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_NVData (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Status (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_FillHeader (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Transmit (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID
UNDI_Receive (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  );

VOID UNDI_APIEntry_new(UINT64);
VOID UNDI_APIEntry_Common(UINT64);

PXE_IPV4 convert_mcip(PXE_MAC_ADDR *);
INT32 validate_mcip (PXE_MAC_ADDR *MCastAddr);

VOID PxeStructInit (PXE_SW_UNDI *PxePtr);
VOID PxeUpdate (NIC_DATA_INSTANCE *NicPtr, PXE_SW_UNDI *PxePtr);

#endif
