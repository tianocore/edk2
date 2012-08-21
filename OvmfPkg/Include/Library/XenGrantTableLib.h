/** @file

  The XenGrantTableLib library provides the operations for consumers to use the Xen grant table.

  Copyright (c) 2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __XEN_GRANT_TABLE_LIB__
#define __XEN_GRANT_TABLE_LIB__

#include <Library/XenLib.h>

///
/// Reference to a grant entry in a specified domain's grant table.
///
typedef UINT32 XEN_GRANT_REF;  //typedef uint32_t grant_ref_t;
///
/// Handle to track a mapping created via a grant reference.
///
typedef UINT32 XEN_GRANT_HANDLE;

typedef UINTN  PG_ENTRY;

//
// Grant table status returns value.
//
#define GNTST_OKAY              0
#define GNTST_GENERAL_ERROR     -1
#define GNTST_BAD_DOMAIN        -2
#define GNTST_BAD_GNTREF        -3
#define GNTST_BAD_HANDLE        -4
#define GNTST_BAD_VIRT_ADDR     -5
#define GNTST_BAD_DEV_ADDR      -6
#define GNTST_NO_DEVICE_SPACE   -7
#define GNTST_PERMISSION_DENIED -8
#define GNTST_BAD_PAGE          -9
#define GNTST_BAD_COPY_ARG      -10
#define GNTST_ADDRESS_TOO_BIG   -11
#define GNTST_EAGAIN            -12

///
/// Type of grant entry.
/// GTF_INVALID: This grant entry grants no privileges.
/// GTF_PERMIT_ACCESS: Allow @DomId to map/access @Frame.
/// GTF_ACCEPT_TRANSFER: Allow @DomId to transfer ownership of one page frame
///                     to this guest. Xen writes the page number to @Frame.
/// GTF_TRANSITIVE: Allow @DomId to transitively access a subrange of
///                @TransGrant in @TransDomid. No mappings are allowed.
///
#define GTF_INVALID         	  (0U<<0)
#define GTF_PERMIT_ACCESS         (1U<<0)
#define GTF_ACCEPT_TRANSFER       (2U<<0)
#define GTF_TRANSITIVE            (3U<<0)
#define GTF_TYPE_MASK             (3U<<0)

///
/// Subflags for GTF_permit_access.
/// GTF_READONLY: Restrict @DomId to read-only mappings and accesses. [GST]
/// GTF_READING: Grant entry is currently mapped for reading by @DomId. [XEN]
/// GTF_WRITING: Grant entry is currently mapped for writing by @DomId. [XEN]
/// GTF_PAT, GTF_PWT, GTF_PCD: (x86) cache attribute flags for the grant [GST]
/// GTF_SUB_PAGE: Grant access to only a subrange of the page.  @DomId
///               will only be allowed to copy from the grant, and not
///               map it. [GST]
/// 
#define _GTF_READONLY       (2)
#define GTF_READONLY        (1U<<_GTF_READONLY)
#define _GTF_READING        (3)
#define GTF_READING         (1U<<_GTF_READING)
#define _GTF_WRITING        (4)
#define GTF_WRITING         (1U<<_GTF_WRITING)
#define _GTF_PWT            (5)
#define GTF_PWT             (1U<<_GTF_PWT)
#define _GTF_PCD            (6)
#define GTF_PCD             (1U<<_GTF_PCD)
#define _GTF_PAT            (7)
#define GTF_PAT             (1U<<_GTF_PAT)
#define _GTF_SUB_PAGE       (8)
#define GTF_SUB_PAGE        (1U<<_GTF_SUB_PAGE)

///
/// grant_entry_v1 in Xen (xen/include/public)
/// A grant table comprises a packed array of grant entries in one or more
/// page frames shared between Xen and a guest.
/// [XEN]: This field is written by Xen and read by the sharing guest.
/// [GST]: This field is written by the guest and read by Xen.
///
typedef struct {
  //
  // Type of grant entry: GFT_XXX. [XEN, GST]
  //
  UINT32          Flags;  //UINT16          Flags;
  //
  // The domain being granted foreign privileges. [GST]
  //
  DOMID           DomId;
  //
  // GTF_permit_access: Frame that @DomId is allowed to map and access. [GST]
  // GTF_accept_transfer: Frame whose ownership transferred by @DomId. [XEN]
  //
  UINT32          Frame;
} GRANT_ENTRY;


///
/// Grant Table queries and uses
/// Prepare for using Hypercall #20: HypervisorGrantTableOp(
///                                    IN UINTN      Cmd,
///                                    IN VOID       *Uop,
///                                    IN UINTN      Count
///                                  );
/// @param IN VOID *Uop: Point to an array of a per-command data structure
/// @param IN UINTN Count: Number of elements in the array 
///
#define GNTTABOP_MAP_GRANT_REF        0
#define GNTTABOP_UNMAP_GRANT_REF      1
#define GNTTABOP_SETUP_TABLE          2
#define GNTTABOP_DUMP_TABLE           3
#define GNTTABOP_TRANSFER             4
#define GNTTABOP_COPY                 5
#define GNTTABOP_QUERY_SIZE           6
#define GNTTABOP_UNMAP_AND_REPLACE    7
//
// Num 8~11: Don't fit for the lagacy interface.
//
#define GNTTABOP_SET_VERSION          8
#define GNTTABOP_GET_STATUS_FRAMES    9
#define GNTTABOP_GET_VERSION          10
#define GNTTABOP_SWAP_GRANT_REF	      11

//
// GNTTABOP_MAP_GRANT_REF: Map the grant entry (<dom>,<ref>) for access
//   by devices and/or host CPUs. If successful, <handle> is a tracking number
//   that must be presented later to destroy the mapping(s). On error, <handle>
//   is a negative status code.
// NOTES:
//   1. If GNTMAP_device_map is specified then <dev_bus_addr> is the address
//      via which I/O devices may access the granted frame.
//   2. If GNTMAP_host_map is specified then a mapping will be added at
//      either a host virtual address in the current address space, or at
//      a PTE at the specified machine address.  The type of mapping to
//      perform is selected through the GNTMAP_contains_pte flag, and the 
//      address is specified in <host_addr>.
//   3. Mappings should only be destroyed via GNTTABOP_unmap_grant_ref. If a
//      host mapping is destroyed by other means then it is *NOT* guaranteed
//      to be accounted to the correct grant reference!
// 
// #define GNTTABOP_MAP_GRANT_REF        0
//
typedef struct {
  //
  // IN parameters
  //
  UINT64             HostAddr;
  UINT32             Flags;
  XEN_GRANT_REF      Ref;
  DOMID              Dom;
  //
  // OUT parameters
  //
  INT16              Status;
  XEN_GRANT_HANDLE   Handle;
  UINT64             DevBusAddr;
} GRANTTABLE_MAP_GRANT_REF;

//
// GNTTABOP_UNMAP_GRANT_REF: Destroy one or more grant-reference mappings
//   tracked by <handle>. If <host_addr> or <dev_bus_addr> is zero, that
//   field is ignored. If non-zero, they must refer to a device/host mapping
//   that is tracked by <handle>
// NOTES:
//   1. The call may fail in an undefined manner if either mapping is not
//      tracked by <handle>.
//   3. After executing a batch of unmaps, it is guaranteed that no stale
//      mappings will remain in the device or host TLBs.
//
// #define GNTTABOP_UNMAP_GRANT_REF      1
typedef struct {
  //
  // IN parameters
  //
  UINT64             HostAddr;
  UINT64             DevBusAddr;
  XEN_GRANT_HANDLE   Handle;
  //
  // OUT parameters
  //
  INT16              Status;
} GRANTTABLE_UNMAP_GRANT_REF;

//
// GNTTABOP_SETUP_TABLE: Set up a grant table for Dom comprising at least NrFrames pages.
//   The frame addresses are written to the FrameList. Only NrFrames addresses are written, 
//   even if the table is larger. 
// Note: 
//   Xen may not support more than a single grant-table page per domain.
// 
// #define GNTTABOP_SETUP_TABLE          2
//
typedef struct {
  //
  // IN parameters
  //
  DOMID           Dom;
  UINT32          NrFrames;
  //
  // OUT parameters
  //
  INT16           Status;
  UINTN           FrameList;  //XEN_GUEST_HANDLE(ulong) frame_list;?
} GRANTTABLE_SETUP_TABLE;

///
/// Operations for Grant Table.
///
/**
  Init the grant table
**/
VOID
EFIAPI
InitGrantTable (
  VOID
  );

/**
  ...

  @param IN MAP     ...

  @return  .

**/
XEN_GRANT_REF
EFIAPI
GrantTableAllocateAndGrant (
  IN     VOID                     **MAP
  );

/**
  ...

  @param IN DomId     ...
  @param IN Frame     ...
  @param IN ReadOnly     ...

  @return  .
  TODO Need to implement now
**/
XEN_GRANT_REF
EFIAPI
GrantTableGrantAccess (
  IN     DOMID                    DomId,
  IN     UINTN                    Frame,
  IN     UINT32                   ReadOnly
  );

/**
  ...

  @param IN Ref     ...

  @return  .
  TODO Need to implement now
**/
UINT32
EFIAPI
GrantTableEndAccess (
  IN     XEN_GRANT_REF            Ref
  );

/**
  ...

  @param IN DomId     ...
  @param IN Pfn     ...

  @return  .

**/
XEN_GRANT_REF
EFIAPI
GrantTableGrantTransfer (
  IN     DOMID                    DomId,
  IN     UINTN                    Pfn
  );

/**
  ...

  @param IN Ref     ...

  @return  .

**/
UINTN
EFIAPI
GrantTableEndTransfer (
  IN     XEN_GRANT_REF            Ref
  );

/**
  ...

  @param IN Status     ...

  @return  .

**/
CHAR8 *
EFIAPI
GrantTableOperateError (
  IN     UINT16                   Status
  );

/**
  ...
**/
VOID
EFIAPI
FinishGrantTable (
  VOID
  );

#endif  //__XEN_GRANT_TABLE_LIB__

