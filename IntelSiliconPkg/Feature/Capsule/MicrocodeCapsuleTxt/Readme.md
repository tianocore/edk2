# How to generate Microcode FMP from Microcode TXT file

1) Copy directory `UefiCpuPkg/Feature/Capsule/MicrocodeUpdateTxt` to `<Your Platform Package>/MicrocodeUpdateTxt`

2) Copy microcode TXT file to`<Your Platform Package>/MicrocodeUpdateTxt/Microcode`

3) Uncomment and update statement in `[Sources]` section of `<Your Platform Package>/MicrocodeUpdateTxt/Microcode/Microcode.inf` with name of Microcode TXT file copied in previous step.

    [Sources]
    <Your Microcode TXT file>

Uncomment and update `FILE DATA` statement in `<Your Platform Package>/MicrocodeUpdateTxt/MicrocodeCapsuleTxt.fdf` with path to a Microcode MCB file.  The MCB file is placed in `$(WORKSPACE)/$(OUTPUT_DIRECTORY)/$(TARGET)_$(TOOL_CHAIN_TAG)/IA32/<Your Platform Package>/MicrocodeUpdateTxt/Microcode/Microcode/OUTPUT/`.

`FILE DATA = <your Microcode MCB file path>`

Uncomment and update `PLATFORM_NAME`, `FLASH_DEFINITION`, `OUTPUT_DIRECTORY` section in `<Your Platform Package>/MicrocodeUpdateTxt/MicrocodeCapsuleTxt.dsc` with <Your Platform Package>.

    PLATFORM_NAME                  = <Your Platform Package>
    FLASH_DEFINITION               = <Your Platform Package>/MicrocodeCapsuleTxt/MicrocodeCapsuleTxt.fdf
    OUTPUT_DIRECTORY               = Build/<Your Platform Package>

Uncomment and update statement in `Components` section of `<Your Platform Package>/MicrocodeUpdateTxt/MicrocodeCapsuleTxt.dsc` with path to a Microcode INF file.

    [Components]
    <Your Microcode INF file>

4) Use EDK II build tools to generate the Microcode FMP Capsule

`build -p <Your Platform Package>/MicrocodeCapsuleTxt/MicrocodeCapsuleTxt.dsc`

5) The generated Microcode FMP Capsule is found at `$(WORKSPACE)/$(OUTPUT_DIRECTORY)/$(TARGET)_$(TOOL_CHAIN_TAG)/FV/MicrocodeCapsule.Cap`


