/** @file
    EFI MM Communication Protocol 2 as defined in the PI 1.7 errata A specification.

    Provides a runtime service for communicating between DXE drivers and a registered MMI handler

Copyright (c) 2020, American Megatrends International LLC. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _MM_COMMUNICATION2_H_
#define _MM_COMMUNICATION2_H_

///
/// Global ID for EFI_MM_COMUNICATION@_PROTOCOL_GUID
///
#define EFI_MM_COMMUNICATION2_PROTOCOL_GUID \
        { 0x378daedc, 0xf06b, 0x4446, { 0x83, 0x14, 0x40, 0xab, 0x93, 0x3c, 0x87, 0xa3 }}


///
/// The forward declaration for EFI_MM_COMMUNICATION2_PROTOCOL_GUID
///
typedef struct _EFI_MM_COMMUNICATION2_PROTOCOL EFI_MM_COMMUNICATION2_PROTOCOL;


/**
 Communicates with a registered handler.

    Usage is identical to EFI_MM_COMMUNICATION_PROTOCOL.Communicate() except for the notes below:
    - Instead of passing just the physical address via the CommBuffer parameter, the caller must pass both the physical and the virtual addresses of the communication buffer.
    - If no virtual remapping has taken place, the physical address will be equal to the virtual address, and so the caller is required to pass the same value for both parameters.

    @param This                 The EFI_MM_COMMUNICATION2_PROTOCOL instance.
    @param CommBufferPhysical   Physical address of the buffer to convey into MMRAM.
    @param CommBufferVirtual    Virtual address of the buffer to convey into MMRAM.
    @param CommSize             The size of the data buffer being passed in. On exit, the
                                size of data being returned. Zero if the handler does not
                                wish to reply with any data. This parameter is optional and may be NULL.


    @retval EFI_SUCCESS             The message was successfully posted.
    @retval EFI_INVALID_PARAMETER   The CommBuffer** parameters do not refer to the same location in memory.
    @retval EFI_BAD_BUFFER_SIZE     The buffer is too large for the MM implementation.
                                    If this error is returned, the MessageLength field
                                    in the CommBuffer header or the integer pointed by
                                    CommSize, are updated to reflect the maximum payload
                                    size the implementation can accommodate.
    @retval EFI_ACCESS_DENIED       The CommunicateBuffer parameter or CommSize parameter,
                                    if not omitted, are in address range that cannot be
                                    accessed by the MM environment.
**/

typedef
EFI_STATUS
(EFIAPI *EFI_MM_COMMUNICATE2)(
IN CONST EFI_MM_COMMUNICATION2_PROTOCOL     *This,
IN OUT VOID                                 *CommBufferPhysical,
IN OUT VOID                                 *CommBufferVirtual,
IN OUT UINTN                                *CommSize OPTIONAL
);


/// This protocol provides a means of communicating between drivers outside
/// of MM and MMI handlers inside of MM, in a way that hides the implementation
/// details regarding whether traditional or standalone MM is being used.

typedef struct _EFI_MM_COMMUNICATION2_PROTOCOL {
EFI_MM_COMMUNICATE2        Communicate;
} EFI_MM_COMMUNICATION2_PROTOCOL;


#endif
