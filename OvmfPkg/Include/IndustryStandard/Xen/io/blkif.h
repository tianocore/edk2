/******************************************************************************
 * blkif.h
 *
 * Unified block-device I/O interface for Xen guest OSes.
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2003-2004, Keir Fraser
 * Copyright (c) 2012, Spectra Logic Corporation
 */

#ifndef __XEN_PUBLIC_IO_BLKIF_H__
#define __XEN_PUBLIC_IO_BLKIF_H__

#include "ring.h"
#include "../grant_table.h"

/*
 * Front->back notifications: When enqueuing a new request, sending a
 * notification can be made conditional on req_event (i.e., the generic
 * hold-off mechanism provided by the ring macros). Backends must set
 * req_event appropriately (e.g., using RING_FINAL_CHECK_FOR_REQUESTS()).
 *
 * Back->front notifications: When enqueuing a new response, sending a
 * notification can be made conditional on rsp_event (i.e., the generic
 * hold-off mechanism provided by the ring macros). Frontends must set
 * rsp_event appropriately (e.g., using RING_FINAL_CHECK_FOR_RESPONSES()).
 */

#ifndef blkif_vdev_t
#define blkif_vdev_t   UINT16
#endif
#define blkif_sector_t UINT64

/*
 * Feature and Parameter Negotiation
 * =================================
 * The two halves of a Xen block driver utilize nodes within the XenStore to
 * communicate capabilities and to negotiate operating parameters.  This
 * section enumerates these nodes which reside in the respective front and
 * backend portions of the XenStore, following the XenBus convention.
 *
 * All data in the XenStore is stored as strings.  Nodes specifying numeric
 * values are encoded in decimal.  Integer value ranges listed below are
 * expressed as fixed sized integer types capable of storing the conversion
 * of a properly formatted node string, without loss of information.
 *
 * Any specified default value is in effect if the corresponding XenBus node
 * is not present in the XenStore.
 *
 * XenStore nodes in sections marked "PRIVATE" are solely for use by the
 * driver side whose XenBus tree contains them.
 *
 * XenStore nodes marked "DEPRECATED" in their notes section should only be
 * used to provide interoperability with legacy implementations.
 *
 * See the XenBus state transition diagram below for details on when XenBus
 * nodes must be published and when they can be queried.
 *
 *****************************************************************************
 *                            Backend XenBus Nodes
 *****************************************************************************
 *
 *------------------ Backend Device Identification (PRIVATE) ------------------
 *
 * mode
 *      Values:         "r" (read only), "w" (writable)
 *
 *      The read or write access permissions to the backing store to be
 *      granted to the frontend.
 *
 * params
 *      Values:         string
 *
 *      A free formatted string providing sufficient information for the
 *      backend driver to open the backing device.  (e.g. the path to the
 *      file or block device representing the backing store.)
 *
 * type
 *      Values:         "file", "phy", "tap"
 *
 *      The type of the backing device/object.
 *
 *--------------------------------- Features ---------------------------------
 *
 * feature-barrier
 *      Values:         0/1 (boolean)
 *      Default Value:  0
 *
 *      A value of "1" indicates that the backend can process requests
 *      containing the BLKIF_OP_WRITE_BARRIER request opcode.  Requests
 *      of this type may still be returned at any time with the
 *      BLKIF_RSP_EOPNOTSUPP result code.
 *
 * feature-flush-cache
 *      Values:         0/1 (boolean)
 *      Default Value:  0
 *
 *      A value of "1" indicates that the backend can process requests
 *      containing the BLKIF_OP_FLUSH_DISKCACHE request opcode.  Requests
 *      of this type may still be returned at any time with the
 *      BLKIF_RSP_EOPNOTSUPP result code.
 *
 * feature-discard
 *      Values:         0/1 (boolean)
 *      Default Value:  0
 *
 *      A value of "1" indicates that the backend can process requests
 *      containing the BLKIF_OP_DISCARD request opcode.  Requests
 *      of this type may still be returned at any time with the
 *      BLKIF_RSP_EOPNOTSUPP result code.
 *
 * feature-persistent
 *      Values:         0/1 (boolean)
 *      Default Value:  0
 *      Notes: 7
 *
 *      A value of "1" indicates that the backend can keep the grants used
 *      by the frontend driver mapped, so the same set of grants should be
 *      used in all transactions. The maximum number of grants the backend
 *      can map persistently depends on the implementation, but ideally it
 *      should be RING_SIZE * BLKIF_MAX_SEGMENTS_PER_REQUEST. Using this
 *      feature the backend doesn't need to unmap each grant, preventing
 *      costly TLB flushes. The backend driver should only map grants
 *      persistently if the frontend supports it. If a backend driver chooses
 *      to use the persistent protocol when the frontend doesn't support it,
 *      it will probably hit the maximum number of persistently mapped grants
 *      (due to the fact that the frontend won't be reusing the same grants),
 *      and fall back to non-persistent mode. Backend implementations may
 *      shrink or expand the number of persistently mapped grants without
 *      notifying the frontend depending on memory constraints (this might
 *      cause a performance degradation).
 *
 *      If a backend driver wants to limit the maximum number of persistently
 *      mapped grants to a value less than RING_SIZE *
 *      BLKIF_MAX_SEGMENTS_PER_REQUEST a LRU strategy should be used to
 *      discard the grants that are less commonly used. Using a LRU in the
 *      backend driver paired with a LIFO queue in the frontend will
 *      allow us to have better performance in this scenario.
 *
 *----------------------- Request Transport Parameters ------------------------
 *
 * max-ring-page-order
 *      Values:         <UINT32>
 *      Default Value:  0
 *      Notes:          1, 3
 *
 *      The maximum supported size of the request ring buffer in units of
 *      lb(machine pages). (e.g. 0 == 1 page,  1 = 2 pages, 2 == 4 pages,
 *      etc.).
 *
 * max-ring-pages
 *      Values:         <UINT32>
 *      Default Value:  1
 *      Notes:          DEPRECATED, 2, 3
 *
 *      The maximum supported size of the request ring buffer in units of
 *      machine pages.  The value must be a power of 2.
 *
 *------------------------- Backend Device Properties -------------------------
 *
 * discard-alignment
 *      Values:         <UINT32>
 *      Default Value:  0
 *      Notes:          4, 5
 *
 *      The offset, in bytes from the beginning of the virtual block device,
 *      to the first, addressable, discard extent on the underlying device.
 *
 * discard-granularity
 *      Values:         <UINT32>
 *      Default Value:  <"sector-size">
 *      Notes:          4
 *
 *      The size, in bytes, of the individually addressable discard extents
 *      of the underlying device.
 *
 * discard-secure
 *      Values:         0/1 (boolean)
 *      Default Value:  0
 *      Notes:          10
 *
 *      A value of "1" indicates that the backend can process BLKIF_OP_DISCARD
 *      requests with the BLKIF_DISCARD_SECURE flag set.
 *
 * info
 *      Values:         <UINT32> (bitmap)
 *
 *      A collection of bit flags describing attributes of the backing
 *      device.  The VDISK_* macros define the meaning of each bit
 *      location.
 *
 * sector-size
 *      Values:         <UINT32>
 *
 *      The logical sector size, in bytes, of the backend device.
 *
 * physical-sector-size
 *      Values:         <UINT32>
 *
 *      The physical sector size, in bytes, of the backend device.
 *
 * sectors
 *      Values:         <UINT64>
 *
 *      The size of the backend device, expressed in units of its logical
 *      sector size ("sector-size").
 *
 *****************************************************************************
 *                            Frontend XenBus Nodes
 *****************************************************************************
 *
 *----------------------- Request Transport Parameters -----------------------
 *
 * event-channel
 *      Values:         <UINT32>
 *
 *      The identifier of the Xen event channel used to signal activity
 *      in the ring buffer.
 *
 * ring-ref
 *      Values:         <UINT32>
 *      Notes:          6
 *
 *      The Xen grant reference granting permission for the backend to map
 *      the sole page in a single page sized ring buffer.
 *
 * ring-ref%u
 *      Values:         <UINT32>
 *      Notes:          6
 *
 *      For a frontend providing a multi-page ring, a "number of ring pages"
 *      sized list of nodes, each containing a Xen grant reference granting
 *      permission for the backend to map the page of the ring located
 *      at page index "%u".  Page indexes are zero based.
 *
 * protocol
 *      Values:         string (XEN_IO_PROTO_ABI_*)
 *      Default Value:  XEN_IO_PROTO_ABI_NATIVE
 *
 *      The machine ABI rules governing the format of all ring request and
 *      response structures.
 *
 * ring-page-order
 *      Values:         <UINT32>
 *      Default Value:  0
 *      Maximum Value:  MAX(ffs(max-ring-pages) - 1, max-ring-page-order)
 *      Notes:          1, 3
 *
 *      The size of the frontend allocated request ring buffer in units
 *      of lb(machine pages). (e.g. 0 == 1 page, 1 = 2 pages, 2 == 4 pages,
 *      etc.).
 *
 * num-ring-pages
 *      Values:         <UINT32>
 *      Default Value:  1
 *      Maximum Value:  MAX(max-ring-pages,(0x1 << max-ring-page-order))
 *      Notes:          DEPRECATED, 2, 3
 *
 *      The size of the frontend allocated request ring buffer in units of
 *      machine pages.  The value must be a power of 2.
 *
 * feature-persistent
 *      Values:         0/1 (boolean)
 *      Default Value:  0
 *      Notes: 7, 8, 9
 *
 *      A value of "1" indicates that the frontend will reuse the same grants
 *      for all transactions, allowing the backend to map them with write
 *      access (even when it should be read-only). If the frontend hits the
 *      maximum number of allowed persistently mapped grants, it can fallback
 *      to non persistent mode. This will cause a performance degradation,
 *      since the backend driver will still try to map those grants
 *      persistently. Since the persistent grants protocol is compatible with
 *      the previous protocol, a frontend driver can choose to work in
 *      persistent mode even when the backend doesn't support it.
 *
 *      It is recommended that the frontend driver stores the persistently
 *      mapped grants in a LIFO queue, so a subset of all persistently mapped
 *      grants gets used commonly. This is done in case the backend driver
 *      decides to limit the maximum number of persistently mapped grants
 *      to a value less than RING_SIZE * BLKIF_MAX_SEGMENTS_PER_REQUEST.
 *
 *------------------------- Virtual Device Properties -------------------------
 *
 * device-type
 *      Values:         "disk", "cdrom", "floppy", etc.
 *
 * virtual-device
 *      Values:         <UINT32>
 *
 *      A value indicating the physical device to virtualize within the
 *      frontend's domain.  (e.g. "The first ATA disk", "The third SCSI
 *      disk", etc.)
 *
 *      See docs/misc/vbd-interface.txt for details on the format of this
 *      value.
 *
 * Notes
 * -----
 * (1) Multi-page ring buffer scheme first developed in the Citrix XenServer
 *     PV drivers.
 * (2) Multi-page ring buffer scheme first used in some RedHat distributions
 *     including a distribution deployed on certain nodes of the Amazon
 *     EC2 cluster.
 * (3) Support for multi-page ring buffers was implemented independently,
 *     in slightly different forms, by both Citrix and RedHat/Amazon.
 *     For full interoperability, block front and backends should publish
 *     identical ring parameters, adjusted for unit differences, to the
 *     XenStore nodes used in both schemes.
 * (4) Devices that support discard functionality may internally allocate space
 *     (discardable extents) in units that are larger than the exported logical
 *     block size. If the backing device has such discardable extents the
 *     backend should provide both discard-granularity and discard-alignment.
 *     Providing just one of the two may be considered an error by the frontend.
 *     Backends supporting discard should include discard-granularity and
 *     discard-alignment even if it supports discarding individual sectors.
 *     Frontends should assume discard-alignment == 0 and discard-granularity
 *     == sector size if these keys are missing.
 * (5) The discard-alignment parameter allows a physical device to be
 *     partitioned into virtual devices that do not necessarily begin or
 *     end on a discardable extent boundary.
 * (6) When there is only a single page allocated to the request ring,
 *     'ring-ref' is used to communicate the grant reference for this
 *     page to the backend.  When using a multi-page ring, the 'ring-ref'
 *     node is not created.  Instead 'ring-ref0' - 'ring-refN' are used.
 * (7) When using persistent grants data has to be copied from/to the page
 *     where the grant is currently mapped. The overhead of doing this copy
 *     however doesn't suppress the speed improvement of not having to unmap
 *     the grants.
 * (8) The frontend driver has to allow the backend driver to map all grants
 *     with write access, even when they should be mapped read-only, since
 *     further requests may reuse these grants and require write permissions.
 * (9) Linux implementation doesn't have a limit on the maximum number of
 *     grants that can be persistently mapped in the frontend driver, but
 *     due to the frontent driver implementation it should never be bigger
 *     than RING_SIZE * BLKIF_MAX_SEGMENTS_PER_REQUEST.
 *(10) The discard-secure property may be present and will be set to 1 if the
 *     backing device supports secure discard.
 */

/*
 * STATE DIAGRAMS
 *
 *****************************************************************************
 *                                   Startup                                 *
 *****************************************************************************
 *
 * Tool stack creates front and back nodes with state XenbusStateInitialising.
 *
 * Front                                Back
 * =================================    =====================================
 * XenbusStateInitialising              XenbusStateInitialising
 *  o Query virtual device               o Query backend device identification
 *    properties.                          data.
 *  o Setup OS device instance.          o Open and validate backend device.
 *                                       o Publish backend features and
 *                                         transport parameters.
 *                                                      |
 *                                                      |
 *                                                      V
 *                                      XenbusStateInitWait
 *
 * o Query backend features and
 *   transport parameters.
 * o Allocate and initialize the
 *   request ring.
 * o Publish transport parameters
 *   that will be in effect during
 *   this connection.
 *              |
 *              |
 *              V
 * XenbusStateInitialised
 *
 *                                       o Query frontend transport parameters.
 *                                       o Connect to the request ring and
 *                                         event channel.
 *                                       o Publish backend device properties.
 *                                                      |
 *                                                      |
 *                                                      V
 *                                      XenbusStateConnected
 *
 *  o Query backend device properties.
 *  o Finalize OS virtual device
 *    instance.
 *              |
 *              |
 *              V
 * XenbusStateConnected
 *
 * Note: Drivers that do not support any optional features, or the negotiation
 *       of transport parameters, can skip certain states in the state machine:
 *
 *       o A frontend may transition to XenbusStateInitialised without
 *         waiting for the backend to enter XenbusStateInitWait.  In this
 *         case, default transport parameters are in effect and any
 *         transport parameters published by the frontend must contain
 *         their default values.
 *
 *       o A backend may transition to XenbusStateInitialised, bypassing
 *         XenbusStateInitWait, without waiting for the frontend to first
 *         enter the XenbusStateInitialised state.  In this case, default
 *         transport parameters are in effect and any transport parameters
 *         published by the backend must contain their default values.
 *
 *       Drivers that support optional features and/or transport parameter
 *       negotiation must tolerate these additional state transition paths.
 *       In general this means performing the work of any skipped state
 *       transition, if it has not already been performed, in addition to the
 *       work associated with entry into the current state.
 */

/*
 * REQUEST CODES.
 */
#define BLKIF_OP_READ              0
#define BLKIF_OP_WRITE             1
/*
 * All writes issued prior to a request with the BLKIF_OP_WRITE_BARRIER
 * operation code ("barrier request") must be completed prior to the
 * execution of the barrier request.  All writes issued after the barrier
 * request must not execute until after the completion of the barrier request.
 *
 * Optional.  See "feature-barrier" XenBus node documentation above.
 */
#define BLKIF_OP_WRITE_BARRIER     2
/*
 * Commit any uncommitted contents of the backing device's volatile cache
 * to stable storage.
 *
 * Optional.  See "feature-flush-cache" XenBus node documentation above.
 */
#define BLKIF_OP_FLUSH_DISKCACHE   3
/*
 * Used in SLES sources for device specific command packet
 * contained within the request. Reserved for that purpose.
 */
#define BLKIF_OP_RESERVED_1        4
/*
 * Indicate to the backend device that a region of storage is no longer in
 * use, and may be discarded at any time without impact to the client.  If
 * the BLKIF_DISCARD_SECURE flag is set on the request, all copies of the
 * discarded region on the device must be rendered unrecoverable before the
 * command returns.
 *
 * This operation is analogous to performing a trim (ATA) or unamp (SCSI),
 * command on a native device.
 *
 * More information about trim/unmap operations can be found at:
 * http://t13.org/Documents/UploadedDocuments/docs2008/
 *     e07154r6-Data_Set_Management_Proposal_for_ATA-ACS2.doc
 * http://www.seagate.com/staticfiles/support/disc/manuals/
 *     Interface%20manuals/100293068c.pdf
 *
 * Optional.  See "feature-discard", "discard-alignment",
 * "discard-granularity", and "discard-secure" in the XenBus node
 * documentation above.
 */
#define BLKIF_OP_DISCARD           5

/*
 * Recognized if "feature-max-indirect-segments" in present in the backend
 * xenbus info. The "feature-max-indirect-segments" node contains the maximum
 * number of segments allowed by the backend per request. If the node is
 * present, the frontend might use blkif_request_indirect structs in order to
 * issue requests with more than BLKIF_MAX_SEGMENTS_PER_REQUEST (11). The
 * maximum number of indirect segments is fixed by the backend, but the
 * frontend can issue requests with any number of indirect segments as long as
 * it's less than the number provided by the backend. The indirect_grefs field
 * in blkif_request_indirect should be filled by the frontend with the
 * grant references of the pages that are holding the indirect segments.
 * These pages are filled with an array of blkif_request_segment that hold the
 * information about the segments. The number of indirect pages to use is
 * determined by the number of segments an indirect request contains. Every
 * indirect page can contain a maximum of
 * (PAGE_SIZE / sizeof(struct blkif_request_segment)) segments, so to
 * calculate the number of indirect pages to use we have to do
 * ceil(indirect_segments / (PAGE_SIZE / sizeof(struct blkif_request_segment))).
 *
 * If a backend does not recognize BLKIF_OP_INDIRECT, it should *not*
 * create the "feature-max-indirect-segments" node!
 */
#define BLKIF_OP_INDIRECT          6

/*
 * Maximum scatter/gather segments per request.
 * This is carefully chosen so that sizeof(blkif_ring_t) <= PAGE_SIZE.
 * NB. This could be 12 if the ring indexes weren't stored in the same page.
 */
#define BLKIF_MAX_SEGMENTS_PER_REQUEST 11

/*
 * Maximum number of indirect pages to use per request.
 */
#define BLKIF_MAX_INDIRECT_PAGES_PER_REQUEST 8

/*
 * NB. first_sect and last_sect in blkif_request_segment, as well as
 * sector_number in blkif_request, are always expressed in 512-byte units.
 * However they must be properly aligned to the real sector size of the
 * physical disk, which is reported in the "physical-sector-size" node in
 * the backend xenbus info. Also the xenbus "sectors" node is expressed in
 * 512-byte units.
 */
struct blkif_request_segment {
    grant_ref_t gref;        /* reference to I/O buffer frame        */
    /* @first_sect: first sector in frame to transfer (inclusive).   */
    /* @last_sect: last sector in frame to transfer (inclusive).     */
    UINT8     first_sect, last_sect;
};

/*
 * Starting ring element for any I/O request.
 */
#if defined(MDE_CPU_IA32)
//
// pack(4) is necessary when these structs are compiled for Ia32.
// Without it, the struct will have a different alignment than the one
// a backend expect for a 32bit guest.
//
#pragma pack(4)
#endif
struct blkif_request {
    UINT8        operation;    /* BLKIF_OP_???                         */
    UINT8        nr_segments;  /* number of segments                   */
    blkif_vdev_t   handle;       /* only for read/write requests         */
    UINT64       id;           /* private guest value, echoed in resp  */
    blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
    struct blkif_request_segment seg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
};
typedef struct blkif_request blkif_request_t;

/*
 * Cast to this structure when blkif_request.operation == BLKIF_OP_DISCARD
 * sizeof(struct blkif_request_discard) <= sizeof(struct blkif_request)
 */
struct blkif_request_discard {
    UINT8        operation;    /* BLKIF_OP_DISCARD                     */
    UINT8        flag;         /* BLKIF_DISCARD_SECURE or zero         */
#define BLKIF_DISCARD_SECURE (1<<0)  /* ignored if discard-secure=0      */
    blkif_vdev_t   handle;       /* same as for read/write requests      */
    UINT64       id;           /* private guest value, echoed in resp  */
    blkif_sector_t sector_number;/* start sector idx on disk             */
    UINT64       nr_sectors;   /* number of contiguous sectors to discard*/
};
typedef struct blkif_request_discard blkif_request_discard_t;

struct blkif_request_indirect {
    UINT8        operation;    /* BLKIF_OP_INDIRECT                    */
    UINT8        indirect_op;  /* BLKIF_OP_{READ/WRITE}                */
    UINT16       nr_segments;  /* number of segments                   */
    UINT64       id;           /* private guest value, echoed in resp  */
    blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
    blkif_vdev_t   handle;       /* same as for read/write requests      */
    grant_ref_t    indirect_grefs[BLKIF_MAX_INDIRECT_PAGES_PER_REQUEST];
#ifdef MDE_CPU_IA32
    UINT64       pad;          /* Make it 64 byte aligned on i386      */
#endif
};
typedef struct blkif_request_indirect blkif_request_indirect_t;

struct blkif_response {
    UINT64        id;              /* copied from request */
    UINT8         operation;       /* copied from request */
    INT16         status;          /* BLKIF_RSP_???       */
};
typedef struct blkif_response blkif_response_t;
#if defined(MDE_CPU_IA32)
#pragma pack()
#endif

/*
 * STATUS RETURN CODES.
 */
 /* Operation not supported (only happens on barrier writes). */
#define BLKIF_RSP_EOPNOTSUPP  -2
 /* Operation failed for some unspecified reason (-EIO). */
#define BLKIF_RSP_ERROR       -1
 /* Operation completed successfully. */
#define BLKIF_RSP_OKAY         0

/*
 * Generate blkif ring structures and types.
 */
DEFINE_RING_TYPES(blkif, struct blkif_request, struct blkif_response);

#define VDISK_CDROM        0x1
#define VDISK_REMOVABLE    0x2
#define VDISK_READONLY     0x4

#endif /* __XEN_PUBLIC_IO_BLKIF_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
