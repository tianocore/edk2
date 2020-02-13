/** @file
  Supports Capsule Dependency Expression.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "FmpDxe.h"
#include "Dependency.h"

//
// Define the initial size of the dependency expression evaluation stack
//
#define DEPEX_STACK_SIZE_INCREMENT  0x1000

//
// Type of stack element
//
typedef enum {
  BooleanType,
  VersionType
} ELEMENT_TYPE;

//
// Value of stack element
//
typedef union {
  BOOLEAN   Boolean;
  UINT32    Version;
} ELEMENT_VALUE;

//
// Stack element used to evaluate dependency expressions
//
typedef struct {
  ELEMENT_VALUE Value;
  ELEMENT_TYPE  Type;
} DEPEX_ELEMENT;

//
// Global variable used to support dependency evaluation
//
UINTN                          mNumberOfFmpInstance = 0;
EFI_FIRMWARE_IMAGE_DESCRIPTOR  **mFmpImageInfoBuf   = NULL;

//
// Indicates the status of dependency check, default value is DEPENDENCIES_SATISFIED.
//
UINT8  mDependenciesCheckStatus = DEPENDENCIES_SATISFIED;

//
// Global stack used to evaluate dependency expressions
//
DEPEX_ELEMENT  *mDepexEvaluationStack        = NULL;
DEPEX_ELEMENT  *mDepexEvaluationStackEnd     = NULL;
DEPEX_ELEMENT  *mDepexEvaluationStackPointer = NULL;

/**
  Grow size of the Depex stack

  @retval EFI_SUCCESS           Stack successfully growed.
  @retval EFI_OUT_OF_RESOURCES  There is not enough system memory to grow the stack.

**/
EFI_STATUS
GrowDepexStack (
  VOID
  )
{
  DEPEX_ELEMENT  *NewStack;
  UINTN          Size;

  Size = DEPEX_STACK_SIZE_INCREMENT;
  if (mDepexEvaluationStack != NULL) {
    Size = Size + (mDepexEvaluationStackEnd - mDepexEvaluationStack);
  }

  NewStack = AllocatePool (Size * sizeof (DEPEX_ELEMENT));
  if (NewStack == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (mDepexEvaluationStack != NULL) {
    //
    // Copy to Old Stack to the New Stack
    //
    CopyMem (
      NewStack,
      mDepexEvaluationStack,
      (mDepexEvaluationStackEnd - mDepexEvaluationStack) * sizeof (DEPEX_ELEMENT)
      );

    //
    // Free The Old Stack
    //
    FreePool (mDepexEvaluationStack);
  }

  //
  // Make the Stack pointer point to the old data in the new stack
  //
  mDepexEvaluationStackPointer = NewStack + (mDepexEvaluationStackPointer - mDepexEvaluationStack);
  mDepexEvaluationStack        = NewStack;
  mDepexEvaluationStackEnd     = NewStack + Size;

  return EFI_SUCCESS;
}

/**
  Push an element onto the Stack.

  @param[in]  Value                  Value to push.
  @param[in]  Type                   Element Type

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.
  @retval EFI_INVALID_PARAMETER  Wrong stack element type.

**/
EFI_STATUS
Push (
  IN UINT32   Value,
  IN UINTN    Type
  )
{
  EFI_STATUS      Status;
  DEPEX_ELEMENT   Element;

  //
  // Check Type
  //
  if (Type != BooleanType && Type != VersionType) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for a stack overflow condition
  //
  if (mDepexEvaluationStackPointer == mDepexEvaluationStackEnd) {
    //
    // Grow the stack
    //
    Status = GrowDepexStack ();
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Element.Value.Version = Value;
  Element.Type = Type;

  //
  // Push the item onto the stack
  //
  *mDepexEvaluationStackPointer = Element;
  mDepexEvaluationStackPointer++;

  return EFI_SUCCESS;
}


/**
  Pop an element from the stack.

  @param[out]  Element                Element to pop.
  @param[in]   Type                   Type of element.

  @retval EFI_SUCCESS            The value was popped onto the stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack.
  @retval EFI_INVALID_PARAMETER  Type is mismatched.

**/
EFI_STATUS
Pop (
  OUT DEPEX_ELEMENT  *Element,
  IN  ELEMENT_TYPE   Type
  )
{
  //
  // Check for a stack underflow condition
  //
  if (mDepexEvaluationStackPointer == mDepexEvaluationStack) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Pop the item off the stack
  //
  mDepexEvaluationStackPointer--;
  *Element = *mDepexEvaluationStackPointer;
  if ((*Element).Type != Type) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

/**
  Evaluate the dependencies.

  @param[in]   Dependencies        Dependency expressions.
  @param[in]   DependenciesSize    Size of Dependency expressions.

  @retval TRUE           Dependency expressions evaluate to TRUE.
  @retval FALSE          Dependency expressions evaluate to FALSE.

**/
BOOLEAN
EvaluateDependencies (
  IN  CONST EFI_FIRMWARE_IMAGE_DEP *     Dependencies,
  IN  CONST UINTN                        DependenciesSize
  )
{
  EFI_STATUS                        Status;
  UINT8                             *Iterator;
  UINT8                             Index;
  DEPEX_ELEMENT                     Element1;
  DEPEX_ELEMENT                     Element2;
  GUID                              ImageTypeId;
  UINT32                            Version;

  if (Dependencies == NULL || DependenciesSize == 0) {
    return FALSE;
  }

  //
  // Clean out memory leaks in Depex Boolean stack. Leaks are only caused by
  // incorrectly formed DEPEX expressions
  //
  mDepexEvaluationStackPointer = mDepexEvaluationStack;

  Iterator = (UINT8 *) Dependencies->Dependencies;
  while (Iterator < (UINT8 *) Dependencies->Dependencies + DependenciesSize) {
    switch (*Iterator)
    {
    case EFI_FMP_DEP_PUSH_GUID:
      if (Iterator + sizeof (EFI_GUID) >= (UINT8 *) Dependencies->Dependencies + DependenciesSize) {
        Status = EFI_INVALID_PARAMETER;
        goto Error;
      }

      CopyGuid (&ImageTypeId, (EFI_GUID *) (Iterator + 1));
      Iterator = Iterator + sizeof (EFI_GUID);

      for (Index = 0; Index < mNumberOfFmpInstance; Index ++){
        if (mFmpImageInfoBuf[Index] == NULL) {
          continue;
        }
        if(CompareGuid (&mFmpImageInfoBuf[Index]->ImageTypeId, &ImageTypeId)){
          Status = Push (mFmpImageInfoBuf[Index]->Version, VersionType);
          if (EFI_ERROR (Status)) {
            goto Error;
          }
          break;
        }
      }
      if (Index == mNumberOfFmpInstance) {
        Status = EFI_NOT_FOUND;
        goto Error;
      }
      break;
    case EFI_FMP_DEP_PUSH_VERSION:
      if (Iterator + sizeof (UINT32) >= (UINT8 *) Dependencies->Dependencies + DependenciesSize ) {
        Status = EFI_INVALID_PARAMETER;
        goto Error;
      }

      Version = *(UINT32 *) (Iterator + 1);
      Status = Push (Version, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Iterator = Iterator + sizeof (UINT32);
      break;
    case EFI_FMP_DEP_VERSION_STR:
      Iterator += AsciiStrnLenS ((CHAR8 *) Iterator, DependenciesSize - (Iterator - Dependencies->Dependencies));
      break;
    case EFI_FMP_DEP_AND:
      Status = Pop (&Element1, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Pop (&Element2, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Push (Element1.Value.Boolean & Element2.Value.Boolean, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_OR:
      Status = Pop (&Element1, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Pop(&Element2, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Push (Element1.Value.Boolean | Element2.Value.Boolean, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_NOT:
      Status = Pop (&Element1, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Push (!(Element1.Value.Boolean), BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_TRUE:
      Status = Push (TRUE, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_FALSE:
      Status = Push (FALSE, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_EQ:
      Status = Pop (&Element1, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Pop (&Element2, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = (Element1.Value.Version == Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_GT:
      Status = Pop (&Element1, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Pop (&Element2, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = (Element1.Value.Version >  Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_GTE:
      Status = Pop (&Element1, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Pop (&Element2, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = (Element1.Value.Version >= Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_LT:
      Status = Pop (&Element1, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Pop (&Element2, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = (Element1.Value.Version <  Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_LTE:
      Status = Pop (&Element1, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = Pop (&Element2, VersionType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      Status = (Element1.Value.Version <= Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      break;
    case EFI_FMP_DEP_END:
      Status = Pop (&Element1, BooleanType);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      return Element1.Value.Boolean;
    default:
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }
    Iterator++;
  }

Error:

  DEBUG ((DEBUG_ERROR, "FmpDxe(%s): EvaluateDependencies() - RESULT = FALSE (Status = %r)\n", mImageIdName, Status));
  return FALSE;
}

/**
  Validate the dependency expression and output its size.

  @param[in]   ImageDepex      Pointer to the EFI_FIRMWARE_IMAGE_DEP.
  @param[in]   MaxDepexSize    Max size of the dependency.
  @param[out]  DepexSize       Size of dependency.

  @retval TRUE           The capsule is valid.
  @retval FALSE          The capsule is invalid.

**/
BOOLEAN
ValidateImageDepex (
  IN  EFI_FIRMWARE_IMAGE_DEP             *ImageDepex,
  IN  CONST UINTN                        MaxDepexSize,
  OUT UINT32                             *DepexSize
  )
{
  UINT8  *Depex;

  *DepexSize = 0;
  Depex = ImageDepex->Dependencies;
  while (Depex < ImageDepex->Dependencies + MaxDepexSize) {
    switch (*Depex)
    {
    case EFI_FMP_DEP_PUSH_GUID:
      Depex += sizeof (EFI_GUID) + 1;
      break;
    case EFI_FMP_DEP_PUSH_VERSION:
      Depex += sizeof (UINT32) + 1;
      break;
    case EFI_FMP_DEP_VERSION_STR:
      Depex += AsciiStrnLenS ((CHAR8 *) Depex, ImageDepex->Dependencies + MaxDepexSize - Depex) + 1;
      break;
    case EFI_FMP_DEP_AND:
    case EFI_FMP_DEP_OR:
    case EFI_FMP_DEP_NOT:
    case EFI_FMP_DEP_TRUE:
    case EFI_FMP_DEP_FALSE:
    case EFI_FMP_DEP_EQ:
    case EFI_FMP_DEP_GT:
    case EFI_FMP_DEP_GTE:
    case EFI_FMP_DEP_LT:
    case EFI_FMP_DEP_LTE:
      Depex += 1;
      break;
    case EFI_FMP_DEP_END:
      Depex += 1;
      *DepexSize = (UINT32)(Depex - ImageDepex->Dependencies);
      return TRUE;
    default:
      return FALSE;
    }
  }

  return FALSE;
}


/**
  Get the size of dependencies. Assume the dependencies is validated before
  calling this function.

  @param[in]   Dependencies    Pointer to the EFI_FIRMWARE_IMAGE_DEP.

  @retval  The size of dependencies.

**/
UINTN
GetDepexSize (
  IN CONST EFI_FIRMWARE_IMAGE_DEP  *Dependencies
  )
{
  UINTN Index;

  if (Dependencies == NULL) {
    return 0;
  }

  Index = 0;
  while (Dependencies->Dependencies[Index] != EFI_FMP_DEP_END) {
    Index ++;
  }

  return Index + 1;
}

/**
  Check dependency for firmware update.

  @param[in]   ImageTypeId         Image Type Id.
  @param[in]   Version             New version.
  @param[in]   Dependencies        The dependencies.
  @param[in]   DependenciesSize    Size of the dependencies
  @param[out]  IsSatisfied         Indicate the dependencies is satisfied or not.

  @retval  EFI_SUCCESS             Dependency Evaluation is successful.
  @retval  Others                  Dependency Evaluation fails with unexpected error.

**/
EFI_STATUS
EvaluateImageDependencies (
  IN CONST EFI_GUID                ImageTypeId,
  IN CONST UINT32                  Version,
  IN CONST EFI_FIRMWARE_IMAGE_DEP  *Dependencies,
  IN CONST UINT32                  DependenciesSize,
  OUT BOOLEAN                      *IsSatisfied
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             Index;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  UINTN                             ImageInfoSize;
  UINT32                            FmpImageInfoDescriptorVer;
  UINT8                             FmpImageInfoCount;
  UINTN                             DescriptorSize;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;
  UINTN                             DepexSize;

  *IsSatisfied       = TRUE;
  PackageVersionName = NULL;

  //
  // Get ImageDescriptors of all FMP instances, and archive them for depex evaluation.
  //
  Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiFirmwareManagementProtocolGuid,
                NULL,
                &mNumberOfFmpInstance,
                &HandleBuffer
                );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  mFmpImageInfoBuf = AllocatePool (sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR *) * mNumberOfFmpInstance);
  if (mFmpImageInfoBuf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < mNumberOfFmpInstance; Index ++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **) &Fmp
                    );
    if (EFI_ERROR(Status)) {
      continue;
    }

    ImageInfoSize = 0;
    Status = Fmp->GetImageInfo (
                    Fmp,
                    &ImageInfoSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
    if (Status != EFI_BUFFER_TOO_SMALL) {
      continue;
    }

    mFmpImageInfoBuf[Index] = AllocateZeroPool (ImageInfoSize);
    if (mFmpImageInfoBuf[Index] == NULL) {
      continue;
    }

    Status = Fmp->GetImageInfo (
                    Fmp,
                    &ImageInfoSize,               // ImageInfoSize
                    mFmpImageInfoBuf[Index],      // ImageInfo
                    &FmpImageInfoDescriptorVer,   // DescriptorVersion
                    &FmpImageInfoCount,           // DescriptorCount
                    &DescriptorSize,              // DescriptorSize
                    &PackageVersion,              // PackageVersion
                    &PackageVersionName           // PackageVersionName
                    );
    if (EFI_ERROR(Status)) {
      FreePool (mFmpImageInfoBuf[Index]);
      mFmpImageInfoBuf[Index] = NULL;
      continue;
    }

    if (PackageVersionName != NULL) {
      FreePool (PackageVersionName);
      PackageVersionName = NULL;
    }
  }

  //
  // Step 1 - Evaluate firmware image's depex, against the version of other Fmp instances.
  //
  if (Dependencies != NULL) {
    *IsSatisfied = EvaluateDependencies (Dependencies, DependenciesSize);
  }

  if (!*IsSatisfied) {
    goto cleanup;
  }

  //
  // Step 2 - Evaluate the depex of all other Fmp instances, against the new version in
  // the firmware image.
  //

  //
  // Update the new version to mFmpImageInfoBuf.
  //
  for (Index = 0; Index < mNumberOfFmpInstance; Index ++) {
    if (mFmpImageInfoBuf[Index] != NULL) {
      if (CompareGuid (&ImageTypeId, &mFmpImageInfoBuf[Index]->ImageTypeId)) {
        mFmpImageInfoBuf[Index]->Version = Version;
        break;
      }
    }
  }

  //
  // Evaluate the Dependencies one by one.
  //
  for (Index = 0; Index < mNumberOfFmpInstance; Index ++) {
    if (mFmpImageInfoBuf[Index] != NULL) {
      //
      // Skip the Fmp instance to be "SetImage".
      //
      if (CompareGuid (&ImageTypeId, &mFmpImageInfoBuf[Index]->ImageTypeId)) {
        continue;
      }
      if ((mFmpImageInfoBuf[Index]->AttributesSupported & IMAGE_ATTRIBUTE_DEPENDENCY) &&
           mFmpImageInfoBuf[Index]->Dependencies != NULL) {
        //
        // Get the size of depex.
        // Assume that the dependencies in EFI_FIRMWARE_IMAGE_DESCRIPTOR is validated when PopulateDescriptor().
        //
        DepexSize = GetDepexSize (mFmpImageInfoBuf[Index]->Dependencies);
        if (DepexSize > 0) {
          *IsSatisfied = EvaluateDependencies (mFmpImageInfoBuf[Index]->Dependencies, DepexSize);
          if (!*IsSatisfied) {
            break;
          }
        }
      }
    }
  }

cleanup:
  if (mFmpImageInfoBuf != NULL) {
    for (Index = 0; Index < mNumberOfFmpInstance; Index ++) {
      if (mFmpImageInfoBuf[Index] != NULL) {
        FreePool (mFmpImageInfoBuf[Index]);
      }
    }
    FreePool (mFmpImageInfoBuf);
  }

  return EFI_SUCCESS;
}
