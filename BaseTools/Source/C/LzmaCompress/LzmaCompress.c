/** @file
  LZMA Compress/Decompress tool (LzmaCompress)

  Based on LZMA SDK 19.00:
    LzmaUtil.c -- Test application for LZMA compression
    2019-02-21 : Igor Pavlov : Public domain

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Sdk/C/Alloc.h"
#include "Sdk/C/7zFile.h"
#include "Sdk/C/7zVersion.h"
#include "Sdk/C/LzmaDec.h"
#include "Sdk/C/LzmaEnc.h"
#include "Sdk/C/Bra.h"
#include "CommonLib.h"
#include "ParseInf.h"

#define LZMA_HEADER_SIZE (LZMA_PROPS_SIZE + 8)

typedef enum {
  NoConverter,
  X86Converter,
  MaxConverter
} CONVERTER_TYPE;

const char *kCantReadMessage = "Can not read input file";
const char *kCantWriteMessage = "Can not write output file";
const char *kCantAllocateMessage = "Can not allocate memory";
const char *kDataErrorMessage = "Data error";
const char *kInvalidParamValMessage = "Invalid parameter value";

static BoolInt mQuietMode = False;
static CONVERTER_TYPE mConType = NoConverter;

UINT64 mDictionarySize = 28;
UINT64 mCompressionMode = 2;

#define UTILITY_NAME "LzmaCompress"
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 2
#define INTEL_COPYRIGHT \
  "Copyright (c) 2009-2018, Intel Corporation. All rights reserved."
void PrintHelp(char *buffer)
{
  strcat(buffer,
      "\n" UTILITY_NAME " - " INTEL_COPYRIGHT "\n"
      "Based on LZMA Utility " MY_VERSION_COPYRIGHT_DATE "\n"
      "\nUsage:  LzmaCompress -e|-d [options] <inputFile>\n"
             "  -e: encode file\n"
             "  -d: decode file\n"
             "  -o FileName, --output FileName: specify the output filename\n"
             "  --f86: enable converter for x86 code\n"
             "  -v, --verbose: increase output messages\n"
             "  -q, --quiet: reduce output messages\n"
             "  --debug [0-9]: set debug level\n"
             "  -a: set compression mode 0 = fast, 1 = normal, default: 1 (normal)\n"
             "  d: sets Dictionary size - [0, 27], default: 24 (16MB)\n"
             "  --version: display the program version and exit\n"
             "  -h, --help: display this help text\n"
             );
}

int PrintError(char *buffer, const char *message)
{
  strcat(buffer, "\nError: ");
  strcat(buffer, message);
  strcat(buffer, "\n");
  return 1;
}

int PrintErrorNumber(char *buffer, SRes val)
{
  sprintf(buffer + strlen(buffer), "\nError code: %x\n", (unsigned)val);
  return 1;
}

int PrintUserError(char *buffer)
{
  return PrintError(buffer, "Incorrect command");
}

void PrintVersion(char *buffer)
{
  sprintf (buffer, "%s Version %d.%d %s ", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
}

static SRes Encode(ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 fileSize, CLzmaEncProps *props)
{
  SRes res;
  size_t inSize = (size_t)fileSize;
  Byte *inBuffer = 0;
  Byte *outBuffer = 0;
  Byte *filteredStream = 0;
  size_t outSize;

  if (inSize != 0) {
    inBuffer = (Byte *)MyAlloc(inSize);
    if (inBuffer == 0)
      return SZ_ERROR_MEM;
  } else {
    return SZ_ERROR_INPUT_EOF;
  }

  if (SeqInStream_Read(inStream, inBuffer, inSize) != SZ_OK) {
    res = SZ_ERROR_READ;
    goto Done;
  }

  // we allocate 105% of original size + 64KB for output buffer
  outSize = (size_t)fileSize / 20 * 21 + (1 << 16);
  outBuffer = (Byte *)MyAlloc(outSize);
  if (outBuffer == 0) {
    res = SZ_ERROR_MEM;
    goto Done;
  }

  {
    int i;
    for (i = 0; i < 8; i++)
      outBuffer[i + LZMA_PROPS_SIZE] = (Byte)(fileSize >> (8 * i));
  }

  if (mConType != NoConverter)
  {
    filteredStream = (Byte *)MyAlloc(inSize);
    if (filteredStream == 0) {
      res = SZ_ERROR_MEM;
      goto Done;
    }
    memcpy(filteredStream, inBuffer, inSize);

    if (mConType == X86Converter) {
      {
        UInt32 x86State;
        x86_Convert_Init(x86State);
        x86_Convert(filteredStream, (SizeT) inSize, 0, &x86State, 1);
      }
    }
  }

  {
    size_t outSizeProcessed = outSize - LZMA_HEADER_SIZE;
    size_t outPropsSize = LZMA_PROPS_SIZE;

    res = LzmaEncode(outBuffer + LZMA_HEADER_SIZE, &outSizeProcessed,
        mConType != NoConverter ? filteredStream : inBuffer, inSize,
        props, outBuffer, &outPropsSize, 0,
        NULL, &g_Alloc, &g_Alloc);

    if (res != SZ_OK)
      goto Done;

    outSize = LZMA_HEADER_SIZE + outSizeProcessed;
  }

  if (outStream->Write(outStream, outBuffer, outSize) != outSize)
    res = SZ_ERROR_WRITE;

Done:
  MyFree(outBuffer);
  MyFree(inBuffer);
  MyFree(filteredStream);

  return res;
}

static SRes Decode(ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 fileSize)
{
  SRes res;
  size_t inSize = (size_t)fileSize;
  Byte *inBuffer = 0;
  Byte *outBuffer = 0;
  size_t outSize = 0;
  size_t inSizePure;
  ELzmaStatus status;
  UInt64 outSize64 = 0;

  int i;

  if (inSize < LZMA_HEADER_SIZE)
    return SZ_ERROR_INPUT_EOF;

  inBuffer = (Byte *)MyAlloc(inSize);
  if (inBuffer == 0)
    return SZ_ERROR_MEM;

  if (SeqInStream_Read(inStream, inBuffer, inSize) != SZ_OK) {
    res = SZ_ERROR_READ;
    goto Done;
  }

  for (i = 0; i < 8; i++)
    outSize64 += ((UInt64)inBuffer[LZMA_PROPS_SIZE + i]) << (i * 8);

  outSize = (size_t)outSize64;
  if (outSize != 0) {
    outBuffer = (Byte *)MyAlloc(outSize);
    if (outBuffer == 0) {
      res = SZ_ERROR_MEM;
      goto Done;
    }
  } else {
    res = SZ_OK;
    goto Done;
  }

  inSizePure = inSize - LZMA_HEADER_SIZE;
  res = LzmaDecode(outBuffer, &outSize, inBuffer + LZMA_HEADER_SIZE, &inSizePure,
      inBuffer, LZMA_PROPS_SIZE, LZMA_FINISH_END, &status, &g_Alloc);

  if (res != SZ_OK)
    goto Done;

  if (mConType == X86Converter)
  {
    UInt32 x86State;
    x86_Convert_Init(x86State);
    x86_Convert(outBuffer, (SizeT) outSize, 0, &x86State, 0);
  }

  if (outStream->Write(outStream, outBuffer, outSize) != outSize)
    res = SZ_ERROR_WRITE;

Done:
  MyFree(outBuffer);
  MyFree(inBuffer);

  return res;
}

int main2(int numArgs, const char *args[], char *rs)
{
  CFileSeqInStream inStream;
  CFileOutStream outStream;
  int res;
  int encodeMode = 0;
  BoolInt modeWasSet = False;
  const char *inputFile = NULL;
  const char *outputFile = "file.tmp";
  int param;
  UInt64 fileSize;
  CLzmaEncProps props;

  LzmaEncProps_Init(&props);
  LzmaEncProps_Normalize(&props);

  FileSeqInStream_CreateVTable(&inStream);
  File_Construct(&inStream.file);

  FileOutStream_CreateVTable(&outStream);
  File_Construct(&outStream.file);

  if (numArgs == 1)
  {
    PrintHelp(rs);
    return 0;
  }

  for (param = 1; param < numArgs; param++) {
    if (strcmp(args[param], "-e") == 0 || strcmp(args[param], "-d") == 0) {
      encodeMode = (args[param][1] == 'e');
      modeWasSet = True;
    } else if (strcmp(args[param], "--f86") == 0) {
      mConType = X86Converter;
    } else if (strcmp(args[param], "-o") == 0 ||
               strcmp(args[param], "--output") == 0) {
      if (numArgs < (param + 2)) {
        return PrintUserError(rs);
      }
      outputFile = args[++param];
    } else if (strcmp(args[param], "--debug") == 0) {
      if (numArgs < (param + 2)) {
        return PrintUserError(rs);
      }
      //
      // For now we silently ignore this parameter to achieve command line
      // parameter compatibility with other build tools.
      //
      param++;
    } else if (strcmp(args[param], "-a") == 0) {
      AsciiStringToUint64(args[param + 1],FALSE,&mCompressionMode);
      if ((mCompressionMode == 0)||(mCompressionMode == 1)){
        props.algo = (int)mCompressionMode;
        param++;
        continue;
      } else {
        return PrintError(rs, kInvalidParamValMessage);
      }
    } else if (strcmp(args[param], "d") == 0) {
      AsciiStringToUint64(args[param + 1],FALSE,&mDictionarySize);
      if (mDictionarySize <= 27) {
        if (mDictionarySize == 0) {
          props.dictSize = 0;
        } else {
          props.dictSize = (1 << mDictionarySize);
        }
        param++;
        continue;
      } else {
        return PrintError(rs, kInvalidParamValMessage);
      }
    } else if (
                strcmp(args[param], "-h") == 0 ||
                strcmp(args[param], "--help") == 0
              ) {
      PrintHelp(rs);
      return 0;
    } else if (
                strcmp(args[param], "-v") == 0 ||
                strcmp(args[param], "--verbose") == 0
              ) {
      //
      // For now we silently ignore this parameter to achieve command line
      // parameter compatibility with other build tools.
      //
    } else if (
                strcmp(args[param], "-q") == 0 ||
                strcmp(args[param], "--quiet") == 0
              ) {
      mQuietMode = True;
    } else if (strcmp(args[param], "--version") == 0) {
      PrintVersion(rs);
      return 0;
    } else if (inputFile == NULL) {
      inputFile = args[param];
    } else {
      return PrintUserError(rs);
    }
  }

  if ((inputFile == NULL) || !modeWasSet) {
    return PrintUserError(rs);
  }

  {
    size_t t4 = sizeof(UInt32);
    size_t t8 = sizeof(UInt64);
    if (t4 != 4 || t8 != 8)
      return PrintError(rs, "Incorrect UInt32 or UInt64");
  }

  if (InFile_Open(&inStream.file, inputFile) != 0)
    return PrintError(rs, "Can not open input file");

  if (OutFile_Open(&outStream.file, outputFile) != 0) {
    File_Close(&inStream.file);
    return PrintError(rs, "Can not open output file");
  }

  File_GetLength(&inStream.file, &fileSize);

  if (encodeMode)
  {
    if (!mQuietMode) {
      printf("Encoding\n");
    }
    res = Encode(&outStream.vt, &inStream.vt, fileSize, &props);
  }
  else
  {
    if (!mQuietMode) {
      printf("Decoding\n");
    }
    res = Decode(&outStream.vt, &inStream.vt, fileSize);
  }

  File_Close(&outStream.file);
  File_Close(&inStream.file);

  if (res != SZ_OK)
  {
    if (res == SZ_ERROR_MEM)
      return PrintError(rs, kCantAllocateMessage);
    else if (res == SZ_ERROR_DATA)
      return PrintError(rs, kDataErrorMessage);
    else if (res == SZ_ERROR_WRITE)
      return PrintError(rs, kCantWriteMessage);
    else if (res == SZ_ERROR_READ)
      return PrintError(rs, kCantReadMessage);
    return PrintErrorNumber(rs, res);
  }
  return 0;
}

int MY_CDECL main(int numArgs, const char *args[])
{
  char rs[2000] = { 0 };
  int res = main2(numArgs, args, rs);
  if (strlen(rs) > 0) {
    puts(rs);
  }
  return res;
}
