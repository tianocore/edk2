# Title: Introduction of `EFI_MM_COMMUNICATE_HEADER_V3` and `MM_COMMUNICATE3_*` interface

## Status: Draft

## Document: UEFI Platform Initialization Specification Version 1.7 Errata A

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

Introduce `EFI_PEI_MM_COMMUNICATION3_PPI` and `EFI_MM_COMMUNICATE3_PROTOCOL` that works with communication buffer starts with `EFI_MM_COMMUNICATE_HEADER_V3` to provide better portability between different architectures (IA32 & X64) and adapt to flexible array supported by modern compilers.

## Benefits of the change

In PI Spec v1.7 Errata A, Vol.4, Sec 5.7 MM Communication Protocol, the MessageLength field of `EFI_MM_COMMUNICATE_HEADER` (also defined as `EFI_SMM_COMMUNICATE_HEADER`) is defined as type UINTN.

But this structure, as a generic definition, could be used for both PEI and DXE MM communication. Thus for a system that supports PEI Standalone MM launch, but operates PEI in 32bit mode and MM foundation in 64bit, the current `EFI_MM_COMMUNICATE_HEADER` definition will cause structure parse error due to UINTN used.

This proposed data structure resolved it by introducing `EFI_MM_COMMUNICATE_HEADER_V3` that defines the MessageSize as UINT64 to remove data size ambiguity.

The data structure still starts with a `HeaderGuid` field that is typed as `EFI_GUID`. This will be populated as `gCommunicateHeaderV3Guid` or `COMMUNICATE_HEADER_V3_GUID` as an indicator to differentiate new data format vesus legacy format.

Furthermore, the addition of signature could help identifying whether the data received is compiliant with this new data structure, which will help for binary release modules to identify usage of legacy `EFI_MM_COMMUNICATE_HEADER`.

Version field is also added to indicate the current version of header in case there is need for minor modification in the future.

In addition, the data field of MM communicate message is replaced with flexible array to allow users not having to consume extra data during communicate and author code more intrinsically.

On the non-MM environment side, the Standalone MM DXE IPL agent can add installation of `EFI_MM_COMMUNICATE3_PROTOCOL`, while the Standalone MM PEI IPL agent that launches MM foundation should publish and only publish `EFI_PEI_MM_COMMUNICATION3_PPI` for MM communication during PEI phase.

For communication data that starts with `EFI_MM_COMMUNICATE_HEADER_V3`, callers always need to use V3 protocol/PPI to communicate with updated MM cores. These interface introductions instead of replacement can maintain the compatibility for existing codebase while resolving size mismatching occurred during data transfer between different architectures.

## Impact of the change

This change will impact the MM cores and IPLs:

```bash
MdeModulePkg/Core/PiSmmCore/PiSmmCore
StandaloneMmPkg/Core/StandaloneMmCore
MdeModulePkg/Core/PiSmmCore/PiSmmIpl
```

To cooporate with the newly proposed data format, existing MM cores need to be updated to parse incoming data properly to tell if the data is compliant with new or legacy format.

The existing PiSmmIpl will need to be updated to publish `EFI_MM_COMMUNICATE3_PROTOCOL` for consumers that would like to invoke MMI with new data format.

For potential proprietary IPLs that launches Standalone MM in PEI phase, if any, the PEIM should change to publish `EFI_PEI_MM_COMMUNICATION3_PPI` instead of `EFI_MM_COMMUNICATE_PPI`.

Accordingly, all consumers in PEI phase that communicate to PEI launched Standalone MM should switch to use `EFI_PEI_MM_COMMUNICATION3_PPI` and `EFI_MM_COMMUNICATE_HEADER_V3`.

## Detailed description of the change [normative updates]

### Specification Changes

1. In PI Specification v1.7 Errata A: Vol. 4-1.5.1 Initializing MM Standalone Mode in PEI phase, the last bullet point of step 3 should be changed to:

    ```c
    Publishes the EFI_PEI_MM_COMMUNICATION3_PPI
    ```

1. In PI Specification v1.7 Errata A: Vol. 4, section 6.5 MM Communication PPI, add the following:

    ```markdown
    # EFI_PEI_MM_COMMUNICATION_PPI

    ## Summary

    This PPI provides a means of communicating between drivers outside of MM and MMI handlers inside of MM in PEI phase.

    ## GUID

      #define EFI_PEI_MM_COMMUNICATION3_PPI_GUID \
      { \
        0xe70febf6, 0x13ef, 0x4a21, { 0x89, 0x9e, 0xb2, 0x36, 0xf8, 0x25, 0xff, 0xc9 } \
      }

    ## PPI Structure

      typedef struct _EFI_PEI_MM_COMMUNICATION3_PPI {
        EFI_PEI_MM_COMMUNICATE3  Communicate;
      } EFI_PEI_MM_COMMUNICATION3_PPI;

    ## Members

    ### Communicate

      Sends/receives a message for a registered handler. See the Communicate() function description.

    ## Description

      This PPI provides services for communicating between PEIM and a registered MMI handler.

    # EFI_PEI_MM_COMMUNICATION_PPI.Communicate()

    ## Summary
      Communicates with a registered handler.

    ## Prototype
      typedef
      EFI_STATUS
      (EFIAPI *EFI_PEI_MM_COMMUNICATE3)(
        IN CONST EFI_PEI_MM_COMMUNICATION3_PPI   *This,
        IN OUT VOID                              *CommBuffer,
        IN OUT UINTN                             *CommSize
        );

    ## Parameters

    ### This
      The EFI_PEI_MM_COMMUNICATE3 instance.

    ### CommBuffer

      Pointer to the buffer to convey into MMRAM.

    ### CommSize

      The size of the data buffer being passed in. On exit, the size of data being returned. Zero if the handler does not wish to reply with any data.

    ## Description

      This function provides a service to send and receive messages from a registered PEI service. The EFI_PEI_MM_COMMUNICATION3_PPI driver is responsible for doing any of the copies such that the data lives in PEI-service-accessible RAM.

      A given implementation of the EFI_PEI_MM_COMMUNICATION3_PPI may choose to use the EFI_MM_CONTROL_PPI for effecting the mode transition, or it may use some other method.

      The agent invoking the communication interface must be physical/virtually 1:1 mapped.

      To avoid confusion in interpreting frames, the CommBuffer parameter should always begin with EFI_MM_COMMUNICATE_HEADER_V3. The header data is mandatory for messages sent into the MM agent.

      Once inside of MM, the MM infrastructure will call all registered handlers with the same HandlerType as the GUID specified by HeaderGuid and the CommBuffer pointing to Data.

      This function is not reentrant.

    ## Status Codes Returned
      EFI_SUCCESS            The message was successfully posted.
      EFI_INVALID_PARAMETER  The CommBuffer was NULL.
    ```

1. In PI Specification v1.7 Errata A: Vol. 4, section 6.5 MM Communication PPI, add the following:

    ```markdown
    # EFI_MM_COMMUNICATION3_PROTOCOL

    ## Summary

      This protocol provides a means of communicating between drivers outside of MM and MMI handlers inside of MM, for communication buffer that is compliant with EFI_MM_COMMUNICATE_HEADER_V3.

    ## GUID

      #define EFI_MM_COMMUNICATION3_PROTOCOL_GUID \
      { \
        0xf7234a14, 0xdf2, 0x46c0, { 0xad, 0x28, 0x90, 0xe6, 0xb8, 0x83, 0xa7, 0x2f } \
      }

    ## Prototype
      typedef struct _EFI_MM_COMMUNICATION3_PROTOCOL {
        EFI_MM_COMMUNICATE3  Communicate;
      } EFI_MM_COMMUNICATION3_PROTOCOL;

    ## Members

    ### Communicate

      Sends/receives a message for a registered handler. See the Communicate() function description.

    ## Description

      This protocol provides runtime services for communicating between DXE drivers and a registered MMI handler.

    # EFI_MM_COMMUNICATION3_PROTOCOL.Communicate()

    ## Summary

      Communicates with a registered handler.

    ## Prototype

      typedef
      EFI_STATUS
      (EFIAPI *EFI_MM_COMMUNICATE3)(
        IN CONST EFI_MM_COMMUNICATION3_PROTOCOL   *This,
        IN OUT VOID                               *CommBufferPhysical,
        IN OUT VOID                               *CommBufferVirtual,
        IN OUT UINTN                              *CommSize OPTIONAL
        );

    ## Parameters

    ### This

      The EFI_MM_COMMUNICATION3_PROTOCOL instance.

    ### CommBufferPhysical

      Physical address of the buffer to convey into MMRAM, of which content must start with EFI_MM_COMMUNICATE_HEADER_V3.

    ### CommBufferVirtual

      Virtual address of the buffer to convey into MMRAM, of which content must start with EFI_MM_COMMUNICATE_HEADER_V3.

    ### CommSize

      The size of the data buffer being passed in. On exit, the size of data being returned. Zero if the handler does not wish to reply with any data. This parameter is optional and may be NULL.

    ## Description

      Usage is similar to EFI_MM_COMMUNICATION_PROTOCOL.Communicate() except for the notes below:

      * Communication buffer transfer to MM core should start with EFI_MM_COMMUNICATE_HEADER_V3.
      * Instead of passing just the physical address via the CommBuffer parameter, the caller must pass both the physical and the virtual addresses of the communication buffer.
      * If no virtual remapping has taken place, the physical address will be equal to the virtual address, and so the caller is required to pass the same value for both parameters.

    ## Related Definitions
      typedef struct {
        EFI_GUID  HeaderGuid;
        UINT32    Signature;
        UINT32    Version;
        EFI_GUID  MessageGuid;
        UINT64    MessageSize;
        UINT8     MessageData[ANYSIZE_ARRAY];
      } EFI_MM_COMMUNICATE_HEADER_V3;

      #define COMMUNICATE_HEADER_V3_GUID \
      { \
        0x68e8c853, 0x2ba9, 0x4dd7, { 0x9a, 0xc0, 0x91, 0xe1, 0x61, 0x55, 0xc9, 0x35 } \
      }

      #define EFI_MM_COMMUNICATE_HEADER_V3_SIGNATURE  0x4D434833 // "MCH3"
      #define EFI_MM_COMMUNICATE_HEADER_V3_VERSION    3

    ### HeaderGuid

      Indicator GUID for MM core that the communication buffer is compliant with this v3 header. Must be COMMUNICATE_HEADER_V3_GUID.

    ### Signature

      Signature to indicate data is compliant with new MM communicate header structure. Must be "MCH3".

    ### Version

      MM communicate data format version, MM foundation entry point should check if incoming data is a supported format before proceeding. Must be 3.

    ### MessageGuid

      Allows for disambiguation of the message format.

    ### MessageSize

      Describes the size of MessageData (in bytes) and does not include the size of the header.

    ### MessageData

      Designates an array of bytes that is MessageSize in size.

    ## Status Codes Returned

      EFI_SUCCESS                 The message was successfully posted.
      EFI_INVALID_PARAMETER       CommBufferPhysical was NULL or CommBufferVirtual was NULL.
      EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation. If this error is returned, the MessageLength field in the CommBuffer header or the integer pointed by CommSize, are updated to reflect the maximum payload size the implementation can accommodate.
      EFI_ACCESS_DENIED           The CommunicateBuffer parameter or CommSize parameter, if not omitted, are in address range that cannot be accessed by the MM environment.
    ```

### Code Changes

1. Add data structure and its related definitions in `MdePkg/Include/Pi/PiMultiPhase.h` to match new definition.

1. Add interface definition of `MdePkg/Include/Protocol/MmCommunication3.h` and `MdePkg/Include/Protocol/MmCommunication3.h`, respectively, to match newly proposed interfaces.

1. Extend PiSmmCore to inspect `HeaderGuid` of incoming communication data. If it matches `COMMUNICATE_HEADER_V3_GUID`, parse the incoming data to start with `EFI_MM_COMMUNICATE_HEADER_V3`, otherwise it will be parse as existing way.

1. Extend StandaloneMmCore to inspect `HeaderGuid` of incoming communication data. If it matches `COMMUNICATE_HEADER_V3_GUID`, parse the incoming data to start with `EFI_MM_COMMUNICATE_HEADER_V3`, otherwise it will be parse as existing way.

1. Extend PiSmmIpl to publish `EFI_MM_COMMUNICATION3_PROTOCOL`, the implementation of `EFI_MM_COMMUNICATION3_PROTOCOL.Communicate()` should parse the incoming data as it starts with `EFI_MM_COMMUNICATE_HEADER_V3`, when applicable.
