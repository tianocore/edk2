/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeiNt32PeCoffExtraActionLib.c

Abstract:

  Provides services to perform additional actions to relocate and unload
  PE/Coff image for NT32 environment specific purpose such as souce level debug.
  This version only works for PEI phase  


**/
//
// The package level header files this module uses
//
#include <PiPei.h>
#include <WinNtPeim.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/NtThunk.h>

#include <PiPei.h>
#include <Library/PeCoffLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PeCoffExtraActionLib.h>

//
// Cache of WinNtThunk protocol
//
EFI_WIN_NT_THUNK_PROTOCOL   *mWinNt = NULL;

/**
  The function caches the pointer of the WinNT thunk functions
  It will ASSERT() if NT thunk ppi is not installed.

  @retval EFI_SUCCESS   WinNT thunk protocol is found and cached.

**/
EFI_STATUS
EFIAPI
Nt32PeCoffGetWinNtThunkStucture (
  )
{
	PEI_NT_THUNK_PPI  *NtThunkPpi;
  EFI_STATUS        Status;


  //
  // Locate NtThunkPpi for retrieving standard output handle
  //
  Status = PeiServicesLocatePpi (
              &gPeiNtThunkPpiGuid,
              0,
              NULL,
              (VOID **) &NtThunkPpi
              );

  ASSERT_EFI_ERROR (Status);

  mWinNt  = (EFI_WIN_NT_THUNK_PROTOCOL *) NtThunkPpi->NtThunk ();
  
  return EFI_SUCCESS;
}

/**
  Convert the passed in Ascii string to Unicode.
  
  This function  Convert the passed in Ascii string to Unicode.Optionally return
   the length of the strings..

  @param  AsciiString    Pointer to an AscII string
  @param  StrLen         Length of string

  @return  Pointer to malloc'ed Unicode version of Ascii

**/
CHAR16 *
AsciiToUnicode (
  IN  CHAR8   *Ascii,
  IN  UINTN   *StrLen OPTIONAL
  )
{
  UINTN   Index;
  CHAR16  *Unicode;

  //
  // Allocate a buffer for unicode string
  //
  for (Index = 0; Ascii[Index] != '\0'; Index++)
    ;
  Unicode = mWinNt->HeapAlloc ( mWinNt->GetProcessHeap (),
                                HEAP_ZERO_MEMORY,
                                ((Index + 1) * sizeof (CHAR16))
                               ); 
  if (Unicode == NULL) {
    return NULL;
  }

  for (Index = 0; Ascii[Index] != '\0'; Index++) {
    Unicode[Index] = (CHAR16) Ascii[Index];
  }

  Unicode[Index] = '\0';

  if (StrLen != NULL) {
    *StrLen = Index;
  }

  return Unicode;
}

/**
  Performs additional actions after a PE/COFF image has been loaded and relocated.

  For NT32, this function load symbols to support source level debugging.

  If ImageContext is NULL, then ASSERT().

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that has already been loaded and relocated.

**/
VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  VOID              *DllEntryPoint;
  CHAR16            *DllFileName;
  HMODULE           Library;
  UINTN             Index;
  
  ASSERT (ImageContext != NULL);

  if (mWinNt == NULL) {
    Nt32PeCoffGetWinNtThunkStucture ();
  }
	//
  // If we load our own PE COFF images the Windows debugger can not source
  //  level debug our code. If a valid PDB pointer exists usw it to load
  //  the *.dll file as a library using Windows* APIs. This allows 
  //  source level debug. The image is still loaded and reloaced
  //  in the Framework memory space like on a real system (by the code above),
  //  but the entry point points into the DLL loaded by the code bellow. 
  //

  DllEntryPoint = NULL;

  //
  // Load the DLL if it's not an EBC image.
  //
  if ((ImageContext->PdbPointer != NULL) &&
      (ImageContext->Machine != EFI_IMAGE_MACHINE_EBC)) {
    //
    // Convert filename from ASCII to Unicode
    //
    DllFileName = AsciiToUnicode (ImageContext->PdbPointer, &Index);

    //
    // Check that we have a valid filename
    //
    if (Index < 5 || DllFileName[Index - 4] != '.') {
      mWinNt->HeapFree (mWinNt->GetProcessHeap (), 0, DllFileName);

      //
      // Never return an error if PeCoffLoaderRelocateImage() succeeded.
      // The image will run, but we just can't source level debug. If we
      // return an error the image will not run.
      //
      return;
    }
    //
    // Replace .PDB with .DLL on the filename
    //
    DllFileName[Index - 3]  = 'D';
    DllFileName[Index - 2]  = 'L';
    DllFileName[Index - 1]  = 'L';

    //
    // Load the .DLL file into the user process's address space for source 
    // level debug
    //
    Library = mWinNt->LoadLibraryEx  (DllFileName, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (Library != NULL) {
      //
      // InitializeDriver is the entry point we put in all our EFI DLL's. The
      // DONT_RESOLVE_DLL_REFERENCES argument to LoadLIbraryEx() supresses the 
      // normal DLL entry point of DllMain, and prevents other modules that are
      // referenced in side the DllFileName from being loaded. There is no error 
      // checking as the we can point to the PE32 image loaded by Tiano. This 
      // step is only needed for source level debuging
      //
      DllEntryPoint = (VOID *) (UINTN) mWinNt->GetProcAddress (Library, "InitializeDriver");

    }

    if ((Library != NULL) && (DllEntryPoint != NULL)) {
      ImageContext->EntryPoint  = (EFI_PHYSICAL_ADDRESS) (UINTN) DllEntryPoint;
      DEBUG ((EFI_D_INFO, "LoadLibraryEx (%s,\n               NULL, DONT_RESOLVE_DLL_REFERENCES)\n", DllFileName));
     } else {
      DEBUG ((EFI_D_ERROR, "WARNING: No source level debug %s. \n", DllFileName));
    }

    mWinNt->HeapFree (mWinNt->GetProcessHeap (), 0, DllFileName);
  }

  //
  // Never return an error if PeCoffLoaderRelocateImage() succeeded.
  // The image will run, but we just can't source level debug. If we
  // return an error the image will not run.
  //
  return;
}  

/**
  Performs additional actions just before a PE/COFF image is unloaded.  Any resources
  that were allocated by PeCoffLoaderRelocateImageExtraAction() must be freed.
  
  For NT32, this function unloads symbols for source level debugging.

  If ImageContext is NULL, then ASSERT().
  
  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that is being unloaded.

**/
VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  ASSERT (ImageContext != NULL);
}
