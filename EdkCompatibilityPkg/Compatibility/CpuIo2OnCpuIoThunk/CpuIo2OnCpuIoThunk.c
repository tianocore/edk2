/** @file
  Implementation of CPU I/O 2 Protocol based on Framework CPU I/O Protocol.

  Intel's Framework CPU I/O Protocol is replaced by CPU I/O 2 Protocol in PI.
  This module produces PI CPU I/O 2 Protocol on top of Framework CPU I/O Protocol.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuIo2OnCpuIoThunk.h"

EFI_HANDLE           mCpuIo2Handle = NULL;
EFI_CPU_IO_PROTOCOL  *mCpuIo;
EFI_CPU_IO2_PROTOCOL mCpuIo2 = {
  {
    CpuMemoryServiceRead,
    CpuMemoryServiceWrite
  },
  {
    CpuIoServiceRead,
    CpuIoServiceWrite
  }
};

/**
  Enables a driver to read memory-mapped registers in the PI System memory space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the memory operation.
  @param[in]       Address      The base address of the memory operation.
  @param[in]       Count        The number of memory operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in, out]   Buffer       The destination buffer to store the results.

  @retval EFI_SUCCESS           The data was read from or written to the EFI system.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system. Or Buffer is NULL.
  @retval EFI_UNSUPPORTED       The Buffer is not aligned for the given Width.
                                Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN     EFI_CPU_IO2_PROTOCOL              *This,
  IN     EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN     UINT64                            Address,
  IN     UINTN                             Count,
  IN OUT VOID                              *Buffer
  )
{
  return mCpuIo->Mem.Read (
                       mCpuIo,
                       Width,
                       Address,
                       Count,
                       Buffer
                       );
}

/**
  Enables a driver to write memory-mapped registers in the PI System memory space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the memory operation.
  @param[in]       Address      The base address of the memory operation.
  @param[in]       Count        The number of memory operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in, out]   Buffer       The source buffer from which to write data.

  @retval EFI_SUCCESS           The data was read from or written to the EFI system.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system. Or Buffer is NULL.
  @retval EFI_UNSUPPORTED       The Buffer is not aligned for the given Width.
                                Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN     EFI_CPU_IO2_PROTOCOL              *This,
  IN     EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN     UINT64                            Address,
  IN     UINTN                             Count,
  IN OUT VOID                              *Buffer
  )
{
  return mCpuIo->Mem.Write (
                       mCpuIo,
                       Width,
                       Address,
                       Count,
                       Buffer
                       );
}

/**
  Enables a driver to read registers in the PI CPU I/O space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the I/O operation.
  @param[in]       Address      The base address of the I/O operation. The caller is responsible
                                for aligning the Address if required. 
  @param[in]       Count        The number of I/O operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in, out]   Buffer       The destination buffer to store the results.

  @retval EFI_SUCCESS           The data was read from or written to the EFI system.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system. Or Buffer is NULL.
  @retval EFI_UNSUPPORTED       The Buffer is not aligned for the given Width.
                                Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN     EFI_CPU_IO2_PROTOCOL              *This,
  IN     EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN     UINT64                            Address,
  IN     UINTN                             Count,
  IN OUT VOID                              *Buffer
  )
{
  return mCpuIo->Io.Read (
                      mCpuIo,
                      Width,
                      Address,
                      Count,
                      Buffer
                      );
}

/**
  Enables a driver to write registers in the PI CPU I/O space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the I/O operation.
  @param[in]       Address      The base address of the I/O operation. The caller is responsible
                                for aligning the Address if required. 
  @param[in]       Count        The number of I/O operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in, out]   Buffer       The source buffer from which to write data.

  @retval EFI_SUCCESS           The data was read from or written to the EFI system.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system. Or Buffer is NULL.
  @retval EFI_UNSUPPORTED       The Buffer is not aligned for the given Width.
                                Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN     EFI_CPU_IO2_PROTOCOL              *This,
  IN     EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN     UINT64                            Address,
  IN     UINTN                             Count,
  IN OUT VOID                              *Buffer
  )
{
  return mCpuIo->Io.Write (
                      mCpuIo,
                      Width,
                      Address,
                      Count,
                      Buffer
                      );
}

/**
  Entrypoint of CPU I/O 2 DXE thunk module.
  
  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
CpuIo2OnCpuIoThunkInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  
  //
  // Locate and cache Framework CPU I/O Protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiCpuIoProtocolGuid, 
                  NULL, 
                  (VOID **) &mCpuIo
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install the CPU I/O 2 Protocol on a new handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCpuIo2Handle,
                  &gEfiCpuIo2ProtocolGuid, &mCpuIo2,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
