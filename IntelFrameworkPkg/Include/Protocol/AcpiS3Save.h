/** @file
  This protocol is used to prepare all information that is needed for the S3 resume boot path. This
  protocol is not required for all platforms.
  
Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This Protocol is defined in Framework of S3 Resume Boot Path Spec.
  Version 0.9.
  
**/

#ifndef _ACPI_S3_SAVE_PROTOCOL_H_
#define _ACPI_S3_SAVE_PROTOCOL_H_

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_ACPI_S3_SAVE_PROTOCOL EFI_ACPI_S3_SAVE_PROTOCOL;

//
// S3 Save Protocol GUID
//
#define EFI_ACPI_S3_SAVE_GUID \
  { \
    0x125f2de1, 0xfb85, 0x440c, {0xa5, 0x4c, 0x4d, 0x99, 0x35, 0x8a, 0x8d, 0x38 } \
  }

//
// Protocol Data Structures
//

/**
 	This function is used to:
  
  - Prepare all information that is needed in the S3 resume boot path. This information can include
  the following:
     -- Framework boot script table
     -- RSDT pointer
     -- Reserved memory for the S3 resume
     
  - Get the minimum legacy memory length (meaning below 1 MB) that is required for the S3 resume boot path.
  If LegacyMemoryAddress is NULL, the firmware will be unable to jump into a real-mode
  waking vector. However, it might still be able to jump into a flat-mode waking vector as long as the
  OS provides a flat-mode waking vector. It is the caller's responsibility to ensure the
  LegacyMemoryAddress is valid. If the LegacyMemoryAddress is higher than 1 MB,
  EFI_INVALID_PARAMETER will be returned.

  @param  This                  A pointer to the EFI_ACPI_S3_SAVE_PROTOCOL instance.
  @param  LegacyMemoryAddress   The base of legacy memory.

  @retval EFI_SUCCESS           All information was saved successfully.
  @retval EFI_INVALID_PARAMETER The memory range is not located below 1 MB.
  @retval EFI_OUT_OF_RESOURCES  Resources were insufficient to save all the information.
  @retval EFI_NOT_FOUND         Some necessary information cannot be found. 

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_S3_SAVE)(
  IN EFI_ACPI_S3_SAVE_PROTOCOL      * This,
  IN VOID                           * LegacyMemoryAddress
  );

/**
  This function returns the size of the legacy memory (meaning below 1 MB) that is required during an S3
  resume. Before the Framework-based firmware transfers control to the OS, it has to transition from
  flat mode into real mode in case the OS supplies only a real-mode waking vector. This transition
  requires a certain amount of legacy memory. After getting the size of legacy memory
  below, the caller is responsible for allocating the legacy memory below 1 MB according to
  the size that is returned. The specific implementation of allocating the legacy memory is out of the
  scope of this specification.

  @param  This                  A pointer to the EFI_ACPI_S3_SAVE_PROTOCOL instance.
  @param  Size   		The returned size of legacy memory below 1MB.

  @retval EFI_SUCCESS           Size was successfully returned.
  @retval EFI_INVALID_PARAMETER The pointer Size is NULL.
    
**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_GET_LEGACY_MEMORY_SIZE)(
  IN  EFI_ACPI_S3_SAVE_PROTOCOL     * This,
  OUT UINTN                         * Size
);

/**
  The EFI_ACPI_S3_SAVE_PROTOCOL is responsible for preparing all the information that the
  Framework needs to restore the platform's preboot state during an S3 resume boot. This
  information can include the following:
    - The Framework boot script table, containing all necessary operations to initialize the platform.
    - ACPI table information, such as RSDT, through which the OS waking vector can be located.
    - The range of reserved memory that can be used on the S3 resume boot path.
  This protocol can be used after the Framework makes sure that the boot process is complete and
  that no hardware has been left unconfigured. Where to call this protocol to save information is implementation-specific. 
  In the case of an EFI-aware OS, ExitBootServices() can be a choice to provide this hook.
  The currently executing EFI OS loader image calls ExitBootServices()to terminate all boot
  services. After ExitBootServices() successfully completes, the loader becomes responsible
  for the continued operation of the system.
  On a normal boot, ExitBootServices() checks if the platform supports S3 by looking for
  EFI_ACPI_S3_SAVE_PROTOCOL. If the protocol exists, ExitBootServices()will assume
  that the target platform supports an S3 resume and then call EFI_ACPI_S3_SAVE_PROTOCOL
  to save the S3 resume information. The entire Framework boot script table will then be generated,
  assuming the platform currently is in the preboot state.
**/
struct _EFI_ACPI_S3_SAVE_PROTOCOL {
  ///
  /// Gets the size of legacy memory below 1 MB that is required for S3 resume.
  ///
  EFI_ACPI_GET_LEGACY_MEMORY_SIZE   GetLegacyMemorySize;
  
  ///
  /// Prepare all information for an S3 resume.
  ///
  EFI_ACPI_S3_SAVE                  S3Save;
};

extern EFI_GUID gEfiAcpiS3SaveProtocolGuid;

#endif
