/**@file

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
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
  This version only works for DXE phase  


**/
//
// The package level header files this module uses
//
#include <WinNtDxe.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/WinNtThunk.h>

#include <Library/PeCoffLib.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffExtraActionLib.h>

#define MAX_PDB_NAME_TO_MOD_HANDLE_ARRAY_SIZE 0x100

typedef struct {
  CHAR8   *PdbPointer;
  VOID    *ModHandle;
} PDB_NAME_TO_MOD_HANDLE;


//
// Cache of WinNtThunk protocol
//
EFI_WIN_NT_THUNK_PROTOCOL   *mWinNt = NULL;

//
// An Array to hold the ModHandle
//
PDB_NAME_TO_MOD_HANDLE  *mPdbNameModHandleArray = NULL;
UINTN                   mPdbNameModHandleArraySize = 0;


/**
  The constructor function gets  the pointer of the WinNT thunk functions
  It will ASSERT() if NT thunk protocol is not installed.

  @retval EFI_SUCCESS   WinNT thunk protocol is found and cached.

**/
EFI_STATUS
EFIAPI
Nt32PeCoffGetWinNtThunkStucture (
  VOID
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;

  //
  // Retrieve WinNtThunkProtocol from GUID'ed HOB
  //
  GuidHob = GetFirstGuidHob (&gEfiWinNtThunkProtocolGuid);
  ASSERT (GuidHob != NULL);
  mWinNt = (EFI_WIN_NT_THUNK_PROTOCOL *)(*(UINTN *)(GET_GUID_HOB_DATA (GuidHob)));
  ASSERT (mWinNt != NULL);


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
  Store the ModHandle in an array indexed by the Pdb File name.
  The ModHandle is needed to unload the image. 


  @param ImageContext - Input data returned from PE Laoder Library. Used to find the 
                 .PDB file name of the PE Image.
  @param ModHandle    - Returned from LoadLibraryEx() and stored for call to 
                 FreeLibrary().

  @return   return EFI_SUCCESS when ModHandle was stored. 

--*/
EFI_STATUS
AddModHandle (
  IN  PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext,
  IN  VOID                                 *ModHandle
  )

{
  UINTN                   Index;
  PDB_NAME_TO_MOD_HANDLE  *Array;
  UINTN                   PreviousSize;
  PDB_NAME_TO_MOD_HANDLE  *TempArray;
  HANDLE                  Handle;

  //
  // Return EFI_ALREADY_STARTED if this DLL has already been loaded
  //
  Array = mPdbNameModHandleArray;
  for (Index = 0; Index < mPdbNameModHandleArraySize; Index++, Array++) {
    if (Array->PdbPointer != NULL && Array->ModHandle == ModHandle) {
      return EFI_ALREADY_STARTED;
    }
  }
  
  Array = mPdbNameModHandleArray;
  for (Index = 0; Index < mPdbNameModHandleArraySize; Index++, Array++) {
    if (Array->PdbPointer == NULL) {
      //
      // Make a copy of the stirng and store the ModHandle
      //
      Handle = mWinNt->GetProcessHeap ();
      Array->PdbPointer = mWinNt->HeapAlloc ( Handle,
                                HEAP_ZERO_MEMORY,
                                AsciiStrLen (ImageContext->PdbPointer) + 1
                               ); 
                               
      ASSERT (Array->PdbPointer != NULL);

      AsciiStrCpy (Array->PdbPointer, ImageContext->PdbPointer);
      Array->ModHandle = ModHandle;
      return EFI_SUCCESS;
    }
  }
  
  //
  // No free space in mPdbNameModHandleArray so grow it by 
  // MAX_PDB_NAME_TO_MOD_HANDLE_ARRAY_SIZE entires. 
  //
  PreviousSize = mPdbNameModHandleArraySize * sizeof (PDB_NAME_TO_MOD_HANDLE);
  mPdbNameModHandleArraySize += MAX_PDB_NAME_TO_MOD_HANDLE_ARRAY_SIZE;
  //
  // re-allocate a new buffer and copy the old values to the new locaiton. 
  //
  TempArray = mWinNt->HeapAlloc ( mWinNt->GetProcessHeap (),
                                HEAP_ZERO_MEMORY,
                                mPdbNameModHandleArraySize * sizeof (PDB_NAME_TO_MOD_HANDLE)
                               ); 
 
  CopyMem ((VOID *) (UINTN) TempArray, (VOID *) (UINTN)mPdbNameModHandleArray, PreviousSize);
  
  mWinNt->HeapFree (mWinNt->GetProcessHeap (), 0, mPdbNameModHandleArray);
  
  mPdbNameModHandleArray = TempArray;
  if (mPdbNameModHandleArray == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }
  
  
  return AddModHandle (ImageContext, ModHandle);
}
/**
  Return the ModHandle and delete the entry in the array.


   @param  ImageContext - Input data returned from PE Laoder Library. Used to find the 
                 .PDB file name of the PE Image.

  @return   
    ModHandle - ModHandle assoicated with ImageContext is returned
    NULL      - No ModHandle associated with ImageContext

**/
VOID *
RemoveModeHandle (
  IN  PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  UINTN                   Index;
  PDB_NAME_TO_MOD_HANDLE  *Array;

  if (ImageContext->PdbPointer == NULL) {
    //
    // If no PDB pointer there is no ModHandle so return NULL
    //
    return NULL;
  }

  Array = mPdbNameModHandleArray;
  for (Index = 0; Index < mPdbNameModHandleArraySize; Index++, Array++) {
    if ((Array->PdbPointer != NULL) && (AsciiStrCmp(Array->PdbPointer, ImageContext->PdbPointer) == 0)) {
      //
      // If you find a match return it and delete the entry
      //
      mWinNt->HeapFree (mWinNt->GetProcessHeap (), 0, Array->PdbPointer);
      Array->PdbPointer = NULL;
      return Array->ModHandle;
    }
  }

  return NULL;
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
  EFI_STATUS        Status;
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
      Status = AddModHandle (ImageContext, Library);
      if (Status == EFI_ALREADY_STARTED) {
        //
        // If the DLL has already been loaded before, then this instance of the DLL can not be debugged.
        //
        ImageContext->PdbPointer = NULL;
        DEBUG ((EFI_D_ERROR, "WARNING: DLL already loaded.  No source level debug %s. \n", DllFileName));
      } else {
        //
        // This DLL is not already loaded, so source level debugging is suported.
        //
        ImageContext->EntryPoint  = (EFI_PHYSICAL_ADDRESS) (UINTN) DllEntryPoint;
        DEBUG ((EFI_D_INFO, "LoadLibraryEx (%s,\n               NULL, DONT_RESOLVE_DLL_REFERENCES)\n", DllFileName));
      }
    } else {
      //
      // This DLL does not support source level debugging at all.
      //
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
  VOID *ModHandle;

  ASSERT (ImageContext != NULL);

  ModHandle = RemoveModeHandle (ImageContext);
  if (ModHandle != NULL) {
    mWinNt->FreeLibrary (ModHandle);
  }
  return;
}
