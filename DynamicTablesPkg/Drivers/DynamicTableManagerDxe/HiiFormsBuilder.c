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

#include "HiiFormsBuilder.h"

static LIST_ENTRY  mHiiHandleList = INITIALIZE_LIST_HEAD_VARIABLE (mHiiHandleList);
static LIST_ENTRY  mFormInfoList  = INITIALIZE_LIST_HEAD_VARIABLE (mFormInfoList);

/** This macro expands to a function that retrieves the Hii Forms
    list from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceStandard,
  EStdObjHiiFormsList,
  CM_STD_OBJ_HII_FORMS_INFO
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
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST      FormsFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST      CfgMgrProtocol,
  IN CONST CM_STD_OBJ_HII_FORMS_INFO                 *CONST  HiiFormsInfo
  )
{
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  HII_FORM_INFO           *FormInfo;
  HII_FORM_HANDLE         *FormHandle;
  HII_FORMS_GENERATOR     *Generator;
  HII_FORMS_GENERATOR_ID  HiiFormGeneratorId;

  ASSERT (FormsFactoryProtocol != NULL);
  ASSERT (CfgMgrProtocol != NULL);

  HiiFormGeneratorId = HiiFormsInfo != NULL ?
                       HiiFormsInfo->FormGeneratorId : EStdHiiFormsIdRootForm;

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

  FormHandle = AllocateZeroPool (sizeof (*FormHandle));
  if (FormHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FormInfo = AllocateZeroPool (sizeof (*FormInfo));
  if (FormInfo == NULL) {
    FreePool (FormHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Generator->BuildHiiForm (
                        Generator,
                        HiiFormsInfo,
                        CfgMgrProtocol,
                        HiiFormGeneratorId == EStdHiiFormsIdRootForm ?
                        &mHiiHandleList : NULL,
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

  FormInfo->Generator   = Generator;
  FormInfo->HiiHandle   = FormHandle->HiiHandle;
  FormInfo->HiiFormInfo = HiiFormsInfo;
  InsertTailList (&mFormInfoList, &FormInfo->List);

  if (HiiFormGeneratorId != EStdHiiFormsIdRootForm) {
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
  FreePool (FormInfo);

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
  UINT32                     HiiFormsCount;
  UINT32                     Idx;
  EFI_STATUS                 Status;
  HII_FORMS_GENERATOR_ID     HiiFormGeneratorId;
  CM_STD_OBJ_HII_FORMS_INFO  *HiiFormsInfo;

  ASSERT (FormsFactoryProtocol != NULL);
  ASSERT (CfgMgrProtocol != NULL);

  Status = GetEStdObjHiiFormsList (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &HiiFormsInfo,
             &HiiFormsCount
             );
  if ((EFI_ERROR (Status)) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Hii Forms List. Status = %r\n",
      Status
      ));
    return Status;
  } else if ((Status == EFI_NOT_FOUND) || (HiiFormsCount == 0)) {
    DEBUG ((
      DEBUG_INFO,
      "EHiiFormsObjList: Got zero HiiFormsCount\n"
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

    if (!IS_VALID_HII_FORMS_GENERATOR_ID (HiiFormGeneratorId)) {
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

  // Add the Top Level Root Form at last
  Status = BuildHiiForm (
             FormsFactoryProtocol,
             CfgMgrProtocol,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to build the Root Hii Form."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  return Status;
}

/** Free up the HII form resources

  Iterate through the list of HII forms that have been
  added, and invoke the forms' FreeHiiFormResources()
  function, which will free up any allocated form
  resources, along with the corresponding HII handle.

**/
static
VOID
FreeHiiFormInfoList (
  VOID
  )
{
  LIST_ENTRY                       *Link;
  LIST_ENTRY                       *Next;
  EFI_STATUS                       Status;
  HII_FORM_INFO                    *Info;
  HII_FORMS_GENERATOR              *Generator;
  CONST CM_STD_OBJ_HII_FORMS_INFO  *HiiFormsInfo;

  Link = GetFirstNode (&mFormInfoList);
  while (!IsNull (&mFormInfoList, Link)) {
    Next = GetNextNode (&mFormInfoList, Link);
    Info = BASE_CR (Link, HII_FORM_INFO, List);

    Generator    = Info->Generator;
    HiiFormsInfo = Info->HiiFormInfo;
    if (Generator->FreeHiiFormResources != NULL) {
      Status = Generator->FreeHiiFormResources (
                            Generator,
                            HiiFormsInfo,
                            &Info->HiiHandle
                            );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: Failed to Free Table Resources." \
          "HiiFormGeneratorId = 0x%x. Status = %r\n",
          Generator->GeneratorID,
          Status
          ));
      }
    } else if (Info->HiiHandle != NULL) {
      HiiRemovePackages (Info->HiiHandle);
    }

    RemoveEntryList (&Info->List);
    FreePool (Info);

    Link = Next;
  }
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
  LIST_ENTRY       *Link;
  LIST_ENTRY       *Next;
  HII_FORM_HANDLE  *Entry;

  Link = GetFirstNode (&mHiiHandleList);
  while (!IsNull (&mHiiHandleList, Link)) {
    Next  = GetNextNode (&mHiiHandleList, Link);
    Entry = BASE_CR (Link, HII_FORM_HANDLE, List);

    RemoveEntryList (&Entry->List);
    FreePool (Entry);

    Link = Next;
  }
}

/** Free up all form resources

  Free up the form resources by iterating the HII_FORM_INFO
  and HII_FORM_HANDLE lists.

**/
VOID
EFIAPI
FreeHiiFormResources (
  VOID
  )
{
  FreeHiiFormInfoList ();
  FreeHiiHandleList ();
}

/** Dynamic Hii Forms Process function.

  Dispatch the generation of HII Forms by getting
  information from the Configuration Manager.

**/
VOID
EFIAPI
DynamicHiiFormsProcess (
  VOID
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
  if ((EFI_ERROR (Status)) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Dynamic Hii Forms processing failure. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    goto exit;
  }

  return;

exit:
  FreeHiiFormResources ();
}
