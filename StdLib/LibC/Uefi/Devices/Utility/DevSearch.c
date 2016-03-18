/** @file
    Device Abstraction: Search device list for a matching device.

    Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/BaseMemoryLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <kfile.h>
#include  <Device/Device.h>
#include  <MainData.h>

/** Find a DeviceNode matching DevName or DevProto, or both.

    If DevName is NULL, then the device name is not used in the search.
    If DevProto is NULL, then the protocol GUID is not used in the search.
    If both are NULL, then INVALID_PARAMETER is returned.
    If both DevName and DevProto are specified, then both must match.
    If both are specified but only one matches, then DEVICE_ERROR is returned.

    @param  DevName     Name of the Device Abstraction to find.
    @param  DevProto    GUID of the Protocol associated with the Device Abstraction.
    @param  Node        Pointer to the pointer to receive the found Device Node's address.

    @retval RETURN_SUCCESS              The desired Device Node was found.
    @retval RETURN_INVALID_PARAMETER    Both DevName and DevProto are NULL or Node is NULL.
    @retval RETURN_DEVICE_ERROR         DevName matched but DevProto did not.
    @retval RETURN_NOT_FOUND            The desired device was not found.
**/
EFI_STATUS
EFIAPI
__DevSearch(
  IN     CHAR16        *DevName,
  IN     GUID          *DevProto,
     OUT DeviceNode   **Node
  )
{
  RETURN_STATUS     Status = RETURN_NOT_FOUND;
  DeviceNode       *WorkNode;
  INT32             DevMatched;

  if(((DevName == NULL) && (DevProto == NULL)) || (Node == NULL)) {
    Status = RETURN_INVALID_PARAMETER;
  }
  else {
    if(IsListEmpty((LIST_ENTRY *)&daDeviceList)) {
      Status = RETURN_NOT_FOUND;
    }
    else {
      /* Traverse the list of Device Nodes hunting for a match */
      WorkNode = (DeviceNode *)GetFirstNode((LIST_ENTRY *)&daDeviceList);
      do {
        /* Use DevMatched to keep track of the three match conditions. */
        DevMatched = 0;
        if(DevName != NULL) {
          ++DevMatched;
          if(wcsncmp(DevName, WorkNode->DevName, wcslen(WorkNode->DevName)) == 0) {
            ++DevMatched;
          }
        }
        /* At this point, DevMatched has one of the following values:
              0   DevName == NULL, no name comparison
              1   DevName did not match WorkNode's name
              2   DevName MATCHED
        */
        if((DevMatched != 1) && (DevProto != NULL) && (WorkNode->DevProto != NULL)) {
          /*  Only bother with the GUID comparison if:
                * There was NOT a name mismatch
                * DevProto is NOT NULL -- there is a GUID to compare
                * WorkNode->DevProto is NOT NULL
          */
          if(CompareGuid(DevProto, WorkNode->DevProto)) {
            // GUIDs matched, we found it
            Status = RETURN_SUCCESS;
            *Node = WorkNode;
            break;
          }
          else {
            // GUIDs did not match
            if(DevMatched == 2) {
              // Name matched, GUID did not!
              Status = RETURN_DEVICE_ERROR;
              break;    // Don't try any more, we have an internal problem
            }
          }
        }
        else {
          if(DevMatched == 2) {
            // Device Name matched, GUIDs skipped
            Status = RETURN_SUCCESS;
            *Node = WorkNode;
            break;
          }
        }
        // Check the next device in the list
        WorkNode = (DeviceNode *)GetNextNode(&daDeviceList, (LIST_ENTRY *)WorkNode);
      } while(&daDeviceList != (LIST_ENTRY *)WorkNode);
    }
  }
  return Status;
}
