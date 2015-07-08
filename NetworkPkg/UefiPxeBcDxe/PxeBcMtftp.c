/** @file
  Functions implementation related with Mtftp for UefiPxeBc Driver.

  Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

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
  This is a callback function when packets are received or transmitted in Mtftp driver.

  A callback function that is provided by the caller to intercept
  the EFI_MTFTP6_OPCODE_DATA or EFI_MTFTP6_OPCODE_DATA8 packets processed in the
  EFI_MTFTP6_PROTOCOL.ReadFile() function, and alternatively to intercept
  EFI_MTFTP6_OPCODE_OACK or EFI_MTFTP6_OPCODE_ERROR packets during a call to
  EFI_MTFTP6_PROTOCOL.ReadFile(), WriteFile() or ReadDirectory().

  @param[in]  This           Pointer to EFI_MTFTP6_PROTOCOL.
  @param[in]  Token          Pointer to EFI_MTFTP6_TOKEN.
  @param[in]  PacketLen      Length of EFI_MTFTP6_PACKET.
  @param[in]  Packet         Pointer to EFI_MTFTP6_PACKET to be checked.

  @retval EFI_SUCCESS    The current operation succeeded.
  @retval EFI_ABORTED    Abort the current transfer process.

**/
EFI_STATUS
EFIAPI
PxeBcMtftp6CheckPacket (
  IN EFI_MTFTP6_PROTOCOL              *This,
  IN EFI_MTFTP6_TOKEN                 *Token,
  IN UINT16                           PacketLen,
  IN EFI_MTFTP6_PACKET                *Packet
  )
{
  PXEBC_PRIVATE_DATA                  *Private;
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL *Callback;
  EFI_STATUS                          Status;

  Private   = (PXEBC_PRIVATE_DATA *) Token->Context;
  Callback  = Private->PxeBcCallback;
  Status    = EFI_SUCCESS;

  if (Packet->OpCode == EFI_MTFTP6_OPCODE_ERROR) {
    //
    // Store the tftp error message into mode data and set the received flag.
    //
    Private->Mode.TftpErrorReceived   = TRUE;
    Private->Mode.TftpError.ErrorCode = (UINT8) Packet->Error.ErrorCode;
    AsciiStrnCpyS (
      Private->Mode.TftpError.ErrorString,
      PXE_MTFTP_ERROR_STRING_LENGTH,
      (CHAR8 *) Packet->Error.ErrorMessage,
      PXE_MTFTP_ERROR_STRING_LENGTH - 1
      );
    Private->Mode.TftpError.ErrorString[PXE_MTFTP_ERROR_STRING_LENGTH - 1] = '\0';
  }

  if (Callback != NULL) {
    //
    // Callback to user if has when received any tftp packet.
    //
    Status = Callback->Callback (
                        Callback,
                        Private->Function,
                        TRUE,
                        PacketLen,
                        (EFI_PXE_BASE_CODE_PACKET *) Packet
                        );
    if (Status != EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE) {
      //
      // User wants to abort current process if not EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE.
      //
      Status = EFI_ABORTED;
    } else {
      //
      // User wants to continue current process if EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE.
      //
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}


/**
  This function is to get the size of a file using Tftp.

  @param[in]      Private        Pointer to PxeBc private data.
  @param[in]      Config         Pointer to EFI_MTFTP6_CONFIG_DATA.
  @param[in]      Filename       Pointer to boot file name.
  @param[in]      BlockSize      Pointer to required block size.
  @param[in, out] BufferSize     Pointer to buffer size.

  @retval EFI_SUCCESS        Sucessfully obtained the size of file.
  @retval EFI_NOT_FOUND      Parse the tftp ptions failed.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Has not obtained the size of the file.

**/
EFI_STATUS
PxeBcMtftp6GetFileSize (
  IN     PXEBC_PRIVATE_DATA           *Private,
  IN     EFI_MTFTP6_CONFIG_DATA       *Config,
  IN     UINT8                        *Filename,
  IN     UINTN                        *BlockSize,
  IN OUT UINT64                       *BufferSize
  )
{
  EFI_MTFTP6_PROTOCOL                 *Mtftp6;
  EFI_MTFTP6_OPTION                   ReqOpt[2];
  EFI_MTFTP6_PACKET                   *Packet;
  EFI_MTFTP6_OPTION                   *Option;
  UINT32                              PktLen;
  UINT8                               OptBuf[128];
  UINT32                              OptCnt;
  EFI_STATUS                          Status;

  *BufferSize               = 0;
  Status                    = EFI_DEVICE_ERROR;
  Mtftp6                    = Private->Mtftp6;
  Packet                    = NULL;
  Option                    = NULL;
  PktLen                    = 0;
  OptCnt                    = 1;
  Config->InitialServerPort = PXEBC_BS_DOWNLOAD_PORT;

  Status = Mtftp6->Configure (Mtftp6, Config);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Build the required options for get info.
  //
  ReqOpt[0].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_TSIZE_INDEX];
  PxeBcUintnToAscDec (0, OptBuf, PXE_MTFTP_OPTBUF_MAXNUM_INDEX);
  ReqOpt[0].ValueStr  = OptBuf;

  if (BlockSize != NULL) {
    ReqOpt[1].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[1].ValueStr  = (UINT8 *) (ReqOpt[0].ValueStr + AsciiStrLen ((CHAR8 *) ReqOpt[0].ValueStr) + 1);
    PxeBcUintnToAscDec (*BlockSize, ReqOpt[1].ValueStr, PXE_MTFTP_OPTBUF_MAXNUM_INDEX - (AsciiStrLen ((CHAR8 *) ReqOpt[0].ValueStr) + 1));
    OptCnt++;
  }

  Status = Mtftp6->GetInfo (
                     Mtftp6,
                     NULL,
                     Filename,
                     NULL,
                     (UINT8) OptCnt,
                     ReqOpt,
                     &PktLen,
                     &Packet
                     );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_TFTP_ERROR) {
      //
      // Store the tftp error message into mode data and set the received flag.
      //
      Private->Mode.TftpErrorReceived   = TRUE;
      Private->Mode.TftpError.ErrorCode = (UINT8) Packet->Error.ErrorCode;
      AsciiStrnCpyS (
        Private->Mode.TftpError.ErrorString,
        PXE_MTFTP_ERROR_STRING_LENGTH,
        (CHAR8 *) Packet->Error.ErrorMessage,
        PXE_MTFTP_ERROR_STRING_LENGTH - 1
        );
      Private->Mode.TftpError.ErrorString[PXE_MTFTP_ERROR_STRING_LENGTH - 1] = '\0';
    }
    goto ON_ERROR;
  }

  //
  // Parse the options in the reply packet.
  //
  OptCnt = 0;
  Status = Mtftp6->ParseOptions (
                     Mtftp6,
                     PktLen,
                     Packet,
                     (UINT32 *) &OptCnt,
                     &Option
                     );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Parse out the value of "tsize" option.
  //
  Status = EFI_NOT_FOUND;
  while (OptCnt != 0) {
    if (AsciiStrnCmp ((CHAR8 *) Option[OptCnt - 1].OptionStr, "tsize", 5) == 0) {
      *BufferSize = AsciiStrDecimalToUint64 ((CHAR8 *) (Option[OptCnt - 1].ValueStr));
      Status      = EFI_SUCCESS;
    }
    OptCnt--;
  }
  FreePool (Option);

ON_ERROR:
  if (Packet != NULL) {
    FreePool (Packet);
  }
  Mtftp6->Configure (Mtftp6, NULL);

  return Status;
}


/**
  This function is to get data of a file using Tftp.

  @param[in]      Private        Pointer to PxeBc private data.
  @param[in]      Config         Pointer to EFI_MTFTP6_CONFIG_DATA.
  @param[in]      Filename       Pointer to boot file name.
  @param[in]      BlockSize      Pointer to required block size.
  @param[in]      BufferPtr      Pointer to buffer.
  @param[in, out] BufferSize     Pointer to buffer size.
  @param[in]      DontUseBuffer  Indicates whether with a receive buffer.

  @retval EFI_SUCCESS        Successfully read the data from the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Read data from file failed.

**/
EFI_STATUS
PxeBcMtftp6ReadFile (
  IN    PXEBC_PRIVATE_DATA            *Private,
  IN     EFI_MTFTP6_CONFIG_DATA       *Config,
  IN     UINT8                        *Filename,
  IN     UINTN                        *BlockSize,
  IN     UINT8                        *BufferPtr,
  IN OUT UINT64                       *BufferSize,
  IN     BOOLEAN                      DontUseBuffer
  )
{
  EFI_MTFTP6_PROTOCOL                 *Mtftp6;
  EFI_MTFTP6_TOKEN                    Token;
  EFI_MTFTP6_OPTION                   ReqOpt[1];
  UINT32                              OptCnt;
  UINT8                               OptBuf[128];
  EFI_STATUS                          Status;

  Status                    = EFI_DEVICE_ERROR;
  Mtftp6                    = Private->Mtftp6;
  OptCnt                    = 0;
  Config->InitialServerPort = PXEBC_BS_DOWNLOAD_PORT;

  Status = Mtftp6->Configure (Mtftp6, Config);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BlockSize != NULL) {
    ReqOpt[0].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    PxeBcUintnToAscDec (*BlockSize, ReqOpt[0].ValueStr, PXE_MTFTP_OPTBUF_MAXNUM_INDEX);
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

  Token.CheckPacket     = PxeBcMtftp6CheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status = Mtftp6->ReadFile (Mtftp6, &Token);
  //
  // Get the real size of received buffer.
  //
  *BufferSize = Token.BufferSize;

  Mtftp6->Configure (Mtftp6, NULL);

  return Status;
}


/**
  This function is used to write the data of a file using Tftp.

  @param[in]       Private        Pointer to PxeBc private data.
  @param[in]       Config         Pointer to EFI_MTFTP6_CONFIG_DATA.
  @param[in]       Filename       Pointer to boot file name.
  @param[in]       Overwrite      Indicate whether with overwrite attribute.
  @param[in]       BlockSize      Pointer to required block size.
  @param[in]       BufferPtr      Pointer to buffer.
  @param[in, out]  BufferSize     Pointer to buffer size.

  @retval EFI_SUCCESS        Successfully wrote the data into a special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Write data into file failed.

**/
EFI_STATUS
PxeBcMtftp6WriteFile (
  IN     PXEBC_PRIVATE_DATA           *Private,
  IN     EFI_MTFTP6_CONFIG_DATA       *Config,
  IN     UINT8                        *Filename,
  IN     BOOLEAN                      Overwrite,
  IN     UINTN                        *BlockSize,
  IN     UINT8                        *BufferPtr,
  IN OUT UINT64                       *BufferSize
  )
{
  EFI_MTFTP6_PROTOCOL                 *Mtftp6;
  EFI_MTFTP6_TOKEN                    Token;
  EFI_MTFTP6_OPTION                   ReqOpt[1];
  UINT32                              OptCnt;
  UINT8                               OptBuf[128];
  EFI_STATUS                          Status;

  Status                    = EFI_DEVICE_ERROR;
  Mtftp6                    = Private->Mtftp6;
  OptCnt                    = 0;
  Config->InitialServerPort = PXEBC_BS_DOWNLOAD_PORT;

  Status = Mtftp6->Configure (Mtftp6, Config);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BlockSize != NULL) {
    ReqOpt[0].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    PxeBcUintnToAscDec (*BlockSize, ReqOpt[0].ValueStr, PXE_MTFTP_OPTBUF_MAXNUM_INDEX);
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
  Token.CheckPacket     = PxeBcMtftp6CheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status = Mtftp6->WriteFile (Mtftp6, &Token);
  //
  // Get the real size of transmitted buffer.
  //
  *BufferSize = Token.BufferSize;

  Mtftp6->Configure (Mtftp6, NULL);

  return Status;
}


/**
  This function is to read the data (file) from a directory using Tftp.

  @param[in]       Private        Pointer to PxeBc private data.
  @param[in]       Config         Pointer to EFI_MTFTP6_CONFIG_DATA.
  @param[in]       Filename       Pointer to boot file name.
  @param[in]       BlockSize      Pointer to required block size.
  @param[in]       BufferPtr      Pointer to buffer.
  @param[in, out]  BufferSize     Pointer to buffer size.
  @param[in]       DontUseBuffer  Indicates whether to use a receive buffer.

  @retval EFI_SUCCESS        Successfully obtained the data from the file included in directory.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Operation failed.

**/
EFI_STATUS
PxeBcMtftp6ReadDirectory (
  IN     PXEBC_PRIVATE_DATA            *Private,
  IN     EFI_MTFTP6_CONFIG_DATA        *Config,
  IN     UINT8                         *Filename,
  IN     UINTN                         *BlockSize,
  IN     UINT8                         *BufferPtr,
  IN OUT UINT64                        *BufferSize,
  IN     BOOLEAN                       DontUseBuffer
  )
{
  EFI_MTFTP6_PROTOCOL                  *Mtftp6;
  EFI_MTFTP6_TOKEN                     Token;
  EFI_MTFTP6_OPTION                    ReqOpt[1];
  UINT32                               OptCnt;
  UINT8                                OptBuf[128];
  EFI_STATUS                           Status;

  Status                    = EFI_DEVICE_ERROR;
  Mtftp6                    = Private->Mtftp6;
  OptCnt                    = 0;
  Config->InitialServerPort = PXEBC_BS_DOWNLOAD_PORT;

  Status = Mtftp6->Configure (Mtftp6, Config);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BlockSize != NULL) {
    ReqOpt[0].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    PxeBcUintnToAscDec (*BlockSize, ReqOpt[0].ValueStr, PXE_MTFTP_OPTBUF_MAXNUM_INDEX);
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

  Token.CheckPacket     = PxeBcMtftp6CheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status = Mtftp6->ReadDirectory (Mtftp6, &Token);
  //
  // Get the real size of received buffer.
  //
  *BufferSize = Token.BufferSize;

  Mtftp6->Configure (Mtftp6, NULL);

  return Status;
}


/**
  This is a callback function when packets are received or transmitted in Mtftp driver.

  A callback function that is provided by the caller to intercept
  the EFI_MTFTP6_OPCODE_DATA or EFI_MTFTP4_OPCODE_DATA8 packets processed in the
  EFI_MTFTP4_PROTOCOL.ReadFile() function, and alternatively to intercept
  EFI_MTFTP4_OPCODE_OACK or EFI_MTFTP4_OPCODE_ERROR packets during a call to
  EFI_MTFTP4_PROTOCOL.ReadFile(), WriteFile() or ReadDirectory().

  @param[in]  This           Pointer to EFI_MTFTP4_PROTOCOL.
  @param[in]  Token          Pointer to EFI_MTFTP4_TOKEN.
  @param[in]  PacketLen      Length of EFI_MTFTP4_PACKET.
  @param[in]  Packet         Pointer to EFI_MTFTP4_PACKET to be checked.

  @retval EFI_SUCCESS    The current operation succeeeded.
  @retval EFI_ABORTED    Abort the current transfer process.

**/
EFI_STATUS
EFIAPI
PxeBcMtftp4CheckPacket (
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
    //
    // Store the tftp error message into mode data and set the received flag.
    //
    Private->Mode.TftpErrorReceived   = TRUE;
    Private->Mode.TftpError.ErrorCode = (UINT8) Packet->Error.ErrorCode;
    AsciiStrnCpyS (
      Private->Mode.TftpError.ErrorString,
      PXE_MTFTP_ERROR_STRING_LENGTH,
      (CHAR8 *) Packet->Error.ErrorMessage,
      PXE_MTFTP_ERROR_STRING_LENGTH - 1
      );
    Private->Mode.TftpError.ErrorString[PXE_MTFTP_ERROR_STRING_LENGTH - 1] = '\0';
  }

  if (Callback != NULL) {
    //
    // Callback to user if has when received any tftp packet.
    //
    Status = Callback->Callback (
                        Callback,
                        Private->Function,
                        TRUE,
                        PacketLen,
                        (EFI_PXE_BASE_CODE_PACKET *) Packet
                        );
    if (Status != EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE) {
      //
      // User wants to abort current process if not EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE.
      //
      Status = EFI_ABORTED;
    } else {
      //
      // User wants to continue current process if EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE.
      //
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}


/**
  This function is to get size of a file using Tftp.

  @param[in]      Private        Pointer to PxeBc private data.
  @param[in]      Config         Pointer to EFI_MTFTP4_CONFIG_DATA.
  @param[in]      Filename       Pointer to boot file name.
  @param[in]      BlockSize      Pointer to required block size.
  @param[in, out] BufferSize     Pointer to buffer size.

  @retval EFI_SUCCESS        Successfully obtained the size of file.
  @retval EFI_NOT_FOUND      Parse the tftp options failed.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Did not obtain the size of the file.

**/
EFI_STATUS
PxeBcMtftp4GetFileSize (
  IN     PXEBC_PRIVATE_DATA         *Private,
  IN     EFI_MTFTP4_CONFIG_DATA     *Config,
  IN     UINT8                      *Filename,
  IN     UINTN                      *BlockSize,
  IN OUT UINT64                     *BufferSize
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

  //
  // Build the required options for get info.
  //
  ReqOpt[0].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_TSIZE_INDEX];
  PxeBcUintnToAscDec (0, OptBuf, PXE_MTFTP_OPTBUF_MAXNUM_INDEX);
  ReqOpt[0].ValueStr  = OptBuf;

  if (BlockSize != NULL) {
    ReqOpt[1].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[1].ValueStr  = (UINT8 *) (ReqOpt[0].ValueStr + AsciiStrLen ((CHAR8 *) ReqOpt[0].ValueStr) + 1);
    PxeBcUintnToAscDec (*BlockSize, ReqOpt[1].ValueStr, PXE_MTFTP_OPTBUF_MAXNUM_INDEX - (AsciiStrLen ((CHAR8 *) ReqOpt[0].ValueStr) + 1));
    OptCnt++;
  }

  Status = Mtftp4->GetInfo (
                     Mtftp4,
                     NULL,
                     Filename,
                     NULL,
                     (UINT8) OptCnt,
                     ReqOpt,
                     &PktLen,
                     &Packet
                     );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_TFTP_ERROR) {
      //
      // Store the tftp error message into mode data and set the received flag.
      //
      Private->Mode.TftpErrorReceived   = TRUE;
      Private->Mode.TftpError.ErrorCode = (UINT8) Packet->Error.ErrorCode;
      AsciiStrnCpyS (
        Private->Mode.TftpError.ErrorString,
        PXE_MTFTP_ERROR_STRING_LENGTH,
        (CHAR8 *) Packet->Error.ErrorMessage,
        PXE_MTFTP_ERROR_STRING_LENGTH - 1
        );
      Private->Mode.TftpError.ErrorString[PXE_MTFTP_ERROR_STRING_LENGTH - 1] = '\0';
    }
    goto ON_ERROR;
  }

  //
  // Parse the options in the reply packet.
  //
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

  //
  // Parse out the value of "tsize" option.
  //
  Status = EFI_NOT_FOUND;
  while (OptCnt != 0) {
    if (AsciiStrnCmp ((CHAR8 *) Option[OptCnt - 1].OptionStr, "tsize", 5) == 0) {
      *BufferSize = AsciiStrDecimalToUint64 ((CHAR8 *) (Option[OptCnt - 1].ValueStr));
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
  This function is to read the data of a file using Tftp.

  @param[in]      Private        Pointer to PxeBc private data.
  @param[in]      Config         Pointer to EFI_MTFTP4_CONFIG_DATA.
  @param[in]      Filename       Pointer to boot file name.
  @param[in]      BlockSize      Pointer to required block size.
  @param[in]      BufferPtr      Pointer to buffer.
  @param[in, out] BufferSize     Pointer to buffer size.
  @param[in]      DontUseBuffer  Indicates whether to use a receive buffer.

  @retval EFI_SUCCESS        Successfully read the data from the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Read data from file failed.

**/
EFI_STATUS
PxeBcMtftp4ReadFile (
  IN     PXEBC_PRIVATE_DATA         *Private,
  IN     EFI_MTFTP4_CONFIG_DATA     *Config,
  IN     UINT8                      *Filename,
  IN     UINTN                      *BlockSize,
  IN     UINT8                      *BufferPtr,
  IN OUT UINT64                     *BufferSize,
  IN     BOOLEAN                    DontUseBuffer
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
    ReqOpt[0].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    PxeBcUintnToAscDec (*BlockSize, ReqOpt[0].ValueStr, PXE_MTFTP_OPTBUF_MAXNUM_INDEX);
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

  Token.CheckPacket     = PxeBcMtftp4CheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status = Mtftp4->ReadFile (Mtftp4, &Token);
  //
  // Get the real size of received buffer.
  //
  *BufferSize = Token.BufferSize;

  Mtftp4->Configure (Mtftp4, NULL);

  return Status;
}


/**
  This function is to write the data of a file using Tftp.

  @param[in]       Private        Pointer to PxeBc private data.
  @param[in]       Config         Pointer to EFI_MTFTP4_CONFIG_DATA.
  @param[in]       Filename       Pointer to boot file name.
  @param[in]       Overwrite      Indicates whether to use the overwrite attribute.
  @param[in]       BlockSize      Pointer to required block size.
  @param[in]       BufferPtr      Pointer to buffer.
  @param[in, out]  BufferSize     Pointer to buffer size.

  @retval EFI_SUCCESS        Successfully write the data  into the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Write data into file failed.

**/
EFI_STATUS
PxeBcMtftp4WriteFile (
  IN     PXEBC_PRIVATE_DATA         *Private,
  IN     EFI_MTFTP4_CONFIG_DATA     *Config,
  IN     UINT8                      *Filename,
  IN     BOOLEAN                    Overwrite,
  IN     UINTN                      *BlockSize,
  IN     UINT8                      *BufferPtr,
  IN OUT UINT64                     *BufferSize
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
    ReqOpt[0].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    PxeBcUintnToAscDec (*BlockSize, ReqOpt[0].ValueStr, PXE_MTFTP_OPTBUF_MAXNUM_INDEX);
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
  Token.CheckPacket     = PxeBcMtftp4CheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status = Mtftp4->WriteFile (Mtftp4, &Token);
  //
  // Get the real size of transmitted buffer.
  //
  *BufferSize = Token.BufferSize;

  Mtftp4->Configure (Mtftp4, NULL);

  return Status;
}


/**
  This function is to get data (file) from a directory using Tftp.

  @param[in]       Private        Pointer to PxeBc private data.
  @param[in]       Config         Pointer to EFI_MTFTP4_CONFIG_DATA.
  @param[in]       Filename       Pointer to boot file name.
  @param[in]       BlockSize      Pointer to required block size.
  @param[in]       BufferPtr      Pointer to buffer.
  @param[in, out]  BufferSize     Pointer to buffer size.
  @param[in]       DontUseBuffer  Indicates whether to use a receive buffer.

  @retval EFI_SUCCES         Successfully obtained the data from the file included in the directory.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Operation failed.

**/
EFI_STATUS
PxeBcMtftp4ReadDirectory (
  IN     PXEBC_PRIVATE_DATA            *Private,
  IN     EFI_MTFTP4_CONFIG_DATA        *Config,
  IN     UINT8                         *Filename,
  IN     UINTN                         *BlockSize,
  IN     UINT8                         *BufferPtr,
  IN OUT UINT64                        *BufferSize,
  IN     BOOLEAN                       DontUseBuffer
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
    ReqOpt[0].OptionStr = (UINT8 *) mMtftpOptions[PXE_MTFTP_OPTION_BLKSIZE_INDEX];
    ReqOpt[0].ValueStr  = OptBuf;
    PxeBcUintnToAscDec (*BlockSize, ReqOpt[0].ValueStr, PXE_MTFTP_OPTBUF_MAXNUM_INDEX);
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

  Token.CheckPacket     = PxeBcMtftp4CheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status = Mtftp4->ReadDirectory (Mtftp4, &Token);
  //
  // Get the real size of received buffer.
  //
  *BufferSize = Token.BufferSize;

  Mtftp4->Configure (Mtftp4, NULL);

  return Status;
}


/**
  This function is wrapper to get the file size using TFTP.

  @param[in]      Private        Pointer to PxeBc private data.
  @param[in]      Config         Pointer to configure data.
  @param[in]      Filename       Pointer to boot file name.
  @param[in]      BlockSize      Pointer to required block size.
  @param[in, out] BufferSize     Pointer to buffer size.

  @retval EFI_SUCCESS        Successfully obtained the size of file.
  @retval EFI_NOT_FOUND      Parse the tftp options failed.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Did not obtain the size of the file.

**/
EFI_STATUS
PxeBcTftpGetFileSize (
  IN     PXEBC_PRIVATE_DATA         *Private,
  IN     VOID                       *Config,
  IN     UINT8                      *Filename,
  IN     UINTN                      *BlockSize,
  IN OUT UINT64                     *BufferSize
  )
{
  if (Private->PxeBc.Mode->UsingIpv6) {
    return PxeBcMtftp6GetFileSize (
             Private,
             (EFI_MTFTP6_CONFIG_DATA *) Config,
             Filename,
             BlockSize,
             BufferSize
             );
  } else {
    return PxeBcMtftp4GetFileSize (
             Private,
             (EFI_MTFTP4_CONFIG_DATA *) Config,
             Filename,
             BlockSize,
             BufferSize
             );
  }
}


/**
  This function is a wrapper to get file using TFTP.

  @param[in]      Private        Pointer to PxeBc private data.
  @param[in]      Config         Pointer to config data.
  @param[in]      Filename       Pointer to boot file name.
  @param[in]      BlockSize      Pointer to required block size.
  @param[in]      BufferPtr      Pointer to buffer.
  @param[in, out] BufferSize     Pointer to buffer size.
  @param[in]      DontUseBuffer  Indicates whether to use a receive buffer.

  @retval EFI_SUCCESS        Sucessfully read the data from the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Read data from file failed.

**/
EFI_STATUS
PxeBcTftpReadFile (
  IN     PXEBC_PRIVATE_DATA         *Private,
  IN     VOID                       *Config,
  IN     UINT8                      *Filename,
  IN     UINTN                      *BlockSize,
  IN     UINT8                      *BufferPtr,
  IN OUT UINT64                     *BufferSize,
  IN     BOOLEAN                    DontUseBuffer
  )
{
  if (Private->PxeBc.Mode->UsingIpv6) {
    return PxeBcMtftp6ReadFile (
             Private,
             (EFI_MTFTP6_CONFIG_DATA *) Config,
             Filename,
             BlockSize,
             BufferPtr,
             BufferSize,
             DontUseBuffer
             );
  } else {
    return PxeBcMtftp4ReadFile (
             Private,
             (EFI_MTFTP4_CONFIG_DATA *) Config,
             Filename,
             BlockSize,
             BufferPtr,
             BufferSize,
             DontUseBuffer
             );
  }
}


/**
  This function is a wrapper to write file using TFTP.

  @param[in]       Private        Pointer to PxeBc private data.
  @param[in]       Config         Pointer to config data.
  @param[in]       Filename       Pointer to boot file name.
  @param[in]       Overwrite      Indicate whether with overwrite attribute.
  @param[in]       BlockSize      Pointer to required block size.
  @param[in]       BufferPtr      Pointer to buffer.
  @param[in, out]  BufferSize     Pointer to buffer size.

  @retval EFI_SUCCESS        Successfully wrote the data into a special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Write data into file failed.

**/
EFI_STATUS
PxeBcTftpWriteFile (
  IN     PXEBC_PRIVATE_DATA         *Private,
  IN     VOID                       *Config,
  IN     UINT8                      *Filename,
  IN     BOOLEAN                    Overwrite,
  IN     UINTN                      *BlockSize,
  IN     UINT8                      *BufferPtr,
  IN OUT UINT64                     *BufferSize
  )
{
  if (Private->PxeBc.Mode->UsingIpv6) {
    return PxeBcMtftp6WriteFile (
             Private,
             (EFI_MTFTP6_CONFIG_DATA *) Config,
             Filename,
             Overwrite,
             BlockSize,
             BufferPtr,
             BufferSize
             );
  } else {
    return PxeBcMtftp4WriteFile (
             Private,
             (EFI_MTFTP4_CONFIG_DATA *) Config,
             Filename,
             Overwrite,
             BlockSize,
             BufferPtr,
             BufferSize
             );
  }
}


/**
  This function is a wrapper to get the data (file) from a directory using TFTP.

  @param[in]       Private        Pointer to PxeBc private data.
  @param[in]       Config         Pointer to config data.
  @param[in]       Filename       Pointer to boot file name.
  @param[in]       BlockSize      Pointer to required block size.
  @param[in]       BufferPtr      Pointer to buffer.
  @param[in, out]  BufferSize     Pointer to buffer size.
  @param[in]       DontUseBuffer  Indicatse whether to use a receive buffer.

  @retval EFI_SUCCES         Successfully obtained the data from the file included in the directory.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Operation failed.

**/
EFI_STATUS
PxeBcTftpReadDirectory (
  IN     PXEBC_PRIVATE_DATA            *Private,
  IN     VOID                          *Config,
  IN     UINT8                         *Filename,
  IN     UINTN                         *BlockSize,
  IN     UINT8                         *BufferPtr,
  IN OUT UINT64                        *BufferSize,
  IN     BOOLEAN                       DontUseBuffer
  )
{
  if (Private->PxeBc.Mode->UsingIpv6) {
    return PxeBcMtftp6ReadDirectory (
             Private,
             (EFI_MTFTP6_CONFIG_DATA *) Config,
             Filename,
             BlockSize,
             BufferPtr,
             BufferSize,
             DontUseBuffer
             );
  } else {
    return PxeBcMtftp4ReadDirectory (
             Private,
             (EFI_MTFTP4_CONFIG_DATA *) Config,
             Filename,
             BlockSize,
             BufferPtr,
             BufferSize,
             DontUseBuffer
             );
  }
}

