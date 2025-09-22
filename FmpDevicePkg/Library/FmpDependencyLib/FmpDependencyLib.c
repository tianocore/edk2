/** @file
  Supports Fmp Capsule Dependency Expression.

  Copyright (c) Microsoft Corporation.<BR>
  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FmpDependencyLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/SystemResourceTable.h>
#include <LastAttemptStatus.h>
#include <FmpLastAttemptStatus.h>

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
  BOOLEAN    Boolean;
  UINT32     Version;
} ELEMENT_VALUE;

//
// Stack element used to evaluate dependency expressions
//
typedef struct {
  ELEMENT_VALUE    Value;
  ELEMENT_TYPE     Type;
} DEPEX_ELEMENT;

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
    DEBUG ((DEBUG_ERROR, "GrowDepexStack: Cannot allocate memory for dependency evaluation stack!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if (mDepexEvaluationStack != NULL) {
    //
    // Copy from Old Stack to the New Stack
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
  IN UINT32  Value,
  IN UINTN   Type
  )
{
  EFI_STATUS     Status;
  DEPEX_ELEMENT  Element;

  //
  // Check Type
  //
  if ((Type != BooleanType) && (Type != VersionType)) {
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
  Element.Type          = Type;

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
    DEBUG ((DEBUG_ERROR, "EvaluateDependency: Stack underflow!\n"));
    return EFI_ACCESS_DENIED;
  }

  //
  // Pop the item off the stack
  //
  mDepexEvaluationStackPointer--;
  *Element = *mDepexEvaluationStackPointer;
  if ((*Element).Type != Type) {
    DEBUG ((DEBUG_ERROR, "EvaluateDependency: Popped element type is mismatched!\n"));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Evaluate the dependencies. The caller must search all the Fmp instances and
  gather their versions into FmpVersions parameter. If there is PUSH_GUID opcode
  in dependency expression with no FmpVersions provided, the dependency will
  evaluate to FALSE.

  @param[in]   Dependencies       Dependency expressions.
  @param[in]   DependenciesSize   Size of Dependency expressions.
  @param[in]   FmpVersions        Array of Fmp ImageTypeId and version. This
                                  parameter is optional and can be set to NULL.
  @param[in]   FmpVersionsCount   Element count of the array. When FmpVersions
                                  is NULL, FmpVersionsCount must be 0.
  @param[out]  LastAttemptStatus  An optional pointer to a UINT32 that holds the
                                  last attempt status to report back to the caller.
                                  This function will set the value to LAST_ATTEMPT_STATUS_SUCCESS
                                  if an error code is not set.

  @retval TRUE    Dependency expressions evaluate to TRUE.
  @retval FALSE   Dependency expressions evaluate to FALSE.

**/
BOOLEAN
EFIAPI
EvaluateDependency (
  IN  EFI_FIRMWARE_IMAGE_DEP        *Dependencies,
  IN  UINTN                         DependenciesSize,
  IN  FMP_DEPEX_CHECK_VERSION_DATA  *FmpVersions       OPTIONAL,
  IN  UINTN                         FmpVersionsCount,
  OUT UINT32                        *LastAttemptStatus OPTIONAL
  )
{
  EFI_STATUS     Status;
  UINT8          *Iterator;
  UINTN          Index;
  DEPEX_ELEMENT  Element1;
  DEPEX_ELEMENT  Element2;
  GUID           ImageTypeId;
  UINT32         Version;
  UINT32         LocalLastAttemptStatus;
  UINT32         DeclaredLength;

  LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;

  //
  // Check if parameter is valid.
  //
  if ((Dependencies == NULL) || (DependenciesSize == 0)) {
    return FALSE;
  }

  if ((FmpVersions == NULL) && (FmpVersionsCount > 0)) {
    return FALSE;
  }

  //
  // Clean out memory leaks in Depex Boolean stack. Leaks are only caused by
  // incorrectly formed DEPEX expressions
  //
  mDepexEvaluationStackPointer = mDepexEvaluationStack;

  Iterator = (UINT8 *)Dependencies->Dependencies;
  while (Iterator < (UINT8 *)Dependencies->Dependencies + DependenciesSize) {
    switch (*Iterator) {
      case EFI_FMP_DEP_PUSH_GUID:
        if (Iterator + sizeof (EFI_GUID) >= (UINT8 *)Dependencies->Dependencies + DependenciesSize) {
          DEBUG ((DEBUG_ERROR, "EvaluateDependency: GUID extends beyond end of dependency expression!\n"));
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_GUID_BEYOND_DEPEX;
          goto Error;
        }

        CopyGuid (&ImageTypeId, (EFI_GUID *)(Iterator + 1));
        Iterator = Iterator + sizeof (EFI_GUID);

        for (Index = 0; Index < FmpVersionsCount; Index++) {
          if (CompareGuid (&FmpVersions[Index].ImageTypeId, &ImageTypeId)) {
            Status = Push (FmpVersions[Index].Version, VersionType);
            if (EFI_ERROR (Status)) {
              LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
              goto Error;
            }

            break;
          }
        }

        if (Index == FmpVersionsCount) {
          DEBUG ((DEBUG_ERROR, "EvaluateDependency: %g is not found!\n", &ImageTypeId));
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_FMP_NOT_FOUND;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_PUSH_VERSION:
        if (Iterator + sizeof (UINT32) >= (UINT8 *)Dependencies->Dependencies + DependenciesSize ) {
          DEBUG ((DEBUG_ERROR, "EvaluateDependency: VERSION extends beyond end of dependency expression!\n"));
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_VERSION_BEYOND_DEPEX;
          goto Error;
        }

        Version = *(UINT32 *)(Iterator + 1);
        Status  = Push (Version, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        Iterator = Iterator + sizeof (UINT32);
        break;
      case EFI_FMP_DEP_VERSION_STR:
        Iterator += AsciiStrnLenS ((CHAR8 *)Iterator, DependenciesSize - (Iterator - Dependencies->Dependencies));
        if (Iterator == (UINT8 *)Dependencies->Dependencies + DependenciesSize) {
          DEBUG ((DEBUG_ERROR, "EvaluateDependency: STRING extends beyond end of dependency expression!\n"));
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_VERSION_STR_BEYOND_DEPEX;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_AND:
        Status = Pop (&Element1, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Pop (&Element2, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Push (Element1.Value.Boolean & Element2.Value.Boolean, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_OR:
        Status = Pop (&Element1, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Pop (&Element2, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Push (Element1.Value.Boolean | Element2.Value.Boolean, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_NOT:
        Status = Pop (&Element1, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Push (!(Element1.Value.Boolean), BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_TRUE:
        Status = Push (TRUE, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_FALSE:
        Status = Push (FALSE, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_EQ:
        Status = Pop (&Element1, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Pop (&Element2, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = (Element1.Value.Version == Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_GT:
        Status = Pop (&Element1, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Pop (&Element2, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = (Element1.Value.Version >  Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_GTE:
        Status = Pop (&Element1, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Pop (&Element2, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = (Element1.Value.Version >= Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_LT:
        Status = Pop (&Element1, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Pop (&Element2, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = (Element1.Value.Version <  Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_LTE:
        Status = Pop (&Element1, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = Pop (&Element2, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        Status = (Element1.Value.Version <= Element2.Value.Version) ? Push (TRUE, BooleanType) : Push (FALSE, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        break;
      case EFI_FMP_DEP_END:
        Status = Pop (&Element1, BooleanType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_POP_FAILURE;
          goto Error;
        }

        return Element1.Value.Boolean;
      case EFI_FMP_DEP_DECLARE_LENGTH:
        if (Iterator + sizeof (UINT32) >= (UINT8 *)Dependencies->Dependencies + DependenciesSize ) {
          DEBUG ((DEBUG_ERROR, "EvaluateDependency: DECLARE_LENGTH extends beyond end of dependency expression!\n"));
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_DECLARE_LENGTH_BEYOND_DEPEX;
          goto Error;
        }

        //
        // This opcode must be the first one in a dependency expression.
        //
        if (Iterator != Dependencies->Dependencies) {
          DEBUG ((DEBUG_ERROR, "EvaluateDependency: DECLARE_LENGTH is not the first opcode!\n"));
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_DECLARE_LENGTH_NOT_FIRST_OPCODE;
          goto Error;
        }

        DeclaredLength = *(UINT32 *)(Iterator + 1);
        if (DeclaredLength != DependenciesSize) {
          DEBUG ((DEBUG_ERROR, "EvaluateDependency: DECLARE_LENGTH is not equal to length of dependency expression!\n"));
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_DECLARE_LENGTH_INCORRECT;
          goto Error;
        }

        Status = Push (DeclaredLength, VersionType);
        if (EFI_ERROR (Status)) {
          LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_PUSH_FAILURE;
          goto Error;
        }

        Iterator = Iterator + sizeof (UINT32);
        break;
      default:
        DEBUG ((DEBUG_ERROR, "EvaluateDependency: Unknown Opcode - %02x!\n", *Iterator));
        LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_UNKNOWN_OPCODE;
        goto Error;
    }

    Iterator++;
  }

  DEBUG ((DEBUG_ERROR, "EvaluateDependency: No EFI_FMP_DEP_END Opcode in expression!\n"));
  LocalLastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_NO_END_OPCODE;

Error:
  if (LastAttemptStatus != NULL) {
    *LastAttemptStatus = LocalLastAttemptStatus;
  }

  return FALSE;
}

/**
  Validate the dependency expression and output its size.

  @param[in]   Dependencies       Pointer to the EFI_FIRMWARE_IMAGE_DEP.
  @param[in]   MaxDepexSize       Max size of the dependency.
  @param[out]  DepexSize          Size of dependency.
  @param[out]  LastAttemptStatus  An optional pointer to a UINT32 that holds the
                                  last attempt status to report back to the caller.
                                  If a last attempt status error code is not returned,
                                  this function will not modify the LastAttemptStatus value.

  @retval TRUE    The dependency expression is valid.
  @retval FALSE   The dependency expression is invalid.

**/
BOOLEAN
EFIAPI
ValidateDependency (
  IN  EFI_FIRMWARE_IMAGE_DEP  *Dependencies,
  IN  UINTN                   MaxDepexSize,
  OUT UINT32                  *DepexSize,
  OUT UINT32                  *LastAttemptStatus OPTIONAL
  )
{
  UINT8  *Depex;

  if (DepexSize != NULL) {
    *DepexSize = 0;
  }

  if (Dependencies == NULL) {
    return FALSE;
  }

  Depex = Dependencies->Dependencies;
  while (Depex < Dependencies->Dependencies + MaxDepexSize) {
    switch (*Depex) {
      case EFI_FMP_DEP_PUSH_GUID:
        Depex += sizeof (EFI_GUID) + 1;
        break;
      case EFI_FMP_DEP_PUSH_VERSION:
        Depex += sizeof (UINT32) + 1;
        break;
      case EFI_FMP_DEP_VERSION_STR:
        Depex += AsciiStrnLenS ((CHAR8 *)Depex, Dependencies->Dependencies + MaxDepexSize - Depex) + 1;
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
        if (DepexSize != NULL) {
          *DepexSize = (UINT32)(Depex - Dependencies->Dependencies);
        }

        return TRUE;
      case EFI_FMP_DEP_DECLARE_LENGTH:
        Depex += sizeof (UINT32) + 1;
        break;
      default:
        return FALSE;
    }
  }

  if (LastAttemptStatus != NULL) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_NO_END_OPCODE;
  }

  return FALSE;
}

/**
  Get dependency from firmware image.

  @param[in]  Image               Points to the firmware image.
  @param[in]  ImageSize           Size, in bytes, of the firmware image.
  @param[out] DepexSize           Size, in bytes, of the dependency.
  @param[out] LastAttemptStatus   An optional pointer to a UINT32 that holds the
                                  last attempt status to report back to the caller.
                                  If a last attempt status error code is not returned,
                                  this function will not modify the LastAttemptStatus value.
  @retval  The pointer to dependency.
  @retval  Null

**/
EFI_FIRMWARE_IMAGE_DEP *
EFIAPI
GetImageDependency (
  IN  EFI_FIRMWARE_IMAGE_AUTHENTICATION  *Image,
  IN  UINTN                              ImageSize,
  OUT UINT32                             *DepexSize,
  OUT UINT32                             *LastAttemptStatus  OPTIONAL
  )
{
  EFI_FIRMWARE_IMAGE_DEP  *Depex;
  UINTN                   MaxDepexSize;

  if (Image == NULL) {
    return NULL;
  }

  //
  // Check to make sure that operation can be safely performed.
  //
  if ((((UINTN)Image + sizeof (Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength) < (UINTN)Image) || \
      (((UINTN)Image + sizeof (Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength) >= (UINTN)Image + ImageSize))
  {
    //
    // Pointer overflow. Invalid image.
    //
    if (LastAttemptStatus != NULL) {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_DEPENDENCY_LIB_ERROR_GET_DEPEX_FAILURE;
    }

    return NULL;
  }

  Depex        = (EFI_FIRMWARE_IMAGE_DEP *)((UINT8 *)Image + sizeof (Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength);
  MaxDepexSize = ImageSize - (sizeof (Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength);

  //
  // Validate the dependency and get the size of dependency
  //
  if (ValidateDependency (Depex, MaxDepexSize, DepexSize, LastAttemptStatus)) {
    return Depex;
  }

  return NULL;
}
