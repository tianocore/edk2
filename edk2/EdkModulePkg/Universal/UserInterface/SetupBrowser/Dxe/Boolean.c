/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Boolean.c

Abstract:

  This routine will evaluate the IFR inconsistency data to determine if
  something is a valid entry for a particular expression

--*/

#include "Setup.h"
#include "Ui.h"

//
// Global stack used to evaluate boolean expresions
//
BOOLEAN *mBooleanEvaluationStack    = (BOOLEAN) 0;
BOOLEAN *mBooleanEvaluationStackEnd = (BOOLEAN) 0;

STATIC
VOID
GrowBooleanStack (
  IN OUT BOOLEAN  **Stack,
  IN     UINTN    StackSizeInBoolean
  )
/*++

Routine Description:

  Grow size of the boolean stack

Arguments:

  Stack     - Old stack on the way in and new stack on the way out

  StackSizeInBoolean - New size of the stack

Returns:

  NONE

--*/
{
  BOOLEAN *NewStack;

  NewStack = AllocatePool (StackSizeInBoolean * sizeof (BOOLEAN));
  ASSERT (NewStack != NULL);

  if (*Stack != NULL) {
    //
    // Copy to Old Stack to the New Stack
    //
    CopyMem (
      NewStack,
      mBooleanEvaluationStack,
      (mBooleanEvaluationStackEnd - mBooleanEvaluationStack) * sizeof (BOOLEAN)
      );

    //
    // Make the Stack pointer point to the old data in the new stack
    //
    *Stack = NewStack + (*Stack - mBooleanEvaluationStack);

    //
    // Free The Old Stack
    //
    FreePool (mBooleanEvaluationStack);
  }

  mBooleanEvaluationStack     = NewStack;
  mBooleanEvaluationStackEnd  = NewStack + StackSizeInBoolean;
}

STATIC
VOID
InitializeBooleanEvaluator (
  VOID
  )
/*++

Routine Description:

  Allocate a global stack for boolean processing.

Arguments:

  NONE

Returns:

  NONE

--*/
{
  BOOLEAN *NullStack;

  NullStack = NULL;
  GrowBooleanStack (&NullStack, 0x1000);
}

STATIC
VOID
PushBool (
  IN OUT BOOLEAN  **Stack,
  IN BOOLEAN      BoolResult
  )
/*++

Routine Description:

  Push an element onto the Boolean Stack

Arguments:

  Stack      - Current stack location.
  BoolResult - BOOLEAN to push.

Returns:

  None.

--*/
{
  CopyMem (*Stack, &BoolResult, sizeof (BOOLEAN));
  *Stack += 1;

  if (*Stack >= mBooleanEvaluationStackEnd) {
    //
    // If we run out of stack space make a new one that is 2X as big. Copy
    // the old data into the new stack and update Stack to point to the old
    // data in the new stack.
    //
    GrowBooleanStack (
      Stack,
      (mBooleanEvaluationStackEnd - mBooleanEvaluationStack) * sizeof (BOOLEAN) * 2
      );
  }
}

STATIC
BOOLEAN
PopBool (
  IN OUT BOOLEAN **Stack
  )
/*++

Routine Description:

  Pop an element from the Boolean stack.

Arguments:

  Stack - Current stack location

Returns:

  Top of the BOOLEAN stack.

--*/
{
  BOOLEAN ReturnValue;

  *Stack -= 1;
  CopyMem (&ReturnValue, *Stack, sizeof (BOOLEAN));
  return ReturnValue;
}

STATIC
EFI_STATUS
GrowBooleanExpression (
  IN      EFI_INCONSISTENCY_DATA  *InconsistentTags,
  OUT     VOID                    **BooleanExpression,
  IN OUT  UINTN                   *BooleanExpressionLength
  )
{
  UINT8 *NewExpression;

  NewExpression = AllocatePool (*BooleanExpressionLength + sizeof (EFI_INCONSISTENCY_DATA));
  ASSERT (NewExpression != NULL);

  if (*BooleanExpression != NULL) {
    //
    // Copy Old buffer to the New buffer
    //
    CopyMem (NewExpression, *BooleanExpression, *BooleanExpressionLength);

    CopyMem (&NewExpression[*BooleanExpressionLength], InconsistentTags, sizeof (EFI_INCONSISTENCY_DATA));

    //
    // Free The Old buffer
    //
    FreePool (*BooleanExpression);
  } else {
    //
    // Copy data into new buffer
    //
    CopyMem (NewExpression, InconsistentTags, sizeof (EFI_INCONSISTENCY_DATA));
  }

  *BooleanExpressionLength  = *BooleanExpressionLength + sizeof (EFI_INCONSISTENCY_DATA);
  *BooleanExpression        = (VOID *) NewExpression;
  return EFI_SUCCESS;
}

STATIC
VOID
CreateBooleanExpression (
  IN  EFI_FILE_FORM_TAGS    *FileFormTags,
  IN  UINT16                Value,
  IN  UINT16                Id,
  IN  BOOLEAN               Complex,
  OUT VOID                  **BooleanExpression,
  OUT UINTN                 *BooleanExpressionLength
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  UINTN                   Count;
  EFI_INCONSISTENCY_DATA  *InconsistentTags;
  EFI_INCONSISTENCY_DATA  FakeInconsistentTags;

  InconsistentTags = FileFormTags->InconsistentTags;

  //
  // Did we run into a question that contains the Id we are looking for?
  //
  for (Count = 0; InconsistentTags->Operand != 0xFF; Count++) {

    //
    // Reserve INVALID_OFFSET_VALUE - 1 for TURE and FALSE, because we need to treat them as well
    // as ideqid etc. but they have no coresponding id, so we reserve this value.
    //
    if (InconsistentTags->QuestionId1 == Id ||
        InconsistentTags->QuestionId1 == INVALID_OFFSET_VALUE - 1) {
      //
      // If !Complex - means evaluate a single if/endif expression
      //
      if (!Complex) {
        //
        // If the ConsistencyId does not match the expression we are looking for
        // skip to the next consistency database entry
        //
        if (InconsistentTags->ConsistencyId != Value) {
          goto NextEntry;
        }
      }
      //
      // We need to rewind to the beginning of the Inconsistent expression
      //
      for (;
           (InconsistentTags->Operand != EFI_IFR_INCONSISTENT_IF_OP) &&
             (InconsistentTags->Operand != EFI_IFR_GRAYOUT_IF_OP) &&
             (InconsistentTags->Operand != EFI_IFR_SUPPRESS_IF_OP);
              ) {
        InconsistentTags = InconsistentTags->Previous;
      }
      //
      // Store the consistency check expression, ensure the next for loop starts at the op-code afterwards
      //
      GrowBooleanExpression (InconsistentTags, BooleanExpression, BooleanExpressionLength);
      InconsistentTags = InconsistentTags->Next;

      //
      // Keep growing until we hit the End expression op-code or we hit the beginning of another
      // consistency check like grayout/suppress
      //
      for (;
           InconsistentTags->Operand != EFI_IFR_END_IF_OP &&
           InconsistentTags->Operand != EFI_IFR_GRAYOUT_IF_OP &&
           InconsistentTags->Operand != EFI_IFR_SUPPRESS_IF_OP;
            ) {
        GrowBooleanExpression (InconsistentTags, BooleanExpression, BooleanExpressionLength);
        InconsistentTags = InconsistentTags->Next;
      }
      //
      // Store the EndExpression Op-code
      //
      GrowBooleanExpression (InconsistentTags, BooleanExpression, BooleanExpressionLength);
    }

NextEntry:
    if (InconsistentTags->Next != NULL) {
      //
      // Skip to next entry
      //
      InconsistentTags = InconsistentTags->Next;
    }
  }

  FakeInconsistentTags.Operand = 0;

  //
  // Add one last expression which will signify we have definitely hit the end
  //
  GrowBooleanExpression (&FakeInconsistentTags, BooleanExpression, BooleanExpressionLength);
}

STATIC
EFI_STATUS
BooleanVariableWorker (
  IN     CHAR16                   *VariableName,
  IN     EFI_VARIABLE_DEFINITION  *VariableDefinition,
  IN     BOOLEAN                  *StackPtr,
  IN OUT UINTN                    *SizeOfVariable,
  IN OUT VOID                     **VariableData
  )
/*++

Routine Description:


Arguments:

Returns:

--*/
{
  EFI_STATUS  Status;

  Status = gRT->GetVariable (
                  VariableName,
                  &VariableDefinition->Guid,
                  NULL,
                  SizeOfVariable,
                  *VariableData
                  );

  if (EFI_ERROR (Status)) {

    if (Status == EFI_BUFFER_TOO_SMALL) {
      *VariableData = AllocatePool (*SizeOfVariable);
      ASSERT (*VariableData != NULL);

      Status = gRT->GetVariable (
                      VariableName,
                      &VariableDefinition->Guid,
                      NULL,
                      SizeOfVariable,
                      *VariableData
                      );
    }

    if (Status == EFI_NOT_FOUND) {
      //
      // This is a serious flaw, we must have some standard result if a variable
      // is not found.  Our default behavior must either be return a TRUE or FALSE
      // since there is nothing else we can really do.  Therefore, my crystal ball
      // says I will return a FALSE
      //
      PushBool (&StackPtr, FALSE);
    }
  }

  return Status;
}

STATIC
UINT8
PredicateIfrType (
  IN  EFI_INCONSISTENCY_DATA      *Iterator
  )
/*++

Routine Description:
  This routine is for the purpose of predicate whether the Ifr is generated by a VfrCompiler greater than or equal to 1.88 or
  less than 1.88 which is legacy.

Arguments:
  Iterator    - The pointer to inconsistency tags

Returns:

  0x2         - If IFR is not legacy

  0x1         - If IFR is legacy

--*/
{
  //
  // legacy Ifr cover the states:
  // Not ...
  // Operand Opcode Operand
  //
  // while Operand means ideqval, TRUE, or other what can be evaluated to True or False,
  // and Opcode means AND or OR.
  //
  if (Iterator->Operand == EFI_IFR_NOT_OP   ||
      Iterator->Operand == 0) {
    return 0x1;
  } else if (Iterator->Operand == EFI_IFR_EQ_VAR_VAL_OP ||
             Iterator->Operand == EFI_IFR_EQ_ID_VAL_OP  ||
             Iterator->Operand == EFI_IFR_EQ_ID_ID_OP   ||
             Iterator->Operand == EFI_IFR_EQ_ID_LIST_OP) {
    Iterator++;
    if (Iterator->Operand == EFI_IFR_AND_OP ||
        Iterator->Operand == EFI_IFR_OR_OP) {
      Iterator--;
      return 0x1;
    }
    Iterator--;
  }
  return 0x2;
}

STATIC
VOID
PostOrderEvaluate (
  IN      EFI_FILE_FORM_TAGS          *FileFormTags,
  IN      UINT16                      Width,
  IN OUT  EFI_INCONSISTENCY_DATA      **PIterator,
  IN OUT  BOOLEAN                     **StackPtr
  )
/*++

Routine Description:
  PostOrderEvaluate is used for Ifr generated by VfrCompiler greater than or equal to 1.88,
  which generate Operand Operand Opcode type Ifr.
  PostOrderEvaluete only evaluate boolean expression part, not suppressif/grayoutif. TRUE,
  FALSE, >=, >, (, ) are supported.

Arguments:

  FileFormTags     - The pointer to the tags of the form

  Width            - Width of Operand, recognized every iteration

  PIterator        - The pointer to inconsistency tags

  StackPtr         - The pointer to the evaluation stack

Returns:

  TRUE             - If value is valid

  FALSE            - If value is not valid

--*/
{
  BOOLEAN                 Operator;
  BOOLEAN                 Operator2;
  UINT16                  *MapBuffer;
  UINT16                  *MapBuffer2;
  UINT16                  MapValue;
  UINT16                  MapValue2;
  UINTN                   SizeOfVariable;
  CHAR16                  VariableName[MAXIMUM_VALUE_CHARACTERS];
  VOID                    *VariableData;
  EFI_VARIABLE_DEFINITION *VariableDefinition;
  EFI_STATUS              Status;
  UINTN                   Index;
  BOOLEAN                 PushValue;

  Operator        = FALSE;
  Operator2       = FALSE;
  MapBuffer       = NULL;
  MapBuffer2      = NULL;
  MapValue        = 0;
  MapValue2       = 0;
  VariableData    = NULL;

  while (TRUE) {
    if ((*PIterator)->Operand == 0) {
      return;
    }

    Width = (*PIterator)->Width;

    //
    //  Because INVALID_OFFSET_VALUE - 1 is reserved for TRUE or FALSE, omit them.
    //
    if ((*PIterator)->QuestionId1 != INVALID_OFFSET_VALUE &&
        (*PIterator)->QuestionId1 != INVALID_OFFSET_VALUE - 1) {
      ExtractNvValue (FileFormTags, (*PIterator)->VariableNumber, Width, (*PIterator)->QuestionId1, (VOID **) &MapBuffer);
      ExtractNvValue (FileFormTags, (*PIterator)->VariableNumber2, Width, (*PIterator)->QuestionId2, (VOID **) &MapBuffer2);
      if (MapBuffer != NULL) {
        if (Width == 2) {
          MapValue = *MapBuffer;
        } else {
          MapValue = (UINT8) *MapBuffer;
        }

        FreePool (MapBuffer);
      }

      if (MapBuffer2 != NULL) {
        if (Width == 2) {
          MapValue2 = *MapBuffer2;
        } else {
          MapValue2 = (UINT8) *MapBuffer2;
        }

        FreePool (MapBuffer2);
      }
    }

    switch ((*PIterator)->Operand) {
    case EFI_IFR_EQ_VAR_VAL_OP:
      UnicodeValueToString (
        VariableName,
        FALSE,
        (UINTN) (*PIterator)->QuestionId1,
        (sizeof (VariableName) / sizeof (VariableName[0])) - 1
        );

      SizeOfVariable = 0;

      ExtractRequestedNvMap (FileFormTags, (*PIterator)->VariableNumber, &VariableDefinition);

      Status = BooleanVariableWorker (
                VariableName,
                VariableDefinition,
                *StackPtr,
                &SizeOfVariable,
                &VariableData
                );

      if (!EFI_ERROR (Status)) {
        if (SizeOfVariable == 1) {
          CopyMem (&MapValue, VariableData, 1);
        } else {
          CopyMem (&MapValue, VariableData, 2);
        }

        //
        // Do operation after knowing the compare operator.
        //
        MapValue2 = (*PIterator)->Value;
        (*PIterator)++;
        if ((*PIterator)->Operand == EFI_IFR_GT_OP) {
          PushValue = (BOOLEAN) (MapValue > MapValue2);
        } else if ((*PIterator)->Operand == EFI_IFR_GE_OP) {
          PushValue = (BOOLEAN) (MapValue >= MapValue2);
        } else {
          (*PIterator)--;
          PushValue = (BOOLEAN) (MapValue == MapValue2);
        }
        PushBool (StackPtr, PushValue);
      }

      break;

    case EFI_IFR_EQ_ID_VAL_OP:
      //
      // Do operation after knowing the compare operator.
      //
      MapValue2 = (*PIterator)->Value;
      (*PIterator)++;
      if ((*PIterator)->Operand == EFI_IFR_GT_OP) {
        PushValue = (BOOLEAN) (MapValue > MapValue2);
      } else if ((*PIterator)->Operand == EFI_IFR_GE_OP) {
        PushValue = (BOOLEAN) (MapValue >= MapValue2);
      } else {
        (*PIterator)--;
        PushValue = (BOOLEAN) (MapValue == MapValue2);
      }
      PushBool (StackPtr, PushValue);
      break;

    case EFI_IFR_EQ_ID_ID_OP:
      //
      // Do operation after knowing the compare operator.
      //
      (*PIterator)++;
      if ((*PIterator)->Operand == EFI_IFR_GT_OP) {
        PushValue = (BOOLEAN) (MapValue > MapValue2);
      } else if ((*PIterator)->Operand == EFI_IFR_GE_OP) {
        PushValue = (BOOLEAN) (MapValue >= MapValue2);
      } else {
        (*PIterator)--;
        PushValue = (BOOLEAN) (MapValue == MapValue2);
      }
      PushBool (StackPtr, PushValue);
      break;

    case EFI_IFR_EQ_ID_LIST_OP:
      for (Index = 0; Index < (*PIterator)->ListLength; Index++) {
        Operator = (BOOLEAN) (MapValue == (*PIterator)->ValueList[Index]);
        if (Operator) {
          break;
        }
      }

      PushBool (StackPtr, Operator);
      break;

    case EFI_IFR_TRUE_OP:
      PushBool (StackPtr, TRUE);
      break;

    case EFI_IFR_FALSE_OP:
      PushBool (StackPtr, FALSE);
      break;

    case EFI_IFR_AND_OP:
      Operator  = PopBool (StackPtr);
      Operator2 = PopBool (StackPtr);
      PushBool (StackPtr, (BOOLEAN) (Operator && Operator2));
      break;
    case EFI_IFR_OR_OP:
      Operator  = PopBool (StackPtr);
      Operator2 = PopBool (StackPtr);
      PushBool (StackPtr, (BOOLEAN) (Operator || Operator2));
      break;
    case EFI_IFR_NOT_OP:
      Operator  = PopBool (StackPtr);
      PushBool (StackPtr, (BOOLEAN) (!Operator));
      break;

    case EFI_IFR_SUPPRESS_IF_OP:
    case EFI_IFR_GRAYOUT_IF_OP:
    case EFI_IFR_INCONSISTENT_IF_OP:
    default:
      //
      // Return to the previous tag if runs out of boolean expression.
      //
      (*PIterator)--;
      return;
    }
    (*PIterator)++;
  }
}

BOOLEAN
ValueIsNotValid (
  IN  BOOLEAN                     Complex,
  IN  UINT16                      Value,
  IN  EFI_TAG                     *Tag,
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  IN  STRING_REF                  *PopUp
  )
/*++

Routine Description:


Arguments:

Returns:

  TRUE - If value is valid

  FALSE - If value is not valid

--*/
{
  BOOLEAN                 *StackPtr;
  EFI_INCONSISTENCY_DATA  *Iterator;
  BOOLEAN                 Operator;
  BOOLEAN                 Operator2;
  UINTN                   Index;
  VOID                    *BooleanExpression;
  UINTN                   BooleanExpressionLength;
  BOOLEAN                 NotOperator;
  BOOLEAN                 OrOperator;
  BOOLEAN                 AndOperator;
  BOOLEAN                 ArtificialEnd;
  UINT16                  *MapBuffer;
  UINT16                  *MapBuffer2;
  UINT16                  MapValue;
  UINT16                  MapValue2;
  UINTN                   SizeOfVariable;
  CHAR16                  VariableName[MAXIMUM_VALUE_CHARACTERS];
  VOID                    *VariableData;
  EFI_STATUS              Status;
  UINT16                  Id;
  UINT16                  Width;
  EFI_VARIABLE_DEFINITION *VariableDefinition;
  BOOLEAN                 CosmeticConsistency;
  UINT8                   IsLegacy;

  VariableData            = NULL;
  BooleanExpressionLength = 0;
  BooleanExpression       = NULL;
  Operator                = FALSE;
  ArtificialEnd           = FALSE;
  CosmeticConsistency     = TRUE;
  IsLegacy                = 0;

  Id                      = Tag->Id;
  if (Tag->StorageWidth == 1) {
    Width = 1;
  } else {
    Width = 2;
  }
  CreateBooleanExpression (FileFormTags, Value, Id, Complex, &BooleanExpression, &BooleanExpressionLength);

  if (mBooleanEvaluationStack == 0) {
    InitializeBooleanEvaluator ();
  }

  if (BooleanExpression == NULL) {
    return FALSE;
  }

  StackPtr    = mBooleanEvaluationStack;
  Iterator    = BooleanExpression;
  MapBuffer   = NULL;
  MapBuffer2  = NULL;
  MapValue    = 0;
  MapValue2   = 0;

  while (TRUE) {
    NotOperator = FALSE;
    OrOperator  = FALSE;
    AndOperator = FALSE;

    if (Iterator->Operand == 0) {
      return Operator;
    }

    //
    //  Because INVALID_OFFSET_VALUE - 1 is reserved for TRUE or FALSE, omit them.
    //
    if (Iterator->QuestionId1 != INVALID_OFFSET_VALUE &&
        Iterator->QuestionId1 != INVALID_OFFSET_VALUE-1) {
      ExtractNvValue (FileFormTags, Iterator->VariableNumber, Width, Iterator->QuestionId1, (VOID **) &MapBuffer);
      ExtractNvValue (FileFormTags, Iterator->VariableNumber2, Width, Iterator->QuestionId2, (VOID **) &MapBuffer2);
      if (MapBuffer != NULL) {
        if (Width == 2) {
          MapValue = *MapBuffer;
        } else {
          MapValue = (UINT8) *MapBuffer;
        }

        FreePool (MapBuffer);
      }

      if (MapBuffer2 != NULL) {
        if (Width == 2) {
          MapValue2 = *MapBuffer2;
        } else {
          MapValue2 = (UINT8) *MapBuffer2;
        }

        FreePool (MapBuffer2);
      }
    }

    switch (Iterator->Operand) {
    case EFI_IFR_SUPPRESS_IF_OP:
      //
      // Must have hit a suppress followed by a grayout or vice-versa
      //
      if (ArtificialEnd) {
        ArtificialEnd = FALSE;
        Operator      = PopBool (&StackPtr);
        if (Operator) {
          Tag->Suppress = TRUE;
        }

        return Operator;
      }

      ArtificialEnd = TRUE;
      *PopUp        = Iterator->Popup;
      break;

    case EFI_IFR_GRAYOUT_IF_OP:
      //
      // Must have hit a suppress followed by a grayout or vice-versa
      //
      if (ArtificialEnd) {
        ArtificialEnd = FALSE;
        Operator      = PopBool (&StackPtr);
        if (Operator) {
          Tag->GrayOut = TRUE;
        }

        return Operator;
      }

      ArtificialEnd = TRUE;
      *PopUp        = Iterator->Popup;
      break;

    case EFI_IFR_INCONSISTENT_IF_OP:
      CosmeticConsistency = FALSE;
      *PopUp              = Iterator->Popup;
      break;

    //
    // In the case of external variable values, we must read the variable which is
    // named by the human readable version of the OpCode->VariableId and the guid of the formset
    //
    case EFI_IFR_EQ_VAR_VAL_OP:
      //
      // To check whether Ifr is legacy. Once every boolean expression.
      //
      if (IsLegacy == 0) {
        IsLegacy = PredicateIfrType (Iterator);
      }
      if (IsLegacy == 0x2) {
        PostOrderEvaluate (FileFormTags, Width, &Iterator, &StackPtr);
        break;
      }

      UnicodeValueToString (
        VariableName,
        FALSE,
        (UINTN) Iterator->QuestionId1,
        (sizeof (VariableName) / sizeof (VariableName[0])) - 1
        );

      SizeOfVariable = 0;

      ExtractRequestedNvMap (FileFormTags, Iterator->VariableNumber, &VariableDefinition);

      Status = BooleanVariableWorker (
                VariableName,
                VariableDefinition,
                StackPtr,
                &SizeOfVariable,
                &VariableData
                );

      if (!EFI_ERROR (Status)) {
        if (SizeOfVariable == 1) {
          CopyMem (&MapValue, VariableData, 1);
        } else {
          CopyMem (&MapValue, VariableData, 2);
        }

        PushBool (&StackPtr, (BOOLEAN) (MapValue == Iterator->Value));
      }

      break;

    case EFI_IFR_EQ_ID_VAL_OP:
      //
      // To check whether Ifr is legacy. Once every boolean expression.
      //
      if (IsLegacy == 0) {
        IsLegacy = PredicateIfrType (Iterator);
      }
      if (IsLegacy == 0x2) {
        PostOrderEvaluate (FileFormTags, Width, &Iterator, &StackPtr);
        break;
      }

      PushBool (&StackPtr, (BOOLEAN) (MapValue == Iterator->Value));
      break;

    case EFI_IFR_EQ_ID_ID_OP:
      //
      // To check whether Ifr is legacy. Once every boolean expression.
      //
      if (IsLegacy == 0) {
        IsLegacy = PredicateIfrType (Iterator);
      }
      if (IsLegacy == 0x2) {
        PostOrderEvaluate (FileFormTags, Width, &Iterator, &StackPtr);
        break;
      }

      PushBool (&StackPtr, (BOOLEAN) (MapValue == MapValue2));
      break;

    case EFI_IFR_EQ_ID_LIST_OP:
      //
      // To check whether Ifr is legacy. Once every boolean expression.
      //
      if (IsLegacy == 0) {
        IsLegacy = PredicateIfrType (Iterator);
      }
      if (IsLegacy == 0x2) {
        PostOrderEvaluate (FileFormTags, Width, &Iterator, &StackPtr);
        break;
      }

      for (Index = 0; Index < Iterator->ListLength; Index++) {
        Operator = (BOOLEAN) (MapValue == Iterator->ValueList[Index]);
        if (Operator) {
          break;
        }
      }

      PushBool (&StackPtr, Operator);
      break;

    case EFI_IFR_AND_OP:
      Iterator++;
      if (Iterator->Operand == EFI_IFR_NOT_OP) {
        NotOperator = TRUE;
        Iterator++;
      }

      if (Iterator->QuestionId1 != INVALID_OFFSET_VALUE) {
        ExtractNvValue (FileFormTags, Iterator->VariableNumber, Width, Iterator->QuestionId1, (VOID **) &MapBuffer);
        ExtractNvValue (FileFormTags, Iterator->VariableNumber2, Width, Iterator->QuestionId2, (VOID **) &MapBuffer2);
        if (MapBuffer != NULL) {
          if (Width == 2) {
            MapValue = *MapBuffer;
          } else {
            MapValue = (UINT8) *MapBuffer;
          }

          FreePool (MapBuffer);
        }

        if (MapBuffer2 != NULL) {
          if (Width == 2) {
            MapValue2 = *MapBuffer2;
          } else {
            MapValue2 = (UINT8) *MapBuffer2;
          }

          FreePool (MapBuffer2);
        }
      }

      switch (Iterator->Operand) {
      case EFI_IFR_EQ_ID_VAL_OP:
        //
        // If Not - flip the results
        //
        if (NotOperator) {
          Operator = (BOOLEAN)!(MapValue == Iterator->Value);
        } else {
          Operator = (BOOLEAN) (MapValue == Iterator->Value);
        }

        PushBool (&StackPtr, Operator);
        break;

      //
      // In the case of external variable values, we must read the variable which is
      // named by the human readable version of the OpCode->VariableId and the guid of the formset
      //
      case EFI_IFR_EQ_VAR_VAL_OP:
        UnicodeValueToString (
          VariableName,
          FALSE,
          (UINTN) Iterator->QuestionId1,
          (sizeof (VariableName) / sizeof (VariableName[0])) - 1
          );

        SizeOfVariable = 0;

        ExtractRequestedNvMap (FileFormTags, Iterator->VariableNumber, &VariableDefinition);

        Status = BooleanVariableWorker (
                  VariableName,
                  VariableDefinition,
                  StackPtr,
                  &SizeOfVariable,
                  &VariableData
                  );

        if (!EFI_ERROR (Status)) {
          if (SizeOfVariable == 1) {
            CopyMem (&MapValue, VariableData, 1);
          } else {
            CopyMem (&MapValue, VariableData, 2);
          }
          //
          // If Not - flip the results
          //
          if (NotOperator) {
            PushBool (&StackPtr, (BOOLEAN)!(MapValue == Iterator->Value));
          } else {
            PushBool (&StackPtr, (BOOLEAN) (MapValue == Iterator->Value));
          }
        }
        break;

      case EFI_IFR_EQ_ID_ID_OP:
        //
        // If Not - flip the results
        //
        if (NotOperator) {
          Operator = (BOOLEAN)!(MapValue == MapValue2);
        } else {
          Operator = (BOOLEAN) (MapValue == MapValue2);
        }

        PushBool (&StackPtr, Operator);
        break;

      case EFI_IFR_EQ_ID_LIST_OP:
        for (Index = 0; Index < Iterator->ListLength; Index++) {
          //
          // If Not - flip the results
          //
          if (NotOperator) {
            Operator = (BOOLEAN)!(MapValue == Iterator->ValueList[Index]);
          } else {
            Operator = (BOOLEAN) (MapValue == Iterator->ValueList[Index]);
          }
          //
          // If We are trying to make sure that MapValue != Item[x], keep looking through
          // the list to make sure we don't equal any other items
          //
          if (Operator && NotOperator) {
            continue;
          }
          //
          // If MapValue == Item, then we have succeeded (first found is good enough)
          //
          if (Operator) {
            break;
          }
        }

        PushBool (&StackPtr, Operator);
        break;

      default:
        return FALSE;
      }

      Operator  = PopBool (&StackPtr);
      Operator2 = PopBool (&StackPtr);
      PushBool (&StackPtr, (BOOLEAN) (Operator && Operator2));
      break;

    case EFI_IFR_OR_OP:
      Iterator++;
      if (Iterator->Operand == EFI_IFR_NOT_OP) {
        NotOperator = TRUE;
        Iterator++;
      }

      if (Iterator->QuestionId1 != INVALID_OFFSET_VALUE) {
        ExtractNvValue (FileFormTags, Iterator->VariableNumber, Width, Iterator->QuestionId1, (VOID **) &MapBuffer);
        ExtractNvValue (FileFormTags, Iterator->VariableNumber2, Width, Iterator->QuestionId2, (VOID **) &MapBuffer2);
        if (MapBuffer != NULL) {
          if (Width == 2) {
            MapValue = *MapBuffer;
          } else {
            MapValue = (UINT8) *MapBuffer;
          }

          FreePool (MapBuffer);
        }

        if (MapBuffer2 != NULL) {
          if (Width == 2) {
            MapValue2 = *MapBuffer2;
          } else {
            MapValue2 = (UINT8) *MapBuffer2;
          }

          FreePool (MapBuffer2);
        }
      }

      switch (Iterator->Operand) {
      case EFI_IFR_EQ_ID_VAL_OP:
        //
        // If Not - flip the results
        //
        if (NotOperator) {
          Operator = (BOOLEAN)!(MapValue == Iterator->Value);
        } else {
          Operator = (BOOLEAN) (MapValue == Iterator->Value);
        }

        PushBool (&StackPtr, Operator);
        break;

      //
      // In the case of external variable values, we must read the variable which is
      // named by the human readable version of the OpCode->VariableId and the guid of the formset
      //
      case EFI_IFR_EQ_VAR_VAL_OP:
        UnicodeValueToString (
          VariableName,
          FALSE,
          (UINTN) Iterator->QuestionId1,
          (sizeof (VariableName) / sizeof (VariableName[0])) - 1
          );

        SizeOfVariable = 0;

        ExtractRequestedNvMap (FileFormTags, Iterator->VariableNumber, &VariableDefinition);

        Status = BooleanVariableWorker (
                  VariableName,
                  VariableDefinition,
                  StackPtr,
                  &SizeOfVariable,
                  &VariableData
                  );

        if (!EFI_ERROR (Status)) {
          if (SizeOfVariable == 1) {
            CopyMem (&MapValue, VariableData, 1);
          } else {
            CopyMem (&MapValue, VariableData, 2);
          }
          //
          // If Not - flip the results
          //
          if (NotOperator) {
            PushBool (&StackPtr, (BOOLEAN)!(MapValue == Iterator->Value));
          } else {
            PushBool (&StackPtr, (BOOLEAN) (MapValue == Iterator->Value));
          }
        }
        break;

      case EFI_IFR_EQ_ID_ID_OP:
        //
        // If Not - flip the results
        //
        if (NotOperator) {
          Operator = (BOOLEAN)!(MapValue == MapValue2);
        } else {
          Operator = (BOOLEAN) (MapValue == MapValue2);
        }

        PushBool (&StackPtr, Operator);
        break;

      case EFI_IFR_EQ_ID_LIST_OP:
        for (Index = 0; Index < Iterator->ListLength; Index++) {
          //
          // If Not - flip the results
          //
          if (NotOperator) {
            Operator = (BOOLEAN)!(MapValue == Iterator->ValueList[Index]);
          } else {
            Operator = (BOOLEAN) (MapValue == Iterator->ValueList[Index]);
          }
          //
          // If We are trying to make sure that MapValue != Item[x], keep looking through
          // the list to make sure we don't equal any other items
          //
          if (Operator && NotOperator) {
            continue;
          }
          //
          // If MapValue == Item, then we have succeeded (first found is good enough)
          //
          if (Operator) {
            break;
          }
        }

        PushBool (&StackPtr, Operator);
        break;

      default:
        return FALSE;
      }

      Operator  = PopBool (&StackPtr);
      Operator2 = PopBool (&StackPtr);
      PushBool (&StackPtr, (BOOLEAN) (Operator || Operator2));
      break;

    case EFI_IFR_NOT_OP:
      //
      // To check whether Ifr is legacy. Once every boolean expression.
      //
      if (IsLegacy == 0) {
        IsLegacy = PredicateIfrType (Iterator);
      }
      if (IsLegacy == 0x2) {
        PostOrderEvaluate (FileFormTags, Width, &Iterator, &StackPtr);
        break;
      }

      //
      // I don't need to set the NotOperator (I know that I have to NOT this in this case
      //
      Iterator++;

      if (Iterator->Operand == EFI_IFR_OR_OP) {
        OrOperator = TRUE;
        Iterator++;
      }

      if (Iterator->Operand == EFI_IFR_AND_OP) {
        AndOperator = TRUE;
        Iterator++;
      }

      if (Iterator->QuestionId1 != INVALID_OFFSET_VALUE) {
        ExtractNvValue (FileFormTags, Iterator->VariableNumber, Width, Iterator->QuestionId1, (VOID **) &MapBuffer);
        ExtractNvValue (FileFormTags, Iterator->VariableNumber2, Width, Iterator->QuestionId2, (VOID **) &MapBuffer2);
        if (MapBuffer != NULL) {
          if (Width == 2) {
            MapValue = *MapBuffer;
          } else {
            MapValue = (UINT8) *MapBuffer;
          }

          FreePool (MapBuffer);
        }

        if (MapBuffer2 != NULL) {
          if (Width == 2) {
            MapValue2 = *MapBuffer2;
          } else {
            MapValue2 = (UINT8) *MapBuffer2;
          }

          FreePool (MapBuffer2);
        }
      }

      switch (Iterator->Operand) {
      case EFI_IFR_EQ_ID_VAL_OP:
        Operator = (BOOLEAN)!(MapValue == Iterator->Value);
        PushBool (&StackPtr, Operator);
        break;

      //
      // In the case of external variable values, we must read the variable which is
      // named by the human readable version of the OpCode->VariableId and the guid of the formset
      //
      case EFI_IFR_EQ_VAR_VAL_OP:
        UnicodeValueToString (
          VariableName,
          FALSE,
          (UINTN) Iterator->QuestionId1,
          (sizeof (VariableName) / sizeof (VariableName[0])) - 1
          );

        SizeOfVariable = 0;

        ExtractRequestedNvMap (FileFormTags, Iterator->VariableNumber, &VariableDefinition);

        Status = BooleanVariableWorker (
                  VariableName,
                  VariableDefinition,
                  StackPtr,
                  &SizeOfVariable,
                  &VariableData
                  );

        if (!EFI_ERROR (Status)) {
          if (SizeOfVariable == 1) {
            CopyMem (&MapValue, VariableData, 1);
          } else {
            CopyMem (&MapValue, VariableData, 2);
          }

          PushBool (&StackPtr, (BOOLEAN)!(MapValue == Iterator->Value));
        }
        break;

      case EFI_IFR_EQ_ID_ID_OP:
        Operator = (BOOLEAN)!(MapValue == MapValue2);
        PushBool (&StackPtr, Operator);
        break;

      case EFI_IFR_EQ_ID_LIST_OP:
        for (Index = 0; Index < Iterator->ListLength; Index++) {
          Operator = (BOOLEAN)!(MapValue == Iterator->ValueList[Index]);
          if (Operator) {
            continue;
          }
        }

        PushBool (&StackPtr, Operator);
        break;

      default:
        return FALSE;
      }

      Operator  = PopBool (&StackPtr);
      Operator2 = PopBool (&StackPtr);

      if (OrOperator) {
        PushBool (&StackPtr, (BOOLEAN) (Operator || Operator2));
      }

      if (AndOperator) {
        PushBool (&StackPtr, (BOOLEAN) (Operator && Operator2));
      }

      if (!OrOperator && !AndOperator) {
        PushBool (&StackPtr, Operator);
      }
      break;

    case EFI_IFR_TRUE_OP:
      //
      // To check whether Ifr is legacy. Once every boolean expression.
      //
      if (IsLegacy == 0) {
        IsLegacy = PredicateIfrType (Iterator);
      }
      if (IsLegacy == 0x2) {
        PostOrderEvaluate (FileFormTags, Width, &Iterator, &StackPtr);
        break;
      }
      break;

    case EFI_IFR_FALSE_OP:
      //
      // To check whether Ifr is legacy. Once every boolean expression.
      //
      if (IsLegacy == 0) {
        IsLegacy = PredicateIfrType (Iterator);
      }
      if (IsLegacy == 0x2) {
        PostOrderEvaluate (FileFormTags, Width, &Iterator, &StackPtr);
        break;
      }
      break;

    case EFI_IFR_END_IF_OP:
      Operator = PopBool (&StackPtr);
      //
      // If there is an error, return, otherwise keep looking - there might
      // be another test that causes an error
      //
      if (Operator) {
        if (Complex && CosmeticConsistency) {
          return EFI_SUCCESS;
        } else {
          return Operator;
        }
      } else {
        //
        // If not doing a global consistency check, the endif is the REAL terminator of this operation
        // This is used for grayout/suppress operations.  InconsistentIf is a global operation so the EndIf is
        // not the end-all be-all of terminators.
        //
        if (!Complex) {
          return Operator;
        }
        break;
      }

    default:
      //
      // Must have hit a non-consistency related op-code after a suppress/grayout
      //
      if (ArtificialEnd) {
        ArtificialEnd = FALSE;
        Operator      = PopBool (&StackPtr);
        return Operator;
      }

      goto Done;
    }

    Iterator++;
  }

Done:
  return FALSE;
}
