# IPMI Blob Transfer Interface Driver

This DXE module is a UEFI implementation of the Phorphor Blob Transfer Interface defined in OpenBMC
https://github.com/openbmc/phosphor-ipmi-blobs

## OpenBMC implements this interface as a protocol, allowing UEFI and BMC to transfer blobs over IPMI.

### Usage:
Any DXE module that wishes to use this protocol should do the following:
1) The module should have a dependency on gEdkiiIpmiBlobTransferProtocolGuid in its inf "Depex" section
2) The module should list gEdkiiIpmiBlobTransferProtocolGuid in its inf "Protocol" section
3) The module's entry point should do a LocateProtocol on gEdkiiIpmiBlobTransferProtocolGuid

### A sample flow of protocol usage is as follows:
1) A call to IpmiBlobTransferOpen ()
2) Iterative calls to IpmiBlobTransferWrite
3) A call to IpmiBlobTransferClose ()

### Unit Tests:
IpmiBlobTransferDxe/UnitTest/ contains host based unit tests of this implementation.
Any changes to IpmiBlobTransferDxe should include proof of successful unit tests.

### Debugging
To assist in debugging any issues, change BLOB_TRANSFER_DEBUG to desired debug level, such as DEBUG_ERROR or DEBUG_INFO.
