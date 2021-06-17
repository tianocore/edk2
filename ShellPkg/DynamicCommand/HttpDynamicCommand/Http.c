/** @file
  The implementation for the 'http' Shell command.

  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved. <BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2020, Broadcom. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Http.h"

#define IP4_CONFIG2_INTERFACE_INFO_NAME_LENGTH 32

//
// Constant strings and definitions related to the message
// indicating the amount of progress in the dowloading of a HTTP file.
//

//
// Number of steps in the progression slider.
//
#define HTTP_PROGRESS_SLIDER_STEPS  \
  ((sizeof (HTTP_PROGR_FRAME) / sizeof (CHAR16)) - 3)

//
// Size in number of characters plus one (final zero) of the message to
// indicate the progress of an HTTP download. The format is "[(progress slider:
// 40 characters)] (nb of KBytes downloaded so far: 7 characters) Kb". There
// are thus the number of characters in HTTP_PROGR_FRAME[] plus 11 characters
// (2 // spaces, "Kb" and seven characters for the number of KBytes).
//
#define HTTP_PROGRESS_MESSAGE_SIZE  \
  ((sizeof (HTTP_PROGR_FRAME) / sizeof (CHAR16)) + 12)

//
// Buffer size. Note that larger buffer does not mean better speed.
//
#define DEFAULT_BUF_SIZE      SIZE_32KB
#define MAX_BUF_SIZE          SIZE_4MB

#define MIN_PARAM_COUNT       2
#define MAX_PARAM_COUNT       4
#define NEED_REDIRECTION(Code) \
  (((Code >= HTTP_STATUS_300_MULTIPLE_CHOICES) \
  && (Code <= HTTP_STATUS_307_TEMPORARY_REDIRECT)) \
  || (Code == HTTP_STATUS_308_PERMANENT_REDIRECT))

#define CLOSE_HTTP_HANDLE(ControllerHandle,HttpChildHandle) \
  do { \
    if (HttpChildHandle) { \
      CloseProtocolAndDestroyServiceChild ( \
        ControllerHandle, \
        &gEfiHttpServiceBindingProtocolGuid, \
        &gEfiHttpProtocolGuid, \
        HttpChildHandle \
        ); \
      HttpChildHandle = NULL; \
    } \
  } while (0)

typedef enum {
  HdrHost,
  HdrConn,
  HdrAgent,
  HdrMax
} HDR_TYPE;

#define USER_AGENT_HDR  "Mozilla/5.0 (EDK2; Linux) Gecko/20100101 Firefox/79.0"

#define TIMER_MAX_TIMEOUT_S   10

//
// File name to use when Uri ends with "/".
//
#define DEFAULT_HTML_FILE     L"index.html"
#define DEFAULT_HTTP_PROTO    L"http"

//
// String to delete the HTTP progress message to be able to update it :
// (HTTP_PROGRESS_MESSAGE_SIZE-1) '\b'.
//
#define HTTP_PROGRESS_DEL \
  L"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\
\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"

#define HTTP_KB              L"\b\b\b\b\b\b\b\b\b\b"
//
// Frame for the progression slider.
//
#define HTTP_PROGR_FRAME     L"[                                        ]"

//
// Improve readability by using these macros.
//
#define PRINT_HII(token,...) \
  ShellPrintHiiEx (\
      -1, -1, NULL, token, mHttpHiiHandle, __VA_ARGS__)

#define PRINT_HII_APP(token,value) \
  PRINT_HII (token, HTTP_APP_NAME, value)

//
// TimeBaseLib.h constants.
// These will be removed once the library gets fixed.
//

//
// Define EPOCH (1970-JANUARY-01) in the Julian Date representation.
//
#define EPOCH_JULIAN_DATE                               2440588

//
// Seconds per unit.
//
#define SEC_PER_MIN                                     ((UINTN)    60)
#define SEC_PER_HOUR                                    ((UINTN)  3600)
#define SEC_PER_DAY                                     ((UINTN) 86400)

//
// String descriptions for server errors.
//
STATIC CONST CHAR16 *ErrStatusDesc[] =
{
  L"400 Bad Request",
  L"401 Unauthorized",
  L"402 Payment required",
  L"403 Forbidden",
  L"404 Not Found",
  L"405 Method not allowed",
  L"406 Not acceptable",
  L"407 Proxy authentication required",
  L"408 Request time out",
  L"409 Conflict",
  L"410 Gone",
  L"411 Length required",
  L"412 Precondition failed",
  L"413 Request entity too large",
  L"414 Request URI to large",
  L"415 Unsupported media type",
  L"416 Requested range not satisfied",
  L"417 Expectation failed",
  L"500 Internal server error",
  L"501 Not implemented",
  L"502 Bad gateway",
  L"503 Service unavailable",
  L"504 Gateway timeout",
  L"505 HTTP version not supported"
};

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-i", TypeValue},
  {L"-k", TypeFlag},
  {L"-l", TypeValue},
  {L"-m", TypeFlag},
  {L"-s", TypeValue},
  {L"-t", TypeValue},
  {NULL , TypeMax}
};

//
// Local File Handle.
//
STATIC SHELL_FILE_HANDLE   mFileHandle = NULL;

//
// Path of the local file, Unicode encoded.
//
STATIC CONST CHAR16        *mLocalFilePath;

STATIC BOOLEAN             gRequestCallbackComplete = FALSE;
STATIC BOOLEAN             gResponseCallbackComplete = FALSE;

STATIC BOOLEAN             gHttpError;

EFI_HII_HANDLE             mHttpHiiHandle;

//
// Functions declarations.
//

/**
  Check and convert the UINT16 option values of the 'http' command.

  @param[in]  ValueStr  Value as an Unicode encoded string.
  @param[out] Value     UINT16 value.

  @retval     TRUE      The value was returned.
  @retval     FALSE     A parsing error occured.
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

  @retval  EFI_SUCCESS  The name of the NIC was returned.
  @retval  Others       The creation of the child for the Managed
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

  @retval  EFI_SUCCESS  The child was created and the protocol opened.
  @retval  Others       Either the creation of the child or the opening
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
  Worker function that download the data of a file from an HTTP server given
  the path of the file and its size.

  @param[in]   Context           A pointer to the download context.

  @retval  EFI_SUCCESS           The file was downloaded.
  @retval  EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval  Others                The downloading of the file
                                 from the server failed.
**/
STATIC
EFI_STATUS
DownloadFile (
  IN   HTTP_DOWNLOAD_CONTEXT   *Context,
  IN   EFI_HANDLE              ControllerHandle,
  IN   CHAR16                  *NicName
  );

/**
  Cleans off leading and trailing spaces and tabs.

  @param[in]                      String pointer to the string to trim them off.

  @retval EFI_SUCCESS             No errors.
  @retval EFI_INVALID_PARAMETER   String pointer is NULL.
**/
STATIC
EFI_STATUS
TrimSpaces (
  IN CHAR16 *String
  )
{
  CHAR16 *Str;
  UINTN  Len;

  ASSERT (String != NULL);

  if (String == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Str = String;

  //
  // Remove any whitespace at the beginning of the Str.
  //
  while (*Str == L' ' || *Str == L'\t') {
    Str++;
  }

  //
  // Remove any whitespace at the end of the Str.
  //
  do {
    Len = StrLen (Str);
    if (!Len || (Str[Len - 1] != L' ' && Str[Len - 1] != '\t')) {
      break;
    }

    Str[Len - 1] = CHAR_NULL;
  } while (Len);

  CopyMem (String, Str, StrSize (Str));

  return EFI_SUCCESS;
}

//
// Callbacks for request and response.
// We just acknowledge that operation has completed here.
//

/**
  Callback to set the request completion flag.

  @param[in] Event:   The event.
  @param[in] Context: pointer to Notification Context.
 **/
STATIC
VOID
EFIAPI
RequestCallback (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  gRequestCallbackComplete = TRUE;
}

/**
  Callback to set the response completion flag.
  @param[in] Event:   The event.
  @param[in] Context: pointer to Notification Context.
 **/
STATIC
VOID
EFIAPI
ResponseCallback (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  gResponseCallbackComplete = TRUE;
}

//
// Set of functions from TimeBaseLib.
// This will be removed once TimeBaseLib is enabled for ShellPkg.
//

/**
  Calculate Epoch days.

  @param[in] Time - a pointer to the EFI_TIME abstraction.

  @retval           Number of days elapsed since EPOCH_JULIAN_DAY.
 **/
STATIC
UINTN
EfiGetEpochDays (
  IN  EFI_TIME  *Time
  )
{
  UINTN a;
  UINTN y;
  UINTN m;
  //
  // Absolute Julian Date representation of the supplied Time.
  //
  UINTN JulianDate;
  //
  // Number of days elapsed since EPOCH_JULIAN_DAY.
  //
  UINTN EpochDays;

  a = (14 - Time->Month) / 12 ;
  y = Time->Year + 4800 - a;
  m = Time->Month + (12 * a) - 3;

  JulianDate = Time->Day + ((153 * m + 2) / 5) + (365 * y) + (y / 4) -
               (y / 100) + (y / 400) - 32045;

  ASSERT (JulianDate >= EPOCH_JULIAN_DATE);
  EpochDays = JulianDate - EPOCH_JULIAN_DATE;

  return EpochDays;
}

/**
  Converts EFI_TIME to Epoch seconds
  (elapsed since 1970 JANUARY 01, 00:00:00 UTC).

  @param[in] Time: a pointer to EFI_TIME abstraction.
 **/
STATIC
UINTN
EFIAPI
EfiTimeToEpoch (
  IN  EFI_TIME  *Time
  )
{
  //
  // Number of days elapsed since EPOCH_JULIAN_DAY.
  //
  UINTN EpochDays;
  UINTN EpochSeconds;

  EpochDays = EfiGetEpochDays (Time);

  EpochSeconds = (EpochDays * SEC_PER_DAY) +
                 ((UINTN)Time->Hour * SEC_PER_HOUR) +
                 (Time->Minute * SEC_PER_MIN) + Time->Second;

  return EpochSeconds;
}

/**
  Function for 'http' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).

  @retval  SHELL_SUCCESS            The 'http' command completed successfully.
  @retval  SHELL_ABORTED            The Shell Library initialization failed.
  @retval  SHELL_INVALID_PARAMETER  At least one of the command's arguments is
                                    not valid.
  @retval  SHELL_OUT_OF_RESOURCES   A memory allocation failed.
  @retval  SHELL_NOT_FOUND          Network Interface Card not found.
  @retval  SHELL_UNSUPPORTED        Command was valid, but the server returned
                                    a status code indicating some error.
                                    Examine the file requested for error body.
**/
SHELL_STATUS
RunHttp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *CheckPackage;
  UINTN                   ParamCount;
  UINTN                   HandleCount;
  UINTN                   NicNumber;
  UINTN                   InitialSize;
  UINTN                   ParamOffset;
  UINTN                   StartSize;
  CHAR16                  *ProblemParam;
  CHAR16                  NicName[IP4_CONFIG2_INTERFACE_INFO_NAME_LENGTH];
  CHAR16                  *Walker1;
  CHAR16                  *VStr;
  CONST CHAR16            *UserNicName;
  CONST CHAR16            *ValueStr;
  CONST CHAR16            *RemoteFilePath;
  CONST CHAR16            *Walker;
  EFI_HTTPv4_ACCESS_POINT IPv4Node;
  EFI_HANDLE              *Handles;
  EFI_HANDLE              ControllerHandle;
  HTTP_DOWNLOAD_CONTEXT   Context;
  BOOLEAN                 NicFound;

  ProblemParam        = NULL;
  RemoteFilePath      = NULL;
  NicFound            = FALSE;
  Handles             = NULL;

  //
  // Initialize the Shell library (we must be in non-auto-init...).
  //
  ParamOffset = 0;
  gHttpError  = FALSE;

  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return SHELL_ABORTED;
  }

  ZeroMem (&Context, sizeof (Context));

  //
  // Parse the command line.
  //
  Status = ShellCommandLineParse (
             ParamList,
             &CheckPackage,
             &ProblemParam,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED)
     && (ProblemParam != NULL))
    {
      PRINT_HII_APP (STRING_TOKEN (STR_GEN_PROBLEM), ProblemParam);
      SHELL_FREE_NON_NULL (ProblemParam);
    } else {
      ASSERT (FALSE);
    }

    goto Error;
  }

  //
  // Check the number of parameters.
  //
  Status = EFI_INVALID_PARAMETER;

  ParamCount = ShellCommandLineGetCount (CheckPackage);
  if (ParamCount > MAX_PARAM_COUNT) {
    PRINT_HII_APP (STRING_TOKEN (STR_GEN_TOO_MANY), NULL);
    goto Error;
  }

  if (ParamCount < MIN_PARAM_COUNT) {
    PRINT_HII_APP (STRING_TOKEN (STR_GEN_TOO_FEW), NULL);
    goto Error;
  }

  ZeroMem (&Context.HttpConfigData, sizeof (Context.HttpConfigData));
  ZeroMem (&IPv4Node, sizeof (IPv4Node));
  IPv4Node.UseDefaultAddress = TRUE;

  Context.HttpConfigData.HttpVersion = HttpVersion11;
  Context.HttpConfigData.AccessPoint.IPv4Node = &IPv4Node;

  //
  // Get the host address (not necessarily IPv4 format).
  //
  ValueStr = ShellCommandLineGetRawValue (CheckPackage, 1);
  if (!ValueStr) {
    PRINT_HII_APP (STRING_TOKEN (STR_GEN_PARAM_INV), ValueStr);
    goto Error;
  } else {
    StartSize = 0;
    TrimSpaces ((CHAR16 *)ValueStr);
    if (!StrStr (ValueStr, L"://")) {
      Context.ServerAddrAndProto = StrnCatGrow (
                                     &Context.ServerAddrAndProto,
                                     &StartSize,
                                     DEFAULT_HTTP_PROTO,
                                     StrLen (DEFAULT_HTTP_PROTO)
                                     );
      Context.ServerAddrAndProto = StrnCatGrow (
                                     &Context.ServerAddrAndProto,
                                     &StartSize,
                                     L"://",
                                     StrLen (L"://")
                                     );
      VStr = (CHAR16 *)ValueStr;
    } else {
      VStr = StrStr (ValueStr, L"://") + StrLen (L"://");
    }

    for (Walker1 = VStr; *Walker1; Walker1++) {
      if (*Walker1 == L'/') {
        break;
      }
    }

    if (*Walker1 == L'/') {
      ParamOffset = 1;
      RemoteFilePath = Walker1;
    }

    Context.ServerAddrAndProto = StrnCatGrow (
                                   &Context.ServerAddrAndProto,
                                   &StartSize,
                                   ValueStr,
                                   StrLen (ValueStr) - StrLen (Walker1)
                                   );
    if (!Context.ServerAddrAndProto) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }
  }

  if (!RemoteFilePath) {
    RemoteFilePath = ShellCommandLineGetRawValue (CheckPackage, 2);
    if (!RemoteFilePath) {
      //
      // If no path given, assume just "/".
      //
      RemoteFilePath = L"/";
    }
  }

  TrimSpaces ((CHAR16 *)RemoteFilePath);

  if (ParamCount == MAX_PARAM_COUNT - ParamOffset) {
    mLocalFilePath = ShellCommandLineGetRawValue (
                       CheckPackage,
                       MAX_PARAM_COUNT - 1 - ParamOffset
                       );
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

  if (!StrLen (mLocalFilePath)) {
    mLocalFilePath = DEFAULT_HTML_FILE;
  }

  InitialSize = 0;
  Context.Uri = StrnCatGrow (
                  &Context.Uri,
                  &InitialSize,
                  RemoteFilePath,
                  StrLen (RemoteFilePath)
                  );
  if (!Context.Uri) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Get the name of the Network Interface Card to be used if any.
  //
  UserNicName = ShellCommandLineGetValue (CheckPackage, L"-i");

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-l");
  if ((ValueStr != NULL)
   && (!StringToUint16 (
          ValueStr,
          &Context.HttpConfigData.AccessPoint.IPv4Node->LocalPort
          )
      ))
  {
    goto Error;
  }

  Context.BufferSize = DEFAULT_BUF_SIZE;

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-s");
  if (ValueStr != NULL) {
    Context.BufferSize = ShellStrToUintn (ValueStr);
    if (!Context.BufferSize || Context.BufferSize > MAX_BUF_SIZE) {
      PRINT_HII_APP (STRING_TOKEN (STR_GEN_PARAM_INV), ValueStr);
      goto Error;
    }
  }

  ValueStr = ShellCommandLineGetValue (CheckPackage, L"-t");
  if (ValueStr != NULL) {
    Context.HttpConfigData.TimeOutMillisec = (UINT32)ShellStrToUintn (ValueStr);
  }

  //
  // Locate all HTTP Service Binding protocols.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    PRINT_HII (STRING_TOKEN (STR_HTTP_ERR_NO_NIC), NULL);
    if (!EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
    }

    goto Error;
  }

  Status = EFI_NOT_FOUND;

  Context.Flags = 0;
  if (ShellCommandLineGetFlag (CheckPackage, L"-m")) {
    Context.Flags |= DL_FLAG_TIME;
  }

  if (ShellCommandLineGetFlag (CheckPackage, L"-k")) {
    Context.Flags |= DL_FLAG_KEEP_BAD;
  }

  for (NicNumber = 0;
       (NicNumber < HandleCount) && (Status != EFI_SUCCESS);
       NicNumber++)
  {
    ControllerHandle = Handles[NicNumber];

    Status = GetNicName (ControllerHandle, NicNumber, NicName);
    if (EFI_ERROR (Status)) {
      PRINT_HII (STRING_TOKEN (STR_HTTP_ERR_NIC_NAME), NicNumber, Status);
      continue;
    }

    if (UserNicName != NULL) {
      if (StrCmp (NicName, UserNicName) != 0) {
        Status = EFI_NOT_FOUND;
        continue;
      }

      NicFound = TRUE;
    }

    Status = DownloadFile (&Context, ControllerHandle, NicName);
    PRINT_HII (STRING_TOKEN (STR_GEN_CRLF), NULL);

    if (EFI_ERROR (Status)) {
      PRINT_HII (
        STRING_TOKEN (STR_HTTP_ERR_DOWNLOAD),
        RemoteFilePath,
        NicName,
        Status
        );
      //
      // If a user aborted the operation,
      // do not try another controller.
      //
      if (Status == EFI_ABORTED) {
        goto Error;
      }
    }

    if (gHttpError) {
     //
     // This is not related to connection, so no need to repeat with
     // another interface.
     //
      break;
    }
  }

  if ((UserNicName != NULL) && (!NicFound)) {
    PRINT_HII (STRING_TOKEN (STR_HTTP_ERR_NIC_NOT_FOUND), UserNicName);
  }

Error:
  ShellCommandLineFreeVarList (CheckPackage);
  SHELL_FREE_NON_NULL (Handles);
  SHELL_FREE_NON_NULL (Context.ServerAddrAndProto);
  SHELL_FREE_NON_NULL (Context.Uri);

  return Status & ~MAX_BIT;
}

/**
  Check and convert the UINT16 option values of the 'http' command

  @param[in]  ValueStr  Value as an Unicode encoded string
  @param[out] Value     UINT16 value

  @retval     TRUE      The value was returned.
  @retval     FALSE     A parsing error occured.
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
    PRINT_HII_APP (STRING_TOKEN (STR_GEN_PARAM_INV), ValueStr);
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

  @retval  EFI_SUCCESS  The name of the NIC was returned.
  @retval  Others       The creation of the child for the Managed
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
    SnpMode.IfType == NET_IFTYPE_ETHERNET ? L"eth%d" : L"unk%d",
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

  @retval  EFI_SUCCESS  The child was created and the protocol opened.
  @retval  Others       Either the creation of the child or the opening
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
  Wait until operation completes. Completion is indicated by
  setting of an appropriate variable.

  @param[in]      Context             A pointer to the HTTP download context.
  @param[in, out]  CallBackComplete   A pointer to the callback completion
                                      variable set by the callback.

  @retval  EFI_SUCCESS                Callback signalled completion.
  @retval  EFI_TIMEOUT                Timed out waiting for completion.
  @retval  Others                     Error waiting for completion.
**/
STATIC
EFI_STATUS
WaitForCompletion (
  IN HTTP_DOWNLOAD_CONTEXT  *Context,
  IN OUT BOOLEAN            *CallBackComplete
  )
{
  EFI_STATUS                Status;
  EFI_EVENT                 WaitEvt;

  Status = EFI_SUCCESS;

  //
  // Use a timer to measure timeout. Cannot use Stall here!
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &WaitEvt
                  );
   ASSERT_EFI_ERROR (Status);

   if (!EFI_ERROR (Status)) {
     Status = gBS->SetTimer (
                     WaitEvt,
                     TimerRelative,
                     EFI_TIMER_PERIOD_SECONDS (TIMER_MAX_TIMEOUT_S)
                     );

     ASSERT_EFI_ERROR (Status);
   }

  while (! *CallBackComplete
      && (!EFI_ERROR (Status))
      && EFI_ERROR (gBS->CheckEvent (WaitEvt)))
  {
    Status = Context->Http->Poll (Context->Http);
    if (!Context->ContentDownloaded
     && CallBackComplete == &gResponseCallbackComplete)
    {
      //
      // An HTTP server may just send a response redirection header.
      // In this case, don't wait for the event as
      // it might never happen and we waste 10s waiting.
      // Note that at this point Response may not has been populated,
      // so it needs to be checked first.
      //
      if (Context->ResponseToken.Message
       && Context->ResponseToken.Message->Data.Response
       && (NEED_REDIRECTION (
            Context->ResponseToken.Message->Data.Response->StatusCode
            )
         ))
      {
        break;
      }
    }
  }

  gBS->SetTimer (WaitEvt, TimerCancel, 0);
  gBS->CloseEvent (WaitEvt);

  if (*CallBackComplete) {
    return EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    Status = EFI_TIMEOUT;
  }

  return Status;
}

/**
  Generate and send a request to the http server.

  @param[in]   Context           HTTP download context.
  @param[in]   DownloadUrl       Fully qualified URL to be downloaded.

  @retval EFI_SUCCESS            Request has been sent successfully.
  @retval EFI_INVALID_PARAMETER  Invalid URL.
  @retval EFI_OUT_OF_RESOURCES   Out of memory.
  @retval EFI_DEVICE_ERROR       If HTTPS is used, this probably
                                 means that TLS support either was not
                                 installed or not configured.
  @retval Others                 Error sending the request.
**/
STATIC
EFI_STATUS
SendRequest (
  IN HTTP_DOWNLOAD_CONTEXT  *Context,
  IN CHAR16                 *DownloadUrl
  )
{
  EFI_HTTP_REQUEST_DATA       RequestData;
  EFI_HTTP_HEADER             RequestHeader[HdrMax];
  EFI_HTTP_MESSAGE            RequestMessage;
  EFI_STATUS                  Status;
  CHAR16                      *Host;
  UINTN                       StringSize;

  ZeroMem (&RequestData, sizeof (RequestData));
  ZeroMem (&RequestHeader, sizeof (RequestHeader));
  ZeroMem (&RequestMessage, sizeof (RequestMessage));
  ZeroMem (&Context->RequestToken, sizeof (Context->RequestToken));

  RequestHeader[HdrHost].FieldName = "Host";
  RequestHeader[HdrConn].FieldName = "Connection";
  RequestHeader[HdrAgent].FieldName = "User-Agent";

  Host = (CHAR16 *)Context->ServerAddrAndProto;
  while (*Host != CHAR_NULL && *Host != L'/') {
    Host++;
  }

  if (*Host == CHAR_NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the next slash.
  //
  Host++;
  //
  // And now the host name.
  //
  Host++;

  StringSize = StrLen (Host) + 1;
  RequestHeader[HdrHost].FieldValue = AllocatePool (StringSize);
  if (!RequestHeader[HdrHost].FieldValue) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeStrToAsciiStrS (
    Host,
    RequestHeader[HdrHost].FieldValue,
    StringSize
    );

  RequestHeader[HdrConn].FieldValue = "close";
  RequestHeader[HdrAgent].FieldValue = USER_AGENT_HDR;
  RequestMessage.HeaderCount = HdrMax;

  RequestData.Method = HttpMethodGet;
  RequestData.Url = DownloadUrl;

  RequestMessage.Data.Request = &RequestData;
  RequestMessage.Headers = RequestHeader;
  RequestMessage.BodyLength = 0;
  RequestMessage.Body = NULL;
  Context->RequestToken.Event = NULL;

  //
  // Completion callback event to be set when Request completes.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RequestCallback,
                  Context,
                  &Context->RequestToken.Event
                  );
  ASSERT_EFI_ERROR (Status);

  Context->RequestToken.Status = EFI_SUCCESS;
  Context->RequestToken.Message = &RequestMessage;
  gRequestCallbackComplete = FALSE;
  Status = Context->Http->Request (Context->Http, &Context->RequestToken);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Status = WaitForCompletion (Context, &gRequestCallbackComplete);
  if (EFI_ERROR (Status)) {
    Context->Http->Cancel (Context->Http, &Context->RequestToken);
  }

Error:
  SHELL_FREE_NON_NULL (RequestHeader[HdrHost].FieldValue);
  if (Context->RequestToken.Event) {
    gBS->CloseEvent (Context->RequestToken.Event);
    ZeroMem (&Context->RequestToken, sizeof (Context->RequestToken));
  }

  return Status;
}

/**
  Update the progress of a file download
  This procedure is called each time a new HTTP body portion is received.

  @param[in]  Context      HTTP download context.
  @param[in]  DownloadLen  Portion size, in bytes.
  @param[in]  Buffer       The pointer to the parsed buffer.

  @retval  EFI_SUCCESS     Portion saved.
  @retval  Other           Error saving the portion.
**/
STATIC
EFI_STATUS
EFIAPI
SavePortion (
  IN HTTP_DOWNLOAD_CONTEXT  *Context,
  IN UINTN                  DownloadLen,
  IN CHAR8                  *Buffer
  )
{
  CHAR16            Progress[HTTP_PROGRESS_MESSAGE_SIZE];
  UINTN             NbOfKb;
  UINTN             Index;
  UINTN             LastStep;
  UINTN             Step;
  EFI_STATUS        Status;

  LastStep = 0;
  Step = 0;

  ShellSetFilePosition (mFileHandle, Context->LastReportedNbOfBytes);
  Status = ShellWriteFile (mFileHandle, &DownloadLen, Buffer);
  if (EFI_ERROR (Status)) {
    if (Context->ContentDownloaded > 0) {
      PRINT_HII (STRING_TOKEN (STR_GEN_CRLF), NULL);
    }

    PRINT_HII (STRING_TOKEN (STR_HTTP_ERR_WRITE), mLocalFilePath, Status);
    return Status;
  }

  if (Context->ContentDownloaded == 0) {
    ShellPrintEx (-1, -1, L"%s       0 Kb", HTTP_PROGR_FRAME);
  }

  Context->ContentDownloaded += DownloadLen;
  NbOfKb = Context->ContentDownloaded >> 10;

  Progress[0] = L'\0';
  if (Context->ContentLength) {
    LastStep  = (Context->LastReportedNbOfBytes * HTTP_PROGRESS_SLIDER_STEPS) /
                 Context->ContentLength;
    Step      = (Context->ContentDownloaded * HTTP_PROGRESS_SLIDER_STEPS) /
                 Context->ContentLength;
  }

  Context->LastReportedNbOfBytes = Context->ContentDownloaded;

  if (Step <= LastStep) {
    if (!Context->ContentLength) {
      //
      // Update downloaded size, there is no length info available.
      //
      ShellPrintEx (-1, -1, L"%s", HTTP_KB);
      ShellPrintEx (-1, -1, L"%7d Kb", NbOfKb);
    }

    return EFI_SUCCESS;
  }

  ShellPrintEx (-1, -1, L"%s", HTTP_PROGRESS_DEL);

  Status = StrCpyS (Progress, HTTP_PROGRESS_MESSAGE_SIZE, HTTP_PROGR_FRAME);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 1; Index < Step; Index++) {
    Progress[Index] = L'=';
  }

  if (Step) {
    Progress[Step] = L'>';
  }

  UnicodeSPrint (
    Progress + (sizeof (HTTP_PROGR_FRAME) / sizeof (CHAR16)) - 1,
    sizeof (Progress) - sizeof (HTTP_PROGR_FRAME),
    L" %7d Kb",
    NbOfKb
    );


  ShellPrintEx (-1, -1, L"%s", Progress);

  return EFI_SUCCESS;
}

/**
  Replace the original Host and Uri with Host and Uri returned by the
  HTTP server in 'Location' header (redirection).

  @param[in]   Location           A pointer to the 'Location' string
                                  provided by HTTP server.
  @param[in]   Context            A pointer to HTTP download context.
  @param[in]   DownloadUrl        Fully qualified HTTP URL.

  @retval  EFI_SUCCESS            Host and Uri were successfully set.
  @retval  EFI_OUT_OF_RESOURCES   Error setting Host or Uri.
**/
STATIC
EFI_STATUS
SetHostURI (
  IN CHAR8                 *Location,
  IN HTTP_DOWNLOAD_CONTEXT *Context,
  IN CHAR16                *DownloadUrl
  )
{
  EFI_STATUS    Status;
  UINTN         StringSize;
  UINTN         FirstStep;
  UINTN         Idx;
  UINTN         Step;
  CHAR8         *Walker;
  CHAR16        *Temp;
  CHAR8         *Tmp;
  CHAR16        *Url;
  BOOLEAN       IsAbEmptyUrl;

  Tmp = NULL;
  Url = NULL;
  IsAbEmptyUrl = FALSE;
  FirstStep = 0;

  StringSize = (AsciiStrSize (Location) * sizeof (CHAR16));
  Url = AllocateZeroPool (StringSize);
  if (!Url) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AsciiStrToUnicodeStrS (
             (CONST CHAR8 *)Location,
             Url,
             StringSize
             );

  if (EFI_ERROR (Status)) {
        goto Error;
  }

  //
  // If an HTTP server redirects to the same location more than once,
  // then stop attempts and tell it is not reachable.
  //
  if (!StrCmp (Url, DownloadUrl)) {
    Status = EFI_NO_MAPPING;
    goto Error;
  }

  if (AsciiStrLen (Location) > 2) {
    //
    // Some servers return 'Location: //server/resource'
    //
    IsAbEmptyUrl = (Location[0] == '/') && (Location[1] == '/');
    if (IsAbEmptyUrl) {
      //
      // Skip first "//"
      //
      Location += 2;
      FirstStep = 1;
    }
  }

  if (AsciiStrStr (Location, "://") || IsAbEmptyUrl) {
    Idx = 0;
    Walker = Location;

    for (Step = FirstStep; Step < 2; Step++) {
      for (; *Walker != '/' && *Walker != '\0'; Walker++) {
        Idx++;
      }

      if (!Step) {
        //
        // Skip "//"
        //
        Idx += 2;
        Walker += 2;
      }
    }

    Tmp = AllocateZeroPool (Idx + 1);
    if (!Tmp) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    CopyMem (Tmp, Location, Idx);

    //
    // Location now points to Uri
    //
    Location += Idx;
    StringSize = (Idx + 1) * sizeof (CHAR16);

    SHELL_FREE_NON_NULL (Context->ServerAddrAndProto);

    Temp = AllocateZeroPool (StringSize);
    if (!Temp) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    Status = AsciiStrToUnicodeStrS (
               (CONST CHAR8 *)Tmp,
               Temp,
               StringSize
               );
    if (EFI_ERROR (Status)) {
      SHELL_FREE_NON_NULL (Temp);
      goto Error;
    }

    Idx = 0;
    if (IsAbEmptyUrl) {
      Context->ServerAddrAndProto = StrnCatGrow (
                                      &Context->ServerAddrAndProto,
                                      &Idx,
                                      L"http://",
                                      StrLen (L"http://")
                                      );
    }

    Context->ServerAddrAndProto = StrnCatGrow (
                                    &Context->ServerAddrAndProto,
                                    &Idx,
                                    Temp,
                                    StrLen (Temp)
                                    );
    SHELL_FREE_NON_NULL (Temp);
    if (!Context->ServerAddrAndProto) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }
  }

  SHELL_FREE_NON_NULL (Context->Uri);

  StringSize = AsciiStrSize (Location) * sizeof (CHAR16);
  Context->Uri = AllocateZeroPool (StringSize);
  if (!Context->Uri) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Now make changes to the Uri part.
  //
  Status = AsciiStrToUnicodeStrS (
             (CONST CHAR8 *)Location,
             Context->Uri,
             StringSize
             );
Error:
  SHELL_FREE_NON_NULL (Tmp);
  SHELL_FREE_NON_NULL (Url);

  return Status;
}

/**
  Message parser callback.
  Save a portion of HTTP body.

  @param[in]   EventType       Type of event. Can be either
                               OnComplete or OnData.
  @param[in]   Data            A pointer to the buffer with data.
  @param[in]   Length          Data length of this portion.
  @param[in]   Context         A pointer to the HTTP download context.

  @retval      EFI_SUCCESS    The portion was processed successfully.
  @retval      Other          Error returned by SavePortion.
**/
STATIC
EFI_STATUS
EFIAPI
ParseMsg (
  IN HTTP_BODY_PARSE_EVENT      EventType,
  IN CHAR8                      *Data,
  IN UINTN                      Length,
  IN VOID                       *Context
  )
{
  if ((Data == NULL)
   || (EventType == BodyParseEventOnComplete)
   || (Context == NULL))
  {
    return EFI_SUCCESS;
  }

  return SavePortion (Context, Length, Data);
}


/**
  Get HTTP server response and collect the whole body as a file.
  Set appropriate status in Context (REQ_OK, REQ_REPEAT, REQ_ERROR).
  Note that even if HTTP server returns an error code, it might send
  the body as well. This body will be collected in the resultant file.

  @param[in]   Context         A pointer to the HTTP download context.
  @param[in]   DownloadUrl     A pointer to the fully qualified URL to download.

  @retval  EFI_SUCCESS         Valid file. Body successfully collected.
  @retval  EFI_HTTP_ERROR      Response is a valid HTTP response, but the
                               HTTP server
                               indicated an error (HTTP code >= 400).
                               Response body MAY contain full
                               HTTP server response.
  @retval Others               Error getting the reponse from the HTTP server.
                               Response body is not collected.
**/
STATIC
EFI_STATUS
GetResponse (
  IN HTTP_DOWNLOAD_CONTEXT    *Context,
  IN CHAR16                   *DownloadUrl
  )
{
  EFI_HTTP_RESPONSE_DATA      ResponseData;
  EFI_HTTP_MESSAGE            ResponseMessage;
  EFI_HTTP_HEADER             *Header;
  EFI_STATUS                  Status;
  VOID                        *MsgParser;
  EFI_TIME                    StartTime;
  EFI_TIME                    EndTime;
  CONST CHAR16                *Desc;
  UINTN                       ElapsedSeconds;
  BOOLEAN                     IsTrunked;
  BOOLEAN                     CanMeasureTime;

  ZeroMem (&ResponseData, sizeof (ResponseData));
  ZeroMem (&ResponseMessage, sizeof (ResponseMessage));
  ZeroMem (&Context->ResponseToken, sizeof (Context->ResponseToken));
  IsTrunked = FALSE;

  ResponseMessage.Body = Context->Buffer;
  Context->ResponseToken.Status = EFI_SUCCESS;
  Context->ResponseToken.Message = &ResponseMessage;
  Context->ContentLength = 0;
  Context->Status = REQ_OK;
  Status = EFI_SUCCESS;
  MsgParser = NULL;
  ResponseData.StatusCode = HTTP_STATUS_UNSUPPORTED_STATUS;
  ResponseMessage.Data.Response = &ResponseData;
  Context->ResponseToken.Event = NULL;
  CanMeasureTime = FALSE;
  if (Context->Flags & DL_FLAG_TIME) {
    ZeroMem (&StartTime, sizeof (StartTime));
    CanMeasureTime = !EFI_ERROR (gRT->GetTime (&StartTime, NULL));
  }

  do {
    SHELL_FREE_NON_NULL (ResponseMessage.Headers);
    ResponseMessage.HeaderCount = 0;
    gResponseCallbackComplete = FALSE;
    ResponseMessage.BodyLength = Context->BufferSize;

    if (ShellGetExecutionBreakFlag ()) {
      Status = EFI_ABORTED;
      break;
    }

    if (!Context->ContentDownloaded && !Context->ResponseToken.Event) {
      Status = gBS->CreateEvent (
                      EVT_NOTIFY_SIGNAL,
                      TPL_CALLBACK,
                      ResponseCallback,
                      Context,
                      &Context->ResponseToken.Event
                      );
      ASSERT_EFI_ERROR (Status);
    } else {
      ResponseMessage.Data.Response = NULL;
    }

    if (EFI_ERROR (Status)) {
      break;
    }

    Status = Context->Http->Response (Context->Http, &Context->ResponseToken);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = WaitForCompletion (Context, &gResponseCallbackComplete);
    if (EFI_ERROR (Status) && ResponseMessage.HeaderCount) {
      Status = EFI_SUCCESS;
    }

    if (EFI_ERROR (Status)) {
      Context->Http->Cancel (Context->Http, &Context->ResponseToken);
      break;
    }

    if (!Context->ContentDownloaded) {
      if (NEED_REDIRECTION (ResponseData.StatusCode)) {
        //
        // Need to repeat the request with new Location (server redirected).
        //
        Context->Status = REQ_NEED_REPEAT;

        Header = HttpFindHeader (
                   ResponseMessage.HeaderCount,
                   ResponseMessage.Headers,
                   "Location"
                   );
        if (Header) {
          Status = SetHostURI (Header->FieldValue, Context, DownloadUrl);
          if (Status == EFI_NO_MAPPING) {
            PRINT_HII (
              STRING_TOKEN (STR_HTTP_ERR_STATUSCODE),
              Context->ServerAddrAndProto,
              L"Recursive HTTP server relocation",
              Context->Uri
              );
          }
        } else {
          //
          // Bad reply from the server. Server must specify the location.
          // Indicate that resource was not found, and no body collected.
          //
          Status = EFI_NOT_FOUND;
        }

        Context->Http->Cancel (Context->Http, &Context->ResponseToken);
        break;
      }

      //
      // Init message-body parser by header information.
      //
      if (!MsgParser) {
        Status = HttpInitMsgParser (
                   ResponseMessage.Data.Request->Method,
                   ResponseData.StatusCode,
                   ResponseMessage.HeaderCount,
                   ResponseMessage.Headers,
                   ParseMsg,
                   Context,
                   &MsgParser
                   );
        if (EFI_ERROR (Status)) {
          break;
        }
      }

      //
      // If it is a trunked message, rely on the parser.
      //
      Header = HttpFindHeader (
                 ResponseMessage.HeaderCount,
                 ResponseMessage.Headers,
                 "Transfer-Encoding"
                 );
      IsTrunked = (Header && !AsciiStrCmp (Header->FieldValue, "chunked"));

      HttpGetEntityLength (MsgParser, &Context->ContentLength);

      if (ResponseData.StatusCode >= HTTP_STATUS_400_BAD_REQUEST
       && (ResponseData.StatusCode != HTTP_STATUS_308_PERMANENT_REDIRECT))
      {
        //
        // Server reported an error via Response code.
        // Collect the body if any.
        //
        if (!gHttpError) {
          gHttpError = TRUE;

          Desc = ErrStatusDesc[ResponseData.StatusCode -
                               HTTP_STATUS_400_BAD_REQUEST];
          PRINT_HII (
            STRING_TOKEN (STR_HTTP_ERR_STATUSCODE),
            Context->ServerAddrAndProto,
            Desc,
            Context->Uri
            );

          //
          // This gives an RFC HTTP error.
          //
          Context->Status = ShellStrToUintn (Desc);
          Status = ENCODE_ERROR (Context->Status);
        }
      }
    }

    //
    // Do NOT try to parse an empty body.
    //
    if (ResponseMessage.BodyLength || IsTrunked) {
      Status = HttpParseMessageBody (
                 MsgParser,
                 ResponseMessage.BodyLength,
                 ResponseMessage.Body
                 );
    }
  } while (!HttpIsMessageComplete (MsgParser)
        && !EFI_ERROR (Status)
        && ResponseMessage.BodyLength);

  if (Context->Status != REQ_NEED_REPEAT
   && Status == EFI_SUCCESS
   && CanMeasureTime)
  {
    if (!EFI_ERROR (gRT->GetTime (&EndTime, NULL))) {
      ElapsedSeconds = EfiTimeToEpoch (&EndTime) - EfiTimeToEpoch (&StartTime);
      Print (
        L",%a%Lus\n",
        ElapsedSeconds ? " " : " < ",
        ElapsedSeconds > 1 ? (UINT64)ElapsedSeconds : 1
        );
    }
  }

  SHELL_FREE_NON_NULL (MsgParser);
  if (Context->ResponseToken.Event) {
    gBS->CloseEvent (Context->ResponseToken.Event);
    ZeroMem (&Context->ResponseToken, sizeof (Context->ResponseToken));
  }

  return Status;
}

/**
  Worker function that downloads the data of a file from an HTTP server given
  the path of the file and its size.

  @param[in]   Context           A pointer to the HTTP download context.
  @param[in]   ControllerHandle  The handle of the network interface controller
  @param[in]   NicName           NIC name

  @retval  EFI_SUCCESS           The file was downloaded.
  @retval  EFI_OUT_OF_RESOURCES  A memory allocation failed.
  #return  EFI_HTTP_ERROR        The server returned a valid HTTP error.
                                 Examine the mLocalFilePath file
                                 to get error body.
  @retval  Others                The downloading of the file from the server
                                 failed.
**/
STATIC
EFI_STATUS
DownloadFile (
  IN HTTP_DOWNLOAD_CONTEXT   *Context,
  IN EFI_HANDLE              ControllerHandle,
  IN CHAR16                  *NicName
  )
{
  EFI_STATUS                 Status;
  CHAR16                     *DownloadUrl;
  UINTN                      UrlSize;
  EFI_HANDLE                 HttpChildHandle;

  ASSERT (Context);
  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DownloadUrl = NULL;
  HttpChildHandle = NULL;

  Context->Buffer = AllocatePool (Context->BufferSize);
  if (Context->Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Open the file.
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
    PRINT_HII_APP (STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), mLocalFilePath);
    goto ON_EXIT;
  }

  do {
    SHELL_FREE_NON_NULL (DownloadUrl);

    CLOSE_HTTP_HANDLE (ControllerHandle, HttpChildHandle);

    Status = CreateServiceChildAndOpenProtocol (
               ControllerHandle,
               &gEfiHttpServiceBindingProtocolGuid,
               &gEfiHttpProtocolGuid,
               &HttpChildHandle,
               (VOID**)&Context->Http
               );

    if (EFI_ERROR (Status)) {
      PRINT_HII (STRING_TOKEN (STR_HTTP_ERR_OPEN_PROTOCOL), NicName, Status);
      goto ON_EXIT;
    }

    Status = Context->Http->Configure (Context->Http, &Context->HttpConfigData);
    if (EFI_ERROR (Status)) {
      PRINT_HII (STRING_TOKEN (STR_HTTP_ERR_CONFIGURE), NicName, Status);
      goto ON_EXIT;
    }

    UrlSize = 0;
    DownloadUrl = StrnCatGrow (
                    &DownloadUrl,
                    &UrlSize,
                    Context->ServerAddrAndProto,
                    StrLen (Context->ServerAddrAndProto)
                    );
    if (Context->Uri[0] != L'/') {
      DownloadUrl = StrnCatGrow (
                      &DownloadUrl,
                      &UrlSize,
                      L"/",
                      StrLen (Context->ServerAddrAndProto)
                      );
    }

    DownloadUrl = StrnCatGrow (
                    &DownloadUrl,
                    &UrlSize,
                    Context->Uri,
                    StrLen (Context->Uri));

    PRINT_HII (STRING_TOKEN (STR_HTTP_DOWNLOADING), DownloadUrl);

    Status = SendRequest (Context, DownloadUrl);
    if (Status) {
      goto ON_EXIT;
    }

    Status = GetResponse (Context, DownloadUrl);

    if (Status) {
      goto ON_EXIT;
    }

  } while (Context->Status == REQ_NEED_REPEAT);

  if (Context->Status) {
    Status = ENCODE_ERROR (Context->Status);
  }

ON_EXIT:
  //
  // Close the file.
  //
  if (mFileHandle != NULL) {
    if (EFI_ERROR (Status) && !(Context->Flags & DL_FLAG_KEEP_BAD)) {
      ShellDeleteFile (&mFileHandle);
    } else {
      ShellCloseFile (&mFileHandle);
    }
  }

  SHELL_FREE_NON_NULL (DownloadUrl);
  SHELL_FREE_NON_NULL (Context->Buffer);

  CLOSE_HTTP_HANDLE (ControllerHandle, HttpChildHandle);

  return Status;
}

/**
  Retrive HII package list from ImageHandle and publish to HII database.

  @param[in] ImageHandle            The image handle of the process.

  @retval HII handle.
**/
EFI_HII_HANDLE
InitializeHiiPackage (
  IN EFI_HANDLE                  ImageHandle
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  EFI_HII_HANDLE              HiiHandle;

  //
  // Retrieve HII package list from ImageHandle.
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
