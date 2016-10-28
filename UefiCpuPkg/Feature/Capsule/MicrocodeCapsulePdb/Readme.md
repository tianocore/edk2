# How to generate Microcode FMP from Microcode PDB file

1) Copy directory `UefiCpuPkg/Feature/Capsule/MicrocodeUpdatePdb` to `<Your Platform Package>/MicrocodeUpdatePdb`.

2) Uncomment and update `FILE DATA` statement in `<Your Platform Package>/MicrocodeUpdatePdb/MicrocodeCapsulePdb.fdf` with path to a Microcode PDB file.  The PDB file can placed in `<Your Platform Package>/MicrocodeUpdatePdb` or any other path.

`FILE DATA = <your Microcode PDB file path>`

Uncomment and update `PLATFORM_NAME`, `FLASH_DEFINITION`, `OUTPUT_DIRECTORY` section in `<Your Platform Package>/MicrocodeUpdatePdb/MicrocodeCapsulePdb.dsc` with <Your Platform Package>.

    PLATFORM_NAME                  = <Your Platform Package>
    FLASH_DEFINITION               = <Your Platform Package>/MicrocodeCapsulePdb/MicrocodeCapsulePdb.fdf
    OUTPUT_DIRECTORY               = Build/<Your Platform Package>

3) Use EDK II build tools to generate the Microcode FMP Capsule

`build -p <Your Platform Package>/MicrocodeCapsulePdb/MicrocodeCapsulePdb.dsc`

4) The Microcode FMP Capsule is generated at `$(WORKSPACE)/$(OUTPUT_DIRECTORY)/$(TARGET)_$(TOOL_CHAIN_TAG)/FV/MicrocodeCapsule.Cap`

