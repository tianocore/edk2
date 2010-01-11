/** @file
  This is a simple shell application

  This should be executed with "/Param2 Val1" and "/Param1" as the 2 command line options!

  Copyright (c) 2008-2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

SHELL_PARAM_ITEM ParamList[] = {
  {L"/Param1", TypeFlag},
  {L"/Param2", TypeValue},
  {L"/Param3", TypeDoubleValue},
  {L"/Param4", TypeMaxValue},
  {NULL, TypeMax}};

/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_FILE_HANDLE     FileHandle;
  EFI_STATUS          Status;
  CHAR16              FileName[100];
  UINTN               BufferSize;
  UINT64              Position;
  UINT8               Buffer[200];
  EFI_FILE_INFO       *pFileInfo;
  UINT64              Size;
  BOOLEAN             NoFile;
  EFI_SHELL_FILE_INFO *pShellFileInfo;
  LIST_ENTRY          *List;
  // CONST CHAR16              *Tester;
  
  FileHandle = NULL;
  StrCpy(FileName, L"testfile.txt");
//  Position = 0;
  pFileInfo = NULL;
  Size = 0;
  NoFile = FALSE;
  pShellFileInfo = NULL;
  List = NULL;

  // command line param functions
  Status = ShellCommandLineParse(ParamList, &List, NULL, FALSE);
  // if you put an invalid parameter you SHOULD hit this assert.
  ASSERT_EFI_ERROR(Status);
  if (List) {
    ASSERT(ShellCommandLineGetFlag(List, L"/Param5") == FALSE);
    ASSERT(ShellCommandLineGetFlag(List, L"/Param1") != FALSE);
    ASSERT(StrCmp(ShellCommandLineGetValue(List, L"/Param2"), L"Val1")==0);
    ASSERT(StrCmp(ShellCommandLineGetRawValue(List, 0), L"SimpleApplication.efi")==0);
    // Tester = ShellCommandLineGetValue(List, L"/Param3");
    // Tester = ShellCommandLineGetValue(List, L"/Param4");

    ShellCommandLineFreeVarList(List);
  } else {
    Print(L"param checking skipped.\r\n");
  }

//  return (EFI_SUCCESS);


  ASSERT(ShellGetExecutionBreakFlag() == FALSE);
  ASSERT(StrCmp(ShellGetCurrentDir(NULL), L"f10:\\") == 0);
  Print(L"execution break and get cur dir - pass\r\n");

  ShellSetPageBreakMode(TRUE);

  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  ASSERT_EFI_ERROR(Status);

  BufferSize = StrSize(FileName);
  Status = ShellWriteFile(FileHandle, &BufferSize, FileName);
  ASSERT_EFI_ERROR(Status);
  Status = ShellGetFilePosition(FileHandle, &Position);
  ASSERT_EFI_ERROR(Status);
  ASSERT(Position == 0x1A);
  Status = ShellSetFilePosition(FileHandle, 0);
  ASSERT_EFI_ERROR(Status);
  BufferSize = sizeof(Buffer) * sizeof(Buffer[0]);
  Status = ShellReadFile(FileHandle, &BufferSize, Buffer);
  ASSERT_EFI_ERROR(Status);
  ASSERT(BufferSize == 0x1A);
  ASSERT(StrCmp((CHAR16*)Buffer, FileName) == 0);
  pFileInfo = ShellGetFileInfo(FileHandle);
  ASSERT(pFileInfo != NULL);
  ASSERT(StrCmp(pFileInfo->FileName, FileName) == 0);
  ASSERT(pFileInfo->FileSize == 0x1A);
  FreePool(pFileInfo);
  pFileInfo = NULL;
  Status = ShellCloseFile(&FileHandle);
  ASSERT_EFI_ERROR(Status);
  Print(L"read, write, create, getinfo - pass\r\n");

  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  ASSERT_EFI_ERROR(Status);
  pFileInfo = ShellGetFileInfo(FileHandle);
  ASSERT(pFileInfo != NULL);
  pFileInfo->FileSize = 0x20;
  Status = ShellSetFileInfo(FileHandle, pFileInfo);
  FreePool(pFileInfo);
  pFileInfo = NULL; 
  ASSERT_EFI_ERROR(Status);
  pFileInfo = ShellGetFileInfo(FileHandle);
  ASSERT(pFileInfo != NULL);
  ASSERT(StrCmp(pFileInfo->FileName, FileName) == 0);
  ASSERT(pFileInfo->PhysicalSize == 0x20);
  ASSERT(pFileInfo->FileSize == 0x20);
  ASSERT((pFileInfo->Attribute&EFI_FILE_DIRECTORY)==0);
  FreePool(pFileInfo);
  Status = ShellGetFileSize(FileHandle, &Size);
  ASSERT(Size == 0x20);
  ASSERT_EFI_ERROR(Status);
  Status = ShellCloseFile(&FileHandle);
  ASSERT_EFI_ERROR(Status);
  Print(L"setinfo and change size, getsize - pass\r\n");
  
  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  ASSERT_EFI_ERROR(Status);

  pFileInfo = ShellGetFileInfo(FileHandle);
  ASSERT(pFileInfo != NULL);
  ASSERT(StrCmp(pFileInfo->FileName, FileName) == 0);
  ASSERT(pFileInfo->PhysicalSize == 0x20);
  ASSERT(pFileInfo->FileSize == 0x20);
  ASSERT((pFileInfo->Attribute&EFI_FILE_DIRECTORY)==0);
  FreePool(pFileInfo);
  pFileInfo = NULL;   
  Status = ShellDeleteFile(&FileHandle);
  ASSERT_EFI_ERROR(Status);
  Print(L"reopen file - pass\r\n");

  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  ASSERT_EFI_ERROR(Status);
  pFileInfo = ShellGetFileInfo(FileHandle);
  ASSERT(pFileInfo != NULL);
  ASSERT(StrCmp(pFileInfo->FileName, FileName) == 0);
  ASSERT(pFileInfo->PhysicalSize == 0x0);
  ASSERT(pFileInfo->FileSize == 0x0);
  ASSERT((pFileInfo->Attribute&EFI_FILE_DIRECTORY)==0);
  FreePool(pFileInfo);
  Status = ShellDeleteFile(&FileHandle);
  ASSERT_EFI_ERROR(Status);
  Print(L"size of empty - pass\r\n");

  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  ASSERT(Status == EFI_NOT_FOUND);
  ASSERT(FileHandle == NULL);

  Status = ShellCreateDirectory(FileName, &FileHandle);
  ASSERT_EFI_ERROR(Status);
  ASSERT(FileHandle != NULL);
  pFileInfo = ShellGetFileInfo(FileHandle);
  ASSERT(pFileInfo != NULL);
  ASSERT(StrCmp(pFileInfo->FileName, FileName) == 0);
  ASSERT(pFileInfo->Attribute&EFI_FILE_DIRECTORY);
  Status = ShellDeleteFile(&FileHandle);
  ASSERT_EFI_ERROR(Status);
  Print(L"Directory create - pass\r\n");
  
  // FindFirst and FindNext
  StrCpy(FileName, L"testDir");
  Status = ShellCreateDirectory(FileName, &FileHandle);
  Status = ShellCloseFile(&FileHandle);
  StrCat(FileName, L"\\File.txt");
  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  ASSERT_EFI_ERROR(Status);
  Status = ShellCloseFile(&FileHandle);
  StrCpy(FileName, L"testDir");
  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  ASSERT_EFI_ERROR(Status);
  Status = ShellFindFirstFile(FileHandle, &pFileInfo);
  ASSERT_EFI_ERROR(Status);
  Status = ShellFindNextFile(FileHandle, pFileInfo, &NoFile);
  ASSERT_EFI_ERROR(Status);
  ASSERT(NoFile == FALSE);
  Status = ShellFindNextFile(FileHandle, pFileInfo, &NoFile);
  ASSERT_EFI_ERROR(Status);
  ASSERT(NoFile == FALSE);
  Status = ShellFindNextFile(FileHandle, pFileInfo, &NoFile);
  ASSERT_EFI_ERROR(Status);
  ///@todo - why is NoFile never set? limitation of NT32 file system?
  Status = ShellDeleteFile(&FileHandle);
  ASSERT(Status == RETURN_WARN_DELETE_FAILURE);
  Print(L"FindFirst - pass\r\n");
  Print(L"FindNext - Verify with real EFI system.  Cant verify NoFile under NT32\r\n");

  // open and close meta arg
  Status = ShellOpenFileMetaArg(L"testDir\\*.*", EFI_FILE_MODE_READ, &pShellFileInfo);
  ASSERT_EFI_ERROR(Status);
  ASSERT(pShellFileInfo->Status == 0);
  ASSERT(StrCmp(pShellFileInfo->FileName, L"File.txt") == 0);
  ASSERT(pShellFileInfo->Handle);
  ASSERT(pShellFileInfo->Info);
  ASSERT(pShellFileInfo->Info->FileSize == 0);
  ASSERT(StrCmp(pShellFileInfo->Info->FileName, L"File.txt") == 0);
  ASSERT(pShellFileInfo->Info->Attribute == 0);

  Status = ShellCloseFileMetaArg(&pShellFileInfo);
  ASSERT_EFI_ERROR(Status);
  Print(L"Open/Close Meta Arg - pass\r\n");

  // now delete that file and that directory
  StrCat(FileName, L"\\File.txt");
  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  ASSERT_EFI_ERROR(Status);
  Status = ShellDeleteFile(&FileHandle);
  StrCpy(FileName, L"testDir");
  ASSERT_EFI_ERROR(Status);
  Status = ShellOpenFileByName(FileName, 
                               &FileHandle, 
                               EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 
                               0
                               );
  Status = ShellDeleteFile(&FileHandle);
  ASSERT_EFI_ERROR(Status);

  // get environment variable
  // made for testing under nt32
  ASSERT(StrCmp(ShellGetEnvironmentVariable(L"path"), L".;f10:\\efi\\tools;f10:\\efi\\boot;f10:\\;f9:\\efi\\tools;f9:\\efi\\boot;f9:\\") == 0);
  Print(L"ShellGetEnvironmentVariable - pass\r\n");

  // set environment variable
  Status = ShellSetEnvironmentVariable(L"", L"", FALSE);
  ASSERT(Status == EFI_UNSUPPORTED);
  Print(L"ShellSetEnvironmentVariable - pass\r\n");

  // ShellExecute
  Status = ShellExecute(&ImageHandle, L"EmptyApplication.efi", TRUE, NULL, NULL);
  ASSERT_EFI_ERROR(Status);
  // the pass printout for this is performed by EmptyApplication
  Print(L"\r\n");

  // page break mode (done last so we can see the results)
  // we set this true at the begining of the program
  // this is enough lines to trigger the page...
  Print(L"1\r\n2\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9\r\n10\r\n11\r\n12\r\n13\r\n14\r\n15\r\n16\r\n17\r\n18\r\n19\r\n20\r\n21\r\n22\r\n23\r\n24\r\n25\r\n26\r\n27\r\n28\r\n29\r\n30\r\n31\r\n");
  ShellSetPageBreakMode(FALSE);
  Print(L"1\r\n2\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9\r\n10\r\n11\r\n12\r\n13\r\n14\r\n15\r\n16\r\n17\r\n18\r\n19\r\n20\r\n21\r\n22\r\n23\r\n24\r\n25\r\n26\r\n27\r\n28\r\n29\r\n30\r\n31\r\n32\r\n33\r\n34\r\n35\r\n36\r\n37\r\n38\r\n39\r\n40\r\n41\r\n42\r\n43\r\n44\r\n45\r\n46\r\n47\r\n48\r\n49\r\n50\r\n51\r\n52\r\n53\r\n54\r\n55\r\n56\r\n57\r\n58\r\n59\r\n60\r\n");

  return EFI_SUCCESS;
}


/*
done - ShellGetFileInfo
done - ShellSetFileInfo
done - ShellOpenFileByDevicePath
done - ShellOpenFileByName
done - ShellCreateDirectory
done - ShellReadFile
done - ShellWriteFile
done - ShellCloseFile
done - ShellDeleteFile
done - ShellSetFilePosition
done - ShellGetFilePosition
???? - ShellFlushFile
done - ShellFindFirstFile
done - ShellFindNextFile
done - ShellGetFileSize
done - ShellGetExecutionBreakFlag
done - ShellGetEnvironmentVariable
done - ShellSetEnvironmentVariable
done - ShellExecute
done - ShellGetCurrentDir
done - ShellSetPageBreakMode
done - ShellOpenFileMetaArg
done - ShellCloseFileMetaArg
done - ShellCommandLineParse
done - ShellCommandLineFreeVarList
done - ShellCommandLineGetFlag
done - ShellCommandLineGetValue
done - ShellCommandLineGetRawValue
*/
