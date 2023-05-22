# UefiPayloadPkg
Provide UEFI Universal Payload for different bootloader to generate EFI environment

# Spec
URL: https://docs.google.com/document/d/1WxEUlCsXpc17DkJhL3XVkOW7e_KIt_zcc9tQ1trOySg/

# Uefi UniversalPayload Format
  | SpecRevision     | Binary Format | HandOffPayload - HOB | HandOffPayload - FDT |
  |------------------|---------------|----------------------|----------------------|
  | 0.7              | ELF           | V (Default)          | X                    |
  | 0.9              | ELF           | V (Default)          | V                    |
  | 1.0              | FIT           | V                    | V (Default)          |

# Binary Format
  - SpecRevision - 0.7
    ```
                  +  +-----------------------+
                  |  | UniversalPayloadEntry | <----------- UefiPayloadPkg\UefiPayloadEntry\UniversalPayloadEntry.c:_ModuleEntryPoint (HOB)
                  |  +-----------------------+
                  |  | .upld_info            | patch by llvm-objcopy
    ELF Format    |  +-----------------------+
                  |  | .upld.uefi_fv         | patch by llvm-objcopy
                  |  +-----------------------+
                  |  | .upld.bds_fv          | patch by llvm-objcopy
                  |  +-----------------------+
                  |  | .upld.<afpx>_fv       | patch by llvm-objcopy
                  +  +-----------------------+
    ```

  - SpecRevision - 0.9
    ```
                  +  +-----------------------+
                  |  | UniversalPayloadEntry | <----------- UefiPayloadPkg\UefiPayloadEntry\UniversalPayloadEntry.c:_ModuleEntryPoint (HOB[Default] or FDT)
                  |  +-----------------------+
                  |  | .upld_info            | patch by llvm-objcopy
    ELF Format    |  +-----------------------+
                  |  | .upld.uefi_fv         | patch by llvm-objcopy
                  |  +-----------------------+
                  |  | .upld.bds_fv          | patch by llvm-objcopy
                  |  +-----------------------+
                  |  | .upld.<afpx>_fv       | patch by llvm-objcopy
                  +  +-----------------------+
    ```

  - SpecRevision - 1.0
    ```
                  +  +-----------------------+
    FIT Data      |  | FIT Header            | <----------- Generate by pylibfdt
                  +  +-----------------------+
    PECOFF Format |  | UniversalPayloadEntry | <----------- UefiPayloadPkg\UefiPayloadEntry\UniversalPayloadEntry.c:_ModuleEntryPoint (HOB or FDT[Default])
                  +  +-----------------------+
    Relocate Data |  | reloc-start           |
                  +  +-----------------------+
                  |  | uefi_fv               | patch it directly
                  |  +-----------------------+
    Multi Binary  |  | bds_fv                | patch it directly
                  |  +-----------------------+
                  |  | afp_xxx_fv            | patch it directly
                  |  +-----------------------+
                  |  | afp_xxx_fv            | patch it directly
                  +  +-----------------------+
    ```

# Environment
  - SpecRevision - 0.7
    ```
    Download and install https://github.com/llvm/llvm-project/releases/tag/llvmorg-10.0.1
    ```
  - SpecRevision - 0.9
    ```
    Download and install https://github.com/llvm/llvm-project/releases/tag/llvmorg-10.0.1
    ```
  - SpecRevision - 1.0
    - Windows
      ```powershell
      Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
      choco install dtc-msys2
      pip3 install pefile
      pip3 install swig
      pip3 install pylibfdt
      ```
    - Ubuntu
      ```bash
      sudo apt install -y u-boot-tools
      pip3 install pefile
      pip3 install swig
      pip3 install pylibfdt
      ```
# How to build UEFI UniversalPayload
  - Windows
    - edksetup Rebuild
  - Linux
    - make -C BaseTools
    - source edksetup.sh

  - SpecRevision - 0.7
    - python UefiPayloadPkg/UniversalPayloadBuild.py -t <TOOL_CHAIN_TAG> -s 0.7
    - llvm-objdump -h Build/UefiPayloadPkgX64/UniversalPayload.elf
  - SpecRevision - 0.9
    - python UefiPayloadPkg/UniversalPayloadBuild.py -t <TOOL_CHAIN_TAG> -s 0.9
    - llvm-objdump -h Build/UefiPayloadPkgX64/UniversalPayload.elf
  - SpecRevision - 1.0
    - python UefiPayloadPkg/UniversalPayloadBuild.py -t <TOOL_CHAIN_TAG> -s 1.0
    - fdtdump Build/UefiPayloadPkgX64/UniversalPayload.fit

# Edk2boot + UefiUniversalPayload
Currently, Spec 0.7 Edk2boot use below way to support compress and sign.

- Spec 0.7 Behavior - Edk2boot + UefiUniversalPayload.elf
  ```
  Boot Flow
  +-------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+-------------------+
  | Platform Init                                                                       | Universal Loader Interface                                                                                | OS                |
  +-------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+-------------------+
                                                                                                                                                                     HOBs
  SEC -> PEI -> DXE -> DXE IPL -> UefiPayloadPkg\PayloadLoaderPeim\PayloadLoaderPeim.c ------------------------------------------------------------------------------------> Load UniversalPayload.elf -> Operation System


  | Platform Initialize - Edk2                                                                                                                                                                      | UniversalPayload - Edk2        |
  +-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+--------------------------------+

  Binary Format

  +-------------------+
  | BIOS.rom          |
  +-------------------+
  | Other Firmware    |
  +-------------------+
  | ...               |  FMMT                                                                                                                                                                        UniversalPayloadBuild.py -s 0.7
  +-------------------+<----------------+-----------------------+  GenFfs    +-----------------------+  Rsa2048Sha256 Sign +-----------------------+  LzmaCompress +----------------------+  GenSec +--------------------------------+
  |                   |                 | EDK2 FFS Header       |<-----------| Rsa2048Sha256 Hash    |<--------------------| UniversalPayload.lzma |<--------------| EDK2 SEC Header      |<--------| UniversalPayload.elf           |
  | RAW Data          |                 +-----------------------+            +-----------------------+                     +-----------------------+               +----------------------+         +--------------------------------+
  |                   |                 | Rsa2048Sha256 Hash    |            | UniversalPayload.lzma |                                                             | UniversalPayload.elf |         | upld_info                      |
  |                   |                 +-----------------------+            +-----------------------+                                                             +----------------------+         +--------------------------------+
  |                   |                 | UniversalPayload.lzma |                                                                                                  | upld_info            |         | upld.uefi_fv                   |
  +-------------------+<----------------+-----------------------+                                                                                                  +----------------------+         +--------------------------------+
  | ...               |                                                                                                                                            | upld.uefi_fv         |         | upld.bds_fv                    |
  +-------------------+                                                                                                                                            +----------------------+         +--------------------------------+
  | Other Firmware    |                                                                                                                                            | upld.bds_fv          |         | upld.AFP1                      |
  +-------------------+                                                                                                                                            +----------------------+         +--------------------------------+
                                                                                                                                                                   | upld.AFP1            |         | upld.AFP2                      |
                                                                                                                                                                   +----------------------+         +--------------------------------+
                                                                                                                                                                   | upld.AFP2            |         | ...                            |
                                                                                                                                                                   +----------------------+         +--------------------------------+
                                                                                                                                                                   | ...                  |         | upld.AFPn                      |
                                                                                                                                                                   +----------------------+         +--------------------------------+
                                                                                                                                                                   | upld.AFPn            |
                                                                                                                                                                   +----------------------+
  ```

Expected, Spec 1.0 Edk2boot use below way to support compress and sign
- Spec 1.0 Behavior - Edk2boot + UefiUniversalPayload.fit
  ```
  Boot Flow
  +-------------------------------------------------------------------------------------+------------------------------------------------------------------------+-------------------+
  | Platform Init                                                                       | Universal Loader Interface                                             | OS                |
  +-------------------------------------------------------------------------------------+------------------------------------------------------------------------+-------------------+
                                                                                                      HOBs or *FDT[Default]
  SEC -> PEI -> DXE -> DXE IPL -> *UefiPayloadPkg\PayloadLoaderPeim\PayloadLoaderPeim.c ----------------------------------------------> Load UniversalPayload.fit -> Operation System

  Binary Format

  | Platform Initialize - Edk2                                                                                                | UniversalPayload - Edk2 (UniversalPayloadBuild.py -s 1.0)                               |
  +---------------------------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------+

  +-------------------+
  | BIOS.rom          |
  +-------------------+
  | Other Firmware    |
  +-------------------+
  | ...               |  FMMT                                                                                                  UniversalPayloadBuild.py -s 1.0   tianocore -> data-offset
  +-------------------+<----------------+--------------------------------+  GenFfs +--------------------------------+  GenSec +--------------------------------+ tianocore -> reloc-start +--------------------------+
  |                   |                 | EDK2 FFS Header                |<--------| EDK2 SEC Header                |<--------| FIT Header                     |<-------------------------| UniversalPayload.pecoff  |
  |                   |                 +--------------------------------+         +--------------------------------+         | compatible="universal-payload";|                          +--------------------------+
  |                   |                 | EDK2 SEC Header                |         | FIT Header                     |         | upl-version <0x0100>;          |
  | RAW Data          |                 +--------------------------------+         |                                |         | images {                       | uefi-fv -> data-offset   +--------------------------+
  |                   |                 | FIT Header                     |         |                                |         |   tianocore {...};             |<-------------------------| uefi_fv                  |
  |                   |                 |                                |         +--------------------------------+         |   uefi-fv {...};               | bds-fv -> data-offset    +--------------------------+
  |                   |                 |                                |         | tianocore -> data              |         |   bds-fv {...};                |<-------------------------| bds_fv                   |
  |                   |                 +--------------------------------+         +--------------------------------+         |   afp1-fv {...};               | AFP1 -> data-offset      +--------------------------+
  |                   |                 | tianocore -> data              |         | tianocore -> reloc-start       |         |   ...                          |<-------------------------| AFP1                     |
  |                   |                 +--------------------------------+         +--------------------------------+         |   afpn-fv {...};               | AFP2 -> data-offset      +--------------------------+
  |                   |                 | tianocore -> reloc-start       |         | uefi-fv -> data                |         | }                              |<-------------------------| AFP2                     |
  |                   |                 +--------------------------------+         +--------------------------------+         | configurations {               | ...                      +--------------------------+
  |                   |                 | uefi-fv -> data                |         | bds-fv -> data                 |         |   conf-1 {...}                 |<-------------------------| ...                      |
  |                   |                 +--------------------------------+         +--------------------------------+         | }                              | AFPn -> data-offset      +--------------------------+
  |                   |                 | bds-fv -> data                 |         | AFP1-fv -> data                |         |                                |<-------------------------| AFPn                     |
  |                   |                 +--------------------------------+         +--------------------------------+         |                                |                          +--------------------------+
  |                   |                 | AFP1-fv -> data                |         | AFP2-fv -> data                |         |                                |
  |                   |                 +--------------------------------+         +--------------------------------+         +--------------------------------+
  |                   |                 | AFP2-fv -> data                |         | ...                            |         | tianocore -> data              |
  |                   |                 +--------------------------------+         +--------------------------------+         +--------------------------------+
  |                   |                 | ...                            |         | AFPn-fv -> data                |         | tianocore -> reloc-start       |
  |                   |                 +--------------------------------+         +--------------------------------+         +--------------------------------+
  |                   |                 | AFPn-fv -> data                |                                                    | uefi-fv -> data                |
  +-------------------+<----------------+--------------------------------+                                                    +--------------------------------+
  | ...               |                                                                                                       | bds-fv -> data                 |
  +-------------------+                                                                                                       +--------------------------------+
  | Other Firmware    |                                                                                                       | AFP1-fv -> data                |
  +-------------------+                                                                                                       +--------------------------------+
                                                                                                                              | AFP2-fv -> data                |
                                                                                                                              +--------------------------------+
                                                                                                                              | ...                            |
                                                                                                                              +--------------------------------+
                                                                                                                              | AFPn-fv -> data                |
                                                                                                                              +--------------------------------+

  ```
