/** @file
  SMM IPL that produces SMM related runtime protocols and load the SMM Core into SMRAM

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/SmmBase2.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/MmCommunication2.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/SmmConfiguration.h>
#include <Protocol/SmmControl2.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/Cpu.h>

#include <Guid/EventGroup.h>
#include <Guid/EventLegacyBios.h>
#include <Guid/LoadModuleAtFixedAddress.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>

#include "PiSmmCorePrivateData.h"

#define SMRAM_CAPABILITIES  (EFI_MEMORY_WB | EFI_MEMORY_UC)

//
// Function prototypes from produced protocols
//

/**
  Indicate whether the driver is currently executing in the SMM Initialization phase.

  @param   This                    The EFI_SMM_BASE2_PROTOCOL instance.
  @param   InSmram                 Pointer to a Boolean which, on return, indicates that the driver is currently executing
                                   inside of SMRAM (TRUE) or outside of SMRAM (FALSE).

  @retval  EFI_INVALID_PARAMETER   InSmram was NULL.
  @retval  EFI_SUCCESS             The call returned successfully.

**/
EFI_STATUS
EFIAPI
SmmBase2InSmram (
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT      BOOLEAN                 *InSmram
  );

/**
  Retrieves the location of the System Management System Table (SMST).

  @param   This                    The EFI_SMM_BASE2_PROTOCOL instance.
  @param   Smst                    On return, points to a pointer to the System Management Service Table (SMST).

  @retval  EFI_INVALID_PARAMETER   Smst or This was invalid.
  @retval  EFI_SUCCESS             The memory was returned to the system.
  @retval  EFI_UNSUPPORTED         Not in SMM.

**/
EFI_STATUS
EFIAPI
SmmBase2GetSmstLocation (
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT      EFI_SMM_SYSTEM_TABLE2   **Smst
  );

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered
  UEFI service.  This function is part of the SMM Communication Protocol that may
  be called in physical mode prior to SetVirtualAddressMap() and in virtual mode
  after SetVirtualAddressMap().

  @param[in] This                The EFI_SMM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in. On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE    The buffer is too large for the MM implementation.
                                 If this error is returned, the MessageLength field
                                 in the CommBuffer header or the integer pointed by
                                 CommSize, are updated to reflect the maximum payload
                                 size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED      The CommunicateBuffer parameter or CommSize parameter,
                                 if not omitted, are in address range that cannot be
                                 accessed by the MM environment.

**/
EFI_STATUS
EFIAPI
SmmCommunicationCommunicate (
  IN CONST EFI_SMM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize OPTIONAL
  );

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in] CommBufferPhysical  Physical address of the MM communication buffer
  @param[in] CommBufferVirtual   Virtual address of the MM communication buffer
  @param[in] CommSize            The size of the data buffer being passed in. On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE    The buffer is too large for the MM implementation.
                                 If this error is returned, the MessageLength field
                                 in the CommBuffer header or the integer pointed by
                                 CommSize, are updated to reflect the maximum payload
                                 size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED      The CommunicateBuffer parameter or CommSize parameter,
                                 if not omitted, are in address range that cannot be
                                 accessed by the MM environment.

**/
EFI_STATUS
EFIAPI
SmmCommunicationMmCommunicate2 (
  IN CONST EFI_MM_COMMUNICATION2_PROTOCOL   *This,
  IN OUT VOID                               *CommBufferPhysical,
  IN OUT VOID                               *CommBufferVirtual,
  IN OUT UINTN                              *CommSize OPTIONAL
  );

/**
  Event notification that is fired every time a gEfiSmmConfigurationProtocol installs.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplSmmConfigurationEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Event notification that is fired every time a DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signalled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplReadyToLockEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Event notification that is fired when DxeDispatch Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplDxeDispatchEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Event notification that is fired when a GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplGuidedEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Event notification that is fired when EndOfDxe Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplEndOfDxeEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
SmmIplSetVirtualAddressNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

//
// Data structure used to declare a table of protocol notifications and event
// notifications required by the SMM IPL
//
typedef struct {
  BOOLEAN           Protocol;
  BOOLEAN           CloseOnLock;
  EFI_GUID          *Guid;
  EFI_EVENT_NOTIFY  NotifyFunction;
  VOID              *NotifyContext;
  EFI_TPL           NotifyTpl;
  EFI_EVENT         Event;
} SMM_IPL_EVENT_NOTIFICATION;

//
// Handle to install the SMM Base2 Protocol and the SMM Communication Protocol
//
EFI_HANDLE  mSmmIplHandle = NULL;

//
// SMM Base 2 Protocol instance
//
EFI_SMM_BASE2_PROTOCOL  mSmmBase2 = {
  SmmBase2InSmram,
  SmmBase2GetSmstLocation
};

//
// SMM Communication Protocol instance
//
EFI_SMM_COMMUNICATION_PROTOCOL  mSmmCommunication = {
  SmmCommunicationCommunicate
};

//
// PI 1.7 MM Communication Protocol 2 instance
//
EFI_MM_COMMUNICATION2_PROTOCOL  mMmCommunication2 = {
  SmmCommunicationMmCommunicate2
};

//
// SMM Core Private Data structure that contains the data shared between
// the SMM IPL and the SMM Core.
//
SMM_CORE_PRIVATE_DATA  mSmmCorePrivateData = {
  SMM_CORE_PRIVATE_DATA_SIGNATURE,    // Signature
  NULL,                               // SmmIplImageHandle
  0,                                  // SmramRangeCount
  NULL,                               // SmramRanges
  NULL,                               // SmmEntryPoint
  FALSE,                              // SmmEntryPointRegistered
  FALSE,                              // InSmm
  NULL,                               // Smst
  NULL,                               // CommunicationBuffer
  0,                                  // BufferSize
  EFI_SUCCESS                         // ReturnStatus
};

//
// Global pointer used to access mSmmCorePrivateData from outside and inside SMM
//
SMM_CORE_PRIVATE_DATA  *gSmmCorePrivate = &mSmmCorePrivateData;

//
// SMM IPL global variables
//
EFI_SMM_CONTROL2_PROTOCOL  *mSmmControl2;
EFI_SMM_ACCESS2_PROTOCOL   *mSmmAccess;
EFI_SMRAM_DESCRIPTOR       *mCurrentSmramRange;
BOOLEAN                    mSmmLocked = FALSE;
BOOLEAN                    mEndOfDxe  = FALSE;
EFI_PHYSICAL_ADDRESS       mSmramCacheBase;
UINT64                     mSmramCacheSize;

EFI_SMM_COMMUNICATE_HEADER mCommunicateHeader;
EFI_LOAD_FIXED_ADDRESS_CONFIGURATION_TABLE    *mLMFAConfigurationTable = NULL;

//
// Table of Protocol notification and GUIDed Event notifications that the SMM IPL requires
//
SMM_IPL_EVENT_NOTIFICATION  mSmmIplEvents[] = {
  //
  // Declare protocol notification on the SMM Configuration protocol.  When this notification is established,
  // the associated event is immediately signalled, so the notification function will be executed and the
  // SMM Configuration Protocol will be found if it is already in the handle database.
  //
  { TRUE,  FALSE, &gEfiSmmConfigurationProtocolGuid,  SmmIplSmmConfigurationEventNotify, &gEfiSmmConfigurationProtocolGuid,  TPL_NOTIFY,   NULL },
  //
  // Declare protocol notification on DxeSmmReadyToLock protocols.  When this notification is established,
  // the associated event is immediately signalled, so the notification function will be executed and the
  // DXE SMM Ready To Lock Protocol will be found if it is already in the handle database.
  //
  { TRUE,  TRUE,  &gEfiDxeSmmReadyToLockProtocolGuid, SmmIplReadyToLockEventNotify,      &gEfiDxeSmmReadyToLockProtocolGuid, TPL_CALLBACK, NULL },
  //
  // Declare event notification on EndOfDxe event.  When this notification is established,
  // the associated event is immediately signalled, so the notification function will be executed and the
  // SMM End Of Dxe Protocol will be found if it is already in the handle database.
  //
  { FALSE, TRUE,  &gEfiEndOfDxeEventGroupGuid,        SmmIplGuidedEventNotify,           &gEfiEndOfDxeEventGroupGuid,        TPL_CALLBACK, NULL },
  //
  // Declare event notification on EndOfDxe event.  This is used to set EndOfDxe event signaled flag.
  //
  { FALSE, TRUE,  &gEfiEndOfDxeEventGroupGuid,        SmmIplEndOfDxeEventNotify,         &gEfiEndOfDxeEventGroupGuid,        TPL_CALLBACK, NULL },
  //
  // Declare event notification on the DXE Dispatch Event Group.  This event is signaled by the DXE Core
  // each time the DXE Core dispatcher has completed its work.  When this event is signalled, the SMM Core
  // if notified, so the SMM Core can dispatch SMM drivers.
  //
  { FALSE, TRUE,  &gEfiEventDxeDispatchGuid,          SmmIplDxeDispatchEventNotify,      &gEfiEventDxeDispatchGuid,          TPL_CALLBACK, NULL },
  //
  // Declare event notification on Ready To Boot Event Group.  This is an extra event notification that is
  // used to make sure SMRAM is locked before any boot options are processed.
  //
  { FALSE, TRUE,  &gEfiEventReadyToBootGuid,          SmmIplReadyToLockEventNotify,      &gEfiEventReadyToBootGuid,          TPL_CALLBACK, NULL },
  //
  // Declare event notification on Legacy Boot Event Group.  This is used to inform the SMM Core that the platform
  // is performing a legacy boot operation, and that the UEFI environment is no longer available and the SMM Core
  // must guarantee that it does not access any UEFI related structures outside of SMRAM.
  // It is also to inform the SMM Core to notify SMM driver that system enter legacy boot.
  //
  { FALSE, FALSE, &gEfiEventLegacyBootGuid,           SmmIplGuidedEventNotify,           &gEfiEventLegacyBootGuid,           TPL_CALLBACK, NULL },
  //
  // Declare event notification on Exit Boot Services Event Group.  This is used to inform the SMM Core
  // to notify SMM driver that system enter exit boot services.
  //
  { FALSE, FALSE, &gEfiEventExitBootServicesGuid,     SmmIplGuidedEventNotify,           &gEfiEventExitBootServicesGuid,     TPL_CALLBACK, NULL },
  //
  // Declare event notification on Ready To Boot Event Group.  This is used to inform the SMM Core
  // to notify SMM driver that system enter ready to boot.
  //
  { FALSE, FALSE, &gEfiEventReadyToBootGuid,          SmmIplGuidedEventNotify,           &gEfiEventReadyToBootGuid,          TPL_CALLBACK, NULL },
  //
  // Declare event notification on SetVirtualAddressMap() Event Group.  This is used to convert gSmmCorePrivate
  // and mSmmControl2 from physical addresses to virtual addresses.
  //
  { FALSE, FALSE, &gEfiEventVirtualAddressChangeGuid, SmmIplSetVirtualAddressNotify,     NULL,                               TPL_CALLBACK, NULL },
  //
  // Terminate the table of event notifications
  //
  { FALSE, FALSE, NULL,                               NULL,                              NULL,                               TPL_CALLBACK, NULL }
};

/**
  Find the maximum SMRAM cache range that covers the range specified by SmramRange.

  This function searches and joins all adjacent ranges of SmramRange into a range to be cached.

  @param   SmramRange       The SMRAM range to search from.
  @param   SmramCacheBase   The returned cache range base.
  @param   SmramCacheSize   The returned cache range size.

**/
VOID
GetSmramCacheRange (
  IN  EFI_SMRAM_DESCRIPTOR *SmramRange,
  OUT EFI_PHYSICAL_ADDRESS *SmramCacheBase,
  OUT UINT64               *SmramCacheSize
  )
{
  UINTN                Index;
  EFI_PHYSICAL_ADDRESS RangeCpuStart;
  UINT64               RangePhysicalSize;
  BOOLEAN              FoundAjacentRange;

  *SmramCacheBase = SmramRange->CpuStart;
  *SmramCacheSize = SmramRange->PhysicalSize;

  do {
    FoundAjacentRange = FALSE;
    for (Index = 0; Index < gSmmCorePrivate->SmramRangeCount; Index++) {
      RangeCpuStart     = gSmmCorePrivate->SmramRanges[Index].CpuStart;
      RangePhysicalSize = gSmmCorePrivate->SmramRanges[Index].PhysicalSize;
      if (RangeCpuStart < *SmramCacheBase && *SmramCacheBase == (RangeCpuStart + RangePhysicalSize)) {
        *SmramCacheBase   = RangeCpuStart;
        *SmramCacheSize  += RangePhysicalSize;
        FoundAjacentRange = TRUE;
      } else if ((*SmramCacheBase + *SmramCacheSize) == RangeCpuStart && RangePhysicalSize > 0) {
        *SmramCacheSize  += RangePhysicalSize;
        FoundAjacentRange = TRUE;
      }
    }
  } while (FoundAjacentRange);

}

/**
  Indicate whether the driver is currently executing in the SMM Initialization phase.

  @param   This                    The EFI_SMM_BASE2_PROTOCOL instance.
  @param   InSmram                 Pointer to a Boolean which, on return, indicates that the driver is currently executing
                                   inside of SMRAM (TRUE) or outside of SMRAM (FALSE).

  @retval  EFI_INVALID_PARAMETER   InSmram was NULL.
  @retval  EFI_SUCCESS             The call returned successfully.

**/
EFI_STATUS
EFIAPI
SmmBase2InSmram (
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT      BOOLEAN                 *InSmram
  )
{
  if (InSmram == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *InSmram = gSmmCorePrivate->InSmm;

  return EFI_SUCCESS;
}

/**
  Retrieves the location of the System Management System Table (SMST).

  @param   This                    The EFI_SMM_BASE2_PROTOCOL instance.
  @param   Smst                    On return, points to a pointer to the System Management Service Table (SMST).

  @retval  EFI_INVALID_PARAMETER   Smst or This was invalid.
  @retval  EFI_SUCCESS             The memory was returned to the system.
  @retval  EFI_UNSUPPORTED         Not in SMM.

**/
EFI_STATUS
EFIAPI
SmmBase2GetSmstLocation (
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT      EFI_SMM_SYSTEM_TABLE2   **Smst
  )
{
  if ((This == NULL) ||(Smst == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!gSmmCorePrivate->InSmm) {
    return EFI_UNSUPPORTED;
  }

  *Smst = gSmmCorePrivate->Smst;

  return EFI_SUCCESS;
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered
  UEFI service.  This function is part of the SMM Communication Protocol that may
  be called in physical mode prior to SetVirtualAddressMap() and in virtual mode
  after SetVirtualAddressMap().

  @param[in] This                The EFI_SMM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in. On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE    The buffer is too large for the MM implementation.
                                 If this error is returned, the MessageLength field
                                 in the CommBuffer header or the integer pointed by
                                 CommSize, are updated to reflect the maximum payload
                                 size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED      The CommunicateBuffer parameter or CommSize parameter,
                                 if not omitted, are in address range that cannot be
                                 accessed by the MM environment.

**/
EFI_STATUS
EFIAPI
SmmCommunicationCommunicate (
  IN CONST EFI_SMM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_SMM_COMMUNICATE_HEADER  *CommunicateHeader;
  BOOLEAN                     OldInSmm;
  UINTN                       TempCommSize;

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *) CommBuffer;

  if (CommSize == NULL) {
    TempCommSize = OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data) + CommunicateHeader->MessageLength;
  } else {
    TempCommSize = *CommSize;
    //
    // CommSize must hold HeaderGuid and MessageLength
    //
    if (TempCommSize < OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // If not already in SMM, then generate a Software SMI
  //
  if (!gSmmCorePrivate->InSmm && gSmmCorePrivate->SmmEntryPointRegistered) {
    //
    // Put arguments for Software SMI in gSmmCorePrivate
    //
    gSmmCorePrivate->CommunicationBuffer = CommBuffer;
    gSmmCorePrivate->BufferSize          = TempCommSize;

    //
    // Generate Software SMI
    //
    Status = mSmmControl2->Trigger (mSmmControl2, NULL, NULL, FALSE, 0);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    //
    // Return status from software SMI
    //
    if (CommSize != NULL) {
      *CommSize = gSmmCorePrivate->BufferSize;
    }
    return gSmmCorePrivate->ReturnStatus;
  }

  //
  // If we are in SMM, then the execution mode must be physical, which means that
  // OS established virtual addresses can not be used.  If SetVirtualAddressMap()
  // has been called, then a direct invocation of the Software SMI is not allowed,
  // so return EFI_INVALID_PARAMETER.
  //
  if (EfiGoneVirtual()) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If we are not in SMM, don't allow call SmiManage() directly when SMRAM is closed or locked.
  //
  if ((!gSmmCorePrivate->InSmm) && (!mSmmAccess->OpenState || mSmmAccess->LockState)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Save current InSmm state and set InSmm state to TRUE
  //
  OldInSmm = gSmmCorePrivate->InSmm;
  gSmmCorePrivate->InSmm = TRUE;

  //
  // Before SetVirtualAddressMap(), we are in SMM or SMRAM is open and unlocked, call SmiManage() directly.
  //
  TempCommSize -= OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
  Status = gSmmCorePrivate->Smst->SmiManage (
                                    &CommunicateHeader->HeaderGuid,
                                    NULL,
                                    CommunicateHeader->Data,
                                    &TempCommSize
                                    );
  TempCommSize += OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
  if (CommSize != NULL) {
    *CommSize = TempCommSize;
  }

  //
  // Restore original InSmm state
  //
  gSmmCorePrivate->InSmm = OldInSmm;

  return (Status == EFI_SUCCESS) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in] CommBufferPhysical  Physical address of the MM communication buffer
  @param[in] CommBufferVirtual   Virtual address of the MM communication buffer
  @param[in] CommSize            The size of the data buffer being passed in. On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE    The buffer is too large for the MM implementation.
                                 If this error is returned, the MessageLength field
                                 in the CommBuffer header or the integer pointed by
                                 CommSize, are updated to reflect the maximum payload
                                 size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED      The CommunicateBuffer parameter or CommSize parameter,
                                 if not omitted, are in address range that cannot be
                                 accessed by the MM environment.

**/
EFI_STATUS
EFIAPI
SmmCommunicationMmCommunicate2 (
  IN CONST EFI_MM_COMMUNICATION2_PROTOCOL   *This,
  IN OUT VOID                               *CommBufferPhysical,
  IN OUT VOID                               *CommBufferVirtual,
  IN OUT UINTN                              *CommSize OPTIONAL
  )
{
  return SmmCommunicationCommunicate (&mSmmCommunication,
                                      CommBufferPhysical,
                                      CommSize);
}

/**
  Event notification that is fired when GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplGuidedEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN                       Size;

  //
  // Use Guid to initialize EFI_SMM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&mCommunicateHeader.HeaderGuid, (EFI_GUID *)Context);
  mCommunicateHeader.MessageLength = 1;
  mCommunicateHeader.Data[0] = 0;

  //
  // Generate the Software SMI and return the result
  //
  Size = sizeof (mCommunicateHeader);
  SmmCommunicationCommunicate (&mSmmCommunication, &mCommunicateHeader, &Size);
}

/**
  Event notification that is fired when EndOfDxe Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplEndOfDxeEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mEndOfDxe = TRUE;
}

/**
  Event notification that is fired when DxeDispatch Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplDxeDispatchEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN                       Size;
  EFI_STATUS                  Status;

  //
  // Keep calling the SMM Core Dispatcher until there is no request to restart it.
  //
  while (TRUE) {
    //
    // Use Guid to initialize EFI_SMM_COMMUNICATE_HEADER structure
    // Clear the buffer passed into the Software SMI.  This buffer will return
    // the status of the SMM Core Dispatcher.
    //
    CopyGuid (&mCommunicateHeader.HeaderGuid, (EFI_GUID *)Context);
    mCommunicateHeader.MessageLength = 1;
    mCommunicateHeader.Data[0] = 0;

    //
    // Generate the Software SMI and return the result
    //
    Size = sizeof (mCommunicateHeader);
    SmmCommunicationCommunicate (&mSmmCommunication, &mCommunicateHeader, &Size);

    //
    // Return if there is no request to restart the SMM Core Dispatcher
    //
    if (mCommunicateHeader.Data[0] != COMM_BUFFER_SMM_DISPATCH_RESTART) {
      return;
    }

    //
    // Close all SMRAM ranges to protect SMRAM
    // NOTE: SMRR is enabled by CPU SMM driver by calling SmmCpuFeaturesInitializeProcessor() from SmmCpuFeaturesLib
    //       so no need to reset the SMRAM to UC in MTRR.
    //
    Status = mSmmAccess->Close (mSmmAccess);
    ASSERT_EFI_ERROR (Status);

    //
    // Print debug message that the SMRAM window is now closed.
    //
    DEBUG ((DEBUG_INFO, "SMM IPL closed SMRAM window\n"));
  }
}

/**
  Event notification that is fired every time a gEfiSmmConfigurationProtocol installs.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplSmmConfigurationEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                      Status;
  EFI_SMM_CONFIGURATION_PROTOCOL  *SmmConfiguration;

  //
  // Make sure this notification is for this handler
  //
  Status = gBS->LocateProtocol (Context, NULL, (VOID **)&SmmConfiguration);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Register the SMM Entry Point provided by the SMM Core with the SMM Configuration protocol
  //
  Status = SmmConfiguration->RegisterSmmEntry (SmmConfiguration, gSmmCorePrivate->SmmEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Set flag to indicate that the SMM Entry Point has been registered which
  // means that SMIs are now fully operational.
  //
  gSmmCorePrivate->SmmEntryPointRegistered = TRUE;

  //
  // Print debug message showing SMM Core entry point address.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL registered SMM Entry Point address %p\n", (VOID *)(UINTN)gSmmCorePrivate->SmmEntryPoint));
}

/**
  Event notification that is fired every time a DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplReadyToLockEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;
  UINTN       Index;

  //
  // See if we are already locked
  //
  if (mSmmLocked) {
    return;
  }

  //
  // Make sure this notification is for this handler
  //
  if (CompareGuid ((EFI_GUID *)Context, &gEfiDxeSmmReadyToLockProtocolGuid)) {
    Status = gBS->LocateProtocol (&gEfiDxeSmmReadyToLockProtocolGuid, NULL, &Interface);
    if (EFI_ERROR (Status)) {
      return;
    }
  } else {
    //
    // If SMM is not locked yet and we got here from gEfiEventReadyToBootGuid being
    // signaled, then gEfiDxeSmmReadyToLockProtocolGuid was not installed as expected.
    // Print a warning on debug builds.
    //
    DEBUG ((DEBUG_WARN, "SMM IPL!  DXE SMM Ready To Lock Protocol not installed before Ready To Boot signal\n"));
  }

  if (!mEndOfDxe) {
    DEBUG ((DEBUG_ERROR, "EndOfDxe Event must be signaled before DxeSmmReadyToLock Protocol installation!\n"));
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED,
      (EFI_SOFTWARE_SMM_DRIVER | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE)
      );
    ASSERT (FALSE);
  }

  //
  // Lock the SMRAM (Note: Locking SMRAM may not be supported on all platforms)
  //
  mSmmAccess->Lock (mSmmAccess);

  //
  // Close protocol and event notification events that do not apply after the
  // DXE SMM Ready To Lock Protocol has been installed or the Ready To Boot
  // event has been signalled.
  //
  for (Index = 0; mSmmIplEvents[Index].NotifyFunction != NULL; Index++) {
    if (mSmmIplEvents[Index].CloseOnLock) {
      gBS->CloseEvent (mSmmIplEvents[Index].Event);
    }
  }

  //
  // Inform SMM Core that the DxeSmmReadyToLock protocol was installed
  //
  SmmIplGuidedEventNotify (Event, (VOID *)&gEfiDxeSmmReadyToLockProtocolGuid);

  //
  // Print debug message that the SMRAM window is now locked.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL locked SMRAM window\n"));

  //
  // Set flag so this operation will not be performed again
  //
  mSmmLocked = TRUE;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
SmmIplSetVirtualAddressNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mSmmControl2);
}

/**
  Get the fixed loading address from image header assigned by build tool. This function only be called
  when Loading module at Fixed address feature enabled.

  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image that needs to be examined by this function.
  @retval EFI_SUCCESS               An fixed loading address is assigned to this image by build tools .
  @retval EFI_NOT_FOUND             The image has no assigned fixed loading address.
**/
EFI_STATUS
GetPeCoffImageFixLoadingAssignedAddress(
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
   UINTN                              SectionHeaderOffset;
   EFI_STATUS                         Status;
   EFI_IMAGE_SECTION_HEADER           SectionHeader;
   EFI_IMAGE_OPTIONAL_HEADER_UNION    *ImgHdr;
   EFI_PHYSICAL_ADDRESS               FixLoadingAddress;
   UINT16                             Index;
   UINTN                              Size;
   UINT16                             NumberOfSections;
   EFI_PHYSICAL_ADDRESS               SmramBase;
   UINT64                             SmmCodeSize;
   UINT64                             ValueInSectionHeader;
   //
   // Build tool will calculate the smm code size and then patch the PcdLoadFixAddressSmmCodePageNumber
   //
   SmmCodeSize = EFI_PAGES_TO_SIZE (PcdGet32(PcdLoadFixAddressSmmCodePageNumber));

   FixLoadingAddress = 0;
   Status = EFI_NOT_FOUND;
   SmramBase = mLMFAConfigurationTable->SmramBase;
   //
   // Get PeHeader pointer
   //
   ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((CHAR8* )ImageContext->Handle + ImageContext->PeCoffHeaderOffset);
   SectionHeaderOffset = ImageContext->PeCoffHeaderOffset +
                         sizeof (UINT32) +
                         sizeof (EFI_IMAGE_FILE_HEADER) +
                         ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader;
   NumberOfSections = ImgHdr->Pe32.FileHeader.NumberOfSections;

   //
   // Get base address from the first section header that doesn't point to code section.
   //
   for (Index = 0; Index < NumberOfSections; Index++) {
     //
     // Read section header from file
     //
     Size = sizeof (EFI_IMAGE_SECTION_HEADER);
     Status = ImageContext->ImageRead (
                              ImageContext->Handle,
                              SectionHeaderOffset,
                              &Size,
                              &SectionHeader
                              );
     if (EFI_ERROR (Status)) {
       return Status;
     }

     Status = EFI_NOT_FOUND;

     if ((SectionHeader.Characteristics & EFI_IMAGE_SCN_CNT_CODE) == 0) {
       //
       // Build tool saves the offset to SMRAM base as image base in PointerToRelocations & PointerToLineNumbers fields in the
       // first section header that doesn't point to code section in image header. And there is an assumption that when the
       // feature is enabled, if a module is assigned a loading address by tools, PointerToRelocations & PointerToLineNumbers
       // fields should NOT be Zero, or else, these 2 fields should be set to Zero
       //
       ValueInSectionHeader = ReadUnaligned64((UINT64*)&SectionHeader.PointerToRelocations);
       if (ValueInSectionHeader != 0) {
         //
         // Found first section header that doesn't point to code section in which build tool saves the
         // offset to SMRAM base as image base in PointerToRelocations & PointerToLineNumbers fields
         //
         FixLoadingAddress = (EFI_PHYSICAL_ADDRESS)(SmramBase + (INT64)ValueInSectionHeader);

         if (SmramBase + SmmCodeSize > FixLoadingAddress && SmramBase <=  FixLoadingAddress) {
           //
           // The assigned address is valid. Return the specified loading address
           //
           ImageContext->ImageAddress = FixLoadingAddress;
           Status = EFI_SUCCESS;
         }
       }
       break;
     }
     SectionHeaderOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
   }
   DEBUG ((EFI_D_INFO|EFI_D_LOAD, "LOADING MODULE FIXED INFO: Loading module at fixed address %x, Status = %r \n", FixLoadingAddress, Status));
   return Status;
}
/**
  Load the SMM Core image into SMRAM and executes the SMM Core from SMRAM.

  @param[in, out] SmramRange            Descriptor for the range of SMRAM to reload the
                                        currently executing image, the rang of SMRAM to
                                        hold SMM Core will be excluded.
  @param[in, out] SmramRangeSmmCore     Descriptor for the range of SMRAM to hold SMM Core.

  @param[in]      Context               Context to pass into SMM Core

  @return  EFI_STATUS

**/
EFI_STATUS
ExecuteSmmCoreFromSmram (
  IN OUT EFI_SMRAM_DESCRIPTOR   *SmramRange,
  IN OUT EFI_SMRAM_DESCRIPTOR   *SmramRangeSmmCore,
  IN     VOID                   *Context
  )
{
  EFI_STATUS                    Status;
  VOID                          *SourceBuffer;
  UINTN                         SourceSize;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINTN                         PageCount;
  EFI_IMAGE_ENTRY_POINT         EntryPoint;

  //
  // Search all Firmware Volumes for a PE/COFF image in a file of type SMM_CORE
  //
  Status = GetSectionFromAnyFvByFileType (
             EFI_FV_FILETYPE_SMM_CORE,
             0,
             EFI_SECTION_PE32,
             0,
             &SourceBuffer,
             &SourceSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize ImageContext
  //
  ImageContext.Handle    = SourceBuffer;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // if Loading module at Fixed Address feature is enabled, the SMM core driver will be loaded to
  // the address assigned by build tool.
  //
  if (PcdGet64(PcdLoadModuleAtFixAddressEnable) != 0) {
    //
    // Get the fixed loading address assigned by Build tool
    //
    Status = GetPeCoffImageFixLoadingAssignedAddress (&ImageContext);
    if (!EFI_ERROR (Status)) {
      //
      // Since the memory range to load SMM CORE will be cut out in SMM core, so no need to allocate and free this range
      //
      PageCount = 0;
      //
      // Reserved Smram Region for SmmCore is not used, and remove it from SmramRangeCount.
      //
      gSmmCorePrivate->SmramRangeCount --;
    } else {
      DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED ERROR: Loading module at fixed address at address failed\n"));
      //
      // Allocate memory for the image being loaded from the EFI_SRAM_DESCRIPTOR
      // specified by SmramRange
      //
      PageCount = (UINTN)EFI_SIZE_TO_PAGES((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);

      ASSERT ((SmramRange->PhysicalSize & EFI_PAGE_MASK) == 0);
      ASSERT (SmramRange->PhysicalSize > EFI_PAGES_TO_SIZE (PageCount));

      SmramRange->PhysicalSize -= EFI_PAGES_TO_SIZE (PageCount);
      SmramRangeSmmCore->CpuStart = SmramRange->CpuStart + SmramRange->PhysicalSize;
      SmramRangeSmmCore->PhysicalStart = SmramRange->PhysicalStart + SmramRange->PhysicalSize;
      SmramRangeSmmCore->RegionState = SmramRange->RegionState | EFI_ALLOCATED;
      SmramRangeSmmCore->PhysicalSize = EFI_PAGES_TO_SIZE (PageCount);

      //
      // Align buffer on section boundary
      //
      ImageContext.ImageAddress = SmramRangeSmmCore->CpuStart;
    }
  } else {
    //
    // Allocate memory for the image being loaded from the EFI_SRAM_DESCRIPTOR
    // specified by SmramRange
    //
    PageCount = (UINTN)EFI_SIZE_TO_PAGES((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);

    ASSERT ((SmramRange->PhysicalSize & EFI_PAGE_MASK) == 0);
    ASSERT (SmramRange->PhysicalSize > EFI_PAGES_TO_SIZE (PageCount));

    SmramRange->PhysicalSize -= EFI_PAGES_TO_SIZE (PageCount);
    SmramRangeSmmCore->CpuStart = SmramRange->CpuStart + SmramRange->PhysicalSize;
    SmramRangeSmmCore->PhysicalStart = SmramRange->PhysicalStart + SmramRange->PhysicalSize;
    SmramRangeSmmCore->RegionState = SmramRange->RegionState | EFI_ALLOCATED;
    SmramRangeSmmCore->PhysicalSize = EFI_PAGES_TO_SIZE (PageCount);

    //
    // Align buffer on section boundary
    //
    ImageContext.ImageAddress = SmramRangeSmmCore->CpuStart;
  }

  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~((EFI_PHYSICAL_ADDRESS)ImageContext.SectionAlignment - 1);

  //
  // Print debug message showing SMM Core load address.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL loading SMM Core at SMRAM address %p\n", (VOID *)(UINTN)ImageContext.ImageAddress));

  //
  // Load the image to our new buffer
  //
  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (!EFI_ERROR (Status)) {
    //
    // Relocate the image in our new buffer
    //
    Status = PeCoffLoaderRelocateImage (&ImageContext);
    if (!EFI_ERROR (Status)) {
      //
      // Flush the instruction cache so the image data are written before we execute it
      //
      InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

      //
      // Print debug message showing SMM Core entry point address.
      //
      DEBUG ((DEBUG_INFO, "SMM IPL calling SMM Core at SMRAM address %p\n", (VOID *)(UINTN)ImageContext.EntryPoint));

      gSmmCorePrivate->PiSmmCoreImageBase = ImageContext.ImageAddress;
      gSmmCorePrivate->PiSmmCoreImageSize = ImageContext.ImageSize;
      DEBUG ((DEBUG_INFO, "PiSmmCoreImageBase - 0x%016lx\n", gSmmCorePrivate->PiSmmCoreImageBase));
      DEBUG ((DEBUG_INFO, "PiSmmCoreImageSize - 0x%016lx\n", gSmmCorePrivate->PiSmmCoreImageSize));

      gSmmCorePrivate->PiSmmCoreEntryPoint = ImageContext.EntryPoint;

      //
      // Execute image
      //
      EntryPoint = (EFI_IMAGE_ENTRY_POINT)(UINTN)ImageContext.EntryPoint;
      Status = EntryPoint ((EFI_HANDLE)Context, gST);
    }
  }

  //
  // Always free memory allocated by GetFileBufferByFilePath ()
  //
  FreePool (SourceBuffer);

  return Status;
}

/**
  SMM split SMRAM entry.

  @param[in, out] RangeToCompare             Pointer to EFI_SMRAM_DESCRIPTOR to compare.
  @param[in, out] ReservedRangeToCompare     Pointer to EFI_SMM_RESERVED_SMRAM_REGION to compare.
  @param[out]     Ranges                     Output pointer to hold split EFI_SMRAM_DESCRIPTOR entry.
  @param[in, out] RangeCount                 Pointer to range count.
  @param[out]     ReservedRanges             Output pointer to hold split EFI_SMM_RESERVED_SMRAM_REGION entry.
  @param[in, out] ReservedRangeCount         Pointer to reserved range count.
  @param[out]     FinalRanges                Output pointer to hold split final EFI_SMRAM_DESCRIPTOR entry
                                             that no need to be split anymore.
  @param[in, out] FinalRangeCount            Pointer to final range count.

**/
VOID
SmmSplitSmramEntry (
  IN OUT EFI_SMRAM_DESCRIPTOR           *RangeToCompare,
  IN OUT EFI_SMM_RESERVED_SMRAM_REGION  *ReservedRangeToCompare,
  OUT    EFI_SMRAM_DESCRIPTOR           *Ranges,
  IN OUT UINTN                          *RangeCount,
  OUT    EFI_SMM_RESERVED_SMRAM_REGION  *ReservedRanges,
  IN OUT UINTN                          *ReservedRangeCount,
  OUT    EFI_SMRAM_DESCRIPTOR           *FinalRanges,
  IN OUT UINTN                          *FinalRangeCount
  )
{
  UINT64    RangeToCompareEnd;
  UINT64    ReservedRangeToCompareEnd;

  RangeToCompareEnd         = RangeToCompare->CpuStart + RangeToCompare->PhysicalSize;
  ReservedRangeToCompareEnd = ReservedRangeToCompare->SmramReservedStart + ReservedRangeToCompare->SmramReservedSize;

  if ((RangeToCompare->CpuStart >= ReservedRangeToCompare->SmramReservedStart) &&
      (RangeToCompare->CpuStart < ReservedRangeToCompareEnd)) {
    if (RangeToCompareEnd < ReservedRangeToCompareEnd) {
      //
      // RangeToCompare  ReservedRangeToCompare
      //                 ----                    ----    --------------------------------------
      //                 |  |                    |  | -> 1. ReservedRangeToCompare
      // ----            |  |                    |--|    --------------------------------------
      // |  |            |  |                    |  |
      // |  |            |  |                    |  | -> 2. FinalRanges[*FinalRangeCount] and increment *FinalRangeCount
      // |  |            |  |                    |  |       RangeToCompare->PhysicalSize = 0
      // ----            |  |                    |--|    --------------------------------------
      //                 |  |                    |  | -> 3. ReservedRanges[*ReservedRangeCount] and increment *ReservedRangeCount
      //                 ----                    ----    --------------------------------------
      //

      //
      // 1. Update ReservedRangeToCompare.
      //
      ReservedRangeToCompare->SmramReservedSize = RangeToCompare->CpuStart - ReservedRangeToCompare->SmramReservedStart;
      //
      // 2. Update FinalRanges[FinalRangeCount] and increment *FinalRangeCount.
      //    Zero RangeToCompare->PhysicalSize.
      //
      FinalRanges[*FinalRangeCount].CpuStart      = RangeToCompare->CpuStart;
      FinalRanges[*FinalRangeCount].PhysicalStart = RangeToCompare->PhysicalStart;
      FinalRanges[*FinalRangeCount].RegionState   = RangeToCompare->RegionState | EFI_ALLOCATED;
      FinalRanges[*FinalRangeCount].PhysicalSize  = RangeToCompare->PhysicalSize;
      *FinalRangeCount += 1;
      RangeToCompare->PhysicalSize = 0;
      //
      // 3. Update ReservedRanges[*ReservedRangeCount] and increment *ReservedRangeCount.
      //
      ReservedRanges[*ReservedRangeCount].SmramReservedStart = FinalRanges[*FinalRangeCount - 1].CpuStart + FinalRanges[*FinalRangeCount - 1].PhysicalSize;
      ReservedRanges[*ReservedRangeCount].SmramReservedSize  = ReservedRangeToCompareEnd - RangeToCompareEnd;
      *ReservedRangeCount += 1;
    } else {
      //
      // RangeToCompare  ReservedRangeToCompare
      //                 ----                    ----    --------------------------------------
      //                 |  |                    |  | -> 1. ReservedRangeToCompare
      // ----            |  |                    |--|    --------------------------------------
      // |  |            |  |                    |  |
      // |  |            |  |                    |  | -> 2. FinalRanges[*FinalRangeCount] and increment *FinalRangeCount
      // |  |            |  |                    |  |
      // |  |            ----                    |--|    --------------------------------------
      // |  |                                    |  | -> 3. RangeToCompare
      // ----                                    ----    --------------------------------------
      //

      //
      // 1. Update ReservedRangeToCompare.
      //
      ReservedRangeToCompare->SmramReservedSize = RangeToCompare->CpuStart - ReservedRangeToCompare->SmramReservedStart;
      //
      // 2. Update FinalRanges[FinalRangeCount] and increment *FinalRangeCount.
      //
      FinalRanges[*FinalRangeCount].CpuStart      = RangeToCompare->CpuStart;
      FinalRanges[*FinalRangeCount].PhysicalStart = RangeToCompare->PhysicalStart;
      FinalRanges[*FinalRangeCount].RegionState   = RangeToCompare->RegionState | EFI_ALLOCATED;
      FinalRanges[*FinalRangeCount].PhysicalSize  = ReservedRangeToCompareEnd - RangeToCompare->CpuStart;
      *FinalRangeCount += 1;
      //
      // 3. Update RangeToCompare.
      //
      RangeToCompare->CpuStart      += FinalRanges[*FinalRangeCount - 1].PhysicalSize;
      RangeToCompare->PhysicalStart += FinalRanges[*FinalRangeCount - 1].PhysicalSize;
      RangeToCompare->PhysicalSize  -= FinalRanges[*FinalRangeCount - 1].PhysicalSize;
    }
  } else if ((ReservedRangeToCompare->SmramReservedStart >= RangeToCompare->CpuStart) &&
             (ReservedRangeToCompare->SmramReservedStart < RangeToCompareEnd)) {
    if (ReservedRangeToCompareEnd < RangeToCompareEnd) {
      //
      // RangeToCompare  ReservedRangeToCompare
      // ----                                    ----    --------------------------------------
      // |  |                                    |  | -> 1. RangeToCompare
      // |  |            ----                    |--|    --------------------------------------
      // |  |            |  |                    |  |
      // |  |            |  |                    |  | -> 2. FinalRanges[*FinalRangeCount] and increment *FinalRangeCount
      // |  |            |  |                    |  |       ReservedRangeToCompare->SmramReservedSize = 0
      // |  |            ----                    |--|    --------------------------------------
      // |  |                                    |  | -> 3. Ranges[*RangeCount] and increment *RangeCount
      // ----                                    ----    --------------------------------------
      //

      //
      // 1. Update RangeToCompare.
      //
      RangeToCompare->PhysicalSize = ReservedRangeToCompare->SmramReservedStart - RangeToCompare->CpuStart;
      //
      // 2. Update FinalRanges[FinalRangeCount] and increment *FinalRangeCount.
      //    ReservedRangeToCompare->SmramReservedSize = 0
      //
      FinalRanges[*FinalRangeCount].CpuStart      = ReservedRangeToCompare->SmramReservedStart;
      FinalRanges[*FinalRangeCount].PhysicalStart = RangeToCompare->PhysicalStart + RangeToCompare->PhysicalSize;
      FinalRanges[*FinalRangeCount].RegionState   = RangeToCompare->RegionState | EFI_ALLOCATED;
      FinalRanges[*FinalRangeCount].PhysicalSize  = ReservedRangeToCompare->SmramReservedSize;
      *FinalRangeCount += 1;
      ReservedRangeToCompare->SmramReservedSize = 0;
      //
      // 3. Update Ranges[*RangeCount] and increment *RangeCount.
      //
      Ranges[*RangeCount].CpuStart      = FinalRanges[*FinalRangeCount - 1].CpuStart + FinalRanges[*FinalRangeCount - 1].PhysicalSize;
      Ranges[*RangeCount].PhysicalStart = FinalRanges[*FinalRangeCount - 1].PhysicalStart + FinalRanges[*FinalRangeCount - 1].PhysicalSize;
      Ranges[*RangeCount].RegionState   = RangeToCompare->RegionState;
      Ranges[*RangeCount].PhysicalSize  = RangeToCompareEnd - ReservedRangeToCompareEnd;
      *RangeCount += 1;
    } else {
      //
      // RangeToCompare  ReservedRangeToCompare
      // ----                                    ----    --------------------------------------
      // |  |                                    |  | -> 1. RangeToCompare
      // |  |            ----                    |--|    --------------------------------------
      // |  |            |  |                    |  |
      // |  |            |  |                    |  | -> 2. FinalRanges[*FinalRangeCount] and increment *FinalRangeCount
      // |  |            |  |                    |  |
      // ----            |  |                    |--|    --------------------------------------
      //                 |  |                    |  | -> 3. ReservedRangeToCompare
      //                 ----                    ----    --------------------------------------
      //

      //
      // 1. Update RangeToCompare.
      //
      RangeToCompare->PhysicalSize = ReservedRangeToCompare->SmramReservedStart - RangeToCompare->CpuStart;
      //
      // 2. Update FinalRanges[FinalRangeCount] and increment *FinalRangeCount.
      //    ReservedRangeToCompare->SmramReservedSize = 0
      //
      FinalRanges[*FinalRangeCount].CpuStart      = ReservedRangeToCompare->SmramReservedStart;
      FinalRanges[*FinalRangeCount].PhysicalStart = RangeToCompare->PhysicalStart + RangeToCompare->PhysicalSize;
      FinalRanges[*FinalRangeCount].RegionState   = RangeToCompare->RegionState | EFI_ALLOCATED;
      FinalRanges[*FinalRangeCount].PhysicalSize  = RangeToCompareEnd - ReservedRangeToCompare->SmramReservedStart;
      *FinalRangeCount += 1;
      //
      // 3. Update ReservedRangeToCompare.
      //
      ReservedRangeToCompare->SmramReservedStart += FinalRanges[*FinalRangeCount - 1].PhysicalSize;
      ReservedRangeToCompare->SmramReservedSize  -= FinalRanges[*FinalRangeCount - 1].PhysicalSize;
    }
  }
}

/**
  Returns if SMRAM range and SMRAM reserved range are overlapped.

  @param[in] RangeToCompare             Pointer to EFI_SMRAM_DESCRIPTOR to compare.
  @param[in] ReservedRangeToCompare     Pointer to EFI_SMM_RESERVED_SMRAM_REGION to compare.

  @retval TRUE  There is overlap.
  @retval FALSE There is no overlap.

**/
BOOLEAN
SmmIsSmramOverlap (
  IN EFI_SMRAM_DESCRIPTOR           *RangeToCompare,
  IN EFI_SMM_RESERVED_SMRAM_REGION  *ReservedRangeToCompare
  )
{
  UINT64    RangeToCompareEnd;
  UINT64    ReservedRangeToCompareEnd;

  RangeToCompareEnd         = RangeToCompare->CpuStart + RangeToCompare->PhysicalSize;
  ReservedRangeToCompareEnd = ReservedRangeToCompare->SmramReservedStart + ReservedRangeToCompare->SmramReservedSize;

  if ((RangeToCompare->CpuStart >= ReservedRangeToCompare->SmramReservedStart) &&
      (RangeToCompare->CpuStart < ReservedRangeToCompareEnd)) {
    return TRUE;
  } else if ((ReservedRangeToCompare->SmramReservedStart >= RangeToCompare->CpuStart) &&
             (ReservedRangeToCompare->SmramReservedStart < RangeToCompareEnd)) {
    return TRUE;
  }
  return FALSE;
}

/**
  Get full SMRAM ranges.

  It will get SMRAM ranges from SmmAccess protocol and SMRAM reserved ranges from
  SmmConfiguration protocol, split the entries if there is overlap between them.
  It will also reserve one entry for SMM core.

  @param[out] FullSmramRangeCount   Output pointer to full SMRAM range count.

  @return Pointer to full SMRAM ranges.

**/
EFI_SMRAM_DESCRIPTOR *
GetFullSmramRanges (
  OUT UINTN     *FullSmramRangeCount
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_CONFIGURATION_PROTOCOL    *SmmConfiguration;
  UINTN                             Size;
  UINTN                             Index;
  UINTN                             Index2;
  EFI_SMRAM_DESCRIPTOR              *FullSmramRanges;
  UINTN                             TempSmramRangeCount;
  UINTN                             AdditionSmramRangeCount;
  EFI_SMRAM_DESCRIPTOR              *TempSmramRanges;
  UINTN                             SmramRangeCount;
  EFI_SMRAM_DESCRIPTOR              *SmramRanges;
  UINTN                             SmramReservedCount;
  EFI_SMM_RESERVED_SMRAM_REGION     *SmramReservedRanges;
  UINTN                             MaxCount;
  BOOLEAN                           Rescan;

  //
  // Get SMM Configuration Protocol if it is present.
  //
  SmmConfiguration = NULL;
  Status = gBS->LocateProtocol (&gEfiSmmConfigurationProtocolGuid, NULL, (VOID **) &SmmConfiguration);

  //
  // Get SMRAM information.
  //
  Size = 0;
  Status = mSmmAccess->GetCapabilities (mSmmAccess, &Size, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  SmramRangeCount = Size / sizeof (EFI_SMRAM_DESCRIPTOR);

  //
  // Get SMRAM reserved region count.
  //
  SmramReservedCount = 0;
  if (SmmConfiguration != NULL) {
    while (SmmConfiguration->SmramReservedRegions[SmramReservedCount].SmramReservedSize != 0) {
      SmramReservedCount++;
    }
  }

  //
  // Reserve one entry for SMM Core in the full SMRAM ranges.
  //
  AdditionSmramRangeCount = 1;
  if (PcdGet64(PcdLoadModuleAtFixAddressEnable) != 0) {
    //
    // Reserve two entries for all SMM drivers and SMM Core in the full SMRAM ranges.
    //
    AdditionSmramRangeCount = 2;
  }

  if (SmramReservedCount == 0) {
    //
    // No reserved SMRAM entry from SMM Configuration Protocol.
    //
    *FullSmramRangeCount = SmramRangeCount + AdditionSmramRangeCount;
    Size = (*FullSmramRangeCount) * sizeof (EFI_SMRAM_DESCRIPTOR);
    FullSmramRanges = (EFI_SMRAM_DESCRIPTOR *) AllocateZeroPool (Size);
    ASSERT (FullSmramRanges != NULL);

    Status = mSmmAccess->GetCapabilities (mSmmAccess, &Size, FullSmramRanges);
    ASSERT_EFI_ERROR (Status);

    return FullSmramRanges;
  }

  //
  // Why MaxCount = X + 2 * Y?
  // Take Y = 1 as example below, Y > 1 case is just the iteration of Y = 1.
  //
  //   X = 1 Y = 1     MaxCount = 3 = 1 + 2 * 1
  //   ----            ----
  //   |  |  ----      |--|
  //   |  |  |  |  ->  |  |
  //   |  |  ----      |--|
  //   ----            ----
  //
  //   X = 2 Y = 1     MaxCount = 4 = 2 + 2 * 1
  //   ----            ----
  //   |  |            |  |
  //   |  |  ----      |--|
  //   |  |  |  |      |  |
  //   |--|  |  |  ->  |--|
  //   |  |  |  |      |  |
  //   |  |  ----      |--|
  //   |  |            |  |
  //   ----            ----
  //
  //   X = 3 Y = 1     MaxCount = 5 = 3 + 2 * 1
  //   ----            ----
  //   |  |            |  |
  //   |  |  ----      |--|
  //   |--|  |  |      |--|
  //   |  |  |  |  ->  |  |
  //   |--|  |  |      |--|
  //   |  |  ----      |--|
  //   |  |            |  |
  //   ----            ----
  //
  //   ......
  //
  MaxCount = SmramRangeCount + 2 * SmramReservedCount;

  Size = MaxCount * sizeof (EFI_SMM_RESERVED_SMRAM_REGION);
  SmramReservedRanges = (EFI_SMM_RESERVED_SMRAM_REGION *) AllocatePool (Size);
  ASSERT (SmramReservedRanges != NULL);
  for (Index = 0; Index < SmramReservedCount; Index++) {
    CopyMem (&SmramReservedRanges[Index], &SmmConfiguration->SmramReservedRegions[Index], sizeof (EFI_SMM_RESERVED_SMRAM_REGION));
  }

  Size = MaxCount * sizeof (EFI_SMRAM_DESCRIPTOR);
  TempSmramRanges = (EFI_SMRAM_DESCRIPTOR *) AllocatePool (Size);
  ASSERT (TempSmramRanges != NULL);
  TempSmramRangeCount = 0;

  SmramRanges = (EFI_SMRAM_DESCRIPTOR *) AllocatePool (Size);
  ASSERT (SmramRanges != NULL);
  Status = mSmmAccess->GetCapabilities (mSmmAccess, &Size, SmramRanges);
  ASSERT_EFI_ERROR (Status);

  do {
    Rescan = FALSE;
    for (Index = 0; (Index < SmramRangeCount) && !Rescan; Index++) {
      //
      // Skip zero size entry.
      //
      if (SmramRanges[Index].PhysicalSize != 0) {
        for (Index2 = 0; (Index2 < SmramReservedCount) && !Rescan; Index2++) {
          //
          // Skip zero size entry.
          //
          if (SmramReservedRanges[Index2].SmramReservedSize != 0) {
            if (SmmIsSmramOverlap (
                  &SmramRanges[Index],
                  &SmramReservedRanges[Index2]
                  )) {
              //
              // There is overlap, need to split entry and then rescan.
              //
              SmmSplitSmramEntry (
                &SmramRanges[Index],
                &SmramReservedRanges[Index2],
                SmramRanges,
                &SmramRangeCount,
                SmramReservedRanges,
                &SmramReservedCount,
                TempSmramRanges,
                &TempSmramRangeCount
                );
              Rescan = TRUE;
            }
          }
        }
        if (!Rescan) {
          //
          // No any overlap, copy the entry to the temp SMRAM ranges.
          // Zero SmramRanges[Index].PhysicalSize = 0;
          //
          CopyMem (&TempSmramRanges[TempSmramRangeCount++], &SmramRanges[Index], sizeof (EFI_SMRAM_DESCRIPTOR));
          SmramRanges[Index].PhysicalSize = 0;
        }
      }
    }
  } while (Rescan);
  ASSERT (TempSmramRangeCount <= MaxCount);

  //
  // Sort the entries
  //
  FullSmramRanges = AllocateZeroPool ((TempSmramRangeCount + AdditionSmramRangeCount) * sizeof (EFI_SMRAM_DESCRIPTOR));
  ASSERT (FullSmramRanges != NULL);
  *FullSmramRangeCount = 0;
  do {
    for (Index = 0; Index < TempSmramRangeCount; Index++) {
      if (TempSmramRanges[Index].PhysicalSize != 0) {
        break;
      }
    }
    ASSERT (Index < TempSmramRangeCount);
    for (Index2 = 0; Index2 < TempSmramRangeCount; Index2++) {
      if ((Index2 != Index) && (TempSmramRanges[Index2].PhysicalSize != 0) && (TempSmramRanges[Index2].CpuStart < TempSmramRanges[Index].CpuStart)) {
        Index = Index2;
      }
    }
    CopyMem (&FullSmramRanges[*FullSmramRangeCount], &TempSmramRanges[Index], sizeof (EFI_SMRAM_DESCRIPTOR));
    *FullSmramRangeCount += 1;
    TempSmramRanges[Index].PhysicalSize = 0;
  } while (*FullSmramRangeCount < TempSmramRangeCount);
  ASSERT (*FullSmramRangeCount == TempSmramRangeCount);
  *FullSmramRangeCount += AdditionSmramRangeCount;

  FreePool (SmramRanges);
  FreePool (SmramReservedRanges);
  FreePool (TempSmramRanges);

  return FullSmramRanges;
}

/**
  The Entry Point for SMM IPL

  Load SMM Core into SMRAM, register SMM Core entry point for SMIs, install
  SMM Base 2 Protocol and SMM Communication Protocol, and register for the
  critical events required to coordinate between DXE and SMM environments.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmIplEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  UINT64                          MaxSize;
  VOID                            *Registration;
  UINT64                          SmmCodeSize;
  EFI_CPU_ARCH_PROTOCOL           *CpuArch;
  EFI_STATUS                      SetAttrStatus;
  EFI_SMRAM_DESCRIPTOR            *SmramRangeSmmDriver;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR MemDesc;

  //
  // Fill in the image handle of the SMM IPL so the SMM Core can use this as the
  // ParentImageHandle field of the Load Image Protocol for all SMM Drivers loaded
  // by the SMM Core
  //
  mSmmCorePrivateData.SmmIplImageHandle = ImageHandle;

  //
  // Get SMM Access Protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **)&mSmmAccess);
  ASSERT_EFI_ERROR (Status);

  //
  // Get SMM Control2 Protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmControl2ProtocolGuid, NULL, (VOID **)&mSmmControl2);
  ASSERT_EFI_ERROR (Status);

  gSmmCorePrivate->SmramRanges = GetFullSmramRanges (&gSmmCorePrivate->SmramRangeCount);

  //
  // Open all SMRAM ranges
  //
  Status = mSmmAccess->Open (mSmmAccess);
  ASSERT_EFI_ERROR (Status);

  //
  // Print debug message that the SMRAM window is now open.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL opened SMRAM window\n"));

  //
  // Find the largest SMRAM range between 1MB and 4GB that is at least 256KB - 4K in size
  //
  mCurrentSmramRange = NULL;
  for (Index = 0, MaxSize = SIZE_256KB - EFI_PAGE_SIZE; Index < gSmmCorePrivate->SmramRangeCount; Index++) {
    //
    // Skip any SMRAM region that is already allocated, needs testing, or needs ECC initialization
    //
    if ((gSmmCorePrivate->SmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
      continue;
    }

    if (gSmmCorePrivate->SmramRanges[Index].CpuStart >= BASE_1MB) {
      if ((gSmmCorePrivate->SmramRanges[Index].CpuStart + gSmmCorePrivate->SmramRanges[Index].PhysicalSize - 1) <= MAX_ADDRESS) {
        if (gSmmCorePrivate->SmramRanges[Index].PhysicalSize >= MaxSize) {
          MaxSize = gSmmCorePrivate->SmramRanges[Index].PhysicalSize;
          mCurrentSmramRange = &gSmmCorePrivate->SmramRanges[Index];
        }
      }
    }
  }

  if (mCurrentSmramRange != NULL) {
    //
    // Print debug message showing SMRAM window that will be used by SMM IPL and SMM Core
    //
    DEBUG ((DEBUG_INFO, "SMM IPL found SMRAM window %p - %p\n",
      (VOID *)(UINTN)mCurrentSmramRange->CpuStart,
      (VOID *)(UINTN)(mCurrentSmramRange->CpuStart + mCurrentSmramRange->PhysicalSize - 1)
      ));

    GetSmramCacheRange (mCurrentSmramRange, &mSmramCacheBase, &mSmramCacheSize);
    //
    // Make sure we can change the desired memory attributes.
    //
    Status = gDS->GetMemorySpaceDescriptor (
                    mSmramCacheBase,
                    &MemDesc
                    );
    ASSERT_EFI_ERROR (Status);
    if ((MemDesc.Capabilities & SMRAM_CAPABILITIES) != SMRAM_CAPABILITIES) {
      gDS->SetMemorySpaceCapabilities (
             mSmramCacheBase,
             mSmramCacheSize,
             MemDesc.Capabilities | SMRAM_CAPABILITIES
             );
    }
    //
    // If CPU AP is present, attempt to set SMRAM cacheability to WB and clear
    // all paging attributes.
    // Note that it is expected that cacheability of SMRAM has been set to WB if CPU AP
    // is not available here.
    //
    CpuArch = NULL;
    Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&CpuArch);
    if (!EFI_ERROR (Status)) {
      MemDesc.Attributes &= ~(EFI_CACHE_ATTRIBUTE_MASK | EFI_MEMORY_ATTRIBUTE_MASK);
      MemDesc.Attributes |= EFI_MEMORY_WB;
      Status = gDS->SetMemorySpaceAttributes (
                      mSmramCacheBase,
                      mSmramCacheSize,
                      MemDesc.Attributes
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "SMM IPL failed to set SMRAM window to EFI_MEMORY_WB\n"));
      }

      DEBUG_CODE (
        gDS->GetMemorySpaceDescriptor (
               mSmramCacheBase,
               &MemDesc
               );
        DEBUG ((DEBUG_INFO, "SMRAM attributes: %016lx\n", MemDesc.Attributes));
        ASSERT ((MemDesc.Attributes & EFI_MEMORY_ATTRIBUTE_MASK) == 0);
      );
    }
    //
    // if Loading module at Fixed Address feature is enabled, save the SMRAM base to Load
    // Modules At Fixed Address Configuration Table.
    //
    if (PcdGet64(PcdLoadModuleAtFixAddressEnable) != 0) {
      //
      // Build tool will calculate the smm code size and then patch the PcdLoadFixAddressSmmCodePageNumber
      //
      SmmCodeSize = LShiftU64 (PcdGet32(PcdLoadFixAddressSmmCodePageNumber), EFI_PAGE_SHIFT);
      //
      // The SMRAM available memory is assumed to be larger than SmmCodeSize
      //
      ASSERT (mCurrentSmramRange->PhysicalSize > SmmCodeSize);
      //
      // Retrieve Load modules At fixed address configuration table and save the SMRAM base.
      //
      Status = EfiGetSystemConfigurationTable (
                &gLoadFixedAddressConfigurationTableGuid,
               (VOID **) &mLMFAConfigurationTable
               );
      if (!EFI_ERROR (Status) && mLMFAConfigurationTable != NULL) {
        mLMFAConfigurationTable->SmramBase = mCurrentSmramRange->CpuStart;
        //
        // Print the SMRAM base
        //
        DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO: TSEG BASE is %x. \n", mLMFAConfigurationTable->SmramBase));
      }

      //
      // Fill the Smram range for all SMM code
      //
      SmramRangeSmmDriver = &gSmmCorePrivate->SmramRanges[gSmmCorePrivate->SmramRangeCount - 2];
      SmramRangeSmmDriver->CpuStart      = mCurrentSmramRange->CpuStart;
      SmramRangeSmmDriver->PhysicalStart = mCurrentSmramRange->PhysicalStart;
      SmramRangeSmmDriver->RegionState   = mCurrentSmramRange->RegionState | EFI_ALLOCATED;
      SmramRangeSmmDriver->PhysicalSize  = SmmCodeSize;

      mCurrentSmramRange->PhysicalSize  -= SmmCodeSize;
      mCurrentSmramRange->CpuStart       = mCurrentSmramRange->CpuStart + SmmCodeSize;
      mCurrentSmramRange->PhysicalStart  = mCurrentSmramRange->PhysicalStart + SmmCodeSize;
    }
    //
    // Load SMM Core into SMRAM and execute it from SMRAM
    //
    Status = ExecuteSmmCoreFromSmram (
               mCurrentSmramRange,
               &gSmmCorePrivate->SmramRanges[gSmmCorePrivate->SmramRangeCount - 1],
               gSmmCorePrivate
               );
    if (EFI_ERROR (Status)) {
      //
      // Print error message that the SMM Core failed to be loaded and executed.
      //
      DEBUG ((DEBUG_ERROR, "SMM IPL could not load and execute SMM Core from SMRAM\n"));

      //
      // Attempt to reset SMRAM cacheability to UC
      //
      if (CpuArch != NULL) {
        SetAttrStatus = gDS->SetMemorySpaceAttributes(
                               mSmramCacheBase,
                               mSmramCacheSize,
                               EFI_MEMORY_UC
                               );
        if (EFI_ERROR (SetAttrStatus)) {
          DEBUG ((DEBUG_WARN, "SMM IPL failed to reset SMRAM window to EFI_MEMORY_UC\n"));
        }
      }
    }
  } else {
    //
    // Print error message that there are not enough SMRAM resources to load the SMM Core.
    //
    DEBUG ((DEBUG_ERROR, "SMM IPL could not find a large enough SMRAM region to load SMM Core\n"));
  }

  //
  // If the SMM Core could not be loaded then close SMRAM window, free allocated
  // resources, and return an error so SMM IPL will be unloaded.
  //
  if (mCurrentSmramRange == NULL || EFI_ERROR (Status)) {
    //
    // Close all SMRAM ranges
    //
    Status = mSmmAccess->Close (mSmmAccess);
    ASSERT_EFI_ERROR (Status);

    //
    // Print debug message that the SMRAM window is now closed.
    //
    DEBUG ((DEBUG_INFO, "SMM IPL closed SMRAM window\n"));

    //
    // Free all allocated resources
    //
    FreePool (gSmmCorePrivate->SmramRanges);

    return EFI_UNSUPPORTED;
  }

  //
  // Install SMM Base2 Protocol and SMM Communication Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSmmIplHandle,
                  &gEfiSmmBase2ProtocolGuid,         &mSmmBase2,
                  &gEfiSmmCommunicationProtocolGuid, &mSmmCommunication,
                  &gEfiMmCommunication2ProtocolGuid, &mMmCommunication2,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Create the set of protocol and event notifications that the SMM IPL requires
  //
  for (Index = 0; mSmmIplEvents[Index].NotifyFunction != NULL; Index++) {
    if (mSmmIplEvents[Index].Protocol) {
      mSmmIplEvents[Index].Event = EfiCreateProtocolNotifyEvent (
                                     mSmmIplEvents[Index].Guid,
                                     mSmmIplEvents[Index].NotifyTpl,
                                     mSmmIplEvents[Index].NotifyFunction,
                                     mSmmIplEvents[Index].NotifyContext,
                                    &Registration
                                    );
    } else {
      Status = gBS->CreateEventEx (
                      EVT_NOTIFY_SIGNAL,
                      mSmmIplEvents[Index].NotifyTpl,
                      mSmmIplEvents[Index].NotifyFunction,
                      mSmmIplEvents[Index].NotifyContext,
                      mSmmIplEvents[Index].Guid,
                      &mSmmIplEvents[Index].Event
                      );
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}
