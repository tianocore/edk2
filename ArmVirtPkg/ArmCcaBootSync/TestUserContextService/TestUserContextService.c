/** @file
  Host based unit test User Context service.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include <Base.h>
#include <Library/ArmCcaBootSyncCryptoLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi/UefiBaseType.h>

#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncSecureChannel.h"
#include "Include/BootSyncProtocol.h"

#include "BootSyncProtocolUserContext.h"

/**
  A structure describing the client connection.
**/
typedef struct {
  /// The linked list node.
  LIST_ENTRY        Node;
  /// A handle to the service thread.
  pthread_t         ThreadId;
  /// A handle to the secure channel.
  SECURE_CHANNEL    Channel;
} CLIENT_CONNECTION;

// The client connection linked list head.
STATIC CLIENT_CONNECTION  mConnList;

// A flag to signal the termination the server procedure.
STATIC BOOLEAN  gExitLoop = FALSE;

/**
  A string table describing the connection status.
*/
STATIC CHAR8  *SessionStateStr[] = {
  "UnConnected",
  "ConnectionEstablished"
};

/**
  A string table describing the attestation status.
*/
STATIC CHAR8  *AttestationStateStr[] = {
  "AttNotDone",
  "AttSuccess",
  "AttFailed"
};

/**
  A string table describing the Boot Sync status.
*/
STATIC CHAR8  *BootSyncStateStr[] = {
  "BootSyncStateUnknown",
  "BootSyncNotDone",
  "BootSyncComplete"
};

/**
  A string table describing the Boot Sync message GUIDs.
*/
STATIC struct {
  EFI_GUID    *Guid;
  CHAR8       *Name;
} MessageMap[] = {
  { &gArmBootSyncKeyEncData,      "ArmBootSyncKeyEncData"      },
  { &gArmBootSyncKeyXchgReqGuid,  "ArmBootSyncKeyXchgReqGuid"  },
  { &gArmBootSyncKeyXchgRespGuid, "ArmBootSyncKeyXchgRespGuid" },
  { &gArmBootSyncAttReqGuid,      "ArmBootSyncAttReqGuid"      },
  { &gArmBootSyncAttRespGuid,     "ArmBootSyncAttRespGuid"     },
  { &gArmBootSyncAttReport,       "ArmBootSyncAttReport"       },
  { &gArmBootSyncAttResult,       "ArmBootSyncAttResult"       },
  { &gArmBootSyncBibReqGuid,      "ArmBootSyncBibReqGuid"      },
  { &gArmBootSyncBibRespGuid,     "ArmBootSyncBibRespGuid"     },
  { &gArmBootSyncRequestOptions,  "ArmBootSyncRequestOptions"  },
  { &gArmBootSyncVarData,         "ArmBootSyncVarData"         },
  { &gArmBootSyncSecretData,      "ArmBootSyncSecretData"      },
  { &gArmBootSyncFinGuid,         "ArmBootSyncFinGuid"         },
  { &gArmBootSyncNackGuid,        "ArmBootSyncNackGuid"        }
};

/**
  A helper function to print the session status.

  @param[in]  Channel  Pointer to the secure channel.

**/
VOID
PrintProtocolStatus (
  IN SECURE_CHANNEL  *Channel
  )
{
  PROTOCOL_STATUS  *ProtocolStatus;

  ProtocolStatus = &Channel->ProtocolStatus;
  printf (
    "Info: Session State: %s\n",
    SessionStateStr[ProtocolStatus->SessionState]
    );
  printf (
    "Info: Attestation State: %s\n",
    AttestationStateStr[ProtocolStatus->AttestationState]
    );
  printf (
    "Info: BootSync State: %s\n",
    BootSyncStateStr[ProtocolStatus->BootSyncState]
    );
}

/**
  A helper function to print the Boot Sync message GUIDs.

  @param[in]  Guid  Message GUID.

**/
void
PrintGuid (
  EFI_GUID  *Guid
  )
{
  int    Index;
  CHAR8  *Msg;
  CHAR8  GuidStr[37]; // 32 hex chars + 4 hyphens + null terminator

  snprintf (
    GuidStr,
    sizeof (GuidStr),
    "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
    Guid->Data1,
    Guid->Data2,
    Guid->Data3,
    Guid->Data4[0],
    Guid->Data4[1],
    Guid->Data4[2],
    Guid->Data4[3],
    Guid->Data4[4],
    Guid->Data4[5],
    Guid->Data4[6],
    Guid->Data4[7]
    );

  Msg = "Not Found";
  for (Index = 0; Index < ARRAY_SIZE (MessageMap); Index++) {
    if (CompareGuid (MessageMap[Index].Guid, Guid)) {
      Msg = MessageMap[Index].Name;
      break;
    }
  }

  printf ("%s - %s\n", GuidStr, Msg);
}

/**
  A function that handles the attestation service requests.

  @param[in]  arg  Pointer to the client connection.

  @retval  A pointer to return when the thread terminates.
**/
void *
ServiceProc (
  void  *arg
  )
{
  EFI_STATUS           Status;
  CLIENT_CONNECTION    *Conn;
  SECURE_CHANNEL       *Channel;
  BOOT_SYNC_GUID_BLOB  *Msg;
  BOOLEAN              BootSyncCompleted;

  printf ("Info: Service procedure.\n");

  if (arg == NULL) {
    printf ("Error: Invalid client connection\n");
    return NULL;
  }

  Conn    = (CLIENT_CONNECTION *)arg;
  Channel = &Conn->Channel;

  PrintProtocolStatus (Channel);

  // Step 1: Establish a secure session
  while ((Channel->ProtocolStatus.SessionState == UnConnected)) {
    Status = EstablishSecureChannel (Channel);
    if (EFI_ERROR (Status)) {
      printf ("Error: Failed to establish secure channel.\n");
      goto ExitHandler;
    }
  }

  Msg = NULL;
  while (Channel->ProtocolStatus.SessionState == ConnectionEstablished) {
    PrintProtocolStatus (Channel);

    Status = ReceiveDecryptBsbMsg (
               Channel,
               (BOOT_SYNC_GUID_BLOB **)&Msg
               );
    if (EFI_ERROR (Status)) {
      printf ("Error: Failed to receive message.\n");
      break;
    }

    PrintGuid (&Msg->Name);

    if (CompareGuid (&Msg->Name, &gArmBootSyncFinGuid)) {
      Channel->ProtocolStatus.SessionState = UnConnected;
      printf ("Info: Received FIN. Disconnecting.\n");
      break;
    } else if (CompareGuid (&Msg->Name, &gArmBootSyncNackGuid)) {
      Channel->ProtocolStatus.SessionState = UnConnected;
      printf ("Error: Received NACK. Disconnecting.\n");
      break;
    } else if (CompareGuid (&Msg->Name, &gArmBootSyncAttReqGuid)) {
      // Step 2: Perform Attestation Report verification.
      BootSyncCompleted = FALSE;
      Status            = BootSyncValidateAttestation (Channel, Msg, &BootSyncCompleted);
      if (EFI_ERROR (Status)) {
        printf ("Error: Attestation verification failed.");
        SendFin (Channel, BOOT_SYNC_COMM_END_PROTOCOL_ERROR);
        break;
      }

      // Update the Boot Sync Status.
      Channel->ProtocolStatus.BootSyncState = BootSyncCompleted ?
                                              BootSyncComplete : BootSyncNotDone;
    }

    // Free the received message
    FreePool (Msg);
    Msg = NULL;
  }   // while

  if (Msg != NULL) {
    FreePool (Msg);
  }

  PrintProtocolStatus (Channel);

  TerminateSecureChannel (Channel);

ExitHandler:
  // Shutdown client socket
  if (Channel->SessionId != MAX_UINT64) {
    shutdown (Channel->SessionId, SHUT_RDWR);
    close (Channel->SessionId);
  }

  // Remove node from List.
  RemoveEntryList (&Conn->Node);
  free (Conn);

  printf ("Info: Service session Closed.\n");
  return NULL;
}

/**
  A function that handles the incomming connections.

  @param[in]  arg  Pointer to the server socked fd.

  @retval  A pointer to return when the thread terminates.
**/
void *
ServerProc (
  void  *arg
  )
{
  int                 Err;
  CLIENT_CONNECTION   *Conn;
  socklen_t           ClientAddrLen;
  struct sockaddr_in  ClientAddr;
  int                 ClientSockFd;
  int                 ServerSockFd;

  if (arg == NULL) {
    printf ("Error: Invalid Server Socket\n");
    return NULL;
  }

  ServerSockFd = *(int *)arg;

  printf ("Info: ServerSockFd = %d\n", ServerSockFd);

  InitializeListHead (&mConnList.Node);

  listen (ServerSockFd, 5);

  while (gExitLoop != TRUE) {
    Conn = (CLIENT_CONNECTION *)malloc (sizeof (CLIENT_CONNECTION));
    if (Conn == NULL) {
      printf ("Error: Out of Memory\n");
      return NULL;
    }

    ZeroMem (Conn, sizeof (CLIENT_CONNECTION));
    ZeroMem (&ClientAddr, sizeof (ClientAddr));
    ClientAddrLen = sizeof (ClientAddr);

    // This is a blocking call that waits
    // for an incomming connection.
    ClientSockFd = accept (
                     ServerSockFd,
                     (struct sockaddr *)&ClientAddr,
                     &ClientAddrLen
                     );
    if (ClientSockFd < 0) {
      // if there was an error in accepting the
      // connection, it would be either due to
      // a network error in which case we log
      // an error message and try to continue
      // or
      // shutdown was signalled, in which case
      // we exit the loop.
      if (gExitLoop != TRUE) {
        printf ("Error: Failed to accept connection!\n");
      }

      free (Conn);
      continue;
    }

    printf ("Info: ClientSockFd = %d\n", ClientSockFd);
    Conn->Channel.SessionId = (UINT64)ClientSockFd;

    // Add Connection to List.
    InsertTailList (&mConnList.Node, &Conn->Node);

    // Start the service thread.
    Err = pthread_create (&Conn->ThreadId, NULL, ServiceProc, Conn);
    if (Err != 0) {
      printf ("Error: Failed to create thread :[%s]", strerror (Err));
      // Shutdown client socket
      shutdown (ClientSockFd, SHUT_RDWR);
      close (ClientSockFd);

      // Remove node from List.
      RemoveEntryList (&Conn->Node);
      free (Conn);
      continue;
    }

    printf ("Info: Thread created successfully\n");
  } // while

  return NULL;
}

/**
  Create and initialise a server socket for the service.

  @param[in]  PortNo          PortNo to start the server socket on.
  @param[out] ServerSockFd    The server socket fd.

  @retval   Negative value on Failure, otherwise success.

**/
int
CreateServer (
  IN  int  PortNo,
  OUT int  *ServerSockFd
  )
{
  int                 Err;
  int                 SockFd;
  int                 EnableOption;
  struct sockaddr_in  ServerAddr;
  pthread_t           ThreadId;

  SockFd = socket (AF_INET, SOCK_STREAM, 0);
  if (SockFd < 0) {
    printf ("Error: Failed to create socket\n");
    *ServerSockFd = -1;
    return -1;
  }

  EnableOption = 1;
  Err          = setsockopt (
                   SockFd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &EnableOption,
                   sizeof (EnableOption)
                   );
  if (Err < 0) {
    printf ("Error: Failed to set socket options\n");
    goto ExitHandler;
  }

  memset ((char *)&ServerAddr, 0x0, sizeof (ServerAddr));

  ServerAddr.sin_family      = AF_INET;
  ServerAddr.sin_addr.s_addr = INADDR_ANY;
  ServerAddr.sin_port        = htons (PortNo);
  Err                        = bind (
                                 SockFd,
                                 (struct sockaddr *)&ServerAddr,
                                 sizeof (ServerAddr)
                                 );
  if (Err < 0) {
    printf ("Error: Failed to bind socket\n");
    goto ExitHandler;
  }

  *ServerSockFd = SockFd;
  Err           = pthread_create (&ThreadId, NULL, ServerProc, ServerSockFd);
  if (Err != 0) {
    printf ("Error: Failed to create ServerProc thread :[%s]", strerror (Err));
    // return -1 as pthread_create returns 0 on success and
    // on error, it returns an error number (which may be positive or negative).
    return -1;
  }

  return Err;

ExitHandler:
  shutdown (SockFd, SHUT_RDWR);
  close (SockFd);
  *ServerSockFd = -1;
  return Err;
}

/**
  Entrypoint for the Test User Context service.

  @param[in] argc         Number of input arguments.
  @param[in] argv         Input arguments.

  @retval    0             Success.
            -1             Failure
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  EFI_STATUS  Status;
  int         ServerSockFd;
  int         PortNo;

  printf ("Test User Context Service.\n");

  if (argc < 2) {
    printf ("Error: Service port number not provided\n");
    return -1;
  }

  PortNo = atoi (argv[1]);
  printf ("Info: Service port number: %d\n", PortNo);

  Status = ArmCcaBootSyncCryptoInit ();
  if (EFI_ERROR (Status)) {
    printf (
      "Error: Failed to init Crypto interfaces!, Status = 0x%x\n",
      Status
      );
    return -1;
  }

  if (CreateServer (PortNo, &ServerSockFd) < 0) {
    printf ("Error: Failed to create server socket\n");
    return -1;
  }

  printf ("Info: Server scoket created fd = %d\n", ServerSockFd);

  printf ("Info: PRESS x + Enter to stop server!\n");
  while (getchar () != 'x') {
    printf ("*");
  }

  gExitLoop = TRUE;

  printf ("Info: Shutting down.\n");
  // Shutdown and close the server socket.
  shutdown (ServerSockFd, SHUT_RDWR);
  close (ServerSockFd);

  return 0;
}
