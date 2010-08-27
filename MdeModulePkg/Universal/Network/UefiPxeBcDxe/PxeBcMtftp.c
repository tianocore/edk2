/** @file
  PxeBc MTFTP functions.
  
Copyright (c) 2007 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PxeBcImpl.h"

CHAR8 *mMtftpOptions[PXE_MTFTP_OPTION_MAXIMUM_INDEX] = {
  "blksize",
  "timeout",
  "tsize",
  "multicast"
};


/**
  This is a callback function when packets received/transmitted in Mtftp driver.

  A callback function that is provided by the caller to intercept               
  the EFI_MTFTP4_OPCODE_DATA or EFI_MTFTP4_OPCODE_DATA8 packets processed in the
  EFI_MTFTP4_PROTOCOL.ReadFile() function, and alternatively to intercept       
  EFI_MTFTP4_OPCODE_OACK or EFI_MTFTP4_OPCODE_ERROR packets during a call to    
  EFI_MTFTP4_PROTOCOL.ReadFile(), WriteFile() or ReadDirectory().
   
  @param  This           Pointer to Mtftp protocol instance
  @param  Token          Pointer to Mtftp token
  @param  PacketLen      Length of Mtftp packet
  @param  Packet         Pointer to Mtftp packet

  @retval EFI_SUCCESS    Operation sucess
  @retval EFI_ABORTED    Abort transfer process 

**/
EFI_STATUS
EFIAPI
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

  Private   = (PXEBC_PRIVATE_DATA *) Token->Context;
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

  @retval EFI_SUCCESS        Get the size of file success
  @retval EFI_NOT_FOUND      Parse the tftp ptions failed.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Other              Has not get the size of the file.
  
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
    if (Status == EFI_TFTP_ERROR) {
      Private->Mode.TftpErrorReceived = TRUE;
      Private->Mode.TftpError.ErrorCode = (UINT8) Packet->Error.ErrorCode;
      AsciiStrnCpy (
        Private->Mode.TftpError.ErrorString, 
        (CHAR8 *) Packet->Error.ErrorMessage, 
        127
        );
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

  FreePool (Option);

ON_ERROR:

  if (Packet != NULL) {
    FreePool (Packet);
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

  @retval EFI_SUCCESS        Read the data success from the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Read data from file failed.
  
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
  Token.Context       = Private;

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

  @retval EFI_SUCCESS        Write the data success into the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Write data into file failed.
  
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
  This function is to get data(file) from a directory(may be a server) by Tftp.

  @param  Private        Pointer to PxeBc private data.
  @param  Config         Pointer to Mtftp configuration data.
  @param  Filename       Pointer to file name.
  @param  BlockSize      Pointer to block size.
  @param  BufferPtr      Pointer to buffer.
  @param  BufferSize     Pointer to buffer size.
  @param  DontUseBuffer  Indicate whether with a receive buffer.

  @retval EFI_SUCCES         Get the data from the file included in directory success. 
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Operation failed.
  
**/
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
  Token.Context       = Private;

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

