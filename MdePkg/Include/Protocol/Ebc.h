/** @file
  Describes the protocol interface to the EBC interpreter.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_EBC_PROTOCOL_H__
#define __EFI_EBC_PROTOCOL_H__

#define EFI_EBC_INTERPRETER_PROTOCOL_GUID \
  { \
    0x13AC6DD1, 0x73D0, 0x11D4, {0xB0, 0x6B, 0x00, 0xAA, 0x00, 0xBD, 0x6D, 0xE7 } \
  }

//
// Protocol Guid Name defined in spec.
//
#define EFI_EBC_PROTOCOL_GUID EFI_EBC_INTERPRETER_PROTOCOL_GUID

//
// Define for forward reference.
//
typedef struct _EFI_EBC_PROTOCOL EFI_EBC_PROTOCOL;

/**
  Create a thunk for an image entry point. In short, given the physical address
  of the entry point for a loaded image, create a thunk that does some 
  fixup of arguments (and perform any other necessary overhead) and then
  calls the original entry point. The caller can then use the returned pointer
  to the created thunk as the new entry point to image.

  @param  This          protocol instance pointer
  @param  ImageHandle   handle to the image. The EBC interpreter may use this to keep
                        track of any resource allocations performed in loading and
                        executing the image.
  @param  EbcEntryPoint the entry point for the image (as defined in the file header)
  @param  Thunk pointer to thunk pointer where the address of the created
                        thunk is returned.

  @return Standard EFI_STATUS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_EBC_CREATE_THUNK) (
  IN EFI_EBC_PROTOCOL           *This,
  IN EFI_HANDLE                 ImageHandle,
  IN VOID                       *EbcEntryPoint,
  OUT VOID                      **Thunk
  );

/**
  Perform any cleanup necessary when an image is unloaded. Basically it gives
  the EBC interpreter the chance to free up any resources allocated during
  load and execution of an EBC image.

  @param  This        protocol instance pointer
  @param  ImageHandle the handle of the image being unloaded.

  @return Standard EFI_STATUS.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_EBC_UNLOAD_IMAGE) (
  IN EFI_EBC_PROTOCOL           *This,
  IN EFI_HANDLE                 ImageHandle
  );

/**
  The I-Cache-flush registration service takes a pointer to a function to
  call to flush the I-Cache. Here's the prototype for that function pointer.

  @param  Start  physical start address of CPU instruction cache to flush.
  @param  Length how many bytes to flush of the instruction cache.

  @return Standard EFI_STATUS.

**/
typedef
EFI_STATUS
(EFIAPI *EBC_ICACHE_FLUSH) (
  IN EFI_PHYSICAL_ADDRESS     Start,
  IN UINT64                   Length
  );

/**
  This routine is called by the core firmware to provide the EBC driver with
  a function to call to flush the CPU's instruction cache following creation
  of a thunk. It is not required.

  @param  This  protocol instance pointer
  @param  Flush pointer to the function to call to flush the CPU instruction
  cache.

  @return Standard EFI_STATUS.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_EBC_REGISTER_ICACHE_FLUSH) (
  IN EFI_EBC_PROTOCOL           *This,
  IN EBC_ICACHE_FLUSH           Flush
  );

/**
  This routine can be called to get the VM revision. It returns the same
  value as the EBC BREAK 1 instruction returns.

  @param  This    protocol instance pointer
  @param  Version pointer to where to return the VM version

  @return Standard EFI_STATUS.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_EBC_GET_VERSION) (
  IN EFI_EBC_PROTOCOL           *This,
  IN OUT UINT64                 *Version
  );

//
// Prototype for the actual EBC protocol interface
//
struct _EFI_EBC_PROTOCOL {
  EFI_EBC_CREATE_THUNK          CreateThunk;
  EFI_EBC_UNLOAD_IMAGE          UnloadImage;
  EFI_EBC_REGISTER_ICACHE_FLUSH RegisterICacheFlush;
  EFI_EBC_GET_VERSION           GetVersion;
};

//
// Extern the global EBC protocol GUID
//
extern EFI_GUID gEfiEbcProtocolGuid;

#endif
