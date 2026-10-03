/**
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-3-Clause-Clear
  @file
  BaseTools/Source/C/Lz4Compress/Lz4Compress.c
  Build as a BaseTools helper (similar to LzmaCompress/BrotliCompress).
  Contract: Lz4Compress -e -o <out> <in>
  Encode only; decompression at boot time is handled by
  MdeModulePkg/Library/Lz4CustomDecompressLib, not by this host tool.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "lz4/lib/lz4.h"
#include "lz4/lib/lz4hc.h"

//
// Layout matches LZ4_DECOMPRESS_HEADER in
// MdeModulePkg/Library/Lz4CustomDecompressLib/Lz4Format.h:
//   [UINT32 OriginalSize][UINT32 CompressedSize][LZ4 block payload]
//
#define LZ4_HEADER_SIZE  8
#define LZ4_HEADER_ORIGINALSIZE_OFFSET    0
#define LZ4_HEADER_COMPRESSEDSIZE_OFFSET  4
#define LZ4_BYTE_ELEMENT_SIZE  1

static const char *kCantOpenInputMessage  = "Can not open input file";
static const char *kCantOpenOutputMessage = "Can not open output file";
static const char *kCantAllocateMessage   = "Can not allocate memory";
static const char *kCompressFailedMessage = "LZ4 compress failed";
static const char *kCantWriteMessage      = "Can not write output file";
static const char *kCantReadMessage       = "Can not read input file";
static const char *kEmptyInputMessage     = "Input file is empty";
static const char *kFileTooLargeMessage   = "Input file is too large";
static const char *kSameFileMessage       = "Input and output file must be different";
static const char *kCantGetFileSizeMessage = "Can not determine input file size";

void PrintUsage(const char *programName)
{
  fprintf(stderr,
    "Usage: %s -e -o <out> <in>\n"
    "  -e: encode (compress) file\n"
    "  -o <out>: specify the output filename\n"
    "  -h, --help: display this help text\n",
    programName);
}

int PrintError(const char *message)
{
  fprintf(stderr, "Error: %s\n", message);
  return 1;
}

int main(int numArgs, const char *args[])
{
  int         encodeFlagSeen = 0;
  int         param;
  int         outSize;
  int         compressedSize;
  long        fileSize;
  char        *inBuffer;
  char        *outBuffer;
  const char  *inputFile = NULL;
  const char  *outputFile = NULL;
  FILE        *inFile;
  FILE        *outFile;

  for (param = 1; param < numArgs; param++) {
    if (strcmp(args[param], "-h") == 0 || strcmp(args[param], "--help") == 0) {
      PrintUsage(args[0]);
      return 0;
    } else if (strcmp(args[param], "-e") == 0) {
      encodeFlagSeen = 1;
    } else if (strcmp(args[param], "-o") == 0) {
      if (param + 1 >= numArgs) {
        PrintUsage(args[0]);
        return 1;
      }
      outputFile = args[++param];
    } else if (inputFile == NULL) {
      inputFile = args[param];
    } else {
      PrintUsage(args[0]);
      return 1;
    }
  }

  if (!encodeFlagSeen || inputFile == NULL || outputFile == NULL) {
    PrintUsage(args[0]);
    return 1;
  }

  if (strcmp(inputFile, outputFile) == 0) {
    return PrintError(kSameFileMessage);
  }

  //
  // Read the entire input file into memory.
  //
  inFile = fopen(inputFile, "rb");
  if (inFile == NULL) {
    return PrintError(kCantOpenInputMessage);
  }

  fseek(inFile, 0, SEEK_END);
  fileSize = ftell(inFile);
  fseek(inFile, 0, SEEK_SET);

  if (fileSize < 0) {
    fclose(inFile);
    return PrintError(kCantGetFileSizeMessage);
  }

  if (fileSize > INT_MAX) {
    fclose(inFile);
    return PrintError(kFileTooLargeMessage);
  }

  if (fileSize == 0) {
    fclose(inFile);
    return PrintError(kEmptyInputMessage);
  }

  inBuffer = (char *)malloc(fileSize);
  if (inBuffer == NULL) {
    fclose(inFile);
    return PrintError(kCantAllocateMessage);
  }

  if (fread(inBuffer, LZ4_BYTE_ELEMENT_SIZE, fileSize, inFile) != (size_t)fileSize) {
    fclose(inFile);
    free(inBuffer);
    return PrintError(kCantReadMessage);
  }
  fclose(inFile);

  //
  // Compress with LZ4 HC and prepend an 8-byte [OriginalSize][CompressedSize]
  // header so the decompressor knows both sizes up front.
  //
  outSize = LZ4_compressBound((int)fileSize);
  outBuffer = (char *)malloc(LZ4_HEADER_SIZE + outSize);
  if (outBuffer == NULL) {
    free(inBuffer);
    return PrintError(kCantAllocateMessage);
  }

  compressedSize = LZ4_compress_HC(inBuffer, outBuffer + LZ4_HEADER_SIZE, (int)fileSize, outSize, LZ4HC_CLEVEL_MAX);
  if (compressedSize <= 0) {
    free(inBuffer);
    free(outBuffer);
    return PrintError(kCompressFailedMessage);
  }

  {
    uint32_t originalSize = (uint32_t)fileSize;
    uint32_t storedCompressedSize = (uint32_t)compressedSize;

    memcpy(outBuffer + LZ4_HEADER_ORIGINALSIZE_OFFSET, &originalSize, sizeof(originalSize));
    memcpy(outBuffer + LZ4_HEADER_COMPRESSEDSIZE_OFFSET, &storedCompressedSize, sizeof(storedCompressedSize));
  }

  outFile = fopen(outputFile, "wb");
  if (outFile == NULL) {
    free(inBuffer);
    free(outBuffer);
    return PrintError(kCantOpenOutputMessage);
  }

  if (fwrite(outBuffer, LZ4_BYTE_ELEMENT_SIZE, LZ4_HEADER_SIZE + compressedSize, outFile) != (size_t)(LZ4_HEADER_SIZE + compressedSize)) {
    fclose(outFile);
    remove(outputFile);
    free(inBuffer);
    free(outBuffer);
    return PrintError(kCantWriteMessage);
  }

  fclose(outFile);
  free(inBuffer);
  free(outBuffer);
  return 0;
}
