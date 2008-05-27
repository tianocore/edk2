/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PxeBcMtftp.c

Abstract:

  PxeBc MTFTP functions


**/

#include "PxeBcImpl.h"

VOID *TokenContext = NULL;

CHAR8 *mMtftpOptions[PXE_MTFTP_OPTION_MAXIMUM_INDEX] = {
  "blksize",
  "timeout",
  "tsize",
  "multicast"
};


/**
  This is a callback function when packets received/transmitted in Mtftp driver.

  @param  This           Pointer to Mtftp protocol instance
  @param  Token          Pointer to Mtftp token
  @param  PacketLen      Length of Mtftp packet
  @param  Packet         Pointer to Mtftp packet

  @return EFI_SUCCESS
  @return EFI_ABORTED

**/
EFI_STATUS
PxeBcCheckPacket (
  IN EFI_MTFTP4_PROTOCOL        *This,
  IN EFI_MTFTP4_TOKEN           *Token,
  IN UINT16                     PacketLen,
  IN EFI_MTFTP4_PACKET          *Packet
  )
{
  PXEBC_PRIVATE_DATA                  *Private;
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL *Callback;
  EFI_STATUS                          Status;

  Private   = (PXEBC_PRIVATE_DATA *) TokenContext;
  Callback  = Private->PxeBcCallback;
  Status    = EFI_SUCCESS;

  if (Packet->OpCode == EFI_MTFTP4_OPCODE_ERROR) {
    Private->Mode.TftpErrorReceived = TRUE;
    Private->Mode.TftpError.ErrorCode = (UINT8) Packet->Error.ErrorCode;
    AsciiStrnCpy (Private->Mode.TftpError.ErrorString, (CHAR8 *) Packet->Error.ErrorMessage, 127);
  }

  if (Callback != NULL) {

    Status = Callback->Callback (
                        Callback,
                        Private->Function,
                        TRUE,
                        PacketLen,
                        (EFI_PXE_BASE_CODE_PACKET *) Packet
                        );
    if (Status != EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE) {

      Status = EFI_ABORTED;
    } else {

      Status = EFI_SUCCESS;
    }
  }

  return Status;
}


/**
  This function is to get size of a file by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  BlockSize      Pointer to block size
  @param  BufferSize     Pointer to buffer size

  @return EFI_SUCCESS
  @return EFI_NOT_FOUND
  @return EFI_DEVICE_ERROR

**/
EFI_STATUS
PxeBcTftpGetFileSize (
  IN PXEBC_PRIVATE_DATA         *Private,
  IN EFI_MTFTP4_CONFIG_DATA     *Config,
  IN UINT8                      *Filename,
  IN UINTN                      *BlockSize,
  IN OUT UINT64                 *BufferSize
  )
{
  EFI_MTFTP4_PROTOCOL *Mtftp4;
  EFI_MTFTP4_OPTION   ReqOpt[2];
  EFI_MTFTP4_PACKET   *Packet;
  EFI_MTFTP4_OPTION   *Option;
  UINT32              PktLen;
  UINT8               OptBuf[128];
  UINT32              OptCnt;
  EFI_STATUS          Status;

  *BufferSize               = 0;
  Status                    = EFI_DEVICE_ERROR;
  Mtftp4                    = Private->Mtftp4;
  Packet                    = NULL;
  Option                    = NULL;
  PktLen                    = 0;
  OptCnt                    = 1;
  Config->InitialServerPort = PXEBC_BS_DOWNLOAD_PORT;

  Status = Mtftp4->Configure (Mtftp4, Config);
  if (EFI_ERROR (Status)) {

    return Status;
  }

  ReqOpt[0].OptionStr = (UINT8*)mMtftpOptions[PXE_MTFTP_OPTION_TSIZE_INDEX];
  UtoA10 (0, (CHAR8 *) OptBuf);
  ReqOpt[0].ValueStr = OptBuf;

  if (BlockSize != NULL) {
    ReqOpt[1].OptionStr = (UINT8*)mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[1].ValueStr  = ReqOpt[0].ValueStr + AsciiStrLen ((CHAR8 *) ReqOpt[0].ValueStr) + 1;
    UtoA10 (*BlockSize, (CHAR8 *) ReqOpt[1].ValueStr);
    OptCnt++;
  }

  Status = Mtftp4->GetInfo (
                    Mtftp4,
                    FALSE,
                    Filename,
                    NULL,
                    (UINT8) OptCnt,
                    ReqOpt,
                    &PktLen,
                    &Packet
                    );

  if (EFI_ERROR (Status)) {
    if (Packet->OpCode == EFI_MTFTP4_OPCODE_ERROR) {
      Private->Mode.TftpErrorReceived = TRUE;
      Private->Mode.TftpError.ErrorCode = (UINT8) Packet->Error.ErrorCode;
      AsciiStrnCpy (Private->Mode.TftpError.ErrorString, (CHAR8 *) Packet->Error.ErrorMessage, 127);
    }
    goto ON_ERROR;
  }

  OptCnt = 0;

  Status = Mtftp4->ParseOptions (
                    Mtftp4,
                    PktLen,
                    Packet,
                    (UINT32 *) &OptCnt,
                    &Option
                    );

  if (EFI_ERROR (Status)) {

    goto ON_ERROR;
  }

  Status = EFI_NOT_FOUND;

  while (OptCnt != 0) {

    if (AsciiStrnCmp ((CHAR8 *) Option[OptCnt - 1].OptionStr, "tsize", 5) == 0) {

      *BufferSize = AtoU64 (Option[OptCnt - 1].ValueStr);
      Status      = EFI_SUCCESS;
    }

    OptCnt--;
  }

  gBS->FreePool (Option);

ON_ERROR:

  if (Packet != NULL) {
    gBS->FreePool (Packet);
  }

  Mtftp4->Configure (Mtftp4, NULL);

  return Status;
}


/**
  This function is to get data of a file by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  BlockSize      Pointer to block size
  @param  BufferPtr      Pointer to buffer
  @param  BufferSize     Pointer to buffer size
  @param  DontUseBuffer  Indicate whether with a receive buffer

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR

**/
EFI_STATUS
PxeBcTftpReadFile (
  IN PXEBC_PRIVATE_DATA         *Private,
  IN EFI_MTFTP4_CONFIG_DATA     *Config,
  IN UINT8                      *Filename,
  IN UINTN                      *BlockSize,
  IN UINT8                      *BufferPtr,
  IN OUT UINT64                 *BufferSize,
  IN BOOLEAN                    DontUseBuffer
  )
{
  EFI_MTFTP4_PROTOCOL *Mtftp4;
  EFI_MTFTP4_TOKEN    Token;
  EFI_MTFTP4_OPTION   ReqOpt[1];
  UINT32              OptCnt;
  UINT8               OptBuf[128];
  EFI_STATUS          Status;

  Status                    = EFI_DEVICE_ERROR;
  Mtftp4                    = Private->Mtftp4;
  OptCnt                    = 0;
  Config->InitialServerPort = PXEBC_BS_DOWNLOAD_PORT;

  Status = Mtftp4->Configure (Mtftp4, Config);
  if (EFI_ERROR (Status)) {

    return Status;
  }

  if (BlockSize != NULL) {

    ReqOpt[0].OptionStr = (UINT8*) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    UtoA10 (*BlockSize, (CHAR8 *) ReqOpt[0].ValueStr);
    OptCnt++;
  }

  Token.Event         = NULL;
  Token.OverrideData  = NULL;
  Token.Filename      = Filename;
  Token.ModeStr       = NULL;
  Token.OptionCount   = OptCnt;
  Token.OptionList    = ReqOpt;
  TokenContext        = Private;

  if (DontUseBuffer) {
    Token.BufferSize  = 0;
    Token.Buffer      = NULL;
  } else {
    Token.BufferSize  = *BufferSize;
    Token.Buffer      = BufferPtr;
  }

  Token.CheckPacket     = PxeBcCheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status = Mtftp4->ReadFile (Mtftp4, &Token);

  *BufferSize = Token.BufferSize;

  Mtftp4->Configure (Mtftp4, NULL);

  return Status;
}


/**
  This function is put data of a file by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  Overwrite      Indicate whether with overwrite attribute
  @param  BlockSize      Pointer to block size
  @param  BufferPtr      Pointer to buffer
  @param  BufferSize     Pointer to buffer size

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR

**/
EFI_STATUS
PxeBcTftpWriteFile (
  IN PXEBC_PRIVATE_DATA         *Private,
  IN EFI_MTFTP4_CONFIG_DATA     *Config,
  IN UINT8                      *Filename,
  IN BOOLEAN                    Overwrite,
  IN UINTN                      *BlockSize,
  IN UINT8                      *BufferPtr,
  IN OUT UINT64                 *BufferSize
  )
{
  EFI_MTFTP4_PROTOCOL *Mtftp4;
  EFI_MTFTP4_TOKEN    Token;
  EFI_MTFTP4_OPTION   ReqOpt[1];
  UINT32              OptCnt;
  UINT8               OptBuf[128];
  EFI_STATUS          Status;

  Status                    = EFI_DEVICE_ERROR;
  Mtftp4                    = Private->Mtftp4;
  OptCnt                    = 0;
  Config->InitialServerPort = PXEBC_BS_DOWNLOAD_PORT;

  Status  = Mtftp4->Configure (Mtftp4, Config);
  if (EFI_ERROR (Status)) {

    return Status;
  }

  if (BlockSize != NULL) {

    ReqOpt[0].OptionStr = (UINT8*) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    UtoA10 (*BlockSize, (CHAR8 *) ReqOpt[0].ValueStr);
    OptCnt++;
  }

  Token.Event           = NULL;
  Token.OverrideData    = NULL;
  Token.Filename        = Filename;
  Token.ModeStr         = NULL;
  Token.OptionCount     = OptCnt;
  Token.OptionList      = ReqOpt;
  Token.BufferSize      = *BufferSize;
  Token.Buffer          = BufferPtr;
  Token.CheckPacket     = PxeBcCheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status      = Mtftp4->WriteFile (Mtftp4, &Token);
  *BufferSize = Token.BufferSize;

  Mtftp4->Configure (Mtftp4, NULL);

  return Status;
}


/**
  This function is to get data of a directory by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  BlockSize      Pointer to block size
  @param  BufferPtr      Pointer to buffer
  @param  BufferSize     Pointer to buffer size
  @param  DontUseBuffer  Indicate whether with a receive buffer

  @return EFI_SUCCES
  @return EFI_DEVICE_ERROR

**/
// GC_NOTO:    EFI_SUCCESS - add return value to function comment
EFI_STATUS
PxeBcTftpReadDirectory (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_MTFTP4_CONFIG_DATA        *Config,
  IN UINT8                         *Filename,
  IN UINTN                         *BlockSize,
  IN UINT8                         *BufferPtr,
  IN OUT UINT64                    *BufferSize,
  IN BOOLEAN                       DontUseBuffer
  )
{
  EFI_MTFTP4_PROTOCOL *Mtftp4;
  EFI_MTFTP4_TOKEN    Token;
  EFI_MTFTP4_OPTION   ReqOpt[1];
  UINT32              OptCnt;
  UINT8               OptBuf[128];
  EFI_STATUS          Status;

  Status                    = EFI_DEVICE_ERROR;
  Mtftp4                    = Private->Mtftp4;
  OptCnt                    = 0;
  Config->InitialServerPort = PXEBC_BS_DOWNLOAD_PORT;

  Status = Mtftp4->Configure (Mtftp4, Config);
  if (EFI_ERROR (Status)) {

    return Status;
  }

  if (BlockSize != NULL) {

    ReqOpt[0].OptionStr = (UINT8*) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    UtoA10 (*BlockSize, (CHAR8 *) ReqOpt[0].ValueStr);
    OptCnt++;
  }

  Token.Event         = NULL;
  Token.OverrideData  = NULL;
  Token.Filename      = Filename;
  Token.ModeStr       = NULL;
  Token.OptionCount   = OptCnt;
  Token.OptionList    = ReqOpt;
  TokenContext        = Private;

  if (DontUseBuffer) {
    Token.BufferSize  = 0;
    Token.Buffer      = NULL;
  } else {
    Token.BufferSize  = *BufferSize;
    Token.Buffer      = BufferPtr;
  }

  Token.CheckPacket     = PxeBcCheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status = Mtftp4->ReadDirectory (Mtftp4, &Token);

  *BufferSize = Token.BufferSize;

  Mtftp4->Configure (Mtftp4, NULL);

  return Status;
}

