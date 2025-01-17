# UefiPayloadPkg
Provide UEFI Universal Payload for different bootloader to generate EFI environment

# Spec
- UniversalPayload URL: https://universalscalablefirmware.github.io/documentation/2_universal_payload.html
- UniversalPayload URL: https://universalpayload.github.io/spec/
- ELF Format URL: https://refspecs.linuxfoundation.org/elf/elf.pdf
- FIT Format URL: https://universalpayload.github.io/spec/chapter2-payload-image-format.html

# Uefi UniversalPayload Format
  | Binary Format | HandOffPayload - HOB |
  |---------------|----------------------|
  | ELF           | V (Default)          |
  | FIT           | V                    |

# Binary Format
  - ELF
    ```
                  +  +-----------------------+
                  |  | UniversalPayloadEntry | <----------- UefiPayloadPkg\UefiPayloadEntry\UniversalPayloadEntry.c:_ModuleEntryPoint (HOB)
                  |  +-----------------------+
                  |  | .upld_info            | patch it directly
    ELF Format    |  +-----------------------+
                  |  | .upld.uefi_fv         | patch it directly
                  |  +-----------------------+
                  |  | .upld.bds_fv          | patch it directly
                  |  +-----------------------+
                  |  | .upld.<afpx>_fv       | patch it directly
                  +  +-----------------------+
    ```

  - FIT
    ```
                  +  +-----------------------+
    FIT Data      |  | FIT Header            | <----------- Generate by pylibfdt
                  +  +-----------------------+
    PECOFF Format |  | UniversalPayloadEntry | <----------- UefiPayloadPkg\UefiPayloadEntry\FitUniversalPayloadEntry.c:_ModuleEntryPoint (HOB)
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

# Build Environment
  - ELF
    ```
    Install GCC compiler on linux and MSVC compiler on windows
    Install CLANG compiler https://github.com/llvm/llvm-project/releases/tag/llvmorg-10.0.1 on windows and linux
    ```
  - FIT
    ```
    Install GCC compiler on linux and MSVC compiler on windows
    pip3 install pefile
    pip3 install pylibfdt
    ```

# How to build UEFI UniversalPayload
  - Windows
    - edksetup Rebuild
  - Linux
    - make -C BaseTools
    - source edksetup.sh

  - UniversalPayload.elf
    - python UefiPayloadPkg/UniversalPayloadBuild.py -t <TOOL_CHAIN_TAG>
    - llvm-objdump -h Build/UefiPayloadPkgX64/UniversalPayload.elf

  - UniversalPayload.fit
    - python UefiPayloadPkg/UniversalPayloadBuild.py -t <TOOL_CHAIN_TAG> --Fit

# How to dump payload binary data
  - UniversalPayload.elf
    - Install elf dump tools https://github.com/llvm/llvm-project/releases/tag/llvmorg-10.0.1
    - llvm-objdump -h Build/UefiPayloadPkgX64/UniversalPayload.elf

  - UniversalPayload.fit
    - Install fdtdump tool
      - Windows
        ```powershell
        Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
        choco install dtc-msys2
        ```
      - Linux
        ```bash
        sudo apt install -y u-boot-tools
        ```

    - fdtdump Build/UefiPayloadPkgX64/UniversalPayload.fit

# Edk2boot + UefiUniversalPayload
ELF Edk2boot use below way to support compress and sign.

- ELF Behavior - Edk2boot + UefiUniversalPayload.elf
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
  | ...               |  FMMT                                                                                                                                                                        UniversalPayloadBuild.py
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

FIT Edk2boot use below way to support compress and sign
- FIT Behavior - Edk2boot + UefiUniversalPayload.fit
  ```
  Boot Flow
  +-------------------------------------------------------------------------------------+------------------------------------------------------------------------+-------------------+
  | Platform Init                                                                       | Universal Loader Interface                                             | OS                |
  +-------------------------------------------------------------------------------------+------------------------------------------------------------------------+-------------------+
                                                                                                      HOBs
  SEC -> PEI -> DXE -> DXE IPL -> *UefiPayloadPkg\PayloadLoaderPeim\PayloadLoaderPeim.c ----------------------------------------------> Load UniversalPayload.fit -> Operation System

  Binary Format

  | Platform Initialize - Edk2                                                                                                | UniversalPayload - Edk2 (UniversalPayloadBuild.py --Fit)                                |
  +---------------------------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------+

  +-------------------+
  | BIOS.rom          |
  +-------------------+
  | Other Firmware    |
  +-------------------+
  | ...               |  FMMT                                                                                                  UniversalPayloadBuild.py --Fit    tianocore -> data-offset
  +-------------------+<----------------+--------------------------------+  GenFfs +--------------------------------+  GenSec +--------------------------------+ tianocore -> reloc-start +--------------------------+
  |                   |                 | EDK2 FFS Header                |<--------| EDK2 SEC Header                |<--------| FIT Header                     |<-------------------------| UniversalPayload.pecoff  |
  |                   |                 +--------------------------------+         +--------------------------------+         | description = "Uefi Payload";  |                          +--------------------------+
  |                   |                 | EDK2 SEC Header                |         | FIT Header                     |         | ...                            |
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
