/*++

Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscBiosVendorFunction.c

Abstract:

  BIOS vendor information boot time changes.
  Misc. subclass type 2.
  SMBIOS type 0.

--*/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"
#include <Library/BiosIdLib.h>
#include <Library/SpiFlash.H>

EFI_SPI_PROTOCOL  *mSpiProtocol = NULL;


/**
  This function read the data from Spi Rom.

  @param BaseAddress                The starting address of the read.
  @param Byte                       The pointer to the destination buffer.
  @param Length                     The number of bytes.
  @param SpiRegionType              Spi Region Type.

  @retval Status

**/
EFI_STATUS
FlashRead (
  IN  UINTN             BaseAddress,
  IN  UINT8             *Byte,
  IN  UINTN             Length,
  IN  SPI_REGION_TYPE   SpiRegionType
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT32                SectorSize;
  UINT32                SpiAddress;
  UINT8                 Buffer[SECTOR_SIZE_4KB];

  SpiAddress = (UINT32)(UINTN)(BaseAddress);
  SectorSize = SECTOR_SIZE_4KB;

  Status = mSpiProtocol->Execute (
                           mSpiProtocol,
                           SPI_READ,
                           SPI_WREN,
                           TRUE,
                           TRUE,
                           FALSE,
                           (UINT32) SpiAddress,
                           SectorSize,
                           Buffer,
                           SpiRegionType
                           );

  if (EFI_ERROR (Status)) {
#ifdef _SHOW_LOG_
    Print(L"Read SPI ROM Failed [%08x]\n", SpiAddress);
#endif
    return Status;
  }

  CopyMem (Byte, (void *)Buffer, Length);

  return Status;
}

/**
  This function returns the value & exponent to Base2 for a given
  Hex value. This is used to calculate the BiosPhysicalDeviceSize.

  @param Value                      The hex value which is to be converted into value-exponent form
  @param Exponent                   The exponent out of the conversion

  @retval EFI_SUCCESS               All parameters were valid and *Value & *Exponent have been set.
  @retval EFI_INVALID_PARAMETER     Invalid parameter was found.

**/
EFI_STATUS
GetValueExponentBase2(
  IN OUT UINTN        *Value,
  OUT    UINTN        *Exponent
  )
{
  if ((Value == NULL) || (Exponent == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  while ((*Value % 2) == 0) {
    *Value=*Value/2;
    (*Exponent)++;
  }

  return EFI_SUCCESS;
}

/**
  Field Filling Function. Transform an EFI_EXP_BASE2_DATA to a byte, with '64k'
  as the unit.

  @param  Base2Data              Pointer to Base2_Data

  @retval EFI_SUCCESS            Transform successfully.
  @retval EFI_INVALID_PARAMETER  Invalid parameter was found.

**/
UINT16
Base2ToByteWith64KUnit (
  IN      EFI_EXP_BASE2_DATA  *Base2Data
  )
{
  UINT16              Value;
  UINT16              Exponent;

  Value     = Base2Data->Value;
  Exponent  = Base2Data->Exponent;
  Exponent -= 16;
  Value <<= Exponent;

  return Value;
}


/**
  This function makes boot time changes to the contents of the
  MiscBiosVendor (Type 0).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscBiosVendor)
{
  CHAR8                 *OptionalStrStart;
  UINTN                 VendorStrLen;
  UINTN                 VerStrLen;
  UINTN                 DateStrLen;
  CHAR16                *Version;
  CHAR16                *ReleaseDate;
  CHAR16                BiosVersion[100];         //Assuming that strings are < 100 UCHAR
  CHAR16                BiosReleaseDate[100];     //Assuming that strings are < 100 UCHAR
  CHAR16                BiosReleaseTime[100];     //Assuming that strings are < 100 UCHAR
  EFI_STATUS            Status;
  EFI_STRING            Char16String;
  STRING_REF            TokenToGet;
  STRING_REF            TokenToUpdate;
  SMBIOS_TABLE_TYPE0    *SmbiosRecord;
  EFI_SMBIOS_HANDLE     SmbiosHandle;
  EFI_MISC_BIOS_VENDOR *ForType0InputData;
  BIOS_ID_IMAGE         BiosIdImage;
  UINT16                UVerStr[32];
  UINTN                 LoopIndex;
  UINTN                 CopyIndex;
  MANIFEST_OEM_DATA     *IFWIVerStruct;
  UINT8                 *Data8 = NULL;
  UINT16                SpaceVer[2]={0x0020,0x0000};
  UINT16                BIOSVersionTemp[100];

  ForType0InputData        = (EFI_MISC_BIOS_VENDOR *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  GetBiosId (&BiosIdImage);

  //
  //  Add VLV2 BIOS Version and Release data
  //
  SetMem(BiosVersion, sizeof(BiosVersion), 0);
  SetMem(BiosReleaseDate, sizeof(BiosReleaseDate), 0);
  SetMem(BiosReleaseTime, sizeof(BiosReleaseTime), 0);
  Status = GetBiosVersionDateTime (BiosVersion, BiosReleaseDate, BiosReleaseTime);
  DEBUG ((EFI_D_ERROR, "GetBiosVersionDateTime :%s %s %s \n", BiosVersion, BiosReleaseDate, BiosReleaseTime));
  if (StrLen (BiosVersion) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BIOS_VERSION);
    HiiSetString (mHiiHandle, TokenToUpdate, BiosVersion, NULL);
  }

  if (StrLen(BiosReleaseDate) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BIOS_RELEASE_DATE);
    HiiSetString (mHiiHandle, TokenToUpdate, BiosReleaseDate, NULL);
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BIOS_VENDOR);
  Char16String = SmbiosMiscGetString (TokenToGet);
  VendorStrLen = StrLen(Char16String);
  if (VendorStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BIOS_VERSION);
  Version = SmbiosMiscGetString (TokenToGet);

  ZeroMem (UVerStr, 2*32);
  ZeroMem (BIOSVersionTemp, 2*100);
  StrCat (BIOSVersionTemp,Version);
  Data8 = AllocatePool (SECTOR_SIZE_4KB);
  ZeroMem (Data8, SECTOR_SIZE_4KB);

  Status = gBS->LocateProtocol (
                  &gEfiSpiProtocolGuid,
                  NULL,
                 (VOID **)&mSpiProtocol
                 );
  if (!EFI_ERROR(Status)) {
    //
    // Get data form SPI ROM.
    //
    Status = FlashRead (
               MEM_IFWIVER_START,
               Data8,
               SECTOR_SIZE_4KB,
               EnumSpiRegionAll
               );
    if (!EFI_ERROR(Status)) {
      for(LoopIndex = 0; LoopIndex <= SECTOR_SIZE_4KB; LoopIndex++) {
        IFWIVerStruct = (MANIFEST_OEM_DATA *)(Data8 + LoopIndex);
        if(IFWIVerStruct->Signature == SIGNATURE_32('$','F','U','D')) {
          DEBUG ((EFI_D_ERROR, "the IFWI Length is:%d\n", IFWIVerStruct->IFWIVersionLen));
          if(IFWIVerStruct->IFWIVersionLen < 32) {
            for(CopyIndex = 0; CopyIndex < IFWIVerStruct->IFWIVersionLen; CopyIndex++) {
              UVerStr[CopyIndex] = (UINT16)IFWIVerStruct->IFWIVersion[CopyIndex];
            }
            UVerStr[CopyIndex] = 0x0000;
            DEBUG ((EFI_D_ERROR, "The IFWI Version is :%s,the IFWI Length is:%d\n", UVerStr,IFWIVerStruct->IFWIVersionLen));
            StrCat(BIOSVersionTemp,SpaceVer);
            StrCat(BIOSVersionTemp,UVerStr);
            DEBUG ((EFI_D_ERROR, "The BIOS and IFWI Version is :%s\n", BIOSVersionTemp));
          }
          break;
        }
      }
    }
  }
  FreePool(Data8);

  VerStrLen = StrLen(BIOSVersionTemp);
  if (VerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BIOS_RELEASE_DATE);
  ReleaseDate = SmbiosMiscGetString (TokenToGet);
  DateStrLen = StrLen(ReleaseDate);
  if (DateStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE0) + VendorStrLen + 1 + VerStrLen + 1 + DateStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE0) + VendorStrLen + 1 + VerStrLen + 1 + DateStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_BIOS_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE0);

  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;

  //
  // Vendor will be the 1st optional string following the formatted structure.
  //
  SmbiosRecord->Vendor = 1;

  //
  // Version will be the 2nd optional string following the formatted structure.
  //
  SmbiosRecord->BiosVersion = 2;
  SmbiosRecord->BiosSegment = (UINT16)ForType0InputData->BiosStartingAddress;

  //
  // ReleaseDate will be the 3rd optional string following the formatted structure.
  //
  SmbiosRecord->BiosReleaseDate = 3;

  //
  // Tiger has no PCD value to indicate BIOS Size, just fill 0 for simply.
  //
  SmbiosRecord->BiosSize = 0;
  SmbiosRecord->BiosCharacteristics = *(MISC_BIOS_CHARACTERISTICS*)(&ForType0InputData->BiosCharacteristics1);

  //
  // CharacterExtensionBytes also store in ForType0InputData->BiosCharacteristics1 later two bytes to save size.
  //
  SmbiosRecord->BIOSCharacteristicsExtensionBytes[0] = *((UINT8 *) &ForType0InputData->BiosCharacteristics1 + 4);
  SmbiosRecord->BIOSCharacteristicsExtensionBytes[1] = *((UINT8 *) &ForType0InputData->BiosCharacteristics1 + 5);

  SmbiosRecord->SystemBiosMajorRelease =  ForType0InputData->BiosMajorRelease;
  SmbiosRecord->SystemBiosMinorRelease =  ForType0InputData->BiosMinorRelease;
  SmbiosRecord->EmbeddedControllerFirmwareMajorRelease = ForType0InputData->BiosEmbeddedFirmwareMajorRelease;
  SmbiosRecord->EmbeddedControllerFirmwareMinorRelease = ForType0InputData->BiosEmbeddedFirmwareMinorRelease;

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(Char16String, OptionalStrStart);
  UnicodeStrToAsciiStr(BIOSVersionTemp, OptionalStrStart + VendorStrLen + 1);
  UnicodeStrToAsciiStr(ReleaseDate, OptionalStrStart + VendorStrLen + 1 + VerStrLen + 1);

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios-> Add(
                      Smbios,
                      NULL,
                      &SmbiosHandle,
                      (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                      );

  FreePool(SmbiosRecord);
  return Status;
}
