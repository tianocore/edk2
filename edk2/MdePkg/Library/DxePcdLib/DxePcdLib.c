/** @file
Implementation of PcdLib class library for DXE phase.

Copyright (c) 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


**/


#include <PiDxe.h>

#include <Protocol/Pcd.h>

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

PCD_PROTOCOL  *mPcd = NULL;


/**
  The constructor function caches the PCD_PROTOCOL pointer.

  @param[in] ImageHandle The firmware allocated handle for the EFI image.  
  @param[in] SystemTable A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS The constructor always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PcdLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // PCD protocol has not been installed, but a module needs to access a
  // dynamic PCD entry.
  // 
  Status = gBS->LocateProtocol (&gPcdProtocolGuid, NULL, (VOID **)&mPcd);
  ASSERT_EFI_ERROR (Status);
  ASSERT (mPcd!= NULL);

  return Status;
}


/**
  Sets the current SKU in the PCD database to the value specified by SkuId.  SkuId is returned.
  If SkuId is not less than PCD_MAX_SKU_ID, then ASSERT().
  
  @param[in]  SkuId     System SKU ID. The SKU value that will be used when the PCD service will retrieve and 
                        set values.

  @return Return the SKU ID that just be set.

**/
UINTN
EFIAPI
LibPcdSetSku (
  IN UINTN  SkuId
  )
{
  ASSERT (SkuId < PCD_MAX_SKU_ID);

  mPcd->SetSku (SkuId);

  return SkuId;
}



/**
  Returns the 8-bit value for the token specified by TokenNumber. 

  @param[in]  TokenNumber   The PCD token number to retrieve a current value for.

  @return Returns the 8-bit value for the token specified by TokenNumber. 

**/
UINT8
EFIAPI
LibPcdGet8 (
  IN UINTN             TokenNumber
  )
{
  return mPcd->Get8 (TokenNumber);
}



/**
  Returns the 16-bit value for the token specified by TokenNumber. 

  @param[in]  TokenNumber   The PCD token number to retrieve a current value for.

  @return Returns the 16-bit value for the token specified by TokenNumber. 

**/
UINT16
EFIAPI
LibPcdGet16 (
  IN UINTN             TokenNumber
  )
{
  return mPcd->Get16 (TokenNumber);
}



/**
  Returns the 32-bit value for the token specified by TokenNumber. 

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the 32-bit value for the token specified by TokenNumber.

**/
UINT32
EFIAPI
LibPcdGet32 (
  IN UINTN             TokenNumber
  )
{
  return mPcd->Get32 (TokenNumber);
}



/**
  Returns the 64-bit value for the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the 64-bit value for the token specified by TokenNumber.

**/
UINT64
EFIAPI
LibPcdGet64 (
  IN UINTN             TokenNumber
  )
{
  return mPcd->Get64 (TokenNumber);
}



/**
  Returns the pointer to the buffer of the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the pointer to the token specified by TokenNumber.

**/
VOID *
EFIAPI
LibPcdGetPtr (
  IN UINTN             TokenNumber
  )
{
  return mPcd->GetPtr (TokenNumber);
}



/**
  Returns the Boolean value of the token specified by TokenNumber. 

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the Boolean value of the token specified by TokenNumber. 

**/
BOOLEAN 
EFIAPI
LibPcdGetBool (
  IN UINTN             TokenNumber
  )
{
  return mPcd->GetBool (TokenNumber);
}



/**
  Returns the size of the token specified by TokenNumber. 

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the size of the token specified by TokenNumber. 

**/
UINTN
EFIAPI
LibPcdGetSize (
  IN UINTN             TokenNumber
  )
{
  return mPcd->GetSize (TokenNumber);
}



/**
  Returns the 8-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the UINT8.

**/
UINT8
EFIAPI
LibPcdGetEx8 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  ASSERT (Guid != NULL);
  
  return mPcd->Get8Ex (Guid, TokenNumber);
}


/**
  Returns the 16-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the UINT16.

**/
UINT16
EFIAPI
LibPcdGetEx16 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  ASSERT (Guid != NULL);

  return mPcd->Get16Ex (Guid, TokenNumber);
}


/**
  Returns the 32-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the UINT32.

**/
UINT32
EFIAPI
LibPcdGetEx32 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  ASSERT (Guid != NULL);

  return mPcd->Get32Ex (Guid, TokenNumber);
}



/**
  Returns the 64-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the UINT64.

**/
UINT64
EFIAPI
LibPcdGetEx64 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  ASSERT (Guid != NULL);
  
  return mPcd->Get64Ex (Guid, TokenNumber);
}



/**
  Returns the pointer to the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the VOID* pointer.

**/
VOID *
EFIAPI
LibPcdGetExPtr (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  ASSERT (Guid != NULL);

  return mPcd->GetPtrEx (Guid, TokenNumber);
}



/**
  Returns the Boolean value of the token specified by TokenNumber and Guid. 
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the BOOLEAN.

**/
BOOLEAN
EFIAPI
LibPcdGetExBool (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  ASSERT (Guid != NULL);

  return mPcd->GetBoolEx (Guid, TokenNumber);
}



/**
  Returns the size of the token specified by TokenNumber and Guid. 
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the size.

**/
UINTN
EFIAPI
LibPcdGetExSize (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  ASSERT (Guid != NULL);

  return mPcd->GetSizeEx (Guid, TokenNumber);
}



/**
  Sets the 8-bit value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 8-bit value to set.

  @return Return the value been set.

**/
UINT8
EFIAPI
LibPcdSet8 (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
{
  EFI_STATUS Status;

  Status = mPcd->Set8 (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);
  
  return Value;
}



/**
  Sets the 16-bit value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 16-bit value to set.

  @return Return the value been set.

**/
UINT16
EFIAPI
LibPcdSet16 (
  IN UINTN             TokenNumber,
  IN UINT16            Value
  )
{
  EFI_STATUS Status;

  Status = mPcd->Set16 (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);
  
  return Value;
}



/**
  Sets the 32-bit value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 32-bit value to set.

  @return Return the value been set.

**/
UINT32
EFIAPI
LibPcdSet32 (
  IN UINTN             TokenNumber,
  IN UINT32             Value
  )
{
  EFI_STATUS Status;
  Status = mPcd->Set32 (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 64-bit value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 64-bit value to set.

  @return Return the value been set.

**/
UINT64
EFIAPI
LibPcdSet64 (
  IN UINTN             TokenNumber,
  IN UINT64             Value
  )
{
  EFI_STATUS Status;

  Status = mPcd->Set64 (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets a buffer for the token specified by TokenNumber to 
  the value specified by Buffer and SizeOfBuffer.  Buffer to
  be set is returned. The content of the buffer could be 
  overwritten if a Callback on SET is registered with this
  TokenNumber.
  
  If SizeOfBuffer is greater than the maximum 
  size support by TokenNumber, then set SizeOfBuffer to the 
  maximum size supported by TokenNumber and return NULL to 
  indicate that the set operation was not actually performed. 
  
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().
  
  @param[in]      TokenNumber   The PCD token number to set a current value for.
  @param[in, out] SizeOfBuffer  The size, in bytes, of Buffer.
                                In out, returns actual size of buff is set. 
  @param[in]      Buffer        A pointer to the buffer to set.

  @return Return the pointer for the buffer been set.

**/
VOID *
EFIAPI
LibPcdSetPtr (
  IN      UINTN             TokenNumber,
  IN OUT  UINTN             *SizeOfBuffer,
  IN      VOID              *Buffer
  )
{
  EFI_STATUS Status;

  ASSERT (SizeOfBuffer != NULL);

  if (*SizeOfBuffer > 0) {
    ASSERT (Buffer != NULL);
  }

  Status = mPcd->SetPtr (TokenNumber, SizeOfBuffer, Buffer);

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return Buffer;
}



/**
  Sets the Boolean value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value       The boolean value to set.

  @return Return the value been set.

**/
BOOLEAN
EFIAPI
LibPcdSetBool (
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
{
  EFI_STATUS Status;

  Status = mPcd->SetBool (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 8-bit value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 8-bit value to set.

  @return Return the value been set.

**/
UINT8
EFIAPI
LibPcdSetEx8 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
{
  EFI_STATUS Status;

  ASSERT (Guid != NULL);

  Status = mPcd->Set8Ex (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 16-bit value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 16-bit value to set.

  @return Return the value been set.

**/
UINT16
EFIAPI
LibPcdSetEx16 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT16            Value
  )
{
  EFI_STATUS Status;

  ASSERT (Guid != NULL);

  Status = mPcd->Set16Ex (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 32-bit value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 32-bit value to set.

  @return Return the value been set.

**/
UINT32
EFIAPI
LibPcdSetEx32 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT32             Value
  )
{
  EFI_STATUS Status;

  ASSERT (Guid != NULL);

  Status = mPcd->Set32Ex (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 64-bit value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 64-bit value to set.

  @return Return the value been set.

**/
UINT64
EFIAPI
LibPcdSetEx64 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT64             Value
  )
{
  EFI_STATUS Status;

  ASSERT (Guid != NULL);

  Status = mPcd->Set64Ex (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets a buffer for the token specified by TokenNumber to the value specified by 
  Buffer and SizeOfBuffer.  Buffer is returned.  If SizeOfBuffer is greater than 
  the maximum size support by TokenNumber, then set SizeOfBuffer to the maximum size 
  supported by TokenNumber and return NULL to indicate that the set operation 
  was not actually performed. 
  
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().
  
  @param[in]        Guid Pointer to a 128-bit unique value that 
                    designates which namespace to set a value from.
  @param[in]        TokenNumber The PCD token number to set a current value for.
  @param[in, out]   SizeOfBuffer The size, in bytes, of Buffer.
                    In out, returns actual size of buffer is set.
  @param[in]        Buffer A pointer to the buffer to set.

  @return Return the pinter to the buffer been set.

**/
VOID *
EFIAPI
LibPcdSetExPtr (
  IN      CONST GUID        *Guid,
  IN      UINTN             TokenNumber,
  IN OUT  UINTN             *SizeOfBuffer,
  IN      VOID              *Buffer
  )
{
  EFI_STATUS  Status;

  ASSERT (Guid != NULL);

  ASSERT (SizeOfBuffer != NULL);

  if (*SizeOfBuffer > 0) {
    ASSERT (Buffer != NULL);
  }

  Status = mPcd->SetPtrEx (Guid, TokenNumber, SizeOfBuffer, Buffer);

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return Buffer;
}



/**
  Sets the Boolean value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  If fail to set pcd value, then ASSERT_EFI_ERROR().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The Boolean value to set.

  @return Return the value been set.

**/
BOOLEAN
EFIAPI
LibPcdSetExBool (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
{
  EFI_STATUS Status;

  ASSERT (Guid != NULL);

  Status = mPcd->SetBoolEx (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  When the token specified by TokenNumber and Guid is set, 
  then notification function specified by NotificationFunction is called.  
  If Guid is NULL, then the default token space is used. 
  If NotificationFunction is NULL, then ASSERT().
  If fail to set callback function, then ASSERT_EFI_ERROR().
  
  @param[in]  Guid Pointer to a 128-bit unique value that designates which 
              namespace to set a value from.  If NULL, then the default 
              token space is used.
  @param[in]  TokenNumber The PCD token number to monitor.
  @param[in]  NotificationFunction The function to call when the token 
              specified by Guid and TokenNumber is set.
**/
VOID
EFIAPI
LibPcdCallbackOnSet (
  IN CONST GUID               *Guid,       OPTIONAL
  IN UINTN                    TokenNumber,
  IN PCD_CALLBACK             NotificationFunction
  )
{
  EFI_STATUS Status;

  ASSERT (NotificationFunction != NULL);

  Status = mPcd->CallbackOnSet (Guid, TokenNumber, NotificationFunction);

  ASSERT_EFI_ERROR (Status);

  return;
}



/**
  Disable a notification function that was established with LibPcdCallbackonSet().
  If NotificationFunction is NULL, then ASSERT().
  If fail to cancel callback function, then ASSERT_EFI_ERROR().
  
  @param[in]  Guid Specify the GUID token space.
  @param[in]  TokenNumber Specify the token number.
  @param[in]  NotificationFunction The callback function to be unregistered.

**/
VOID
EFIAPI
LibPcdCancelCallback (
  IN CONST GUID               *Guid,       OPTIONAL
  IN UINTN                    TokenNumber,
  IN PCD_CALLBACK             NotificationFunction
  )
{
  EFI_STATUS Status;

  ASSERT (NotificationFunction != NULL);
    
  Status = mPcd->CancelCallback (Guid, TokenNumber, NotificationFunction);

  ASSERT_EFI_ERROR (Status);

  return;
}



/**
  Retrieves the next PCD token number from the token space specified by Guid.  
  If Guid is NULL, then the default token space is used.  If TokenNumber is 0, 
  then the first token number is returned.  Otherwise, the token number that 
  follows TokenNumber in the token space is returned.  If TokenNumber is the last 
  token number in the token space, then 0 is returned.  If TokenNumber is not 0 and 
  is not in the token space specified by Guid, then ASSERT().
  If Fail to get next token, then ASSERT_EFI_ERROR().

  @param[in]  Guid        Pointer to a 128-bit unique value that designates which namespace 
                          to set a value from.  If NULL, then the default token space is used.
  @param[in]  TokenNumber The previous PCD token number.  If 0, then retrieves the first PCD 
                          token number.

  @return The next valid token number.

**/
UINTN                      
EFIAPI
LibPcdGetNextToken (
  IN CONST GUID             *Guid, OPTIONAL
  IN       UINTN            TokenNumber
  )
{
  EFI_STATUS Status;

  Status = mPcd->GetNextToken (Guid, &TokenNumber);

  ASSERT_EFI_ERROR (Status);

  return TokenNumber;
}



/**
  Retrieves the next PCD token space from a token space specified by Guid.
  Guid of NULL is reserved to mark the default local token namespace on the current
  platform. If Guid is NULL, then the GUID of the first non-local token space of the 
  current platform is returned. If Guid is the last non-local token space, 
  then NULL is returned. 

  If Guid is not NULL and is not a valid token space in the current platform, then ASSERT().
  If fail to get next token space, then ASSERT_EFI_ERROR().
  
  @param[in]  Guid  Pointer to a 128-bit unique value that designates from which namespace 
                    to start the search.

  @return The next valid token namespace.

**/
GUID *           
EFIAPI
LibPcdGetNextTokenSpace (
  IN CONST GUID  *Guid
  )
{
  EFI_STATUS Status;

  Status = mPcd->GetNextTokenSpace (&Guid);

  ASSERT_EFI_ERROR (Status);

  return (GUID *) Guid;
}


/**
  Sets the PCD entry specified by PatchVariable to the value specified by Buffer 
  and SizeOfBuffer.  Buffer is returned.  If SizeOfBuffer is greater than 
  MaximumDatumSize, then set SizeOfBuffer to MaximumDatumSize and return 
  NULL to indicate that the set operation was not actually performed.  
  If SizeOfBuffer is set to MAX_ADDRESS, then SizeOfBuffer must be set to 
  MaximumDatumSize and NULL must be returned.
  
  If PatchVariable is NULL, then ASSERT().
  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[in] PatchVariable      A pointer to the global variable in a module that is 
                                the target of the set operation.
  @param[in] MaximumDatumSize   The maximum size allowed for the PCD entry specified by PatchVariable.
  @param[in, out] SizeOfBuffer  A pointer to the size, in bytes, of Buffer.
                                In out, returns actual size of buffer is set.
  @param[in] Buffer             A pointer to the buffer to used to set the target variable.

  @return Return the pinter to the buffer been set.

**/
VOID *
EFIAPI
LibPatchPcdSetPtr (
  IN        VOID        *PatchVariable,
  IN        UINTN       MaximumDatumSize,
  IN OUT    UINTN       *SizeOfBuffer,
  IN CONST  VOID        *Buffer
  )
{
  ASSERT (PatchVariable != NULL);
  ASSERT (SizeOfBuffer  != NULL);
  
  if (*SizeOfBuffer > 0) {
    ASSERT (Buffer != NULL);
  }

  if ((*SizeOfBuffer > MaximumDatumSize) ||
      (*SizeOfBuffer == MAX_ADDRESS)) {
    *SizeOfBuffer = MaximumDatumSize;
    return NULL;
  }
    
  CopyMem (PatchVariable, Buffer, *SizeOfBuffer);
  
  return (VOID *) Buffer;
}



