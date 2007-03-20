/*++
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module name:
    snp.h

Abstract:

Revision history:
    2000-Feb-03 M(f)J   Genesis.
--*/
#ifndef _SNP_H
#define _SNP_H


#include "IndustryStandard/pci22.h"

#define FOUR_GIGABYTES  (UINT64) 0x100000000ULL

#define SNP_DRIVER_SIGNATURE  EFI_SIGNATURE_32 ('s', 'n', 'd', 's')
#define MAX_MAP_LENGTH        100

#define PCI_BAR_IO_MASK       0x00000003
#define PCI_BAR_IO_MODE       0x00000001

#define PCI_BAR_MEM_MASK      0x0000000F
#define PCI_BAR_MEM_MODE      0x00000000
#define PCI_BAR_MEM_64BIT     0x00000004

typedef struct {
  UINT32                      Signature;
  EFI_LOCK                    lock;

  EFI_SIMPLE_NETWORK_PROTOCOL snp;
  EFI_SIMPLE_NETWORK_MODE     mode;

  EFI_HANDLE                  device_handle;
  EFI_DEVICE_PATH_PROTOCOL    *device_path;

  //
  //  Local instance data needed by SNP driver
  //
  //  Pointer to S/W UNDI API entry point
  //  This will be NULL for H/W UNDI
  //
  EFI_STATUS (*issue_undi32_command) (UINT64 cdb);

  BOOLEAN               is_swundi;

  //
  // undi interface number, if one undi manages more nics
  //
  PXE_IFNUM             if_num;

  //
  //  Allocated tx/rx buffer that was passed to UNDI Initialize.
  //
  UINT32                tx_rx_bufsize;
  VOID                  *tx_rx_buffer;
  //
  // mappable buffers for receive and fill header for undi3.0
  // these will be used if the user buffers are above 4GB limit (instead of
  // mapping the user buffers)
  //
  UINT8                 *receive_buf;
  VOID                  *ReceiveBufUnmap;
  UINT8                 *fill_hdr_buf;
  VOID                  *FillHdrBufUnmap;

  EFI_PCI_IO_PROTOCOL   *IoFncs;
  UINT8                 IoBarIndex;
  UINT8                 MemoryBarIndex;
  BOOLEAN               IsOldUndi;  // true for EFI1.0 UNDI (3.0) drivers
  //
  // Buffers for command descriptor block, command parameter block
  // and data block.
  //
  PXE_CDB               cdb;
  VOID                  *cpb;
  VOID                  *CpbUnmap;
  VOID                  *db;

  //
  // UNDI structure, we need to remember the init info for a long time!
  //
  PXE_DB_GET_INIT_INFO  init_info;

  VOID                  *SnpDriverUnmap;
  //
  // when ever we map an address, we must remember it's address and the un-map
  // cookie so that we can unmap later
  //
  struct s_map_list {
    EFI_PHYSICAL_ADDRESS  virt;
    VOID                  *map_cookie;
  } map_list[MAX_MAP_LENGTH];
}
SNP_DRIVER;

#define EFI_SIMPLE_NETWORK_DEV_FROM_THIS(a) CR (a, SNP_DRIVER, snp, SNP_DRIVER_SIGNATURE)

//
// Global Variables
//
extern EFI_COMPONENT_NAME_PROTOCOL  gSimpleNetworkComponentName;
extern EFI_DRIVER_BINDING_PROTOCOL  gSimpleNetworkDriverBinding;

extern EFI_PCI_IO_PROTOCOL          *mPciIoFncs;

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
SimpleNetworkComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
SimpleNetworkComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

//
//  Virtual to physical mapping for all UNDI 3.0s.
//
extern struct                       s_v2p {
  struct s_v2p          *next;
  VOID                  *vaddr;
  UINTN                 bsize;
  EFI_PHYSICAL_ADDRESS  paddr;
  VOID                  *unmap;
}
*_v2p;

EFI_STATUS
add_v2p (
  struct s_v2p                  **v2p,
  EFI_PCI_IO_PROTOCOL_OPERATION type,
  VOID                          *vaddr,
  UINTN                         bsize
  )
;

EFI_STATUS
find_v2p (
  struct s_v2p **v2p,
  VOID         *vaddr
  )
;

EFI_STATUS
del_v2p (
  VOID *vaddr
  )
;

extern
VOID
snp_undi32_callback_block_30 (
  IN UINT32 Enable
  )
;

extern
VOID
snp_undi32_callback_delay_30 (
  IN UINT64 MicroSeconds
  )
;

extern
VOID
snp_undi32_callback_memio_30 (
  IN UINT8      ReadOrWrite,
  IN UINT8      NumBytes,
  IN UINT64     MemOrPortAddress,
  IN OUT UINT64 BufferPtr
  )
;

extern
VOID
snp_undi32_callback_v2p_30 (
  IN UINT64     CpuAddr,
  IN OUT UINT64 DeviceAddrPtr
  )
;

extern
VOID
snp_undi32_callback_block (
  IN UINT64 UniqueId,
  IN UINT32 Enable
  )
;

extern
VOID
snp_undi32_callback_delay (
  IN UINT64 UniqueId,
  IN UINT64 MicroSeconds
  )
;

extern
VOID
snp_undi32_callback_memio (
  IN UINT64     UniqueId,
  IN UINT8      ReadOrWrite,
  IN UINT8      NumBytes,
  IN UINT64     MemOrPortAddr,
  IN OUT UINT64 BufferPtr
  )
;

extern
VOID
snp_undi32_callback_map (
  IN UINT64     UniqueId,
  IN UINT64     CpuAddr,
  IN UINT32     NumBytes,
  IN UINT32     Direction,
  IN OUT UINT64 DeviceAddrPtr
  )
;

extern
VOID
snp_undi32_callback_unmap (
  IN UINT64             UniqueId,
  IN UINT64             CpuAddr,
  IN UINT32             NumBytes,
  IN UINT32             Direction,
  IN UINT64 DeviceAddr  // not a pointer to device address
  )
;

extern
VOID
snp_undi32_callback_sync (
  IN UINT64             UniqueId,
  IN UINT64             CpuAddr,
  IN UINT32             NumBytes,
  IN UINT32             Direction,
  IN UINT64 DeviceAddr  // not a pointer to device address
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_start (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_initialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this,
  IN UINTN                       extra_rx_buffer_size OPTIONAL,
  IN UINTN                       extra_tx_buffer_size OPTIONAL
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *this,
  IN BOOLEAN                      ExtendedVerification
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_shutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_receive_filters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  IN UINT32                      enable,
  IN UINT32                      disable,
  IN BOOLEAN                     reset_mcast_filter,
  IN UINTN                       mcast_filter_count OPTIONAL,
  IN EFI_MAC_ADDRESS             * mcast_filter OPTIONAL
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_station_address (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  IN BOOLEAN                     reset,
  IN EFI_MAC_ADDRESS             *new OPTIONAL
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_statistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  * this,
  IN BOOLEAN                      reset,
  IN OUT UINTN                    *statistics_size OPTIONAL,
  IN OUT EFI_NETWORK_STATISTICS   * statistics_table OPTIONAL
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_mcast_ip_to_mac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this,
  IN BOOLEAN                     IPv6,
  IN EFI_IP_ADDRESS              *IP,
  OUT EFI_MAC_ADDRESS            *MAC
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_nvdata (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this,
  IN BOOLEAN                     read_write,
  IN UINTN                       offset,
  IN UINTN                       buffer_size,
  IN OUT VOID                    *buffer
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_get_status (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  OUT UINT32                     *interrupt_status OPTIONAL,
  OUT VOID                       **tx_buffer OPTIONAL
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_transmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  IN UINTN                       header_size,
  IN UINTN                       buffer_size,
  IN VOID                        *buffer,
  IN EFI_MAC_ADDRESS             * src_addr OPTIONAL,
  IN EFI_MAC_ADDRESS             * dest_addr OPTIONAL,
  IN UINT16                      *protocol OPTIONAL
  )
;

extern
EFI_STATUS
EFIAPI
snp_undi32_receive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  OUT UINTN                      *header_size OPTIONAL,
  IN OUT UINTN                   *buffer_size,
  OUT VOID                       *buffer,
  OUT EFI_MAC_ADDRESS            * src_addr OPTIONAL,
  OUT EFI_MAC_ADDRESS            * dest_addr OPTIONAL,
  OUT UINT16                     *protocol OPTIONAL
  )
;

VOID
EFIAPI
SnpWaitForPacketNotify (
  IN EFI_EVENT  Event,
  IN VOID       *SnpPtr
  );

EFI_STATUS
pxe_start (
  SNP_DRIVER *snp
  );

EFI_STATUS
pxe_stop (
  SNP_DRIVER *snp
  );

EFI_STATUS
pxe_init (
  SNP_DRIVER *snp,
  UINT16     OpFlags
  );

EFI_STATUS
pxe_shutdown (
  SNP_DRIVER *snp
  );

EFI_STATUS
pxe_get_stn_addr (
  SNP_DRIVER *snp
  );

typedef
EFI_STATUS
(*issue_undi32_command) (
  UINT64 cdb
  );

typedef
VOID
(*ptr) (
  VOID
  );

#define SNP_MEM_PAGES(x)  (((x) - 1) / 4096 + 1)

#endif /*  _SNP_H  */
