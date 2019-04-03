/** @file
*  Header containing the structure specific to the Silicon Image I3132 Sata PCI card
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __SATASII3132_H
#define __SATASII3132_H

#include <PiDxe.h>

#include <Protocol/AtaPassThru.h>
#include <Protocol/PciIo.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/Pci.h>

#define SATA_SII3132_DEVICE_ID      0x3132
#define SATA_SII3132_VENDOR_ID      0x1095

#define SII3132_PORT_SIGNATURE_PMP      0x96690101
#define SII3132_PORT_SIGNATURE_ATAPI    0xEB140101
#define SII3132_PORT_SIGNATURE_ATA      0x00000101

/*
 * Silicon Image SiI3132 Registers
 */
#define SII3132_GLOBAL_CONTROL_REG              0x40
#define SII3132_GLOBAL_FLASHADDR_REG            0x70

#define SII3132_PORT_STATUS_REG                 0x1000
#define SII3132_PORT_CONTROLSET_REG             0x1000
#define SII3132_PORT_CONTROLCLEAR_REG           0x1004
#define SII3132_PORT_INTSTATUS_REG              0x1008
#define SII3132_PORT_ENABLEINT_REG              0x1010
#define SII3132_PORT_INTCLEAR_REG               0x1014
#define SII3132_PORT_32BITACTIVADDR_REG         0x101C
#define SII3132_PORT_CMDEXECFIFO_REG            0x1020
#define SII3132_PORT_CMDERROR_REG               0x1024
#define SII3132_PORT_ERRCOUNTDECODE             0x1040
#define SII3132_PORT_ERRCOUNTCRC                0x1044
#define SII3132_PORT_ERRCOUNTHANDSHAKE          0x1048
#define SII3132_PORT_SLOTSTATUS_REG             0x1800
#define SII3132_PORT_CMDACTIV_REG               0x1C00
#define SII3132_PORT_SSTATUS_REG                0x1F04

#define SII3132_PORT_CONTROL_RESET              (1 << 0)
#define SII3132_PORT_DEVICE_RESET               (1 << 1)
#define SII3132_PORT_CONTROL_INT                (1 << 2)
#define SII3132_PORT_CONTROL_32BITACTIVATION    (1 << 10)

#define SII3132_PORT_STATUS_PORTREADY           0x80000000

#define SII3132_PORT_INT_CMDCOMPL               (1 << 0)
#define SII3132_PORT_INT_CMDERR                 (1 << 1)
#define SII3132_PORT_INT_PORTRDY                (1 << 2)

#define SATA_SII3132_MAXPORT    2

#define PRB_CTRL_ATA            0x0
#define PRB_CTRL_PROT_OVERRIDE  0x1
#define PRB_CTRL_RESTRANSMIT    0x2
#define PRB_CTRL_EXT_CMD        0x4
#define PRB_CTRL_RCV            0x8
#define PRB_CTRL_PKT_READ       0x10
#define PRB_CTRL_PKT_WRITE      0x20
#define PRB_CTRL_INT_MASK       0x40
#define PRB_CTRL_SRST           0x80

#define PRB_PROT_PACKET         0x01
#define PRB_PROT_LEGACY_QUEUE   0x02
#define PRB_PROT_NATIVE_QUEUE   0x04
#define PRB_PROT_READ           0x08
#define PRB_PROT_WRITE          0x10
#define PRB_PROT_TRANSPARENT    0x20

#define SGE_XCF     (1 << 28)
#define SGE_DRD     (1 << 29)
#define SGE_LNK     (1 << 30)
#define SGE_TRM     0x80000000

typedef struct _SATA_SI3132_SGE {
    UINT32      DataAddressLow;
    UINT32      DataAddressHigh;
    UINT32      DataCount;
    UINT32      Attributes;
} SATA_SI3132_SGE;

typedef struct _SATA_SI3132_FIS {
    UINT8               FisType;
    UINT8               Control;
    UINT8               Command;
    UINT8               Features;
    UINT8               Fis[5 * 4];
} SATA_SI3132_FIS;

typedef struct _SATA_SI3132_PRB {
    UINT16              Control;
    UINT16              ProtocolOverride;
    UINT32              RecTransCount;
    SATA_SI3132_FIS     Fis;
    SATA_SI3132_SGE     Sge[2];
} SATA_SI3132_PRB;

typedef struct _SATA_SI3132_DEVICE {
    LIST_ENTRY                  Link; // This attribute must be the first entry of this structure (to avoid pointer computation)
    UINTN                       Index;
    struct _SATA_SI3132_PORT    *Port;  //Parent Port
    UINT32                      BlockSize;
} SATA_SI3132_DEVICE;

typedef struct _SATA_SI3132_PORT {
    UINTN                           Index;
    UINTN                           RegBase;
    struct _SATA_SI3132_INSTANCE    *Instance;

    //TODO: Support Port multiplier
    LIST_ENTRY                      Devices;

    SATA_SI3132_PRB*                HostPRB;
    EFI_PHYSICAL_ADDRESS            PhysAddrHostPRB;
    VOID*                           PciAllocMappingPRB;
} SATA_SI3132_PORT;

typedef struct _SATA_SI3132_INSTANCE {
    UINTN                       Signature;

    SATA_SI3132_PORT            Ports[SATA_SII3132_MAXPORT];

    EFI_ATA_PASS_THRU_PROTOCOL  AtaPassThruProtocol;

    EFI_PCI_IO_PROTOCOL         *PciIo;
} SATA_SI3132_INSTANCE;

#define SATA_SII3132_SIGNATURE              SIGNATURE_32('s', 'i', '3', '2')
#define INSTANCE_FROM_ATAPASSTHRU_THIS(a)   CR(a, SATA_SI3132_INSTANCE, AtaPassThruProtocol, SATA_SII3132_SIGNATURE)

#define SATA_GLOBAL_READ32(Offset, Value)  PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, 0, Offset, 1, Value)
#define SATA_GLOBAL_WRITE32(Offset, Value) { UINT32 Value32 = Value; PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 0, Offset, 1, &Value32); }

#define SATA_PORT_READ32(Offset, Value)  PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, 1, Offset, 1, Value)
#define SATA_PORT_WRITE32(Offset, Value) { UINT32 Value32 = Value; PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 1, Offset, 1, &Value32); }

#define SATA_TRACE(txt)  DEBUG((EFI_D_VERBOSE, "ARM_SATA: " txt "\n"))

extern EFI_COMPONENT_NAME_PROTOCOL  gSataSiI3132ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gSataSiI3132ComponentName2;

/*
 * Component Name Protocol Functions
 */
EFI_STATUS
EFIAPI
SataSiI3132ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
SataSiI3132ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

EFI_STATUS SiI3132HwResetPort (SATA_SI3132_PORT *Port);

/*
 * Driver Binding Protocol Functions
 */
EFI_STATUS
EFIAPI
SataSiI3132DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
SataSiI3132DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
SataSiI3132DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  );

EFI_STATUS SiI3132AtaPassThruCommand (
  IN     SATA_SI3132_INSTANCE             *pSataSiI3132Instance,
  IN     SATA_SI3132_PORT                 *pSataPort,
  IN     UINT16                           PortMultiplierPort,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET *Packet,
  IN     EFI_EVENT                        Event OPTIONAL
  );

/**
 * EFI ATA Pass Thru Protocol
 */
EFI_STATUS SiI3132AtaPassThru (
  IN     EFI_ATA_PASS_THRU_PROTOCOL       *This,
  IN     UINT16                           Port,
  IN     UINT16                           PortMultiplierPort,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET *Packet,
  IN     EFI_EVENT                        Event OPTIONAL
  );

EFI_STATUS SiI3132GetNextPort (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN OUT UINT16                 *Port
  );

EFI_STATUS SiI3132GetNextDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port,
  IN OUT UINT16                 *PortMultiplierPort
  );

EFI_STATUS SiI3132BuildDevicePath (
  IN     EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN     UINT16                     Port,
  IN     UINT16                     PortMultiplierPort,
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath
  );

EFI_STATUS SiI3132GetDevice (
  IN  EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  OUT UINT16                     *Port,
  OUT UINT16                     *PortMultiplierPort
  );

EFI_STATUS SiI3132ResetPort (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port
  );

EFI_STATUS SiI3132ResetDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port,
  IN UINT16                     PortMultiplierPort
  );

#endif
