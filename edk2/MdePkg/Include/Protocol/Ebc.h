/** @file
  Describes the protocol interface to the EBC interpreter.

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
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
  Creates a thunk for an EBC entry point, returning the address of the thunk.
  
  A PE32+ EBC image, like any other PE32+ image, contains an optional header that specifies the
  entry point for image execution. However for EBC images this is the entry point of EBC
  instructions, so is not directly executable by the native processor. Therefore when an EBC image is
  loaded, the loader must call this service to get a pointer to native code (thunk) that can be executed
  which will invoke the interpreter to begin execution at the original EBC entry point.

  @param  This          A pointer to the EFI_EBC_PROTOCOL instance.
  @param  ImageHandle   Handle of image for which the thunk is being created.
  @param  EbcEntryPoint Address of the actual EBC entry point or protocol service the thunk should call.
  @param  Thunk         Returned pointer to a thunk created.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Image entry point is not 2-byte aligned.
  @retval EFI_OUT_OF_RESOURCES   Memory could not be allocated for the thunk.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_EBC_CREATE_THUNK)(
  IN EFI_EBC_PROTOCOL           *This,
  IN EFI_HANDLE                 ImageHandle,
  IN VOID                       *EbcEntryPoint,
  OUT VOID                      **Thunk
  );

/**
  Called prior to unloading an EBC image from memory.

  This function is called after an EBC image has exited, but before the image is actually unloaded. It
  is intended to provide the interpreter with the opportunity to perform any cleanup that may be
  necessary as a result of loading and executing the image.

  @param  This          A pointer to the EFI_EBC_PROTOCOL instance.
  @param  ImageHandle   Image handle of the EBC image that is being unloaded from memory.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Image handle is not recognized as belonging 
                                 to an EBC image that has been executed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_EBC_UNLOAD_IMAGE)(
  IN EFI_EBC_PROTOCOL           *This,
  IN EFI_HANDLE                 ImageHandle
  );

/**
  This is the prototype for the Flush callback routine. A pointer to a routine 
  of this type is passed to the EBC EFI_EBC_REGISTER_ICACHE_FLUSH protocol service.

  @param  Start  The beginning physical address to flush from the processor's instruction cache.
  @param  Length The number of bytes to flush from the processor's instruction cache.

  @retval EFI_SUCCESS            The function completed successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EBC_ICACHE_FLUSH)(
  IN EFI_PHYSICAL_ADDRESS     Start,
  IN UINT64                   Length
  );

/**
  This routine is called by the core firmware to provide the EBC driver with
  a function to call to flush the CPU's instruction cache following creation
  of a thunk. It is not required.

  @param  This       A pointer to the EFI_EBC_PROTOCOL instance.
  @param  Flush      Pointer to a function of type EBC_ICACH_FLUSH.

  @retval EFI_SUCCESS            The function completed successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_EBC_REGISTER_ICACHE_FLUSH)(
  IN EFI_EBC_PROTOCOL           *This,
  IN EBC_ICACHE_FLUSH           Flush
  );

/**
  Called to get the version of the interpreter.

  This function is called to get the version of the loaded EBC interpreter. The value and format of the
  returned version is identical to that returned by the EBC BREAK 1 instruction.

  @param  This       A pointer to the EFI_EBC_PROTOCOL instance.  
  @param  Version Pointer to where to store the returned version of the interpreter.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Version pointer is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_EBC_GET_VERSION)(
  IN EFI_EBC_PROTOCOL           *This,
  IN OUT UINT64                 *Version
  );

//
// Prototype for the actual EBC protocol interface
//
/**
  This protocol provides the services that allow execution of EBC images.

  @par Protocol Description:
  The EFI EBC protocol provides services to load and execute EBC images, which will typically be
  loaded into option ROMs. The image loader will load the EBC image, perform standard relocations,
  and invoke the CreateThunk() service to create a thunk for the EBC image's entry point. The
  image can then be run using the standard EFI start image services.

  @param CreateThunk 
  Creates a thunk for an EBC image entry point or protocol service,
  and returns a pointer to the thunk. 
  
  @param UnloadImage 
  Called when an EBC image is unloaded to allow the interpreter to
  perform any cleanup associated with the image execution. 
  
  @param RegisterICacheFlush
  Called to register a callback function that the EBC interpreter can
  call to flush the processor instruction cache after creating thunks.
  
  @param GetVersion 
  Called to get the version of the associated EBC interpreter.

**/
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
