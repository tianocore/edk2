# **Platform Runtime Mechanism Sample Modules**

The PRM module samples provided here serve as focused examples of how to perform various tasks in a PRM module. The
samples can also be used to verify the basic infrastructure needed in your firmware implementation is working as
expected by checking that the sample modules are found properly and the handlers perform their tasks as noted.

## **IMPORTANT NOTE**

> The sample modules have currently only been tested on the Visual Studio compiler tool chain. Sample module
build may fail on other tool chains. A future work item is to enable broader build support.

## How to Build PRM Sample Modules

The sample modules are built as part of the normal `PrmPkg` build so you can follow the
[package build instructions](../../Readme.md#how-to-build-prmpkg) and then find the PRM sample binaries in your
workspace build output directory. For example, if your build workspace is called "edk2" and you build
64-bit binaries on the Visual Studio 2017 tool chain, your sample module binaries will be in the following
location: \
``edk2/Build/Prm/DEBUG_VS2017/X64/PrmPkg/Samples``

### Build an Individual PRM Sample Module

Note that the build command does provide the option to build a specific module in a package which can result in
faster build time. If you would like to just build a single PRM module that can be done by specifying the path to
the module INF file with the "-m" argument to `build`. For example, this command builds 32-bit and 64-bit binaries
with Visual Studio 2019: \
``build -p PrmPkg/PrmPkg.dsc -m PrmPkg/Samples/PrmSampleContextBufferModule/PrmSampleContextBufferModule.inf -a IA32 -a X64 -t VS2019``

## PRM Sample Module User's Guide

The following table provides an overview of each sample module provided. By nature, different PRM handlers have
different requirements. The information here is summarized for a user to understand how to use a given sample
PRM handler along with GUID/name information to identify the sample PRM modules and their PRM handlers.

It is recommended that all PRM authors write a similar set of documentation for their users to better understand
and interact with their PRM modules.

---

### Module: PRM Sample ACPI Parameter Buffer

>* Name: `PrmSampleAcpiParameterBufferModule`
>* GUID: `dc2a58a6-5927-4776-b995-d118a27335a2`
> * Purpose:
>   * Provides an example of how to configure an ACPI parameter buffer

**Handlers:**

#### Handler: Check Parameter Buffer PRM Handler

* Name: `CheckParamBufferPrmHandler`
* GUID: `2e4f2d13-6240-4ed0-a401-c723fbdc34e8`
* Actions:
  * Checks for the data signature ‘T’, ‘E’, ‘S’, ‘T’ (DWORD) at the beginning of the parameter buffer.

* Parameter Buffer Required: Yes
* Parameter Buffer Contents:
  * A data signature of ['T', 'E', 'S', 'T'] (DWORD) at the beginning of the buffer.

* Context Buffer Required: No

* Runtime MMIO Range(s) Required: No

### Module: PRM Sample Context Buffer

>* Name: `PrmSampleContextBufferModule`
>* GUID: `5a6cf42b-8bb4-472c-a233-5c4dc4033dc7`
> * Purpose:
>   * Provides an example of how to configure a static data buffer (which is pointed to in a context buffer) in
      firmware and consume the buffer contents at runtime

**Handlers:**

#### Handler: Check Static Data Buffer PRM Handler

* Name: `CheckStaticDataBufferPrmHandler`
* GUID: `e1466081-7562-430f-896b-b0e523dc335a`
* Actions:
  * Checks that the context buffer signature and static data buffer signature match in the context buffer provided.

* Parameter Buffer Required: No

* Context Buffer Required: Yes
  * Static Data Buffer Contents:

  ```c
  #define   SOME_VALUE_ARRAY_MAX_VALUES   16

  typedef struct {
    BOOLEAN       Policy1Enabled;
    BOOLEAN       Policy2Enabled;
    UINT8         SomeValueArray[SOME_VALUE_ARRAY_MAX_VALUES];
  } STATIC_DATA_SAMPLE_CONTEXT_BUFFER_MODULE;
  ```

* Runtime MMIO Range(s) Required: No

### Module: PRM Sample Hardware Access Buffer

>* Name: `PrmSampleHardwareAccessModule`
>* GUID: `0ef93ed7-14ae-425b-928f-b85a6213b57e`
> * Purpose:
>   * Demonstrate access of several types of hardware resources from a PRM module

**Handlers:**

#### Handler: MSR Access Microcode Signature PRM Handler

* Name: `MsrAccessMicrocodeSignaturePrmHandler`
* GUID: `2120cd3c-848b-4d8f-abbb-4b74ce64ac89`
* Actions:
  * Access the loaded microcode signature at MSR 0x8B.

* Parameter Buffer Required: No

* Context Buffer Required: No

* Runtime MMIO Range(s) Required: No

#### Handler: MSR Access MTRR Dump PRM Handler

* Name: `MsrAccessMtrrDumpPrmHandler`
* GUID: `ea0935a7-506b-4159-bbbb-48deeecb6f58`
* Actions:
  * Access the fixed and variable MTRR values using MSRs.

* Parameter Buffer Required: No

* Context Buffer Required: No

* Runtime MMIO Range(s) Required: No

#### Handler: HPET MMIO Access PRM Handler

* Name: `MmioAccessHpetPrmHandler`
* GUID: `1bd1bda9-909a-4614-9699-25ec0c2783f7`
* Actions:
  * Access some HPET registers using MMIO at 0xFED00000.

* Parameter Buffer Required: No

* Context Buffer Required: No

* Runtime MMIO Range(s) Required: Yes
  * Physical Base Address: 0xFED00000
  * Length: 0x1000
