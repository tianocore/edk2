\page mipi_syst_api_overview Instrumentation API Overview

[TOC]

Overview               {#mipi_syst_api}
==============================================================================
The SyS-T instrumentation API is exposed as a set of CPP Macros with the
prefix `MIPI_SYST_`. The definitions for the instrumentation API are provided by
the C-Language header file `mipi_syst.h`. This is the only header needed for
adding instrumentation calls to your software.

See the @ref mipi_syst_example_page for a `hello world` style example that uses
the instrumentation API.

SyS-T Library States {#mipi_syst_api_state}
==============================================================================

The SyS-T library has 2 levels of data states to store information. These are:

1. global state (mipi_syst_header structure)
2. per handle state (mipi_syst_handle structure)

The global state contains data for the operation of the library itself and
is shared between SyS-T handles. The global state is initialized and
destroyed using the #MIPI_SYST_INIT and #MIPI_SYST_SHUTDOWN functions.

SyS-T can operate with multiple global state structures. In this case the
initialization functions with the "_STATE" suffix in their names must be used
to specify the intended state (See #MIPI_SYST_INIT_STATE,
#MIPI_SYST_SHUTDOWN_STATE, #MIPI_SYST_INIT_HANDLE_STATE and #MIPI_SYST_ALLOC_HANDLE_STATE).

The handle state is unique for each SyS-T
handle and allows sending trace data without causing data races with other
SyS-T handles.

SyS-T Handles {#mipi_syst_api_handles}
==============================================================================
Each message generating instrumentation API expects a pointer to a `SyS-T handle`
as the first parameter. This handle builds the connection between the
Software API and trace emitting hardware or driver code. It can best be seen
as something similar to a FILE * in traditional C/C++ file IO.

A SyS-T handle is obtained by using one of the #MIPI_SYST_ALLOC_HANDLE,
#MIPI_SYST_ALLOC_HANDLE_STATE, #MIPI_SYST_INIT_HANDLE, or
#MIPI_SYST_INIT_HANDLE_STATE macros. A handle is destroyed by calling
#MIPI_SYST_DELETE_HANDLE on it.

SyS-T and Multi-threading {#mipi_syst_api_threading}
==============================================================================
The SyS-T library provides a thread safe usage model that, for performance
reasons, does not use any data locking techniques. It is important to
understand this model to avoid incorrect usage that would result in hard to
analyze data races or corrupted message data output. The threading model is
defined by the following rules:

- SyS-T operates thread safe, as long as __SyS-T handles are not shared
  between simultaneously executing threads__.
- Sharing a SyS-T handle between threads requires locking around each
  instrumentation API call in the code using SyS-T.
- Platform adaption code that modifies the global library mipi_syst_header
  state may need locking inside its hook functions to protect state data
  members. See the mipi_stp_sim platform code in
  platform/mipi_stp_sim/src/mipi_syst_platform.c for an example where locking
  is required.

The recommended SyS-T usage is to create a SyS-T handle exclusively for each
thread and to store it in thread local storage. This usage provides low
software overhead, as no locking is required while utilizing trace arbiter
hardware or trace creating drivers that are capable of supporting multiple
data channels in parallel.

API Name Conventions    {#mipi_syst_api_conventions}
==============================================================================
All API functions are provided as CPP macros with the prefix `MIPI_SYST_`.

The data transmitting instrumentation calls exist in up to 4 variants for
generating additional location information. Location information describes
the instrumentation position of the API as either an instruction pointer
address, or a source "file:line" ID pair. This information is used during
trace decoding to map message data back to source locations. The name
conventions for the variants are:

- **MIPI_SYST_<API-NAME>(...)**<BR>
  API without location information generation

- **MIPI_SYST_<API-NAME>_LOC16(..,f, ...)**<BR>
  API variant that includes the instrumentation source position as a
  "file:line" id pair of 16-bit width each. The file id is user defined
  and passed as an additional parameter. This API requires the SyS-T platform
  feature define #MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD.

- **MIPI_SYST_<API-NAME>_LOC32(..,f, ...)**<BR>
  API variant that includes the instrumentation source position as a
  "file:line" id pair of 32-bit width each. The file id is user defined
  and passed as an additional parameter. This API requires the SyS-T platform
  feature define #MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD.

- **MIPI_SYST_<API-NAME>_LOCADDR(...)**<BR>
  API variant that includes the instrumentation instruction address as
  location. This API requires the SyS-T platform feature defines
  #MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD and #MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS.

See description of the #MIPI_SYST_DEBUG API call as an example showing all
variants.

SyS-T Message Attributes  {#mipi_syst_api_attributes}
==============================================================================
The SyS-T message produced by an API call contains mandatory and optional
data attributes. This chapter describes the attributes and how they are
defined when adding instrumentation calls.The message attributes are:

- A 32-bit header describing the message type and content
- An optional 128-bit wide GUID identifying the message originating software
  module.
- An optional location record
- An optional payload length
- API specific payload data of up to 64 Kbytes
- An optional CRC-32C checksum.

![SyS-T Message Anatomy](mipi_sys_t_message_anatomy.png)

SyS-T Message Header {#mipi_syst_api_header}
------------------------------------------------------------------------------
Each SyS-T message starts with a 32-bit header, which is defined by the data
structure mipi_syst_msg_tag. The header contains the following information:

- __Type and Subtype__ fields identifying the Message Type.<BR>
  These fields are set internally by the API call.

- Software __Module__ and __Unit__ IDs<BR>
  These fields identify the software module that originated the message. They
  are taken from the SyS-T handle used in the call. See the API calls
  #MIPI_SYST_SET_HANDLE_GUID_UNIT and #MIPI_SYST_SET_HANDLE_MODULE_UNIT for an
  example how the origin fields are used.

- __Message Severity__<BR>
  Each message contains a 3-bit severity level defined by the mipi_syst_severity
  enumeration.

- __Content__ Bits<BR>
  These bits  define what other optional attributes are present in the message.

Message Origin GUID {#mipi_syst_api_guid}
------------------------------------------------------------------------------
A message can contain a 128-bit wide GUID to identify the instrumented SW
module in a unique way. The GUID is emitted directly after the header if
the handle was initialized using the #MIPI_SYST_SET_HANDLE_GUID_UNIT API. The
GUID structure follows the RFC 4122 conventions and can be obtained
using various tools. One example is the `uuidgen` command on Linux.

Message Location Information {#mipi_syst_api_loc}
------------------------------------------------------------------------------
A message can contain location information describing the position of its
instrumentation call either as an instruction pointer address or as a
"file:line" ID pair. This location information is generated if one of the
API variants with the `_LOCADDR`, `_LOC16` or `_LOC32` suffix in the name is
used.

Message Payload {#mipi_syst_api_payload}
------------------------------------------------------------------------------
A message contains type-specific payload of up to 64 Kbytes, which is
generated internally by the API function from its parameters. These functions
also insert a 16-bit payload length field when when enabled by
the MIPI_SYST_ENABLE_HANDLE_LENGTH API.

Message Checksum {#mipi_syst_api_chkssum}
------------------------------------------------------------------------------
A message can end with a 32-bit CRC32C checksum calculated over all of the
message bytes, excluding the checksum value itself. The CRC generation is a
per SyS-T handle choice. It is enabled or disabled by the
#MIPI_SYST_ENABLE_HANDLE_CHECKSUM API. The default is disabled.

