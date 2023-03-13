\page mipi_syst_adapting_page Adapting the SyS-T Library

[TOC]

SyS-T Platform Code {#mipi_syst_adapting_platform}
==============================================================================
The SyS-T library uses a platform concept for integrating it into a system.
A platform is responsible for the following:

   * Providing the low level trace output code that interfaces with the
    underlying trace transport system. This is typically trace aggregating
    hardware or a protocol driver. The ``example`` platform code
    simply prints the SyS-T protocol output to stdout.

   * Providing platform hook functions for SyS-T global and handle state
     manipulations. This is platform specific code that allows efficient
     processing of SyS-T.

SyS-T ships with the following example platforms:

Platform Directory|  Description
------------|--------------------------------------------
example     | Simple trace output driver that prints all IOs as hexadecimal data strings to ``stdout``. The [printer](@ref mipi_syst_printer_page) project can consume this output and decode it into human readable text using comma separated value format (CSV).
mipi_stp_sim| [MIPI System Trace Protocol](https://mipi.org/specifications/stp) format file writer. It shows how to connect the library with an STP generator. In this example code, a software encoder is used to produce the STP data.
nop         | A minimal boilerplate platform implementation that allows the library to compile but doesn't produce any output.

Adapting SyS-T means providing your own platform code. The recommended procedure
is to clone an existing platform folder and adapt it as needed. The following
chapter shows an example of how this can be done.

Copy an Existing Platform {#mipi_syst_adapting_copy}
==============================================================================

Example:

     $ cd platform
     $ cp -rv example myplatform

Adapting the Platform {#mipi_syst_adapting_adapt}
==============================================================================

Modify the copied platfrom code to interface with the system by performing
the following changes

 * Implement the hook functions for state and handle initialisation and
   destruction.
 * Interface the data output writer code with the trace transport.
   The writer routine in the library assumes a MIPI STP protocol style
   output processor and calls a set of STP tailored macros named
   ```MIPI_SYST_OUTPUT_*``` for the actual write operations. These macros
   are defined in ```platform.h```. See #MIPI_SYST_OUTPUT_D32TS for an example
   macro definition to emit a SyS-T message header which will turn into a MIPI
   STP D32TS packet when using STP output. Alternatively replace the entire
   writer code in a case where the MIPI STP writer cannot be efficently
   interfaced with the transport.

Edit the Build Configuration {#mipi_syst_adapting_build}
==============================================================================
Define the CMake symbol ```SYST_BUILD_PLATFORM_NAME``` to point to the new
platform directory name. See the page @ref mipi_syst_building_page
for further information on building SyS-T.

Specify  the API conformance level by setting the CMAKE symbol
```SYST_CFG_CONFORMANCE_LEVEL``` to one of the following:

| Conformance level | Data Size                                              |
| ------------------|--------------------------------------------------------|
|  10               | Minimal - only Short and Compact Messages              |
|  20               | Low overhead - excluding  printf and crc32             |
|  30               | complete - All messages and options                    |

