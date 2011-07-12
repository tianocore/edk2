/** @file
  Header file for CD recovery PEIM

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_CD_EXPRESS_H_
#define _PEI_CD_EXPRESS_H_


#include <PiPei.h>

#include <Ppi/BlockIo.h>
#include <Guid/RecoveryDevice.h>
#include <Ppi/DeviceRecoveryModule.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>


#pragma pack(1)

#define PEI_CD_EXPRESS_MAX_BLOCK_IO_PPI   8
#define PEI_CD_EXPRESS_MAX_CAPSULE_NUMBER 16

#define PEI_CD_BLOCK_SIZE                 0x800
#define PEI_MEMMORY_PAGE_SIZE             0x1000

//
// Recovery file name (in root directory)
//
#define PEI_RECOVERY_FILE_NAME  "FVMAIN.FV"

//
// Following are defined according to ISO-9660 specification
//
#define PEI_CD_STANDARD_ID                      "CD001"
#define PEI_CD_EXPRESS_STANDARD_ID_SIZE         5

#define PEI_CD_EXPRESS_VOLUME_TYPE_OFFSET       0
#define PEI_CD_EXPRESS_STANDARD_ID_OFFSET       1
#define PEI_CD_EXPRESS_VOLUME_SPACE_OFFSET      80
#define PEI_CD_EXPRESS_ROOT_DIR_RECORD_OFFSET   156

#define PEI_CD_EXPRESS_VOLUME_TYPE_PRIMARY      1
#define PEI_CD_EXPRESS_VOLUME_TYPE_TERMINATOR   255

#define PEI_CD_EXPRESS_DIR_FILE_REC_FLAG_ISDIR  0x02

typedef struct {
  UINTN                           CapsuleStartLBA;
  UINTN                           CapsuleSize;
  UINTN                           IndexBlock;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI   *BlockIo;
} PEI_CD_EXPRESS_CAPSULE_DATA;

#define PEI_CD_EXPRESS_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('p', 'c', 'd', 'e')

typedef struct {

  UINTN                                 Signature;
  EFI_PEI_SERVICES                      **PeiServices;
  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI    DeviceRecoveryPpi;
  EFI_PEI_PPI_DESCRIPTOR                PpiDescriptor;
  EFI_PEI_NOTIFY_DESCRIPTOR             NotifyDescriptor;

  UINT8                                 *BlockBuffer;
  UINTN                                 CapsuleCount;
  PEI_CD_EXPRESS_CAPSULE_DATA           CapsuleData[PEI_CD_EXPRESS_MAX_CAPSULE_NUMBER];

} PEI_CD_EXPRESS_PRIVATE_DATA;

#define PEI_CD_EXPRESS_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
          PEI_CD_EXPRESS_PRIVATE_DATA, \
          DeviceRecoveryPpi, \
          PEI_CD_EXPRESS_PRIVATE_DATA_SIGNATURE \
      )

typedef struct {
  UINT8   Length;
  UINT8   ExtendedAttributeRecordLength;
  UINT32  LocationOfExtent[2];
  UINT32  DataLength[2];
  UINT8   DateTime[7];
  UINT8   Flag;
  UINT8   FileUnitSize;
  UINT8   InterleaveGapSize;
  UINT32  VolumeSequenceNumber;
  UINT8   FileIDLength;
  UINT8   FileID[1];
} PEI_CD_EXPRESS_DIR_FILE_RECORD;

/**
  BlockIo installation notification function. 
  
  This function finds out all the current Block IO PPIs in the system and add them
  into private data.

  @param  PeiServices            Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor       Address of the notification descriptor data structure.
  @param  Ppi                    Address of the PPI that was installed.

  @retval EFI_SUCCESS            The function completes successfully.

**/
EFI_STATUS
EFIAPI
BlockIoNotifyEntry (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Finds out all the current Block IO PPIs in the system and add them into private data.

  @param PrivateData                    The private data structure that contains recovery module information.

  @retval EFI_SUCCESS                   The blocks and volumes are updated successfully.

**/
EFI_STATUS
UpdateBlocksAndVolumes (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA     *PrivateData
  );

/**
  Returns the number of DXE capsules residing on the device.

  This function searches for DXE capsules from the associated device and returns
  the number and maximum size in bytes of the capsules discovered. Entry 1 is 
  assumed to be the highest load priority and entry N is assumed to be the lowest 
  priority.

  @param[in]  PeiServices              General-purpose services that are available 
                                       to every PEIM
  @param[in]  This                     Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI
                                       instance.
  @param[out] NumberRecoveryCapsules   Pointer to a caller-allocated UINTN. On 
                                       output, *NumberRecoveryCapsules contains 
                                       the number of recovery capsule images 
                                       available for retrieval from this PEIM 
                                       instance.

  @retval EFI_SUCCESS        One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
GetNumberRecoveryCapsules (
  IN EFI_PEI_SERVICES                               **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI             *This,
  OUT UINTN                                         *NumberRecoveryCapsules
  );

/**
  Returns the size and type of the requested recovery capsule.

  This function gets the size and type of the capsule specified by CapsuleInstance.

  @param[in]  PeiServices       General-purpose services that are available to every PEIM
  @param[in]  This              Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI 
                                instance.
  @param[in]  CapsuleInstance   Specifies for which capsule instance to retrieve 
                                the information.  This parameter must be between 
                                one and the value returned by GetNumberRecoveryCapsules() 
                                in NumberRecoveryCapsules.
  @param[out] Size              A pointer to a caller-allocated UINTN in which 
                                the size of the requested recovery module is 
                                returned.
  @param[out] CapsuleType       A pointer to a caller-allocated EFI_GUID in which 
                                the type of the requested recovery capsule is 
                                returned.  The semantic meaning of the value 
                                returned is defined by the implementation.

  @retval EFI_SUCCESS        One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
GetRecoveryCapsuleInfo (
  IN  EFI_PEI_SERVICES                              **PeiServices,
  IN  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI            *This,
  IN  UINTN                                         CapsuleInstance,
  OUT UINTN                                         *Size,
  OUT EFI_GUID                                      *CapsuleType
  );

/**
  Loads a DXE capsule from some media into memory.

  This function, by whatever mechanism, retrieves a DXE capsule from some device
  and loads it into memory. Note that the published interface is device neutral.

  @param[in]     PeiServices       General-purpose services that are available 
                                   to every PEIM
  @param[in]     This              Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI
                                   instance.
  @param[in]     CapsuleInstance   Specifies which capsule instance to retrieve.
  @param[out]    Buffer            Specifies a caller-allocated buffer in which 
                                   the requested recovery capsule will be returned.

  @retval EFI_SUCCESS        The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A requested recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
LoadRecoveryCapsule (
  IN EFI_PEI_SERVICES                             **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI           *This,
  IN UINTN                                        CapsuleInstance,
  OUT VOID                                        *Buffer
  );

/**
  Finds out the recovery capsule in the current volume.

  @param PrivateData                    The private data structure that contains recovery module information.

  @retval EFI_SUCCESS                   The recovery capsule is successfully found in the volume.
  @retval EFI_NOT_FOUND                 The recovery capsule is not found in the volume.

**/
EFI_STATUS
EFIAPI
FindRecoveryCapsules (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA            *PrivateData
  );

/**
  Retrieves the recovery capsule in root directory of the current volume.

  @param PrivateData                    The private data structure that contains recovery module information.
  @param BlockIoPpi                     The Block IO PPI used to access the volume.
  @param IndexBlockDevice               The index of current block device.
  @param Lba                            The starting logic block address to retrieve capsule.

  @retval EFI_SUCCESS                   The recovery capsule is successfully found in the volume.
  @retval EFI_NOT_FOUND                 The recovery capsule is not found in the volume.
  @retval Others                        

**/
EFI_STATUS
EFIAPI
RetrieveCapsuleFileFromRoot (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA        *PrivateData,
  IN EFI_PEI_RECOVERY_BLOCK_IO_PPI          *BlockIoPpi,
  IN UINTN                                  IndexBlockDevice,
  IN UINT32                                 Lba
  );


/**
  This function compares two ASCII strings in case sensitive/insensitive way.

  @param  Source1           The first string.
  @param  Source2           The second string.
  @param  Size              The maximum comparison length.
  @param  CaseSensitive     Flag to indicate whether the comparison is case sensitive.

  @retval TRUE              The two strings are the same.
  @retval FALSE             The two string are not the same.

**/
BOOLEAN
StringCmp (
  IN UINT8      *Source1,
  IN UINT8      *Source2,
  IN UINTN      Size,
  IN BOOLEAN    CaseSensitive
  );

#pragma pack()

#endif
