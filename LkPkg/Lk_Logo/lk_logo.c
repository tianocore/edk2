/** @file
    This sample application bases on HelloWorld PCD setting 
    to print "UEFI Hello World!" to the UEFI Console.

    Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials                          
    are licensed and made available under the terms and conditions of the BSD License         
    which accompanies this distribution.  The full text of the license may be found at        
    http://opensource.org/licenses/bsd-license.php                                            

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include <Uefi.h>
#include  <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
// #include  <AMI/ProtocolLib.h>
#include  <AMI/ProtocolLib.h>

EFI_STATUS LkBlt(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput) 
{   
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL color[] = {
                                                {0, 0, 255, 0}, // red 
                                                {250, 250, 255, 0} // snow
                                            };


    Status  = GraphicsOutput->Blt(
                GraphicsOutput,
                &color[1],
                EfiBltVideoFill,
                0,0,
                0,0,
                100,100,
                0);
    if (EFI_ERROR (Status)){
        return Status;

    }
    Print(L"Blt sucess!\n");
    return EFI_SUCCESS;


}


EFI_STATUS
EFIAPI
UefiMain (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    UINTN       HandleCount;
    UINT32      ModeIndex;
    UINTN       SizeOfInfo;
    UINTN       Index; 
    EFI_HANDLE  *HandleBuffer;
    EFI_STATUS  Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;


    HandleCount = CountProtocolInstances(&gEfiGraphicsOutputProtocolGuid, &HandleBuffer);
    Print(L"HandleCount = %d\n", HandleCount);
    for ( Index = 0; Index < HandleCount; Index++)
    {   GraphicsOutput = NULL;
        Status = gBS->HandleProtocol(
            HandleBuffer[Index],
            &gEfiGraphicsOutputProtocolGuid,
            (VOID **)&GraphicsOutput);
        if (EFI_ERROR(Status)) {
            continue;
        }
        for (ModeIndex = 0; ModeIndex < GraphicsOutput->Mode->MaxMode; ModeIndex++) {

            Status = GraphicsOutput->QueryMode(
                        GraphicsOutput, ModeIndex, &SizeOfInfo, &Info);

            Print(L"modeindex = %d SizeOfInfo = %d\n", ModeIndex, SizeOfInfo);
            Print(L"modeindex = %d Info->Version = 0x%04x\n", ModeIndex, Info->Version);
            Print(L"modeindex = %d Info->HorizontalResolution = %d\n", ModeIndex, Info->HorizontalResolution);
            Print(L"modeindex = %d Info->VerticalResolution = %d\n", ModeIndex, Info->VerticalResolution);
            Print(L"modeindex = %d Info->PixelsPerScanLine = %d\n", ModeIndex, Info->PixelsPerScanLine);
        }
        Status = LkBlt(GraphicsOutput);
        Print(L"LkBlt Status = %r\n", Status);
    }




    return EFI_SUCCESS;
}