/*++

Copyright (c) 1999  - 2014, Intel Corporation.  All rights reserved.
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   



Module Name:

  MiscOemType0x94Function.c

Abstract:

  The function that processes the Smbios data type 0x94.

--*/

#include "CommonHeader.h"

#include "MiscSubclassDriver.h"
#include <Protocol/DataHub.h>
#include <Library/HiiLib.h>
#include <Protocol/CpuIo2.h>
#include <Library/PrintLib.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/IdeControllerInit.h>
#include <Protocol/MpService.h>
#include <Protocol/PchPlatformPolicy.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/I2cBus.h>

#include <Library/IoLib.h>
#include <Library/I2CLib.h>
#include <Library/CpuIA32.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/PlatformInfo.h>
#include <Guid/SetupVariable.h>
#include <Guid/Vlv2Variable.h>

#include "Valleyview.h"
#include "VlvAccess.h"
#include "PchAccess.h"
#include "SetupMode.h"
#include "PchCommonDefinitions.h"
#include <PlatformBaseAddresses.h>

typedef struct {
  UINT8 RevId;
  CHAR8 String[16];
} SB_REV;

//
// Silicon Steppings
//
SB_REV  SBRevisionTable[] = {
  {V_PCH_LPC_RID_0, "(A0 Stepping)"},
  {V_PCH_LPC_RID_1, "(A0 Stepping)"},
  {V_PCH_LPC_RID_2, "(A1 Stepping)"},
  {V_PCH_LPC_RID_3, "(A1 Stepping)"},
  {V_PCH_LPC_RID_4, "(B0 Stepping)"},
  {V_PCH_LPC_RID_5, "(B0 Stepping)"},
  {V_PCH_LPC_RID_6, "(B1 Stepping)"},
  {V_PCH_LPC_RID_7, "(B1 Stepping)"},
  {V_PCH_LPC_RID_8, "(B2 Stepping)"},
  {V_PCH_LPC_RID_9, "(B2 Stepping)"},
  {V_PCH_LPC_RID_A, "(B3 Stepping)"},
  {V_PCH_LPC_RID_B, "(B3 Stepping)"}
};

#define LEFT_JUSTIFY  0x01
#define PREFIX_SIGN   0x02
#define PREFIX_BLANK  0x04
#define COMMA_TYPE    0x08
#define LONG_TYPE     0x10
#define PREFIX_ZERO   0x20

#define ICH_REG_REV                 0x08
#define MSR_IA32_PLATFORM_ID        0x17
#define CHARACTER_NUMBER_FOR_VALUE  30


UINT8  ReadBuffer[20];  //Version report length
UINT8  WriteBuffer[22] = {0x40,0x01,0x14,0x00,0x06,0x51,0x02,0x07,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //Version request

/**

  VSPrint worker function that prints a Value as a decimal number in Buffer

  @param Buffer  Location to place ascii decimal number string of Value.
  @param Value   Decimal value to convert to a string in Buffer.
  @param Flags   Flags to use in printing decimal string, see file header for details.
  @param Width   Width of hex value.

  @retval Number of characters printed.

**/
UINTN
EfiValueToString (
  IN  OUT CHAR16  *Buffer,
  IN  INT64       Value,
  IN  UINTN       Flags,
  IN  UINTN       Width
  )
{
  CHAR16    TempBuffer[CHARACTER_NUMBER_FOR_VALUE];
  CHAR16    *TempStr;
  CHAR16    *BufferPtr;
  UINTN     Count;
  UINTN     ValueCharNum;
  UINTN     Remainder;
  CHAR16    Prefix;
  UINTN     Index;
  BOOLEAN   ValueIsNegative;
  UINT64    TempValue;

  TempStr         = TempBuffer;
  BufferPtr       = Buffer;
  Count           = 0;
  ValueCharNum    = 0;
  ValueIsNegative = FALSE;

  if (Width > CHARACTER_NUMBER_FOR_VALUE - 1) {
    Width = CHARACTER_NUMBER_FOR_VALUE - 1;
  }

  if (Value < 0) {
    Value           = -Value;
    ValueIsNegative = TRUE;
  }

  do {
    TempValue = Value;
    Value = (INT64)DivU64x32 ((UINT64)Value, 10);
    Remainder = (UINTN)((UINT64)TempValue - 10 * Value);
    *(TempStr++) = (CHAR16)(Remainder + '0');
    ValueCharNum++;
    Count++;
    if ((Flags & COMMA_TYPE) == COMMA_TYPE) {
      if (ValueCharNum % 3 == 0 && Value != 0) {
        *(TempStr++) = ',';
        Count++;
      }
    }
  } while (Value != 0);

  if (ValueIsNegative) {
    *(TempStr++)    = '-';
    Count++;
  }

  if ((Flags & PREFIX_ZERO) && !ValueIsNegative) {
    Prefix = '0';
  } else {
    Prefix = ' ';
  }

  Index = Count;
  if (!(Flags & LEFT_JUSTIFY)) {
    for (; Index < Width; Index++) {
      *(TempStr++) = Prefix;
    }
  }

  //
  // Reverse temp string into Buffer.
  //
  if (Width > 0 && (UINTN) (TempStr - TempBuffer) > Width) {
    TempStr = TempBuffer + Width;
  }
  Index = 0;
  while (TempStr != TempBuffer) {
    *(BufferPtr++) = *(--TempStr);
    Index++;
  }

  *BufferPtr = 0;
  return Index;
}

static CHAR16 mHexStr[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
                            L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
/**
  VSPrint worker function that prints a Value as a hex number in Buffer

  @param Buffer   Location to place ascii hex string of Value.
  @param Value    Hex value to convert to a string in Buffer.
  @param Flags    Flags to use in printing Hex string, see file header for details.
  @param Width    Width of hex value.

  @retval Number of characters printed.

**/
UINTN
EfiValueToHexStr (
  IN  OUT CHAR16  *Buffer,
  IN  UINT64      Value,
  IN  UINTN       Flags,
  IN  UINTN       Width
  )
{
  CHAR16  TempBuffer[CHARACTER_NUMBER_FOR_VALUE];
  CHAR16  *TempStr;
  CHAR16  Prefix;
  CHAR16  *BufferPtr;
  UINTN   Count;
  UINTN   Index;

  TempStr   = TempBuffer;
  BufferPtr = Buffer;

  //
  // Count starts at one since we will null terminate. Each iteration of the
  // loop picks off one nibble. Oh yea TempStr ends up backwards
  //
  Count = 0;

  if (Width > CHARACTER_NUMBER_FOR_VALUE - 1) {
    Width = CHARACTER_NUMBER_FOR_VALUE - 1;
  }

  do {
    Index = ((UINTN)Value & 0xf);
    *(TempStr++) = mHexStr[Index];
    Value = RShiftU64 (Value, 4);
    Count++;
  } while (Value != 0);

  if (Flags & PREFIX_ZERO) {
    Prefix = '0';
  } else {
    Prefix = ' ';
  }

  Index = Count;
  if (!(Flags & LEFT_JUSTIFY)) {
    for (; Index < Width; Index++) {
      *(TempStr++) = Prefix;
    }
  }

  //
  // Reverse temp string into Buffer.
  //
  if (Width > 0 && (UINTN) (TempStr - TempBuffer) > Width) {
    TempStr = TempBuffer + Width;
  }
  Index = 0;
  while (TempStr != TempBuffer) {
    *(BufferPtr++) = *(--TempStr);
    Index++;
  }

  *BufferPtr = 0;
  return Index;
}

/**
  Converts MAC address to Unicode string.
  The value is 64-bit and the resulting string will be 12
  digit hex number in pairs of digits separated by dashes.

  @param String - string that will contain the value
  @param Val    - value to convert

**/
CHAR16 *
StrMacToString (
  OUT CHAR16              *String,
  IN  EFI_MAC_ADDRESS     *MacAddr,
  IN  UINT32              AddrSize
  )
{
  UINT32  i;

  for (i = 0; i < AddrSize; i++) {

    EfiValueToHexStr (
      &String[2 * i],
      MacAddr->Addr[i] & 0xFF,
      PREFIX_ZERO,
      2
      );
  }

  //
  // Terminate the string.
  //
  String[2 * AddrSize] = L'\0';

  return String;
}



EFI_STATUS
TJudgeHandleIsPCIDevice(
  EFI_HANDLE    Handle,
  UINT8            Device,
  UINT8            Funs
)
{
  EFI_STATUS  Status;
  EFI_DEVICE_PATH   *DPath;
  EFI_DEVICE_PATH   *DevicePath;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DPath
                  );
  if(!EFI_ERROR(Status))
  {
    DevicePath = DPath;
    while(!IsDevicePathEnd(DPath))
    {
      if((DPath->Type == HARDWARE_DEVICE_PATH) && (DPath->SubType == HW_PCI_DP))
      {
        PCI_DEVICE_PATH   *PCIPath;

        PCIPath = (PCI_DEVICE_PATH*) DPath;
        DPath = NextDevicePathNode(DPath);
        if(IsDevicePathEnd(DPath) && (PCIPath->Device == Device) && (PCIPath->Function == Funs))
        {
          return EFI_SUCCESS;
        }
      }
      else
      {
        DPath = NextDevicePathNode(DPath);
      }
    }
  }
  return EFI_UNSUPPORTED;
}

EFI_STATUS
TSearchChildHandle(
  EFI_HANDLE Father,
  EFI_HANDLE *Child
  )
{
  EFI_STATUS                                                 Status;
  UINTN                                                          HandleIndex;
  EFI_GUID                                                     **ProtocolGuidArray = NULL;
  UINTN                                                          ArrayCount;
  UINTN                                                          ProtocolIndex;
  UINTN                                                          OpenInfoCount;
  UINTN                                                          OpenInfoIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfo = NULL;
  UINTN                                                          mHandleCount;
  EFI_HANDLE                                                 *mHandleBuffer= NULL;

  //
  // Retrieve the list of all handles from the handle database
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &mHandleCount,
                  &mHandleBuffer
                  );

  for (HandleIndex = 0; HandleIndex < mHandleCount; HandleIndex++)
  {
    //
    // Retrieve the list of all the protocols on each handle
    //
    Status = gBS->ProtocolsPerHandle (
                    mHandleBuffer[HandleIndex],
                    &ProtocolGuidArray,
                    &ArrayCount
                    );
    if (!EFI_ERROR (Status))
    {
      for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++)
      {
        Status = gBS->OpenProtocolInformation (
                        mHandleBuffer[HandleIndex],
                        ProtocolGuidArray[ProtocolIndex],
                        &OpenInfo,
                        &OpenInfoCount
                        );
        if (!EFI_ERROR (Status))
        {
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++)
          {
            if(OpenInfo[OpenInfoIndex].AgentHandle == Father)
            {
              if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) == EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER)
              {
                *Child = mHandleBuffer[HandleIndex];
		  Status = EFI_SUCCESS;
		  goto TryReturn;
              }
            }
          }
	   Status = EFI_NOT_FOUND;
        }
      }
      if(OpenInfo != NULL)
      {
        FreePool(OpenInfo);
	 OpenInfo = NULL;
      }
    }
    FreePool (ProtocolGuidArray);
    ProtocolGuidArray = NULL;
  }
TryReturn:
  if(OpenInfo != NULL)
  {
    FreePool (OpenInfo);
    OpenInfo = NULL;
  }
  if(ProtocolGuidArray != NULL)
  {
    FreePool(ProtocolGuidArray);
    ProtocolGuidArray = NULL;
  }
  if(mHandleBuffer != NULL)
  {
    FreePool (mHandleBuffer);
    mHandleBuffer = NULL;
  }
  return Status;
}

EFI_STATUS
TGetDriverName(
  EFI_HANDLE   Handle,
  CHAR16         *Name
)
{
  EFI_DRIVER_BINDING_PROTOCOL        *BindHandle = NULL;
  EFI_STATUS                                        Status;
  UINT32                                               Version;
  UINT16                                               *Ptr;
  Status = gBS->OpenProtocol(
                  Handle,
                  &gEfiDriverBindingProtocolGuid,
                                   (VOID**)&BindHandle,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR(Status))
  {
    return EFI_NOT_FOUND;
  }

  Version = BindHandle->Version;
  Ptr = (UINT16*)&Version;
  UnicodeSPrint(Name, 40, L"%d.%d.%d", Version >> 24 , (Version >>16)& 0x0f ,*(Ptr));
  return EFI_SUCCESS;
}

EFI_STATUS
TGetGOPDriverName(
  CHAR16 *Name
)
{
  UINTN                         HandleCount;
  EFI_HANDLE                *Handles= NULL;
  UINTN                         Index;
  EFI_STATUS                Status;
  EFI_HANDLE                Child = 0;

  Status = gBS->LocateHandleBuffer(
		              ByProtocol,
		              &gEfiDriverBindingProtocolGuid,
		              NULL,
		              &HandleCount,
		              &Handles
                  );
  for (Index = 0; Index < HandleCount ; Index++)
  {
    Status = TSearchChildHandle(Handles[Index], &Child);
    if(!EFI_ERROR(Status))
    {
      Status = TJudgeHandleIsPCIDevice(Child, 0x02, 0x00);
      if(!EFI_ERROR(Status))
      {
        return TGetDriverName(Handles[Index], Name);
      }
    }
  }
  return EFI_UNSUPPORTED;
}

EFI_STATUS
TGetTouchFirmwareVersion(
 )
{
 EFI_STATUS rc=EFI_SUCCESS;
 UINTN      TouchVer = 0;
 UINTN     Size = sizeof(UINTN);


 CHAR16     Buffer[40];

 rc = gRT->GetVariable(
             L"TouchVer",
             &gEfiVlv2VariableGuid,
             NULL,
             &Size,
             &TouchVer
             );
 if(!EFI_ERROR(rc)){
  UnicodeSPrint(Buffer, sizeof(Buffer), L"%02x.%02x", (TouchVer&0xFFFF)>>8,TouchVer&0xFF);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_TOUCH_VERSION), Buffer, NULL);
 }

 return EFI_SUCCESS;
}

EFI_STATUS
UpdatePlatformInformation (
  )
{
  UINT32                   MicroCodeVersion;
  CHAR16                   Buffer[40];
  UINT8                    IgdVBIOSRevH;
  UINT8                    IgdVBIOSRevL;
  UINT16                   EDX;
  EFI_IA32_REGISTER_SET    RegSet;
  EFI_LEGACY_BIOS_PROTOCOL *LegacyBios = NULL;
  EFI_STATUS               Status;
  UINT8                    CpuFlavor=0;
  EFI_PEI_HOB_POINTERS     GuidHob;
  EFI_PLATFORM_INFO_HOB    *mPlatformInfo=NULL;
  UINTN                    NumHandles;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             Index;
  DXE_PCH_PLATFORM_POLICY_PROTOCOL  *PchPlatformPolicy;
  UINTN                             PciD31F0RegBase;
  UINT8                             count;
  UINT8                             Data8;
  UINT8                             Data8_1;

  CHAR16                            Name[40];
  UINT32                            MrcVersion;

  UINT8					KscFwRevH =0;
  UINT8					KscFwRevL =0;

  //
  // Get the HOB list.  If it is not present, then ASSERT.
  //
  GuidHob.Raw = GetHobList ();
  if (GuidHob.Raw != NULL) {
    if ((GuidHob.Raw = GetNextGuidHob (&gEfiPlatformInfoGuid, GuidHob.Raw)) != NULL) {
      mPlatformInfo = GET_GUID_HOB_DATA (GuidHob.Guid);
    }
  }

  //
  //VBIOS version
  //
  Status = gBS->LocateProtocol(
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  (VOID **)&LegacyBios
                  );

  RegSet.X.AX = 0x5f01;
  Status = LegacyBios->Int86 (LegacyBios, 0x10, &RegSet);
  ASSERT_EFI_ERROR(Status);

  //
  // simulate AMI int15 (ax=5f01) handler
  // check NbInt15.asm in AMI code for asm edition
  //
  EDX = (UINT16)((RegSet.E.EBX >> 16) & 0xffff);
  IgdVBIOSRevH = (UINT8)(((EDX & 0x0F00) >> 4) | (EDX & 0x000F));
  IgdVBIOSRevL = (UINT8)(((RegSet.X.BX & 0x0F00) >> 4) | (RegSet.X.BX & 0x000F));

  if (IgdVBIOSRevH==0 && IgdVBIOSRevL==0) {
    HiiSetString(mHiiHandle, STRING_TOKEN(STR_CHIP_IGD_VBIOS_REV_VALUE), L"N/A", NULL);
  } else {
    UnicodeSPrint (Buffer, sizeof (Buffer), L"%02X%02X", IgdVBIOSRevH,IgdVBIOSRevL);
    HiiSetString(mHiiHandle, STRING_TOKEN(STR_CHIP_IGD_VBIOS_REV_VALUE), Buffer, NULL);
  }

  Status = TGetGOPDriverName(Name);

  if(!EFI_ERROR(Status))
  {
    HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_GOP_VERSION), Name, NULL);
  }

  //
  //CpuFlavor
  //
  //VLV
  //VLV-DC Tablet        000
  //VLV-QC Notebook      001
  //VLV-QC Desktop       010
  //
  //CPU flavor
  //
  CpuFlavor = RShiftU64 (EfiReadMsr (MSR_IA32_PLATFORM_ID), 50) & 0x07;

  switch(CpuFlavor){
    case 0x0:
        UnicodeSPrint (Buffer, sizeof (Buffer), L"%s (%01x)", L"VLV-DC Tablet", CpuFlavor);
        break;
    case 0x01:
        UnicodeSPrint (Buffer, sizeof (Buffer), L"%s (%01x)", L"VLV-QC Notebook", CpuFlavor);
        break;
    case 0x02:
        UnicodeSPrint (Buffer, sizeof (Buffer), L"%s (%01x)", L"VLV-QC Desktop", CpuFlavor);
        break;
    case 0x03:
        UnicodeSPrint (Buffer, sizeof (Buffer), L"%s (%01x)", L"VLV-QC Notebook", CpuFlavor);
        break;
    default:
        UnicodeSPrint (Buffer, sizeof (Buffer), L"%s (%01x)", L"Unknown CPU", CpuFlavor);
        break;
  }
  HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_CPU_FLAVOR_VALUE), Buffer, NULL);

  if ( NULL != mPlatformInfo) {
    //
    // Board Id
    //
    UnicodeSPrint (Buffer, sizeof (Buffer), L"%x", mPlatformInfo->BoardId);
    HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_BOARD_ID_VALUE), Buffer, NULL);

    //
    // FAB ID
    //
    UnicodeSPrint (Buffer, sizeof (Buffer), L"%x", mPlatformInfo->BoardRev);
    HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_FAB_ID_VALUE), Buffer, NULL);
  }

  //
  //Update MRC Version
  //
  MrcVersion = MmioRead32 (MmPciAddress (0, 0, 0, 0, 0xF0));
  MrcVersion &= 0xffff;
  Index = EfiValueToString (Buffer, MrcVersion/100, PREFIX_ZERO, 0);
  StrCat (Buffer, L".");
  EfiValueToString (Buffer + Index + 1, (MrcVersion%100)/10, PREFIX_ZERO, 0);
  EfiValueToString (Buffer + Index + 2, (MrcVersion%100)%10, PREFIX_ZERO, 0);
  HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_MRC_VERSION_VALUE), Buffer, NULL);

  //
  //Update Soc Version
  //

  //
  // Retrieve all instances of PCH Platform Policy protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gDxePchPlatformPolicyProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Find the matching PCH Policy protocol
    //
    for (Index = 0; Index < NumHandles; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gDxePchPlatformPolicyProtocolGuid,
                      (VOID **) &PchPlatformPolicy
                      );
      if (!EFI_ERROR (Status)) {
        PciD31F0RegBase = MmPciAddress (
                            0,
                            PchPlatformPolicy->BusNumber,
                            PCI_DEVICE_NUMBER_PCH_LPC,
                            PCI_FUNCTION_NUMBER_PCH_LPC,
                            0
                            );

         Data8 = MmioRead8 (PciD31F0RegBase + R_PCH_LPC_RID_CC);
         count = sizeof (SBRevisionTable) / sizeof (SBRevisionTable[0]);
         for (Index = 0; Index < count; Index++) {
           if(Data8 == SBRevisionTable[Index].RevId) {
              UnicodeSPrint (Buffer, sizeof (Buffer), L"%02x %a", Data8, SBRevisionTable[Index].String);
              HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SOC_VALUE), Buffer, NULL);
             break;
           }
         }
        break;
      }
    }
  }

  //
  // Microcode Revision
  //
  EfiWriteMsr (EFI_MSR_IA32_BIOS_SIGN_ID, 0);
  EfiCpuid (EFI_CPUID_VERSION_INFO, NULL);
  MicroCodeVersion = (UINT32) RShiftU64 (EfiReadMsr (EFI_MSR_IA32_BIOS_SIGN_ID), 32);
  UnicodeSPrint (Buffer, sizeof (Buffer), L"%x", MicroCodeVersion);
  HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_PROCESSOR_MICROCODE_VALUE), Buffer, NULL);


  //
  //Secure boot
  //
  Data8 = SystemConfiguration.SecureBoot;
  UnicodeSPrint (Buffer, sizeof(Buffer), L"%x", Data8);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_SECURE_BOOT), Buffer, NULL);

  //
  //Bootmode
  //
  BootMode = GetBootModeHob();
  UnicodeSPrint (Buffer, sizeof(Buffer), L"%x", BootMode);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_BOOT_MODE), Buffer, NULL);

  //
  //SpeedStep
  //
  Data8 = 1;
  UnicodeSPrint (Buffer, sizeof(Buffer), L"%x", Data8);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_SPEED_STEP), Buffer, NULL);

  //
  //CPU Turbo
  //
  Data8 = 2;
  UnicodeSPrint (Buffer, sizeof(Buffer), L"%x", Data8);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_CPU_TURBO), Buffer, NULL);

  //
  //CState
  //
  Data8 = 3;
  UnicodeSPrint (Buffer, sizeof(Buffer), L"%x", Data8);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_CSTATE), Buffer, NULL);

  //
  //GFX Turbo
  //
  Data8 = SystemConfiguration.IgdTurboEnabled;
  UnicodeSPrint (Buffer, sizeof(Buffer), L"%x", Data8);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_GFX_TURBO), Buffer, NULL);

  Data8 = 0;
  UnicodeSPrint (Buffer, sizeof(Buffer), L"%x", Data8);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_S0IX_VALUE), Buffer, NULL);

  //
  //RC6
  //
  Data8 = 0; 
  UnicodeSPrint (Buffer, sizeof(Buffer), L"%x", Data8);
  HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_RC6_VALUE), Buffer, NULL);

  //
  // Punit Version
  //
  Data8 = 0;
  UnicodeSPrint (Buffer, sizeof (Buffer), L"0x%x", Data8);
  HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_PUNIT_FW_VALUE), Buffer, NULL);

  //
  //  PMC Version
  //
  Data8 = (UINT8)((MmioRead32 (PMC_BASE_ADDRESS + R_PCH_PMC_PRSTS)>>16)&0x00FF);
  Data8_1 = (UINT8)((MmioRead32 (PMC_BASE_ADDRESS + R_PCH_PMC_PRSTS)>>24)&0x00FF);
  UnicodeSPrint (Buffer, sizeof (Buffer), L"0x%X_%X", Data8_1, Data8);
  HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_PMC_FW_VALUE), Buffer, NULL);

  //
  //PMIC Version
  //
  Status = ByteReadI2C(PMICI2cBus, PMICI2cAdd, PMICVendorOffset, 1, &Data8);
  if(!EFI_ERROR(Status)){
  	Status = ByteReadI2C(PMICI2cBus, PMICI2cAdd, PMICRevOffset, 1, &Data8_1);
	if(!EFI_ERROR(Status)){
      UnicodeSPrint(Buffer, sizeof(Buffer), L"%02x.%02x", Data8, Data8_1);
      HiiSetString(mHiiHandle, STRING_TOKEN(STR_MISC_PMIC_VERSION), Buffer, NULL);
	}
  }

  TGetTouchFirmwareVersion();

  return EFI_SUCCESS;
}


/**
  Smbios OEM type 0x94 callback.

  @param Event    Event whose notification function is being invoked.
  @param Context  Pointer to the notification functions context, which is implementation dependent.

  @retval None

**/
VOID
AddSmbiosT0x94Callback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS            Status;
  UINTN                 SECVerStrLen = 0;
  UINTN                 uCodeVerStrLen = 0;
  UINTN                 GOPStrLen = 0;
  UINTN                 MRCVersionStrLen = 0;
  UINTN                 PMCVersionStrLen = 0;
  UINTN                 ULPMCVersionStrLen = 0;
  UINTN                 PUNITVersionStrLen = 0;
  UINTN                 SOCVersionStrLen = 0;
  UINTN                 BOARDVersionStrLen = 0;
  UINTN                 FABVersionStrLen = 0;
  UINTN                 CPUFLAVORStrLen = 0;
  UINTN                 BIOSVersionStrLen = 0;
  UINTN                 PMICVersionStrLen = 0;
  UINTN                 TOUCHVersionStrLen = 0;
  UINTN                 SecureBootModeLen = 0;
  UINTN                 BootModeLen = 0;
  UINTN                 SpeedStepModeLen = 0;
  UINTN                 MaxCStateLen = 0;
  UINTN                 CpuTurboLen = 0;
  UINTN                 GfxTurboLen = 0;
  UINTN                 IdleReserveLen = 0;
  UINTN                 RC6Len = 0;

  SMBIOS_TABLE_TYPE94    *SmbiosRecord;
  EFI_SMBIOS_HANDLE     SmbiosHandle;
  EFI_MISC_OEM_TYPE_0x94  *ForType94InputData;
  CHAR16                *SECVer;
  CHAR16                *uCodeVer;
  CHAR16                *GOPVer;
  CHAR16                *MrcVer;
  CHAR16                *PmcVer;
  CHAR16                *UlpmcVer;
  CHAR16                *PunitVer;
  CHAR16                *SocVer;
  CHAR16                *BoardVer;
  CHAR16                *FabVer;
  CHAR16                *CpuFlavor;
  CHAR16                *BiosVer;
  CHAR16                *PmicVer;
  CHAR16                *TouchVer = L"15.16";
  CHAR16                *SecureBootMode;
  CHAR16                *BootMode;
  CHAR16                *SpeedStepMode;
  CHAR16                *MaxCState;
  CHAR16                *CpuTurbo;
  CHAR16                *GfxTurbo;
  CHAR16                *IdleReserve;
  CHAR16                *RC6;

  UINTN                 RecordLen = 0;
  UINTN                 StrIdx = 0;


  STRING_REF            TokenToGet;
  CHAR8                 *OptionalStrStart;
  EFI_SMBIOS_PROTOCOL               *SmbiosProtocol;

  ForType94InputData        = (EFI_MISC_OEM_TYPE_0x94 *)Context;

  DEBUG ((EFI_D_INFO, "Executing SMBIOS T0x94 callback.\n"));

  gBS->CloseEvent (Event);    // Unload this event.

  //
  // First check for invalid parameters.
  //
  if (Context == NULL) {
    return;
  }

  UpdatePlatformInformation();

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **) &SmbiosProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  TokenToGet = STRING_TOKEN (STR_MISC_SEC_VERSION);
  SECVer = SmbiosMiscGetString (TokenToGet);
  SECVerStrLen = StrLen(SECVer);
  if (SECVerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_UCODE_VERSION);
  uCodeVer = SmbiosMiscGetString (TokenToGet);
  uCodeVerStrLen = StrLen(uCodeVer);
  if (uCodeVerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_GOP_VERSION);
  GOPVer = SmbiosMiscGetString (TokenToGet);
  GOPStrLen = StrLen(GOPVer);
  if (GOPStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_MRC_VERSION_VALUE);
  MrcVer = SmbiosMiscGetString (TokenToGet);
  MRCVersionStrLen = StrLen(MrcVer);
  if (MRCVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_PMC_FW_VALUE);
  PmcVer = SmbiosMiscGetString (TokenToGet);
  PMCVersionStrLen = StrLen(PmcVer);
  if (PMCVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_ULPMC_FW_VALUE);
  UlpmcVer = SmbiosMiscGetString (TokenToGet);
  ULPMCVersionStrLen = StrLen(UlpmcVer);
  if (ULPMCVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_PUNIT_FW_VALUE);
  PunitVer = SmbiosMiscGetString (TokenToGet);
  PUNITVersionStrLen = StrLen(PunitVer);
  if (PUNITVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_SOC_VALUE);
  SocVer = SmbiosMiscGetString (TokenToGet);
  SOCVersionStrLen = StrLen(SocVer);
  if (SOCVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BOARD_ID_VALUE);
  BoardVer = SmbiosMiscGetString (TokenToGet);
  BOARDVersionStrLen = StrLen(BoardVer);
  if (BOARDVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_FAB_ID_VALUE);
  FabVer = SmbiosMiscGetString (TokenToGet);
  FABVersionStrLen = StrLen(FabVer);
  if (FABVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CPU_FLAVOR_VALUE);
  CpuFlavor = SmbiosMiscGetString (TokenToGet);
  CPUFLAVORStrLen = StrLen(CpuFlavor);
  if (CPUFLAVORStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BIOS_VERSION);
  BiosVer = SmbiosMiscGetString (TokenToGet);
  BIOSVersionStrLen = StrLen(BiosVer);
  if (BIOSVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_PMIC_VERSION);
  PmicVer = SmbiosMiscGetString (TokenToGet);
  PMICVersionStrLen = StrLen(PmicVer);
  if (PMICVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_TOUCH_VERSION);
  TouchVer = SmbiosMiscGetString (TokenToGet);
  TOUCHVersionStrLen = StrLen(TouchVer);
  if (TOUCHVersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_SECURE_BOOT);
  SecureBootMode = SmbiosMiscGetString(TokenToGet);
  SecureBootModeLen = StrLen(SecureBootMode);
  if (SecureBootModeLen > SMBIOS_STRING_MAX_LENGTH) {
  	return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BOOT_MODE);
  BootMode = SmbiosMiscGetString(TokenToGet);
  BootModeLen = StrLen(BootMode);
  if (BootModeLen > SMBIOS_STRING_MAX_LENGTH) {
  	return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_SPEED_STEP);
  SpeedStepMode = SmbiosMiscGetString(TokenToGet);
  SpeedStepModeLen = StrLen(SpeedStepMode);
  if (SpeedStepModeLen > SMBIOS_STRING_MAX_LENGTH) {
  	return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CPU_TURBO);
  CpuTurbo = SmbiosMiscGetString(TokenToGet);
  CpuTurboLen = StrLen(CpuTurbo);
  if (CpuTurboLen > SMBIOS_STRING_MAX_LENGTH) {
  	return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CSTATE);
  MaxCState = SmbiosMiscGetString(TokenToGet);
  MaxCStateLen = StrLen(MaxCState);
  if (MaxCStateLen > SMBIOS_STRING_MAX_LENGTH) {
  	return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_GFX_TURBO);
  GfxTurbo = SmbiosMiscGetString(TokenToGet);
  GfxTurboLen = StrLen(GfxTurbo);
  if (GfxTurboLen > SMBIOS_STRING_MAX_LENGTH) {
  	return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_S0IX_VALUE);
  IdleReserve = SmbiosMiscGetString(TokenToGet);
  IdleReserveLen = StrLen(IdleReserve);
  if (S0ixLen > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_RC6_VALUE);
  RC6 = SmbiosMiscGetString(TokenToGet);
  RC6Len = StrLen(RC6);
  if (RC6Len > SMBIOS_STRING_MAX_LENGTH) {
    return;
  }

  RecordLen = sizeof (SMBIOS_TABLE_TYPE94) + SECVerStrLen + 1 + uCodeVerStrLen + 1 + GOPStrLen + 1 + PMCVersionStrLen + 1 + \
                      TOUCHVersionStrLen + 1 + PMICVersionStrLen + 1 + BIOSVersionStrLen + 1 + CPUFLAVORStrLen + 1 + \
                      BOARDVersionStrLen + 1 + FABVersionStrLen + 1 + PUNITVersionStrLen+ 1 + ULPMCVersionStrLen + 1 + \
                      MRCVersionStrLen + 1 + SOCVersionStrLen + 1 + SecureBootModeLen + 1 + BootModeLen + 1 + \
                      SpeedStepModeLen + 1 + CpuTurboLen + 1 + MaxCStateLen + 1 + GfxTurboLen + 1 + + RC6Len + 1 + 1;

  SmbiosRecord = AllocatePool(RecordLen);

  ZeroMem(SmbiosRecord, RecordLen);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_MISC_VERSION_INFO;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE94);

  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;

  SmbiosRecord->GopVersion = 1;

  SmbiosRecord->SECVersion = 2;

  SmbiosRecord->MRCVersion = 3;

  SmbiosRecord->uCodeVersion = 4;

  SmbiosRecord->PUnitVersion = 5;

  SmbiosRecord->PMCVersion = 6;

  SmbiosRecord->ULPMCVersion = 7;

  SmbiosRecord->SoCVersion = 8;

  SmbiosRecord->BoardVersion = 9;

  SmbiosRecord->FabVersion = 10;

  SmbiosRecord->CPUFlavor = 11;

  SmbiosRecord->BiosVersion = 12;

  SmbiosRecord->PmicVersion = 13;

  SmbiosRecord->TouchVersion = 14;

  SmbiosRecord->SecureBoot = 15;

  SmbiosRecord->BootMode = 16;

  SmbiosRecord->SpeedStepMode= 17;

  SmbiosRecord->CPUTurboMode = 18;

  SmbiosRecord->MaxCState = 19;

  SmbiosRecord->GfxTurbo = 20;
  SmbiosRecord->IdleReserve = 21;

  SmbiosRecord->RC6 = 22;


  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(GOPVer, OptionalStrStart);
  StrIdx +=  GOPStrLen + 1;

  UnicodeStrToAsciiStr(SECVer, OptionalStrStart + StrIdx);
  StrIdx +=  SECVerStrLen + 1;

  UnicodeStrToAsciiStr(MrcVer, OptionalStrStart + StrIdx);
  StrIdx +=  MRCVersionStrLen + 1;

  UnicodeStrToAsciiStr(uCodeVer, OptionalStrStart + StrIdx);
  StrIdx +=  uCodeVerStrLen + 1;

  UnicodeStrToAsciiStr(PunitVer, OptionalStrStart + StrIdx);
  StrIdx +=  PUNITVersionStrLen + 1;

  UnicodeStrToAsciiStr(PmcVer, OptionalStrStart + StrIdx);
  StrIdx +=  PMCVersionStrLen + 1;

  UnicodeStrToAsciiStr(UlpmcVer, OptionalStrStart + StrIdx);
  StrIdx +=  ULPMCVersionStrLen + 1;


  UnicodeStrToAsciiStr(SocVer, OptionalStrStart + StrIdx);
  StrIdx +=  SOCVersionStrLen +1;

  UnicodeStrToAsciiStr(BoardVer, OptionalStrStart + StrIdx);
  StrIdx +=  BOARDVersionStrLen + 1;

  UnicodeStrToAsciiStr(FabVer, OptionalStrStart + StrIdx);
  StrIdx +=  FABVersionStrLen + 1;

  UnicodeStrToAsciiStr(CpuFlavor, OptionalStrStart + StrIdx);
  StrIdx +=  CPUFLAVORStrLen + 1;

  UnicodeStrToAsciiStr(BiosVer, OptionalStrStart + StrIdx);
  StrIdx +=  BIOSVersionStrLen + 1;

  UnicodeStrToAsciiStr(PmicVer, OptionalStrStart + StrIdx);
  StrIdx +=  PMICVersionStrLen + 1;

  UnicodeStrToAsciiStr(TouchVer, OptionalStrStart + StrIdx);
  StrIdx +=  TOUCHVersionStrLen + 1;

  UnicodeStrToAsciiStr(SecureBootMode, OptionalStrStart + StrIdx);
  StrIdx +=  SecureBootModeLen + 1;

  UnicodeStrToAsciiStr(BootMode, OptionalStrStart + StrIdx);
  StrIdx +=  BootModeLen + 1;

  UnicodeStrToAsciiStr(SpeedStepMode, OptionalStrStart + StrIdx);
  StrIdx +=  SpeedStepModeLen + 1;

  UnicodeStrToAsciiStr(CpuTurbo, OptionalStrStart + StrIdx);
  StrIdx +=  CpuTurboLen + 1;

  UnicodeStrToAsciiStr(MaxCState, OptionalStrStart + StrIdx);
  StrIdx +=  MaxCStateLen + 1;

  UnicodeStrToAsciiStr(GfxTurbo, OptionalStrStart + StrIdx);
  StrIdx +=  GfxTurboLen + 1;

  UnicodeStrToAsciiStr(IdleReserve, OptionalStrStart + StrIdx);
  StrIdx +=  S0ixLen + 1;

  UnicodeStrToAsciiStr(RC6, OptionalStrStart + StrIdx);
  StrIdx +=  RC6Len + 1;

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = SmbiosProtocol-> Add (
                              SmbiosProtocol,
                              NULL,
                              &SmbiosHandle,
                              (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                              );

  FreePool(SmbiosRecord);
  return;
}

/**
  This function makes boot time changes to the contents of the
  MiscOemType0x94 (Type 0x94).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscOemType0x94)
{
  EFI_STATUS                    Status;
  EFI_EVENT                     AddSmbiosT0x94CallbackEvent;

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             AddSmbiosT0x94Callback,
             RecordData,
             &AddSmbiosT0x94CallbackEvent
             );

  ASSERT_EFI_ERROR (Status);
  return Status;

}
