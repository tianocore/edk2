/** @file
Implementation of PcdLib class library for PEI phase.

Copyright (c) 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name: PeiPcdLib.c

**/



/**
  The constructor function retrieve the PCD_PPI pointer.

  @param[in] VOID
  
  @retval PCD_PPI * The pointer to the PCD_PPI.

**/
STATIC
PCD_PPI  *
GetPcdPpiPtr (
  VOID
  ) 
{
  EFI_STATUS        Status;
  PCD_PPI           *PcdPpi;
  EFI_PEI_SERVICES  **PeiServices;


  PeiServices = GetPeiServicesTablePointer ();

  Status = (**PeiServices).LocatePpi (
                             PeiServices,
                             &gPcdPpiGuid,
                             0,
                             NULL,
                             (VOID **)&PcdPpi
                             );

  ASSERT_EFI_ERROR (Status);

  return PcdPpi;
}



/**
  Sets the current SKU in the PCD database to the value specified by SkuId.  SkuId is returned.

  @param[in]  SkuId The SKU value that will be used when the PCD service will retrieve and 
              set values associated with a PCD token.

  @retval SKU_ID Return the SKU ID that just be set.

**/
UINTN
EFIAPI
LibPcdSetSku (
  IN UINTN  SkuId
  )
{

  ASSERT (SkuId < 0x100);

  GetPcdPpiPtr()->SetSku (SkuId);;

  return SkuId;
}



/**
  Returns the 8-bit value for the token specified by TokenNumber. 

  @param[in]  The PCD token number to retrieve a current value for.

  @retval UINT8 Returns the 8-bit value for the token specified by TokenNumber. 

**/
UINT8
EFIAPI
LibPcdGet8 (
  IN UINTN                        TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->Get8 (TokenNumber);
}



/**
  Returns the 16-bit value for the token specified by TokenNumber. 

  @param[in]  The PCD token number to retrieve a current value for.

  @retval UINT16 Returns the 16-bit value for the token specified by TokenNumber. 

**/
UINT16
EFIAPI
LibPcdGet16 (
  IN UINTN                        TokenNumber
  )
{
  PCD_PPI  *PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->Get16 (TokenNumber);
}



/**
  Returns the 32-bit value for the token specified by TokenNumber. 

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval UINT32 Returns the 32-bit value for the token specified by TokenNumber.

**/
UINT32
EFIAPI
LibPcdGet32 (
  IN UINTN                        TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->Get32 (TokenNumber);
}



/**
  Returns the 64-bit value for the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval UINT64 Returns the 64-bit value for the token specified by TokenNumber.

**/
UINT64
EFIAPI
LibPcdGet64 (
  IN UINTN                        TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->Get64 (TokenNumber);
}



/**
  Returns the pointer to the buffer of the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval VOID* Returns the pointer to the token specified by TokenNumber.

**/
VOID *
EFIAPI
LibPcdGetPtr (
  IN UINTN                        TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->GetPtr (TokenNumber);
}



/**
  Returns the Boolean value of the token specified by TokenNumber. 

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval BOOLEAN Returns the Boolean value of the token specified by TokenNumber. 

**/
BOOLEAN 
EFIAPI
LibPcdGetBool (
  IN UINTN                        TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->GetBool (TokenNumber);
}



/**
  Returns the size of the token specified by TokenNumber. 

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval UINTN Returns the size of the token specified by TokenNumber. 

**/
UINTN
EFIAPI
LibPcdGetSize (
  IN UINTN                        TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->GetSize (TokenNumber);
}



/**
  Returns the 8-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval UINT8 Return the UINT8.

**/
UINT8
EFIAPI
LibPcdGetEx8 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  ASSERT (Guid != NULL);

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->Get8Ex (Guid, TokenNumber);
}



/**
  Returns the 16-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval UINT16 Return the UINT16.

**/
UINT16
EFIAPI
LibPcdGetEx16 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  ASSERT (Guid != NULL);

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->Get16Ex (Guid, TokenNumber);
}



/**
  Returns the 32-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval UINT32 Return the UINT32.

**/
UINT32
EFIAPI
LibPcdGetEx32 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  ASSERT (Guid != NULL);

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->Get32Ex (Guid, TokenNumber);
}




/**
  Returns the 64-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval UINT64 Return the UINT64.

**/
UINT64
EFIAPI
LibPcdGetEx64 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  ASSERT (Guid != NULL);

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->Get64Ex (Guid, TokenNumber);
}



/**
  Returns the pointer to the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval VOID* Return the VOID* pointer.

**/
VOID *
EFIAPI
LibPcdGetExPtr (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  ASSERT (Guid != NULL);

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->GetPtrEx (Guid, TokenNumber);
}



/**
  Returns the Boolean value of the token specified by TokenNumber and Guid. 
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval BOOLEAN Return the BOOLEAN.

**/
BOOLEAN
EFIAPI
LibPcdGetExBool (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  ASSERT (Guid != NULL);

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->GetBoolEx (Guid, TokenNumber);
}



/**
  Returns the size of the token specified by TokenNumber and Guid. 
  If Guid is NULL, then ASSERT(). 

  @param[in]  Guid Pointer to a 128-bit unique value that designates 
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @retval UINTN Return the size.

**/
UINTN
EFIAPI
LibPcdGetExSize (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber
  )
{
  PCD_PPI * PcdPpi;

  ASSERT (Guid != NULL);

  PcdPpi = GetPcdPpiPtr ();

  return PcdPpi->GetSizeEx (Guid, TokenNumber);
}



/**
  Sets the 8-bit value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 8-bit value to set.

  @retval UINT8 Return the value been set.

**/
UINT8
EFIAPI
LibPcdSet8 (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
{
  EFI_STATUS Status;
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  Status = PcdPpi->Set8 (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);
  
  return Value;
}



/**
  Sets the 16-bit value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 16-bit value to set.

  @retval UINT16 Return the value been set.

**/
UINT16
EFIAPI
LibPcdSet16 (
  IN UINTN             TokenNumber,
  IN UINT16            Value
  )
{
  EFI_STATUS Status;
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  Status = PcdPpi->Set16 (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);
  
  return Value;
}



/**
  Sets the 32-bit value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 32-bit value to set.

  @retval UINT32 Return the value been set.

**/
UINT32
EFIAPI
LibPcdSet32 (
  IN UINTN             TokenNumber,
  IN UINT32             Value
  )
{
  EFI_STATUS Status;
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  Status = PcdPpi->Set32 (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 64-bit value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 64-bit value to set.

  @retval UINT64 Return the value been set.

**/
UINT64
EFIAPI
LibPcdSet64 (
  IN UINTN             TokenNumber,
  IN UINT64             Value
  )
{
  EFI_STATUS Status;
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  Status = PcdPpi->Set64 (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets a buffer for the token specified by TokenNumber to 
  the value specified by Buffer and SizeOfValue.  Buffer to
  be set is returned. The content of the buffer could be 
  overwritten if a Callback on SET is registered with this
  TokenNumber.
  
  If SizeOfValue is greater than the maximum 
  size support by TokenNumber, then set SizeOfValue to the 
  maximum size supported by TokenNumber and return NULL to 
  indicate that the set operation was not actually performed. 
  
  If SizeOfValue > 0 and Buffer is NULL, then ASSERT().
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in,out] SizeOfBuffer The size, in bytes, of Buffer.
  @param[in]  Value A pointer to the buffer to set.

  @retval VOID* Return the pointer for the buffer been set.

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
  PCD_PPI    *PcdPpi;

  ASSERT (SizeOfBuffer != NULL);

  if (*SizeOfBuffer > 0) {
    ASSERT (Buffer != NULL);
  }

  PcdPpi = GetPcdPpiPtr ();
  
  Status = PcdPpi->SetPtr (TokenNumber, SizeOfBuffer, Buffer);

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return Buffer;
}



/**
  Sets the Boolean value for the token specified by TokenNumber 
  to the value specified by Value.  Value is returned.
  
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The boolean value to set.

  @retval BOOLEAN Return the value been set.

**/
BOOLEAN
EFIAPI
LibPcdSetBool (
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
{
  EFI_STATUS Status;
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  Status = PcdPpi->SetBool (TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 8-bit value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 8-bit value to set.

  @retval UINT8 Return the value been set.

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
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  ASSERT (Guid != NULL);

  Status = PcdPpi->Set8Ex (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 16-bit value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 16-bit value to set.

  @retval UINT8 Return the value been set.

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
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  ASSERT (Guid != NULL);
  Status = PcdPpi->Set16Ex (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets the 32-bit value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The 32-bit value to set.

  @retval UINT32 Return the value been set.

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
  PCD_PPI * PcdPpi;
  ASSERT (Guid != NULL);
  PcdPpi = GetPcdPpiPtr ();


  Status = PcdPpi->Set32Ex (Guid, TokenNumber, Value);

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

  @retval UINT64 Return the value been set.

**/
UINT64
EFIAPI
LibPcdSetEx64 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT64            Value
  )
{
  EFI_STATUS Status;
  PCD_PPI * PcdPpi;
  ASSERT (Guid != NULL);
  PcdPpi = GetPcdPpiPtr ();


  Status = PcdPpi->Set64Ex (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  Sets a buffer for the token specified by TokenNumber to the value specified by 
  Buffer and SizeOfValue.  Buffer is returned.  If SizeOfValue is greater than 
  the maximum size support by TokenNumber, then set SizeOfValue to the maximum size 
  supported by TokenNumber and return NULL to indicate that the set operation 
  was not actually performed. 
  
  If SizeOfValue > 0 and Buffer is NULL, then ASSERT().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in, out] SizeOfBuffer The size, in bytes, of Buffer.
  @param[in]  Buffer A pointer to the buffer to set.

  @retval VOID * Return the pinter to the buffer been set.

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
  EFI_STATUS      Status;
  PCD_PPI         *PcdPpi;
 ASSERT (SizeOfBuffer != NULL);
  if (*SizeOfBuffer > 0) {
    ASSERT (Buffer != NULL);
  }
  ASSERT (Guid != NULL);
  PcdPpi = GetPcdPpiPtr ();

  Status = PcdPpi->SetPtrEx (Guid, TokenNumber, SizeOfBuffer, Buffer);

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return Buffer;
}



/**
  Sets the Boolean value for the token specified by TokenNumber and 
  Guid to the value specified by Value. Value is returned.
  If Guid is NULL, then ASSERT().
  
  @param[in]  Guid Pointer to a 128-bit unique value that 
              designates which namespace to set a value from.
  @param[in]  TokenNumber The PCD token number to set a current value for.
  @param[in]  Value The Boolean value to set.

  @retval Boolean Return the value been set.

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
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();

  ASSERT (Guid != NULL);
  Status = PcdPpi->SetBoolEx (Guid, TokenNumber, Value);

  ASSERT_EFI_ERROR (Status);

  return Value;
}



/**
  When the token specified by TokenNumber and Guid is set, 
  then notification function specified by NotificationFunction is called.  
  If Guid is NULL, then the default token space is used. 
  If NotificationFunction is NULL, then ASSERT().

  @param[in]  Guid Pointer to a 128-bit unique value that designates which 
              namespace to set a value from.  If NULL, then the default 
              token space is used.
  @param[in]  TokenNumber The PCD token number to monitor.
  @param[in]  NotificationFunction The function to call when the token 
              specified by Guid and TokenNumber is set.

  @retval VOID

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
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();


  Status = PcdPpi->CallbackOnSet (Guid, TokenNumber, NotificationFunction);

  ASSERT_EFI_ERROR (Status);

  return;
}



/**
  Disable a notification function that was established with LibPcdCallbackonSet().
  If NotificationFunction is NULL, then ASSERT().

  @param[in]  Guid Specify the GUID token space.
  @param[in]  TokenNumber Specify the token number.
  @param[in]  NotificationFunction The callback function to be unregistered.

  @retval VOID

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
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();


  Status = PcdPpi->CancelCallback (Guid, TokenNumber, NotificationFunction);

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

  @param[in]  Pointer to a 128-bit unique value that designates which namespace 
              to set a value from.  If NULL, then the default token space is used.
  @param[in]  The previous PCD token number.  If 0, then retrieves the first PCD 
              token number.

  @retval UINTN            The next valid token number.

**/
UINTN                      
EFIAPI
LibPcdGetNextToken (
  IN CONST GUID             *Guid, OPTIONAL
  IN       UINTN            TokenNumber
  )
{
  EFI_STATUS Status;
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();


  Status = PcdPpi->GetNextToken (Guid, &TokenNumber);

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


  
  @param[in]  Pointer to a 128-bit unique value that designates from which namespace 
              to start the search.

  @retval CONST GUID *  The next valid token namespace.

**/
GUID *           
EFIAPI
LibPcdGetNextTokenSpace (
  IN CONST GUID  *Guid
  )
{
  EFI_STATUS Status;
  PCD_PPI * PcdPpi;

  PcdPpi = GetPcdPpiPtr ();


  Status = PcdPpi->GetNextTokenSpace (&Guid);

  ASSERT_EFI_ERROR (Status);

  return (GUID *)Guid;
}



/**
  Sets the PCD entry specified by PatchVariable to the value specified by Buffer 
  and SizeOfValue.  Buffer is returned.  If SizeOfValue is greater than 
  MaximumDatumSize, then set SizeOfValue to MaximumDatumSize and return 
  NULL to indicate that the set operation was not actually performed.  
  If SizeOfValue is set to MAX_ADDRESS, then SizeOfValue must be set to 
  MaximumDatumSize and NULL must be returned.
  
  If PatchVariable is NULL, then ASSERT().
  If SizeOfValue is NULL, then ASSERT().
  If SizeOfValue > 0 and Buffer is NULL, then ASSERT().

  @param[in] PatchVariable      A pointer to the global variable in a module that is 
                                the target of the set operation.
  @param[in] MaximumDatumSize   The maximum size allowed for the PCD entry specified by PatchVariable.
  @param[in, out] SizeOfBuffer  A pointer to the size, in bytes, of Buffer.
  @param[in] Buffer             A pointer to the buffer to used to set the target variable.

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


