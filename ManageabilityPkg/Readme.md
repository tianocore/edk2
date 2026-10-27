# EDK2 Manageability Package

edk2 ManageabilityPkg is introduced to provide edk2 drivers and
libraries for industry platform management standards, such as PLDM (Platform
Level Data Model), MCTP (Management Component Transfer Protocol), IPMI
(Intelligent Platform Management Interface) and others. The framework of
ManageabilityPkg is designed to flexibly support the transport interfaces for above
industry standards, the transport interfaces such as KCS or I2C for IPMI, PCI VDM
(Vendor Defined Message), I2C or KCS for MCTP, or the OEM proprietary transports.

Below figure shows the driver stacks which are abstracted to support disparate
transports for specifications of platform management.
![Manageability Package Driver Stack](https://github.com/tianocore/edk2/tree/master/ManageabilityPkg/Documents/Media/ManageabilityDriverStack.svg)

## Manageability Transport edk2 Driver Stack
edk2 manageability transport library is designed to incorporated with disparate
manageability protocols. In order to flexibly facilitating the transport
functionalities, below functions must be provided by manageability transport
library to compliant with the framework of ManageabilityPkg design.

* ### ***AcquireTransportSession()***
    edk2 manageability protocol driver invokes this function to acquire the
    transport session to transmit manageability packet. The implementation
    could reset the transport interface, initial the transport interface
    internal structure and the [Manageability Transport Function Structure](#manageability-transport-function-structure).
    The [Transport Token](#transfer-token) is returned to the caller for further manageability packet
    transmit and receive.


* ### ***GetTransportCapability()***
    edk2 Manageability protocol driver invokes this function to get the capabilities
    of transport interface. The implementation of manageability transport library
    can optionally support advanced features such as multi-sessions on transport
    interface or asynchronous transfer.

* ### ***ReleaseTransportSession()***
    edk2 manageability protocol driver invokes this function to release the transport
    session.

  ### **Manageability Transport Function Structure**

  Below is the function structure initiated by the manageability transport library and
  returned in the transport token when caller invokes [AcquireTransportSession](#acquiretransportsession)
  function.


    ***MANAGEABILITY_TRANSPORT_FUNCTION*** is declared as an union to accommodate
    different versions of transport function set. This allows the function added to
    support the new functionalities in the future and also keep the backward compatibility.
    Developers can not modify the previous version of function structure, instead
    developers can create a new function structure with the increased version number
    that includes the previous version of function set and new added functions.

```
    typedef union {
      MANAGEABILITY_TRANSPORT_FUNCTION_V1_0  *Version1_0;
    } MANAGEABILITY_TRANSPORT_FUNCTION;

    struct _MANAGEABILITY_TRANSPORT_FUNCTION_V1_0 {
      MANAGEABILITY_TRANSPORT_INIT              TransportInit;
      MANAGEABILITY_TRANSPORT_STATUS            TransportStatus;
      MANAGEABILITY_TRANSPORT_RESET             TransportReset;
      MANAGEABILITY_TRANSPORT_TRANSMIT_RECEIVE  TransportTransmitReceive;
    };
```
* ***TransportInit()***

    Manageability protocol driver invokes this function to initial the transport
    interface with the optional hardware configuration of the transport interface.
    Whether the transport interface requires initialization or not depends on the
    transport interface characteristics. The hardware information is optional passed to
    this function if the manageability protocol driver would like to use the different
    hardware configuration of transport interface. For example, the different I/O
    address of KCS interface, or the memory mapped I/O KCS interface. The structure
    format of hardware information is created according to the hardware design of
    transport interface.

* ***TransportStatus()***

    Manageability protocol driver invokes this function to get the status of transport
    interface. How does this function report the status depends on the transport
    interface characteristics and the implementation of this function.

* ***TransportReset()***

    Manageability protocol driver can invoke this function to reset and recover the
    transport interface in case the error status is reported by the transport
    interface. Whether the transport interface can be reset or not depends on the
    transport interface characteristics and the implementation of this function.

* ***TransportTransmitReceive()***
    Manageability protocol driver invokes this function to transmit and/or receive the
    packet. Caller has to setup the [Transfer Token](#transfer-token) when invoke to
    this function.

  ### **Transfer Token**

```
    struct _MANAGEABILITY_TRANSFER_TOKEN {
      EFI_EVENT                                  ReceiveEvent;
      MANAGEABILITY_TRANSPORT_HEADER             TransmitHeader;
      UINT16                                     TransmitHeaderSize;
      MANAGEABILITY_TRANSPORT_TRAILER            TransmitTrailer;
      UINT16                                     TransmitTrailerSize;
      MANAGEABILITY_TRANSMIT_PACKAGE             TransmitPackage;
      MANAGEABILITY_RECEIVE_PACKAGE              ReceivePackage;
      EFI_STATUS                                 TransferStatus;
      MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  TransportAdditionalStatus;
    };
```

* ***ReceiveEvent***

    The optional EFI event is created to wait for the signal of response data
    readiness. The parameter is valid only if the transport interface supports
    asynchronous transfer. MANAGEABILITY_TRANSPORT_CAPABILITY_ASYNCHRONOUS_TRANSFER bit
    must be set in the returned *TransportCapability* parameter in
    [GetTransportCapability()](#gettransportcapability) to indicate the
    transport interface is capable for asynchronous transfer.

* ***TransmitHeader***

    The transmit header is different according to the disparate transport interfaces
    for the manageability protocol. For example, the transmit header is different when
    MCTP packet is transmitted over I2C and KCS. Having this header abstracted from
    manageability transport library to make the transport implementation agnostic to the
    manageability protocol specification.

* ***TransmitHeaderSize***

    This indicates the size of TransmitHeader.

* ***TransmitTrailer***

    The transmit trailer may be different according to the disparate transport
    interface for the manageability protocol. Having this header abstracted from
    manageability transport library to make the transport implementation agnostic to the
    manageability protocol specification.

* ***TransmitTrailerSize***

    This indicates the size of TransmitTrailer.

* ***TransmitPackage***

    The buffer of packet to transmit, this could be a value of NULL if the manageability
    protocol is going to harvest the datagram sent by the management entity.

* ***ReceivePackage***

    The receive buffer, this could be a value of NULL if the manageability
    protocol is going to send the request to management endpoint which has no response
    required.

* ***TransferStatus***

    In order to support both synchronous and asynchronous transfer with a unified
    function, ***TransportTransmitReceive()*** is designed as a
    non return value function. Instead, the status of transmit/receive is returned
    in the transfer token. When the asynchronous transfer, manageability transport
    library has to set the status in transfer token before signal ***ReceiveEvent***.

* ***TransportAdditionalStatus***

    The additional transport status after the transfer.

## Industry Standard Manageability Protocol edk2 Driver Stack
The edk2 implementation of industry manageability standards such as IpmiProtocol and
MctpProtocol abstract the command layer and provide the unified protocol interface to
build up the manageability packet for subordinate specifications. edk2 manageability
protocol driver is linked with desired manageability transport library base on the
platform design.

## Transport Implementation

   The manageability transport library could have the implementation in library or
   invoke the existing edk2 protocol which is defined in UEFI or PI specifications.
   This is the implementation decision made by the developer when introduce a new
   manageability transport library.

## IPMI KCS Register Addresses

Platforms may choose either supported address configuration:

- Set `PcdIpmiKcsIoBaseAddress` for a contiguous layout. When both explicit
  register PCDs are `0xFFFF`, data is at the base address and command/status is
  at base plus one. The default addresses remain `0xCA2/0xCA3`.
- Set both explicit register address PCDs as FixedAtBuild for an explicit layout.
  The explicit addresses are used directly and the base PCD is ignored.

Neither configuration is deprecated. For example, non-contiguous ports
`0x62/0x66` are configured as:

```text
[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdIpmiKcsIoDataAddress|0x62
  gEfiMdePkgTokenSpaceGuid.PcdIpmiKcsIoCommandAddress|0x66
```

Configuring only one explicit address, or specifying the same address for both
registers, fails the build. Command addresses below data addresses are supported.
`0xFFFF` is reserved for selecting the base-address configuration.

This selection applies to the DXE, PEI, SMM and Standalone MM IPMI modules,
the KCS transport's default hardware configuration, and the BmcAcpi SSDT.
Caller-provided KCS hardware information and MCTP address configuration are
unchanged.

BmcAcpi retains generic access to `PcdIpmiKcsIoBaseAddress`, preserving both
FixedAtBuild and PatchableInModule configurations. Base-address mode retains one
two-byte I/O resource whose base is patched at runtime. Explicit mode describes
two one-byte I/O resources generated at build time; the base PCD does not
modify them. ACPI OEM ID and OEM table ID PCDs remain generic inputs.

### IPMI BMC ACPI Namespace

`PcdIpmiBmcAcpiUseGenericNamespace` is a FixedAtBuild BOOLEAN PCD. Its `FALSE`
default preserves `\_SB.PC00.LPC0.IPMC`. Set it to `TRUE` in the platform DSC
to place the device directly under the system bus at `\_SB.IPMC`:

```text
[PcdsFixedAtBuild]
  gManageabilityPkgTokenSpaceGuid.PcdIpmiBmcAcpiUseGenericNamespace|TRUE
```

Only these two namespace placements are supported; the device name remains `IPMC`.
Namespace selection is independent of the KCS register address configuration.
The existing ASL preprocessing rules select matching `External` and `Scope`
paths at build time, without BaseTools changes or runtime namespace rewriting.
The default placement still requires the platform to provide `\_SB.PC00.LPC0`.
