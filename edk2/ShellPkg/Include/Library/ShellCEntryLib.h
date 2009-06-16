/** @file
  Provides application point extension for "C" style main funciton 

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/**
  Intermediate entry point for the application that will in turn call into the "C" 
  style main function.

  this application must have a function like:
  INT32 
  EFIAPI 
  main(
    UINTN Argc, 
    CHAR16 **Argv
  );
**/
EFI_STATUS
EFIAPI
ShellCEntry(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );