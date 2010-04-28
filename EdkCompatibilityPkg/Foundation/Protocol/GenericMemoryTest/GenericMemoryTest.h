/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    GenericMemoryTest.h

Abstract:

    The EFI generic memory test protocol
    For more information please look at EfiMemoryTest.doc

--*/

#ifndef __GENERIC_MEMORY_TEST_H__
#define __GENERIC_MEMORY_TEST_H__

#define EFI_GENERIC_MEMORY_TEST_PROTOCOL_GUID  \
  { 0x309de7f1, 0x7f5e, 0x4ace, {0xb4, 0x9c, 0x53, 0x1b, 0xe5, 0xaa, 0x95, 0xef}}

EFI_FORWARD_DECLARATION (EFI_GENERIC_MEMORY_TEST_PROTOCOL);

typedef enum {
  IGNORE,
  QUICK,
  SPARSE,
  EXTENSIVE,
  MAXLEVEL
} EXTENDMEM_COVERAGE_LEVEL;

typedef
EFI_STATUS
(EFIAPI *EFI_MEMORY_TEST_INIT) (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This,
  IN  EXTENDMEM_COVERAGE_LEVEL                 Level,
  OUT BOOLEAN                                  *RequireSoftECCInit
  )
/*++

  Routine Description:
    Initialize the generic memory test.

  Arguments:
    This                  - Protocol instance pointer.
    Level                 - The coverage level of the memory test.
    RequireSoftECCInit    - Indicate if the memory need software ECC init.

  Returns:
    EFI_SUCCESS           - The generic memory test initialized correctly.
    EFI_NO_MEDIA          - There is not any non-tested memory found, in this
                            function if not any non-tesed memory found means 
                            that the memory test driver have not detect any
                            non-tested extended memory of current system.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_PERFORM_MEMORY_TEST) (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This,
  OUT UINT64                                   *TestedMemorySize,
  OUT UINT64                                   *TotalMemorySize,
  OUT BOOLEAN                                  *ErrorOut,
  IN BOOLEAN                                   IfTestAbort
  )
/*++

  Routine Description:
    Perform the memory test.

  Arguments:
    This                  - Protocol instance pointer.
    TestedMemorySize      - Return the tested extended memory size.
    TotalMemorySize       - Return the whole system physical memory size, this 
                            value may be changed if in some case some error 
                            DIMMs be disabled.
    ErrorOut              - Any time the memory error occurs, this will be TRUE.
    IfTestAbort           - Indicate if the user press "ESC" to skip the memory
                            test.

  Returns:
    EFI_SUCCESS           - One block of memory test ok, the block size is hide
                            internally.
    EFI_NOT_FOUND         - Indicate all the non-tested memory blocks have 
                            already go through.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_MEMORY_TEST_FINISHED) (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  )
/*++

  Routine Description:
    The memory test finished.

  Arguments:
    This                  - Protocol instance pointer.

  Returns:
    EFI_SUCCESS           - Successful free all the generic memory test driver
                            allocated resource and notify to platform memory
                            test driver that memory test finished.

--*/
;
  
typedef
EFI_STATUS
(EFIAPI *EFI_MEMORY_TEST_COMPATIBLE_RANGE) (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This,
  IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
  IN  UINT64                                   Length
  )
/*++
  
  Routine Description:
    Provide capability to test compatible range which used by some sepcial
    driver required using memory range before BDS perform memory test.
    
  Arguments:
    This                  - Protocol instance pointer.
    StartAddress          - The start address of the memory range.
    Length                - The memory range's length.
    
  Return:
    EFI_SUCCESS           - The compatible memory range pass the memory test.
    EFI_DEVICE_ERROR      - The compatible memory range test find memory error
                            and also return return the error address.
    
--*/
;

struct _EFI_GENERIC_MEMORY_TEST_PROTOCOL {
  EFI_MEMORY_TEST_INIT              MemoryTestInit;
  EFI_PERFORM_MEMORY_TEST           PerformMemoryTest;
  EFI_MEMORY_TEST_FINISHED          Finished;
  EFI_MEMORY_TEST_COMPATIBLE_RANGE  CompatibleRangeTest;
};

extern EFI_GUID gEfiGenericMemTestProtocolGuid;
#endif
