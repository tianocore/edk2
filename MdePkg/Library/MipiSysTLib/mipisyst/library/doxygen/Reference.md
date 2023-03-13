API Quick-Reference    {#mipi_syst_api_page}
======================================
This page provides a quick reference for the SyS-T API macros. A
detailed description of these APIs and their their parameter
definitions is available by clicking on the individual API names.

## State lifetime handling macros ##
<table border=1 cols="3" align="left" width="100%">
  <tr>
    <td width="30%"><b>API Define</b></td>
    <td><b>Description</b></td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_INIT(f,p)
    </td>
    <td>SyS-T global platform initialization using an internal shared state header
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_INIT_STATE(s, f,p)
    </td>
    <td>SyS-T platform initialization using a provided state header structure
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_SHUTDOWN()
    </td>
    <td>SyS-T global platform shutdown
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_SHUTDOWN_STATE(s)
    </td>
    <td>SyS-T platform shutdown using a provided state header structure
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_INIT_HANDLE(h,p)
    </td>
    <td>Initialize a static (not heap allocated) SyS-T handle.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_INIT_HANDLE_STATE(s, h,p)
    </td>
    <td>Initialize a static (not heap allocated) SyS-T handle using a provided state header structure.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_ALLOC_HANDLE(p)
    </td>
    <td>Initialize heap allocated SyS-T handle.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_ALLOC_HANDLE_STATE(s, p)
    </td>
    <td>Initialize heap allocated SyS-T handle using a provided state header structure.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_DELETE_HANDLE(p)
    </td>
    <td>Delete a SyS-T handle.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_SET_HANDLE_ORIGIN(h, o)
    </td>
    <td>Set the client origin for the given SyS-T handle based on a mipi_syst_origin structure.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_SET_HANDLE_MODULE_UNIT(h,m,u)
    </td>
    <td>Specify module and unit ID of the given SyS-T handle.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_SET_HANDLE_GUID_UNIT(h, g, u)
    </td>
    <td>Specify GUID and unit ID of the given SyS-T handle.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_ENABLE_HANDLE_LENGTH(h, v)
    </td>
    <td>Enable or disable length field generation over the given SyS-T handle.
    </td>
  </tr>

  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_ENABLE_HANDLE_CHECKSUM(h,v)
    </td>
    <td>Enable or disable checksum generation over the given SyS-T handle.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(h, v)
    </td>
    <td>Enable or disable additional protocol times tamps over the
        given SyS-T handle.
    </td>
  </tr>
</table>
The availability of the macros #MIPI_SYST_ALLOC_HANDLE, #MIPI_SYST_SET_HANDLE_GUID_UNIT,
and #MIPI_SYST_ENABLE_HANDLE_CHECKSUM depend on
platform feature enable defines. Attempting to use an unsupported API will result
in a compile error.
<BR>

## String emitting macros ##
<table border=1 cols="3" align="left" width="100%">
  <tr>
    <td width="30%"><b>API Define</b></td>
    <td><b>Description</b></td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_DEBUG(svh,sev,str,len)<BR>
       MIPI_SYST_DEBUG_LOC16(svh,sev,file,str,len)<BR>
       MIPI_SYST_DEBUG_LOC32(svh,sev,file,str,len)<BR>
       MIPI_SYST_DEBUG_LOCADDR(svh,sev,str,len)
    </td>
    <td>Send A UTF-8 character string with given severity and length<BR>
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       \link MIPI_SYST_PRINTF MIPI_SYST_PRINTF(svh,sev,str, ...)\endlink<BR>
       \link MIPI_SYST_PRINTF_LOC16 MIPI_SYST_PRINTF_LOC16(svh,sev,file,str, ...)\endlink<BR>
       \link MIPI_SYST_PRINTF_LOC32 MIPI_SYST_PRINTF_LOC32(svh,sev,file,str, ...)\endlink<BR>
       \link MIPI_SYST_PRINTF_LOCADDR MIPI_SYST_PRINTF_LOCADDR(svh,sev,str, ...)\endlink
    </td>
    <td>Send a C-language style *printf()* API format string and its
        arguments. This API follows the calling convention of
        printf() as defined by the C99 C-Language standard.<BR>
        <B>Notes:</B> This API is enabled by the SyS-T platform
        define @ref MIPI_SYST_PCFG_ENABLE_PRINTF_API that must be set in
        addition to @ref MIPI_SYST_PCFG_ENABLE_STRING_API.<BR>
        Enabling this API implies the availability of the C-Language
        standard header files stdarg.h and stddef.h. These are needed
        due to the variable argument support of the *printf()* calling
        convention.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_FUNC_ENTER(svh,sev)<BR>
       MIPI_SYST_FUNC_ENTER_LOC16(svh,sev,file)<BR>
       MIPI_SYST_FUNC_ENTER_LOC32(svh,sev,file)<BR>
       MIPI_SYST_FUNC_ENTER_LOCADDR(svh,sev)
    </td>
    <td>Send function enter string message. The payload is the
        UTF-8 name of the surrounding function.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_FUNC_EXIT(svh,sev)<BR>
       MIPI_SYST_FUNC_EXIT_LOC16(svh,sev,file)<BR>
       MIPI_SYST_FUNC_EXIT_LOC32(svh,sev,file)<BR>
       MIPI_SYST_FUNC_EXIT_LOCADDR(svh,sev)
    </td>
    <td>Send function exit string message. The payload is the
        UTF-8 name of the surrounding function.
    </td>
  </tr>
   <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_ASSERT(svh,sev,cond)<BR>
       MIPI_SYST_ASSERT_LOC16(svh,sev,file,cond)<BR>
       MIPI_SYST_ASSERT_LOC32(svh,sev,file,cond)<BR>
       MIPI_SYST_ASSERT_LOCADDR(svh,sev,cond)
    </td>
    <td>Send [file]:[line] assertion notice string message if the passed
        condition is false
    </td>
  </tr>
</table>
The string emitting macros must be enabled by the SyS-T platform
define #MIPI_SYST_PCFG_ENABLE_STRING_API, otherwise they expand to nothing
inside the code.
<BR>
## Catalog message macros ##
<table border=1 cols="3" align="left" width="100%">
  <tr>
    <td width="30%"><b>API Define</b></td>
    <td><b>Description</b></td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
        \link MIPI_SYST_CATALOG64(svh,sev,id, ...)\endlink<BR>
    </td>
    <td> Send catalog message with variable number of parameters defined
         by the format string that matches the 64-bit catalog ID.</td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_CATALOG64_0(svh,sev,id)<BR>
       ...<BR>
       MIPI_SYST_CATALOG64_6(svh, sev, id,p1,p2,p3,p4,p5,p6)
    </td>
    <td> Send catalog message with 0-6 parameters.The MIPI_SYST_CATALOG64*
         defines build a family of macros that are used to send 64-bit wide
         user defined catalog message IDs with up to six 32-bit wide
         parameters into the trace stream.</td>
  </tr>
  <tr>
     <td style="white-space:nowrap">
        MIPI_SYST_CATPRINTF64_0(svh, sev, id, fmt)<BR>
        ...<BR>
        MIPI_SYST_CATPRINTF64_6(svh, sev, id, fmt, p1,p2,p3,p4,p5,p6)
     </td>
     <td> Printf style wrapper for the catalog functions that support an
          additional fmt string parameter. The functions directly map to
          their matching catalog call by dropping the fmt parameter.
          Their purpose is to use printf style instrumentation in source
          code together with catalog messages. The format strings are not
          part of the message but can be extracted from source code to
          pretty print catalog messages during trace decode.</td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       \link MIPI_SYST_CATALOG32(svh,sev,id, ...)\endlink<BR>
    </td>
    <td> Send catalog message with variable number of parameters defined
         by the format string that matches the 32-bit catalog ID.</td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_CATALOG32_0(svh, sev, id)<BR>
       ...<BR>
       MIPI_SYST_CATALOG32_6(svh, sev, id,p1,p2,p3,p4,p5,p6)
    </td>
    <td> Send catalog message with 0-6 parameters. The MIPI_SYST_CATALOG32*
         defines build a family of macros that are used to send 32-bit wide
         user defined catalog message IDs with up to six 32-bit wide
         parameters into the trace stream.</td>
  </tr>
  <tr>
     <td style="white-space:nowrap">
        MIPI_SYST_CATPRINTF32_0(svh, sev, id, fmt)<BR>
        ...<BR>
        MIPI_SYST_CATPRINTF32_6(svh, sev, id, fmt, p1,p2,p3,p4,p5,p6)
     </td>
     <td> Printf style wrapper for the catalog functions that support an
          additional fmt string parameter. The functions directly map to
          their matching catalog call by dropping the fmt parameter. Their purpose is
          to use printf style instrumentation in source code together with
          catalog messages. The format strings are not part of the message
          but can be extracted from source code to pretty print catalog
          messages during trace decode.</td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
        MIPI_SYST_HASH(fmt, offset)<BR>
    </td>
    <td> The MIPI_SYST_HASH macro computes a 32-bit hash value for the fmt
         string constant that is usable as a numeric CatalogID parameter.
         The offset parameter is used to change the result in case of hash
         value collisions with other strings. It is initially set to zero
         and is only changed to a different value if a collision occurs.
         It is used together with automated catalog ID generation
         decribed here: \link mipi_syst_catgen_page\endlink.</td>
  </tr>
</table>
The catalog message macros must be enabled by the SyS-T platform
defines #MIPI_SYST_PCFG_ENABLE_CATID32_API or #MIPI_SYST_PCFG_ENABLE_CATID64_API,
otherwise they expand to nothing inside the code.


## Raw data message macros ##
<table border=1 cols="3" align="left" width="100%">
  <tr>
    <td width="30%"><b>API Define</b></td>
    <td><b>Description</b></td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_SHORT32(h, v)
    </td>
    <td> Send short data message with 28-bit user defined payload.
         This API is intended for space and cpu restricted environments
         that cannot support more complex message types.</td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_SHORT64(h, v)
    </td>
    <td> Send short data message with 60-bit user defined payload.
         This API is intended for space and cpu restricted environments
         that cannot support more complex message types.</td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_WRITE(h, sev, id, p, len)
    </td>
    <td> Send raw data message with user defined payload.</td>
  </tr>
</table>
The #MIPI_SYST_WRITE API message macro must be enabled by the SyS-T platform
define #MIPI_SYST_PCFG_ENABLE_WRITE_API, otherwise it expands to nothing inside
the code. The #MIPI_SYST_SHORT32 and MIPI_SYST_SHORT64 APIs are always available.

## Client software build ID message macros ##
<table border=1 cols="3" align="left" width="100%">
  <tr>
    <td width="30%"><b>API Define</b></td>
    <td><b>Description</b></td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_BUILD(h, sev, ver, text, len)
    </td>
    <td> Send 64-bit client software build ID number and optional version text.</td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_BUILD_COMPACT32(h, v)
    </td>
    <td> Send a 32-bit compact client software build ID message with 22-bit wide build number.
    </td>
  </tr>
  <tr>
    <td style="white-space:nowrap">
       MIPI_SYST_BUILD_COMPACT64(h, v)
    </td>
    <td> Send a 64-bit compact client software build ID message value with a 54-bit wide build number.
    </td>
  </tr>
</table>