## @file
# Convert a binary file to a VOID* PCD value or DSC file VOID* PCD statement.
#
# Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

'''
BinToPcd
'''

import sys
import argparse
import re
import xdrlib

#
# Globals for help information
#
__prog__        = 'BinToPcd'
__version__     = '%s Version %s' % (__prog__, '0.91 ')
__copyright__   = 'Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.'
__description__ = 'Convert one or more binary files to a VOID* PCD value or DSC file VOID* PCD statement.\n'

if __name__ == '__main__':
  def ValidateUnsignedInteger (Argument):
    try:
      Value = int (Argument, 0)
    except:
      Message = '%s is not a valid integer value.' % (Argument)
      raise argparse.ArgumentTypeError(Message)
    if Value < 0:
      Message = '%s is a negative value.' % (Argument)
      raise argparse.ArgumentTypeError(Message)
    return Value

  def ValidatePcdName (Argument):
    if re.split('[a-zA-Z\_][a-zA-Z0-9\_]*\.[a-zA-Z\_][a-zA-Z0-9\_]*', Argument) <> ['','']:
      Message = '%s is not in the form <PcdTokenSpaceGuidCName>.<PcdCName>' % (Argument)
      raise argparse.ArgumentTypeError(Message)
    return Argument

  def ValidateGuidName (Argument):
    if re.split('[a-zA-Z\_][a-zA-Z0-9\_]*', Argument) <> ['','']:
      Message = '%s is not a valid GUID C name' % (Argument)
      raise argparse.ArgumentTypeError(Message)
    return Argument

  def ByteArray (Buffer, Xdr = False):
    if Xdr:
      #
      # If Xdr flag is set then encode data using the Variable-Length Opaque
      # Data format of RFC 4506 External Data Representation Standard (XDR).
      #
      XdrEncoder = xdrlib.Packer()
      for Item in Buffer:
        XdrEncoder.pack_bytes(Item)
      Buffer = XdrEncoder.get_buffer()
    else:
      #
      # If Xdr flag is not set, then concatenate all the data
      #
      Buffer = ''.join(Buffer)
    #
    # Return a PCD value of the form '{0x01, 0x02, ...}' along with the PCD length in bytes
    #
    return '{%s}' % (', '.join(['0x%02x' % (ord(Item)) for Item in Buffer])), len (Buffer)

  #
  # Create command line argument parser object
  #
  parser = argparse.ArgumentParser(prog = __prog__, version = __version__,
                                   description = __description__ + __copyright__,
                                   conflict_handler = 'resolve')
  parser.add_argument("-i", "--input", dest = 'InputFile', type = argparse.FileType('rb'), action='append', required = True,
                      help = "Input binary filename.  Multiple input files are combined into a single PCD.")
  parser.add_argument("-o", "--output", dest = 'OutputFile', type = argparse.FileType('wb'),
                      help = "Output filename for PCD value or PCD statement")
  parser.add_argument("-p", "--pcd", dest = 'PcdName', type = ValidatePcdName,
                      help = "Name of the PCD in the form <PcdTokenSpaceGuidCName>.<PcdCName>")
  parser.add_argument("-t", "--type", dest = 'PcdType', default = None, choices = ['VPD','HII'],
                      help = "PCD statement type (HII or VPD).  Default is standard.")
  parser.add_argument("-m", "--max-size", dest = 'MaxSize', type = ValidateUnsignedInteger,
                      help = "Maximum size of the PCD.  Ignored with --type HII.")
  parser.add_argument("-f", "--offset", dest = 'Offset', type = ValidateUnsignedInteger,
                      help = "VPD offset if --type is VPD.  UEFI Variable offset if --type is HII.")
  parser.add_argument("-n", "--variable-name", dest = 'VariableName',
                      help = "UEFI variable name.  Only used with --type HII.")
  parser.add_argument("-g", "--variable-guid", type = ValidateGuidName, dest = 'VariableGuid',
                      help = "UEFI variable GUID C name.  Only used with --type HII.")
  parser.add_argument("-x", "--xdr", dest = 'Xdr', action = "store_true",
                      help = "Encode PCD using the Variable-Length Opaque Data format of RFC 4506 External Data Representation Standard (XDR)")
  parser.add_argument("-v", "--verbose", dest = 'Verbose', action = "store_true",
                      help = "Increase output messages")
  parser.add_argument("-q", "--quiet", dest = 'Quiet', action = "store_true",
                      help = "Reduce output messages")
  parser.add_argument("--debug", dest = 'Debug', type = int, metavar = '[0-9]', choices = range(0,10), default = 0,
                      help = "Set debug level")

  #
  # Parse command line arguments
  #
  args = parser.parse_args()

  #
  # Read all binary input files
  #
  Buffer = []
  for File in args.InputFile:
    try:
      Buffer.append(File.read())
      File.close()
    except:
      print 'BinToPcd: error: can not read binary input file', File
      sys.exit()

  #
  # Convert PCD to an encoded string of hex values and determine the size of
  # the encoded PCD in bytes.
  #
  PcdValue, PcdSize = ByteArray (Buffer, args.Xdr)

  #
  # Convert binary buffer to a DSC file PCD statement
  #
  if args.PcdName is None:
    #
    # If PcdName is None, then only a PCD value is being requested.
    #
    Pcd = PcdValue
    if args.Verbose:
      print 'PcdToBin: Convert binary file to PCD Value'
  elif args.PcdType is None:
    #
    # If --type is neither VPD nor HII, then use PCD statement syntax that is
    # compatible with [PcdsFixedAtBuild], [PcdsPatchableInModule],
    # [PcdsDynamicDefault], and [PcdsDynamicExDefault].
    #
    if args.MaxSize is None:
      #
      # If --max-size is not provided, then do not generate the syntax that
      # includes the maximum size.
      #
      Pcd = '  %s|%s' % (args.PcdName, PcdValue)
    elif args.MaxSize < PcdSize:
      print 'BinToPcd: error: argument --max-size is smaller than input file.'
      sys.exit()
    else:
      Pcd = '  %s|%s|VOID*|%d' % (args.PcdName, PcdValue, args.MaxSize)

    if args.Verbose:
      print 'PcdToBin: Convert binary file to PCD statement compatible with PCD sections:'
      print '    [PcdsFixedAtBuild]'
      print '    [PcdsPatchableInModule]'
      print '    [PcdsDynamicDefault]'
      print '    [PcdsDynamicExDefault]'
  elif args.PcdType == 'VPD':
    if args.MaxSize is None:
      #
      # If --max-size is not provided, then set maximum size to the size of the
      # binary input file
      #
      args.MaxSize = PcdSize
    if args.MaxSize < PcdSize:
      print 'BinToPcd: error: argument --max-size is smaller than input file.'
      sys.exit()
    if args.Offset is None:
      #
      # if --offset is not provided, then set offset field to '*' so build
      # tools will compute offset of PCD in VPD region.
      #
      Pcd = '  %s|*|%d|%s' % (args.PcdName, args.MaxSize, PcdValue)
    else:
      #
      # Use the --offset value provided.
      #
      Pcd = '  %s|%d|%d|%s' % (args.PcdName, args.Offset, args.MaxSize, PcdValue)
    if args.Verbose:
      print 'PcdToBin: Convert binary file to PCD statement compatible with PCD sections'
      print '    [PcdsDynamicVpd]'
      print '    [PcdsDynamicExVpd]'
  elif args.PcdType == 'HII':
    if args.VariableGuid is None:
      print 'BinToPcd: error: argument --variable-guid is required for --type HII.'
      sys.exit()
    if args.VariableName is None:
      print 'BinToPcd: error: argument --variable-name is required for --type HII.'
      sys.exit()
    if args.Offset is None:
      #
      # Use UEFI Variable offset of 0 if --offset is not provided
      #
      args.Offset = 0
    Pcd = '  %s|L"%s"|%s|%d|%s' % (args.PcdName, args.VariableName, args.VariableGuid, args.Offset, PcdValue)
    if args.Verbose:
      print 'PcdToBin: Convert binary file to PCD statement compatible with PCD sections'
      print '    [PcdsDynamicHii]'
      print '    [PcdsDynamicExHii]'

  #
  # Write PCD value or PCD statement to the output file
  #
  try:
    args.OutputFile.write (Pcd)
    args.OutputFile.close ()
  except:
    #
    # If output file is not specified or it can not be written, then write the
    # PCD value or PCD statement to the console
    #
    print Pcd
