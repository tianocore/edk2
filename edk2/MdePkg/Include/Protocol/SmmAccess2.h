/** @file
  EFI SMM Access2 Protocol as defined in the PI 1.2 specification.

  This protocol is used to control the visibility of the SMRAM on the platform.
  It abstracts the location and characteristics of SMRAM.  The expectation is
  that the north bridge or memory controller would publish this protocol.

  The principal functionality found in the memory controller includes the following: 
  - Exposing the SMRAM to all non-SMM agents, or the "open" state
  - Shrouding the SMRAM to all but the SMM agents, or the "closed" state
  - Preserving the system integrity, or "locking" the SMRAM, such that the settings cannot be 
    perturbed by either boot service or runtime agents 

  Copyright (c) 2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _SMM_ACCESS2_H_
#define _SMM_ACCESS2_H_

#include <PiDxe.h>

#define EFI_SMM_ACCESS2_PROTOCOL_GUID \
  { \
     0xc2702b74, 0x800c, 0x4131, {0x87, 0x46, 0x8f, 0xb5, 0xb8, 0x9c, 0xe4, 0xac } \
  }

///
/// SMRAM states and capabilities
///
#define EFI_SMRAM_OPEN                  0x00000001
#define EFI_SMRAM_CLOSED                0x00000002
#define EFI_SMRAM_LOCKED                0x00000004
#define EFI_CACHEABLE                   0x00000008
#define EFI_ALLOCATED                   0x00000010
#define EFI_NEEDS_TESTING               0x00000020
#define EFI_NEEDS_ECC_INITIALIZATION    0x00000040

///
/// Structure describing a SMRAM region and its accessibility attributes
///
typedef struct {
  ///
  /// Designates the physical address of the SMRAM in memory. This view of memory is 
  /// the same as seen by I/O-based agents, for example, but it may not be the address seen 
  /// by the processors.
  ///
  EFI_PHYSICAL_ADDRESS  PhysicalStart;
  ///
  /// Designates the address of the SMRAM, as seen by software executing on the 
  /// processors. This address may or may not match PhysicalStart.
  ///
  EFI_PHYSICAL_ADDRESS  CpuStart;       
  ///
  /// Describes the number of bytes in the SMRAM region.
  ///
  UINT64                PhysicalSize;
  ///
  /// Describes the accessibility attributes of the SMRAM.  These attributes include the 
  /// hardware state (e.g., Open/Closed/Locked), capability (e.g., cacheable), logical 
  /// allocation (e.g., allocated), and pre-use initialization (e.g., needs testing/ECC 
  /// initialization).
  ///
  UINT64                RegionState;
} EFI_SMRAM_DESCRIPTOR;

typedef struct _EFI_SMM_ACCESS2_PROTOCOL  EFI_SMM_ACCESS2_PROTOCOL;

/**
  Opens the SMRAM area to be accessible by a boot-service driver.

  This function "opens" SMRAM so that it is visible while not inside of SMM. The function should 
  return EFI_UNSUPPORTED if the hardware does not support hiding of SMRAM. The function 
  should return EFI_DEVICE_ERROR if the SMRAM configuration is locked.

  @param[in] This           The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_UNSUPPORTED   The system does not support opening and closing of SMRAM.
  @retval EFI_DEVICE_ERROR  SMRAM cannot be opened, perhaps because it is locked.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_OPEN)(
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

/**
  Inhibits access to the SMRAM.

  This function "closes" SMRAM so that it is not visible while outside of SMM. The function should 
  return EFI_UNSUPPORTED if the hardware does not support hiding of SMRAM.

  @param[in] This           The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_UNSUPPORTED   The system does not support opening and closing of SMRAM.
  @retval EFI_DEVICE_ERROR  SMRAM cannot be closed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CLOSE)(
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

/**
  Inhibits access to the SMRAM.

  This function prohibits access to the SMRAM region.  This function is usually implemented such 
  that it is a write-once operation. 

  @param[in] This          The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS      The device was successfully locked.
  @retval EFI_UNSUPPORTED  The system does not support locking of SMRAM.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_LOCK)(
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

/**
  Queries the memory controller for the possible regions that will support SMRAM.

  @param[in]     This           The EFI_SMM_ACCESS2_PROTOCOL instance.
  @param[in,out] SmramMapSize   A pointer to the size, in bytes, of the SmramMemoryMap buffer.
  @param[in,out] SmramMap       A pointer to the buffer in which firmware places the current memory map.

  @retval EFI_SUCCESS           The chipset supported the given resource.
  @retval EFI_BUFFER_TOO_SMALL  The SmramMap parameter was too small.  The current buffer size 
                                needed to hold the memory map is returned in SmramMapSize.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CAPABILITIES)(
  IN CONST EFI_SMM_ACCESS2_PROTOCOL  *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  );

///
///  EFI SMM Access2 Protocol is used to control the visibility of the SMRAM on the platform.
///  It abstracts the location and characteristics of SMRAM.  The expectation is
///  that the north bridge or memory controller would publish this protocol.
/// 
struct _EFI_SMM_ACCESS2_PROTOCOL {
  EFI_SMM_OPEN          Open;
  EFI_SMM_CLOSE         Close;
  EFI_SMM_LOCK          Lock;
  EFI_SMM_CAPABILITIES  GetCapabilities;
  ///
  /// Indicates the current state of the SMRAM. Set to TRUE if SMRAM is locked.
  ///
  BOOLEAN               LockState;
  ///
  /// Indicates the current state of the SMRAM. Set to TRUE if SMRAM is open.
  ///
  BOOLEAN               OpenState;
};

extern EFI_GUID gEfiSmmAccess2ProtocolGuid;

#endif

