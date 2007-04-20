/**@file
  This file contains the form processing code to the HII database.
  
Copyright (c) 2006 - 2007 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include "HiiDatabase.h"

STATIC
CHAR16*
Ascii2Unicode (
  OUT CHAR16         *UnicodeStr,
  IN  CHAR8          *AsciiStr
  )
/*++
  
  Routine Description:

    This function converts ASCII string to Unicode string.
  
  Arguments:

    UnicodeStr     - NULL terminated Unicode output string.
    AsciieStr      - NULL terminated ASCII input string.
 
  Returns: 

    Start of the Unicode ouput string.
    
--*/

{
  CHAR16      *Str = UnicodeStr;  
  while (TRUE) {
    *(UnicodeStr++) = (CHAR16) *AsciiStr;
    if (*(AsciiStr++) == '\0') {
      return Str;
    }
  }
}

STATIC
CHAR8*
Unicode2Ascii (
  OUT CHAR8          *AsciiStr,
  IN  CHAR16         *UnicodeStr
  )
/*++
  
  Routine Description:

    This function converts Unicode string to ASCII string.
  
  Arguments:

    AsciieStr      - NULL terminated ASCII output string.
    UnicodeStr     - NULL terminated Unicode input string.
 
  Returns: 

    Start of the ASCII ouput string.
    
--*/

{
  CHAR8      *Str = AsciiStr;  
  while (TRUE) {
    *(AsciiStr++) = (CHAR8) *UnicodeStr;
    if (*(UnicodeStr++) == '\0') {
      return Str;
    }
  }
}

STATIC
VOID
ExtractDevicePathData (
  IN     EFI_HII_DATA_TABLE   *DataTable,
  IN     UINT8                *IfrData,
  IN OUT UINT8                **ExportBufferPtr
  )
/*++

Routine Description:
  
Arguments:

Returns: 

--*/
{
  UINT8 *ExportBuffer;

  ExportBuffer = *ExportBufferPtr;

  //
  // BUGBUG - don't have devicepath data yet, setting dummy value
  //
  DataTable++;
  ExportBuffer  = (UINT8 *) DataTable;
  ((EFI_HII_DEVICE_PATH_PACK *) ExportBuffer)->Header.Type = EFI_HII_DEVICE_PATH;
  ((EFI_HII_DEVICE_PATH_PACK *) ExportBuffer)->Header.Length = (UINT32) (sizeof (EFI_HII_DEVICE_PATH_PACK) + sizeof (EFI_DEVICE_PATH_PROTOCOL));

  //
  // BUGBUG - part of hack - skip the Device Path Pack.....place some data
  //
  ExportBuffer  = ExportBuffer + sizeof (EFI_HII_DEVICE_PATH_PACK);

  ((EFI_DEVICE_PATH_PROTOCOL *) ExportBuffer)->Type     = EFI_END_ENTIRE_DEVICE_PATH;
  ((EFI_DEVICE_PATH_PROTOCOL *) ExportBuffer)->SubType  = EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;

  //
  // BUGBUG - still part of hack....
  //
  ExportBuffer      = ExportBuffer + sizeof (EFI_DEVICE_PATH_PROTOCOL);
  *ExportBufferPtr  = ExportBuffer;
}

STATIC
VOID
ExtractVariableData (
  IN OUT EFI_HII_DATA_TABLE   *DataTable,
  IN     UINT8                *IfrData,
  IN OUT UINT8                **ExportBufferPtr
  )
/*++

Routine Description:

  This function extract the EFI_HII_VARIABLE_PACK portion from the 
  each of the EFI_HII_PACKAGE_INSTANCE in HII handle database.
  
Arguments:

  DataTable       - On input, this parameter point to the EFI_HII_DATA_TABLE structure
                    of the final data buffer for the EFI_HII_EXPORT interface. This function
                    update the NumberOfVariableData attribute.
  IfrData         - It points to a staring address of a EFI_HII_IFR_PACK structure.
  ExportBufferPtr - On input, it points the starting address of the data buffer to 
                    host the variable pack. On output, it is the starting address
                    of data buffer for the next extraction operation.
Returns: 

  VOID
  
--*/
{
  EFI_HII_VARIABLE_PACK       *VariableContents;
  UINT8                       *ExportBuffer;
  UINTN                       Index;
  UINTN                       Index2;
  UINTN                       TempValue;
  UINTN                       TempValue2;
  EFI_FORM_CALLBACK_PROTOCOL  *FormCallback;
  EFI_PHYSICAL_ADDRESS        CallbackHandle;
  EFI_STATUS                  Status;
  CHAR16                      *String;

  FormCallback    = NULL;
  CallbackHandle  = 0;
  ExportBuffer    = *ExportBufferPtr;

  for (Index = 0; IfrData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    VariableContents = (EFI_HII_VARIABLE_PACK *) ExportBuffer;

    switch (IfrData[Index]) {
    case EFI_IFR_FORM_SET_OP:
      TempValue = EFI_HII_VARIABLE;
      CopyMem (&VariableContents->Header.Type, &TempValue, sizeof (UINT16));
      CopyMem (&TempValue, &((EFI_IFR_FORM_SET *) &IfrData[Index])->NvDataSize, sizeof (UINT16));

      //
      // If the variable has 0 size, do not process it
      //
      if (TempValue == 0) {
        break;
      }
      //
      // Add the size of the variable pack overhead.  Later, will also add the size of the
      // name of the variable.
      //
      TempValue = TempValue + sizeof (EFI_HII_VARIABLE_PACK);

      CopyMem (&VariableContents->Header.Length, &TempValue, sizeof (UINT32));
      CopyMem (
        &CallbackHandle,
        &((EFI_IFR_FORM_SET *) &IfrData[Index])->CallbackHandle,
        sizeof (EFI_PHYSICAL_ADDRESS)
        );
      if (CallbackHandle != 0) {
        Status = gBS->HandleProtocol (
                        (EFI_HANDLE) (UINTN) CallbackHandle,
                        &gEfiFormCallbackProtocolGuid,
                        (VOID *) &FormCallback
                        );
        ASSERT_EFI_ERROR (Status);
      }
      //
      // Since we have a "Setup" variable that wasn't specified by a variable op-code
      // it will have a VariableId of 0.  All other variable op-codes will have a designation
      // of VariableId 1+
      //
      TempValue = 0;
      CopyMem (&VariableContents->VariableId, &TempValue, sizeof (UINT16));
      CopyMem (&VariableContents->VariableGuid, &((EFI_IFR_FORM_SET *) &IfrData[Index])->Guid, sizeof (EFI_GUID));
      TempValue = sizeof (SETUP_MAP_NAME);
      CopyMem (&VariableContents->VariableNameLength, &TempValue, sizeof (UINT32));

      //
      // Add the size of the name to the Header Length
      //
      TempValue2 = 0;
      CopyMem (&TempValue2, &VariableContents->Header.Length, sizeof (UINT32));
      TempValue2 = TempValue + TempValue2;
      CopyMem (&VariableContents->Header.Length, &TempValue2, sizeof (UINT32));

      ExportBuffer = ExportBuffer + sizeof (EFI_HII_VARIABLE_PACK);
      CopyMem (ExportBuffer, SETUP_MAP_NAME, sizeof (SETUP_MAP_NAME));
      ExportBuffer = ExportBuffer + sizeof (SETUP_MAP_NAME);

      CopyMem (&TempValue, &((EFI_IFR_FORM_SET *) &IfrData[Index])->NvDataSize, sizeof (UINT16));

      if ((FormCallback != NULL) && (FormCallback->NvRead != NULL)) {
        Status = FormCallback->NvRead (
                                 FormCallback,
                                 (CHAR16 *) SETUP_MAP_NAME,
                                 (EFI_GUID *)(UINTN)&VariableContents->VariableGuid,
                                 NULL,
                                 &TempValue,
                                 ExportBuffer
                                 );
        ASSERT_EFI_ERROR (Status);
      } else {
        Status = gRT->GetVariable (
                        (CHAR16 *) SETUP_MAP_NAME,
                        (EFI_GUID *)(UINTN)&VariableContents->VariableGuid,
                        NULL,
                        &TempValue,
                        ExportBuffer
                        );
        ASSERT_EFI_ERROR (Status);
      }

      ExportBuffer = (UINT8 *) (UINTN) (((UINTN) ExportBuffer) + TempValue);
      DataTable->NumberOfVariableData++;
      break;

    case EFI_IFR_VARSTORE_OP:
      TempValue = EFI_HII_VARIABLE;
      CopyMem (&VariableContents->Header.Type, &TempValue, sizeof (UINT16));
      CopyMem (&TempValue, &((EFI_IFR_VARSTORE *) &IfrData[Index])->Size, sizeof (UINT16));

      //
      // If the variable has 0 size, do not process it
      //
      if (TempValue == 0) {
        break;
      }
      //
      // Add the size of the variable pack overhead.  Later, will also add the size of the
      // name of the variable.
      //
      TempValue = TempValue + sizeof (EFI_HII_VARIABLE_PACK);

      CopyMem (&VariableContents->Header.Length, &TempValue, sizeof (UINT32));
      CopyMem (&VariableContents->VariableId, &((EFI_IFR_VARSTORE *) &IfrData[Index])->VarId, sizeof (UINT16));
      CopyMem (&VariableContents->VariableGuid, &((EFI_IFR_VARSTORE *) &IfrData[Index])->Guid, sizeof (EFI_GUID));
      TempValue = (UINTN) ((EFI_IFR_VARSTORE *) &IfrData[Index])->Header.Length - sizeof (EFI_IFR_VARSTORE);
      TempValue = TempValue * 2;
      CopyMem (&VariableContents->VariableNameLength, &TempValue, sizeof (UINT32));

      //
      // Add the size of the name to the Header Length
      //
      TempValue2 = 0;
      CopyMem (&TempValue2, &VariableContents->Header.Length, sizeof (UINT32));
      TempValue2 = TempValue + TempValue2;
      CopyMem (&VariableContents->Header.Length, &TempValue2, sizeof (UINT32));

      ExportBuffer  = ExportBuffer + sizeof (EFI_HII_VARIABLE_PACK);
      String        = (CHAR16 *) ExportBuffer;
      for (Index2 = 0; Index2 < TempValue / 2; Index2++) {
        ExportBuffer[Index2 * 2]      = IfrData[Index + sizeof (EFI_IFR_VARSTORE) + Index2];
        ExportBuffer[Index2 * 2 + 1]  = 0;
      }

      ExportBuffer = ExportBuffer + TempValue;

      CopyMem (&TempValue, &((EFI_IFR_VARSTORE *) &IfrData[Index])->Size, sizeof (UINT16));

      if ((FormCallback != NULL) && (FormCallback->NvRead != NULL)) {
        Status = FormCallback->NvRead (
                                 FormCallback,
                                 String,
                                 (EFI_GUID *)(UINTN)&VariableContents->VariableGuid,
                                 NULL,
                                 &TempValue,
                                 ExportBuffer
                                 );
        ASSERT_EFI_ERROR (Status);
      } else {
        Status = gRT->GetVariable (
                        String,
                        (EFI_GUID *)(UINTN)&VariableContents->VariableGuid,
                        NULL,
                        &TempValue,
                        ExportBuffer
                        );
        ASSERT_EFI_ERROR (Status);
      }

      ExportBuffer = (UINT8 *) (UINTN) (((UINTN) ExportBuffer) + TempValue);
      DataTable->NumberOfVariableData++;
      break;
    }

    Index = IfrData[Index + 1] + Index;
  }
  //
  // If we have added a variable pack, add a dummy empty one to signify the end
  //
  if (ExportBuffer != *ExportBufferPtr) {
    VariableContents  = (EFI_HII_VARIABLE_PACK *) ExportBuffer;
    TempValue         = EFI_HII_VARIABLE;
    CopyMem (&VariableContents->Header.Type, &TempValue, sizeof (UINT16));
    TempValue = sizeof (EFI_HII_VARIABLE_PACK);
    CopyMem (&VariableContents->Header.Length, &TempValue, sizeof (UINT32));
    ExportBuffer = ExportBuffer + sizeof (EFI_HII_VARIABLE_PACK);
  }

  *ExportBufferPtr = ExportBuffer;
}

EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     EFI_HII_HANDLE   Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  )
/*++

Routine Description:
  
  This function allows a program to extract a form or form package that has 
  previously been registered with the EFI HII database.

Arguments:

Returns: 

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_IFR_PACK          *FormPack;
  UINT8                     *RawData;
  UINT8                     *ExportBuffer;
  EFI_HII_EXPORT_TABLE      *ExportTable;
  EFI_HII_DATA_TABLE        *DataTable;
  BOOLEAN                   VariableExist;
  UINT16                    NumberOfHiiDataTables;
  UINTN                     SizeNeeded;
  UINTN                     Index;
  UINTN                     VariableSize;
  UINTN                     TempValue;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData               = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase        = HiiData->DatabaseHead;

  FormPack              = NULL;
  RawData               = NULL;
  PackageInstance       = NULL;
  NumberOfHiiDataTables = 0;
  VariableSize          = 0;
  TempValue             = 0;
  SizeNeeded            = sizeof (EFI_HII_EXPORT_TABLE);

  //
  // How many total tables are there?
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    if ((Handle != 0) && (Handle != HandleDatabase->Handle)) {
      continue;
    }

    VariableExist = FALSE;
    NumberOfHiiDataTables++;
    PackageInstance = HandleDatabase->Buffer;
    if (PackageInstance == NULL) {
      continue;
    }
    //
    // Extract Size of Export Package
    //
    SizeNeeded = SizeNeeded + PackageInstance->IfrSize 
                            + PackageInstance->StringSize
                            + sizeof (EFI_HII_DATA_TABLE)
                            + sizeof (EFI_HII_DEVICE_PATH_PACK);

    //
    // BUGBUG We aren't inserting Device path data yet
    //
    SizeNeeded = SizeNeeded + sizeof (EFI_DEVICE_PATH_PROTOCOL);

    //
    // Extract Size of Variable Data
    //
    if (PackageInstance->IfrSize > 0) {
      FormPack = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));
    } else {
      //
      // No IFR? No variable information
      //
      continue;
    }

    RawData = (UINT8 *) FormPack;

    for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
      switch (RawData[Index]) {
      case EFI_IFR_FORM_SET_OP:
        CopyMem (&VariableSize, &((EFI_IFR_FORM_SET *) &RawData[Index])->NvDataSize, sizeof (UINT16));
        SizeNeeded    = SizeNeeded + VariableSize + sizeof (SETUP_MAP_NAME) + sizeof (EFI_HII_VARIABLE_PACK);
        VariableExist = TRUE;
        break;

      case EFI_IFR_VARSTORE_OP:
        CopyMem (&VariableSize, &((EFI_IFR_VARSTORE *) &RawData[Index])->Size, sizeof (UINT16));
        SizeNeeded = SizeNeeded + VariableSize + sizeof (EFI_HII_VARIABLE_PACK);
        //
        // We will be expanding the stored ASCII name to a Unicode string.  This will cause some memory overhead
        // Since the VARSTORE size already takes in consideration the ASCII size, we need to size it and add another
        // instance of it.  Essentially, 2 ASCII strings == 1 Unicode string in size.
        //
        TempValue     = (UINTN) ((EFI_IFR_VARSTORE *) &RawData[Index])->Header.Length - sizeof (EFI_IFR_VARSTORE);
        SizeNeeded    = SizeNeeded + TempValue * 2;
        VariableExist = TRUE;
        break;
      }

      Index = RawData[Index + 1] + Index;
    }
    //
    // If a variable exists for this handle, add an additional variable pack overhead to
    // indicate that we will have an extra null Variable Pack to signify the end of the Variable Packs
    //
    if (VariableExist) {
      SizeNeeded = SizeNeeded + sizeof (EFI_HII_VARIABLE_PACK);
    }
  }

  if (SizeNeeded > *BufferSize) {
    *BufferSize = SizeNeeded;
    return EFI_BUFFER_TOO_SMALL;
  }
  //
  // Zero out the incoming buffer
  //
  ZeroMem (Buffer, *BufferSize);

  //
  // Cast the Buffer to EFI_HII_EXPORT_TABLE
  //
  ExportTable = (EFI_HII_EXPORT_TABLE *) Buffer;

  //
  // Set the Revision for the Export Table
  //
  CopyMem (&ExportTable->Revision, &gEfiHiiProtocolGuid, sizeof (EFI_GUID));

  ExportBuffer    = (UINT8 *) (UINTN) (((UINT8 *) ExportTable) + sizeof (EFI_HII_EXPORT_TABLE));
  HandleDatabase  = HiiData->DatabaseHead;

  //
  // Check numeric value against the head of the database
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    DataTable       = (EFI_HII_DATA_TABLE *) ExportBuffer;
    PackageInstance = HandleDatabase->Buffer;
    //
    // If not asking for a specific handle, export the entire database
    //
    if (Handle == 0) {
      ExportTable->NumberOfHiiDataTables = NumberOfHiiDataTables;
      CopyMem (&DataTable->PackageGuid, &PackageInstance->Guid, sizeof (EFI_GUID));
      DataTable->HiiHandle        = PackageInstance->Handle;
      DataTable->DevicePathOffset = (UINT32) (sizeof (EFI_HII_DATA_TABLE));

      //
      // Start Dumping DevicePath
      //
      ExtractDevicePathData (DataTable, RawData, &ExportBuffer);

      if (((UINTN) ExportBuffer) == ((UINTN) DataTable)) {
        //
        // If there is no DevicePath information - set offset to 0 to signify the absence of data to parse
        //
        DataTable->DevicePathOffset = 0;
      }

      DataTable->VariableDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

      if (PackageInstance->IfrSize > 0) {
        FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));

        RawData   = (UINT8 *) FormPack;
        TempValue = 0;

        //
        // Start dumping the Variable Data
        //
        ExtractVariableData (DataTable, RawData, &ExportBuffer);
        DataTable->IfrDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

        if (DataTable->VariableDataOffset == DataTable->IfrDataOffset) {
          DataTable->VariableDataOffset = 0;
        }
        //
        // Start dumping the IFR data (Note:  It is in an IFR PACK)
        //
        CopyMem (ExportBuffer, &PackageInstance->IfrData, PackageInstance->IfrSize);
        ExportBuffer                = (UINT8 *) (UINTN) (((UINTN) ExportBuffer) + PackageInstance->IfrSize);
        DataTable->StringDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

        //
        // Start dumping the String data (Note:  It is in a String PACK)
        //
        if (PackageInstance->StringSize > 0) {
          RawData = (UINT8 *) (((UINTN) &PackageInstance->IfrData) + PackageInstance->IfrSize);
          CopyMem (ExportBuffer, RawData, PackageInstance->StringSize);
          DataTable->DataTableSize = (UINT32) (DataTable->StringDataOffset + PackageInstance->StringSize);

          CopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
          for (; TempValue != 0;) {
            DataTable->NumberOfLanguages++;
            ExportBuffer = ExportBuffer + ((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length;
            CopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
          }

          ExportBuffer = ExportBuffer + sizeof (EFI_HII_STRING_PACK);
        } else {
          DataTable->StringDataOffset = 0;
        }
      } else {
        //
        // No IFR? No variable information.  If Offset is 0, means there is none.  (Hmm - this might be prunable - no strings to export if no IFR - we always have a stub)
        //
        DataTable->VariableDataOffset = 0;
        DataTable->IfrDataOffset      = 0;
        DataTable->StringDataOffset   = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

        //
        // Start dumping the String data - NOTE:  It is in String Pack form
        //
        if (PackageInstance->StringSize > 0) {
          RawData = (UINT8 *) (((UINTN) &PackageInstance->IfrData) + PackageInstance->IfrSize);
          CopyMem (ExportBuffer, RawData, PackageInstance->StringSize);
          DataTable->DataTableSize = (UINT32) (DataTable->StringDataOffset + PackageInstance->StringSize);

          CopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
          for (; TempValue != 0;) {
            DataTable->NumberOfLanguages++;
            ExportBuffer = ExportBuffer + ((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length;
            CopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
          }

          ExportBuffer = ExportBuffer + sizeof (EFI_HII_STRING_PACK);
        } else {
          DataTable->StringDataOffset = 0;
        }
      }
    } else {
      //
      // Match the numeric value with the database entry - if matched, extract PackageInstance
      //
      if (Handle == HandleDatabase->Handle) {
        PackageInstance                     = HandleDatabase->Buffer;
        ExportTable->NumberOfHiiDataTables  = NumberOfHiiDataTables;
        DataTable->HiiHandle                = PackageInstance->Handle;
        CopyMem (&DataTable->PackageGuid, &PackageInstance->Guid, sizeof (EFI_GUID));

        //
        // Start Dumping DevicePath
        //
        ExtractDevicePathData (DataTable, RawData, &ExportBuffer);
        DataTable->VariableDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

        if (PackageInstance->IfrSize > 0) {
          FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));

          RawData   = (UINT8 *) FormPack;
          TempValue = 0;

          //
          // Start dumping the Variable Data
          //
          ExtractVariableData (DataTable, RawData, &ExportBuffer);
          DataTable->IfrDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

          if (DataTable->VariableDataOffset == DataTable->IfrDataOffset) {
            DataTable->VariableDataOffset = 0;
          }
          //
          // Start dumping the IFR data
          //
          CopyMem (ExportBuffer, &PackageInstance->IfrData, PackageInstance->IfrSize);
          ExportBuffer                = (UINT8 *) (UINTN) (((UINTN) ExportBuffer) + PackageInstance->IfrSize);
          DataTable->StringDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

          //
          // Start dumping the String data - NOTE:  It is in String Pack form
          //
          if (PackageInstance->StringSize > 0) {
            RawData = (UINT8 *) (((UINTN) &PackageInstance->IfrData) + PackageInstance->IfrSize);
            CopyMem (ExportBuffer, RawData, PackageInstance->StringSize);
            DataTable->DataTableSize = (UINT32) (DataTable->StringDataOffset + PackageInstance->StringSize);

            CopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
            for (; TempValue != 0;) {
              DataTable->NumberOfLanguages++;
              ExportBuffer = ExportBuffer + ((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length;
              CopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
            }

            ExportBuffer = ExportBuffer + sizeof (EFI_HII_STRING_PACK);
          } else {
            DataTable->StringDataOffset = 0;
          }
        } else {
          //
          // No IFR? No variable information.  If Offset is 0, means there is none.
          //
          DataTable->VariableDataOffset = 0;
          DataTable->IfrDataOffset      = 0;
          DataTable->StringDataOffset   = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

          //
          // Start dumping the String data - Note:  It is in String Pack form
          //
          if (PackageInstance->StringSize > 0) {
            RawData = (UINT8 *) (((UINTN) &PackageInstance->IfrData) + PackageInstance->IfrSize);
            CopyMem (ExportBuffer, RawData, PackageInstance->StringSize);
            DataTable->DataTableSize = (UINT32) (DataTable->StringDataOffset + PackageInstance->StringSize);

            CopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
            for (; TempValue != 0;) {
              DataTable->NumberOfLanguages++;
              ExportBuffer = ExportBuffer + ((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length;
              CopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
            }

            ExportBuffer = ExportBuffer + sizeof (EFI_HII_STRING_PACK);
          } else {
            DataTable->StringDataOffset = 0;
          }
        }
        break;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiGetForms (
  IN     EFI_HII_PROTOCOL   *This,
  IN     EFI_HII_HANDLE     Handle,
  IN     EFI_FORM_ID        FormId,
  IN OUT UINTN              *BufferLengthTemp,
  OUT    UINT8              *Buffer
  )
/*++

Routine Description:
  
  This function allows a program to extract a form or form package that has 
  previously been registered with the EFI HII database.

Arguments:
  This         - A pointer to the EFI_HII_PROTOCOL instance.
  
  Handle       - Handle on which the form resides. Type EFI_HII_HANDLE is defined in 
                 EFI_HII_PROTOCOL.NewPack() in the Packages section.
            
  FormId       - The ID of the form to return. If the ID is zero, the entire form package is returned.
                 Type EFI_FORM_ID is defined in "Related Definitions" below.
            
  BufferLength - On input, the length of the Buffer. On output, the length of the returned buffer, if
                 the length was sufficient and, if it was not, the length that is required to fit the
                 requested form(s).
                  
  Buffer       - The buffer designed to receive the form(s).

Returns: 

  EFI_SUCCESS           -  Buffer filled with the requested forms. BufferLength
                           was updated.
                           
  EFI_INVALID_PARAMETER -  The handle is unknown.
  
  EFI_NOT_FOUND         -  A form on the requested handle cannot be found with the
                           requested FormId.
                           
  EFI_BUFFER_TOO_SMALL  - The buffer provided was not large enough to allow the form to be stored.

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_IFR_PACK          *FormPack;
  EFI_IFR_FORM              *Form;
  EFI_IFR_OP_HEADER         *Location;
  UINT16                    *BufferLength = (UINT16 *) BufferLengthTemp;
  UINTN                     FormLength;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase  = HiiData->DatabaseHead;

  PackageInstance = NULL;

  FormLength      = 0;

  //
  // Check numeric value against the head of the database
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    //
    // Match the numeric value with the database entry - if matched, extract PackageInstance
    //
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
      break;
    }
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_NOT_FOUND;
  }
  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  if (PackageInstance->IfrSize > 0) {
    FormPack = (EFI_HII_IFR_PACK *) (&PackageInstance->IfrData);
  } else {
    //
    // If there is no IFR data return an error
    //
    return EFI_NOT_FOUND;
  }
  //
  // If requesting the entire Form Package
  //
  if (FormId == 0) {
    //
    // Return an error if buffer is too small
    //
    if (PackageInstance->IfrSize > *BufferLength || Buffer == NULL) {
      *BufferLength = (UINT16) PackageInstance->IfrSize;
      return EFI_BUFFER_TOO_SMALL;
    }

    CopyMem (Buffer, FormPack, PackageInstance->IfrSize);
    return EFI_SUCCESS;
  } else {
    FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));
    Location  = (EFI_IFR_OP_HEADER *) FormPack;

    //
    // Look for the FormId requested
    //
    for (; Location->OpCode != EFI_IFR_END_FORM_SET_OP;) {
      switch (Location->OpCode) {
      case EFI_IFR_FORM_OP:
        Form = (EFI_IFR_FORM *) Location;

        //
        // If we found a Form Op-code and it is of the correct Id, copy it and return
        //
        if (Form->FormId == FormId) {
          //
          // Calculate the total size of form
          //
          for (FormLength = 0; Location->OpCode != EFI_IFR_END_FORM_OP; ) {
            FormLength += Location->Length;
            Location    = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
          }
          FormLength += Location->Length;
          Location    = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);

          if ((Buffer == NULL) || (FormLength > *BufferLength)) {
            *BufferLengthTemp = FormLength;
            return EFI_BUFFER_TOO_SMALL;
          }
          
          //
          // Rewind to start offset of the found Form
          //
          Location   = (EFI_IFR_OP_HEADER *) ((CHAR8 *)Location - FormLength);
          CopyMem (Buffer, Location, FormLength);
          return EFI_SUCCESS;
        }

      default:
        break;
      }
      //
      // Go to the next Op-Code
      //
      Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
    }
  }

  return EFI_NOT_FOUND;
}

//
// Helper functions to HiiGetDefaultImage()
//

STATIC
UINT8*
HiiGetDefaultImageInitPack (
  IN OUT EFI_HII_VARIABLE_PACK_LIST  *VariablePackItem,
  IN     EFI_IFR_VARSTORE            *VarStore
  )
/*++
    
  Routine Description:

    Initialize the EFI_HII_VARIABLE_PACK_LIST structure and
    prepare it ready to be used by HiiGetDefaultImagePopulateMap ().
      
  Arguments:

    VariablePackItem     - Variable Package List.
    VarStore             - IFR variable storage.
   
  Returns: 

    Return the pointer to the Map space.
      
--*/
{
  CHAR16                *Name16;
  CHAR8                 *Name8;
  CHAR8                 *Map;
  EFI_HII_VARIABLE_PACK *VariablePack;

  //
  // Set pointer the pack right after the node
  //
  VariablePackItem->VariablePack = (EFI_HII_VARIABLE_PACK *) (VariablePackItem + 1);
  VariablePack                   = VariablePackItem->VariablePack;

  //
  // Copy the var name to VariablePackItem from VarStore
  // Needs ASCII->Unicode conversion.
  //
  ASSERT (VarStore->Header.Length > sizeof (*VarStore));
  Name8  = (CHAR8 *) (VarStore + 1);
  Name16 = (CHAR16 *) (VariablePack + 1);
  Ascii2Unicode (Name16, Name8);

  //
  // Compute the other fields of the VariablePackItem
  //
  VariablePack->VariableId         = VarStore->VarId;
  CopyMem (&VariablePack->VariableGuid, &VarStore->Guid, sizeof (EFI_GUID));
  VariablePack->VariableNameLength = (UINT32) ((StrLen (Name16) + 1) * 2);
  VariablePack->Header.Length      = sizeof (*VariablePack) 
                                              + VariablePack->VariableNameLength
                                              + VarStore->Size;
  //
  // Return the pointer to the Map space.
  //
  Map = (CHAR8 *) Name16 + VariablePack->VariableNameLength;

  return (UINT8 *)Map;
}

STATIC
VOID
HiiGetDefaultImagePopulateMap (
  IN OUT UINT8                        *Map,  
  IN     EFI_IFR_OP_HEADER            *FormSet,
  IN     EFI_IFR_VARSTORE             *VarStore,
  IN     UINTN                        DefaultMask
  )
/*++
    
  Routine Description:

   Fill the Map with all the default values either from NV or Hii database.
      
  Arguments:

   Map         - Memory pointer to hold the default values.
   FormSet     - The starting EFI_IFR_OP_HEADER to begin retriving default values.
   VarStore    - IFR variable storage.
   DefaultMask - The mask used to get the default variable.
   
  Returns: 

   VOID
      
--*/
{
  EFI_STATUS                     Status;
  EFI_IFR_OP_HEADER              *IfrItem;
  UINT16                         VarId;
  EFI_IFR_VARSTORE_SELECT        *VarSelect;
  EFI_IFR_ONE_OF_OPTION          *OneOfOpt;
  EFI_IFR_CHECKBOX               *CheckBox;
  EFI_IFR_NUMERIC                *Numeric;
  UINTN                          Size;
  UINTN                          SizeTmp;
  EFI_IFR_NV_DATA                *IfrNvData;
  EFI_GUID                       Guid;
  CHAR16                         *Name16;
  CHAR8                          *Name8;  
  EFI_HANDLE                      CallbackHandle;
  EFI_FORM_CALLBACK_PROTOCOL     *FormCallbackProt;

  //
  // Get the Map's Name/Guid/Szie from the Varstore.
  // VARSTORE contains the Name in ASCII format (@#$^&!), must convert it to Unicode.
  //
  ASSERT (VarStore->Header.Length >= sizeof (*VarStore));
  Name8  = (CHAR8 *) (VarStore + 1);
  Name16 = AllocateZeroPool ((VarStore->Header.Length - sizeof (*VarStore)) * sizeof (CHAR16));
  Ascii2Unicode (Name16, Name8);
  CopyMem (&Guid, &VarStore->Guid, sizeof(EFI_GUID));
  Size = VarStore->Size;

  //
  // First, check if the map exists in the NV. If so, get it from NV and exit.
  //
  if (DefaultMask == EFI_IFR_FLAG_MANUFACTURING) {
    //
    // Check if Manufaturing Defaults exist in the NV.
    //
    Status = EfiLibHiiVariableOverrideBySuffix (
                  HII_VARIABLE_SUFFIX_MANUFACTURING_OVERRIDE,
                  Name16,
                  &Guid,
                  Size,
                  Map
                  );
  } else {
    //
    // All other cases default to Defaults. Check if Defaults exist in the NV.
    //
    Status = EfiLibHiiVariableOverrideBySuffix (
                  HII_VARIABLE_SUFFIX_DEFAULT_OVERRIDE,
                  Name16,
                  &Guid,
                  Size,
                  Map
                  );
  }
  if (!EFI_ERROR (Status)) {
    //
    // Either Defaults/Manufacturing variable exists and appears to be valid. 
    // The map is read, exit w/ success now.
    //
    FreePool (Name16);
    return;
  }

  //
  // First, prime the map with what already is in the NV.
  // This is needed to cover a situation where the IFR does not contain all the 
  // defaults; either deliberately not having appropriate IFR, or in case of IFR_STRING, there is no default.
  // Ignore status. Either it gets read or not. 
  //  
  FormCallbackProt = NULL;
  CopyMem (&CallbackHandle, &((EFI_IFR_FORM_SET*) FormSet)->CallbackHandle, sizeof (CallbackHandle));
  if (CallbackHandle != NULL) {
    Status = gBS->HandleProtocol (
                    (EFI_HANDLE) (UINTN) CallbackHandle,
                    &gEfiFormCallbackProtocolGuid,
                    (VOID *) &FormCallbackProt
                    );
  }
  if ((NULL != FormCallbackProt) && (NULL != FormCallbackProt->NvRead)) {
    //
    // Attempt to read using NvRead() callback. Probe first for existence and correct variable size.
    //
    SizeTmp = 0;
    Status = FormCallbackProt->NvRead (
                    FormCallbackProt,
                    Name16,
                    &Guid,
                    0,
                    &SizeTmp,
                    NULL
                    );
    if ((EFI_BUFFER_TOO_SMALL == Status) && (SizeTmp == Size)) {
      Status = FormCallbackProt->NvRead (
                      FormCallbackProt,
                      Name16,
                      &Guid,
                      0,
                      &SizeTmp,
                      Map
                      );
      ASSERT_EFI_ERROR (Status);
      ASSERT (SizeTmp == Size);
    }
  } else {
    //
    // No callback available for this formset, read straight from NV. Deliberately ignore the Status. 
    // The buffer will only be written if variable exists nd has correct size.
    //
    Status = EfiLibHiiVariableRetrieveFromNv (
                    Name16,
                    &Guid,
                    Size,
                    (VOID **) &Map
                    );
  }

  //
  // Iterate all IFR statements and for applicable, retrieve the default into the Map.
  //
  for (IfrItem = FormSet, VarId = 0; 
       IfrItem->OpCode != EFI_IFR_END_FORM_SET_OP; 
       IfrItem = (EFI_IFR_OP_HEADER *) ((UINT8*) IfrItem + IfrItem->Length)
      ) {

    //
    // Observe VarStore switch.
    //
    if (EFI_IFR_VARSTORE_SELECT_OP == IfrItem->OpCode) {
      VarSelect = (EFI_IFR_VARSTORE_SELECT *) IfrItem;
      VarId = VarSelect->VarId;
      continue;
    }


    //
    // Skip opcodes that reference other VarStore than that specific to current map.
    // 
    if (VarId != VarStore->VarId) {
      continue;
    }
    
    //
    // Extract the default value from this opcode if applicable, and apply it to the map.
    //
    IfrNvData = (EFI_IFR_NV_DATA *) IfrItem;
    switch (IfrItem->OpCode) {

      case EFI_IFR_ONE_OF_OP:
        ASSERT (IfrNvData->QuestionId + IfrNvData->StorageWidth <= VarStore->Size);
        //
        // Get to the first EFI_IFR_ONE_OF_OPTION_OP
        //
        IfrItem = (EFI_IFR_OP_HEADER *) ((UINT8*) IfrItem + IfrItem->Length); 
        ASSERT (EFI_IFR_ONE_OF_OPTION_OP == IfrItem->OpCode);

        OneOfOpt = (EFI_IFR_ONE_OF_OPTION *)IfrItem;
        //
        // In the worst case, the first will be the default.
        //
        CopyMem (Map + IfrNvData->QuestionId, &OneOfOpt->Value, IfrNvData->StorageWidth);

        while (EFI_IFR_ONE_OF_OPTION_OP == IfrItem->OpCode) {

          OneOfOpt = (EFI_IFR_ONE_OF_OPTION *)IfrItem;
            if (DefaultMask == EFI_IFR_FLAG_MANUFACTURING) {
              if (0 != (OneOfOpt->Flags & EFI_IFR_FLAG_MANUFACTURING)) {
                //
                // In the worst case, the first will be the default.
                //
                CopyMem (Map + IfrNvData->QuestionId, &OneOfOpt->Value, IfrNvData->StorageWidth);
                break;
              }
            } else {
              if (OneOfOpt->Flags & EFI_IFR_FLAG_DEFAULT) {
                //
                // In the worst case, the first will be the default.
                //
                CopyMem (Map + IfrNvData->QuestionId, &OneOfOpt->Value, IfrNvData->StorageWidth);
                break;
              }
            }

          IfrItem = (EFI_IFR_OP_HEADER *)((UINT8*)IfrItem + IfrItem->Length);
        }
        continue;
        break;

      case EFI_IFR_CHECKBOX_OP:
        ASSERT (IfrNvData->QuestionId + IfrNvData->StorageWidth <= VarStore->Size);
        CheckBox = (EFI_IFR_CHECK_BOX *)IfrItem;        
        if (DefaultMask == EFI_IFR_FLAG_MANUFACTURING) {
          if (0 != (CheckBox->Flags & EFI_IFR_FLAG_MANUFACTURING)) {
            *(UINT8 *) (Map + IfrNvData->QuestionId) = TRUE;
          }
        } else {
          if (CheckBox->Flags & EFI_IFR_FLAG_DEFAULT) {
            *(UINT8 *) (Map + IfrNvData->QuestionId) = TRUE;
          }
        }
        break;

      case EFI_IFR_NUMERIC_OP:
        ASSERT (IfrNvData->QuestionId + IfrNvData->StorageWidth <= VarStore->Size);
        Numeric = (EFI_IFR_NUMERIC *) IfrItem;
        CopyMem (Map + IfrNvData->QuestionId, &Numeric->Default, IfrNvData->StorageWidth);
        break;

      case EFI_IFR_ORDERED_LIST_OP:
      case EFI_IFR_PASSWORD_OP:
      case EFI_IFR_STRING_OP:
        //
        // No support for default value for these opcodes.
        //
      break;
    }
  }

  FreePool (Name16);

}


EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL            *This,
  IN     EFI_HII_HANDLE              Handle,
  IN     UINTN                       DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  )
/*++
    
  Routine Description:

  This function allows a program to extract the NV Image 
  that represents the default storage image
      
  Arguments:
    This             - A pointer to the EFI_HII_PROTOCOL instance.
    Handle           - The HII handle from which will have default data retrieved.
    UINTN            - Mask used to retrieve the default image.
    VariablePackList - Callee allocated, tightly-packed, link list data 
                         structure that contain all default varaible packs
                         from the Hii Database.
    
  Returns: 
    EFI_NOT_FOUND         - If Hii database does not contain any default images.
    EFI_INVALID_PARAMETER - Invalid input parameter.
    EFI_SUCCESS           - Operation successful.
      
--*/
{
  EFI_HII_HANDLE_DATABASE        *HandleDatabase;
  EFI_HII_PACKAGE_INSTANCE       *PackageInstance;
  EFI_IFR_OP_HEADER              *FormSet;
  EFI_IFR_OP_HEADER              *IfrItem;
  EFI_IFR_VARSTORE               *VarStore;
  EFI_IFR_VARSTORE               *VarStoreDefault;
  UINTN                          SetupMapNameSize;
  UINTN                          SizeOfMaps;
  EFI_HII_VARIABLE_PACK_LIST     *PackList;
  EFI_HII_VARIABLE_PACK_LIST     *PackListNext;  
  EFI_HII_VARIABLE_PACK_LIST     *PackListLast;
  UINT8                          *Map;


  //
  // Find the IFR pack from the handle. Then get the formset from the pack.
  //
  PackageInstance = NULL;
  HandleDatabase  = (EFI_HII_DATA_FROM_THIS (This))->DatabaseHead;
  for ( ; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
      break;
    }
  }
  if (PackageInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  FormSet = (EFI_IFR_OP_HEADER *) ((UINT8 *) &PackageInstance->IfrData + sizeof (EFI_HII_IFR_PACK));

  //
  // Get the sizes of all the VARSTOREs in this VFR.
  // Then allocate enough space for all of them plus all maps
  //
  SizeOfMaps = 0;
  IfrItem    = FormSet;
  while (EFI_IFR_END_FORM_SET_OP != IfrItem->OpCode) {

    if (EFI_IFR_VARSTORE_OP == IfrItem->OpCode) {
      VarStore = (EFI_IFR_VARSTORE *) IfrItem;
      //
      // Size of the map
      //
      SizeOfMaps += VarStore->Size; 
      //
      // add the size of the string, in Unicode
      //
      SizeOfMaps += (VarStore->Header.Length - sizeof (*VarStore)) * 2; 
      //
      // Space for node
      //
      SizeOfMaps += sizeof (EFI_HII_VARIABLE_PACK);      
      //
      // Space for linked list node 
      //
      SizeOfMaps += sizeof (EFI_HII_VARIABLE_PACK_LIST); 
    }

    IfrItem = (EFI_IFR_OP_HEADER *) ((UINT8 *) IfrItem + IfrItem->Length);
  }

  //
  // If the FormSet OpCode has a non-zero NvDataSize. There is a default 
  // NvMap with ID=0, GUID that of the formset itself and "Setup" as name.
  //
  SetupMapNameSize = StrLen (SETUP_MAP_NAME) + 1;
  VarStoreDefault  = AllocateZeroPool (sizeof (*VarStoreDefault) + SetupMapNameSize);

  if (0 != ((EFI_IFR_FORM_SET*)FormSet)->NvDataSize) {

    VarStoreDefault->Header.OpCode = EFI_IFR_VARSTORE_OP;
    VarStoreDefault->Header.Length = (UINT8) (sizeof (*VarStoreDefault) + SetupMapNameSize);
    Unicode2Ascii ((CHAR8 *) (VarStoreDefault + 1), SETUP_MAP_NAME);
    CopyMem (&VarStoreDefault->Guid, &((EFI_IFR_FORM_SET*) FormSet)->Guid, sizeof (EFI_GUID));
    VarStoreDefault->VarId = 0;
    VarStoreDefault->Size = ((EFI_IFR_FORM_SET*) FormSet)->NvDataSize;

    //
    // Size of the map
    //
    SizeOfMaps += VarStoreDefault->Size; 
    //
    // add the size of the string
    //
    SizeOfMaps += sizeof (SETUP_MAP_NAME); 
    //
    // Space for node
    //
    SizeOfMaps += sizeof (EFI_HII_VARIABLE_PACK);      
    //
    // Space for linked list node 
    //
    SizeOfMaps += sizeof (EFI_HII_VARIABLE_PACK_LIST); 
  }

  if (0 == SizeOfMaps) {
    //
    // The IFR does not have any explicit or default map(s).
    //
    return EFI_NOT_FOUND; 
  }

  //
  // Allocate the return buffer
  //
  PackList = AllocateZeroPool (SizeOfMaps);
  ASSERT (NULL != PackList); 

  PackListNext = PackList;
  PackListLast = PackList;

  //
  // Handle the default map first, if any.
  //
  if (0 != VarStoreDefault->Size) {

    Map = HiiGetDefaultImageInitPack (PackListNext, VarStoreDefault);

    HiiGetDefaultImagePopulateMap (Map, FormSet, VarStoreDefault, DefaultMask);

    PackListNext->NextVariablePack = (EFI_HII_VARIABLE_PACK_LIST *) ((UINT8 *) PackListNext->VariablePack + PackListNext->VariablePack->Header.Length);
    PackListLast = PackListNext;
    PackListNext = PackListNext->NextVariablePack;
  }


  //
  // Handle the explicit varstore(s)
  //
  IfrItem = FormSet;
  while (EFI_IFR_END_FORM_SET_OP != IfrItem->OpCode) {

    if (EFI_IFR_VARSTORE_OP == IfrItem->OpCode) {

      Map = HiiGetDefaultImageInitPack (PackListNext, (EFI_IFR_VARSTORE *) IfrItem);

      HiiGetDefaultImagePopulateMap (Map, FormSet, (EFI_IFR_VARSTORE *) IfrItem, DefaultMask);

      PackListNext->NextVariablePack = (EFI_HII_VARIABLE_PACK_LIST *) ((UINT8 *) PackListNext->VariablePack + PackListNext->VariablePack->Header.Length);
      PackListLast = PackListNext;
      PackListNext = PackListNext->NextVariablePack;
    }

    IfrItem = (EFI_IFR_OP_HEADER *) ((UINT8 *) IfrItem + IfrItem->Length);
  }

  PackListLast->NextVariablePack = NULL;
  *VariablePackList = PackList;
  
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_PROTOCOL       *This,
  IN EFI_HII_HANDLE         Handle,
  IN EFI_FORM_LABEL         Label,
  IN BOOLEAN                AddData,
  IN EFI_HII_UPDATE_DATA    *Data
  )
/*++

Routine Description:
  This function allows the caller to update a form that has 
  previously been registered with the EFI HII database.

Arguments:
  Handle     - Hii Handle associated with the Formset to modify
  Label      - Update information starting immediately after this label in the IFR
  AddData    - If TRUE, add data.  If FALSE, remove data
  Data       - If adding data, this is the pointer to the data to add

Returns: 
  EFI_SUCCESS - Update success.
  Other       - Update fail.

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_IFR_PACK          *FormPack;
  EFI_IFR_OP_HEADER         *Location;
  EFI_IFR_OP_HEADER         *DataLocation;
  UINT8                     *OtherBuffer;
  UINT8                     *TempBuffer;
  UINT8                     *OrigTempBuffer;
  UINTN                     TempBufferSize;
  UINTN                     Index;

  OtherBuffer = NULL;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase  = HiiData->DatabaseHead;

  PackageInstance = NULL;

  //
  // Check numeric value against the head of the database
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    //
    // Match the numeric value with the database entry - if matched, extract PackageInstance
    //
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
      break;
    }
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Calculate and allocate space for retrieval of IFR data
  //
  DataLocation    = (EFI_IFR_OP_HEADER *) &Data->Data;
  TempBufferSize  = (CHAR8 *) (&PackageInstance->IfrData) - (CHAR8 *) (PackageInstance);

  for (Index = 0; Index < Data->DataCount; Index++) {
    TempBufferSize += DataLocation->Length;
    DataLocation = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (DataLocation) + DataLocation->Length);
  }

  TempBufferSize += PackageInstance->IfrSize + PackageInstance->StringSize;

  TempBuffer      = AllocateZeroPool (TempBufferSize);
  ASSERT (TempBuffer != NULL);

  OrigTempBuffer  = TempBuffer;

  //
  // We update only packages with IFR information in it
  //
  if (PackageInstance->IfrSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (
    TempBuffer,
    PackageInstance,
    ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER) - (CHAR8 *) (PackageInstance))
    );

  TempBuffer = TempBuffer + ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER) - (CHAR8 *) (PackageInstance));

  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));
  Location  = (EFI_IFR_OP_HEADER *) FormPack;

  //
  // Look for the FormId requested
  //
  for (; Location->OpCode != EFI_IFR_END_FORM_SET_OP;) {
    switch (Location->OpCode) {
    case EFI_IFR_FORM_SET_OP:
      //
      // If the FormSet has an update pending, pay attention.
      //
      if (Data->FormSetUpdate) {
        ((EFI_IFR_FORM_SET *) Location)->CallbackHandle = Data->FormCallbackHandle;
      }

      CopyMem (TempBuffer, Location, Location->Length);
      TempBuffer = TempBuffer + Location->Length;
      break;

    case EFI_IFR_FORM_OP:
      //
      // If the Form has an update pending, pay attention.
      //
      if (Data->FormUpdate) {
        ((EFI_IFR_FORM *) Location)->FormTitle = Data->FormTitle;
      }

      CopyMem (TempBuffer, Location, Location->Length);
      TempBuffer = TempBuffer + Location->Length;
      break;

    case EFI_IFR_LABEL_OP:
      //
      // If the label does not match the requested update point, ignore it
      //
      if (((EFI_IFR_LABEL *) Location)->LabelId != Label) {
        //
        // Copy the label
        //
        CopyMem (TempBuffer, Location, Location->Length);
        TempBuffer = TempBuffer + Location->Length;

        //
        // Go to the next Op-Code
        //
        Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
        continue;
      }

      if (AddData) {
        //
        // Copy the label
        //
        CopyMem (TempBuffer, Location, Location->Length);
        TempBuffer = TempBuffer + Location->Length;

        //
        // Add the DataCount amount of opcodes to TempBuffer
        //
        DataLocation = (EFI_IFR_OP_HEADER *) &Data->Data;
        for (Index = 0; Index < Data->DataCount; Index++) {
          CopyMem (TempBuffer, DataLocation, DataLocation->Length);
          ((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->IfrSize += DataLocation->Length;
          OtherBuffer = ((UINT8 *) &((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->StringSize + sizeof (UINTN));
          CopyMem (OtherBuffer, &((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->IfrSize, 2);
          TempBuffer    = TempBuffer + DataLocation->Length;
          DataLocation  = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (DataLocation) + DataLocation->Length);
        }
        //
        // Go to the next Op-Code
        //
        Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
        continue;
      } else {
        //
        // Copy the label
        //
        CopyMem (TempBuffer, Location, Location->Length);
        TempBuffer  = TempBuffer + Location->Length;
        Location    = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);

        //
        // Remove the DataCount amount of opcodes unless we run into an end of form or a label
        //
        for (Index = 0; Index < Data->DataCount; Index++) {
          //
          // If we are about to skip an end form - bail out, since that is illegal
          //
          if ((Location->OpCode == EFI_IFR_END_FORM_OP) || (Location->OpCode == EFI_IFR_LABEL_OP)) {
            break;
          }
          //
          // By skipping Location entries, we are in effect not copying what was previously there
          //
          ((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->IfrSize -= Location->Length;
          OtherBuffer = ((UINT8 *) &((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->StringSize + sizeof (UINTN));
          CopyMem (OtherBuffer, &((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->IfrSize, 2);
          Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
        }
      }

    default:
      CopyMem (TempBuffer, Location, Location->Length);
      TempBuffer = TempBuffer + Location->Length;
      break;
    }
    //
    // Go to the next Op-Code
    //
    Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
  }
  //
  // Copy the last op-code left behind from the for loop
  //
  CopyMem (TempBuffer, Location, Location->Length);

  //
  // Advance to beginning of strings and copy them
  //
  TempBuffer  = TempBuffer + Location->Length;
  Location    = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
  CopyMem (TempBuffer, Location, PackageInstance->StringSize);

  //
  // Free the old buffer, and assign into our database the latest buffer
  //
  FreePool (HandleDatabase->Buffer);
  HandleDatabase->Buffer = OrigTempBuffer;

  return EFI_SUCCESS;
}
