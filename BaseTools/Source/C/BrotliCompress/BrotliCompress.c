/** @file
  BrotliCompress Compress/Decompress tool (BrotliCompress)

  Copyright (c) 2020, ByoSoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/* Command line interface for Brotli library. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "./brotli/c/common/constants.h"
#include "./brotli/c/common/version.h"
#include <brotli/decode.h>
#include <brotli/encode.h>

#if !defined(_WIN32)
#include <unistd.h>
#include <utime.h>
#else
#include <io.h>
#include <share.h>
#include <sys/utime.h>

#if !defined(__MINGW32__)
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#endif

#define fopen ms_fopen
#define open ms_open

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define fseek _fseeki64
#define ftell _ftelli64
#endif

static FILE* ms_fopen(const char* FileName, const char* Mode) {
  FILE* Result;
  Result = NULL;
  fopen_s(&Result, FileName, Mode);
  return Result;
}

static int ms_open(const char* FileName, int Oflag, int Pmode) {
  int Result;
  Result = -1;
  _sopen_s(&Result, FileName, Oflag | O_BINARY, _SH_DENYNO, Pmode);
  return Result;
}
#endif  /* WIN32 */


#ifndef _MAX_PATH
#define _MAX_PATH 500
#endif

#define DEFAULT_LGWIN 22
#define DECODE_HEADER_SIZE 0x10
#define GAP_MEM_BLOCK 0x1000
size_t ScratchBufferSize = 0;
static const size_t kFileBufferSize  = 1 << 19;

static void Version(void) {
  int Major;
  int Minor;
  int Patch;
  Major = BROTLI_VERSION >> 24;
  Minor = (BROTLI_VERSION >> 12) & 0xFFF;
  Patch = BROTLI_VERSION & 0xFFF;
  printf("BrotliCompress %d.%d.%d\n", Major, Minor, Patch);
}

static void Usage() {
  printf("Usage: %s [OPTION]... [FILE]...\n", __FILE__);
  printf(
"Options:\n"
"  -e, --compress              compress\n"
"  -d, --decompress            decompress\n"
"  -h, --help                  display this help and exit\n");
  printf(
"  -o FILE, --output=FILE      output file (only if 1 input file)\n");
 printf(
"  -g NUM, --gap=NUM           scratch memory gap level (1-16)\n");
  printf(
"  -q NUM, --quality=NUM       compression level (%d-%d)\n",
          BROTLI_MIN_QUALITY, BROTLI_MAX_QUALITY);
  printf(
"  -v, --version               display version and exit\n");
}

static int64_t FileSize(const char* Path) {
  FILE *FileHandle;
  int64_t RetVal;
  FileHandle = fopen(Path, "rb");

  if (FileHandle == NULL) {
    printf ("Failed to open file [%s]\n", Path);
    return -1;
  }
  if (fseek(FileHandle, 0L, SEEK_END) != 0) {
    printf ("Failed to seek file [%s]\n", Path);
    fclose(FileHandle);
    return -1;
  }
  RetVal = ftell(FileHandle);
  if (fclose(FileHandle) != 0) {
    printf ("Failed to close file [%s]\n", Path);
    return -1;
  }
  return RetVal;
}

static BROTLI_BOOL HasMoreInput(FILE *FileHandle) {
  return feof(FileHandle) ? BROTLI_FALSE : BROTLI_TRUE;
}

int OpenFiles(char *InputFile, FILE **InHandle, char *OutputFile, FILE **OutHandle) {
  *InHandle = NULL;
  *OutHandle = NULL;
  *InHandle = fopen(InputFile, "rb");
  if (*InHandle == NULL) {
    printf("Failed to open input file [%s]\n", InputFile);
    return BROTLI_FALSE;
  }

  *OutHandle = fopen(OutputFile, "wb+");
  if (*OutHandle == NULL) {
    printf("Failed to open output file [%s]\n", OutputFile);
    fclose(*InHandle);
    return BROTLI_FALSE;
  }
  return BROTLI_TRUE;
}

int CompressFile(char *InputFile, uint8_t *InputBuffer, char *OutputFile, uint8_t *OutputBuffer, int Quality, int Gap) {
  int64_t InputFileSize;
  FILE *InputFileHandle;
  FILE *OutputFileHandle;
  BrotliEncoderState *EncodeState;
  uint32_t LgWin;
  BROTLI_BOOL IsEof;
  size_t AvailableIn;
  const uint8_t *NextIn;
  size_t AvailableOut;
  uint8_t *NextOut;
  uint8_t *Input;
  uint8_t *Output;
  size_t TotalOut;
  size_t OutSize;
  uint32_t SizeHint;
  BROTLI_BOOL IsOk;
  AvailableIn = 0;
  IsEof = BROTLI_FALSE;
  Input = InputBuffer;
  Output = OutputBuffer;
  IsOk = BROTLI_TRUE;
  LgWin = DEFAULT_LGWIN;

  InputFileSize = FileSize(InputFile);

  IsOk = OpenFiles(InputFile, &InputFileHandle, OutputFile, &OutputFileHandle);
  if (!IsOk) {
    return IsOk;
  }

  fseek (OutputFileHandle, DECODE_HEADER_SIZE, SEEK_SET);

  EncodeState = BrotliEncoderCreateInstance(NULL, NULL, NULL);
  if (!EncodeState) {
    printf("Out of memory\n");
    IsOk = BROTLI_FALSE;
    goto Finish;
  }
  BrotliEncoderSetParameter(EncodeState, BROTLI_PARAM_QUALITY, (uint32_t)Quality);

  if (InputFileSize >= 0) {
    LgWin = BROTLI_MIN_WINDOW_BITS;
    while (BROTLI_MAX_BACKWARD_LIMIT(LgWin) < InputFileSize) {
      LgWin++;
      if (LgWin == BROTLI_MAX_WINDOW_BITS) {
        break;
      }
    }
  }
  BrotliEncoderSetParameter(EncodeState, BROTLI_PARAM_LGWIN, LgWin);
  if (InputFileSize > 0) {
    SizeHint = InputFileSize < (1 << 30)? (uint32_t)InputFileSize : (1u << 30);
    BrotliEncoderSetParameter(EncodeState, BROTLI_PARAM_SIZE_HINT, SizeHint);
  }

  AvailableIn = 0;
  NextIn = NULL;
  AvailableOut = kFileBufferSize;
  NextOut = Output;
  for (;;) {
    if (AvailableIn == 0 && !IsEof) {
      AvailableIn = fread(Input, 1, kFileBufferSize, InputFileHandle);
      NextIn = Input;
      if (ferror(InputFileHandle)) {
        printf("Failed to read input [%s]\n", InputFile);
        IsOk = BROTLI_FALSE;
        goto Finish;
      }
      IsEof = !HasMoreInput(InputFileHandle);
    }

    if (!IsEof){
      do{
        if (!BrotliEncoderCompressStream(EncodeState,
        BROTLI_OPERATION_FLUSH,
        &AvailableIn, &NextIn, &AvailableOut, &NextOut, &TotalOut)) {
          printf("Failed to compress data [%s]\n", InputFile);
          IsOk = BROTLI_FALSE;
          goto Finish;
        }
        OutSize = (size_t)(NextOut - Output);
        if (OutSize > 0) {
          fwrite(Output, 1, OutSize, OutputFileHandle);
          if (ferror(OutputFileHandle)) {
            printf("Failed to write output [%s]\n", OutputFile);
            IsOk = BROTLI_FALSE;
            goto Finish;
          }
        }
        NextOut = Output;
        AvailableOut = kFileBufferSize;
      }
      while (AvailableIn > 0 || BrotliEncoderHasMoreOutput(EncodeState));
    }
    else{
      do{
        if (!BrotliEncoderCompressStream(EncodeState,
        BROTLI_OPERATION_FINISH,
        &AvailableIn, &NextIn, &AvailableOut, &NextOut, &TotalOut)) {
          printf("Failed to compress data [%s]\n", InputFile);
          IsOk = BROTLI_FALSE;
          goto Finish;
        }
        OutSize = (size_t)(NextOut - Output);
        if (OutSize > 0) {
          fwrite(Output, 1, OutSize, OutputFileHandle);
          if (ferror(OutputFileHandle)) {
            printf("Failed to write output [%s]\n", OutputFile);
            IsOk = BROTLI_FALSE;
            goto Finish;
          }
        }
        NextOut = Output;
        AvailableOut = kFileBufferSize;
      }
      while (AvailableIn > 0 || BrotliEncoderHasMoreOutput(EncodeState));
    }
    if (BrotliEncoderIsFinished(EncodeState)){
      break;
    }
  }

Finish:
  if (EncodeState) {
    BrotliEncoderDestroyInstance(EncodeState);
  }
  if (InputFileHandle) {
    fclose(InputFileHandle);
  }
  if (OutputFileHandle) {
    fclose(OutputFileHandle);
  }
  return IsOk;
}

/* Default BrotliAllocFunc */
void* BrotliAllocFunc(void* Opaque, size_t Size) {
  *(size_t *)Opaque = *(size_t *) Opaque + Size;
  return malloc(Size);
}

/* Default BrotliFreeFunc */
void BrotliFreeFunc(void* Opaque, void* Address) {
  free(Address);
}

int DecompressFile(char *InputFile, uint8_t *InputBuffer, char *OutputFile, uint8_t *OutputBuffer, int Quality, int Gap) {
  FILE *InputFileHandle;
  FILE *OutputFileHandle;
  BrotliDecoderState *DecoderState;
  BrotliDecoderResult Result;
  size_t AvailableIn;
  const uint8_t *NextIn;
  size_t AvailableOut;
  uint8_t *NextOut;
  uint8_t *Input;
  uint8_t *Output;
  size_t OutSize;
  BROTLI_BOOL IsOk;
  AvailableIn = 0;
  Input = InputBuffer;
  Output = OutputBuffer;
  IsOk = BROTLI_TRUE;

  IsOk = OpenFiles(InputFile, &InputFileHandle, OutputFile, &OutputFileHandle);
  if (!IsOk) {
        return IsOk;
  }
  fseek(InputFileHandle, DECODE_HEADER_SIZE, SEEK_SET);

  DecoderState = BrotliDecoderCreateInstance(BrotliAllocFunc, BrotliFreeFunc, &ScratchBufferSize);
  if (!DecoderState) {
    printf("Out of memory\n");
    IsOk = BROTLI_FALSE;
    goto Finish;
  }
  /* This allows decoding "large-window" streams. Though it creates
       fragmentation (new builds decode streams that old builds don't),
       it is better from used experience perspective. */
  BrotliDecoderSetParameter(DecoderState, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);

  AvailableIn = 0;
  NextIn = NULL;
  AvailableOut = kFileBufferSize;
  NextOut = Output;
  Result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
  for (;;) {
    if (Result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
      if (!HasMoreInput(InputFileHandle)) {
        printf("Corrupt input [%s]\n", InputFile);
        IsOk = BROTLI_FALSE;
        goto Finish;
      }
      AvailableIn = fread(Input, 1, kFileBufferSize, InputFileHandle);
      NextIn = Input;
      if (ferror(InputFileHandle)) {
        printf("Failed to read input [%s]\n", InputFile);
        IsOk = BROTLI_FALSE;
        goto Finish;
      }
    } else if (Result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
      OutSize = (size_t) (NextOut - Output);
      if (OutSize > 0) {
        fwrite(Output, 1, OutSize, OutputFileHandle);
        if (ferror(OutputFileHandle)) {
          printf("Failed to write output [%s]\n", OutputFile);
          IsOk = BROTLI_FALSE;
          goto Finish;
        }
      }
      AvailableOut = kFileBufferSize;
      NextOut = Output;
    } else if (Result == BROTLI_DECODER_RESULT_SUCCESS) {
      OutSize = (size_t) (NextOut - Output);
      if (OutSize > 0) {
        fwrite(Output, 1, OutSize, OutputFileHandle);
        if (ferror(OutputFileHandle)) {
          printf("Failed to write output [%s]\n", OutputFile);
          IsOk = BROTLI_FALSE;
          goto Finish;
        }
      }
      AvailableOut = 0;
      if (AvailableIn != 0 || HasMoreInput(InputFileHandle)) {
        printf("Corrupt input [%s]\n", InputFile);
        IsOk = BROTLI_FALSE;
        goto Finish;
      }
    } else {
      printf("Corrupt input [%s]\n", InputFile);
      IsOk = BROTLI_FALSE;
      goto Finish;
    }
    if (!HasMoreInput(InputFileHandle) && Result == BROTLI_DECODER_RESULT_SUCCESS ) {
      break;
    }
    Result = BrotliDecoderDecompressStream(DecoderState, &AvailableIn, &NextIn, &AvailableOut, &NextOut, 0);
  }
Finish:
  if (DecoderState) {
    BrotliDecoderDestroyInstance(DecoderState);
  }
  if (InputFileHandle) {
    fclose(InputFileHandle);
  }
  if (OutputFileHandle) {
    fclose(OutputFileHandle);
  }
  return IsOk;
}

int main(int argc, char** argv) {
  BROTLI_BOOL CompressBool;
  BROTLI_BOOL DecompressBool;
  char *OutputFile;
  char *InputFile;
  char OutputTmpFile[_MAX_PATH];
  FILE *OutputHandle;
  int Quality;
  int Gap;
  int OutputFileLength;
  int InputFileLength;
  int Ret;
  size_t InputFileSize;
  uint8_t *Buffer;
  uint8_t *InputBuffer;
  uint8_t *OutputBuffer;
  int64_t Size;

  InputFile = NULL;
  OutputFile = NULL;
  CompressBool = BROTLI_FALSE;
  DecompressBool = BROTLI_FALSE;
  //
  //Set default Quality and Gap
  //
  Quality = 9;
  Gap = 1;
  InputFileSize = 0;
  Ret = 0;

  if (argc < 2) {
    Usage();
    return 1;
  }
  if (strcmp(argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0 ) {
    Usage();
    return 0;
  }
  if (strcmp(argv[1], "-v") == 0 || strcmp (argv[1], "--version") == 0 ) {
    Version();
    return 0;
  }
  while (argc > 1) {
    if (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "--compress") == 0 ) {
      CompressBool = BROTLI_TRUE;
      if (DecompressBool) {
        printf("Can't use -e/--compress with -d/--decompess on the same time\n");
        return 1;
      }
      argc--;
      argv++;
      continue;
    }
    if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--decompress") == 0 ) {
      DecompressBool = BROTLI_TRUE;
      if (CompressBool) {
        printf("Can't use -e/--compress with -d/--decompess on the same time\n");
        return 1;
      }
      argc--;
      argv++;
      continue;
    }
    if (strcmp(argv[1], "-o") == 0 || strncmp(argv[1], "--output", 8) == 0) {
      if (strcmp(argv[1], "-o") == 0) {
        OutputFileLength = strlen(argv[2]);
        if (OutputFileLength > _MAX_PATH) {
          printf ("The file path %s is too long\n", argv[2]);
          return 1;
        }
        OutputFile = argv[2];
        if (OutputFile == NULL) {
          fprintf(stderr, "Input file can't be null\n");
          return 1;
        }
        argc--;
        argv++;
      } else {
        OutputFileLength = strlen(argv[1] - 9);
        OutputFile = (char *)argv[1] + 9;
      }
      argc--;
      argv++;
      continue;
    }
    if (strcmp(argv[1], "-q") == 0 || strncmp(argv[1], "--quality", 9) == 0) {
      if (strcmp(argv[1], "-q") == 0) {
        Quality = strtol(argv[2], NULL, 16);
        argc--;
        argv++;
      } else {
        Quality = strtol((char *)argv[1] + 10, NULL, 16);
      }
      argc--;
      argv++;
      continue;
    }
    if (strcmp(argv[1], "-g") == 0 || strncmp(argv[1], "--gap", 5) == 0) {
      if (strcmp(argv[1], "-g") == 0) {
        Gap = strtol(argv[2], NULL, 16);
        argc--;
        argv++;
      } else {
        Gap = strtol((char *)argv[1] + 6, NULL, 16);
      }
      argc--;
      argv++;
      continue;
    }
    if (argc > 1) {
      InputFileLength = strlen(argv[1]);
      if (InputFileLength > _MAX_PATH - 1) {
        printf ("The file path %s is too long\n", argv[2]);
        return 1;
      }
      InputFile = argv[1];
      if (InputFile == NULL) {
       printf("Input file can't be null\n");
       return 1;
      }
      argc--;
      argv++;
    }
  }

  Buffer = (uint8_t*)malloc(kFileBufferSize * 2);
  if (!Buffer) {
    printf("Out of memory\n");
    goto Finish;
  }
  memset(Buffer, 0, kFileBufferSize*2);
  InputBuffer = Buffer;
  OutputBuffer = Buffer + kFileBufferSize;
  if (CompressBool) {
    //
    // Compress file
    //
    Ret = CompressFile(InputFile, InputBuffer, OutputFile, OutputBuffer, Quality, Gap);
    if (!Ret) {
      printf ("Failed to compress file [%s]\n", InputFile);
      goto Finish;
    }
    //
    // Decompress file for get Outputfile size
    //
    strcpy (OutputTmpFile, OutputFile);
    if (strlen(InputFile) + strlen(".tmp") < _MAX_PATH) {
      strcat(OutputTmpFile, ".tmp");
    } else {
      printf ("Output file path is too long[%s]\n", OutputFile);
      Ret = BROTLI_FALSE;
      goto Finish;
    }
    memset(Buffer, 0, kFileBufferSize*2);
    Ret = DecompressFile(OutputFile, InputBuffer, OutputTmpFile, OutputBuffer, Quality, Gap);
    if (!Ret) {
      printf ("Failed to decompress file [%s]\n", OutputFile);
      goto Finish;
    }
    remove (OutputTmpFile);

    //
    // fill decoder header
    //
    InputFileSize = FileSize(InputFile);
    Size = (int64_t)InputFileSize;
    OutputHandle = fopen(OutputFile, "rb+"); /* open output_path file and add in head info */
    fwrite(&Size, 1, sizeof(int64_t), OutputHandle);
    ScratchBufferSize += Gap * GAP_MEM_BLOCK; /* there is a memory gap between IA32 and X64 environment*/
    ScratchBufferSize += kFileBufferSize * 2;
    Size = (int64_t) ScratchBufferSize;
    fwrite(&Size, 1, sizeof(int64_t), OutputHandle);
    if (fclose(OutputHandle) != 0) {
      printf("Failed to close output file [%s]\n", OutputFile);
      Ret = BROTLI_FALSE;
      goto Finish;
    }
  } else {
    Ret = DecompressFile(InputFile, InputBuffer, OutputFile, OutputBuffer, Quality, Gap);
    if (!Ret) {
      printf ("Failed to decompress file [%s]\n", InputFile);
      goto Finish;
    }
  }
  Finish:
  if (Buffer != NULL) {
    free (Buffer);
  }
  return !Ret;
}
