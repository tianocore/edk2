/*++

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Handle.c

Abstract:

  Support for Handle lib fucntions.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"

EFI_STATUS
EfiLibLocateHandleProtocolByProtocols (
  IN OUT EFI_HANDLE        * Handle, OPTIONAL
  OUT    VOID              **Interface, OPTIONAL
  ...
  )
/*++
Routine Description:

  Function locates Protocol and/or Handle on which all Protocols specified
  as a variable list are installed.
  It supports continued search. The caller must assure that no handles are added
  or removed while performing continued search, by e.g., rising the TPL and not
  calling any handle routines. Otherwise the behavior is undefined.

Arguments:

  Handle        - The address of handle to receive the handle on which protocols
                  indicated by the variable list are installed.
                  If points to NULL, all handles are searched. If pointing to a
                  handle returned from previous call, searches starting from next handle.
                  If NULL, the parameter is ignored.

  Interface     - The address of a pointer to a protocol interface that will receive
                  the interface indicated by first variable argument.
                  If NULL, the parameter is ignored.

  ...           - A variable argument list containing protocol GUIDs. Must end with NULL.

Returns:

  EFI_SUCCESS  - All the protocols where found on same handle.
  EFI_NOT_FOUND - A Handle with all the protocols installed was not found.
  Other values as may be returned from LocateHandleBuffer() or HandleProtocol().

--*/
{
  VA_LIST     args;
  EFI_STATUS  Status;
  EFI_GUID    *Protocol;
  EFI_GUID    *ProtocolFirst;
  EFI_HANDLE  *HandleBuffer;
  UINTN       NumberOfHandles;
  UINTN       Idx;
  VOID        *AnInterface;

  AnInterface = NULL;
  VA_START (args, Interface);
  ProtocolFirst = VA_ARG (args, EFI_GUID *);
  VA_END (args);

  //
  // Get list of all handles that support the first protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  ProtocolFirst,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if this is a countinuation of handle searching.
  //
  Idx = 0;
  if ((Handle != NULL) && (*Handle != NULL)) {
    //
    // Leave the Idx just beyond the matching handle.
    //
    for (; Idx < NumberOfHandles;) {
      if (*Handle == HandleBuffer[Idx++]) {
        break;
      }
    }
  }

  //
  // Iterate handles testing for presence of remaining protocols.
  //
  for (; Idx < NumberOfHandles; Idx++) {

    //
    // Start with the second protocol, the first one is sure on this handle.
    //
    VA_START (args, Interface);
    Protocol = VA_ARG (args, EFI_GUID *);

    //
    // Iterate protocols from the variable list.
    //
    while (TRUE) {

      Protocol = VA_ARG (args, EFI_GUID *);

      if (Protocol == NULL) {

        //
        // If here, the list was iterated successfully
        // finding each protocol on a single handle.
        //

        Status = EFI_SUCCESS;

        //
        // OPTIONAL parameter returning the Handle.
        //
        if (Handle != NULL) {
          *Handle = HandleBuffer[Idx];
        }

        //
        // OPTIONAL parameter returning the first rotocol's Interface.
        //
        if (Interface != NULL) {
          Status = gBS->HandleProtocol (
                          HandleBuffer[Idx],
                          ProtocolFirst,
                          Interface
                          );
        }

        VA_END (args);
        
        goto lbl_out;
      }

      Status = gBS->HandleProtocol (
                      HandleBuffer[Idx],
                      Protocol,
                      &AnInterface
                      );
      if (EFI_ERROR (Status)) {

        //
        // This handle does not have the iterated protocol.
        //
        break;
      }
    }

    VA_END (args);
  }

  //
  // If here, no handle that bears all the protocols was found.
  //
  Status = EFI_NOT_FOUND;

lbl_out:
  gBS->FreePool (HandleBuffer);
  return Status;
}
