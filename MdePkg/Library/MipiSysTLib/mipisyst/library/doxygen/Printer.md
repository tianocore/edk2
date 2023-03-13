\page mipi_syst_printer_page Instrumentation API Examples

[TOC]

SyS-T Protocol Printer {#mipi_syst_printer}
==============================================================================
The project includes a SyS-T data protocol pretty printer tool
in the printer subdirectory. The printer is a standalone application
written in C++11. It supports reading the output from instrumented
applications using the example platform from the SyS-T
instrumentation library.
The tool scans the output for lines starting with ``SYS-T RAW DATA:`` and
converts the hex dumps into binary data for decoding. The printer can
be easily adapted to real trace data transports by replacing the code
in ``printer/src/mipi_syst_main.cpp`` with an appropriate data reader.

Building the Printer
------------------------------------------------------------------------------
The following transcript shows how to build the printer on a Linux console.
The printer is a standalone application and independent from the
instrumentation library or example projects.

```
$ cmake ../../sys-t/printer
-- The C compiler identification is GNU 5.4.0
-- The CXX compiler identification is GNU 5.4.0
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done
-- Generating done
-- Build files have been written to: /users/mipi/prj/syst_build/printer

$ make
Scanning dependencies of target systprint
[ 14%] Building CXX object CMakeFiles/systprint.dir/src/mipi_syst_main.cpp.o
[ 28%] Building CXX object CMakeFiles/systprint.dir/src/mipi_syst_collateral.cpp.o
[ 42%] Building CXX object CMakeFiles/systprint.dir/src/mipi_syst_printf.cpp.o
[ 57%] Building CXX object CMakeFiles/systprint.dir/src/mipi_syst_decode.cpp.o
[ 71%] Building CXX object CMakeFiles/systprint.dir/src/mipi_syst_message.cpp.o
[100%] Linking CXX executable systprint
[100%] Built target systprint
```

Testing the Printer
------------------------------------------------------------------------------
The printer project comes with a self test feature. The ``printer/test``
directory contains reference input and output files collected using the
``example/client`` example application. To run the printer test use the
following command (or the cmake test driver command ``ctest``) in the
printer build directory:

```
$ make test
Running tests...
Test project /users/mipi/prj/syst_build/printer
    Start 1: print_client_example
1/3 Test #1: print_client_example ...............   Passed    0.01 sec
    Start 2: diff_output_with_32bit_reference
2/3 Test #2: diff_output_with_32bit_reference ...   Passed    0.04 sec
    Start 3: diff_output_with_64bit_reference
3/3 Test #3: diff_output_with_64bit_reference ...   Passed    0.03 sec

100% tests passed, 0 tests failed out of 3

Total Test time (real) =   0.11 sec
```

To actually see the printer output, run the printer directly using command
line arguments, or indirectly through the test driver in verbose mode
(```ctest --verbose```). The following transcript shows how to call the
printer directly:

```
$systprint --short_guid {494E5443-8A9C-4014-A65A-2F36A36D96E4} --collateral ../../sys-t/printer/test/collateral.xml ../../sys-t/printer/test/input_client64.txt

Decode Status,Payload,Type,Severity,Origin,Unit,Message TimeStamp,Context TimeStamp,Location,Raw Length,Checksum,Collateral
OK,"0x0000000000010000 version banner string",BUILD:LONG,MAX,example,1,0x00054A4B376A70E9,0x0000000000000000,,62,0x4DDEF5B9,../../sys-t/printer/test/collateral.xml
OK,"SyS-T Library version 1.0.0",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000001,./systclient.c:64,48,0x7A34B527,../../sys-t/printer/test/collateral.xml
OK,"+-------------------------------------------------------+",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000002,./othersource.c:40,36,0x7CBB44B6,../../sys-t/printer/test/collateral.xml
OK,"|               ____         _____   _______            |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000003,./othersource.c:41,36,0x2761EBF4,../../sys-t/printer/test/collateral.xml
OK,"|              / ___|       / ____| |__   __|           |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000004,./othersource.c:42,36,0x55C63EAB,../../sys-t/printer/test/collateral.xml
OK,"|             | |___  __  _| |___ _____| |              |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000005,./othersource.c:43,36,0xE3885FB4,../../sys-t/printer/test/collateral.xml
OK,"|              \___ \| | | |\___ \_____| |              |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000006,./othersource.c:44,36,0x4C13A7F5,../../sys-t/printer/test/collateral.xml
OK,"|              ____| | |_| |____| |    | |              |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000007,./othersource.c:45,36,0xE2C8BDC2,../../sys-t/printer/test/collateral.xml
OK,"|             |_____/ \__| |_____/     |_|              |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000008,./othersource.c:46,36,0xD0734297,../../sys-t/printer/test/collateral.xml
OK,"|                      _/ /                             |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x0000000000000009,./othersource.c:47,36,0x6D704426,../../sys-t/printer/test/collateral.xml
OK,"|                     |__/                              |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x000000000000000A,./othersource.c:48,36,0x0A8FD609,../../sys-t/printer/test/collateral.xml
OK,"+-------------------------------------------------------+",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x000000000000000B,./othersource.c:49,36,0x1E99CD8F,../../sys-t/printer/test/collateral.xml
OK,"|    catalog Format  |         Printed Result           |",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A70E9,0x000000000000000C,./othersource.c:231,36,0xA17B5C1C,../../sys-t/printer/test/collateral.xml
OK,"|---------------------------------strings---------------|",CATALOG:ID32P64,INFO,example,1,0x00054A4B376A74D1,0x000000000000000D,./othersource.c:232,36,0x11A215E6,../../sys-t/printer/test
(...)
$
```