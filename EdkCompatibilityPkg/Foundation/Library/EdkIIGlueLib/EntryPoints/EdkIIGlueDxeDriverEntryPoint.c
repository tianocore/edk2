/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueDxeDriverEntryPoint.c
  
Abstract: 

  DXE Driver entry point template file

--*/

#include "EdkIIGlueDxe.h"
#include "Common/EdkIIGlueDependencies.h"

//
// Check definitions in .inf file to determine __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__
//
// __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__
//    1: only install ComponentName, DriverConfiguration and DriverDiagnostics
//    2: only install ComponentName2, DriverConfiguration2 and DriverDiagnostics2
//    3: install all including ComponentName, DriverConfiguration, DriverDiagnostics AND
//                             ComponentName2, DriverConfiguration2 and DriverDiagnostics2
//    4: install automatically according to EFI_SPECIFICATION_VERSION:
//        if EFI_SPECIFICATION_VERSION <  0x00020000: only install ComponentName, DriverConfiguration and DriverDiagnostics
//        if EFI_SPECIFICATION_VERSION >= 0x00020000: only install ComponentName2, DriverConfiguration2 and DriverDiagnostics2
//
#ifdef __EDKII_GLUE_DRIVER_MODEL_VERSION_1_DEFINED__
#undef __EDKII_GLUE_DRIVER_MODEL_VERSION_1_DEFINED__
#endif

#ifdef __EDKII_GLUE_DRIVER_MODEL_VERSION_2_DEFINED__
#undef __EDKII_GLUE_DRIVER_MODEL_VERSION_2_DEFINED__
#endif

#ifdef  __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__
#undef  __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__
#endif

#if defined(__EDKII_GLUE_COMPONENT_NAME_PROTOCOL_INSTANCE__) || defined(__EDKII_GLUE_DRIVER_CONFIGURATION_PROTOCOL_INSTANCE__) || defined(__EDKII_GLUE_DRIVER_DIAGNOSTICS_PROTOCOL_INSTANCE__)
#define __EDKII_GLUE_DRIVER_MODEL_VERSION_1_DEFINED__
#endif

#if defined(__EDKII_GLUE_COMPONENT_NAME2_PROTOCOL_INSTANCE__) || defined(__EDKII_GLUE_DRIVER_CONFIGURATION2_PROTOCOL_INSTANCE__) || defined(__EDKII_GLUE_DRIVER_DIAGNOSTICS2_PROTOCOL_INSTANCE__)
#define __EDKII_GLUE_DRIVER_MODEL_VERSION_2_DEFINED__
#endif

#if defined(__EDKII_GLUE_DRIVER_MODEL_VERSION_1_DEFINED__) && !defined(__EDKII_GLUE_DRIVER_MODEL_VERSION_2_DEFINED__) && !defined(__EDKII_GLUE_DRIVER_MODEL_AUTO_SELECT_PROTOCOLS_BY_EFI_VERSION__)
#define __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ 1
#endif

#if !defined(__EDKII_GLUE_DRIVER_MODEL_VERSION_1_DEFINED__) && defined(__EDKII_GLUE_DRIVER_MODEL_VERSION_2_DEFINED__) && !defined(__EDKII_GLUE_DRIVER_MODEL_AUTO_SELECT_PROTOCOLS_BY_EFI_VERSION__)
#define __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ 2
#endif

#if defined(__EDKII_GLUE_DRIVER_MODEL_VERSION_1_DEFINED__) && defined(__EDKII_GLUE_DRIVER_MODEL_VERSION_2_DEFINED__) && !defined(__EDKII_GLUE_DRIVER_MODEL_AUTO_SELECT_PROTOCOLS_BY_EFI_VERSION__)
#define __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ 3
#endif

//
//  To use Auto-Select, it must be:
//    1. both Protocol and Protocol2 are defined
//    2. The Protocol and Protocol2 must use same names, for example, gDriverControllerName as __EDKII_GLUE_COMPONENT_NAME_PROTOCOL_INSTANCE__ and __EDKII_GLUE_COMPONENT_NAME2_PROTOCOL_INSTANCE__
//
#if defined(__EDKII_GLUE_DRIVER_MODEL_AUTO_SELECT_PROTOCOLS_BY_EFI_VERSION__)
#if !(defined(__EDKII_GLUE_DRIVER_MODEL_VERSION_1_DEFINED__) && defined(__EDKII_GLUE_DRIVER_MODEL_VERSION_2_DEFINED__))
#error "To use Auto-Select please define both Protocol and Protocol2 instances"
#endif
#define __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ 4
#endif

#if (__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 4)
#undef  __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#define __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__  2
#else
#define __EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__  1
#endif
#endif

#if (__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 1)
UINT8 _gEdkIIGlueDriverModelProtocolSelection = 1;
#elif (__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 2)
UINT8 _gEdkIIGlueDriverModelProtocolSelection = 2;
#elif (__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 3)
UINT8 _gEdkIIGlueDriverModelProtocolSelection = 3;
#else
UINT8 _gEdkIIGlueDriverModelProtocolSelection = 0;
#endif

STATIC EFI_EVENT  _mDriverExitBootServicesNotifyEvent;

//
// Driver Model related definitions.
// LIMITATION: only support one instance of Driver Model protocols per driver.
// In case where multiple Driver Model protocols need to be installed in a single driver,
// manually edit this file and compile/link the modified file with the driver.
//
#if defined(__EDKII_GLUE_DRIVER_BINDING_PROTOCOL_INSTANCE__)
extern EFI_DRIVER_BINDING_PROTOCOL __EDKII_GLUE_DRIVER_BINDING_PROTOCOL_INSTANCE__;
#endif

#if (__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 1) || (__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 3)
#if defined(__EDKII_GLUE_COMPONENT_NAME_PROTOCOL_INSTANCE__)
extern EFI_COMPONENT_NAME_PROTOCOL   __EDKII_GLUE_COMPONENT_NAME_PROTOCOL_INSTANCE__;
#endif

#if defined(__EDKII_GLUE_DRIVER_CONFIGURATION_PROTOCOL_INSTANCE__)
extern EFI_DRIVER_CONFIGURATION_PROTOCOL __EDKII_GLUE_DRIVER_CONFIGURATION_PROTOCOL_INSTANCE__;
#endif
#endif

#if defined(__EDKII_GLUE_DRIVER_DIAGNOSTICS_PROTOCOL_INSTANCE__)
extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL __EDKII_GLUE_DRIVER_DIAGNOSTICS_PROTOCOL_INSTANCE__;
#endif

#if (__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 2) || (__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 3)
#if defined(__EDKII_GLUE_COMPONENT_NAME2_PROTOCOL_INSTANCE__)
extern EFI_COMPONENT_NAME2_PROTOCOL  __EDKII_GLUE_COMPONENT_NAME2_PROTOCOL_INSTANCE__;
#endif

#if defined(__EDKII_GLUE_DRIVER_CONFIGURATION2_PROTOCOL_INSTANCE__)
extern EFI_DRIVER_CONFIGURATION2_PROTOCOL __EDKII_GLUE_DRIVER_CONFIGURATION2_PROTOCOL_INSTANCE__;
#endif

#if defined(__EDKII_GLUE_DRIVER_DIAGNOSTICS2_PROTOCOL_INSTANCE__)
extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL __EDKII_GLUE_DRIVER_DIAGNOSTICS2_PROTOCOL_INSTANCE__;
#endif
#endif

GLOBAL_REMOVE_IF_UNREFERENCED const EFI_DRIVER_MODEL_PROTOCOL_LIST  _gDriverModelProtocolList[] = {
 {
#ifdef __EDKII_GLUE_DRIVER_BINDING_PROTOCOL_INSTANCE__
  &__EDKII_GLUE_DRIVER_BINDING_PROTOCOL_INSTANCE__,
#else
  NULL,
#endif

#if defined(__EDKII_GLUE_COMPONENT_NAME_PROTOCOL_INSTANCE__) && !(__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 2)
  &__EDKII_GLUE_COMPONENT_NAME_PROTOCOL_INSTANCE__,
#else
  NULL,
#endif

#if defined(__EDKII_GLUE_DRIVER_CONFIGURATION_PROTOCOL_INSTANCE__) && !(__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 2)
  &__EDKII_GLUE_DRIVER_CONFIGURATION_PROTOCOL_INSTANCE__,
#else
  NULL,
#endif

#if defined(__EDKII_GLUE_DRIVER_DIAGNOSTICS_PROTOCOL_INSTANCE__) && !(__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 2)
 &__EDKII_GLUE_DRIVER_DIAGNOSTICS_PROTOCOL_INSTANCE__,
#else
  NULL,
#endif

#if defined(__EDKII_GLUE_COMPONENT_NAME2_PROTOCOL_INSTANCE__) && !(__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 1)
  &__EDKII_GLUE_COMPONENT_NAME2_PROTOCOL_INSTANCE__,
#else
  NULL,
#endif

#if defined(__EDKII_GLUE_DRIVER_CONFIGURATION2_PROTOCOL_INSTANCE__) && !(__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 1)
  &__EDKII_GLUE_DRIVER_CONFIGURATION2_PROTOCOL_INSTANCE__,
#else
  NULL,
#endif

#if defined(__EDKII_GLUE_DRIVER_DIAGNOSTICS2_PROTOCOL_INSTANCE__) && !(__EDKII_GLUE_DRIVER_MODEL_PROTOCOL_SELECTION__ == 1)
 &__EDKII_GLUE_DRIVER_DIAGNOSTICS2_PROTOCOL_INSTANCE__,
#else
  NULL,
#endif
  }
};

//
// NOTE: Limitation:
// Only one handler for SetVirtualAddressMap Event and ExitBootServices Event each
//

/*
 *  This is the WRONG macro and it's kept only for backward compatibility.
 */
#ifdef __EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT__HANDLER__
VOID
EFIAPI
__EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT__HANDLER__ (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );
#endif

/*
 *  This is the CORRECT macro users should use.
 */  
#ifdef __EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT_HANDLER__
VOID
EFIAPI
__EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT_HANDLER__ (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );
#endif

/*
 *  Both __EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT__HANDLER__ and 
 *  __EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT_HANDLER__ are kept here although
 *  the former is a WRONG macro. It's kept only for backward compatibility.
 *  For a single module, it's not likely that both macros are defined.
 */
GLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverSetVirtualAddressMapEvent[] = {
#ifdef __EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT__HANDLER__
  __EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT__HANDLER__,
#endif

#ifdef __EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT_HANDLER__
  __EDKII_GLUE_SET_VIRTUAL_ADDRESS_MAP_EVENT_HANDLER__,
#endif

  NULL
};

/*
 *  This is the WRONG macro and it's kept only for backward compatibility.
 */
#ifdef __EDKII_GLUE_EXTI_BOOT_SERVICES_EVENT__HANDLER__
VOID
__EDKII_GLUE_EXTI_BOOT_SERVICES_EVENT__HANDLER__ (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );
#endif

/*
 *  This is the CORRECT macro users should use.
 */
#ifdef __EDKII_GLUE_EXIT_BOOT_SERVICES_EVENT_HANDLER__
VOID
__EDKII_GLUE_EXIT_BOOT_SERVICES_EVENT_HANDLER__ (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );
#endif

/**
  Set AtRuntime flag as TRUE after ExitBootServices
  
  @param[in]  Event   The Event that is being processed
  @param[in]  Context Event Context
**/
VOID
EFIAPI
RuntimeDriverExitBootServices (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );


/*
 *  Both __EDKII_GLUE_EXTI_BOOT_SERVICES_EVENT__HANDLER__ and 
 *  __EDKII_GLUE_EXIT_BOOT_SERVICES_EVENT_HANDLER__ are kept here although
 *  the former is a WRONG macro. It's kept only for backward compatibility.
 *  For a single module, it's not likely that both macros are defined.
 */
GLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverExitBootServicesEvent[] = {
#ifdef __EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__  
  //
  // only Runtime drivers need to link EdkDxeRuntimeDriverLib
  //
  RuntimeDriverExitBootServices,
#endif
    
#ifdef __EDKII_GLUE_EXTI_BOOT_SERVICES_EVENT__HANDLER__
  __EDKII_GLUE_EXTI_BOOT_SERVICES_EVENT__HANDLER__,
#endif

#ifdef __EDKII_GLUE_EXIT_BOOT_SERVICES_EVENT_HANDLER__
  __EDKII_GLUE_EXIT_BOOT_SERVICES_EVENT_HANDLER__,
#endif

  NULL
};


//
// Module Unload Handler
//

#ifdef __EDKII_GLUE_MODULE_UNLOAD_HANDLER__
EFI_STATUS
EFIAPI
__EDKII_GLUE_MODULE_UNLOAD_HANDLER__ (
  EFI_HANDLE        ImageHandle
  );
#endif

EFI_STATUS
EFIAPI
ProcessModuleUnloadList (
  EFI_HANDLE  ImageHandle
  )
{
#ifdef __EDKII_GLUE_MODULE_UNLOAD_HANDLER__
  return (__EDKII_GLUE_MODULE_UNLOAD_HANDLER__ (ImageHandle));
#else
  return EFI_SUCCESS;
#endif
}

#ifdef __EDKII_GLUE_EFI_CALLER_ID_GUID__
  GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiCallerIdGuid = __EDKII_GLUE_EFI_CALLER_ID_GUID__;
#endif

//
// Library constructors
//
VOID
EFIAPI
ProcessLibraryConstructorList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
//
// Declare "Status" if any of the following libraries are used
//
#if defined(__EDKII_GLUE_DXE_HOB_LIB__)                     \
    || defined(__EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__) \
    || defined(__EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__)        \
    || defined(__EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__)   \
    || defined(__EDKII_GLUE_DXE_SERVICES_TABLE_LIB__)       \
    || defined(__EDKII_GLUE_DXE_SMBUS_LIB__)                \
    || defined(__EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB__) \
    || defined(__EDKII_GLUE_EDK_DXE_SAL_LIB__)              \
    || defined(__EDKII_GLUE_DXE_IO_LIB_CPU_IO__)            \
    || defined(__EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__)
  EFI_STATUS  Status;
#endif

//
// EdkII Glue Library Constructors: 
// NOTE: the constructors must be called according to dependency order
//
// UefiBootServicesTableLib     UefiBootServicesTableLibConstructor()
// UefiRuntimeServicesTableLib  UefiRuntimeServicesTableLibConstructor() 
// DxeServicesTableLib          DxeServicesTableLibConstructor()
// DxeIoLibCpuIo                IoLibConstructor()
// DxeSalLib                    DxeSalLibConstructor(), IPF only
// EdkDxeRuntimeDriverLib       RuntimeDriverLibConstruct()
// SmmRuntimeDxeReportStatusCodeLib ReportStatusCodeLibConstruct()
// UefiDriverModelLib           UefiDriverModelLibConstructor()
// DxeHobLib                    HobLibConstructor()
// DxeSmbusLib                  SmbusLibConstructor()    
// 

#ifdef __EDKII_GLUE_UEFI_BOOT_SERVICES_TABLE_LIB__
  Status = UefiBootServicesTableLibConstructor (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB__
  Status = UefiRuntimeServicesTableLibConstructor (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_DXE_SERVICES_TABLE_LIB__
  Status = DxeServicesTableLibConstructor (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status); 
#endif

#ifdef __EDKII_GLUE_DXE_IO_LIB_CPU_IO__
  Status = IoLibConstructor (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_EDK_DXE_SAL_LIB__
  Status = DxeSalLibConstructor(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__
  Status = RuntimeDriverLibConstruct (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__
  Status = ReportStatusCodeLibConstruct (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__
  Status = UefiDriverModelLibConstructor (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_DXE_HOB_LIB__
  Status = HobLibConstructor (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_DXE_SMBUS_LIB__
  Status = SmbusLibConstructor (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

}

//
// Library Destructors
//
VOID
EFIAPI
ProcessLibraryDestructorList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
#if defined (__EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__) \
    || defined (__EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__) \
    || defined (__EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__)
  EFI_STATUS  Status;    
#endif

//
// NOTE: the destructors must be called according to dependency order
//
#ifdef __EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__
  Status = UefiDriverModelLibDestructor (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_SMM_RUNTIME_DXE_REPORT_STATUS_CODE_LIB__
  Status = ReportStatusCodeLibDestruct (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif

#ifdef __EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__
  Status = RuntimeDriverLibDeconstruct (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);
#endif
}


/**
  Unload function that is registered in the LoadImage protocol.  It un-installs
  protocols produced and deallocates pool used by the driver.  Called by the core
  when unloading the driver.

  @param  ImageHandle

  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
_DriverUnloadHandler (
  EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS  Status;

  //
  // If an UnloadImage() handler is specified, then call it
  //
  Status = ProcessModuleUnloadList (ImageHandle);

  //
  // If the driver specific unload handler does not return an error, then call all of the
  // library destructors.  If the unload handler returned an error, then the driver can not be
  // unloaded, and the library destructors should not be called
  //
  if (!EFI_ERROR (Status)) {
    //
    // Close our ExitBootServices () notify function
    //

#if __EDKII_GLUE_HAVE_DRIVER_EXIT_BOOT_SERVICES_EVENT__
    if (_gDriverExitBootServicesEvent[0] != NULL) {
      ASSERT (gBS != NULL);
      Status = gBS->CloseEvent (_mDriverExitBootServicesNotifyEvent);
      ASSERT_EFI_ERROR (Status);
    }
#endif

    ProcessLibraryDestructorList (ImageHandle, gST);
  }

  //
  // Return the status from the driver specific unload handler
  //
  return Status;
}

VOID
EFIAPI
_DriverExitBootServices (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Set AtRuntime flag as TRUE after ExitBootServices

Arguments:

  Event   - The Event that is being processed
  
  Context - Event Context

Returns: 

  None

--*/
{
#if __EDKII_GLUE_HAVE_DRIVER_EXIT_BOOT_SERVICES_EVENT__
  EFI_EVENT_NOTIFY  ChildNotifyEventHandler;
  UINTN             Index;

  for (Index = 0; _gDriverExitBootServicesEvent[Index] != NULL; Index++) {
    ChildNotifyEventHandler = _gDriverExitBootServicesEvent[Index];
    ChildNotifyEventHandler (Event, NULL);
  }
#endif  
}

EFI_DRIVER_ENTRY_POINT (_ModuleEntryPoint);

//
// Module Entry Point
//
#ifdef __EDKII_GLUE_MODULE_ENTRY_POINT__
EFI_STATUS
EFIAPI
__EDKII_GLUE_MODULE_ENTRY_POINT__ (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  );
#endif

/**
  Enrty point to DXE Driver.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.

  @retval  EFI_SUCCESS One or more of the drivers returned a success code.
  @retval  !EFI_SUCESS The return status from the last driver entry point in the list.

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;

//  if (_gUefiDriverRevision != 0) {
//    //
//    // Make sure that the EFI/UEFI spec revision of the platform is >= EFI/UEFI spec revision of the driver
//    //
//    if (SystemTable->Hdr.Revision < _gUefiDriverRevision) {
//      return EFI_INCOMPATIBLE_VERSION;
//    }
//  }

//  DEBUG ((EFI_D_ERROR, "EdkII Glue Driver Entry - 0\n"));

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (ImageHandle, SystemTable);

  //
  // Register our ExitBootServices () notify function
  //
#if __EDKII_GLUE_HAVE_DRIVER_EXIT_BOOT_SERVICES_EVENT__
  if (_gDriverExitBootServicesEvent[0] != NULL) {
    Status = SystemTable->BootServices->CreateEvent (
                    EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES,
                    EFI_TPL_NOTIFY,
                    _DriverExitBootServices,
                    NULL,
                    &_mDriverExitBootServicesNotifyEvent
                    );

    ASSERT_EFI_ERROR (Status);
  }
#endif

  //
  //  Install unload handler...
  //
  //
  //  Add conditional macro to save size. The 4 macros check against
  //  potential functions which may be invoked, if there is no function
  //  to be called, we don't register Unload callback.
  //
#if ( defined(__EDKII_GLUE_MODULE_UNLOAD_HANDLER__)                    \
      || defined(__EDKII_GLUE_UEFI_DRIVER_MODEL_LIB__)                 \
      || defined(__EDKII_GLUE_EDK_DXE_RUNTIME_DRIVER_LIB__) )          \
    || __EDKII_GLUE_HAVE_DRIVER_EXIT_BOOT_SERVICES_EVENT__ 

    do {   
    EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

    Status = SystemTable->BootServices->HandleProtocol (
                    ImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    ASSERT_EFI_ERROR (Status);
    LoadedImage->Unload = _DriverUnloadHandler;
    } while(0);

#endif  

  //
  // Call the driver entry point
  //
  #ifdef __EDKII_GLUE_MODULE_ENTRY_POINT__
  Status = (__EDKII_GLUE_MODULE_ENTRY_POINT__ (ImageHandle, SystemTable));
  #else
  Status = EFI_SUCCESS;
  #endif

  //
  // If all of the drivers returned errors, then invoke all of the library destructors
  //
  if (EFI_ERROR (Status)) {
    //
    // Close our ExitBootServices () notify function
    //
#if __EDKII_GLUE_HAVE_DRIVER_EXIT_BOOT_SERVICES_EVENT__    
    if (_gDriverExitBootServicesEvent[0] != NULL) {
    	EFI_STATUS CloseEventStatus;
      CloseEventStatus = SystemTable->BootServices->CloseEvent (_mDriverExitBootServicesNotifyEvent);
      ASSERT_EFI_ERROR (CloseEventStatus);
    }
#endif

    ProcessLibraryDestructorList (ImageHandle, SystemTable);
  }

  //
  // Return the cummalative return status code from all of the driver entry points
  //
  return Status;
}


/**
  Enrty point wrapper of DXE Driver.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.

  @retval  EFI_SUCCESS One or more of the drivers returned a success code.
  @retval  !EFI_SUCESS The return status from the last driver entry point in the list.

  EBC build envrionment has /D $(IMAGE_ENTRY_POINT)=EfiMain which overrides what GlueLib 
  defines: /D IMAGE_ENTRY_POINT=_ModuleEntryPoint, so _ModuleEntryPoint will be replaced with
  EfiMain thus the function below isn't needed in EBC envrionment.

**/
#ifndef MDE_CPU_EBC
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return _ModuleEntryPoint (ImageHandle, SystemTable);
}
#endif

//
// Guids not present in EDK code base
//

//
// Protocol/Arch Protocol GUID globals
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gUefiDriverConfigurationProtocolGuid             = { 0xbfd7dc1d, 0x24f1, 0x40d9, { 0x82, 0xe7, 0x2e, 0x09, 0xbb, 0x6b, 0x4e, 0xbe } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gUefiDriverDiagnosticsProtocolGuid               = { 0x4d330321, 0x025f, 0x4aac, { 0x90, 0xd8, 0x5e, 0xd9, 0x00, 0x17, 0x3b, 0x63 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiArpProtocolGuid                              = { 0xf4b427bb, 0xba21, 0x4f16, { 0xbc, 0x4e, 0x43, 0xe4, 0x16, 0xab, 0x61, 0x9c } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiArpServiceBindingProtocolGuid                = { 0xf44c00ee, 0x1f2c, 0x4a00, { 0xaa, 0x09, 0x1c, 0x9f, 0x3e, 0x08, 0x00, 0xa3 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiDhcp4ProtocolGuid                            = { 0x8a219718, 0x4ef5, 0x4761, { 0x91, 0xc8, 0xc0, 0xf0, 0x4b, 0xda, 0x9e, 0x56 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiDhcp4ServiceBindingProtocolGuid              = { 0x9d9a39d8, 0xbd42, 0x4a73, { 0xa4, 0xd5, 0x8e, 0xe9, 0x4b, 0xe1, 0x13, 0x80 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiIp4ProtocolGuid                              = { 0x41d94cd2, 0x35b6, 0x455a, { 0x82, 0x58, 0xd4, 0xe5, 0x13, 0x34, 0xaa, 0xdd } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiIp4ServiceBindingProtocolGuid                = { 0xc51711e7, 0xb4bf, 0x404a, { 0xbf, 0xb8, 0x0a, 0x04, 0x8e, 0xf1, 0xff, 0xe4 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiIp4ConfigProtocolGuid                        = { 0x3b95aa31, 0x3793, 0x434b, { 0x86, 0x67, 0xc8, 0x07, 0x08, 0x92, 0xe0, 0x5e } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiManagedNetworkProtocolGuid                   = { 0x3b95aa31, 0x3793, 0x434b, { 0x86, 0x67, 0xc8, 0x07, 0x08, 0x92, 0xe0, 0x5e } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiManagedNetworkServiceBindingProtocolGuid     = { 0xf36ff770, 0xa7e1, 0x42cf, { 0x9e, 0xd2, 0x56, 0xf0, 0xf2, 0x71, 0xf4, 0x4c } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiMtftp4ProtocolGuid                           = { 0x3ad9df29, 0x4501, 0x478d, { 0xb1, 0xf8, 0x7f, 0x7f, 0xe7, 0x0e, 0x50, 0xf3 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiMtftp4ServiceBindingProtocolGuid             = { 0x2FE800BE, 0x8F01, 0x4aa6, { 0x94, 0x6B, 0xD7, 0x13, 0x88, 0xE1, 0x83, 0x3F } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiTcp4ProtocolGuid                             = { 0x65530BC7, 0xA359, 0x410f, { 0xB0, 0x10, 0x5A, 0xAD, 0xC7, 0xEC, 0x2B, 0x62 } };     
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiTcp4ServiceBindingProtocolGuid               = { 0x00720665, 0x67EB, 0x4a99, { 0xBA, 0xF7, 0xD3, 0xC3, 0x3A, 0x1C, 0x7C, 0xC9 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiUdp4ProtocolGuid                             = { 0x3ad9df29, 0x4501, 0x478d, { 0xb1, 0xf8, 0x7f, 0x7f, 0xe7, 0x0e, 0x50, 0xf3 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiUdp4ServiceBindingProtocolGuid               = { 0x83f01464, 0x99bd, 0x45e5, { 0xb3, 0x83, 0xaf, 0x63, 0x05, 0xd8, 0xe9, 0xe6 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiAuthenticationInfoProtocolGuid               = { 0x7671d9d0, 0x53db, 0x4173, { 0xaa, 0x69, 0x23, 0x27, 0xf2, 0x1f, 0x0b, 0xc7 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiDevicePathFromTextProtocolGuid               = { 0x5c99a21,  0xc70f, 0x4ad2, { 0x8a, 0x5f, 0x35, 0xdf, 0x33, 0x43, 0xf5, 0x1e } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiDevicePathToTextProtocolGuid                 = { 0x8b843e20, 0x8132, 0x4852, { 0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiDevicePathUtilitiesProtocolGuid              = { 0x379be4e,  0xd706, 0x437d, { 0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHashProtocolGuid                             = { 0xc5184932, 0xdba5, 0x46db, { 0xa5, 0xba, 0xcc, 0x0b, 0xda, 0x9c, 0x14, 0x35 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHashServiceBindingProtocolGuid               = { 0x42881c98, 0xa4f3, 0x44b0, { 0xa3, 0x9d, 0xdf, 0xa1, 0x86, 0x67, 0xd8, 0xcd } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiIScsiInitiatorNameProtocolGuid               = { 0xa6a72875, 0x2962, 0x4c18, { 0x9f, 0x46, 0x8d, 0xa6, 0x44, 0xcc, 0xfe, 0x00 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiExtScsiPassThruProtocolGuid                  = { 0x1d3de7f0, 0x0807, 0x424f, { 0xaa, 0x69, 0x11, 0xa5, 0x4e, 0x19, 0xa4, 0x6f } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiTapeIoProtocolGuid                           = { 0x1e93e633, 0xd65a, 0x459e, { 0xab, 0x84, 0x93, 0xd9, 0xec, 0x26, 0x6d, 0x18 } };
#if (EFI_SPECIFICATION_VERSION < 0x00020000)
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiUsb2HcProtocolGuid                           = { 0x3e745226, 0x9818, 0x45b6, { 0xa2, 0xac, 0xd7, 0xcd, 0x0e, 0x8b, 0xa2, 0xbc } };
#endif

//
// PPI GUID globals
//

//
// GUID globals
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHobMemoryAllocBspStoreGuid              = { 0x564b33cd, 0xc92a, 0x4593, { 0x90, 0xbf, 0x24, 0x73, 0xe4, 0x3c, 0x63, 0x22 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHobMemoryAllocStackGuid                 = { 0x4ed4bf27, 0x4092, 0x42e9, { 0x80, 0x7d, 0x52, 0x7b, 0x1d, 0x00, 0xc9, 0xbd } }; 
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHobMemoryAllocModuleGuid                = { 0xf8e21975, 0x0899, 0x4f58, { 0xa4, 0xbe, 0x55, 0x25, 0xa9, 0xc6, 0xd7, 0x7a } }; 
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiAuthenticationChapRadiusGuid            = { 0xd6062b50, 0x15ca, 0x11da, { 0x92, 0x19, 0x00, 0x10, 0x83, 0xff, 0xca, 0x4d } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiAuthenticationChapLocalGuid             = { 0xc280c73e, 0x15ca, 0x11da, { 0xb0, 0xca, 0x00, 0x10, 0x83, 0xff, 0xca, 0x4d } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHashAlgorithmSha1Guid                   = { 0x2ae9d80f, 0x3fb2, 0x4095, { 0xb7, 0xb1, 0xe9, 0x31, 0x57, 0xb9, 0x46, 0xb6 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHashAlgorithmSha224Guid                 = { 0x8df01a06, 0x9bd5, 0x4bf7, { 0xb0, 0x21, 0xdb, 0x4f, 0xd9, 0xcc, 0xf4, 0x5b } }; 
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHashAlgorithmSha256Guid                 = { 0x51aa59de, 0xfdf2, 0x4ea3, { 0xbc, 0x63, 0x87, 0x5f, 0xb7, 0x84, 0x2e, 0xe9 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHashAlgorithmSha384Guid                 = { 0xefa96432, 0xde33, 0x4dd2, { 0xae, 0xe6, 0x32, 0x8c, 0x33, 0xdf, 0x77, 0x7a } };  
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHashAlgorithmSha512Guid                 = { 0xcaa4381e, 0x750c, 0x4770, { 0xb8, 0x70, 0x7a, 0x23, 0xb4, 0xe4, 0x21, 0x30 } };  
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiHashAlgorithmMD5Guid                    = { 0xaf7c79c,  0x65b5, 0x4319, { 0xb0, 0xae, 0x44, 0xec, 0x48, 0x4e, 0x4a, 0xd7 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gBootObjectAuthorizationParmsetGuid         = { 0xedd35e31, 0x7b9,  0x11d2, { 0x83, 0xa3, 0x00, 0xa0, 0xc9, 0x1f, 0xad, 0xcf } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gSmmCommunicateHeaderGuid                   = { 0xf328e36c, 0x23b6, 0x4a95, { 0x85, 0x4b, 0x32, 0xe1, 0x95, 0x34, 0xcd, 0x75 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiCapsuleGuid                             = { 0x3B6686BD, 0x0D76, 0x4030, { 0xB7, 0x0E, 0xB5, 0x51, 0x9E, 0x2F, 0xC5, 0xA0 } };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiConfigFileNameGuid                      = { 0x98B8D59B, 0xE8BA, 0x48EE, { 0x98, 0xDD, 0xC2, 0x95, 0x39, 0x2F, 0x1E, 0xDB } };
