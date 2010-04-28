/*++

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  IfrVariable.c

Abstract:
  Variable/Map manipulations routines

--*/

#include "IfrLibrary.h"

VOID
EfiLibHiiVariablePackGetMap (
  IN    EFI_HII_VARIABLE_PACK       *Pack,  
  OUT   CHAR16                      **Name,  OPTIONAL
  OUT   EFI_GUID                    **Guid,  OPTIONAL
  OUT   UINT16                      *Id,     OPTIONAL
  OUT   VOID                        **Var,   OPTIONAL
  OUT   UINTN                       *Size    OPTIONAL
  )
/*++

Routine Description:

  Extracts a variable form a Pack.

Arguments:

  Pack - List of variables
  Name - Name of the variable/map
  Guid - GUID of the variable/map
  Var  - Pointer to the variable/map
  Size - Size of the variable/map in bytes

Returns: 

  VOID
  
--*/
{
  if (NULL != Name) {
    *Name = (VOID *) (Pack + 1);
  }
    
  if (NULL != Guid) { 
    *Guid = &Pack->VariableGuid;
  }
    
    
  if (NULL != Id) {
    *Id   = Pack->VariableId;
  }
    
  if (NULL != Var) {
    *Var  = (VOID *) ((CHAR8 *) (Pack + 1) + Pack->VariableNameLength);
  }
    
  if (NULL != Size) {
    *Size = Pack->Header.Length - sizeof (*Pack) - Pack->VariableNameLength;
  }
}


UINTN
EfiLibHiiVariablePackListGetMapCnt (
  IN    EFI_HII_VARIABLE_PACK_LIST   *List
  )
  
/*++

Routine Description:

  Finds a count of the variables/maps in the List.

Arguments:

  List - List of variables

Returns: 

  UINTN - The number of map count.

--*/

{
  UINTN   Cnt = 0;
  while (NULL != List) {
    Cnt++;
    List = List->NextVariablePack;
  }
  return Cnt;
}


VOID
EfiLibHiiVariablePackListForEachVar (
  IN    EFI_HII_VARIABLE_PACK_LIST               *List,
  IN    EFI_LIB_HII_VARIABLE_PACK_LIST_CALLBACK  *Callback
  )
/*++

Routine Description:

  Will iterate all variable/maps as appearing 
  in List and for each, it will call the Callback.

Arguments:

  List     - List of variables
  Callback - Routine to be called for each iterated variable.

Returns: 

  VOID
  
--*/

{
  CHAR16    *MapName;
  EFI_GUID  *MapGuid;
  UINT16    MapId;
  VOID      *Map;
  UINTN     MapSize;

  while (NULL != List) {
    EfiLibHiiVariablePackGetMap (List->VariablePack, &MapName, &MapGuid, &MapId, &Map, &MapSize);
    //
    // call the callback
    //
    Callback (MapName, MapGuid, MapId, Map, MapSize); 
    List = List->NextVariablePack;
  }
}


EFI_STATUS
EfiLibHiiVariablePackListGetMapByIdx (
  IN    UINTN                       Idx,  
  IN    EFI_HII_VARIABLE_PACK_LIST  *List,  
  OUT   CHAR16                      **Name,  OPTIONAL
  OUT   EFI_GUID                    **Guid,  OPTIONAL
  OUT   UINT16                      *Id,     OPTIONAL
  OUT   VOID                        **Var,
  OUT   UINTN                       *Size
  )

/*++

Routine Description:

  Finds a variable form List given 
  the order number as appears in the List.

Arguments:

  Idx  - The index of the variable/map to retrieve
  List - List of variables
  Name - Name of the variable/map
  Guid - GUID of the variable/map
  Var  - Pointer to the variable/map
  Size - Size of the variable/map in bytes

Returns:

  EFI_SUCCESS   - Variable is found, OUT parameters are valid
  EFI_NOT_FOUND - Variable is not found, OUT parameters are not valid

--*/
{
  CHAR16     *MapName;
  EFI_GUID   *MapGuid;
  UINT16     MapId;
  VOID       *Map;
  UINTN      MapSize;

  while (NULL != List) {
    EfiLibHiiVariablePackGetMap (List->VariablePack, &MapName, &MapGuid, &MapId, &Map, &MapSize);
    if (0 == Idx--) {
      *Var  = Map;
      *Size = MapSize;

      if (NULL != Name) {
        *Name = MapName;
      }

      if (NULL != Guid) {
        *Guid = MapGuid;
      }
        
      if (NULL != Id) {
        *Id   = MapId;
      }
        
      return EFI_SUCCESS; // Map found
    }
    List = List->NextVariablePack;
  }
  //
  // If here, the map is not found
  //
  return EFI_NOT_FOUND; 
}


EFI_STATUS
EfiLibHiiVariablePackListGetMapById (
  IN    UINT16                        Id,  
  IN    EFI_HII_VARIABLE_PACK_LIST    *List,
  OUT   CHAR16                        **Name,  OPTIONAL
  OUT   EFI_GUID                      **Guid,  OPTIONAL
  OUT   VOID                          **Var,
  OUT   UINTN                         *Size
  )
  
/*++

Routine Description:

  Finds a variable form List given the 
  order number as appears in the List.

Arguments:

  Id   - The ID of the variable/map to retrieve
  List - List of variables
  Name - Name of the variable/map
  Guid - GUID of the variable/map
  Var  - Pointer to the variable/map
  Size - Size of the variable/map in bytes

Returns:

  EFI_SUCCESS   - Variable is found, OUT parameters are valid
  EFI_NOT_FOUND - Variable is not found, OUT parameters are not valid

--*/

{ 
  CHAR16    *MapName;
  EFI_GUID  *MapGuid;
  UINT16    MapId;
  VOID      *Map;
  UINTN     MapSize;

  while (NULL != List) {
    EfiLibHiiVariablePackGetMap (List->VariablePack, &MapName, &MapGuid, &MapId, &Map, &MapSize);
    if (MapId == Id) {
      *Var  = Map;
      *Size = MapSize;
      if (NULL != Name) {
         *Name = MapName;
      }
      if (NULL != Guid) {
        *Guid = MapGuid;
      }
      //
      // Map found
      //
      return EFI_SUCCESS; 
    }
    List = List->NextVariablePack;
  }
  //
  // If here, the map is not found
  //
  return EFI_NOT_FOUND; 
}


EFI_STATUS
EfiLibHiiVariablePackListGetMap (
  IN    EFI_HII_VARIABLE_PACK_LIST   *List,
  IN    CHAR16                       *Name,
  IN    EFI_GUID                     *Guid,
  OUT   UINT16                       *Id,
  OUT   VOID                         **Var, 
  OUT   UINTN                        *Size
  )

/*++

Routine Description:

  Finds a variable form EFI_HII_VARIABLE_PACK_LIST given name and GUID.

Arguments:

  List - List of variables
  Name - Name of the variable/map to be found
  Guid - GUID of the variable/map to be found
  Var  - Pointer to the variable/map found
  Size - Size of the variable/map in bytes found

Returns:

  EFI_SUCCESS   - variable is found, OUT parameters are valid
  EFI_NOT_FOUND - variable is not found, OUT parameters are not valid

--*/

{ 
  VOID      *Map;
  UINTN     MapSize;
  UINT16    MapId;
  CHAR16    *MapName;
  EFI_GUID  *MapGuid;

  while (NULL != List) {
    EfiLibHiiVariablePackGetMap (List->VariablePack, &MapName, &MapGuid, &MapId, &Map, &MapSize);
    if ((0 == EfiStrCmp (Name, MapName)) && EfiCompareGuid (Guid, MapGuid)) {
      *Id   = MapId;
      *Var  = Map;
      *Size = MapSize;
      return EFI_SUCCESS;
    }
    List = List->NextVariablePack;
  }
  //
  // If here, the map is not found
  //
  return EFI_NOT_FOUND;
}

EFI_STATUS
EfiLibHiiVariableRetrieveFromNv (
  IN  CHAR16                 *Name,
  IN  EFI_GUID               *Guid,
  IN  UINTN                   Size,
  OUT VOID                  **Var
  )
/*++

Routine Description:
  Finds out if a variable of specific Name/Guid/Size exists in NV. 
  If it does, it will retrieve it into the Var. 

Arguments:
  Name, Guid, Size - Parameters of the variable to retrieve. Must match exactly.
  Var              - Variable will be retrieved into buffer pointed by this pointer.
                     If pointing to NULL, the buffer will be allocated. Caller is responsible for releasing the buffer.
Returns:
  EFI_SUCCESS    - The variable of exact Name/Guid/Size parameters was retrieved and written to Var.
  EFI_NOT_FOUND  - The variable of this Name/Guid was not found in the NV.
  EFI_LOAD_ERROR - The variable in the NV was of different size, or NV API returned error.

--*/
{
  EFI_STATUS                  Status;
  UINTN                       SizeNv;

  //
  // Test for existence of the variable.
  //
  SizeNv = 0;
  Status = gRT->GetVariable (Name, Guid, NULL, &SizeNv, NULL);
  if (EFI_BUFFER_TOO_SMALL != Status) {
    ASSERT (EFI_SUCCESS != Status);
    return EFI_NOT_FOUND;
  }
  if (SizeNv != Size) {
    //
    // The variable is considered corrupt, as it has different size from expected.
    //
    return EFI_LOAD_ERROR; 
  }

  if (NULL == *Var) {
    *Var = EfiLibAllocatePool (Size);
    ASSERT (NULL != *Var);
  }
  SizeNv = Size;
  //
  // Final read into the Var
  //
  Status = gRT->GetVariable (Name, Guid, NULL, &SizeNv, *Var); 
  //
  // No tolerance for random failures. Such behavior is undetermined and not validated.
  //
  ASSERT_EFI_ERROR (Status); 
  ASSERT (SizeNv == Size);
  return EFI_SUCCESS;
}



EFI_STATUS
EfiLibHiiVariableOverrideIfSuffix (
  IN  CHAR16                 *Suffix,
  IN  CHAR16                 *Name,
  IN  EFI_GUID               *Guid,
  IN  UINTN                   Size,
  OUT VOID                   *Var
  )  
/*++

Routine Description:
  Overrrides the variable with NV data if found.
  But it only does it if the Name ends with specified Suffix.
  For example, if Suffix="MyOverride" and the Name="XyzSetupMyOverride",
  the Suffix matches the end of Name, so the variable will be loaded from NV
  provided the variable exists and the GUID and Size matches.

Arguments:
  Suffix           - Suffix the Name should end with.
  Name, Guid, Size - Parameters of the variable to retrieve. Must match exactly.
  Var              - Variable will be retrieved into this buffer.
                     Caller is responsible for providing storage of exactly Size size in bytes.
Returns:
  EFI_SUCCESS           - The variable was overriden with NV variable of same Name/Guid/Size.
  EFI_INVALID_PARAMETER - The name of the variable does not end with <Suffix>.
  EFI_NOT_FOUND         - The variable of this Name/Guid was not found in the NV.
  EFI_LOAD_ERROR        - The variable in the NV was of different size, or NV API returned error.

--*/
{
  UINTN         StrLen;
  UINTN         StrLenSuffix;

  StrLen       = EfiStrLen (Name);
  StrLenSuffix = EfiStrLen (Suffix);
  if ((StrLen <= StrLenSuffix) || (0 != EfiStrCmp (Suffix, &Name[StrLen - StrLenSuffix]))) {
    //
    // Not ending with <Suffix>.
    //
    return EFI_INVALID_PARAMETER; 
  }
  return EfiLibHiiVariableRetrieveFromNv (Name, Guid, Size, &Var);
}



EFI_STATUS
EfiLibHiiVariableOverrideBySuffix (
  IN  CHAR16                 *Suffix,
  IN  CHAR16                 *Name,
  IN  EFI_GUID               *Guid,
  IN  UINTN                   Size,
  OUT VOID                   *Var
  ) 
/*++

Routine Description:
  Overrrides the variable with NV data if found.
  But it only does it if the NV contains the same variable with Name is appended with Suffix.  
  For example, if Suffix="MyOverride" and the Name="XyzSetup",
  the Suffix will be appended to the end of Name, and the variable with Name="XyzSetupMyOverride"
  will be loaded from NV provided the variable exists and the GUID and Size matches.

Arguments:
  Suffix           - Suffix the variable will be appended with.
  Name, Guid, Size - Parameters of the variable to retrieve. Must match exactly.
  Var              - Variable will be retrieved into this buffer.
                     Caller is responsible for providing storage of exactly Size size in bytes.

Returns:
  EFI_SUCCESS    - The variable was overriden with NV variable of same Name/Guid/Size.
  EFI_NOT_FOUND  - The variable of this Name/Guid was not found in the NV.
  EFI_LOAD_ERROR - The variable in the NV was of different size, or NV API returned error.

--*/
{
  EFI_STATUS    Status;
  CHAR16       *NameSuffixed;
  UINTN         NameLength;
  UINTN         SuffixLength;

  //
  // enough to concatenate both strings.
  //
  NameLength   = EfiStrLen (Name);
  SuffixLength = EfiStrLen (Suffix);
  NameSuffixed = EfiLibAllocateZeroPool ((NameLength + SuffixLength + 1) * sizeof (CHAR16)); 
  
  EfiStrCpy (NameSuffixed, Name);
  EfiStrCat (NameSuffixed, Suffix);
  
  Status = EfiLibHiiVariableRetrieveFromNv (NameSuffixed, Guid, Size, &Var);
  gBS->FreePool (NameSuffixed);
  
  return Status;
}

