/** @file
  The AhciPei driver is used to manage ATA hard disk device working under AHCI
  mode at PEI phase.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _AHCI_PEI_H_
#define _AHCI_PEI_H_

#include <PiPei.h>

#include <IndustryStandard/Atapi.h>

#include <Ppi/AtaAhciController.h>
#include <Ppi/IoMmu.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/AtaPassThru.h>
#include <Ppi/BlockIo.h>
#include <Ppi/BlockIo2.h>
#include <Ppi/StorageSecurityCommand.h>

#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/DevicePathLib.h>

//
// Structure forward declarations
//
typedef struct _PEI_AHCI_CONTROLLER_PRIVATE_DATA PEI_AHCI_CONTROLLER_PRIVATE_DATA;

#include "AhciPeiPassThru.h"
#include "AhciPeiBlockIo.h"
#include "AhciPeiStorageSecurity.h"

//
// ATA AHCI driver implementation related definitions
//
//
// Refer SATA1.0a spec section 5.2, the Phy detection time should be less than 10ms.
// The value is in millisecond units. Add a bit of margin for robustness.
//
#define AHCI_BUS_PHY_DETECT_TIMEOUT  15
//
// Refer SATA1.0a spec, the bus reset time should be less than 1s.
// The value is in 100ns units.
//
#define AHCI_PEI_RESET_TIMEOUT  10000000
//
// Time out Value for ATA pass through protocol, in 100ns units.
//
#define ATA_TIMEOUT  30000000
//
// Maximal number of Physical Region Descriptor Table entries supported.
//
#define AHCI_MAX_PRDT_NUMBER  8

#define AHCI_CAPABILITY_OFFSET  0x0000
#define   AHCI_CAP_SAM          BIT18
#define   AHCI_CAP_SSS          BIT27

#define AHCI_GHC_OFFSET    0x0004
#define   AHCI_GHC_RESET   BIT0
#define   AHCI_GHC_ENABLE  BIT31

#define AHCI_IS_OFFSET  0x0008
#define AHCI_PI_OFFSET  0x000C

#define AHCI_MAX_PORTS  32

typedef struct {
  UINT32    Lower32;
  UINT32    Upper32;
} DATA_32;

typedef union {
  DATA_32    Uint32;
  UINT64     Uint64;
} DATA_64;

#define AHCI_ATAPI_SIG_MASK  0xFFFF0000
#define AHCI_ATA_DEVICE_SIG  0x00000000

//
// Each PRDT entry can point to a memory block up to 4M byte
//
#define AHCI_MAX_DATA_PER_PRDT  0x400000

#define AHCI_FIS_REGISTER_H2D           0x27             // Register FIS - Host to Device
#define   AHCI_FIS_REGISTER_H2D_LENGTH  20
#define AHCI_FIS_REGISTER_D2H           0x34             // Register FIS - Device to Host
#define AHCI_FIS_PIO_SETUP              0x5F             // PIO Setup FIS - Device to Host

#define AHCI_D2H_FIS_OFFSET  0x40
#define AHCI_PIO_FIS_OFFSET  0x20
#define AHCI_FIS_TYPE_MASK   0xFF

//
// Port register
//
#define AHCI_PORT_START           0x0100
#define AHCI_PORT_REG_WIDTH       0x0080
#define AHCI_PORT_CLB             0x0000
#define AHCI_PORT_CLBU            0x0004
#define AHCI_PORT_FB              0x0008
#define AHCI_PORT_FBU             0x000C
#define AHCI_PORT_IS              0x0010
#define AHCI_PORT_IE              0x0014
#define AHCI_PORT_CMD             0x0018
#define   AHCI_PORT_CMD_ST        BIT0
#define   AHCI_PORT_CMD_SUD       BIT1
#define   AHCI_PORT_CMD_POD       BIT2
#define   AHCI_PORT_CMD_CLO       BIT3
#define   AHCI_PORT_CMD_FRE       BIT4
#define   AHCI_PORT_CMD_FR        BIT14
#define   AHCI_PORT_CMD_CR        BIT15
#define   AHCI_PORT_CMD_CPD       BIT20
#define   AHCI_PORT_CMD_ATAPI     BIT24
#define   AHCI_PORT_CMD_DLAE      BIT25
#define   AHCI_PORT_CMD_ALPE      BIT26
#define   AHCI_PORT_CMD_ACTIVE    (1 << 28)
#define   AHCI_PORT_CMD_ICC_MASK  (BIT28 | BIT29 | BIT30 | BIT31)

#define AHCI_PORT_TFD         0x0020
#define   AHCI_PORT_TFD_ERR   BIT0
#define   AHCI_PORT_TFD_DRQ   BIT3
#define   AHCI_PORT_TFD_BSY   BIT7
#define   AHCI_PORT_TFD_MASK  (BIT7 | BIT3 | BIT0)

#define AHCI_PORT_SIG              0x0024
#define AHCI_PORT_SSTS             0x0028
#define   AHCI_PORT_SSTS_DET_MASK  0x000F
#define   AHCI_PORT_SSTS_DET       0x0001
#define   AHCI_PORT_SSTS_DET_PCE   0x0003

#define AHCI_PORT_SCTL             0x002C
#define   AHCI_PORT_SCTL_IPM_INIT  0x0300

#define AHCI_PORT_SERR  0x0030
#define AHCI_PORT_CI    0x0038

#define IS_ALIGNED(addr, size)         (((UINTN) (addr) & (size - 1)) == 0)
#define TIMER_PERIOD_SECONDS(Seconds)  MultU64x32((UINT64)(Seconds), 10000000)

#pragma pack(1)

//
// Received FIS structure
//
typedef struct {
  UINT8     AhciDmaSetupFis[0x1C];         // Dma Setup Fis: offset 0x00
  UINT8     AhciDmaSetupFisRsvd[0x04];
  UINT8     AhciPioSetupFis[0x14];         // Pio Setup Fis: offset 0x20
  UINT8     AhciPioSetupFisRsvd[0x0C];
  UINT8     AhciD2HRegisterFis[0x14];      // D2H Register Fis: offset 0x40
  UINT8     AhciD2HRegisterFisRsvd[0x04];
  UINT64    AhciSetDeviceBitsFis;          // Set Device Bits Fix: offset 0x58
  UINT8     AhciUnknownFis[0x40];          // Unknown Fis: offset 0x60
  UINT8     AhciUnknownFisRsvd[0x60];
} EFI_AHCI_RECEIVED_FIS;

//
// Command List structure includes total 32 entries.
// The entry Data structure is listed at the following.
//
typedef struct {
  UINT32    AhciCmdCfl   : 5;  // Command FIS Length
  UINT32    AhciCmdA     : 1;  // ATAPI
  UINT32    AhciCmdW     : 1;  // Write
  UINT32    AhciCmdP     : 1;  // Prefetchable
  UINT32    AhciCmdR     : 1;  // Reset
  UINT32    AhciCmdB     : 1;  // BIST
  UINT32    AhciCmdC     : 1;  // Clear Busy upon R_OK
  UINT32    AhciCmdRsvd  : 1;
  UINT32    AhciCmdPmp   : 4;  // Port Multiplier Port
  UINT32    AhciCmdPrdtl : 16; // Physical Region Descriptor Table Length
  UINT32    AhciCmdPrdbc;      // Physical Region Descriptor Byte Count
  UINT32    AhciCmdCtba;       // Command Table Descriptor Base Address
  UINT32    AhciCmdCtbau;      // Command Table Descriptor Base Address Upper 32-BITs
  UINT32    AhciCmdRsvd1[4];
} EFI_AHCI_COMMAND_LIST;

//
// This is a software constructed FIS.
// For Data transfer operations, this is the H2D Register FIS format as
// specified in the Serial ATA Revision 2.6 specification.
//
typedef struct {
  UINT8    AhciCFisType;
  UINT8    AhciCFisPmNum  : 4;
  UINT8    AhciCFisRsvd   : 1;
  UINT8    AhciCFisRsvd1  : 1;
  UINT8    AhciCFisRsvd2  : 1;
  UINT8    AhciCFisCmdInd : 1;
  UINT8    AhciCFisCmd;
  UINT8    AhciCFisFeature;
  UINT8    AhciCFisSecNum;
  UINT8    AhciCFisClyLow;
  UINT8    AhciCFisClyHigh;
  UINT8    AhciCFisDevHead;
  UINT8    AhciCFisSecNumExp;
  UINT8    AhciCFisClyLowExp;
  UINT8    AhciCFisClyHighExp;
  UINT8    AhciCFisFeatureExp;
  UINT8    AhciCFisSecCount;
  UINT8    AhciCFisSecCountExp;
  UINT8    AhciCFisRsvd3;
  UINT8    AhciCFisControl;
  UINT8    AhciCFisRsvd4[4];
  UINT8    AhciCFisRsvd5[44];
} EFI_AHCI_COMMAND_FIS;

//
// ACMD: ATAPI command (12 or 16 bytes)
//
typedef struct {
  UINT8    AtapiCmd[0x10];
} EFI_AHCI_ATAPI_COMMAND;

//
// Physical Region Descriptor Table includes up to 65535 entries
// The entry data structure is listed at the following.
// the actual entry number comes from the PRDTL field in the command
// list entry for this command slot.
//
typedef struct {
  UINT32    AhciPrdtDba;       // Data Base Address
  UINT32    AhciPrdtDbau;      // Data Base Address Upper 32-BITs
  UINT32    AhciPrdtRsvd;
  UINT32    AhciPrdtDbc   : 22; // Data Byte Count
  UINT32    AhciPrdtRsvd1 : 9;
  UINT32    AhciPrdtIoc   : 1; // Interrupt on Completion
} EFI_AHCI_COMMAND_PRDT;

//
// Command table Data structure which is pointed to by the entry in the command list
//
typedef struct {
  EFI_AHCI_COMMAND_FIS      CommandFis;       // A software constructed FIS.
  EFI_AHCI_ATAPI_COMMAND    AtapiCmd;         // 12 or 16 bytes ATAPI cmd.
  UINT8                     Reserved[0x30];
  //
  // The scatter/gather list for Data transfer.
  //
  EFI_AHCI_COMMAND_PRDT     PrdtTable[AHCI_MAX_PRDT_NUMBER];
} EFI_AHCI_COMMAND_TABLE;

#pragma pack()

typedef struct {
  EFI_AHCI_RECEIVED_FIS     *AhciRFis;
  EFI_AHCI_COMMAND_LIST     *AhciCmdList;
  EFI_AHCI_COMMAND_TABLE    *AhciCmdTable;
  UINTN                     MaxRFisSize;
  UINTN                     MaxCmdListSize;
  UINTN                     MaxCmdTableSize;
  VOID                      *AhciRFisMap;
  VOID                      *AhciCmdListMap;
  VOID                      *AhciCmdTableMap;
} EFI_AHCI_REGISTERS;

//
// Unique signature for AHCI ATA device information structure.
//
#define AHCI_PEI_ATA_DEVICE_DATA_SIGNATURE  SIGNATURE_32 ('A', 'P', 'A', 'D')

//
// AHCI mode device information structure.
//
typedef struct {
  UINT32                              Signature;
  LIST_ENTRY                          Link;

  UINT16                              Port;
  UINT16                              PortMultiplier;
  UINT8                               FisIndex;
  UINTN                               DeviceIndex;
  ATA_IDENTIFY_DATA                   *IdentifyData;

  BOOLEAN                             Lba48Bit;
  BOOLEAN                             TrustComputing;
  UINTN                               TrustComputingDeviceIndex;
  EFI_PEI_BLOCK_IO2_MEDIA             Media;

  PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private;
} PEI_AHCI_ATA_DEVICE_DATA;

#define AHCI_PEI_ATA_DEVICE_INFO_FROM_THIS(a)  \
  CR (a,                                       \
      PEI_AHCI_ATA_DEVICE_DATA,                \
      Link,                                    \
      AHCI_PEI_ATA_DEVICE_DATA_SIGNATURE       \
      );

//
// Unique signature for private data structure.
//
#define AHCI_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('A','P','C','P')

//
// ATA AHCI controller private data structure.
//
struct _PEI_AHCI_CONTROLLER_PRIVATE_DATA {
  UINT32                                Signature;
  UINTN                                 MmioBase;
  UINTN                                 DevicePathLength;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;

  EFI_ATA_PASS_THRU_MODE                AtaPassThruMode;
  EDKII_PEI_ATA_PASS_THRU_PPI           AtaPassThruPpi;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI         BlkIoPpi;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI        BlkIo2Ppi;
  EDKII_PEI_STORAGE_SECURITY_CMD_PPI    StorageSecurityPpi;
  EFI_PEI_PPI_DESCRIPTOR                AtaPassThruPpiList;
  EFI_PEI_PPI_DESCRIPTOR                BlkIoPpiList;
  EFI_PEI_PPI_DESCRIPTOR                BlkIo2PpiList;
  EFI_PEI_PPI_DESCRIPTOR                StorageSecurityPpiList;
  EFI_PEI_NOTIFY_DESCRIPTOR             EndOfPeiNotifyList;

  EFI_AHCI_REGISTERS                    AhciRegisters;

  UINT32                                PortBitMap;
  UINT32                                ActiveDevices;
  UINT32                                TrustComputingDevices;
  LIST_ENTRY                            DeviceList;

  UINT16                                PreviousPort;
  UINT16                                PreviousPortMultiplier;
};

#define GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_PASS_THRU(a)           \
  CR (a, PEI_AHCI_CONTROLLER_PRIVATE_DATA, AtaPassThruPpi, AHCI_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO(a)               \
  CR (a, PEI_AHCI_CONTROLLER_PRIVATE_DATA, BlkIoPpi, AHCI_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO2(a)              \
  CR (a, PEI_AHCI_CONTROLLER_PRIVATE_DATA, BlkIo2Ppi, AHCI_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_STROAGE_SECURITY(a)    \
  CR (a, PEI_AHCI_CONTROLLER_PRIVATE_DATA, StorageSecurityPpi, AHCI_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_NOTIFY(a)              \
  CR (a, PEI_AHCI_CONTROLLER_PRIVATE_DATA, EndOfPeiNotifyList, AHCI_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)

//
// Global variables
//
extern UINT32  mMaxTransferBlockNumber[2];

//
// Internal functions
//

/**
  Callback for EDKII_ATA_AHCI_HOST_CONTROLLER_PPI installation.

  @param[in] PeiServices         Pointer to PEI Services Table.
  @param[in] NotifyDescriptor    Pointer to the descriptor for the Notification
                                 event that caused this function to execute.
  @param[in] Ppi                 Pointer to the PPI data associated with this function.

  @retval EFI_SUCCESS            The function completes successfully
  @retval Others                 Cannot initialize AHCI controller from given EDKII_ATA_AHCI_HOST_CONTROLLER_PPI

**/
EFI_STATUS
EFIAPI
AtaAhciHostControllerPpiInstallationCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Callback for EDKII_PCI_DEVICE_PPI installation.

  @param[in] PeiServices         Pointer to PEI Services Table.
  @param[in] NotifyDescriptor    Pointer to the descriptor for the Notification
                                 event that caused this function to execute.
  @param[in] Ppi                 Pointer to the PPI data associated with this function.

  @retval EFI_SUCCESS            The function completes successfully
  @retval Others                 Cannot initialize AHCI controller from given PCI_DEVICE_PPI

**/
EFI_STATUS
EFIAPI
AtaAhciPciDevicePpiInstallationCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
IoMmuAllocateBuffer (
  IN UINTN                  Pages,
  OUT VOID                  **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
EFI_STATUS
IoMmuFreeBuffer (
  IN UINTN  Pages,
  IN VOID   *HostAddress,
  IN VOID   *Mapping
  );

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
IoMmuMap (
  IN  EDKII_IOMMU_OPERATION  Operation,
  IN VOID                    *HostAddress,
  IN  OUT UINTN              *NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT VOID                   **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
**/
EFI_STATUS
IoMmuUnmap (
  IN VOID  *Mapping
  );

/**
  One notified function to cleanup the allocated DMA buffers at EndOfPei.

  @param[in] PeiServices         Pointer to PEI Services Table.
  @param[in] NotifyDescriptor    Pointer to the descriptor for the Notification
                                 event that caused this function to execute.
  @param[in] Ppi                 Pointer to the PPI data associated with this function.

  @retval EFI_SUCCESS    The function completes successfully

**/
EFI_STATUS
EFIAPI
AhciPeimEndOfPei (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Collect the number of bits set within a port bitmap.

  @param[in]    PortBitMap    A 32-bit wide bit map of ATA AHCI ports.

  @retval The number of bits set in the bitmap.

**/
UINT8
AhciGetNumberOfPortsFromMap (
  IN UINT32  PortBitMap
  );

/**
  Start a PIO Data transfer on specific port.

  @param[in]     Private            The pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA.
  @param[in]     Port               The number of port.
  @param[in]     PortMultiplier     The number of port multiplier.
  @param[in]     FisIndex           The offset index of the FIS base address.
  @param[in]     Read               The transfer direction.
  @param[in]     AtaCommandBlock    The EFI_ATA_COMMAND_BLOCK data.
  @param[in,out] AtaStatusBlock     The EFI_ATA_STATUS_BLOCK data.
  @param[in,out] MemoryAddr         The pointer to the data buffer.
  @param[in]     DataCount          The data count to be transferred.
  @param[in]     Timeout            The timeout value of PIO data transfer, uses
                                    100ns as a unit.

  @retval EFI_DEVICE_ERROR        The PIO data transfer abort with error occurs.
  @retval EFI_TIMEOUT             The operation is time out.
  @retval EFI_UNSUPPORTED         The device is not ready for transfer.
  @retval EFI_OUT_OF_RESOURCES    The operation fails due to lack of resources.
  @retval EFI_SUCCESS             The PIO data transfer executes successfully.

**/
EFI_STATUS
AhciPioTransfer (
  IN     PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private,
  IN     UINT8                             Port,
  IN     UINT8                             PortMultiplier,
  IN     UINT8                             FisIndex,
  IN     BOOLEAN                           Read,
  IN     EFI_ATA_COMMAND_BLOCK             *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK              *AtaStatusBlock,
  IN OUT VOID                              *MemoryAddr,
  IN     UINT32                            DataCount,
  IN     UINT64                            Timeout
  );

/**
  Start a non data transfer on specific port.

  @param[in]     Private            The pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA.
  @param[in]     Port               The number of port.
  @param[in]     PortMultiplier     The number of port multiplier.
  @param[in]     FisIndex           The offset index of the FIS base address.
  @param[in]     AtaCommandBlock    The EFI_ATA_COMMAND_BLOCK data.
  @param[in,out] AtaStatusBlock     The EFI_ATA_STATUS_BLOCK data.
  @param[in]     Timeout            The timeout value of non data transfer, uses
                                    100ns as a unit.

  @retval EFI_DEVICE_ERROR        The non data transfer abort with error occurs.
  @retval EFI_TIMEOUT             The operation is time out.
  @retval EFI_UNSUPPORTED         The device is not ready for transfer.
  @retval EFI_SUCCESS             The non data transfer executes successfully.

**/
EFI_STATUS
AhciNonDataTransfer (
  IN     PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private,
  IN     UINT8                             Port,
  IN     UINT8                             PortMultiplier,
  IN     UINT8                             FisIndex,
  IN     EFI_ATA_COMMAND_BLOCK             *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK              *AtaStatusBlock,
  IN     UINT64                            Timeout
  );

/**
  Initialize ATA host controller at AHCI mode.

  The function is designed to initialize ATA host controller.

  @param[in,out] Private    A pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA instance.

  @retval EFI_SUCCESS             The ATA AHCI controller is initialized successfully.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource to complete while initializing
                                  the controller.
  @retval Others                  A device error occurred while initializing the
                                  controller.

**/
EFI_STATUS
AhciModeInitialization (
  IN OUT PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private
  );

/**
  Transfer data from ATA device.

  This function performs one ATA pass through transaction to transfer data from/to
  ATA device. It chooses the appropriate ATA command and protocol to invoke PassThru
  interface of ATA pass through.

  @param[in]     DeviceData        A pointer to PEI_AHCI_ATA_DEVICE_DATA structure.
  @param[in,out] Buffer            The pointer to the current transaction buffer.
  @param[in]     StartLba          The starting logical block address to be accessed.
  @param[in]     TransferLength    The block number or sector count of the transfer.
  @param[in]     IsWrite           Indicates whether it is a write operation.

  @retval EFI_SUCCESS    The data transfer is complete successfully.
  @return others         Some error occurs when transferring data.

**/
EFI_STATUS
TransferAtaDevice (
  IN     PEI_AHCI_ATA_DEVICE_DATA  *DeviceData,
  IN OUT VOID                      *Buffer,
  IN     EFI_LBA                   StartLba,
  IN     UINT32                    TransferLength,
  IN     BOOLEAN                   IsWrite
  );

/**
  Trust transfer data from/to ATA device.

  This function performs one ATA pass through transaction to do a trust transfer
  from/to ATA device. It chooses the appropriate ATA command and protocol to invoke
  PassThru interface of ATA pass through.

  @param[in]     DeviceData     Pointer to PEI_AHCI_ATA_DEVICE_DATA structure.
  @param[in,out] Buffer         The pointer to the current transaction buffer.
  @param[in]     SecurityProtocolId
                                The value of the "Security Protocol" parameter
                                of the security protocol command to be sent.
  @param[in]     SecurityProtocolSpecificData
                                The value of the "Security Protocol Specific"
                                parameter of the security protocol command to
                                be sent.
  @param[in]     TransferLength The block number or sector count of the transfer.
  @param[in]     IsTrustSend    Indicates whether it is a trust send operation
                                or not.
  @param[in]     Timeout        The timeout, in 100ns units, to use for the execution
                                of the security protocol command. A Timeout value
                                of 0 means that this function will wait indefinitely
                                for the security protocol command to execute. If
                                Timeout is greater than zero, then this function
                                will return EFI_TIMEOUT if the time required to
                                execute the receive data command is greater than
                                Timeout.
  @param[out]    TransferLengthOut
                                A pointer to a buffer to store the size in bytes
                                of the data written to the buffer. Ignore it when
                                IsTrustSend is TRUE.

  @retval EFI_SUCCESS    The data transfer is complete successfully.
  @return others         Some error occurs when transferring data.

**/
EFI_STATUS
TrustTransferAtaDevice (
  IN     PEI_AHCI_ATA_DEVICE_DATA  *DeviceData,
  IN OUT VOID                      *Buffer,
  IN     UINT8                     SecurityProtocolId,
  IN     UINT16                    SecurityProtocolSpecificData,
  IN     UINTN                     TransferLength,
  IN     BOOLEAN                   IsTrustSend,
  IN     UINT64                    Timeout,
  OUT    UINTN                     *TransferLengthOut
  );

/**
  Get the size of the current device path instance.

  @param[in]  DevicePath             A pointer to the EFI_DEVICE_PATH_PROTOCOL
                                     structure.
  @param[out] InstanceSize           The size of the current device path instance.
  @param[out] EntireDevicePathEnd    Indicate whether the instance is the last
                                     one in the device path strucure.

  @retval EFI_SUCCESS    The size of the current device path instance is fetched.
  @retval Others         Fails to get the size of the current device path instance.

**/
EFI_STATUS
GetDevicePathInstanceSize (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT UINTN                     *InstanceSize,
  OUT BOOLEAN                   *EntireDevicePathEnd
  );

/**
  Check the validity of the device path of a ATA AHCI host controller.

  @param[in] DevicePath          A pointer to the EFI_DEVICE_PATH_PROTOCOL
                                 structure.
  @param[in] DevicePathLength    The length of the device path.

  @retval EFI_SUCCESS              The device path is valid.
  @retval EFI_INVALID_PARAMETER    The device path is invalid.

**/
EFI_STATUS
AhciIsHcDevicePathValid (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINTN                     DevicePathLength
  );

/**
  Build the device path for an ATA device with given port and port multiplier number.

  @param[in]  Private               A pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA
                                    data structure.
  @param[in]  Port                  The given port number.
  @param[in]  PortMultiplierPort    The given port multiplier number.
  @param[out] DevicePathLength      The length of the device path in bytes specified
                                    by DevicePath.
  @param[out] DevicePath            The device path of ATA device.

  @retval EFI_SUCCESS               The operation succeeds.
  @retval EFI_INVALID_PARAMETER     The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES      The operation fails due to lack of resources.

**/
EFI_STATUS
AhciBuildDevicePath (
  IN  PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private,
  IN  UINT16                            Port,
  IN  UINT16                            PortMultiplierPort,
  OUT UINTN                             *DevicePathLength,
  OUT EFI_DEVICE_PATH_PROTOCOL          **DevicePath
  );

/**
  Collect the ports that need to be enumerated on a controller for S3 phase.

  @param[in]  HcDevicePath          Device path of the controller.
  @param[in]  HcDevicePathLength    Length of the device path specified by
                                    HcDevicePath.
  @param[out] PortBitMap            Bitmap that indicates the ports that need
                                    to be enumerated on the controller.

  @retval    The number of ports that need to be enumerated.

**/
UINT8
AhciS3GetEumeratePorts (
  IN  EFI_DEVICE_PATH_PROTOCOL  *HcDevicePath,
  IN  UINTN                     HcDevicePathLength,
  OUT UINT32                    *PortBitMap
  );

#endif
