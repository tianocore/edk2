/** @file
  A shell application to dump dynamic PCD settings.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/UnicodeCollation.h>
#include <Protocol/PiPcd.h>
#include <Protocol/Pcd.h>
#include <Protocol/PiPcdInfo.h>
#include <Protocol/PcdInfo.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/Shell.h>

//
// String token ID of help message text.
// Shell supports to find help message in the resource section of an application image if
// .MAN file is not found. This global variable is added to make build tool recognizes
// that the help string is consumed by user and then build tool will add the string into
// the resource section. Thus the application can use '-?' option to show help message in
// Shell.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_STRING_ID  mStrDumpDynPcdHelpTokenId = STRING_TOKEN (STR_DUMP_DYN_PCD_HELP_INFORMATION);

#define MAJOR_VERSION  1
#define MINOR_VERSION  0

static EFI_UNICODE_COLLATION_PROTOCOL  *mUnicodeCollation     = NULL;
static EFI_PCD_PROTOCOL                *mPiPcd                = NULL;
static PCD_PROTOCOL                    *mPcd                  = NULL;
static EFI_GET_PCD_INFO_PROTOCOL       *mPiPcdInfo            = NULL;
static GET_PCD_INFO_PROTOCOL           *mPcdInfo              = NULL;
static CHAR16                          *mTempPcdNameBuffer    = NULL;
static UINTN                           mTempPcdNameBufferSize = 0;

static CONST CHAR8  mHex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

static UINTN   Argc;
static CHAR16  **Argv;

/**

  This function parse application ARG.

  @return Status
**/
static
EFI_STATUS
GetArg (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellParameters
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Argc = ShellParameters->Argc;
  Argv = ShellParameters->Argv;
  return EFI_SUCCESS;
}

/**
   Display current version.
**/
static
VOID
ShowVersion (
  )
{
  Print (L"DumpDynPcd Version %d.%02d\n", MAJOR_VERSION, MINOR_VERSION);
}

/**
   Display Usage and Help information.
**/
static
VOID
ShowHelp (
  )
{
  Print (L"Dump dynamic[ex] PCD info.\n");
  Print (L"\n");
  Print (L"DumpDynPcd [PcdName]\n");
  Print (L"\n");
  Print (L"  PcdName    Specifies the name of PCD.\n");
  Print (L"             A literal[or partial] name or a pattern as specified in\n");
  Print (L"             the MetaiMatch() function of the EFI_UNICODE_COLLATION2_PROCOOL.\n");
  Print (L"             If it is absent, dump all PCDs' info.\n");
  Print (L"The PCD data is printed as hexadecimal dump.\n");
}

/**
  Dump some hexadecimal data to the screen.

  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the printing.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to print out.
**/
static
VOID
DumpHex (
  IN UINTN  Indent,
  IN UINTN  Offset,
  IN UINTN  DataSize,
  IN VOID   *UserData
  )
{
  UINT8  *Data;

  CHAR8  Val[50];

  CHAR8  Str[20];

  UINT8  TempByte;
  UINTN  Size;
  UINTN  Index;

  Data = UserData;
  while (DataSize != 0) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      TempByte           = Data[Index];
      Val[Index * 3 + 0] = mHex[TempByte >> 4];
      Val[Index * 3 + 1] = mHex[TempByte & 0xF];
      Val[Index * 3 + 2] = (CHAR8)((Index == 7) ? '-' : ' ');
      Str[Index]         = (CHAR8)((TempByte < ' ' || TempByte > 'z') ? '.' : TempByte);
    }

    Val[Index * 3] = 0;
    Str[Index]     = 0;
    Print (L"%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str);

    Data     += Size;
    Offset   += Size;
    DataSize -= Size;
  }
}

/**
  Safely append with automatic string resizing given length of Destination and
  desired length of copy from Source.

  append the first D characters of Source to the end of Destination, where D is
  the lesser of Count and the StrLen() of Source. If appending those D characters
  will fit within Destination (whose Size is given as CurrentSize) and
  still leave room for a NULL terminator, then those characters are appended,
  starting at the original terminating NULL of Destination, and a new terminating
  NULL is appended.

  If appending D characters onto Destination will result in a overflow of the size
  given in CurrentSize the string will be grown such that the copy can be performed
  and CurrentSize will be updated to the new size.

  If Source is NULL, there is nothing to append, just return the current buffer in
  Destination.

  if Destination is NULL, then ASSERT()
  if Destination's current length (including NULL terminator) is already more then
  CurrentSize, then ASSERT()

  @param[in, out] Destination   The String to append onto
  @param[in, out] CurrentSize   on call the number of bytes in Destination.  On
                                return possibly the new size (still in bytes).  if NULL
                                then allocate whatever is needed.
  @param[in]      Source        The String to append from

  @return Destination           return the resultant string.
**/
static
CHAR16 *
InternalStrnCatGrow (
  IN OUT CHAR16        **Destination,
  IN OUT UINTN         *CurrentSize,
  IN     CONST CHAR16  *Source
  )
{
  UINTN  DestinationStartSize;
  UINTN  NewSize;
  UINTN  SourceLen;

  SourceLen = StrLen (Source);

  //
  // ASSERTs
  //
  ASSERT (Destination != NULL);

  //
  // If there's nothing to do then just return Destination
  //
  if (Source == NULL) {
    return (*Destination);
  }

  //
  // allow for un-initialized pointers, based on size being 0
  //
  if ((CurrentSize != NULL) && (*CurrentSize == 0)) {
    *Destination = NULL;
  }

  //
  // allow for NULL pointers address as Destination
  //
  if (*Destination != NULL) {
    ASSERT (CurrentSize != 0);
    DestinationStartSize = StrSize (*Destination);
    ASSERT (DestinationStartSize <= *CurrentSize);
  } else {
    DestinationStartSize = 0;
  }

  //
  // Test and grow if required
  //
  if (CurrentSize != NULL) {
    NewSize = *CurrentSize;
    if (NewSize < DestinationStartSize + (SourceLen * sizeof (CHAR16))) {
      while (NewSize < (DestinationStartSize + (SourceLen*sizeof (CHAR16)))) {
        NewSize += 2 * SourceLen * sizeof (CHAR16);
      }

      *Destination = ReallocatePool (*CurrentSize, NewSize, *Destination);
      *CurrentSize = NewSize;
    }
  } else {
    NewSize      = (SourceLen + 1)*sizeof (CHAR16);
    *Destination = AllocateZeroPool (NewSize);
  }

  //
  // Now use standard StrnCat on a big enough buffer
  //
  if (*Destination == NULL) {
    return (NULL);
  }

  StrnCatS (*Destination, NewSize/sizeof (CHAR16), Source, SourceLen);
  return *Destination;
}

/**
  Get PCD type string based on input PCD type.

  @param[in]    TokenSpace      PCD Token Space.
  @param[in]    PcdType         The input PCD type.

  @return       Pointer to PCD type string.
**/
static
CHAR16 *
GetPcdTypeString (
  IN CONST EFI_GUID  *TokenSpace,
  IN EFI_PCD_TYPE    PcdType
  )
{
  UINTN   BufLen;
  CHAR16  *RetString;

  BufLen    = 0;
  RetString = NULL;

  switch (PcdType) {
    case EFI_PCD_TYPE_8:
      InternalStrnCatGrow (&RetString, &BufLen, L"UINT8");
      break;
    case EFI_PCD_TYPE_16:
      InternalStrnCatGrow (&RetString, &BufLen, L"UINT16");
      break;
    case EFI_PCD_TYPE_32:
      InternalStrnCatGrow (&RetString, &BufLen, L"UINT32");
      break;
    case EFI_PCD_TYPE_64:
      InternalStrnCatGrow (&RetString, &BufLen, L"UINT64");
      break;
    case EFI_PCD_TYPE_BOOL:
      InternalStrnCatGrow (&RetString, &BufLen, L"BOOLEAN");
      break;
    case EFI_PCD_TYPE_PTR:
      InternalStrnCatGrow (&RetString, &BufLen, L"POINTER");
      break;
    default:
      InternalStrnCatGrow (&RetString, &BufLen, L"UNKNOWN");
      break;
  }

  if (TokenSpace == NULL) {
    InternalStrnCatGrow (&RetString, &BufLen, L":DYNAMIC");
  } else {
    InternalStrnCatGrow (&RetString, &BufLen, L":DYNAMICEX");
  }

  return RetString;
}

/**
  Dump PCD info.

  @param[in]    TokenSpace      PCD Token Space.
  @param[in]    TokenNumber     PCD Token Number.
  @param[in]    PcdInfo         Pointer to PCD info.
**/
static
VOID
DumpPcdInfo (
  IN CONST EFI_GUID  *TokenSpace,
  IN UINTN           TokenNumber,
  IN EFI_PCD_INFO    *PcdInfo
  )
{
  CHAR16   *RetString;
  UINT8    Uint8;
  UINT16   Uint16;
  UINT32   Uint32;
  UINT64   Uint64;
  BOOLEAN  Boolean;
  VOID     *PcdData;

  RetString = NULL;

  if (PcdInfo->PcdName != NULL) {
    Print (L"%a\n", PcdInfo->PcdName);
  } else {
    if (TokenSpace == NULL) {
      Print (L"Default Token Space\n");
    } else {
      Print (L"%g\n", TokenSpace);
    }
  }

  RetString = GetPcdTypeString (TokenSpace, PcdInfo->PcdType);

  switch (PcdInfo->PcdType) {
    case EFI_PCD_TYPE_8:
      if (TokenSpace == NULL) {
        Uint8 = mPcd->Get8 (TokenNumber);
      } else {
        Uint8 = mPiPcd->Get8 (TokenSpace, TokenNumber);
      }

      Print (L"  Token = 0x%08x - Type = %H%-17s%N - Size = 0x%x - Value = 0x%x\n", TokenNumber, RetString, PcdInfo->PcdSize, Uint8);
      break;
    case EFI_PCD_TYPE_16:
      if (TokenSpace == NULL) {
        Uint16 = mPcd->Get16 (TokenNumber);
      } else {
        Uint16 = mPiPcd->Get16 (TokenSpace, TokenNumber);
      }

      Print (L"  Token = 0x%08x - Type = %H%-17s%N - Size = 0x%x - Value = 0x%x\n", TokenNumber, RetString, PcdInfo->PcdSize, Uint16);
      break;
    case EFI_PCD_TYPE_32:
      if (TokenSpace == NULL) {
        Uint32 = mPcd->Get32 (TokenNumber);
      } else {
        Uint32 = mPiPcd->Get32 (TokenSpace, TokenNumber);
      }

      Print (L"  Token = 0x%08x - Type = %H%-17s%N - Size = 0x%x - Value = 0x%x\n", TokenNumber, RetString, PcdInfo->PcdSize, Uint32);
      break;
    case EFI_PCD_TYPE_64:
      if (TokenSpace == NULL) {
        Uint64 = mPcd->Get64 (TokenNumber);
      } else {
        Uint64 = mPiPcd->Get64 (TokenSpace, TokenNumber);
      }

      Print (L"  Token = 0x%08x - Type = %H%-17s%N - Size = 0x%x - Value = 0x%lx\n", TokenNumber, RetString, PcdInfo->PcdSize, Uint64);
      break;
    case EFI_PCD_TYPE_BOOL:
      if (TokenSpace == NULL) {
        Boolean = mPcd->GetBool (TokenNumber);
      } else {
        Boolean = mPiPcd->GetBool (TokenSpace, TokenNumber);
      }

      Print (L"  Token = 0x%08x - Type = %H%-17s%N - Size = 0x%x - Value = %a\n", TokenNumber, RetString, PcdInfo->PcdSize, Boolean ? "TRUE" : "FALSE");
      break;
    case EFI_PCD_TYPE_PTR:
      if (TokenSpace == NULL) {
        PcdData = mPcd->GetPtr (TokenNumber);
      } else {
        PcdData = mPiPcd->GetPtr (TokenSpace, TokenNumber);
      }

      Print (L"  Token = 0x%08x - Type = %H%-17s%N - Size = 0x%x\n", TokenNumber, RetString, PcdInfo->PcdSize);
      DumpHex (2, 0, PcdInfo->PcdSize, PcdData);
      break;
    default:
      return;
  }

  if (RetString != NULL) {
    FreePool (RetString);
  }

  Print (L"\n");
}

/**
  Show one or all PCDs' info.

  @param[in]  InputPcdName       Pointer to PCD name to show. If NULL, show all PCDs' info.

  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Not enough resources were available to run the command.
  @retval EFI_ABORTED            Aborted by user.
  @retval EFI_NOT_FOUND          The specified PCD is not found.
**/
static
EFI_STATUS
ProcessPcd (
  IN CHAR16  *InputPcdName
  )
{
  EFI_STATUS    Status;
  EFI_GUID      *TokenSpace;
  UINTN         TokenNumber;
  EFI_PCD_INFO  PcdInfo;
  BOOLEAN       Found;
  UINTN         PcdNameSize;

  PcdInfo.PcdName = NULL;
  PcdInfo.PcdSize = 0;
  PcdInfo.PcdType = 0xFF;
  Found           = FALSE;

  Print (L"Current system SKU ID: 0x%x\n\n", mPiPcdInfo->GetSku ());

  TokenSpace = NULL;
  do {
    TokenNumber = 0;
    do {
      Status = mPiPcd->GetNextToken (TokenSpace, &TokenNumber);
      if (!EFI_ERROR (Status) && (TokenNumber != 0)) {
        if (TokenSpace == NULL) {
          //
          // PCD in default Token Space.
          //
          mPcdInfo->GetInfo (TokenNumber, &PcdInfo);
        } else {
          mPiPcdInfo->GetInfo (TokenSpace, TokenNumber, &PcdInfo);
        }

        if (InputPcdName != NULL) {
          if (PcdInfo.PcdName == NULL) {
            continue;
          }

          PcdNameSize = AsciiStrSize (PcdInfo.PcdName) * sizeof (CHAR16);
          if (mTempPcdNameBuffer == NULL) {
            mTempPcdNameBufferSize = PcdNameSize;
            mTempPcdNameBuffer     = AllocatePool (mTempPcdNameBufferSize);
          } else if (mTempPcdNameBufferSize < PcdNameSize) {
            mTempPcdNameBuffer     = ReallocatePool (mTempPcdNameBufferSize, PcdNameSize, mTempPcdNameBuffer);
            mTempPcdNameBufferSize = PcdNameSize;
          }

          if (mTempPcdNameBuffer == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }

          AsciiStrToUnicodeStrS (PcdInfo.PcdName, mTempPcdNameBuffer, mTempPcdNameBufferSize / sizeof (CHAR16));
          //
          // Compare the input PCD name with the PCD name in PCD database.
          //
          if ((StrStr (mTempPcdNameBuffer, InputPcdName) != NULL) ||
              ((mUnicodeCollation != NULL) && mUnicodeCollation->MetaiMatch (mUnicodeCollation, mTempPcdNameBuffer, InputPcdName)))
          {
            //
            // Found matched PCD.
            //
            DumpPcdInfo (TokenSpace, TokenNumber, &PcdInfo);
            Found = TRUE;
          }
        } else {
          DumpPcdInfo (TokenSpace, TokenNumber, &PcdInfo);
        }
      }
    } while (!EFI_ERROR (Status) && TokenNumber != 0);

    Status = mPiPcd->GetNextTokenSpace ((CONST EFI_GUID **)&TokenSpace);
  } while (!EFI_ERROR (Status) && TokenSpace != NULL);

  if ((InputPcdName != NULL) && !Found) {
    //
    // The specified PCD is not found, print error.
    //
    Print (L"%EError. %NNo matching PCD found: %s.\n", InputPcdName);
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Main entrypoint for DumpDynPcd shell application.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_INVALID_PARAMETER  Command usage error.
  @retval EFI_OUT_OF_RESOURCES   Not enough resources were available to run the command.
  @retval EFI_ABORTED            Aborted by user.
  @retval EFI_NOT_FOUND          The specified PCD is not found.
  @retval Others                 Error status returned from gBS->LocateProtocol.
**/
EFI_STATUS
EFIAPI
DumpDynPcdMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  CHAR16      *InputPcdName;

  InputPcdName = NULL;

  Status = gBS->LocateProtocol (&gEfiUnicodeCollation2ProtocolGuid, NULL, (VOID **)&mUnicodeCollation);
  if (EFI_ERROR (Status)) {
    mUnicodeCollation = NULL;
  }

  Status = gBS->LocateProtocol (&gEfiPcdProtocolGuid, NULL, (VOID **)&mPiPcd);
  if (EFI_ERROR (Status)) {
    Print (L"DumpDynPcd: %EError. %NPI PCD protocol is not present.\n");
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiGetPcdInfoProtocolGuid, NULL, (VOID **)&mPiPcdInfo);
  if (EFI_ERROR (Status)) {
    Print (L"DumpDynPcd: %EError. %NPI PCD info protocol is not present.\n");
    return Status;
  }

  Status = gBS->LocateProtocol (&gPcdProtocolGuid, NULL, (VOID **)&mPcd);
  if (EFI_ERROR (Status)) {
    Print (L"DumpDynPcd: %EError. %NPCD protocol is not present.\n");
    return Status;
  }

  Status = gBS->LocateProtocol (&gGetPcdInfoProtocolGuid, NULL, (VOID **)&mPcdInfo);
  if (EFI_ERROR (Status)) {
    Print (L"DumpDynPcd: %EError. %NPCD info protocol is not present.\n");
    return Status;
  }

  //
  // get the command line arguments
  //
  Status = GetArg ();
  if (EFI_ERROR (Status)) {
    Print (L"DumpDynPcd: %EError. %NThe input parameters are not recognized.\n");
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  if (Argc > 2) {
    Print (L"DumpDynPcd: %EError. %NToo many arguments specified.\n");
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  if (Argc == 1) {
    Status = ProcessPcd (InputPcdName);
    goto Done;
  }

  if ((StrCmp (Argv[1], L"-?") == 0) || (StrCmp (Argv[1], L"-h") == 0) || (StrCmp (Argv[1], L"-H") == 0)) {
    ShowHelp ();
    goto Done;
  } else {
    if ((StrCmp (Argv[1], L"-v") == 0) || (StrCmp (Argv[1], L"-V") == 0)) {
      ShowVersion ();
      goto Done;
    } else {
      if (StrStr (Argv[1], L"-") != NULL) {
        Print (L"DumpDynPcd: %EError. %NThe argument '%B%s%N' is invalid.\n", Argv[1]);
        goto Done;
      }
    }
  }

  InputPcdName = Argv[1];
  Status       = ProcessPcd (InputPcdName);

Done:

  if (mTempPcdNameBuffer != NULL) {
    FreePool (mTempPcdNameBuffer);
  }

  return Status;
}
