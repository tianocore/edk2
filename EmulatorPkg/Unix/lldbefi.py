#!/usr/bin/python

#
#  Copyright 2014 Apple Inc. All righes reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import lldb
import os
import uuid
import string
import commands
import optparse
import shlex

guid_dict = {}


def EFI_GUID_TypeSummary (valobj,internal_dict):
    """ Type summary for EFI GUID, print C Name if known
    """
    # typedef struct {
    #   UINT32  Data1;
    #   UINT16  Data2;
    #   UINT16  Data3;
    #   UINT8   Data4[8];
    # } EFI_GUID;
    SBError = lldb.SBError()

    data1_val = valobj.GetChildMemberWithName('Data1')
    data1 = data1_val.GetValueAsUnsigned(0)
    data2_val = valobj.GetChildMemberWithName('Data2')
    data2 = data2_val.GetValueAsUnsigned(0)
    data3_val = valobj.GetChildMemberWithName('Data3')
    data3 = data3_val.GetValueAsUnsigned(0)
    str = "%x-%x-%x-" % (data1, data2, data3)

    data4_val = valobj.GetChildMemberWithName('Data4')
    for i in range (data4_val.num_children):
        if i == 2:
            str +='-'
        str += "%02x" % data4_val.GetChildAtIndex(i).data.GetUnsignedInt8(SBError, 0)

    return guid_dict.get (str.upper(), '')



EFI_STATUS_Dict = {
    (0x8000000000000000 |  1): "Load Error",
    (0x8000000000000000 |  2): "Invalid Parameter",
    (0x8000000000000000 |  3): "Unsupported",
    (0x8000000000000000 |  4): "Bad Buffer Size",
    (0x8000000000000000 |  5): "Buffer Too Small",
    (0x8000000000000000 |  6): "Not Ready",
    (0x8000000000000000 |  7): "Device Error",
    (0x8000000000000000 |  8): "Write Protected",
    (0x8000000000000000 |  9): "Out of Resources",
    (0x8000000000000000 | 10): "Volume Corrupt",
    (0x8000000000000000 | 11): "Volume Full",
    (0x8000000000000000 | 12): "No Media",
    (0x8000000000000000 | 13): "Media changed",
    (0x8000000000000000 | 14): "Not Found",
    (0x8000000000000000 | 15): "Access Denied",
    (0x8000000000000000 | 16): "No Response",
    (0x8000000000000000 | 17): "No mapping",
    (0x8000000000000000 | 18): "Time out",
    (0x8000000000000000 | 19): "Not started",
    (0x8000000000000000 | 20): "Already started",
    (0x8000000000000000 | 21): "Aborted",
    (0x8000000000000000 | 22): "ICMP Error",
    (0x8000000000000000 | 23): "TFTP Error",
    (0x8000000000000000 | 24): "Protocol Error",

                          0 : "Success",
                          1 : "Warning Unknown Glyph",
                          2 : "Warning Delete Failure",
                          3 : "Warning Write Failure",
                          4 : "Warning Buffer Too Small",

    (0x80000000         |  1): "Load Error",
    (0x80000000         |  2): "Invalid Parameter",
    (0x80000000         |  3): "Unsupported",
    (0x80000000         |  4): "Bad Buffer Size",
    (0x80000000         |  5): "Buffer Too Small",
    (0x80000000         |  6): "Not Ready",
    (0x80000000         |  7): "Device Error",
    (0x80000000         |  8): "Write Protected",
    (0x80000000         |  9): "Out of Resources",
    (0x80000000         | 10): "Volume Corrupt",
    (0x80000000         | 11): "Volume Full",
    (0x80000000         | 12): "No Media",
    (0x80000000         | 13): "Media changed",
    (0x80000000         | 14): "Not Found",
    (0x80000000         | 15): "Access Denied",
    (0x80000000         | 16): "No Response",
    (0x80000000         | 17): "No mapping",
    (0x80000000         | 18): "Time out",
    (0x80000000         | 19): "Not started",
    (0x80000000         | 20): "Already started",
    (0x80000000         | 21): "Aborted",
    (0x80000000         | 22): "ICMP Error",
    (0x80000000         | 23): "TFTP Error",
    (0x80000000         | 24): "Protocol Error",
}

def EFI_STATUS_TypeSummary (valobj,internal_dict):
  #
  # Return summary string for EFI_STATUS from dictionary
  #
  Status = valobj.GetValueAsUnsigned(0)
  return EFI_STATUS_Dict.get (Status, '')


def EFI_TPL_TypeSummary (valobj,internal_dict):
  #
  # Return TPL values
  #

  if valobj.TypeIsPointerType():
    return ""

  Tpl = valobj.GetValueAsUnsigned(0)
  if   Tpl < 4:
    Str = "%d" % Tpl
  elif Tpl == 6:
    Str = "TPL_DRIVER (Obsolete Concept in edk2)"
  elif Tpl < 8:
    Str = "TPL_APPLICATION"
    if Tpl - 4 > 0:
      Str += " + " + "%d" % (Tpl - 4)
  elif Tpl < 16:
    Str = "TPL_CALLBACK"
    if Tpl - 8 > 0:
      Str += " + " + "%d" % (Tpl - 4)
  elif Tpl < 31:
    Str = "TPL_NOTIFY"
    if Tpl - 16 > 0:
      Str += " + " + "%d" % (Tpl - 4)
  elif Tpl == 31:
    Str = "TPL_HIGH_LEVEL"
  else:
    Str = "Invalid TPL"

  return Str


def CHAR16_TypeSummary (valobj,internal_dict):
  #
  # Display EFI CHAR16 'unsigned short' as string
  #
  SBError = lldb.SBError()
  Str = ''
  if valobj.TypeIsPointerType():
    if valobj.GetValueAsUnsigned () == 0:
      return "NULL"

    # CHAR16 *   max string size 1024
    for i in range (1024):
      Char = valobj.GetPointeeData(i,1).GetUnsignedInt16(SBError, 0)
      if SBError.fail or Char == 0:
        break
      Str += unichr (Char)
    Str = 'L"' + Str + '"'
    return Str.encode ('utf-8', 'replace')

  if valobj.num_children == 0:
    # CHAR16
    if chr (valobj.unsigned) in string.printable:
      Str = "L'" + unichr (valobj.unsigned) + "'"
      return Str.encode ('utf-8', 'replace')
  else:
    # CHAR16 []
    for i in range (valobj.num_children):
      Char = valobj.GetChildAtIndex(i).data.GetUnsignedInt16(SBError, 0)
      if Char == 0:
        break
      Str += unichr (Char)
    Str = 'L"' + Str + '"'
    return Str.encode ('utf-8', 'replace')

  return Str

def CHAR8_TypeSummary (valobj,internal_dict):
  #
  # Display EFI CHAR8 'signed char' as string
  # unichr() is used as a junk string can produce an error message like this:
  # UnicodeEncodeError: 'ascii' codec can't encode character u'\x90' in position 1: ordinal not in range(128)
  #
  SBError = lldb.SBError()
  Str = ''
  if valobj.TypeIsPointerType():
    if valobj.GetValueAsUnsigned () == 0:
      return "NULL"

    # CHAR8 *   max string size 1024
    for i in range (1024):
      Char = valobj.GetPointeeData(i,1).GetUnsignedInt8(SBError, 0)
      if SBError.fail or Char == 0:
        break
      Str += unichr (Char)
    Str = '"' + Str + '"'
    return Str.encode ('utf-8', 'replace')

  if valobj.num_children == 0:
    # CHAR8
    if chr (valobj.unsigned) in string.printable:
      Str = '"' + unichr (valobj.unsigned)  + '"'
      return Str.encode ('utf-8', 'replace')
  else:
    # CHAR8 []
    for i in range (valobj.num_children):
      Char = valobj.GetChildAtIndex(i).data.GetUnsignedInt8(SBError, 0)
      if Char == 0:
        break
      Str += unichr (Char)
    Str = '"' + Str + '"'
    return Str.encode ('utf-8', 'replace')

  return Str

device_path_dict = {
  (0x01, 0x01): "PCI_DEVICE_PATH",
  (0x01, 0x02): "PCCARD_DEVICE_PATH",
  (0x01, 0x03): "MEMMAP_DEVICE_PATH",
  (0x01, 0x04): "VENDOR_DEVICE_PATH",
  (0x01, 0x05): "CONTROLLER_DEVICE_PATH",
  (0x02, 0x01): "ACPI_HID_DEVICE_PATH",
  (0x02, 0x02): "ACPI_EXTENDED_HID_DEVICE_PATH",
  (0x02, 0x03): "ACPI_ADR_DEVICE_PATH",
  (0x03, 0x01): "ATAPI_DEVICE_PATH",
  (0x03, 0x12): "SATA_DEVICE_PATH",
  (0x03, 0x02): "SCSI_DEVICE_PATH",
  (0x03, 0x03): "FIBRECHANNEL_DEVICE_PATH",
  (0x03, 0x04): "F1394_DEVICE_PATH",
  (0x03, 0x05): "USB_DEVICE_PATH",
  (0x03, 0x0f): "USB_CLASS_DEVICE_PATH",
  (0x03, 0x10): "FW_SBP2_UNIT_LUN_DEVICE_PATH",
  (0x03, 0x11): "DEVICE_LOGICAL_UNIT_DEVICE_PATH",
  (0x03, 0x06): "I2O_DEVICE_PATH",
  (0x03, 0x0b): "MAC_ADDR_DEVICE_PATH",
  (0x03, 0x0c): "IPv4_DEVICE_PATH",
  (0x03, 0x09): "INFINIBAND_DEVICE_PATH",
  (0x03, 0x0e): "UART_DEVICE_PATH",
  (0x03, 0x0a): "VENDOR_DEVICE_PATH",
  (0x03, 0x13): "ISCSI_DEVICE_PATH",
  (0x04, 0x01): "HARDDRIVE_DEVICE_PATH",
  (0x04, 0x02): "CDROM_DEVICE_PATH",
  (0x04, 0x03): "VENDOR_DEVICE_PATH",
  (0x04, 0x04): "FILEPATH_DEVICE_PATH",
  (0x04, 0x05): "MEDIA_PROTOCOL_DEVICE_PATH",
  (0x05, 0x01): "BBS_BBS_DEVICE_PATH",
  (0x7F, 0xFF): "EFI_DEVICE_PATH_PROTOCOL",
  (0xFF, 0xFF): "EFI_DEVICE_PATH_PROTOCOL",
}

def EFI_DEVICE_PATH_PROTOCOL_TypeSummary (valobj,internal_dict):
  #
  #
  #
  if valobj.TypeIsPointerType():
    # EFI_DEVICE_PATH_PROTOCOL *
    return ""

  Str = ""
  if valobj.num_children == 3:
    # EFI_DEVICE_PATH_PROTOCOL
    Type    = valobj.GetChildMemberWithName('Type').unsigned
    SubType = valobj.GetChildMemberWithName('SubType').unsigned
    if (Type, SubType) in device_path_dict:
      TypeStr = device_path_dict[Type, SubType]
    else:
      TypeStr = ""

    LenLow  = valobj.GetChildMemberWithName('Length').GetChildAtIndex(0).unsigned
    LenHigh = valobj.GetChildMemberWithName('Length').GetChildAtIndex(1).unsigned
    Len = LenLow + (LenHigh >> 8)

    Address = long ("%d" % valobj.addr)
    if (Address == lldb.LLDB_INVALID_ADDRESS):
      # Need to reserach this, it seems to be the nested struct case
      ExprStr = ""
    elif (Type & 0x7f == 0x7f):
      ExprStr = "End Device Path" if SubType == 0xff else "End This Instance"
    else:
      ExprStr = "expr *(%s *)0x%08x" % (TypeStr, Address)

    Str =  " {\n"
    Str += "   (UINT8) Type    = 0x%02x // %s\n" % (Type, "END" if (Type & 0x7f == 0x7f) else "")
    Str += "   (UINT8) SubType = 0x%02x // %s\n" % (SubType, ExprStr)
    Str += "   (UINT8 [2]) Length = { // 0x%04x (%d) bytes\n" % (Len, Len)
    Str += "     (UINT8) [0] = 0x%02x\n" % LenLow
    Str += "     (UINT8) [1] = 0x%02x\n" % LenHigh
    Str +=  "   }\n"
    if (Type & 0x7f == 0x7f) and (SubType == 0xff):
      pass
    elif ExprStr != "":
      NextNode = Address + Len
      Str += "// Next node 'expr *(EFI_DEVICE_PATH_PROTOCOL *)0x%08x'\n" % NextNode

  return Str



def TypePrintFormating(debugger):
    #
    # Set the default print formating for EFI types in lldb.
    # seems lldb defaults to decimal.
    #
    category = debugger.GetDefaultCategory()
    FormatBool = lldb.SBTypeFormat(lldb.eFormatBoolean)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("BOOLEAN"), FormatBool)

    FormatHex  = lldb.SBTypeFormat(lldb.eFormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINT64"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INT64"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINT32"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INT32"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINT16"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INT16"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINT8"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INT8"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINTN"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INTN"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("CHAR8"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("CHAR16"), FormatHex)

    category.AddTypeFormat(lldb.SBTypeNameSpecifier("EFI_PHYSICAL_ADDRESS"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("PHYSICAL_ADDRESS"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("EFI_STATUS"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("EFI_TPL"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("EFI_LBA"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("EFI_BOOT_MODE"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("EFI_FV_FILETYPE"), FormatHex)

    #
    # Smart type printing for EFI
    #
    debugger.HandleCommand("type summary add EFI_GUID --python-function lldbefi.EFI_GUID_TypeSummary")
    debugger.HandleCommand("type summary add EFI_STATUS --python-function lldbefi.EFI_STATUS_TypeSummary")
    debugger.HandleCommand("type summary add EFI_TPL --python-function lldbefi.EFI_TPL_TypeSummary")
    debugger.HandleCommand("type summary add EFI_DEVICE_PATH_PROTOCOL --python-function lldbefi.EFI_DEVICE_PATH_PROTOCOL_TypeSummary")

    debugger.HandleCommand("type summary add CHAR16 --python-function lldbefi.CHAR16_TypeSummary")
    debugger.HandleCommand('type summary add --regex "CHAR16 \[[0-9]+\]" --python-function lldbefi.CHAR16_TypeSummary')
    debugger.HandleCommand("type summary add CHAR8 --python-function lldbefi.CHAR8_TypeSummary")
    debugger.HandleCommand('type summary add --regex "CHAR8 \[[0-9]+\]" --python-function lldbefi.CHAR8_TypeSummary')


gEmulatorBreakWorkaroundNeeded = True

def LoadEmulatorEfiSymbols(frame, bp_loc , internal_dict):
    #
    # This is an lldb breakpoint script, and assumes the breakpoint is on a
    # function with the same prototype as SecGdbScriptBreak(). The
    # argument names are important as lldb looks them up.
    #
    # VOID
    # SecGdbScriptBreak (
    #   char                *FileName,
    #   int                 FileNameLength,
    #   long unsigned int   LoadAddress,
    #   int                 AddSymbolFlag
    #   )
    # {
    #   return;
    # }
    #
    # When the emulator loads a PE/COFF image, it calls the stub function with
    # the filename of the symbol file, the length of the FileName, the
    # load address and a flag to indicate if this is a load or unload operation
    #
    global gEmulatorBreakWorkaroundNeeded

    if gEmulatorBreakWorkaroundNeeded:
        # turn off lldb debug prints on SIGALRM (EFI timer tick)
        frame.thread.process.target.debugger.HandleCommand("process handle SIGALRM -n false")
        gEmulatorBreakWorkaroundNeeded = False

    # Convert C string to Python string
    Error = lldb.SBError()
    FileNamePtr = frame.FindVariable ("FileName").GetValueAsUnsigned()
    FileNameLen = frame.FindVariable ("FileNameLength").GetValueAsUnsigned()
    FileName = frame.thread.process.ReadCStringFromMemory (FileNamePtr, FileNameLen, Error)
    if not Error.Success():
        print "!ReadCStringFromMemory() did not find a %d byte C string at %x" % (FileNameLen, FileNamePtr)
        # make breakpoint command contiue
        frame.GetThread().GetProcess().Continue()

    debugger = frame.thread.process.target.debugger
    if frame.FindVariable ("AddSymbolFlag").GetValueAsUnsigned() == 1:
        LoadAddress = frame.FindVariable ("LoadAddress").GetValueAsUnsigned()

        debugger.HandleCommand ("target modules add  %s" % FileName)
        print "target modules load --slid 0x%x %s" % (LoadAddress, FileName)
        debugger.HandleCommand ("target modules load --slide 0x%x --file %s" % (LoadAddress, FileName))
    else:
        target = debugger.GetSelectedTarget()
        for SBModule in target.module_iter():
            ModuleName  = SBModule.GetFileSpec().GetDirectory() + '/'
            ModuleName += SBModule.GetFileSpec().GetFilename()
            if FileName == ModuleName or FileName == SBModule.GetFileSpec().GetFilename():
                target.ClearModuleLoadAddress (SBModule)
                if not target.RemoveModule (SBModule):
                    print "!lldb.target.RemoveModule (%s) FAILED" % SBModule

    # make breakpoint command contiue
    frame.thread.process.Continue()

def GuidToCStructStr (guid, Name=False):
  #
  # Convert a 16-byte bytesarry (or bytearray compat object) to C guid string
  # { 0xB402621F, 0xA940, 0x1E4A, { 0x86, 0x6B, 0x4D, 0xC9, 0x16, 0x2B, 0x34, 0x7C } }
  #
  # Name=True means lookup name in GuidNameDict and us it if you find it
  #

  if not isinstance (guid, bytearray):
    # convert guid object to UUID, and UUID to bytearray
    Uuid = uuid.UUID(guid)
    guid = bytearray (Uuid.bytes_le)

  return "{ 0x%02.2X%02.2X%02.2X%02.2X, 0x%02.2X%02.2X, 0x%02.2X%02.2X, { 0x%02.2X, 0x%02.2X, 0x%02.2X, 0x%02.2X, 0x%02.2X, 0x%02.2X, 0x%02.2X, 0x%02.2X } }" % \
         (guid[3], guid[2], guid[1], guid[0], guid[5], guid[4], guid[7], guid[6], guid[8], guid[9], guid[10], guid[11], guid[12], guid[13], guid[14], guid[15])

def ParseGuidString(GuidStr):
  #
  # Error check and convert C Guid init to string
  # ParseGuidString("49152E77-1ADA-4764-B7A2-7AFEFED95E8B")
  # ParseGuidString("{ 0xBA24B391, 0x73FD, 0xC54C, { 0x9E, 0xAF, 0x0C, 0xA7, 0x8A, 0x35, 0x46, 0xD1 } }")
  #

  if "{" in GuidStr                                                                                         :
    # convert C form "{ 0xBA24B391, 0x73FD, 0xC54C, { 0x9E, 0xAF, 0x0C, 0xA7, 0x8A, 0x35, 0x46, 0xD1 } }"
    # to string form BA24B391-73FD-C54C-9EAF-0CA78A3546D1
    # make a list of Hex numbers like: ['0xBA24B391', '0x73FD', '0xC54C', '0x9E', '0xAF', '0x0C', '0xA7', '0x8A', '0x35', '0x46', '0xD1']
    Hex = ''.join(x for x in GuidStr if x not in '{,}').split()
    Str = "%08X-%04X-%04X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X" % \
          (int(Hex[0], 0), int(Hex[1], 0), int(Hex[2], 0), int(Hex[3], 0), int(Hex[4], 0), \
           int(Hex[5], 0), int(Hex[6], 0), int(Hex[7], 0), int(Hex[8], 0), int(Hex[9], 0), int(Hex[10], 0))
  elif GuidStr.count('-') == 4:
    # validate "49152E77-1ADA-4764-B7A2-7AFEFED95E8B" form
    Check = "%s" % str(uuid.UUID(GuidStr)).upper()
    if GuidStr.upper() == Check:
      Str = GuidStr.upper()
    else:
      Ste = ""
  else:
    Str = ""

  return Str


def create_guid_options():
    usage = "usage: %prog [data]"
    description='''lookup EFI_GUID by CName, C struct, or GUID string and print out all three.
    '''
    parser = optparse.OptionParser(description=description, prog='guid',usage=usage)
    return parser

def efi_guid_command(debugger, command, result, dict):
    # Use the Shell Lexer to properly parse up command options just like a
    # shell would
    command_args = shlex.split(command)
    parser = create_guid_options()
    try:
        (options, args) = parser.parse_args(command_args)
        if len(args) >= 1:
          if args[0] == "{":
              # caller forgot to quote the string"
              # mark arg[0] a string containing all args[n]
              args[0] = ' '.join(args)
          GuidStr = ParseGuidString (args[0])
          if GuidStr == "":
              # return Key of GuidNameDict for value args[0]
              GuidStr = [Key for Key, Value in guid_dict.iteritems() if Value == args[0]][0]
          GuidStr = GuidStr.upper()
    except:
        # if you don't handle exceptions, passing an incorrect argument to the OptionParser will cause LLDB to exit
        # (courtesy of OptParse dealing with argument errors by throwing SystemExit)
        result.SetError ("option parsing failed")
        return


    if len(args) >= 1:
        if GuidStr in guid_dict:
            print "%s = %s" % (guid_dict[GuidStr], GuidStr)
            print "%s = %s" % (guid_dict[GuidStr], GuidToCStructStr (GuidStr))
        else:
            print GuidStr
    else:
        # dump entire dictionary
        width = max(len(v) for k,v in guid_dict.iteritems())
        for value in sorted(guid_dict, key=guid_dict.get):
            print '%-*s %s %s' % (width, guid_dict[value], value, GuidToCStructStr(value))

    return


#
########## Code that runs when this script is imported into LLDB ###########
#
def __lldb_init_module (debugger, internal_dict):
    # This initializer is being run from LLDB in the embedded command interpreter
    # Make the options so we can generate the help text for the new LLDB
    # command line command prior to registering it with LLDB below

    global guid_dict

    # Source Guid.xref file if we can find it
    inputfile = os.getcwd()
    inputfile += os.sep + os.pardir + os.sep + 'FV' + os.sep + 'Guid.xref'
    with open(inputfile) as f:
        for line in f:
            data = line.split(' ')
            if len(data) >= 2:
                guid_dict[data[0].upper()] = data[1].strip('\n')

    # init EFI specific type formaters
    TypePrintFormating (debugger)


    # add guid command
    parser = create_guid_options()
    efi_guid_command.__doc__ = parser.format_help()
    debugger.HandleCommand('command script add -f lldbefi.efi_guid_command guid')


    Target = debugger.GetTargetAtIndex(0)
    if Target:
        Breakpoint = Target.BreakpointCreateByName('SecGdbScriptBreak')
        if Breakpoint.GetNumLocations() == 1:
            # Set the emulator breakpoints, if we are in the emulator
            debugger.HandleCommand("breakpoint command add -s python -F lldbefi.LoadEmulatorEfiSymbols {id}".format(id=Breakpoint.GetID()))
            print 'Type r to run emulator. SecLldbScriptBreak armed. EFI modules should now get source level debugging in the emulator.'
