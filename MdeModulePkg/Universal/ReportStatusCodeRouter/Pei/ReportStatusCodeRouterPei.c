/** @file
  Report Status Code Router PEIM which produces Report Stataus Code Handler PPI and Status Code PPI.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ReportStatusCodeRouterPei.h"

EFI_PEI_RSC_HANDLER_PPI     mRscHandlerPpi = {
  Register,
  Unregister
  };

EFI_PEI_PROGRESS_CODE_PPI     mStatusCodePpi = {
  ReportDispatcher
  };

EFI_PEI_PPI_DESCRIPTOR   mRscHandlerPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiRscHandlerPpiGuid,
    &mRscHandlerPpi
  }
};

EFI_PEI_PPI_DESCRIPTOR   mStatusCodePpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiStatusCodePpiGuid,
    &mStatusCodePpi
  }
};

/**
  Worker function to create one memory status code GUID'ed HOB,
  using PacketIndex to identify the packet.

  @param   PacketIndex    Index of records packet.

  @return  Pointer to the memory status code packet.

**/
UINTN *
CreateRscHandlerCallbackPacket (
  VOID
  )
{
  UINTN  *NumberOfEntries;

  //
  // Build GUID'ed HOB with PCD defined size.
  //
  NumberOfEntries = BuildGuidHob (
                      &gStatusCodeCallbackGuid,
                      sizeof (EFI_PEI_RSC_HANDLER_CALLBACK) * 64 + sizeof (UINTN)
                      );
  ASSERT (NumberOfEntries != NULL);

  *NumberOfEntries = 0;

  return NumberOfEntries;
}

/**
  Register the callback function for ReportStatusCode() notification.
  
  When this function is called the function pointer is added to an internal list and any future calls to
  ReportStatusCode() will be forwarded to the Callback function.

  @param[in] Callback           A pointer to a function of type EFI_PEI_RSC_HANDLER_CALLBACK that is called
                                when a call to ReportStatusCode() occurs.
                        
  @retval EFI_SUCCESS           Function was successfully registered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_OUT_OF_RESOURCES  The internal buffer ran out of space. No more functions can be
                                registered.
  @retval EFI_ALREADY_STARTED   The function was already registered. It can't be registered again.
  
**/
EFI_STATUS
EFIAPI
Register (
  IN EFI_PEI_RSC_HANDLER_CALLBACK Callback
  )
{
  EFI_PEI_HOB_POINTERS          Hob;
  EFI_PEI_RSC_HANDLER_CALLBACK  *CallbackEntry;
  UINTN                         *NumberOfEntries;
  UINTN                         Index;
  UINTN                         FreeEntryIndex;
  UINTN                         *FreePacket;

  if (Callback == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Hob.Raw        = GetFirstGuidHob (&gStatusCodeCallbackGuid);
  FreePacket     = NULL;
  FreeEntryIndex = 0;
  while (Hob.Raw != NULL) {
    NumberOfEntries = GET_GUID_HOB_DATA (Hob);
    CallbackEntry   = (EFI_PEI_RSC_HANDLER_CALLBACK *) (NumberOfEntries + 1);
    if (FreePacket == NULL && *NumberOfEntries < 64) {
      //
      // If current total number of handlers does not exceed 64, put new handler
      // at the last of packet
      //
      FreePacket = NumberOfEntries;
      FreeEntryIndex = *NumberOfEntries;
    }
    for (Index = 0; Index < *NumberOfEntries; Index++) {
      if (CallbackEntry[Index] == Callback) {
        //
        // If the function was already registered. It can't be registered again.
        //
        return EFI_ALREADY_STARTED;
      }
      if (FreePacket == NULL && CallbackEntry[Index] == NULL) {
        //
        // If the total number of handlers in current packet is max value 64,
        // search an entry with NULL pointer and fill new handler into this entry.
        //  
        FreePacket = NumberOfEntries;
        FreeEntryIndex = Index;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextGuidHob (&gStatusCodeCallbackGuid, Hob.Raw);
  }

  if (FreePacket == NULL) {
    FreePacket = CreateRscHandlerCallbackPacket();
  }

  CallbackEntry = (EFI_PEI_RSC_HANDLER_CALLBACK *) (FreePacket + 1);
  CallbackEntry[FreeEntryIndex] = Callback;
  
  if (*FreePacket == FreeEntryIndex) {
    //
    // If new registered callback is added as a new entry in the packet,
    // increase the total number of handlers in the packet.
    //
    *FreePacket += 1;
  }

  return EFI_SUCCESS;
}

/**
  Remove a previously registered callback function from the notification list.
  
  ReportStatusCode() messages will no longer be forwarded to the Callback function.
  
  @param[in] Callback           A pointer to a function of type EFI_PEI_RSC_HANDLER_CALLBACK that is to be
                                unregistered.

  @retval EFI_SUCCESS           The function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_NOT_FOUND         The callback function was not found to be unregistered.
                        
**/
EFI_STATUS
EFIAPI
Unregister (
  IN EFI_PEI_RSC_HANDLER_CALLBACK Callback
  )
{
  EFI_PEI_HOB_POINTERS            Hob;
  EFI_PEI_RSC_HANDLER_CALLBACK    *CallbackEntry;
  UINTN                           *NumberOfEntries;
  UINTN                           Index;

  if (Callback == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Hob.Raw  = GetFirstGuidHob (&gStatusCodeCallbackGuid);
  while (Hob.Raw != NULL) {
    NumberOfEntries = GET_GUID_HOB_DATA (Hob);
    CallbackEntry   = (EFI_PEI_RSC_HANDLER_CALLBACK *) (NumberOfEntries + 1);
    for (Index = 0; Index < *NumberOfEntries; Index++) {
      if (CallbackEntry[Index] == Callback) {
        //
        // Set removed entry as NULL.
        //
        CallbackEntry[Index] = NULL;
        return EFI_SUCCESS;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextGuidHob (&gStatusCodeCallbackGuid, Hob.Raw);
  }

  return EFI_NOT_FOUND;
}

/**
  Publishes an interface that allows PEIMs to report status codes.

  This function implements EFI_PEI_PROGRESS_CODE_PPI.ReportStatusCode().
  It publishes an interface that allows PEIMs to report status codes.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or
                           software entity. This includes information about the class and
                           subclass that is used to classify the entity as well as an operation.
                           For progress codes, the operation is the current activity.
                           For error codes, it is the exception.For debug codes,it is not defined at this time.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. A system may contain multiple entities that match a class/subclass
                           pairing. The instance differentiates between them. An instance of 0 indicates
                           that instance information is unavailable, not meaningful, or not relevant.
                           Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS      The function completed successfully.

**/
EFI_STATUS
EFIAPI
ReportDispatcher (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 *CallerId OPTIONAL,
  IN CONST EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
{
  EFI_PEI_HOB_POINTERS            Hob;
  EFI_PEI_RSC_HANDLER_CALLBACK    *CallbackEntry;
  UINTN                           *NumberOfEntries;
  UINTN                           Index;

  Hob.Raw  = GetFirstGuidHob (&gStatusCodeCallbackGuid);
  while (Hob.Raw != NULL) {
    NumberOfEntries = GET_GUID_HOB_DATA (Hob);
    CallbackEntry   = (EFI_PEI_RSC_HANDLER_CALLBACK *) (NumberOfEntries + 1);
    for (Index = 0; Index < *NumberOfEntries; Index++) {
      if (CallbackEntry[Index] != NULL) {
      CallbackEntry[Index](
        PeiServices,
        CodeType,
        Value,
        Instance,
        CallerId,
        Data
        );
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextGuidHob (&gStatusCodeCallbackGuid, Hob.Raw);
  }

  return EFI_SUCCESS;
}

/**
  Entry point of Status Code PEIM.
  
  This function is the entry point of this Status Code Router PEIM.
  It produces Report Stataus Code Handler PPI and Status Code PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point of DXE IPL PEIM executes successfully.

**/
EFI_STATUS
EFIAPI
GenericStatusCodePeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_PPI_DESCRIPTOR     *OldDescriptor;
  EFI_PEI_PROGRESS_CODE_PPI  *OldStatusCodePpi;

  CreateRscHandlerCallbackPacket ();

  //
  // Install Report Status Code Handler PPI
  //
  Status = PeiServicesInstallPpi (mRscHandlerPpiList);
  ASSERT_EFI_ERROR (Status);

  //
  // Install Status Code PPI. PI spec specifies that there can be only one instance
  // of this PPI in system. So first check if other instance already exists.
  // If no other instance exists, then just install the PPI.
  // If other instance already exists, then reinstall it.
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiStatusCodePpiGuid,
             0,
             &OldDescriptor,
             (VOID **) &OldStatusCodePpi
             );
  if (!EFI_ERROR (Status)) {
    Status = PeiServicesReInstallPpi (OldDescriptor, mStatusCodePpiList);
  } else {
    Status = PeiServicesInstallPpi (mStatusCodePpiList);
  }
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
