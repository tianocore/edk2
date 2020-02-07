/** @file
  The implementation for the 'tftp' Shell command.

  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved. <BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Tftp.h"

#define IP4_CONFIG2_INTERFACE_INFO_NAME_LENGTH 32
EFI_HII_HANDLE   mTftpHiiHandle;

/*
   Constant strings and definitions related to the message indicating the amount of
   progress in the downloading of a TFTP file.
*/

// Frame for the progression slider
STATIC CONST CHAR16 mTftpProgressFrame[] = L"[                                        ]";

// Number of steps in the progression slider
#define TFTP_PROGRESS_SLIDER_STEPS  ((sizeof (mTftpProgressFrame) / sizeof (CHAR16)) - 3)

// Size in number of characters plus one (final zero) of the message to
// indicate the progress of a TFTP download. The format is "[(progress slider:
// 40 characters)] (nb of KBytes downloaded so far: 7 characters) Kb". There
// are thus the number of characters in mTftpProgressFrame[] plus 11 characters
// (2 // spaces, "Kb" and seven characters for the number of KBytes).
#define TFTP_PROGRESS_MESSAGE_SIZE  ((sizeof (mTftpProgressFrame) / sizeof (CHAR16)) + 12)

// String to delete the TFTP progress message to be able to update it :
// (TFTP_PROGRESS_MESSAGE_SIZE-1) '\b'
STATIC CONST CHAR16 mTftpProgressDelete[] = L"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";

// Local File Handle
SHELL_FILE_HANDLE     mFileHandle;

// Path of the local file, Unicode encoded
CONST CHAR16         *mLocalFilePath;

/**
  Check and convert the UINT16 option values of the 'tftp' command

  @param[in]  ValueStr  Value as an Unicode encoded string
  @param[out] Value     UINT16 value

  @return     TRUE      The value was returned.
  @return     FALSE     A parsing error occurred.
**/
STATIC
BOOLEAN
StringToUint16 (
  IN   CONST CHAR16  *ValueStr,
  OUT  UINT16        *Value
  );

/**
  Get the name of the NIC.

  @param[in]   ControllerHandle  The network physical device handle.
  @param[in]   NicNumber         The network physical device number.
  @param[out]  NicName           Address where to store the NIC name.
                                 The memory area has to be at least
                                 IP4_CONFIG2_INTERFACE_INFO_NAME_LENGTH
                                 double byte wide.

  @return  EFI_SUCCESS  The name of the NIC was returned.
  @return  Others       The creation of the child for the Managed
                        Network Service failed or the opening of
                        the Managed Network Protocol failed or
                        the operational parameters for the
                        Managed Network Protocol could not be
                        read.
**/
STATIC
EFI_STATUS
GetNicName (
  IN   EFI_HANDLE  ControllerHandle,
  IN   UINTN       NicNumber,
  OUT  CHAR16      *NicName
  );

/**
  Create a child for the service identified by its service binding protocol GUID
  and get from the child the interface of the protocol identified by its GUID.

  @param[in]   ControllerHandle            Controller handle.
  @param[in]   ServiceBindingProtocolGuid  Service binding protocol GUID of the
                                           service to be created.
  @param[in]   ProtocolGuid                GUID of the protocol to be open.
  @param[out]  ChildHandle                 Address where the handler of the
                                           created child is returned. NULL is
                                           returned in case of error.
  @param[out]  Interface                   Address where a pointer to the
                                           protocol interface is returned in
                                           case of success.

  @return  EFI_SUCCESS  The child was created and the protocol opened.
  @return  Others       Either the creation of the child or the opening
                        of the protocol failed.
**/
STATIC
EFI_STATUS
CreateServiceChildAndOpenProtocol (
  IN   EFI_HANDLE  ControllerHandle,
  IN   EFI_GUID    *ServiceBindingProtocolGuid,
  IN   EFI_GUID    *ProtocolGuid,
  OUT  EFI_HANDLE  *ChildHandle,
  OUT  VOID        **Interface
  );

/**
  Close the protocol identified by its GUID on the child handle of the service
  identified by its service binding protocol GUID, then destroy the child
  handle.

  @param[in]  ControllerHandle            Controller handle.
  @param[in]  ServiceBindingProtocolGuid  Service binding protocol GUID of the
                                          service to be destroyed.
  @param[in]  ProtocolGuid                GUID of the protocol to be closed.
  @param[in]  ChildHandle                 Handle of the child to be destroyed.

**/
STATIC
VOID
CloseProtocolAndDestroyServiceChild (
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_GUID    *ServiceBindingProtocolGuid,
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  ChildHandle
  );

/**
  Worker function that gets the size in numbers of bytes of a file from a TFTP
  server before to download the file.

  @param[in]   Mtftp4    MTFTP4 protocol interface
  @param[in]   FilePath  Path of the file, ASCII encoded
  @param[out]  FileSize  Address where to store the file size in number of
                         bytes.

  @retval  EFI_SUCCESS      The size of the file was returned.
  @retval  EFI_UNSUPPORTED  The server does not support the "tsize" option.
  @retval  Others           Error when retrieving the information from the server
                            (see EFI_MTFTP4_PROTOCOL.GetInfo() status codes)
                            or error when parsing the response of the server.
**/
STATIC
EFI_STATUS
GetFileSize (
  IN   EFI_MTFTP4_PROTOCOL  *Mtftp4,
  IN   CONST CHAR8          *FilePath,
  OUT  UINTN                *FileSize
  );

/**
  Worker function that download the data of a file from a TFTP server given
  the path of the file and its size.

  @param[in]   Mtftp4         MTFTP4 protocol interface
  @param[in]   FilePath       Path of the file, Unicode encoded
  @param[in]   AsciiFilePath  Path of the file, ASCII encoded
  @param[in]   FileSize       Size of the file in number of bytes
  @param[in]   BlockSize      Value of the TFTP blksize option
  @param[in]   WindowSize     Value of the TFTP window size option

  @retval  EFI_SUCCESS           The file was downloaded.
  @retval  EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval  Others                The downloading of the file from the server failed
                                 (see EFI_MTFTP4_PROTOCOL.ReadFile() status codes).

**/
STATIC
EFI_STATUS
DownloadFile (
  IN   EFI_MTFTP4_PROTOCOL  *Mtftp4,
  IN   CONST CHAR16         *FilePath,
  IN   CONST CHAR8          *AsciiFilePath,
  IN   UINTN                FileSize,
  IN   UINT16               BlockSize,
  IN   UINT16               WindowSize
  );

/**
  Update the progress of a file download
  This procedure is called each time a new TFTP packet is received.

  @param[in]  This       MTFTP4 protocol interface
  @param[in]  Token      Parameters for the download of the file
  @param[in]  PacketLen  Length of the packet
  @param[in]  Packet     Address of the packet

  @retval  EFI_SUCCESS  All packets are accepted.

**/
STATIC
EFI_STATUS
EFIAPI
CheckPacket (
  IN EFI_MTFTP4_PROTOCOL  *This,
  IN EFI_MTFTP4_TOKEN     *Token,
  IN UINT16               PacketLen,
  IN EFI_MTFTP4_PACKET    *Packet
  );

EFI_MTFTP4_CONFIG_DATA DefaultMtftp4ConfigData = {
  TRUE,                             // Use default setting
  { { 0, 0, 0, 0 } },               // StationIp         - Not relevant as UseDefaultSetting=TRUE
  { { 0, 0, 0, 0 } },               // SubnetMask        - Not relevant as UseDefaultSetting=TRUE
  0,                                // LocalPort         - Automatically assigned port number.
  { { 0, 0, 0, 0 } },               // GatewayIp         - Not relevant as UseDefaultSetting=TRUE
  { { 0, 0, 0, 0 } },               // ServerIp          - Not known yet
  69,                               // InitialServerPort - Standard TFTP server port
  6,                                // TryCount          - The number of times to transmit request packets and wait for a response.
  4                                 // TimeoutValue      - Retransmission timeout in seconds.
};

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-i", TypeValue},
  {L"-l", TypeValue},
  {L"-r", TypeValue},
  {L"-c", TypeValue},
  {L"-t", TypeValue},
  {L"-s", TypeValue},
  {L"-w", TypeValue},
  {NULL , TypeMax}
  };

///
/// The default block size (512) of tftp is defined in the RFC1350.
///
#define MTFTP_DEFAULT_BLKSIZE      512
///
/// The valid range of block size option is defined in the RFC2348.
///
#define MTFTP_MIN_BLKSIZE          8
#define MTFTP_MAX_BLKSIZE          65464
///
/// The default windowsize (1) of tftp.
///
#define MTFTP_DEFAULT_WINDOWSIZE   1
///
/// The valid range of window size option.
/// Note that: RFC 7440 does not mention max window size value, but for the
/// stability reason, the value is limited to 64.
///
#define MTFTP_MIN_WINDOWSIZE       1
#define MTFTP_MAX_WINDOWSIZE       64

/**
  Function for 'tftp' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).

  @return  SHELL_SUCCESS            The 'tftp' command completed successfully.
  @return  SHELL_ABORTED            The Shell Library initialization failed.
  @return  SHELL_INVALID_PARAMETER  At least one of the command's arguments is
                                    not valid.
  @return  SHELL_OUT_OF_RESOURCES   A memory allocation failed.
  @return  SHELL_NOT_FOUND          Network Interface Card not found or server
                                    error or file error.

**/
SHELL_STATUS
RunTftp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SHELL_STATUS            ShellStatus;
  EFI_STATUS              Status;
  LIST_ENTRY              *CheckPackage;
  CHAR16                  *ProblemParam;
  UINTN                   ParamCount;
  CONST CHAR16            *UserNicName;
  BOOLEAN                 NicFound;
  CONST CHAR16            *ValueStr;
  CONST CHAR16            *RemoteFilePath;
  CHAR8                   *AsciiRemoteFilePath;
  UINTN                   FilePathSize;
  CONST CHAR16            *Walker;
  EFI_MTFTP4_CONFIG_DATA  Mtftp4ConfigData;
  EFI_HANDLE              *Handles;
  UINTN                   HandleCount;
  UINTN                   NicNumber;
  CHAR16                  NicName[IP4_CONFIG2_INTERFACE_INFO_NAME_LENGTH];
  EFI_HANDLE              ControllerHandle;
  EFI_HANDLE              Mtftp4ChildHandle;
  EFI_MTFTP4_PROTOCOL     *Mtftp4;
  UINTN                   FileSize;
  UINT16                  BlockSize;
  UINT16                  WindowSize;

  ShellStatus         = SHELL_INVALID_PARAMETER;
  ProblemParam        = NULL;
  NicFound            = FALSE;
  AsciiRemoteFilePath = NULL;
  Handles             = NULL;
  FileSize            = 0;
  BlockSize           = MTFTP_DEFAULT_BLKSIZE;
  WindowSize          = MTFTP_DEFAULT_WINDOWSIZE;

  //
  // Initialize the Shell library (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return SHELL_ABORTED;
  }

  //
  // Parse the command line.
  //
  Status = ShellCommandLineParse (ParamList, &CheckPackage, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) &&
        (ProblemParam != NULL) ) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), mTftpHiiHandle,
        L"tftp", ProblemParam
        );
      FreePool (ProblemParam);
    } else {
      ASSERT (FALSE);
    }
    goto Error;
  }

  //
  // Check the number of parameters
  //
  ParamCount = ShellCommandLineGetCount (CheckPackage);
  if (ParamCount > 4) {
    ShellPrintHiiEx (
      -1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY),
      mTftpHiiHandle, L"tftp"
      );
    goto Error;
  }
  if (ParamCount < 3) {
    ShellPrintHiiEx (
      -1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW),
      mTftpHiiHandle, L"tftp"
      );
    goto Error;
  }

  CopyMem (&Mtftp4ConfigData, &DefaultMtftp4ConfigData, sizeof (EFI_MTFTP4_CONFIG_DATA));

  //
  // Check the host IPv4 address
  //
  ValueStr = ShellCommandLineGetRawValue (CheckPackage, 1);
  Status = NetLibStrToIp4 (ValueStr, &Mtftp4ConfigData.ServerIp);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV),
      mTftpHiiHandle, L"tftp", ValueStr
    );
    goto Error;
  }

  RemoteFilePath = ShellCommandLineGetRawValue (CheckPackage, 2);
  ASSERT(RemoteFilePath != NULL);
  FilePathSize = StrLen (RemoteFilePath) + 1;
  AsciiRemoteFilePath = AllocatePool (FilePathSize);
  if (AsciiRemoteFilePath == NULL) {
    ShellStatus = SHELL_OUT_OF_RESOURCES;
    goto Error;
  }
  UnicodeStrToAsciiStrS (RemoteFilePath, AsciiRemoteFilePath, FilePathSize);

  if (ParamCount == 4) {
    mLocalFilePath = ShellCommandLineGetRawValue (CheckPackage, 3);
  } else {
    Walker = RemoteFilePath + StrLen (RemoteFilePath);
    while ((--Walker) >= RemoteFilePath) {
      if ((*Walker == L'\\') ||
          (*Walker == L'/' )    ) {
        break;
      }
    }
    mLocalFilePath = Walker + 1;
  }

  //
  // Get the name of the Network Interface Card to be used if any.
  //
  UserNicName = ShellCommandLineGetValue (CheckPackage, L"-i");

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-l");
  if (ValueStr != NULL) {
    if (!StringToUint16 (ValueStr, &Mtftp4ConfigData.LocalPort)) {
      goto Error;
    }
  }

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-r");
  if (ValueStr != NULL) {
    if (!StringToUint16 (ValueStr, &Mtftp4ConfigData.InitialServerPort)) {
      goto Error;
    }
  }

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-c");
  if (ValueStr != NULL) {
    if (!StringToUint16 (ValueStr, &Mtftp4ConfigData.TryCount)) {
      goto Error;
    }

    if (Mtftp4ConfigData.TryCount == 0) {
      Mtftp4ConfigData.TryCount = 6;
    }
  }

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-t");
  if (ValueStr != NULL) {
    if (!StringToUint16 (ValueStr, &Mtftp4ConfigData.TimeoutValue)) {
      goto Error;
    }
    if (Mtftp4ConfigData.TimeoutValue == 0) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV),
        mTftpHiiHandle, L"tftp", ValueStr
      );
      goto Error;
    }
  }

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-s");
  if (ValueStr != NULL) {
    if (!StringToUint16 (ValueStr, &BlockSize)) {
      goto Error;
    }
    if (BlockSize < MTFTP_MIN_BLKSIZE || BlockSize > MTFTP_MAX_BLKSIZE) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV),
        mTftpHiiHandle, L"tftp", ValueStr
      );
      goto Error;
    }
  }

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-w");
  if (ValueStr != NULL) {
    if (!StringToUint16 (ValueStr, &WindowSize)) {
      goto Error;
    }
    if (WindowSize < MTFTP_MIN_WINDOWSIZE || WindowSize > MTFTP_MAX_WINDOWSIZE) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV),
        mTftpHiiHandle, L"tftp", ValueStr
      );
      goto Error;
    }
  }

  //
  // Locate all MTFTP4 Service Binding protocols
  //
  ShellStatus = SHELL_NOT_FOUND;
  Status = gBS->LocateHandleBuffer (
                 ByProtocol,
                 &gEfiManagedNetworkServiceBindingProtocolGuid,
                 NULL,
                 &HandleCount,
                 &Handles
                 );
  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    ShellPrintHiiEx (
      -1, -1, NULL, STRING_TOKEN (STR_TFTP_ERR_NO_NIC),
      mTftpHiiHandle
    );
    goto Error;
  }

  for (NicNumber = 0;
       (NicNumber < HandleCount) && (ShellStatus != SHELL_SUCCESS);
       NicNumber++) {
    ControllerHandle = Handles[NicNumber];

    Status = GetNicName (ControllerHandle, NicNumber, NicName);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_TFTP_ERR_NIC_NAME),
        mTftpHiiHandle, NicNumber, Status
      );
      continue;
    }

    if (UserNicName != NULL) {
      if (StrCmp (NicName, UserNicName) != 0) {
        continue;
      }
      NicFound = TRUE;
    }

    Status = CreateServiceChildAndOpenProtocol (
               ControllerHandle,
               &gEfiMtftp4ServiceBindingProtocolGuid,
               &gEfiMtftp4ProtocolGuid,
               &Mtftp4ChildHandle,
               (VOID**)&Mtftp4
               );
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_TFTP_ERR_OPEN_PROTOCOL),
        mTftpHiiHandle, NicName, Status
      );
      continue;
    }

    Status = Mtftp4->Configure (Mtftp4, &Mtftp4ConfigData);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_TFTP_ERR_CONFIGURE),
        mTftpHiiHandle, NicName, Status
      );
      goto NextHandle;
    }

    Status = GetFileSize (Mtftp4, AsciiRemoteFilePath, &FileSize);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_TFTP_ERR_FILE_SIZE),
        mTftpHiiHandle, RemoteFilePath, NicName, Status
      );
      goto NextHandle;
    }

    Status = DownloadFile (Mtftp4, RemoteFilePath, AsciiRemoteFilePath, FileSize, BlockSize, WindowSize);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_TFTP_ERR_DOWNLOAD),
        mTftpHiiHandle, RemoteFilePath, NicName, Status
      );
      goto NextHandle;
    }

    ShellStatus = SHELL_SUCCESS;

    NextHandle:

    CloseProtocolAndDestroyServiceChild (
      ControllerHandle,
      &gEfiMtftp4ServiceBindingProtocolGuid,
      &gEfiMtftp4ProtocolGuid,
      Mtftp4ChildHandle
      );
  }

  if ((UserNicName != NULL) && (!NicFound)) {
    ShellPrintHiiEx (
      -1, -1, NULL, STRING_TOKEN (STR_TFTP_ERR_NIC_NOT_FOUND),
      mTftpHiiHandle, UserNicName
    );
  }

  Error:

  ShellCommandLineFreeVarList (CheckPackage);
  if (AsciiRemoteFilePath != NULL) {
    FreePool (AsciiRemoteFilePath);
  }
  if (Handles != NULL) {
    FreePool (Handles);
  }

  if ((ShellStatus != SHELL_SUCCESS) && (EFI_ERROR(Status))) {
    ShellStatus = Status & ~MAX_BIT;
  }

  return ShellStatus;
}

/**
  Check and convert the UINT16 option values of the 'tftp' command

  @param[in]  ValueStr  Value as an Unicode encoded string
  @param[out] Value     UINT16 value

  @return     TRUE      The value was returned.
  @return     FALSE     A parsing error occurred.
**/
STATIC
BOOLEAN
StringToUint16 (
  IN   CONST CHAR16  *ValueStr,
  OUT  UINT16        *Value
  )
{
  UINTN  Val;

  Val = ShellStrToUintn (ValueStr);
  if (Val > MAX_UINT16) {
    ShellPrintHiiEx (
      -1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV),
      mTftpHiiHandle, L"tftp", ValueStr
    );
    return FALSE;
  }

  *Value = (UINT16)Val;
  return TRUE;
}

/**
  Get the name of the NIC.

  @param[in]   ControllerHandle  The network physical device handle.
  @param[in]   NicNumber         The network physical device number.
  @param[out]  NicName           Address where to store the NIC name.
                                 The memory area has to be at least
                                 IP4_CONFIG2_INTERFACE_INFO_NAME_LENGTH
                                 double byte wide.

  @return  EFI_SUCCESS  The name of the NIC was returned.
  @return  Others       The creation of the child for the Managed
                        Network Service failed or the opening of
                        the Managed Network Protocol failed or
                        the operational parameters for the
                        Managed Network Protocol could not be
                        read.
**/
STATIC
EFI_STATUS
GetNicName (
  IN   EFI_HANDLE  ControllerHandle,
  IN   UINTN       NicNumber,
  OUT  CHAR16      *NicName
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    MnpHandle;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;
  EFI_SIMPLE_NETWORK_MODE       SnpMode;

  Status = CreateServiceChildAndOpenProtocol (
             ControllerHandle,
             &gEfiManagedNetworkServiceBindingProtocolGuid,
             &gEfiManagedNetworkProtocolGuid,
             &MnpHandle,
             (VOID**)&Mnp
             );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Status = Mnp->GetModeData (Mnp, NULL, &SnpMode);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    goto Error;
  }

  UnicodeSPrint (
    NicName,
    IP4_CONFIG2_INTERFACE_INFO_NAME_LENGTH,
    SnpMode.IfType == NET_IFTYPE_ETHERNET ?
    L"eth%d" :
    L"unk%d" ,
    NicNumber
    );

  Status = EFI_SUCCESS;

Error:

  if (MnpHandle != NULL) {
    CloseProtocolAndDestroyServiceChild (
      ControllerHandle,
      &gEfiManagedNetworkServiceBindingProtocolGuid,
      &gEfiManagedNetworkProtocolGuid,
      MnpHandle
      );
  }

  return Status;
}

/**
  Create a child for the service identified by its service binding protocol GUID
  and get from the child the interface of the protocol identified by its GUID.

  @param[in]   ControllerHandle            Controller handle.
  @param[in]   ServiceBindingProtocolGuid  Service binding protocol GUID of the
                                           service to be created.
  @param[in]   ProtocolGuid                GUID of the protocol to be open.
  @param[out]  ChildHandle                 Address where the handler of the
                                           created child is returned. NULL is
                                           returned in case of error.
  @param[out]  Interface                   Address where a pointer to the
                                           protocol interface is returned in
                                           case of success.

  @return  EFI_SUCCESS  The child was created and the protocol opened.
  @return  Others       Either the creation of the child or the opening
                        of the protocol failed.
**/
STATIC
EFI_STATUS
CreateServiceChildAndOpenProtocol (
  IN   EFI_HANDLE  ControllerHandle,
  IN   EFI_GUID    *ServiceBindingProtocolGuid,
  IN   EFI_GUID    *ProtocolGuid,
  OUT  EFI_HANDLE  *ChildHandle,
  OUT  VOID        **Interface
  )
{
  EFI_STATUS  Status;

  *ChildHandle = NULL;
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             gImageHandle,
             ServiceBindingProtocolGuid,
             ChildHandle
             );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    *ChildHandle,
                    ProtocolGuid,
                    Interface,
                    gImageHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      NetLibDestroyServiceChild (
        ControllerHandle,
        gImageHandle,
        ServiceBindingProtocolGuid,
        *ChildHandle
        );
      *ChildHandle = NULL;
    }
  }

  return Status;
}

/**
  Close the protocol identified by its GUID on the child handle of the service
  identified by its service binding protocol GUID, then destroy the child
  handle.

  @param[in]  ControllerHandle            Controller handle.
  @param[in]  ServiceBindingProtocolGuid  Service binding protocol GUID of the
                                          service to be destroyed.
  @param[in]  ProtocolGuid                GUID of the protocol to be closed.
  @param[in]  ChildHandle                 Handle of the child to be destroyed.

**/
STATIC
VOID
CloseProtocolAndDestroyServiceChild (
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_GUID    *ServiceBindingProtocolGuid,
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  ChildHandle
  )
{
  gBS->CloseProtocol (
         ChildHandle,
         ProtocolGuid,
         gImageHandle,
         ControllerHandle
         );

  NetLibDestroyServiceChild (
    ControllerHandle,
    gImageHandle,
    ServiceBindingProtocolGuid,
    ChildHandle
    );
}

/**
  Worker function that gets the size in numbers of bytes of a file from a TFTP
  server before to download the file.

  @param[in]   Mtftp4    MTFTP4 protocol interface
  @param[in]   FilePath  Path of the file, ASCII encoded
  @param[out]  FileSize  Address where to store the file size in number of
                         bytes.

  @retval  EFI_SUCCESS      The size of the file was returned.
  @retval  EFI_UNSUPPORTED  The server does not support the "tsize" option.
  @retval  Others           Error when retrieving the information from the server
                            (see EFI_MTFTP4_PROTOCOL.GetInfo() status codes)
                            or error when parsing the response of the server.
**/
STATIC
EFI_STATUS
GetFileSize (
  IN   EFI_MTFTP4_PROTOCOL  *Mtftp4,
  IN   CONST CHAR8          *FilePath,
  OUT  UINTN                *FileSize
  )
{
  EFI_STATUS         Status;
  EFI_MTFTP4_OPTION  ReqOpt[1];
  EFI_MTFTP4_PACKET  *Packet;
  UINT32             PktLen;
  EFI_MTFTP4_OPTION  *TableOfOptions;
  EFI_MTFTP4_OPTION  *Option;
  UINT32             OptCnt;
  UINT8              OptBuf[128];

  ReqOpt[0].OptionStr = (UINT8*)"tsize";
  OptBuf[0] = '0';
  OptBuf[1] = 0;
  ReqOpt[0].ValueStr = OptBuf;

  Status = Mtftp4->GetInfo (
             Mtftp4,
             NULL,
             (UINT8*)FilePath,
             NULL,
             1,
             ReqOpt,
             &PktLen,
             &Packet
             );

  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Status = Mtftp4->ParseOptions (
                     Mtftp4,
                     PktLen,
                     Packet,
                     (UINT32 *) &OptCnt,
                     &TableOfOptions
                     );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Option = TableOfOptions;
  while (OptCnt != 0) {
    if (AsciiStrnCmp ((CHAR8 *)Option->OptionStr, "tsize", 5) == 0) {
      *FileSize = AsciiStrDecimalToUintn ((CHAR8 *)Option->ValueStr);
      break;
    }
    OptCnt--;
    Option++;
  }
  FreePool (TableOfOptions);

  if (OptCnt == 0) {
    Status = EFI_UNSUPPORTED;
  }

Error :

  return Status;
}

/**
  Worker function that download the data of a file from a TFTP server given
  the path of the file and its size.

  @param[in]   Mtftp4         MTFTP4 protocol interface
  @param[in]   FilePath       Path of the file, Unicode encoded
  @param[in]   AsciiFilePath  Path of the file, ASCII encoded
  @param[in]   FileSize       Size of the file in number of bytes
  @param[in]   BlockSize      Value of the TFTP blksize option
  @param[in]   WindowSize     Value of the TFTP window size option

  @retval  EFI_SUCCESS           The file was downloaded.
  @retval  EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval  Others                The downloading of the file from the server failed
                                 (see EFI_MTFTP4_PROTOCOL.ReadFile() status codes).

**/
STATIC
EFI_STATUS
DownloadFile (
  IN   EFI_MTFTP4_PROTOCOL  *Mtftp4,
  IN   CONST CHAR16         *FilePath,
  IN   CONST CHAR8          *AsciiFilePath,
  IN   UINTN                FileSize,
  IN   UINT16               BlockSize,
  IN   UINT16               WindowSize
  )
{
  EFI_STATUS            Status;
  DOWNLOAD_CONTEXT      *TftpContext;
  EFI_MTFTP4_TOKEN      Mtftp4Token;
  UINT8                 BlksizeBuf[10];
  UINT8                 WindowsizeBuf[10];

  ZeroMem (&Mtftp4Token, sizeof (EFI_MTFTP4_TOKEN));

  TftpContext = AllocatePool (sizeof (DOWNLOAD_CONTEXT));
  if (TftpContext == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }
  TftpContext->FileSize = FileSize;
  TftpContext->DownloadedNbOfBytes   = 0;
  TftpContext->LastReportedNbOfBytes = 0;

  Mtftp4Token.Filename    = (UINT8*)AsciiFilePath;
  Mtftp4Token.CheckPacket = CheckPacket;
  Mtftp4Token.Context     = (VOID*)TftpContext;
  Mtftp4Token.OptionCount = 0;
  Mtftp4Token.OptionList  = AllocatePool (sizeof (EFI_MTFTP4_OPTION) * 2);
  if (Mtftp4Token.OptionList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  if (BlockSize != MTFTP_DEFAULT_BLKSIZE) {
    Mtftp4Token.OptionList[Mtftp4Token.OptionCount].OptionStr = (UINT8 *) "blksize";
    AsciiSPrint ((CHAR8 *) BlksizeBuf, sizeof (BlksizeBuf), "%d", BlockSize);
    Mtftp4Token.OptionList[Mtftp4Token.OptionCount].ValueStr  = BlksizeBuf;
    Mtftp4Token.OptionCount ++;
  }

  if (WindowSize != MTFTP_DEFAULT_WINDOWSIZE) {
    Mtftp4Token.OptionList[Mtftp4Token.OptionCount].OptionStr = (UINT8 *) "windowsize";
    AsciiSPrint ((CHAR8 *) WindowsizeBuf, sizeof (WindowsizeBuf), "%d", WindowSize);
    Mtftp4Token.OptionList[Mtftp4Token.OptionCount].ValueStr  = WindowsizeBuf;
    Mtftp4Token.OptionCount ++;
  }

  ShellPrintHiiEx (
    -1, -1, NULL, STRING_TOKEN (STR_TFTP_DOWNLOADING),
    mTftpHiiHandle, FilePath
    );

  //
  // OPEN FILE
  //
  if (!EFI_ERROR (ShellFileExists (mLocalFilePath))) {
    ShellDeleteFileByName (mLocalFilePath);
  }

  Status = ShellOpenFileByName (
              mLocalFilePath,
              &mFileHandle,
              EFI_FILE_MODE_CREATE |
              EFI_FILE_MODE_WRITE  |
              EFI_FILE_MODE_READ,
              0
              );
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL),
      mTftpHiiHandle, L"tftp", mLocalFilePath
    );
    goto Error;
  }

  Status = Mtftp4->ReadFile (Mtftp4, &Mtftp4Token);
  ShellPrintHiiEx (
    -1, -1, NULL, STRING_TOKEN (STR_GEN_CRLF),
    mTftpHiiHandle
    );

  //
  // CLOSE FILE
  //
  ShellCloseFile (&mFileHandle);

Error :
  if (TftpContext != NULL) {
    FreePool (TftpContext);
  }

  if (Mtftp4Token.OptionList != NULL) {
    FreePool (Mtftp4Token.OptionList);
  }

  return Status;
}

/**
  Update the progress of a file download
  This procedure is called each time a new TFTP packet is received.

  @param[in]  This       MTFTP4 protocol interface
  @param[in]  Token      Parameters for the download of the file
  @param[in]  PacketLen  Length of the packet
  @param[in]  Packet     Address of the packet

  @retval  EFI_SUCCESS  All packets are accepted.

**/
STATIC
EFI_STATUS
EFIAPI
CheckPacket (
  IN EFI_MTFTP4_PROTOCOL  *This,
  IN EFI_MTFTP4_TOKEN     *Token,
  IN UINT16               PacketLen,
  IN EFI_MTFTP4_PACKET    *Packet
  )
{
  DOWNLOAD_CONTEXT  *Context;
  CHAR16            Progress[TFTP_PROGRESS_MESSAGE_SIZE];
  UINTN             NbOfKb;
  UINTN             Index;
  UINTN             LastStep;
  UINTN             Step;
  UINTN             DownloadLen;
  EFI_STATUS        Status;

  if ((NTOHS (Packet->OpCode)) != EFI_MTFTP4_OPCODE_DATA) {
    return EFI_SUCCESS;
  }

  Context = (DOWNLOAD_CONTEXT*)Token->Context;

  //
  // The data in the packet are prepended with two UINT16 :
  // . OpCode = EFI_MTFTP4_OPCODE_DATA
  // . Block  = the number of this block of data
  //
  DownloadLen = (UINTN)PacketLen - sizeof (Packet->OpCode) - sizeof (Packet->Data.Block);

  ShellSetFilePosition(mFileHandle, Context->DownloadedNbOfBytes);
  Status = ShellWriteFile (mFileHandle, &DownloadLen, Packet->Data.Data);
  if (EFI_ERROR (Status)) {
    if (Context->DownloadedNbOfBytes > 0) {
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN (STR_GEN_CRLF),
        mTftpHiiHandle
      );
    }
    ShellPrintHiiEx (
      -1, -1, NULL, STRING_TOKEN (STR_TFTP_ERR_WRITE),
      mTftpHiiHandle, mLocalFilePath, Status
    );
    return Status;
  }

  if (Context->DownloadedNbOfBytes == 0) {
    ShellPrintEx (-1, -1, L"%s       0 Kb", mTftpProgressFrame);
  }

  Context->DownloadedNbOfBytes += DownloadLen;
  NbOfKb = Context->DownloadedNbOfBytes / 1024;

  Progress[0] = L'\0';
  LastStep  = (Context->LastReportedNbOfBytes * TFTP_PROGRESS_SLIDER_STEPS) / Context->FileSize;
  Step      = (Context->DownloadedNbOfBytes * TFTP_PROGRESS_SLIDER_STEPS) / Context->FileSize;

  if (Step <= LastStep) {
    return EFI_SUCCESS;
  }

  ShellPrintEx (-1, -1, L"%s", mTftpProgressDelete);

  Status = StrCpyS (Progress, TFTP_PROGRESS_MESSAGE_SIZE, mTftpProgressFrame);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  for (Index = 1; Index < Step; Index++) {
    Progress[Index] = L'=';
  }
  Progress[Step] = L'>';

  UnicodeSPrint (
    Progress + (sizeof (mTftpProgressFrame) / sizeof (CHAR16)) - 1,
    sizeof (Progress) - sizeof (mTftpProgressFrame),
    L" %7d Kb",
    NbOfKb
    );
  Context->LastReportedNbOfBytes = Context->DownloadedNbOfBytes;

  ShellPrintEx (-1, -1, L"%s", Progress);

  return EFI_SUCCESS;
}

/**
  Retrieve HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
EFI_HII_HANDLE
InitializeHiiPackage (
  EFI_HANDLE                  ImageHandle
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  EFI_HII_HANDLE              HiiHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &HiiHandle
                           );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  return HiiHandle;
}
