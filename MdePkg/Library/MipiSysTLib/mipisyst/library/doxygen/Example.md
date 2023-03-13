\page mipi_syst_example_page Instrumentation API Examples

[TOC]

SyS-T ships with various example applications that show how to use
instrumentation calls. The examples are stored inside the examples
directory of the distribution file set. A CMake script is provided for
building the examples on CMake supported platforms. To build the
examples without CMake, add the SyS-T include directory to the list of
include directories for the compiler being used and tell the linker
to link against the SyS-T library.

Example for building a SyS-T example on Linux:

      $ export MIPI_SYST_SDK=<enter path to SyS-T-SDK here>
      $ gcc -o hello hello.c -I$MIPI_SYST_SDK/include -L$MIPI_SYST_SDK/lib -lmipi_syst


SyS-T "hello world" Style Application
===========================================================================
This example is stored in the examples/hello directory of the SyS-T
distribution.

Example source code:

\code{.c}
#include "mipi_syst.h"   /* SyS-T definitions     */


/* Define origin used by this example as the message client ID
 */
static const struct mipi_syst_origin origin =
MIPI_SYST_GEN_ORIGIN_GUID(0x494E5443, 0xA2AE, 0x4C70, 0xABB5, 0xD1A79E9CEA35, 1);

int main(int argc, char* argv[])
{
    struct mipi_syst_handle* systh;

    /* Initialize a SyS-T output handle structure
     */
    systh = MIPI_SYST_ALLOC_HANDLE( &origin );

    /* Send a string message with payload "Hello World!"
     */
    MIPI_SYST_DEBUG(systh, MIPI_SYST_SEVERITY_INFO, "Hello world!" , /*length*/ 12);

    /* Release any resources associated with this SyS-T handle.
     */
    MIPI_SYST_DELETE_HANDLE(systh);

    return 0;
}
\endcode

The example will produce output like that below if the ``example`` platform is used.
It shows which MIPI STP protocol packets are created to transport the
SYS-T message data.

```
STP Protocol Output:
  0 <D32TS>  01800242               // SyS-T header
  1 <D64>    704caea243544e49       // GUID part #1
  2 <D64>    35ea9c9ea7d1b5ab       // GUID part #2
  3 <D64>    6f77206f6c6c6548       // Payload part #1
  4 <D32>    21646c72               // Payload part #2
  5 <FLAG>                          // end of record
SYS-T RAW DATA: 42108001494E5443A2AE4C70ABB5D1A79E9CEA3548656C6C6F20776F726C6421
```
