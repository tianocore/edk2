/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiBind.h

Abstract:

  Tiano PEI core and PEIM binding macros

--*/

#ifndef _PEI_BIND_H_
#define _PEI_BIND_H_

#ifdef EFI_DEBUG

#ifdef EFI_NT_EMULATOR

#if (PI_SPECIFICATION_VERSION < 0x00010000)

#define EFI_PEI_CORE_ENTRY_POINT(InitFunction)                \
          UINTN                                               \
          __stdcall                                           \
          _DllMainCRTStartup (                                \
              UINTN    Inst,                                  \
              UINTN    reason_for_call,                       \
              VOID    *rserved                                \
              )                                               \
          {                                                   \
              return 1;                                       \
          }                                                   \
                                                              \
          EFI_STATUS                                          \
          __declspec( dllexport  )                            \
          __cdecl                                             \
          InitializeDriver (                                  \
            IN EFI_PEI_STARTUP_DESCRIPTOR *PeiStartup         \
              )                                               \
          {                                                   \
              return InitFunction(PeiStartup);                \
          }

#else
#define EFI_PEI_CORE_ENTRY_POINT(InitFunction)                \
          UINTN                                               \
          __stdcall                                           \
          _DllMainCRTStartup (                                \
              UINTN    Inst,                                  \
              UINTN    reason_for_call,                       \
              VOID    *rserved                                \
              )                                               \
          {                                                   \
              return 1;                                       \
          }                                                   \
                                                              \
          EFI_STATUS                                          \
          __declspec( dllexport  )                            \
          __cdecl                                             \
          InitializeDriver (                                  \
            IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData,    \
            IN CONST EFI_PEI_PPI_DESCRIPTOR *PpiList         \
              )                                               \
          {                                                   \
              return InitFunction(SecCoreData, PpiList);      \
          }

#endif

#define EFI_PEIM_ENTRY_POINT(InitFunction)                    \
          UINTN                                               \
          __stdcall                                           \
          _DllMainCRTStartup (                                \
              UINTN    Inst,                                  \
              UINTN    reason_for_call,                       \
              VOID    *rserved                                \
              )                                               \
          {                                                   \
              return 1;                                       \
          }                                                   \
                                                              \
          EFI_STATUS                                          \
          __declspec( dllexport  )                            \
          __cdecl                                             \
          InitializeDriver (                                  \
              IN EFI_FFS_FILE_HEADER       *FfsHeader,        \
              IN EFI_PEI_SERVICES          **PeiServices      \
              )                                               \
          {                                                   \
              return InitFunction(FfsHeader, PeiServices);    \
          }

#else

#define EFI_PEI_CORE_ENTRY_POINT(InitFunction)                
#define EFI_PEIM_ENTRY_POINT(InitFunction)

#endif

#else

#ifdef EFI_NT_EMULATOR

#if (PI_SPECIFICATION_VERSION < 0x00010000)

#define EFI_PEI_CORE_ENTRY_POINT(InitFunction)                \
          EFI_STATUS                                          \
          __declspec( dllexport  )                            \
          __cdecl                                             \
          InitializeDriver (                                  \
            IN EFI_PEI_STARTUP_DESCRIPTOR *PeiStartup         \
              )                                               \
          {                                                   \
              return InitFunction(PeiStartup);                \
          }

#else
#define EFI_PEI_CORE_ENTRY_POINT(InitFunction)                \
          EFI_STATUS                                          \
          __declspec( dllexport  )                            \
          __cdecl                                             \
          InitializeDriver (                                  \
            IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData,    \
            IN CONST EFI_PEI_PPI_DESCRIPTOR *PpiList         \
            )                                               \
          {                                                   \
              return InitFunction(SecCoreData, PpiList);     \
          }

#endif

          
#define EFI_PEIM_ENTRY_POINT(InitFunction)                    \
          EFI_STATUS                                          \
          __declspec( dllexport  )                            \
          __cdecl                                             \
          InitializeDriver (                                  \
              IN EFI_FFS_FILE_HEADER       *FfsHeader,        \
              IN EFI_PEI_SERVICES          **PeiServices      \
              )                                               \
          {                                                   \
              return InitFunction(FfsHeader, PeiServices);    \
          }
#else

#define EFI_PEI_CORE_ENTRY_POINT(InitFunction)                
#define EFI_PEIM_ENTRY_POINT(InitFunction)

#endif
#endif
#endif
