/** @file
  LZMA Compress/Decompress tool (LzmaCompress)

  Based on LZMA SDK 4.65:
    LzmaUtil.c -- Test application for LZMA compression
    2008-11-23 : Igor Pavlov : Public domain

  Copyright (c) 2006 - 2009, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

const char *kCantReadMessage = "Can not read input file";
const char *kCantWriteMessage = "Can not write output file";
const char *kCantAllocateMessage = "Can not allocate memory";
const char *kDataErrorMessage = "Data error";

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static Bool mQuietMode = False;

#define UTILITY_NAME "LzmaCompress"
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1
#define INTEL_COPYRIGHT \
  "Copyright (c) 2009, Intel Corporation. All rights reserved."
void PrintHelp(char *buffer)
{
  strcat(buffer,
      "\n" UTILITY_NAME " - " INTEL_COPYRIGHT "\n"
      "Based on LZMA Utility " MY_VERSION_COPYRIGHT_DATE "\n"
      "\nUsage:  LzmaCompress -e|-d [options] <inputFile>\n"
             "  -e: encode file\n"
             "  -d: decode file\n"
             "  -o FileName, --output FileName: specify the output filename\n"
             "  -v, --verbose: increase output messages\n"
             "  -q, --quiet: reduce output messages\n"
             "  --debug [0-9]: set debug level\n"
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
  sprintf (buffer, "%s Version %d.%d", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
}

#define IN_BUF_SIZE (1 << 16)
#define OUT_BUF_SIZE (1 << 16)

static SRes Decode2(CLzmaDec *state, ISeqOutStream *outStream, ISeqInStream *inStream,
    UInt64 unpackSize)
{
  int thereIsSize = (unpackSize != (UInt64)(Int64)-1);
  Byte inBuf[IN_BUF_SIZE];
  Byte outBuf[OUT_BUF_SIZE];
  size_t inPos = 0, inSize = 0, outPos = 0;
  LzmaDec_Init(state);
  for (;;)
  {
    if (inPos == inSize)
    {
      inSize = IN_BUF_SIZE;
      RINOK(inStream->Read(inStream, inBuf, &inSize));
      inPos = 0;
    }
    {
      SRes res;
      SizeT inProcessed = inSize - inPos;
      SizeT outProcessed = OUT_BUF_SIZE - outPos;
      ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
      ELzmaStatus status;
      if (thereIsSize && outProcessed > unpackSize)
      {
        outProcessed = (SizeT)unpackSize;
        finishMode = LZMA_FINISH_END;
      }

      res = LzmaDec_DecodeToBuf(state, outBuf + outPos, &outProcessed,
        inBuf + inPos, &inProcessed, finishMode, &status);
      inPos += inProcessed;
      outPos += outProcessed;
      unpackSize -= outProcessed;

      if (outStream)
        if (outStream->Write(outStream, outBuf, outPos) != outPos)
          return SZ_ERROR_WRITE;

      outPos = 0;

      if (res != SZ_OK || (thereIsSize && unpackSize == 0))
        return res;

      if (inProcessed == 0 && outProcessed == 0)
      {
        if (thereIsSize || status != LZMA_STATUS_FINISHED_WITH_MARK)
          return SZ_ERROR_DATA;
        return res;
      }
    }
  }
}

static SRes Decode(ISeqOutStream *outStream, ISeqInStream *inStream)
{
  UInt64 unpackSize;
  int i;
  SRes res = 0;

  CLzmaDec state;

  /* header: 5 bytes of LZMA properties and 8 bytes of uncompressed size */
  unsigned char header[LZMA_PROPS_SIZE + 8];

  /* Read and parse header */

  RINOK(SeqInStream_Read(inStream, header, sizeof(header)));

  unpackSize = 0;
  for (i = 0; i < 8; i++)
    unpackSize += (UInt64)header[LZMA_PROPS_SIZE + i] << (i * 8);

  LzmaDec_Construct(&state);
  RINOK(LzmaDec_Allocate(&state, header, LZMA_PROPS_SIZE, &g_Alloc));
  res = Decode2(&state, outStream, inStream, unpackSize);
  LzmaDec_Free(&state, &g_Alloc);
  return res;
}

static SRes Encode(ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 fileSize, char *rs)
{
  CLzmaEncHandle enc;
  SRes res;
  CLzmaEncProps props;

  rs = rs;

  enc = LzmaEnc_Create(&g_Alloc);
  if (enc == 0)
    return SZ_ERROR_MEM;

  LzmaEncProps_Init(&props);
  res = LzmaEnc_SetProps(enc, &props);

  if (res == SZ_OK)
  {
    Byte header[LZMA_PROPS_SIZE + 8];
    size_t headerSize = LZMA_PROPS_SIZE;
    int i;

    res = LzmaEnc_WriteProperties(enc, header, &headerSize);
    for (i = 0; i < 8; i++)
      header[headerSize++] = (Byte)(fileSize >> (8 * i));
    if (outStream->Write(outStream, header, headerSize) != headerSize)
      res = SZ_ERROR_WRITE;
    else
    {
      if (res == SZ_OK)
        res = LzmaEnc_Encode(enc, outStream, inStream, NULL, &g_Alloc, &g_Alloc);
    }
  }
  LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
  return res;
}

int main2(int numArgs, const char *args[], char *rs)
{
  CFileSeqInStream inStream;
  CFileOutStream outStream;
  int res;
  int encodeMode = 0;
  Bool modeWasSet = False;
  const char *inputFile = NULL;
  const char *outputFile = "file.tmp";
  int param;

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

  if (OutFile_Open(&outStream.file, outputFile) != 0)
    return PrintError(rs, "Can not open output file");

  if (encodeMode)
  {
    UInt64 fileSize;
    File_GetLength(&inStream.file, &fileSize);
    if (!mQuietMode) {
      printf("Encoding\n");
    }
    res = Encode(&outStream.s, &inStream.s, fileSize, rs);
  }
  else
  {
    if (!mQuietMode) {
      printf("Decoding\n");
    }
    res = Decode(&outStream.s, &inStream.s);
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
