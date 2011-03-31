/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PerformanceLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/BdsUnixLib.h>

#include <Protocol/Bds.h>
#include <Protocol/DevicePathToText.h>

#include <Guid/GlobalVariable.h>

#define MAX_CMD_LINE            256

VOID
EFIAPI
BdsEntry (
  IN EFI_BDS_ARCH_PROTOCOL  *This
  );

EFI_HANDLE  mBdsImageHandle = NULL;
EFI_BDS_ARCH_PROTOCOL  gBdsProtocol = {
  BdsEntry,
};

EFI_STATUS GetEnvironmentVariable (
    IN  CONST CHAR16*   VariableName,
    IN  VOID*           DefaultValue,
    IN  UINTN           DefaultSize,
    OUT VOID**          Value)
{
    EFI_STATUS  Status;
    UINTN       Size;

    // Try to get the variable size.
    *Value = NULL;
    Size = 0;
    Status = gRT->GetVariable ((CHAR16 *) VariableName, &gEfiGlobalVariableGuid, NULL, &Size, *Value);
    if (Status == EFI_NOT_FOUND) {
        // If the environment variable does not exist yet then set it with the default value
        Status = gRT->SetVariable (
                      (CHAR16*)VariableName,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      DefaultSize,
                      DefaultValue
                      );
        *Value = DefaultValue;
    } else if (Status == EFI_BUFFER_TOO_SMALL) {
        // Get the environment variable value
        *Value = AllocatePool (Size);
        if (*Value == NULL) {
            return EFI_OUT_OF_RESOURCES;
        }

        Status = gRT->GetVariable ((CHAR16 *)VariableName, &gEfiGlobalVariableGuid, NULL, &Size, *Value);
        if (EFI_ERROR (Status)) {
            FreePool(*Value);
            return EFI_INVALID_PARAMETER;
        }
    } else {
        *Value = DefaultValue;
        return Status;
    }

    return EFI_SUCCESS;
}

VOID InitializeConsole (
  VOID                   
  )
{
  EFI_STATUS                Status;
  UINTN                     NoHandles;
  EFI_HANDLE                *Buffer;

  //
  // Now we need to setup the EFI System Table with information about the console devices.
  // This code is normally in the console spliter driver on platforms that support multiple 
  // consoles at the same time
  //
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleTextOutProtocolGuid, NULL, &NoHandles, &Buffer);
  if (!EFI_ERROR (Status)) {
    // Use the first SimpleTextOut we find and update the EFI System Table
    gST->ConsoleOutHandle = Buffer[0];
    gST->StandardErrorHandle = Buffer[0];
    Status = gBS->HandleProtocol (Buffer[0], &gEfiSimpleTextOutProtocolGuid, (VOID **)&gST->ConOut);
    ASSERT_EFI_ERROR (Status);
    
    gST->StdErr = gST->ConOut;
    
    FreePool (Buffer);
  } 
  
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleTextInProtocolGuid, NULL, &NoHandles, &Buffer);
  if (!EFI_ERROR (Status)) {
    // Use the first SimpleTextIn we find and update the EFI System Table
    gST->ConsoleInHandle = Buffer[0];
    Status = gBS->HandleProtocol (Buffer[0], &gEfiSimpleTextInProtocolGuid, (VOID **)&gST->ConIn);
    ASSERT_EFI_ERROR (Status);
    
    FreePool (Buffer);
  }
}

EFI_STATUS
GetHIInputAscii (
    CHAR8   *CmdLine,
    UINTN   MaxCmdLine
) {
    UINTN           CmdLineIndex;
    UINTN           WaitIndex;
    CHAR8           Char;
    EFI_INPUT_KEY   Key;
    EFI_STATUS      Status;

    CmdLine[0] = '\0';

    for (CmdLineIndex = 0; CmdLineIndex < MaxCmdLine; ) {
        Status = gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &WaitIndex);
        ASSERT_EFI_ERROR (Status);

        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        ASSERT_EFI_ERROR (Status);

        Char = (CHAR8)Key.UnicodeChar;
        if ((Char == '\n') || (Char == '\r') || (Char == 0x7f)) {
            CmdLine[CmdLineIndex] = '\0';
            AsciiPrint ("\n");
            return EFI_SUCCESS;
        } else if ((Char == '\b') || (Key.ScanCode == SCAN_LEFT) || (Key.ScanCode == SCAN_DELETE)){
            if (CmdLineIndex != 0) {
                CmdLineIndex--;
                AsciiPrint ("\b \b");
            }
        } else {
            CmdLine[CmdLineIndex++] = Char;
            AsciiPrint ("%c", Char);
        }
    }

    return EFI_SUCCESS;
}

VOID
ListDevicePaths (
		IN BOOLEAN	AllDrivers
) {
	EFI_STATUS Status;
	UINTN                         		HandleCount;
	EFI_HANDLE                    		*HandleBuffer;
	UINTN                         		Index;
	EFI_DEVICE_PATH_PROTOCOL*     		DevicePathProtocol;
	CHAR16*					      		String;
	EFI_DEVICE_PATH_TO_TEXT_PROTOCOL*	EfiDevicePathToTextProtocol;

	if (AllDrivers) {
		BdsConnectAllDrivers();
	}

    Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&EfiDevicePathToTextProtocol);
	if (EFI_ERROR (Status)) {
		AsciiPrint ("Did not find the DevicePathToTextProtocol.\n");
	    return;
	}

	Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiDevicePathProtocolGuid, NULL, &HandleCount, &HandleBuffer);
	if (EFI_ERROR (Status)) {
		AsciiPrint ("No device path found\n");
	    return;
	}

	for (Index = 0; Index < HandleCount; Index++) {
		Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePathProtocol);
		String = EfiDevicePathToTextProtocol->ConvertDevicePathToText(DevicePathProtocol,TRUE,TRUE);
		Print (L"\t- [%d] %s\n",Index, String);
	}
}

INTN BdsComparefile (
    IN  CHAR16        *DeviceFilePath1,
    IN  CHAR16        *DeviceFilePath2,
    VOID **FileImage1,VOID **FileImage2,UINTN* FileSize
    );

/**
  This function uses policy data from the platform to determine what operating 
  system or system utility should be loaded and invoked.  This function call 
  also optionally make the use of user input to determine the operating system 
  or system utility to be loaded and invoked.  When the DXE Core has dispatched 
  all the drivers on the dispatch queue, this function is called.  This 
  function will attempt to connect the boot devices required to load and invoke 
  the selected operating system or system utility.  During this process, 
  additional firmware volumes may be discovered that may contain addition DXE 
  drivers that can be dispatched by the DXE Core.   If a boot device cannot be 
  fully connected, this function calls the DXE Service Dispatch() to allow the 
  DXE drivers from any newly discovered firmware volumes to be dispatched.  
  Then the boot device connection can be attempted again.  If the same boot 
  device connection operation fails twice in a row, then that boot device has 
  failed, and should be skipped.  This function should never return.

  @param  This             The EFI_BDS_ARCH_PROTOCOL instance.

  @return None.

**/
VOID
EFIAPI
BdsEntry (
  IN EFI_BDS_ARCH_PROTOCOL  *This
  )
{
    EFI_STATUS          Status;
    CHAR8               CmdLine[MAX_CMD_LINE];
    VOID*               DefaultVariableValue;
    UINTN               DefaultVariableSize;
    CHAR16              *LinuxKernelDP;
    CHAR8               *LinuxAtag;
    CHAR16              *FdtDP;

    PERF_END   (NULL, "DXE", NULL, 0);
    PERF_START (NULL, "BDS", NULL, 0);

    InitializeConsole();

    while (1) {
      // Get the Linux Kernel Device Path from Environment Variable
      DefaultVariableValue = (VOID*)PcdGetPtr(PcdLinuxKernelDP);
      DefaultVariableSize = StrSize((CHAR16*)DefaultVariableValue);
      GetEnvironmentVariable(L"LinuxKernelDP",DefaultVariableValue,DefaultVariableSize,(VOID**)&LinuxKernelDP);

      // Get the Linux ATAG from Environment Variable
      DefaultVariableValue = (VOID*)PcdGetPtr(PcdLinuxAtag);
      DefaultVariableSize = AsciiStrSize((CHAR8*)DefaultVariableValue);
      GetEnvironmentVariable(L"LinuxAtag",DefaultVariableValue,DefaultVariableSize,(VOID**)&LinuxAtag);

      // Get the FDT Device Path from Environment Variable
      DefaultVariableValue = (VOID*)PcdGetPtr(PcdFdtDP);
      DefaultVariableSize = StrSize((CHAR16*)DefaultVariableValue);
      GetEnvironmentVariable(L"FdtDP",DefaultVariableValue,DefaultVariableSize,(VOID**)&FdtDP);

        AsciiPrint ("1. Start EBL\n\r");
        AsciiPrint ("2. List Device Paths of all the drivers\n");
        AsciiPrint ("3. Start Linux\n");
        Print (L"\t- Kernel: %s\n", LinuxKernelDP);
        AsciiPrint ("\t- Atag: %a\n", LinuxAtag);
        Print (L"\t- Fdt: %s\n", FdtDP);
        AsciiPrint ("Choice: ");

        Status = GetHIInputAscii(CmdLine,MAX_CMD_LINE);
        ASSERT_EFI_ERROR (Status);
        if (AsciiStrCmp(CmdLine,"1") == 0) {
            // Start EBL
            Status = BdsLoadApplication(L"Ebl");
            if (Status == EFI_NOT_FOUND) {
                AsciiPrint ("Error: EFI Application not found.\n");
            } else {
                AsciiPrint ("Error: Status Code: 0x%X\n",(UINT32)Status);
            }
        } else if (AsciiStrCmp(CmdLine,"2") == 0) {
        	ListDevicePaths (TRUE);
        } else if (AsciiStrCmp(CmdLine,"3") == 0) {
            // Start Linux Kernel
            Status = BdsBootLinux(LinuxKernelDP,LinuxAtag,FdtDP);
            if (EFI_ERROR(Status)) {
                AsciiPrint ("Error: Fail to start Linux (0x%X)\n",(UINT32)Status);
            }
        } else {
            AsciiPrint ("Error: Invalid choice.\n");
        }
    }
}

EFI_STATUS
EFIAPI
BdsInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;

  mBdsImageHandle = ImageHandle;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mBdsImageHandle,
                  &gEfiBdsArchProtocolGuid, &gBdsProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
