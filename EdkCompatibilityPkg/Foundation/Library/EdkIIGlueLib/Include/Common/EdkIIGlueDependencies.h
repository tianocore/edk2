/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueDepedencies.h

Abstract:

  Header file that lists dependency relations among library instances

--*/

#ifndef __EDKII_GLUE_DEPENDENCIES_H__
#define __EDKII_GLUE_DEPENDENCIES_H__

#include "EdkIIGlueProcessorBind.h"

//
// Declarations of dependencies among EDKII Glue Library instances and EDK Libraries
// Pay attention to the order of following #define structures
//


//
// PeiDxeDebugLibReportStatusCode
// Actually almost every module and GlueLib instance needs this library, but GlueLib
// instances don't have to list this library in their own inf files. Module inf
// does this.
//
#ifdef __EDKII_GLUE_PEI_DXE_DEBUG_LIB_REPORT_STATUS_CODE__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
#endif

//
//  BaseDebugLibNull
//
#ifdef __EDKII_GLUE_BASE_DEBUG_LIB_NULL__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_PRINT_LIB__
  #define __EDKII_GLUE_BASE_PRINT_LIB__
  #endif
#endif

//
//  DxePerformanceLib
//
#ifdef __EDKII_GLUE_DXE_PERFORMANCE_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #define __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #endif
  #ifndef __EDKII_GLUE_DXE_HOB_LIB__
  #define __EDKII_GLUE_DXE_HOB_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_LIB__
  #define __EDKII_GLUE_UEFI_LIB__
  #endif
#endif

//
//  PeiPerformanceLib
//
#ifdef __EDKII_GLUE_PEI_PERFORMANCE_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_PEI_SERVICES_LIB__
  #define __EDKII_GLUE_PEI_SERVICES_LIB__
  #endif
#endif

//
// EdkDxeRuntimeDriverLib
//
#ifdef __EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__
  #ifndef __EDKII_GLUE_UEFI_LIB__
  #define __EDKII_GLUE_UEFI_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
#ifdef MDE_CPU_IPF // IPF
  #ifndef __EDKII_GLUE_EDK_DXE_SAL_LIB__
  #define __EDKII_GLUE_EDK_DXE_SAL_LIB__
  #endif
#endif // IPF
#endif

//
//  EdkDxeSalLib
//
#ifdef __EDKII_GLUE_EDK_DXE_SAL_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
#endif

//
// BasePciLibCf8
//
#ifdef __EDKII_GLUE_BASE_PCI_LIB_CF8__
  #ifndef __EDKII_GLUE_BASE_PCI_CF8_LIB__
  #define __EDKII_GLUE_BASE_PCI_CF8_LIB__
  #endif
#endif

//
// BasePciLibPciExpress
//
#ifdef __EDKII_GLUE_BASE_PCI_LIB_PCI_EXPRESS__
  #ifndef __EDKII_GLUE_BASE_PCI_EXPRESS_LIB__
  #define __EDKII_GLUE_BASE_PCI_EXPRESS_LIB__
  #endif
#endif

//
// BasePciCf8Lib
//
#ifdef __EDKII_GLUE_BASE_PCI_CF8_LIB__
#ifndef MDE_CPU_EBC
  #ifndef __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #define __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #endif
#else
  #ifndef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #define __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #endif
#endif  
#endif

//
// BasePciExpressLib
//
#ifdef __EDKII_GLUE_BASE_PCI_EXPRESS_LIB__
#ifndef MDE_CPU_EBC
  #ifndef __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #define __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #endif
#else
  #ifndef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #define __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #endif
#endif
#endif

//
// BaseTimerLibLocalApic
//
#ifdef __EDKII_GLUE_BASE_TIMER_LIB_LOCAL_APIC__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
#ifndef MDE_CPU_EBC  
  #ifndef __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #define __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #endif
#else
  #ifndef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #define __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #endif
#endif  
#endif

//
// DxeReportStatusCodeLib
//
#ifdef  __EDKII_GLUE_DXE_REPORT_STATUS_CODE_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
#ifndef MDE_CPU_EBC
  #ifndef __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #define __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #endif
#else
  #ifndef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #define __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #endif
#endif  
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB__
  #endif
#endif

//
// SmmRuntimeDxeReportStatusCodeLib - typically used by SMM driver and Runtime driver
//
#ifdef __EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB__
  #endif
#endif

//
// PeiReportStatusCodeLib
//
#ifdef  __EDKII_GLUE_PEI_REPORT_STATUS_CODE_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
#ifndef MDE_CPU_EBC  
  #ifndef __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #define __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #endif
#else
  #ifndef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #define __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #endif
#endif  
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)  
  #ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #endif
#elif defined(MDE_CPU_IPF)
  #ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #endif
#endif  
  //
  //  If necessary, __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__ or
  //  __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__ can be
  //  replaced with __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB__
  //
#endif

//
// BasePeCoffLib
//
#ifdef __EDKII_GLUE_BASE_PE_COFF_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
#endif

//
// BaseUefiDecompressLib
//
#ifdef  __EDKII_GLUE_BASE_UEFI_DECOMPRESS_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
#endif

//
// DxeHobLib
//
#ifdef  __EDKII_GLUE_DXE_HOB_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_LIB__
  #define __EDKII_GLUE_UEFI_LIB__
  #endif
#endif

//
// HiiLib
//
#ifdef  __EDKII_GLUE_HII_LIB__
  #ifndef __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #define __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #endif
#endif

//
// UefiDevicePathLib
//
#ifdef  __EDKII_GLUE_UEFI_DEVICE_PATH_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
  #ifndef __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #define __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #endif
#endif

//
// DxeServicesTableLib
//
#ifdef __EDKII_GLUE_DXE_SERVICES_TABLE_LIB__
  #ifndef __EDKII_GLUE_UEFI_LIB__
  #define __EDKII_GLUE_UEFI_LIB__
  #endif
#endif

//
// UefiLib
//
#ifdef  __EDKII_GLUE_UEFI_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
  #ifndef __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #define __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #endif
#endif

//
// DxeMemoryAllocationLib
//
#ifdef  __EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
#endif

//
// DxeSmbusLib
//
#ifdef  __EDKII_GLUE_DXE_SMBUS_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
#endif

//
// PeiHobLib
//
#ifdef  __EDKII_GLUE_PEI_HOB_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
  #ifndef __EDKII_GLUE_PEI_SERVICES_LIB__
  #define __EDKII_GLUE_PEI_SERVICES_LIB__
  #endif
#endif

//
// PeiMemoryAllocationLib
//
#ifdef  __EDKII_GLUE_PEI_MEMORY_ALLOCATION_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)  
  #ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #endif
#elif defined(MDE_CPU_IPF)
  #ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #endif
#endif  
  //
  //  If necessary, __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__ or
  //  __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__ can be
  //  replaced with __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB__
  //
#endif

//
// PeiResourcePublicationLib
//
#ifdef  __EDKII_GLUE_PEI_RESOURCE_PUBLICATION_LIB__
  #ifndef __EDKII_GLUE_PEI_SERVICES_LIB__
  #define __EDKII_GLUE_PEI_SERVICES_LIB__
  #endif
#endif

//
// PeiServicesLib
//
#ifdef  __EDKII_GLUE_PEI_SERVICES_LIB__
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)  
  #ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #endif
#elif defined(MDE_CPU_IPF)
  #ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #endif
#endif  
  //
  //  If necessary, __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__ or
  //  __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__ can be
  //  replaced with __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB__
  //
#endif

//
// PeiSmbusLib
//
#ifdef  __EDKII_GLUE_PEI_SMBUS_LIB__
  #ifndef __EDKII_GLUE_BASE_MEMORY_LIB__
  #define __EDKII_GLUE_BASE_MEMORY_LIB__
  #endif
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)  
  #ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #endif
#elif defined(MDE_CPU_IPF)
  #ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #endif
#endif  
  //
  //  If necessary, __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__ or
  //  __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__ can be
  //  replaced with __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB__
  //
#endif

//
// PeiServicesTablePointerLibMm7
//
#ifdef  __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
#endif

//
// PeiServicesTablePointerLibKr1
//
#ifdef  __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
#endif


//
// UefiDriverModelLib
//
#ifdef  __EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
#endif

//
// BasePrintLib
//
#ifdef __EDKII_GLUE_BASE_PRINT_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
#endif

//
// BaseMemoryLib
//
#ifdef __EDKII_GLUE_BASE_MEMORY_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
#endif

//
// BasePostCodeLibPort80
//
#ifdef __EDKII_GLUE_BASE_POST_CODE_LIB_PORT_80__
#ifndef MDE_CPU_EBC
  #ifndef __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #define __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #endif
#else
  #ifndef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #define __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #endif
#endif  
#endif

//
// BaseIoLibIntrinsic
//
#ifdef __EDKII_GLUE_BASE_IO_LIB_INTRINSIC__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
#endif

//
// DxeIoLibCpuIo
//
#ifdef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
  #ifndef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #define __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  #endif
#endif

//
// UefiBootServicesTableLib
//
#ifdef  __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
#endif

//
// BaseCacheMaintenanceLib
//
#ifdef __EDKII_GLUE_BASE_CACHE_MAINTENANCE_LIB__
  #ifndef __EDKII_GLUE_BASE_LIB__
  #define __EDKII_GLUE_BASE_LIB__
  #endif
#endif

//
// BaseLib
//
#ifdef __EDKII_GLUE_BASE_LIB__
#endif

//
// UefiRuntimeServicesTableLib
//
#ifdef __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB__
#endif

//
// BasePeCoffGetEntryPointLib
//
#ifdef __EDKII_GLUE_BASE_PE_COFF_GET_ENTRY_POINT_LIB__
#endif

//
// PeiServicesTablePointerLib
//
#ifdef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB__
#endif

//
// BasePostCodeLibDebug
//
#ifdef __EDKII_GLUE_BASE_POST_CODE_LIB_DEBUG__
  //
  // A DebugLib instance
  // Usually EdkIIGluePeiDxeDebugLibReportStatusCodeLib is listed in module inf
  //
#endif

//
// PeiDxePostCodeLibReportStatusCode
//
#ifdef __EDKII_GLUE_PEI_DXE_POST_CODE_LIB_REPORT_STATUS_CODE__
  //
  // PEI or DXE ReportStatusCodeLib instance
  // Usually EdkIIGluePei/DxeReportStatusCodeLib is listed in module inf
  //
#endif

//
//  Whether _gDriverExitBootServicesEvent[] contains effective handler
//
#if defined(__EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__) || defined(__EDKII_GLUE_EXIT_BOOT_SERVICES_EVENT_HANDLER__)
  #define __EDKII_GLUE_HAVE_DRIVER_EXIT_BOOT_SERVICES_EVENT__ 1
#else  
  #define __EDKII_GLUE_HAVE_DRIVER_EXIT_BOOT_SERVICES_EVENT__ 0
#endif

//
// Check against multiple instances of same library class being used
//
#if defined(__EDKII_GLUE_PEI_DXE_DEBUG_LIB_REPORT_STATUS_CODE__) && defined(__EDKII_GLUE_BASE_DEBUG_LIB_NULL__)
  #error EdkIIGluePeiDxeDebugLibReportStatusCode and EdkIIGlueBaseDebugLibNull: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_BASE_PCI_LIB_PCI_EXPRESS__) && defined(__EDKII_GLUE_BASE_PCI_LIB_CF8__)
  #error EdkIIGluePciLibPciExpress and EdkIIGluePciLibCf8: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_DXE_HOB_LIB__) && defined(__EDKII_GLUE_PEI_HOB_LIB__)
  #error EdkIIGlueDxeHobLib and EdkIIGluePeiHobLib: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_BASE_POST_CODE_LIB_PORT_80__) && defined(__EDKII_GLUE_BASE_POST_CODE_LIB_DEBUG__)
  #error EdkIIGlueBasePostCodeLibPort80 and EdkIIGlueBasePostCodeLibDebug: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_BASE_POST_CODE_LIB_PORT_80__) && defined(__EDKII_GLUE_PEI_DXE_POST_CODE_LIB_REPORT_STATUS_CODE__)
  #error EdkIIGlueBasePostCodeLibPort80 and EdkIIGluePeiDxePostCodeLibReportStatusCode: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_BASE_POST_CODE_LIB_DEBUG__) && defined(__EDKII_GLUE_PEI_DXE_POST_CODE_LIB_REPORT_STATUS_CODE__)
  #error EdkIIGlueBasePostCodeLibDebug and EdkIIGluePeiDxePostCodeLibReportStatusCode: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB__) && defined(__EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__)
  #error EdkIIGluePeiServicesTablePointerLib and EdkIIGluePeiServicesTablePointerLibMm7: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB__) && defined(__EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__)
  #error EdkIIGluePeiServicesTablePointerLib and EdkIIGluePeiServicesTablePointerLibKr1: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__) && defined(__EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__)
  #error EdkIIGluePeiServicesTablePointerLibMm7 and EdkIIGluePeiServicesTablePointerLibKr1: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_DXE_REPORT_STATUS_CODE_LIB__) && defined(__EDKII_GLUE_PEI_REPORT_STATUS_CODE_LIB__)
  #error EdkIIGlueDxeReportStatusCodeLib and EdkIIGluePeiReportStatusCodeLib: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__) && defined(__EDKII_GLUE_PEI_REPORT_STATUS_CODE_LIB__)
  #error EdkIIGlueSmmRuntimeDxeReportStatusCodeLib and EdkIIGluePeiReportStatusCodeLib: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_DXE_REPORT_STATUS_CODE_LIB__) && defined(__EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__)
  #error EdkIIGlueDxeReportStatusCodeLib and EdkIIGlueSmmRuntimeDxeReportStatusCodeLib: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_DXE_MEMORY_ALLOCATION_LIB__) && defined(__EDKII_GLUE_PEI_MEMORY_ALLOCATION_LIB__)
  #error EdkIIGlueDxeMemoryAllocationLib and EdkIIGluePeiMemoryAllocationLib: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_DXE_SMBUS_LIB__) && defined(__EDKII_GLUE_PEI_SMBUS_LIB__)
  #error EdkIIGlueDxeSmbusLib and EdkIIGluePeiSmbusLib: can only be mutual exclusively used.
#endif

#if defined(__EDKII_GLUE_BASE_IO_LIB_INTRINSIC__) && defined(__EDKII_GLUE_DXE_IO_LIB_CPU_IO__)
  #error EdkIIGlueBaseIoLibIntrinsic and EdkIIGlueDxeIoLibCpuIo: can only be mutual exclusively used.
#endif

//
// Some instances must be supplied
//
#ifdef __EDKII_GLUE_PEI_DXE_DEBUG_LIB_REPORT_STATUS_CODE__
  #if !defined(__EDKII_GLUE_DXE_REPORT_STATUS_CODE_LIB__) \
      && !defined(__EDKII_GLUE_PEI_REPORT_STATUS_CODE_LIB__) \
      && !defined(__EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__)
    #error You use EdkIIGluePeiDxeDebugLibReportStatusCode, \
      so either EdkIIGlueDxeReportStatusCodeLib, EdkIIGluePeiReportStatusCodeLib, \
      or EdkIIGlueSmmRuntimeDxeReportStatusCodeLib must be supplied
  #endif
#endif

#ifdef __EDKII_GLUE_BASE_POST_CODE_LIB_DEBUG__
  #if !defined(__EDKII_GLUE_PEI_DXE_DEBUG_LIB_REPORT_STATUS_CODE__) && !defined(__EDKII_GLUE_BASE_DEBUG_LIB_NULL__)
    #error You use EdkIIGlueBasePostCodeLibDebug, so either EdkIIGluePeiDxeDebugLibReportStatusCode or EdkIIGlueBaseDebugLibNull must be supplied
  #endif
#endif

//
//  EdkIIGlueUefiDriverModelLib used, but no Driver Binding Protocol defined
//
#ifdef __EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__
  #ifndef __EDKII_GLUE_DRIVER_BINDING_PROTOCOL_INSTANCE__
    #error "EdkIIGlueUefiDriverModelLib used, but no Driver Binding Protocol defined. Please define __EDKII_GLUE_DRIVER_BINDING_PROTOCOL_INSTANCE__."
  #endif
#endif

//
// EdkII Glue Library Constructors:
// NOTE: the constructors must be called according to dependency order
//
// UefiBootServicesTableLib         UefiBootServicesTableLibConstructor()
// DxeIoLibCpuIo                    IoLibConstructor()
// UefiRuntimeServicesTableLib      UefiRuntimeServicesTableLibConstructor()
// EdkDxeRuntimeDriverLib           RuntimeDriverLibConstruct()
// SmmRuntimeDxeReportStatusCodeLib ReportStatusCodeLibConstruct()
// DxeHobLib                        HobLibConstructor()
// UefiDriverModelLib               UefiDriverModelLibConstructor()
// PeiServicesTablePointerLib       PeiServicesTablePointerLibConstructor()
// PeiServicesTablePointerLibMm7    PeiServicesTablePointerLibConstructor()
// PeiServicesTablePointerLibKr1    PeiServicesTablePointerLibConstructor()
// DxeSmbusLib                      SmbusLibConstructor()
// DxeServicesTableLib              DxeServicesTableLibConstructor()
// DxeSalLib                        DxeSalLibConstructor()
//

#ifdef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
EFI_STATUS
EFIAPI
UefiBootServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
EFI_STATUS
EFIAPI
IoLibConstructor (
  IN      EFI_HANDLE                ImageHandle,
  IN      EFI_SYSTEM_TABLE          *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB__
EFI_STATUS
EFIAPI
UefiRuntimeServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__
EFI_STATUS
EFIAPI
RuntimeDriverLibConstruct (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__
EFI_STATUS
EFIAPI
ReportStatusCodeLibConstruct (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_DXE_HOB_LIB__
EFI_STATUS
EFIAPI
HobLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__
EFI_STATUS
EFIAPI
UefiDriverModelLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB__
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibConstructor (
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  );
#endif

#ifdef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_MM7__
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibConstructor (
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  );
#endif

#ifdef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_KR1__
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibConstructor (
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  );
#endif

#ifdef __EDKII_GLUE_DXE_SMBUS_LIB__
EFI_STATUS
EFIAPI
SmbusLibConstructor (
  IN EFI_HANDLE                ImageHandle,
  IN EFI_SYSTEM_TABLE          *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_DXE_SERVICES_TABLE_LIB__
EFI_STATUS
EFIAPI
DxeServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_EDK_DXE_SAL_LIB__
EFI_STATUS
EFIAPI
DxeSalLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

//
// EdkII Glue Library Destructors:
// NOTE: the destructors must be called according to dependency order
//
// UefiDriverModelLibDestructor    UefiDriverModelLibDestructor()
// SmmRuntimeDxeReportStatusCodeLib ReportStatusCodeLibDestruct()
// EdkDxeRuntimeDriverLib          RuntimeDriverLibDeconstruct()
//
#ifdef __EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__
EFI_STATUS
EFIAPI
UefiDriverModelLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__
EFI_STATUS
EFIAPI
ReportStatusCodeLibDestruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

#ifdef __EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__
EFI_STATUS
EFIAPI
RuntimeDriverLibDeconstruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

#endif
