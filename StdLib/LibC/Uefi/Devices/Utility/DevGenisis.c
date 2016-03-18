/** @file
    Device Abstraction: device creation utility functions.

    Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/MemoryAllocationLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <stdarg.h>
#include  <sys/poll.h>
#include  <kfile.h>
#include  <Device/Device.h>
#include  <MainData.h>

LIST_ENTRY    daDeviceList    = INITIALIZE_LIST_HEAD_VARIABLE(daDeviceList);
DeviceNode   *daDefaultDevice = NULL;     ///< Device to use if nothing else found
DeviceNode   *daRootDevice    = NULL;     ///< Device containing the root file system
DeviceNode   *daCurrentDevice = NULL;     ///< Device currently being accessed

/* Commonly used fileops
      fnullop_*   Does nothing and returns success.
      fbadop_*    Does nothing and returns EPERM
*/
int     EFIAPI fnullop_fcntl (struct __filedes *filp, UINT32 Cmd, void *p3, void *p4)
{ return 0; }

short  EFIAPI fnullop_poll  (struct __filedes *filp, short Events)
{
  return ((POLLIN | POLLRDNORM | POLLOUT) & Events);
}

int     EFIAPI fnullop_flush (struct __filedes *filp)
{ return 0; }

int     EFIAPI fbadop_stat   (struct __filedes *filp, struct stat *StatBuf, void *Buf)
{
  errno = EPERM;
  return -1;
}

int     EFIAPI fbadop_ioctl  (struct __filedes *filp, ULONGN Cmd, va_list argp)
{
  errno = EPERM;
  return -1;
}

int     EFIAPI fbadop_delete (struct __filedes *filp)
{
  errno = EPERM;
  return -1;
}

int     EFIAPI fbadop_mkdir  (const char *path, __mode_t perms)
{
  errno = EPERM;
  return -1;
}

int     EFIAPI fbadop_rename   (const char *from, const char *to)
{
  errno = EPERM;
  return -1;
}

int     EFIAPI fbadop_rmdir    (struct __filedes *filp)
{
  errno = EPERM;
  return -1;
}

/** Add a new device to the device list.
    If both DevName and DevProto are NULL, register this as the Default device.

    @param  DevName       Name of the device to add.
    @param  DevProto      Pointer to the GUID identifying the protocol associated with this device.
                          If DevProto is NULL, startup code will not try to find instances
                          of this device.
    @param  OpenFunc      Pointer to the device's Open function.
    @param  InstanceList  Optional pointer to the device's initialized instance list.
                          If InstanceList is NULL, the application startup code will
                          scan for instances of the protocol identified by DevProto and
                          populate the InstanceList in the order those protocols are found.
    @param  NumInstance   Number of instances in InstanceList.
    @param  Modes         Bit-mapped flags indicating operations (R, W, RW, ...) permitted to this device.

**/
DeviceNode *
EFIAPI
__DevRegister(
  IN const CHAR16          *DevName,
  IN GUID                  *DevProto,
  IN FO_OPEN                OpenFunc,
  IN void                  *InstanceList,
  IN int                    NumInstance,
  IN UINT32                 InstanceSize,
  IN UINT32                 Modes
  )
{
  DeviceNode         *Node;
  GenericInstance    *GIp;
  char               *GenPtr;
  int                 i;

  /* Validate parameters */
  if(((DevName == NULL) && (DevProto != NULL)) ||
      (OpenFunc == NULL)) {
    EFIerrno = RETURN_INVALID_PARAMETER;
    return NULL;
  }
  Node = (DeviceNode *)AllocateZeroPool(sizeof(DeviceNode));
  if(Node == NULL) {
    EFIerrno = RETURN_OUT_OF_RESOURCES;
    return NULL;
  }

  Node->DevName       = DevName;
  Node->DevProto      = DevProto;
  Node->InstanceList  = InstanceList;
  Node->OpenFunc      = OpenFunc;
  Node->InstanceSize  = InstanceSize;
  Node->NumInstances  = NumInstance;
  Node->OpModes       = Modes;

  /* Update the Parent member of each element of the InstanceList */
  if(InstanceList != NULL) {
    GenPtr = InstanceList;

    for(i = 0; i < NumInstance; ++i) {    // Iterate through each element of InstanceList
      GIp = (GenericInstance *)GenPtr;
      GIp->Parent = Node;                     // Initializing the Parent member & InstanceNum
      //GIp->InstanceNum = i;
      GenPtr += InstanceSize;
    }
  }
  if(DevName == NULL) {
    if(daDefaultDevice != NULL) {
      EFIerrno = RETURN_INVALID_PARAMETER;
      return NULL;
    }
    daDefaultDevice = Node;
  }
  else {
    (void) InsertTailList(&daDeviceList, &Node->DevList);
  }
  EFIerrno = RETURN_SUCCESS;
  return Node;
}
