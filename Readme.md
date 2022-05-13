# Fmmt & Fce Tools
## Overview
This branch is for holding the Firmware Module Management Tool(FMMT) and Firmware Configuration Editor(FCE) tools.
This branch is based on the edk2 SHA-1: 0e31124877cc8bc0140a03ad3196f0d58b2fd966 and it's for resolving the merge conflict issues.

The branch owner: Bob Feng <bob.c.feng@intel.com>

## Feature introduction

### FCE Overview
Firmware Configuration Editor (FCE) enables you to retrieve and
change HII configuration ("Setup") data in Firmware Device (*.fd) files.

The "*.fd" file must be built using the BaseTools in order for the Intel(R) FCE
tool to successfully locate the HII data.  The BaseTools detect the offset of the 
HII data stored as global data in the PE/COFF image at build time, and then stores 
it in a newly created raw section of FFS file.

This tool only manipulates uncompressed versions of EFI_VARSTORE_IFR and 
EFI_VARSTORE_IFR_EFI and it only supports the following question types: CHECKBOX, 
ORDERED_LIST, ONE_OF, NUMERIC and STRING.

### FMMT Overview
The Firmware Device is a persistent physical repository that contains firmware code and/or data. The firmware code and/or data stored in Firmware Volumes. In firmware development, binary file has its firmware layout following the Platform-Initialization Specification. Thus, operation on FV file / FFS file (Firmware File) is an efficient and convenient way for firmware function testing and developing. FMMT Python tool is used for firmware files operation.
The FMMT tool is capable of:
- Parse a FD (Firmware Device) / FV (Firmware Volume) / FFS (Firmware Files)
- Add a new FFS into a FV file (both included in a FD file or not)
- Replace an FFS in a FV file with a new FFS file
- Delete an FFS in a FV file (both included in a FD file or not)
- Extract the FFS from a FV file (both included in a FD file or not)

## Timeline
Target for 2022 Q3