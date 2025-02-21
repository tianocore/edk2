/** @file MockMmServicesTableLib.h
  Google Test mocks for MmServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_MM_SERVICES_TABLE_LIB_H_
#define MOCK_MM_SERVICES_TABLE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/MmServicesTableLib.h>
}

//
// Declarations to handle usage of the MmServicesTableLib by creating mock
//
struct MockMmServicesTableLib {
  MOCK_INTERFACE_DECLARATION (MockMmServicesTableLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmAllocatePool,
    (
     IN  EFI_MEMORY_TYPE             PoolType,
     IN  UINTN                       Size,
     OUT VOID                        **Buffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmFreePool,
    (
     IN  VOID                        *Buffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmAllocatePages,
    (
     IN  EFI_ALLOCATE_TYPE           Type,
     IN  EFI_MEMORY_TYPE             MemoryType,
     IN  UINTN                       Pages,
     OUT EFI_PHYSICAL_ADDRESS        *Memory
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmFreePages,
    (
     IN  EFI_PHYSICAL_ADDRESS        Memory,
     IN  UINTN                       Pages
    )
    );

  // MP service
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmStartupThisAp,
    (
     IN     EFI_AP_PROCEDURE  Procedure,
     IN     UINTN             CpuNumber,
     IN OUT VOID              *ProcArguments OPTIONAL
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmInstallProtocolInterface,
    (
     IN OUT EFI_HANDLE               *Handle,
     IN     EFI_GUID                 *Protocol,
     IN     EFI_INTERFACE_TYPE       InterfaceType,
     IN     VOID                     *Interface
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmUninstallProtocolInterface,
    (
     IN EFI_HANDLE               Handle,
     IN EFI_GUID                 *Protocol,
     IN VOID                     *Interface
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmHandleProtocol,
    (
     IN  EFI_HANDLE              Handle,
     IN  EFI_GUID                *Protocol,
     OUT VOID                    **Interface
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmRegisterProtocolNotify,
    (
     IN  CONST EFI_GUID     *Protocol,
     IN  EFI_MM_NOTIFY_FN   Function,
     OUT VOID               **Registration
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmLocateHandle,
    (
     IN     EFI_LOCATE_SEARCH_TYPE  SearchType,
     IN     EFI_GUID                *Protocol,
     IN     VOID                    *SearchKey,
     IN OUT UINTN                   *BufferSize,
     OUT    EFI_HANDLE              *Buffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmLocateProtocol,
    (
     IN  EFI_GUID  *Protocol,
     IN  VOID      *Registration  OPTIONAL,
     OUT VOID      **Interface
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmiManage,
    (
     IN CONST EFI_GUID  *HandlerType,
     IN CONST VOID      *Context,
     IN OUT VOID        *CommBuffer,
     IN OUT UINTN       *CommBufferSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmInterruptRegister,
    (
     IN  EFI_MM_HANDLER_ENTRY_POINT Handler,
     IN  CONST EFI_GUID *HandlerType,
     OUT EFI_HANDLE    *DispatchHandle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmInterruptUnRegister,
    (
     IN EFI_HANDLE  DispatchHandle
    )
    );
};

#endif
