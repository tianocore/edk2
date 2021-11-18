/** @file
  The default version of EFI_PEI_CPU_IO_PPI support published by PeiServices in
  PeiCore initialization phase.

  EFI_PEI_CPU_IO_PPI is installed by some platform or chipset-specific PEIM that
  abstracts the processor-visible I/O operations. When PeiCore is started, the
  default version of EFI_PEI_CPU_IO_PPI will be assigned to PeiServices table.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"

///
/// This default instance of EFI_PEI_CPU_IO_PPI install assigned to EFI_PEI_SERVICE.CpuIo
/// when PeiCore's initialization.
///
EFI_PEI_CPU_IO_PPI  gPeiDefaultCpuIoPpi = {
  {
    PeiDefaultMemRead,
    PeiDefaultMemWrite
  },
  {
    PeiDefaultIoRead,
    PeiDefaultIoWrite
  },
  PeiDefaultIoRead8,
  PeiDefaultIoRead16,
  PeiDefaultIoRead32,
  PeiDefaultIoRead64,
  PeiDefaultIoWrite8,
  PeiDefaultIoWrite16,
  PeiDefaultIoWrite32,
  PeiDefaultIoWrite64,
  PeiDefaultMemRead8,
  PeiDefaultMemRead16,
  PeiDefaultMemRead32,
  PeiDefaultMemRead64,
  PeiDefaultMemWrite8,
  PeiDefaultMemWrite16,
  PeiDefaultMemWrite32,
  PeiDefaultMemWrite64
};

/**
  Memory-based read services.

  This function is to perform the Memory Access Read service based on installed
  instance of the EFI_PEI_CPU_IO_PPI.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices           An indirect pointer to the PEI Services Table
                                published by the PEI Foundation.
  @param  This                  Pointer to local data for the interface.
  @param  Width                 The width of the access. Enumerated in bytes.
  @param  Address               The physical address of the access.
  @param  Count                 The number of accesses to perform.
  @param  Buffer                A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.
**/
EFI_STATUS
EFIAPI
PeiDefaultMemRead (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  IN  OUT VOID                  *Buffer
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Memory-based write services.

  This function is to perform the Memory Access Write service based on installed
  instance of the EFI_PEI_CPU_IO_PPI.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices           An indirect pointer to the PEI Services Table
                                published by the PEI Foundation.
  @param  This                  Pointer to local data for the interface.
  @param  Width                 The width of the access. Enumerated in bytes.
  @param  Address               The physical address of the access.
  @param  Count                 The number of accesses to perform.
  @param  Buffer                A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.
**/
EFI_STATUS
EFIAPI
PeiDefaultMemWrite (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  IN  OUT VOID                  *Buffer
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  IO-based read services.

  This function is to perform the IO-base read service for the EFI_PEI_CPU_IO_PPI.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices           An indirect pointer to the PEI Services Table
                                published by the PEI Foundation.
  @param  This                  Pointer to local data for the interface.
  @param  Width                 The width of the access. Enumerated in bytes.
  @param  Address               The physical address of the access.
  @param  Count                 The number of accesses to perform.
  @param  Buffer                A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.
**/
EFI_STATUS
EFIAPI
PeiDefaultIoRead (
  IN      CONST EFI_PEI_SERVICES    **PeiServices,
  IN      CONST EFI_PEI_CPU_IO_PPI  *This,
  IN      EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN      UINT64                    Address,
  IN      UINTN                     Count,
  IN OUT  VOID                      *Buffer
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  IO-based write services.

  This function is to perform the IO-base write service for the EFI_PEI_CPU_IO_PPI.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices           An indirect pointer to the PEI Services Table
                                published by the PEI Foundation.
  @param  This                  Pointer to local data for the interface.
  @param  Width                 The width of the access. Enumerated in bytes.
  @param  Address               The physical address of the access.
  @param  Count                 The number of accesses to perform.
  @param  Buffer                A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.
**/
EFI_STATUS
EFIAPI
PeiDefaultIoWrite (
  IN      CONST EFI_PEI_SERVICES    **PeiServices,
  IN      CONST EFI_PEI_CPU_IO_PPI  *This,
  IN      EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN      UINT64                    Address,
  IN      UINTN                     Count,
  IN OUT  VOID                      *Buffer
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  8-bit I/O read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 8-bit value returned from the I/O space.
**/
UINT8
EFIAPI
PeiDefaultIoRead8 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address
  )
{
  return 0;
}

/**
  Reads an 16-bit I/O port.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return A 16-bit value returned from the I/O space.
**/
UINT16
EFIAPI
PeiDefaultIoRead16 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address
  )
{
  return 0;
}

/**
  Reads an 32-bit I/O port.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return A 32-bit value returned from the I/O space.
**/
UINT32
EFIAPI
PeiDefaultIoRead32 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address
  )
{
  return 0;
}

/**
  Reads an 64-bit I/O port.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return A 64-bit value returned from the I/O space.
**/
UINT64
EFIAPI
PeiDefaultIoRead64 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address
  )
{
  return 0;
}

/**
  8-bit I/O write operations.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then do
  nothing.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.
**/
VOID
EFIAPI
PeiDefaultIoWrite8 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address,
  IN  UINT8                     Data
  )
{
}

/**
  16-bit I/O write operations.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then do
  nothing.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.
**/
VOID
EFIAPI
PeiDefaultIoWrite16 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address,
  IN  UINT16                    Data
  )
{
}

/**
  32-bit I/O write operations.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then do
  nothing.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.
**/
VOID
EFIAPI
PeiDefaultIoWrite32 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address,
  IN  UINT32                    Data
  )
{
}

/**
  64-bit I/O write operations.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then do
  nothing.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.
**/
VOID
EFIAPI
PeiDefaultIoWrite64 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address,
  IN  UINT64                    Data
  )
{
}

/**
  8-bit memory read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 8-bit value returned from the memory space.

**/
UINT8
EFIAPI
PeiDefaultMemRead8 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address
  )
{
  return 0;
}

/**
  16-bit memory read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 16-bit value returned from the memory space.

**/
UINT16
EFIAPI
PeiDefaultMemRead16 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address
  )
{
  return 0;
}

/**
  32-bit memory read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 32-bit value returned from the memory space.

**/
UINT32
EFIAPI
PeiDefaultMemRead32 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address
  )
{
  return 0;
}

/**
  64-bit memory read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 64-bit value returned from the memory space.

**/
UINT64
EFIAPI
PeiDefaultMemRead64 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address
  )
{
  return 0;
}

/**
  8-bit memory write operations.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then do
  nothing.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

**/
VOID
EFIAPI
PeiDefaultMemWrite8 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address,
  IN  UINT8                     Data
  )
{
}

/**
  16-bit memory write operations.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then do
  nothing.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

**/
VOID
EFIAPI
PeiDefaultMemWrite16 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address,
  IN  UINT16                    Data
  )
{
}

/**
  32-bit memory write operations.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then do
  nothing.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

**/
VOID
EFIAPI
PeiDefaultMemWrite32 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address,
  IN  UINT32                    Data
  )
{
}

/**
  64-bit memory write operations.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then do
  nothing.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

**/
VOID
EFIAPI
PeiDefaultMemWrite64 (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  UINT64                    Address,
  IN  UINT64                    Data
  )
{
}
