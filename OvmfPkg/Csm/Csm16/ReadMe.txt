This module allows a CSM16 binary to be easily included
in the OVMF.fd output file.

=== How to use Csm16.inf ===

1. Copy the CSM16 binary to OvmfPkg/Csm/Csm16/Csm16.bin
2. Build OVMF with CSM_ENABLE defined.

   For example:
   * build -D CSM_ENABLE, or
   * OvmfPkg/build.sh -D CSM_ENABLE

