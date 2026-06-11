/** @file
  Hii Forms Builder.

  Copyright (c) 2026, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <HiiFormsGenerator.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <Uefi/UefiInternalFormRepresentation.h>

STATIC LIST_ENTRY  mHiiHandleList = INITIALIZE_LIST_HEAD_VARIABLE (mHiiHandleList);

/** This macro expands to a function that retrieves the Hii Forms
    list from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceHiiForms,
  EHiiFormsObjList,
  CM_HII_FORMS_OBJ_INFO
  )

/** A helper function to invoke a Hii Form generator

  This is a helper function that invokes the Hii Form generator interface
  for dynamically generating an HII Form.

  @param [in]  FormsFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  HiiFormsInfo         Pointer to the Hii Forms Info.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         Required object is not found.
  @retval EFI_BAD_BUFFER_SIZE   Size returned by the Configuration Manager
                                is less than the Object size for the
                                requested object.
**/
static
EFI_STATUS
EFIAPI
BuildHiiForm (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  FormsFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN CONST CM_HII_FORMS_OBJ_INFO                 *CONST  HiiFormsInfo
  )
{
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  HII_FORM_HANDLE         *FormHandle;
  HII_FORMS_GENERATOR     *Generator;
  HII_FORMS_GENERATOR_ID  HiiFormGeneratorId;

  ASSERT (FormsFactoryProtocol != NULL);
  ASSERT (CfgMgrProtocol != NULL);

  HiiFormGeneratorId = HiiFormsInfo != NULL ? HiiFormsInfo->FormGeneratorId : 0;

  DEBUG ((
    DEBUG_INFO,
    " HiiFormGeneratorId = 0x%x\n",
    HiiFormGeneratorId
    ));

  Generator = NULL;
  Status    = FormsFactoryProtocol->GetHiiFormsGenerator (
                                      FormsFactoryProtocol,
                                      HiiFormGeneratorId,
                                      &Generator
                                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Hii Forms Generator not found." \
      " HiiFormGeneratorId = 0x%x. Status = %r\n",
      HiiFormGeneratorId,
      Status
      ));
    return Status;
  }

  if (Generator == NULL) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: Generator found : %s\n",
    Generator->Description
    ));

  FormHandle = AllocatePool (sizeof (*FormHandle));
  if (FormHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Generator->BuildHiiForm (
                        Generator,
                        HiiFormsInfo,
                        CfgMgrProtocol,
                        HiiFormGeneratorId == 0 ? &mHiiHandleList : NULL,
                        &FormHandle->HiiHandle
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Build the Hii Form." \
      " HiiFormGeneratorId = 0x%x. Status = %r\n",
      HiiFormGeneratorId,
      Status
      ));
    goto err_handler;
  }

  if (HiiFormGeneratorId != EStdHiiFormsIdPlatCfg) {
    InsertTailList (&mHiiHandleList, &FormHandle->List);
  } else {
    FreePool (FormHandle);
  }

  return EFI_SUCCESS;

err_handler:
  // Free any resources allocated for generating the Form.
  if (Generator->FreeHiiFormResources != NULL) {
    Status1 = Generator->FreeHiiFormResources (
                           Generator,
                           HiiFormsInfo,
                           &FormHandle->HiiHandle
                           );
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to Free Table Resources." \
        "HiiFormGeneratorId = 0x%x. Status = %r\n",
        HiiFormGeneratorId,
        Status1
        ));
    }

    // Return the first error status in case of failure
    if (!EFI_ERROR (Status)) {
      Status = Status1;
    }
  }

  FreePool (FormHandle);

  return Status;
}

/** Generate Dynamic HII Forms.

  The function gathers the information necessary for generating the
  Dynamic HII Forms by invoking the generators.

  @param [in]  FormsFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.

  @retval EFI_SUCCESS           Success.
  @retval EFI_NOT_FOUND         If a generator is not found.
  @retval EFI_ALREADY_STARTED   If a generator is already installed.
**/
static
EFI_STATUS
EFIAPI
ProcessHiiForms (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  FormsFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol
  )
{
  EFI_STATUS              Status;
  CM_HII_FORMS_OBJ_INFO   *HiiFormsInfo;
  UINT32                  HiiFormsCount;
  UINT32                  Idx;
  HII_FORMS_GENERATOR_ID  HiiFormGeneratorId;

  ASSERT (FormsFactoryProtocol != NULL);
  ASSERT (CfgMgrProtocol != NULL);

  Status = GetEHiiFormsObjList (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &HiiFormsInfo,
             &HiiFormsCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Hii Forms List. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (HiiFormsCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: EHiiFormsObjList: Got zero HiiFormsCount\n"
      ));
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: EHiiFormsObjList: HiiFormsCount = %d\n",
    HiiFormsCount
    ));

  for (Idx = 0; Idx < HiiFormsCount; Idx++) {
    HiiFormGeneratorId = HiiFormsInfo[Idx].FormGeneratorId;

    DEBUG ((
      DEBUG_INFO,
      "INFO: HiiFormsInfo[%d].FormGeneratorId = 0x%x\n",
      Idx,
      HiiFormGeneratorId
      ));

    if (!IS_VALID_STD_HII_FORMS_GENERATOR_ID (HiiFormGeneratorId)) {
      DEBUG ((
        DEBUG_WARN,
        "WARNING: Invalid Hii Forms Generator ID = 0x%x, Skipping...\n",
        HiiFormGeneratorId
        ));
      continue;
    }

    Status = BuildHiiForm (
               FormsFactoryProtocol,
               CfgMgrProtocol,
               &HiiFormsInfo[Idx]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to build Hii Form."
        " Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  // Add the Platform Configuration Top Level Form at last
  Status = BuildHiiForm (
             FormsFactoryProtocol,
             CfgMgrProtocol,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to build Platform Configuration Hii Form."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  return Status;
}

/** Free up the HII Handles from the list

  Iterate through the list of HII Handles that have been
  added, and free up the memory associated with these
  handles.

**/
static
VOID
FreeHiiHandleList (
  VOID
  )
{
  HII_FORM_HANDLE  *Entry;
  HII_FORM_HANDLE  *NextEntry;

  Entry = (HII_FORM_HANDLE *)GetFirstNode (&mHiiHandleList);
  while (!IsNull (&mHiiHandleList, &Entry->List)) {
    NextEntry = (HII_FORM_HANDLE *)GetNextNode (&mHiiHandleList, &Entry->List);

    if (Entry->HiiHandle != NULL) {
      HiiRemovePackages (Entry->HiiHandle);
    }

    RemoveEntryList (&Entry->List);
    FreePool (Entry);

    Entry = NextEntry;
  }
}

/** Dynamic Hii Forms Process function.

  This event notification indicates that the Configuration Manager protocol
  is ready. Therefore, dispatch the generation of HII Forms by getting
  information from the Configuration Manager.

  @param  [in]  Event     The Event that is signalled.
  @param  [in]  Context   The Context information.

**/
VOID
EFIAPI
DynamicHiiFormsProcess (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                             Status;
  EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CfgMgrProtocol;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO  *CfgMfrInfo;
  EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *FormsFactoryProtocol;

  // Locate the Dynamic Table Factory
  Status = gBS->LocateProtocol (
                  &gEdkiiDynamicTableFactoryProtocolGuid,
                  NULL,
                  (VOID **)&FormsFactoryProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to find Dynamic Table Factory protocol." \
      " Status = %r\n",
      Status
      ));
    return;
  }

  // Locate the Configuration Manager for the Platform
  Status = gBS->LocateProtocol (
                  &gEdkiiConfigurationManagerProtocolGuid,
                  NULL,
                  (VOID **)&CfgMgrProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to find Configuration Manager protocol. Status = %r\n",
      Status
      ));
    return;
  }

  Status = GetCgfMgrInfo (CfgMgrProtocol, &CfgMfrInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Configuration Manager info. Status = %r\n",
      Status
      ));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: Configuration Manager Version = 0x%x, OemID = %c%c%c%c%c%c\n",
    CfgMfrInfo->Revision,
    CfgMfrInfo->OemId[0],
    CfgMfrInfo->OemId[1],
    CfgMfrInfo->OemId[2],
    CfgMfrInfo->OemId[3],
    CfgMfrInfo->OemId[4],
    CfgMfrInfo->OemId[5]
    ));

  Status = ProcessHiiForms (FormsFactoryProtocol, CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Dynamic Hii Forms processing failure. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    goto exit;
  }

  Status = gBS->CloseEvent (Event);
  ASSERT_EFI_ERROR (Status);

  return;

exit:
  FreeHiiHandleList ();
}
