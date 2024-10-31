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

#include <Base.h>
#include <Library/ArmCcaBootSyncCryptoLib.h>
#include <Uefi/UefiBaseType.h>

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

  printf ("Info: Shutting down.\n");
  // Shutdown and close the server socket.
  shutdown (ServerSockFd, SHUT_RDWR);
  close (ServerSockFd);

  return 0;
}
