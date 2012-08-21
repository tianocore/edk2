/** @file
  Xen PV test Dxe.

  Copyright (c) 2011-2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _HYPERCALL_TEST_DXE_H_
#define _HYPERCALL_TEST_DXE_H_

#include <PiDxe.h>

#include <Library/XenHypercallLib.h>
#include <Library/XenLib.h>
#include <Library/XenGrantTableLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Guid/XenInfo.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>


///
/// Front block driver for OVMF
///
//TODO move to blkfront.h/***Defined in the blkfront.h***/

typedef UINT16  BLKIF_VDEV;
typedef UINT64  BLKIF_SECTOR;

//
// Maximum scatter/gather segments per request.
// This is carefully chosen so that sizeof(BLKIF_RING) <= PAGE_SIZE.
// NB. This could be 12 if the ring indexes weren't stored in the same page.
//
#define BLKIF_MAX_SEGMENTS_PER_REQUEST 11

//
// NB. first_sect and last_sect in blkif_request_segment, as well as
// sector_number in blkif_request, are always expressed in 512-byte units.
// However they must be properly aligned to the real sector size of the
// physical disk, which is reported in the "sector-size" node in the backend
// xenbus info. Also the xenbus "sectors" node is expressed in 512-byte units.
//
typedef struct {
  //
  // Reference to I/O buffer frame
  //
  XEN_GRANT_REF            Gref;
  //
  // FirstSect: first sector in frame to transfer (inclusive).
  // LastSect:  last sector in frame to transfer (inclusive).
  //
  UINT8                    FirstSect;
  UINT8                    LastSect;
} BLKIF_REQUEST_SEGMENT;

//
// Starting ring element for any I/O request.
//
typedef struct {
  //
  // BLKIF_OP_XXX
  //
  UINT8                    Operation;
  //
  // Number of segments
  //
  UINT8                    NrSegments;
  //
  // Only for read/write requests
  //
  BLKIF_VDEV               Handle;
  //
  // Private guest value, echoed in resp
  //
  UINT64                   Id;
  //
  // Start sector idx on disk (r/w only)
  //
  BLKIF_SECTOR             SectorNumber;
  BLKIF_REQUEST_SEGMENT    Seg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
} BLKIF_REQUEST;

typedef struct {
  //
  // Copy from reques
  //
  UINT64                   Id;
  //
  // Copied from request
  //
  UINT8                    Operation;
  //
  // BLKIF_RSP_XXX
  //
  UINT16                   Status;
} BLKIF_RESPONSE;


//
// Generate BLKIF ring structures and types
// We get struct BLKIF_SRING_ENTRY, BLKIF_SRING, BLKIF_FRONT_RING
//
DEFINE_RING_TYPES(BLKIF, BLKIF_REQUEST, BLKIF_RESPONSE);

typedef struct {
  UINTN                    Sectors;
  UINT32                   SectorSize;
  UINTN                    Mode;
  UINTN                    Info;
  UINTN                    Barrier;
  UINTN                    Flush;
} BLK_FRONT_INFO;



//TODO move to blkfront.c/***Defined in the blkfront.c***/
typedef enum {
  BLKIF_STATE_DISCONNECTED,
  BLKIF_STATE_CONNECTED,
  BLKIF_STATE_SUSPENDED
} EFI_BLKIF_STATE;

typedef struct {
  VOID                     *Page;
  XEN_GRANT_REF            Gref;
} BLK_BUFFER;

typedef struct {
  DOMID                    DomId;

  BLKIF_FRONT_RING         Ring;
  XEN_GRANT_REF            RingRef;
  EFI_EVENT_CHANNEL_PORT   EvtChn;
  BLKIF_VDEV               Handle;

  CHAR8                    *NodeName;
  CHAR8                    *Backend;
  BLK_FRONT_INFO           Info;

  XENBUS_EVENT_QUEUE       Events;
  //UINTN                    Fd;
} BLK_FRONT_DEV;


#endif

