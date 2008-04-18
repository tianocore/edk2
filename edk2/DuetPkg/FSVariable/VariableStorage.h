/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    VariableStorage.h

Abstract:

    handles variable store/reads with memory and file

Revision History

--*/
#ifndef _VARIABLE_STORAGE_H_
#define _VARIABLE_STORAGE_H_

#define VAR_DEFAULT_VALUE           (0xff)
#define VAR_DEFAULT_VALUE_16        EFI_SIGNATURE_16 (VAR_DEFAULT_VALUE, VAR_DEFAULT_VALUE)
#define VAR_DEFAULT_VALUE_32        EFI_SIGNATURE_32 (VAR_DEFAULT_VALUE, VAR_DEFAULT_VALUE, \
                                                      VAR_DEFAULT_VALUE, VAR_DEFAULT_VALUE)

typedef struct _VARIABLE_STORAGE VARIABLE_STORAGE;

EFI_STATUS
FileStorageConstructor (
  OUT VARIABLE_STORAGE      **VarStore,
  OUT EFI_EVENT_NOTIFY      *GoVirtualEvent,
  IN  EFI_PHYSICAL_ADDRESS  NvStorageBase,
  IN  UINTN                 Size,
  IN  UINT32                VolumeId,
  IN  CHAR16                *FilePath
  );

EFI_STATUS
MemStorageConstructor (
  OUT VARIABLE_STORAGE          **VarStore,
  OUT EFI_EVENT_NOTIFY          *GoVirtualEvent,
  IN  UINTN                     Size
  );

typedef
EFI_STATUS
(EFIAPI *ERASE_STORE) (
  IN VARIABLE_STORAGE   *This
  );

typedef
EFI_STATUS
(EFIAPI *WRITE_STORE) (
  IN VARIABLE_STORAGE   *This,
  IN UINTN              Offset,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  );

struct _VARIABLE_STORAGE {

  //
  // Functions to access the storage
  //
  ERASE_STORE           Erase;
  WRITE_STORE           Write;
};

typedef struct _VS_FILE_INFO {
  UINT8                     *FileData;      // local buffer for reading acceleration

  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;    // device having storage file
  UINT32                    VolumeId;
  CHAR16                    FilePath[256];
} VS_FILE_INFO;

typedef struct _VS_MEM_INFO {
  UINT8                     *MemData;
} VS_MEM_INFO;

typedef struct _VS_DEV {
  UINT32             Signature;
  VARIABLE_STORAGE   VarStore;
  UINTN              Size;
  
  union {
    //
    // finally visit FileInfo.FileData or MemInfo.MemData
    //
    UINT8            *Data;
    
    VS_FILE_INFO     FileInfo;
    VS_MEM_INFO      MemInfo;
  } Info;

} VS_DEV;

#define DEV_FROM_THIS(a)        CR (a, VS_DEV, VarStore, VARIABLE_STORE_SIGNATURE)

#define VAR_DATA_PTR(a)         ((a)->Info.Data)
#define VAR_FILE_DEVICEPATH(a)  ((a)->Info.FileInfo.DevicePath)
#define VAR_FILE_VOLUMEID(a)    ((a)->Info.FileInfo.VolumeId)
#define VAR_FILE_FILEPATH(a)    ((a)->Info.FileInfo.FilePath)


#endif
