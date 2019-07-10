/** @file

 The API to parse the binary.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __GNUC__
#include "windows.h"
#else
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include "BinaryParse.h"
#include "BinaryCreate.h"
#include "VariableCommon.h"
#include "FirmwareVolumeBufferLib.h"

extern G_EFI_FD_INFO  gEfiFdInfo;
extern EFI_HANDLE     mParsedGuidedSectionTools;
extern CHAR8          mInputFdName[MAX_FILENAME_LEN];

//
// The Guid to sign the position of Vfr and Uni array in FV
//
EFI_GUID  gVfrArrayAttractGuid                         = EFI_VFR_ATTRACT_GUID;
EFI_GUID  gUniStrArrayAttractGuid                      = EFI_UNI_STR_ATTRACT_GUID;
EFI_GUID  gEfiSystemNvDataFvGuid                       = EFI_SYSTEM_NVDATA_FV_GUID;
EFI_GUID  gEfiCrc32GuidedSectionExtractionProtocolGuid = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;

/**
  Converts a three byte length value into a UINT32.

  @param  ThreeByteLength   Pointer to the first of the 3 byte length.

  @retval  Length           Size of the section

**/
static
UINT32
Get3ByteLength (
  IN UINT8     *ThreeByteLength
  )
{
  UINT32  Length;

  Length = 0;

  if (ThreeByteLength == NULL) {
    return 0;
  }

  Length  = *((UINT32 *) ThreeByteLength);
  Length  = Length & 0x00FFFFFF;

  return Length;
}

/**
  Generate the unique template filename.
**/
CHAR8 *
GenTempFile (
 VOID
 )
{
  CHAR8   *TemString;
  TemString = NULL;
#ifndef __GNUC__
  TemString = CloneString (tmpnam (NULL));
#else
  CHAR8 tmp[] = "/tmp/fileXXXXXX";
  UINTN Fdtmp;
  Fdtmp = mkstemp(tmp);
  TemString = CloneString(tmp);
  close(Fdtmp);
#endif
  return TemString;
}

/**
  Check whether exist the same Ifr FFS. If not existed, return TRUE.

  @param[in]   FfsImage   The pointer to the binary.
  @param[in]   FileSize   The size of binary.

  @return The string after convert.
**/
static
BOOLEAN
NotExistSameFfsIfr (
  IN  VOID     *FfsImage
)
{
  UINT32  Index;

  Index = 0;

  while (gEfiFdInfo.FfsArray[Index] != NULL) {
    if (memcmp (gEfiFdInfo.FfsArray[Index], FfsImage, sizeof (EFI_GUID)) == 0) {
      return FALSE;
    }
    Index++;
  }
  return TRUE;
}

/**
  This function returns the next larger size that meets the alignment
  requirement specified.

  @param  ActualSize        The size.
  @param  Alignment         The desired alignment.

  @retval The Occupied length

**/
static
UINT32
GetOccupiedSize (
  IN UINT32  ActualSize,
  IN UINT32  Alignment
  )
{
  UINT32  OccupiedSize;

  OccupiedSize = ActualSize;
  while ((OccupiedSize & (Alignment - 1)) != 0) {
    OccupiedSize++;
  }

  return OccupiedSize;
}


/**
  Parses FFS Sections, and remove the FFS headers. Tis function olny handle one efi in this FFS.

  @param  SectionBuffer     The section base address
  @param  BufferLength      The length of FFS.
  @param  EfiBufferHeader   The structure dual pointer to the efi informations

  @retval  EFI_SUCCESS      The application exited normally.
  @retval  EFI_ABORTED       An error occurred.

**/
EFI_STATUS
ParseSection (
  IN      BOOLEAN              IsFfsOrEfi,
  IN OUT  UINT8                *SectionBuffer,
  IN      UINT32               BufferLength,
  IN OUT  EFI_SECTION_STRUCT   **EfiBufferHeader
  )
{
  UINT32              ParsedLength;
  EFI_SECTION_TYPE    Type;
  UINT8               *Ptr;
  UINT32              SectionLength;
  UINT8               *CompressedBuffer;
  UINT32              CompressedLength;
  UINT8               *UncompressedBuffer;
  UINT32              UncompressedLength;
  UINT8               CompressionType;
  DECOMPRESS_FUNCTION DecompressFunction;
  GETINFO_FUNCTION    GetInfoFunction;
  UINT32              ScratchSize;
  UINT8               *ScratchBuffer;
  EFI_STATUS          Status;
  UINT32              DstSize;
  CHAR8               *ExtractionTool;
  CHAR8               *ToolInputFile;
  CHAR8               *ToolOutputFile;
  CHAR8               *SystemCommandFormatString;
  CHAR8               *SystemCommand;
  UINT8               *ToolOutputBuffer;
  UINT32              ToolOutputLength;
  BOOLEAN             HasDepexSection;

  Ptr                       = NULL;
  SectionLength             = 0;
  CompressedBuffer          = NULL;
  CompressedLength          = 0;
  UncompressedBuffer        = NULL;
  UncompressedLength        = 0;
  CompressionType           = 0;
  ScratchSize               = 0;
  ScratchBuffer             = NULL;
  Status                    = EFI_SUCCESS;
  DstSize                   = 0;
  ExtractionTool            = NULL;
  ToolInputFile             = NULL;
  ToolOutputFile            = NULL;
  SystemCommandFormatString = NULL;
  SystemCommand             = NULL;

  //
  // Jump the FFS header
  //
  if (IsFfsOrEfi) {
    SectionBuffer = SectionBuffer + sizeof (EFI_FFS_FILE_HEADER);
    BufferLength  = BufferLength - sizeof (EFI_FFS_FILE_HEADER);
  }
  ParsedLength     = 0;
  HasDepexSection  = FALSE;
  ExtractionTool   = NULL;
  ToolOutputLength = 0;
  ToolOutputBuffer = NULL;

  (*EfiBufferHeader)->Length = BufferLength;

  while (ParsedLength < BufferLength) {
    Ptr           = SectionBuffer + ParsedLength;

    SectionLength = Get3ByteLength (((EFI_COMMON_SECTION_HEADER *) Ptr)->Size);
    Type          = ((EFI_COMMON_SECTION_HEADER *) Ptr)->Type;

    //
    // This is sort of an odd check, but is necessary because FFS files are
    // padded to a QWORD boundary, meaning there is potentially a whole section
    // header worth of 0xFF bytes.
    //
    if ((SectionLength == 0xffffff) && (Type == 0xff)) {
      ParsedLength += 4;
      continue;
    }

    switch (Type) {

    case EFI_SECTION_PE32:
    case EFI_SECTION_TE:
      //
      //Got the correct address
      //
     (*EfiBufferHeader)->BufferBase = (UINTN)(Ptr + sizeof (EFI_COMMON_SECTION_HEADER));
      return EFI_SUCCESS;

    case EFI_SECTION_RAW:
    case EFI_SECTION_PIC:
       break;

    case EFI_SECTION_USER_INTERFACE:
      HasDepexSection = FALSE;
      break;

    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
    case EFI_SECTION_COMPATIBILITY16:
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
      break;

    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_SMM_DEPEX:
      HasDepexSection = TRUE;
      break;

    case EFI_SECTION_VERSION:
      break;
    case EFI_SECTION_COMPRESSION:
      UncompressedBuffer  = NULL;
      CompressedLength    = SectionLength - sizeof (EFI_COMPRESSION_SECTION);
      UncompressedLength  = ((EFI_COMPRESSION_SECTION *) Ptr)->UncompressedLength;
      CompressionType     = ((EFI_COMPRESSION_SECTION *) Ptr)->CompressionType;

      if (CompressionType == EFI_NOT_COMPRESSED) {
        if (CompressedLength != UncompressedLength) {
          Error (
            NULL,
            0,
            0,
            "file is not compressed, but the compressed length does not match the uncompressed length",
            NULL
            );
          return EFI_ABORTED;
        }

        UncompressedBuffer = Ptr + sizeof (EFI_COMPRESSION_SECTION);
      } else if (CompressionType == EFI_STANDARD_COMPRESSION) {
        GetInfoFunction     = EfiGetInfo;
        DecompressFunction  = EfiDecompress;
        CompressedBuffer  = Ptr + sizeof (EFI_COMPRESSION_SECTION);

        Status = GetInfoFunction (
                   CompressedBuffer,
                   CompressedLength,
                   &DstSize,
                   &ScratchSize
                   );
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0003, "error getting compression info from compression section", NULL);
          return EFI_ABORTED;
        }

        if (DstSize != UncompressedLength) {
          Error (NULL, 0, 0003, "compression error in the compression section", NULL);
          return EFI_ABORTED;
        }

        ScratchBuffer       = malloc (ScratchSize);
        if (ScratchBuffer == NULL) {
          return EFI_ABORTED;
        }
        UncompressedBuffer  = malloc (UncompressedLength);
        if (UncompressedBuffer == NULL) {
          free (ScratchBuffer);
          return EFI_ABORTED;
        }
        memset (UncompressedBuffer, 0, UncompressedLength);

        Status = DecompressFunction (
                   CompressedBuffer,
                   CompressedLength,
                   UncompressedBuffer,
                   UncompressedLength,
                   ScratchBuffer,
                   ScratchSize
                  );
        free (ScratchBuffer);
        if (Status != EFI_SUCCESS) {
          Error (NULL, 0, 0003, "decompress failed", NULL);
          free (UncompressedBuffer);
          return EFI_ABORTED;
        }
      } else {
        Error (NULL, 0, 0003, "unrecognized compression type", "type 0x%X", CompressionType);
        return EFI_ABORTED;
      }

      Status = ParseSection (FALSE, UncompressedBuffer, UncompressedLength, EfiBufferHeader);
      if (Status != EFI_SUCCESS) {
        Error (NULL, 0, 0003, "failed to parse section", NULL);
        free (UncompressedBuffer);
        UncompressedBuffer = NULL;
      } else {
        return EFI_SUCCESS;
      }
      //
      // Store the allocate memory address for UncompressedBuffer
      //
      if (UncompressedBuffer != NULL) {
        (*EfiBufferHeader)->UncompressedBuffer[(*EfiBufferHeader)->UnCompressIndex] = (UINTN) UncompressedBuffer;
        (*EfiBufferHeader)->UnCompressIndex = (*EfiBufferHeader)->UnCompressIndex + 1;
      }
      break;

    case EFI_SECTION_GUID_DEFINED:
      //
      // Decompress failed, and then check for CRC32 sections which we can handle internally if needed.
      // Maybe this section is no-compressed.
      //
      if (!CompareGuid (
           &((EFI_GUID_DEFINED_SECTION *) Ptr)->SectionDefinitionGuid,
           &gEfiCrc32GuidedSectionExtractionProtocolGuid
           )) {
        //
        // CRC32 guided section
        //
        Status = ParseSection (
                   FALSE,
                   SectionBuffer + ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset,
                   BufferLength - ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset,
                   EfiBufferHeader
                  );
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0003, "parse of CRC32 GUIDED section failed", NULL);
          return EFI_ABORTED;
        } else {
          return EFI_SUCCESS;
        }
      } else {
        ExtractionTool = LookupGuidedSectionToolPath (
                           mParsedGuidedSectionTools,
                           &((EFI_GUID_DEFINED_SECTION *) Ptr)->SectionDefinitionGuid
                           );

        if (ExtractionTool != NULL) {
          ToolInputFile  = GenTempFile ();
          ToolOutputFile = GenTempFile ();
          //
          // Construction 'system' command string
          //
          SystemCommandFormatString = "%s -d -o \"%s\" \"%s\"";
          SystemCommand = malloc (
                            strlen (SystemCommandFormatString) \
                            + strlen (ExtractionTool)          \
                            + strlen (ToolInputFile)           \
                            + strlen (ToolOutputFile)          \
                            + 1
                            );
          if (SystemCommand == NULL) {
            free (ExtractionTool);
            free (ToolInputFile);
            free (ToolOutputFile);
            return EFI_ABORTED;
          }
          sprintf (
            SystemCommand,
            "%s -d -o \"%s\" \"%s\"",
            ExtractionTool,
            ToolOutputFile,
            ToolInputFile
            );
          free (ExtractionTool);

          Status = PutFileImage (
                     ToolInputFile,
                     (CHAR8*) Ptr + ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset,
                     SectionLength - ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset
                   );

          if (HasDepexSection) {
            HasDepexSection = FALSE;
          }

          system (SystemCommand);
          remove (ToolInputFile);
          free (ToolInputFile);
          ToolInputFile = NULL;
          free (SystemCommand);
          SystemCommand = NULL;

          if (EFI_ERROR (Status)) {
            Error ("FCE", 0, 0004, "unable to decoded GUIDED section", NULL);
            free (ToolOutputFile);
            return EFI_ABORTED;
          }

          Status = GetFileImage (
                     ToolOutputFile,
                     (CHAR8 **)&ToolOutputBuffer,
                     &ToolOutputLength
                    );
          remove (ToolOutputFile);
          free (ToolOutputFile);
          ToolOutputFile = NULL;
          if (EFI_ERROR (Status)) {
            return EFI_ABORTED;
          }
        }
        Status = ParseSection (
                  FALSE,
                  ToolOutputBuffer,
                  ToolOutputLength,
                  EfiBufferHeader
                  );
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0003, "parse of decoded GUIDED section failed", NULL);
          return EFI_ABORTED;
        }
      }
      break;

    default:
      ;
    }
    ParsedLength += SectionLength;
    //
    // We make then next section begin on a 4-byte boundary
    //
    ParsedLength = GetOccupiedSize (ParsedLength, 4);
  }

  return EFI_ABORTED;
}

static
BOOLEAN
GetNextOffset (
  IN UINT8 *Data,
  IN EFI_GUID *Guid,
  IN UINTN Len,
  IN OUT UINTN *Offset
  )
{
  UINTN NextOffset;
  if (*Offset >= Len || Len - *Offset <= sizeof (EFI_GUID)) {
    return FALSE;
  }

  for (NextOffset = *Offset; NextOffset < Len - sizeof (EFI_GUID); NextOffset++) {
    if (CompareGuid(Guid, (EFI_GUID*)(Data + NextOffset)) == 0) {
      *Offset = NextOffset + sizeof(EFI_GUID);
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Get the address by Guid.

  Parse the FFS image, and find the GUID address.There may be some Guids matching the
  searched Guid.

  @param Fv                     the Pointer to the image.
  @param Guid                   The Guid need to find.
  @param Offset                 The dual Pointer to the offset.
  @param NumOfMatchGuid         The number of matching Guid offset.

  @retval EFI_SUCCESS           The Search was complete successfully
  @return EFI_ABORTED           An error occurred
**/
EFI_STATUS
GetAddressByGuid (
  IN  VOID        *Fv,
  IN  EFI_GUID    *Guid,
  IN  UINTN       Len,
  OUT UINTN       **Offset,
  OUT UINT8       *NumOfMatchGuid
  )
{
  VOID        *LocalFv;
  UINT8       Flag;

  EFI_RAW_SECTION* Section;
  UINT8       *RawData;
  VOID*       SectionStart;
  UINTN       NextOffset;
  UINTN       Key;
  UINTN       TotalSectionsSize;
  UINTN       SecLen;
  UINTN       SecHdr;
  EFI_STATUS  Status;

  if( (Fv == NULL) || (Fv == NULL) || (Guid == NULL) || Len == 0 ){
    return EFI_ABORTED;
  }

  LocalFv         = Fv;
  Flag            = 0;
  Section         = NULL;
  Key             = 0;

  if (NumOfMatchGuid != NULL) {
    *NumOfMatchGuid = 0;
  }

  SectionStart = (VOID*)((UINTN)LocalFv + FvBufGetFfsHeaderSize(LocalFv));
  TotalSectionsSize = Len - FvBufGetFfsHeaderSize(LocalFv);
  while (TRUE) {
    Status = FvBufFindNextSection (
               SectionStart,
               TotalSectionsSize,
               &Key,
               (VOID **)&Section
               );
    if (Section == NULL || EFI_ERROR (Status)) {
      break;
    }

    if (EFI_SECTION_RAW == Section->Type) {
      if ((*(UINT32 *)Section->Size & 0xffffff) == 0xffffff) {
        SecLen = ((EFI_RAW_SECTION2 *)Section)->ExtendedSize;
        SecHdr = sizeof(EFI_RAW_SECTION2);
      } else {
        SecLen = *(UINT32 *)Section->Size & 0xffffff;
        SecHdr = sizeof(EFI_RAW_SECTION);
      }
      if (SecLen <= SecHdr || SecLen - SecHdr < sizeof(EFI_GUID)) {
        continue;
      }
      RawData = (UINT8 *)Section + SecHdr;
      NextOffset = 0;
      while (GetNextOffset(RawData, Guid, SecLen - SecHdr, &NextOffset)) {
        Flag = 1;
        if ((NumOfMatchGuid != NULL) && (Offset != NULL)) {
          if (*NumOfMatchGuid == 0) {
            *Offset = malloc (sizeof (UINTN) * MAX_MATCH_GUID_NUM);
            if (*Offset == NULL) {
              return EFI_ABORTED;
            }
            memset (*Offset, 0, sizeof (UINTN) * MAX_MATCH_GUID_NUM);
          }
          *(*Offset + *NumOfMatchGuid) = NextOffset + (RawData - (UINT8 *)Fv);
          (*NumOfMatchGuid)++;
        } else {
          return EFI_SUCCESS;
        }
      }
    }
  }

  if( Flag == 0 ) {
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}

/**
  Search the VfrBin Base address.

  According the known GUID gVfrArrayAttractGuid to get the base address from FFS.

  @param Fv                    the Pointer to the FFS
  @param EfiAddr               the Pointer to the EFI in FFS
  @param Length                the length of Fv
  @param Offset                the Pointer to the Addr (Offset)
  @param NumOfMachingOffset    the number of Addr (Offset)

  @retval EFI_SUCCESS          Get the address successfully.
**/
EFI_STATUS
SearchVfrBinInFFS (
   IN  VOID      *Fv,
   IN  VOID      *EfiAddr,
   IN  UINTN     Length,
   OUT UINTN    **Offset,
   OUT UINT8     *NumOfMachingOffset
  )
{
  UINTN        Index;
  EFI_STATUS   Status;
  UINTN        VirOffValue;

  Index       = 0;
  Status      = EFI_SUCCESS;
  VirOffValue = 0;

  if ((Fv == NULL) || (Offset == NULL)) {
    return EFI_ABORTED;
  }
  Status = GetAddressByGuid (
             Fv,
             &gVfrArrayAttractGuid,
             Length,
             Offset,
             NumOfMachingOffset
             );
  if (Status != EFI_SUCCESS) {
    return EFI_ABORTED;
  }

  while (Index < *NumOfMachingOffset) {
    //
    // Got the virOffset after the GUID
    //
    VirOffValue = *(UINTN *)((UINTN)Fv + *(*Offset + Index));
    //
    //Transfer the offset to the VA address. One modules may own more VfrBin address.
    //
    *(*Offset + Index) = (UINTN) EfiAddr + VirOffValue;
    Index++;
  }
  return EFI_SUCCESS;
}

/**
  Search the UniBin Base address.

  According the known GUID gUniStrArrayAttractGuid to get the base address from FFS.

  @param Fv                    the Pointer to the FFS
  @param EfiAddr               the Pointer to the EFI in FFS
  @param Length                the length of Fv
  @param Offset                the Pointer to the Addr (Offset)

  @retval Base address         Get the address successfully.
**/
EFI_STATUS
SearchUniBinInFFS (
   IN VOID      *Fv,
   IN  VOID     *EfiAddr,
   IN  UINTN    Length,
   OUT UINTN    **Offset
  )
{
  UINT8        NumOfMachingOffset;
  EFI_STATUS   Status;
  UINTN        VirOffValue;

  NumOfMachingOffset = 0;
  Status             = EFI_SUCCESS;
  VirOffValue        = 0;

  if ((Fv == NULL) || (Offset == NULL)) {
    return EFI_ABORTED;
  }
  Status = GetAddressByGuid (
             Fv,
             &gUniStrArrayAttractGuid,
             Length,
             Offset,
             &NumOfMachingOffset
             );
  if (Status != EFI_SUCCESS) {
    return EFI_ABORTED;
  }
  //
  //Transfer the offset to the VA address. There is only one UniArray in one modules.
  //
  if (NumOfMachingOffset == 1) {
    VirOffValue  = *(UINTN *)((UINTN)Fv + **Offset);
    **Offset     = (UINTN) EfiAddr + VirOffValue;
  } else {
    printf ("Error. Find more than 1 UniBin in FFS.\n");
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SearchNvStoreDatabaseInFd(
  IN VOID     *Fv,
  IN UINTN    length
  )
{
  EFI_STATUS   Status;
  UINTN        Offset;
  PCD_NV_STORE_DEFAULT_BUFFER_HEADER *NvStoreHeader;
  Status = EFI_SUCCESS;
  Offset = 0;
  if (Fv == NULL) {
    printf ("The FV is NULL.");
    return EFI_ABORTED;
  }
  while (Offset < (length - sizeof(PCD_NV_STORE_DEFAULT_BUFFER_HEADER))){
    NvStoreHeader = (PCD_NV_STORE_DEFAULT_BUFFER_HEADER *)((UINT8*)Fv + Offset);
    if (NvStoreHeader->Signature == PCD_NV_STORE_DEFAULT_BUFFER_SIGNATURE) {
      gEfiFdInfo.ExistNvStoreDatabase = TRUE;
      gEfiFdInfo.NvStoreDatabase = (UINT8 *) NvStoreHeader;
      break;
    }
    Offset++;
  }
  if (Offset == (length - sizeof(PCD_NV_STORE_DEFAULT_BUFFER_HEADER)) || gEfiFdInfo.ExistNvStoreDatabase != TRUE) {
    //printf ("Not found the PcdNvStoreDefaultValueBuffer\n");
    return Status;
  }
  return Status;
}

/**
  Get the address by Guid.

  Parse the FD image, and find the GUID address.There may be some Guids matching the
  searched Guid.

  @param Fv                     the Pointer to the image.
  @param Guid                   The Guid need to find.
  @param Offset                 The dual Pointer to the offset.
  @param NumOfMatchGuid         The number of matching Guid offset.

  @retval EFI_SUCCESS           The Search was complete successfully
  @return EFI_ABORTED           An error occurred
**/
EFI_STATUS
GetVariableAddressByGuid (
  IN  VOID        *Fv,
  IN  EFI_GUID    *Guid,
  IN  UINTN       Len,
  OUT UINTN       **Offset,
  OUT UINT8       *NumOfMatchGuid
  )
{
  UINTN       NextOffset;
  UINT8       Flag;

  if( (Fv == NULL) || (Fv == NULL) || (Guid == NULL) ){
    return EFI_ABORTED;
  }

  Flag            = 0;
  NextOffset      = 0;

  if (NumOfMatchGuid != NULL) {
    *NumOfMatchGuid = 0;
  }
  while (GetNextOffset(Fv, Guid, Len, &NextOffset)) {
    Flag = 1;
    if (NumOfMatchGuid != NULL && Offset != NULL) {
      if (*NumOfMatchGuid == 0) {
        *Offset = malloc (sizeof (UINTN) * MAX_MATCH_GUID_NUM);
        if (*Offset == NULL) {
          return EFI_ABORTED;
        }
        memset (*Offset, 0, sizeof (UINTN) * MAX_MATCH_GUID_NUM);
      }
      *(*Offset + *NumOfMatchGuid) = NextOffset;
      (*NumOfMatchGuid)++;
    } else {
      return EFI_SUCCESS;
    }
  }

  if( Flag == 0 ) {
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}

/**
  Search the EFI Variable Base address.

  According the known GUID gEfiSystemNvDataFvGuid to get the base address from FFS.

  @param Fv                    the Pointer to the FFS
  @param Length                the length of Fv
  @param Offset                the Pointer to the Addr (Offset)
  @param NumOfMachingOffset    the number of IFR array in one FFS

  @retval EFI_SUCCESS          Get the address successfully.
  @retval EFI_ABORTED          An error occured.
**/
EFI_STATUS
SearchEfiVarInFFS (
   IN VOID      *Fv,
   IN  UINTN     Length,
   OUT UINTN    **Offset,
   OUT UINT8    *NumOfMachingOffset
  )
{
  EFI_STATUS   Status;
  UINT8        Index;

  Status = EFI_SUCCESS;
  Index  = 0;

  if ((Fv == NULL) || (Offset == NULL)) {
    printf ("The FV or offset is NULL.");
    return EFI_ABORTED;
  }
  Status = GetVariableAddressByGuid (
             Fv,
             &gEfiSystemNvDataFvGuid,
             Length,
             Offset,
             NumOfMachingOffset
             );
  if (Status != EFI_SUCCESS) {
    return EFI_ABORTED;
  }
  //
  //Transfer the offset to the VA address.
  //
  while (Index < *NumOfMachingOffset) {
    *(*Offset + Index) = (UINTN) Fv + *(*Offset + Index);
    Index++;
  }
  return EFI_SUCCESS;
}

/**
  Parse the Ffs header to get the size.

  @param  InputFile             The pointer to the input file
  @param  FvSize                The pointer to the file size

  @return EFI_SUCCESS           Get the file size successfully
**/
EFI_STATUS
ReadFfsHeader (
  IN   FILE       *InputFile,
  OUT  UINT32     *FvSize
  )
{
  EFI_FFS_FILE_HEADER         FfsHeader;
  EFI_FV_FILETYPE             Type;

  //
  // Check input parameters
  //
  if ((InputFile == NULL) || (FvSize == NULL)) {
    return EFI_ABORTED;
  }
  //
  // Read the header
  //
  fread (
    &FfsHeader,
    sizeof (EFI_FFS_FILE_HEADER),
    1,
    InputFile
    );
  Type    = FfsHeader.Type;

  if (Type == EFI_FV_FILETYPE_DRIVER) {
    *FvSize = *(UINT32 *)FfsHeader.Size & 0xffffff;
  } else if (Type == EFI_FV_FILETYPE_APPLICATION) {
    *FvSize = *(UINT32 *)FfsHeader.Size & 0xffffff;
  } else if (Type == EFI_FV_FILETYPE_FREEFORM) {
    *FvSize = *(UINT32 *)FfsHeader.Size & 0xffffff;
  } else {
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}

/*
  Read the length of the whole FD

  This function determines the size of the FV.

  @param   InputFile        The file that contains the FV image.
  @param   FvSize           The size of the FV.

  @retval  EFI_SUCCESS      The application exited normally.
  @retval  EFI_ABORTED      An error occurred.

**/
static
EFI_STATUS
ReadFdHeader (
  IN FILE       *InputFile,
  OUT UINT32    *FvSize
  )
{
  //
  // Check input parameters
  //
  if ((InputFile == NULL) || (FvSize == NULL)) {
    return EFI_ABORTED;
  }
  *FvSize = 0;
  //
  // Get the total size of FD file (Fixed the length)
  //
  fseek(InputFile,0,SEEK_END);
  *FvSize = ftell(InputFile);
  fseek(InputFile,0,SEEK_SET);

  if (*FvSize == 0) {
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}

/**
  Read the file to memory.

  @param   InputFile        The file that contains the FV image.
  @param   Size             The size of the file.

  @retval The pointer to the begining position of memory.
**/
VOID *
ReadFileToMemory (
  IN CHAR8      *FileName,
  OUT UINT32    *Size
  )
{
  FILE                          *InFile;
  VOID                          *Address;
  UINT32                        BytesRead;
  EFI_STATUS                    Status;

  InFile          = NULL;
  Address         = NULL;
  BytesRead       = 0;
  Status          = EFI_SUCCESS;

  InFile = fopen (FileName,"rb");
  if (InFile == NULL) {
    return NULL;
  }
  //
  // Determine the size of FV
  //
  Status = ReadFdHeader (InFile, Size);
  if (Status != EFI_SUCCESS) {
    fclose (InFile);
    return NULL;
  }
  //
  // Allocate a buffer for the FV image
  //
  Address = malloc (*Size);
  if (Address == NULL) {
    fclose (InFile);
    return NULL;
  }
  memset (Address, 0, *Size);
  //
  // Seek to the start of the image, then read the entire FV to the buffer
  //
  fseek (InFile, 0, SEEK_SET);
  BytesRead = fread (Address, 1, *Size, InFile);
  fclose (InFile);
  if ((UINTN) BytesRead != *Size) {
    free (Address);
    return NULL;
  }
  return Address;
}

/**
  Search the EFI variables address in Fd.

  Open and read the *.fd to the memory, initialize the global structure.
  Update the EFI variables addr and the begining position of memory.

  @retval EFI_SUCCESS          Get the address successfully.
**/
EFI_STATUS
GetEfiVariablesAddr (
  BOOLEAN UqiIsSet
  )
{
  VOID                          *FdImage;
  UINT32                        FdSize;
  EFI_STATUS                    Status;
  UINTN                         *EfiVarAddr;
  UINT8                         NumOfMachingVar;
  UINT32                        Index;
  BOOLEAN                       GotFlag;
  EFI_FIRMWARE_VOLUME_HEADER    *Variable;
  BOOLEAN                       AuthencitatedMonotonicOrNot;
  BOOLEAN                       AuthencitatedBasedTimeOrNot;
  BOOLEAN                       NormalOrNot;

  FdImage         = NULL;
  FdSize          = 0;
  Status          = EFI_SUCCESS;
  EfiVarAddr      = NULL;
  NumOfMachingVar = 0;
  Index           = 0;
  GotFlag         = TRUE;
  Variable        = NULL;

  FdImage = ReadFileToMemory (mInputFdName, &FdSize);
  if (FdImage == NULL) {
    return EFI_ABORTED;
  }
  if (!UqiIsSet) {
    Status = SearchNvStoreDatabaseInFd(FdImage, FdSize);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  Status = SearchEfiVarInFFS (
             FdImage,
             FdSize,
             &EfiVarAddr,
             &NumOfMachingVar
             );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Check the signature "_FVH"
  //
  Index       = 0;
  GotFlag     = FALSE;

  while (Index < NumOfMachingVar) {
    Variable = (EFI_FIRMWARE_VOLUME_HEADER *)(*(EfiVarAddr + Index) - 0x20);
    if (Variable->Signature == 0x4856465F) {
      AuthencitatedMonotonicOrNot  = CheckMonotonicBasedVarStore ((UINT8 *)Variable + Variable->HeaderLength);
      AuthencitatedBasedTimeOrNot  = CheckTimeBasedVarStoreOrNot ((UINT8 *)Variable + Variable->HeaderLength);
      NormalOrNot                  = CheckNormalVarStoreOrNot ((UINT8 *)Variable + Variable->HeaderLength);
      if (AuthencitatedMonotonicOrNot || AuthencitatedBasedTimeOrNot || NormalOrNot) {
        GotFlag = TRUE;
        gEfiFdInfo.EfiVariableAddr = (UINTN)Variable;
        break;
      }
    }
    Index++;
  }
  free (EfiVarAddr);
  if (!GotFlag) {
    return EFI_ABORTED;
  }
  gEfiFdInfo.Fd              = FdImage;
  gEfiFdInfo.FdSize          = FdSize;

  return EFI_SUCCESS;
}

/**
  Pick up the FFS which includes IFR section.

  Parse all FFS extracted by BfmLib, and save all which includes IFR
  Binary to gEfiFdInfo structure.

  @retval EFI_SUCCESS          Get the address successfully.
  @retval EFI_BUFFER_TOO_SMALL Memory can't be allocated.
  @retval EFI_ABORTED          Read FFS Failed.
**/
EFI_STATUS
FindFileInFolder (
  IN   CHAR8    *FolderName,
  OUT  BOOLEAN  *ExistStorageInBfv,
  OUT  BOOLEAN  *SizeOptimized
)
{
  CHAR8           *FileName;
  CHAR8           *CurFolderName;
  EFI_STATUS      Status;
  UINTN           MaxFileNameLen;
  UINTN           Index;
  CHAR8           FileNameArry[300];
  FILE            *FfsFile;
  UINTN           FileSize;
  VOID            *FfsImage;
  UINTN           BytesRead;
#ifndef __GNUC__
  HANDLE          FindHandle;
  WIN32_FIND_DATA FindFileData;
#else
  struct dirent *pDirent;
  DIR *pDir;
#endif

  FileName       = NULL;
  CurFolderName  = NULL;
  Status         = EFI_SUCCESS;
  MaxFileNameLen = 0;
  Index          = 0;
  FileSize       = 0;
  BytesRead      = 0;
  FfsImage       = NULL;
  FfsFile        = NULL;

  MaxFileNameLen = strlen (FolderName) + MAX_FILENAME_LEN;
  CurFolderName  = (CHAR8 *)calloc(
                     strlen (FolderName) + strlen (OS_SEP_STR) + strlen ("*.*")+ 1,
                     sizeof(CHAR8)
                     );
  if (CurFolderName == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }
  strcpy (CurFolderName, FolderName);
  strcat (CurFolderName, OS_SEP_STR);
  strcat (CurFolderName, "*.*");
  FileName = (CHAR8 *)calloc(
               MaxFileNameLen,
               sizeof(CHAR8)
               );
  if (FileName == NULL) {
    free (CurFolderName);
    return EFI_BUFFER_TOO_SMALL;
  }

#ifndef __GNUC__
  if((FindHandle = FindFirstFile(CurFolderName, &FindFileData)) != INVALID_HANDLE_VALUE){
    do {
      memset (FileName, 0, MaxFileNameLen);
      if ((strcmp (FindFileData.cFileName, ".") == 0)
      || (strcmp (FindFileData.cFileName, "..") == 0)
      ) {
        continue;
      }
      if (strlen(FolderName) + strlen ("\\") + strlen (FindFileData.cFileName) > 300 - 1) {
        Status = EFI_ABORTED;
        goto Done;
      }
      snprintf (FileNameArry, 300, "%s%c%s", FolderName, OS_SEP, FindFileData.cFileName);
      FfsFile = fopen (FileNameArry, "rb");
      if (FfsFile == NULL) {
        Status = EFI_ABORTED;
        goto Done;
      }
      Status = ReadFfsHeader (FfsFile, (UINT32 *)&FileSize);
      if (EFI_ERROR (Status)) {
        fclose (FfsFile);
        Status  = EFI_SUCCESS;
        continue;
      }
      //
      // Allocate a buffer for the FFS file
      //
      FfsImage = malloc (FileSize);
      if (FfsImage == NULL) {
        fclose (FfsFile);
        Status = EFI_BUFFER_TOO_SMALL;
        goto Done;
      }
      //
      // Seek to the start of the image, then read the entire FV to the buffer
      //
      fseek (FfsFile, 0, SEEK_SET);
      BytesRead = fread (FfsImage, 1, FileSize, FfsFile);
      fclose (FfsFile);

      if ((UINTN) BytesRead != FileSize) {
        free (FfsImage);
        Status = EFI_ABORTED;
        goto Done;
      }
      //
      // Check whether exists the storage ffs in BFV for multi-platform mode
      //
      if (CompareGuid(&gEfiFfsBfvForMultiPlatformGuid,(EFI_GUID *) FfsImage) == 0) {
        *ExistStorageInBfv            = TRUE;
      *SizeOptimized                = FALSE;
        gEfiFdInfo.StorageFfsInBfv    = FfsImage;
        continue;
      }
    //
      // Check whether exists the optimized storage ffs in BFV for multi-platform mode
      //
      if (CompareGuid(&gEfiFfsBfvForMultiPlatformGuid2,(EFI_GUID *) FfsImage) == 0) {
        *ExistStorageInBfv            = TRUE;
    *SizeOptimized                = TRUE;
        gEfiFdInfo.StorageFfsInBfv    = FfsImage;
        continue;
      }
      //
      // Check whether current FFS includes IFR
      //
      Status = GetAddressByGuid (
                 FfsImage,
                 &gVfrArrayAttractGuid,
                 FileSize,
                 NULL,
                 NULL
                );
      if (EFI_ERROR (Status)) {
        free (FfsImage);
        Status  = EFI_SUCCESS;
      } else {
       //
       // Check whether existed same IFR binary. If existed, not insert the new one.
       //
       if (NotExistSameFfsIfr (FfsImage)) {
         gEfiFdInfo.FfsArray[Index] = FfsImage;
         gEfiFdInfo.Length[Index++] = FileSize;
       }
      }

    } while (FindNextFile (FindHandle, &FindFileData));
    FindClose(FindHandle);
  } else {
    Status = EFI_ABORTED;
    goto Done;
  }

Done:
  free (CurFolderName);
  free (FileName);

#else
  if((pDir = opendir(FolderName)) != NULL){
    while ((pDirent = readdir(pDir)) != NULL){
      memset (FileName, 0, MaxFileNameLen);
      if ((strcmp (pDirent->d_name, ".") == 0)
      || (strcmp (pDirent->d_name, "..") == 0)
      ) {
        continue;
      }
      if (strlen(FolderName) + 1 + strlen (pDirent->d_name) > 300 - 1) {
        Status = EFI_ABORTED;
        goto Done;
      }
      snprintf (FileNameArry, 300, "%s%c%s", FolderName, OS_SEP, pDirent->d_name);
      FfsFile = fopen (FileNameArry, "rb");
      Status = ReadFfsHeader (FfsFile, (UINT32 *)&FileSize);
      if (EFI_ERROR (Status)) {
        fclose (FfsFile);
        Status  = EFI_SUCCESS;
        continue;
      }
      //
      // Allocate a buffer for the FFS file
      //
      FfsImage = malloc (FileSize);
      if (FfsImage == NULL) {
        fclose (FfsFile);
        Status = EFI_BUFFER_TOO_SMALL;
        goto Done;
      }
      //
      // Seek to the start of the image, then read the entire FV to the buffer
      //
      fseek (FfsFile, 0, SEEK_SET);
      BytesRead = fread (FfsImage, 1, FileSize, FfsFile);
      fclose (FfsFile);

      if ((UINTN) BytesRead != FileSize) {
        free (FfsImage);
        Status = EFI_ABORTED;
        goto Done;
      }
      //
      // Check whether exists the storage ffs in BFV for multi-platform mode
      //
      if (CompareGuid(&gEfiFfsBfvForMultiPlatformGuid,(EFI_GUID *) FfsImage) == 0) {
        *ExistStorageInBfv            = TRUE;
        *SizeOptimized                = FALSE;
        gEfiFdInfo.StorageFfsInBfv    = FfsImage;
        continue;
      }
      //
      // Check whether exists the optimized storage ffs in BFV for multi-platform mode
      //
      if (CompareGuid(&gEfiFfsBfvForMultiPlatformGuid2,(EFI_GUID *) FfsImage) == 0) {
        *ExistStorageInBfv            = TRUE;
        *SizeOptimized                = TRUE;
        gEfiFdInfo.StorageFfsInBfv    = FfsImage;
        continue;
      }
      //
      // Check whether current FFS includes IFR
      //
      Status = GetAddressByGuid (
                 FfsImage,
                 &gVfrArrayAttractGuid,
                 FileSize,
                 NULL,
                 NULL
                );
      if (EFI_ERROR (Status)) {
        free (FfsImage);
        Status  = EFI_SUCCESS;
      } else {
       //
       // Check whether existed same IFR binary. If existed, not insert the new one.
       //
       if (NotExistSameFfsIfr (FfsImage)) {
         gEfiFdInfo.FfsArray[Index] = FfsImage;
         gEfiFdInfo.Length[Index++] = FileSize;
       }
      }

    }
    closedir(pDir);
  } else {
    Status = EFI_ABORTED;
    goto Done;
  }

Done:
  free (CurFolderName);
  free (FileName);
#endif
  return Status;
}

