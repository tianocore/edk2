/** @file
  Implement the poll API.

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <SocketInternals.h>


/** Poll the socket for activity

  @param [in] pDescriptor Descriptor address for the file
  @param [in] Events      Mask of events to detect

  @return     Detected events for the socket
 **/
short
EFIAPI
BslSocketPoll (
  IN struct __filedes * pDescriptor,
  IN short Events
  )
{
  short                 DetectedEvents;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;

  //  Locate the socket protocol
  DetectedEvents = 0;
  pSocketProtocol = BslValidateSocketFd ( pDescriptor, &errno );
  if ( NULL != pSocketProtocol ) {
    //  Poll the socket
    (void) pSocketProtocol->pfnPoll ( pSocketProtocol,
                                      Events,
                                      &DetectedEvents,
                                      &errno );
  }
  //  Return the detected events
  return DetectedEvents;
}
