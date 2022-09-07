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


EFI_STATUS
EFIAPI
UefiMain (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    UINTN  HandleCount; 
    HandleCount = CountProtocolInstances(&gEfiGraphicsOutputProtocolGuid);
    Print(L"HandleCount = %d\n", HandleCount);



    return EFI_SUCCESS;
}