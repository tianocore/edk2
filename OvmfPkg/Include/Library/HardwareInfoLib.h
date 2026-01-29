/*/@file
  Hardware info parsing functions.
  Binary data is expected as a consecutive series of header - object pairs.
  Complete library providing static Qemu fw-cfg wrappers as well as list-like
  interface to dynamically manipulate hardware info objects and parsing from
  a generic blob.

  Copyright 2021 - 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HARDWARE_INFO_LIB_H__
#define __HARDWARE_INFO_LIB_H__

#include "../Library/HardwareInfoLib/HardwareInfoTypesLib.h"

/**
  Read, if available, the next Type element in the FwCfg file.
  The FwCfg item must already be selected, this is a wrapper around
  QemuFwCfgReadBytes and the Data pointer should be set to an existent
  memory location with TypeSize bytes allocated for the date to be
  properly written. If a Type element is found in the file which has a
  size (in the header) greater than TypeSize, it is skipped.

  @param[in]    Type             Hardware Info Type to search for
  @param[in]    TypeSize         Size (in bytes) of the structure intended to
                                 be used to dereference the data
  @param[in]    TotalFileSize    Total size (in bytes) of the FwCfg file from
                                 which the data is read.
  @param[out]   Data             Pointer to a memory allocated instance into
                                 which the data is written to.
  @param[out]   DataSize         Size in bytes of the actually filled
                                 data available in the Data object after a
                                 successful operation
  @param[inout] ReadIndex        Index of the next byte to be read. Incremented
                                 accordingly after a read operation to reflect
                                 up to date status

  @retval EFI_SUCCESS             Next element found and read into Data
  @retval EFI_INVALID_PARAMETER   Operation failed
  @retval EFI_END_OF_FILE         End of the file reached, no more elements
                                  to read.
**/
EFI_STATUS
QemuFwCfgReadNextHardwareInfoByType (
  IN      HARDWARE_INFO_TYPE  Type,
  IN      UINTN               TypeSize,
  IN      UINTN               TotalFileSize,
  OUT     VOID                *Data,
  OUT     UINTN               *DataSize         OPTIONAL,
  IN OUT  UINTN               *ReadIndex
  );

/**
  Parse binary data containing resource information of multiple hardware
  elements into a list of interpreted resources.
  The translation is done on a copy-parse base so the blob can be freed
  afterwards.

  @param[in]  Blob           Binary data to be parsed
  @param[in]  BlobSize       Size (in bytes) of the binary data
  @param[in]  TypeFilter     Optional type to filter entries. Set to
                             undefined to disable filtering and retrieve all
  @param[out] ListHead       Head of the list to populate hardware information

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_INVALID_PARAMETER  Provided Blob inforation is invalid
  @retval EFI_OUT_OF_RESOURCES   Out of memory, list populated as far as
                                 possible
**/
EFI_STATUS
CreateHardwareInfoList (
  IN  UINT8               *Blob,
  IN  UINTN               BlobSize,
  IN  HARDWARE_INFO_TYPE  TypeFilter,
  OUT LIST_ENTRY          *ListHead
  );

/**
  Free the dynamically allocated list of HADWARE_INFO items populated
  during parsing of Blob

  @param ListHead         Head of the list to be destroyed
**/
VOID
FreeHardwareInfoList (
  IN OUT  LIST_ENTRY  *ListHead
  );

/**
  Retrieve the number of hardware components of a specific type
  in the list.

  @param[in]  ListHead       Head of the hardware info list
  @param[in]  Type           Type of hardware elements to count
  @param[in]  TypeSize       Size (in bytes) of the structure intended to
                             be used to dereference the data
  @return Count of elements of Type found
**/
UINTN
GetHardwareInfoCountByType (
  IN  LIST_ENTRY          *ListHead,
  IN  HARDWARE_INFO_TYPE  Type,
  IN  UINTN               TypeSize
  );

/**
  Get the First Hardware Info entry in the list of the specified type

  @param[in]  ListHead     Head of the hardware info list
  @param[in]  Type         Hardware Info Type to search for
  @param[in]  TypeSize     Size (in bytes) of the structure intended to
                           be used to dereference the data
  @return Link of first entry of specified type or list head if not found
**/
LIST_ENTRY *
GetFirstHardwareInfoByType (
  IN  LIST_ENTRY          *ListHead,
  IN  HARDWARE_INFO_TYPE  Type,
  IN  UINTN               TypeSize
  );

/**
  Get the Next Hardware Info entry in the list with the specified
  type, which follows the provided Node.

  @param[in]  ListHead       Head of the hardware info list
  @param[in]  Node           Current, already processed, node's link
  @param[in]  Type           Hardware Info Type to search for
  @param[in]  TypeSize       Size (in bytes) of the structure intended to
                             be used to dereference the data
  @return Link of next entry, after Node, of the specified type.
          List head otherwise
**/
LIST_ENTRY *
GetNextHardwareInfoByType (
  IN  LIST_ENTRY          *ListHead,
  IN  LIST_ENTRY          *Node,
  IN  HARDWARE_INFO_TYPE  Type,
  IN  UINTN               TypeSize
  );

/**
  Assess if Node stands at the end of the doubly linked list

  @param[in]  ListHead      Head of the hardware info list
  @param[in]  Node          Current Node link

  @retval TRUE  Node is at the end of the list
  @retval FALSE Node is not at the end of the list
**/
BOOLEAN
EndOfHardwareInfoList (
  IN  LIST_ENTRY  *ListHead,
  IN  LIST_ENTRY  *Node
  );

#endif // __HARDWARE_INFO_LIB_H__
