## @file
# This tool encodes and decodes GUIDed FFS sections or FMP capsule for a GUID type of
# EFI_CERT_TYPE_RSA2048_SHA256_GUID defined in the UEFI 2.4 Specification as
#   {0xa7717414, 0xc616, 0x4977, {0x94, 0x20, 0x84, 0x47, 0x12, 0xa7, 0x35, 0xbf}}
# This tool has been tested with OpenSSL 1.0.1e 11 Feb 2013
#
# Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
Rsa2048Sha256Sign
'''
from __future__ import print_function

import os
import sys
import argparse
import subprocess
import uuid
import struct
import collections
from Common.BuildVersion import gBUILD_VERSION

#
# Globals for help information
#
__prog__      = 'Rsa2048Sha256Sign'
__version__   = '%s Version %s' % (__prog__, '0.9 ' + gBUILD_VERSION)
__copyright__ = 'Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.'
__usage__     = '%s -e|-d [options] <input_file>' % (__prog__)

#
# GUID for SHA 256 Hash Algorithm from UEFI Specification
#
EFI_HASH_ALGORITHM_SHA256_GUID = uuid.UUID('{51aa59de-fdf2-4ea3-bc63-875fb7842ee9}')

#
# Structure definition to unpack EFI_CERT_BLOCK_RSA_2048_SHA256 from UEFI 2.4 Specification
#
#   typedef struct _EFI_CERT_BLOCK_RSA_2048_SHA256 {
#     EFI_GUID HashType;
#     UINT8 PublicKey[256];
#     UINT8 Signature[256];
#   } EFI_CERT_BLOCK_RSA_2048_SHA256;
#
EFI_CERT_BLOCK_RSA_2048_SHA256        = collections.namedtuple('EFI_CERT_BLOCK_RSA_2048_SHA256', ['HashType', 'PublicKey', 'Signature'])
EFI_CERT_BLOCK_RSA_2048_SHA256_STRUCT = struct.Struct('16s256s256s')

#
# Filename of test signing private key that is stored in same directory as this tool
#
TEST_SIGNING_PRIVATE_KEY_FILENAME = 'TestSigningPrivateKey.pem'

if __name__ == '__main__':
  #
  # Create command line argument parser object
  #
  parser = argparse.ArgumentParser(prog=__prog__, usage=__usage__, description=__copyright__, conflict_handler='resolve')
  group = parser.add_mutually_exclusive_group(required=True)
  group.add_argument("-e", action="store_true", dest='Encode', help='encode file')
  group.add_argument("-d", action="store_true", dest='Decode', help='decode file')
  group.add_argument("--version", action='version', version=__version__)
  parser.add_argument("-o", "--output", dest='OutputFile', type=str, metavar='filename', help="specify the output filename", required=True)
  parser.add_argument("--monotonic-count", dest='MonotonicCountStr', type=str, help="specify the MonotonicCount in FMP capsule.")
  parser.add_argument("--private-key", dest='PrivateKeyFile', type=argparse.FileType('rb'), help="specify the private key filename.  If not specified, a test signing key is used.")
  parser.add_argument("-v", "--verbose", dest='Verbose', action="store_true", help="increase output messages")
  parser.add_argument("-q", "--quiet", dest='Quiet', action="store_true", help="reduce output messages")
  parser.add_argument("--debug", dest='Debug', type=int, metavar='[0-9]', choices=range(0, 10), default=0, help="set debug level")
  parser.add_argument(metavar="input_file", dest='InputFile', type=argparse.FileType('rb'), help="specify the input filename")

  #
  # Parse command line arguments
  #
  args = parser.parse_args()

  #
  # Generate file path to Open SSL command
  #
  OpenSslCommand = 'openssl'
  try:
    OpenSslPath = os.environ['OPENSSL_PATH']
    OpenSslCommand = os.path.join(OpenSslPath, OpenSslCommand)
    if ' ' in OpenSslCommand:
      OpenSslCommand = '"' + OpenSslCommand + '"'
  except:
    pass

  #
  # Verify that Open SSL command is available
  #
  try:
    Process = subprocess.Popen('%s version' % (OpenSslCommand), stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
  except:
    print('ERROR: Open SSL command not available.  Please verify PATH or set OPENSSL_PATH')
    sys.exit(1)

  Version = Process.communicate()
  if Process.returncode != 0:
    print('ERROR: Open SSL command not available.  Please verify PATH or set OPENSSL_PATH')
    sys.exit(Process.returncode)
  print(Version[0].decode('utf-8'))

  #
  # Read input file into a buffer and save input filename
  #
  args.InputFileName   = args.InputFile.name
  args.InputFileBuffer = args.InputFile.read()
  args.InputFile.close()

  #
  # Save output filename and check if path exists
  #
  OutputDir = os.path.dirname(args.OutputFile)
  if not os.path.exists(OutputDir):
    print('ERROR: The output path does not exist: %s' % OutputDir)
    sys.exit(1)
  args.OutputFileName = args.OutputFile

  #
  # Save private key filename and close private key file
  #
  try:
    args.PrivateKeyFileName = args.PrivateKeyFile.name
    args.PrivateKeyFile.close()
  except:
    try:
      #
      # Get path to currently executing script or executable
      #
      if hasattr(sys, 'frozen'):
          RsaToolPath = sys.executable
      else:
          RsaToolPath = sys.argv[0]
      if RsaToolPath.startswith('"'):
          RsaToolPath = RsaToolPath[1:]
      if RsaToolPath.endswith('"'):
          RsaToolPath = RsaToolPath[:-1]
      args.PrivateKeyFileName = os.path.join(os.path.dirname(os.path.realpath(RsaToolPath)), TEST_SIGNING_PRIVATE_KEY_FILENAME)
      args.PrivateKeyFile = open(args.PrivateKeyFileName, 'rb')
      args.PrivateKeyFile.close()
    except:
      print('ERROR: test signing private key file %s missing' % (args.PrivateKeyFileName))
      sys.exit(1)

  #
  # Extract public key from private key into STDOUT
  #
  Process = subprocess.Popen('%s rsa -in "%s" -modulus -noout' % (OpenSslCommand, args.PrivateKeyFileName), stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
  PublicKeyHexString = Process.communicate()[0].split(b'=')[1].strip()
  PublicKeyHexString = PublicKeyHexString.decode('utf-8')
  PublicKey = ''
  while len(PublicKeyHexString) > 0:
    PublicKey = PublicKey + PublicKeyHexString[0:2]
    PublicKeyHexString=PublicKeyHexString[2:]
  if Process.returncode != 0:
    sys.exit(Process.returncode)

  if args.MonotonicCountStr:
    try:
      if args.MonotonicCountStr.upper().startswith('0X'):
        args.MonotonicCountValue = int(args.MonotonicCountStr, 16)
      else:
        args.MonotonicCountValue = int(args.MonotonicCountStr)
    except:
        pass

  if args.Encode:
    FullInputFileBuffer = args.InputFileBuffer
    if args.MonotonicCountStr:
      format = "%dsQ" % len(args.InputFileBuffer)
      FullInputFileBuffer = struct.pack(format, args.InputFileBuffer, args.MonotonicCountValue)
    #
    # Sign the input file using the specified private key and capture signature from STDOUT
    #
    Process = subprocess.Popen('%s dgst -sha256 -sign "%s"' % (OpenSslCommand, args.PrivateKeyFileName), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    Signature = Process.communicate(input=FullInputFileBuffer)[0]
    if Process.returncode != 0:
      sys.exit(Process.returncode)

    #
    # Write output file that contains hash GUID, Public Key, Signature, and Input data
    #
    args.OutputFile = open(args.OutputFileName, 'wb')
    args.OutputFile.write(EFI_HASH_ALGORITHM_SHA256_GUID.bytes_le)
    args.OutputFile.write(bytearray.fromhex(str(PublicKey)))
    args.OutputFile.write(Signature)
    args.OutputFile.write(args.InputFileBuffer)
    args.OutputFile.close()

  if args.Decode:
    #
    # Parse Hash Type, Public Key, and Signature from the section header
    #
    Header = EFI_CERT_BLOCK_RSA_2048_SHA256._make(EFI_CERT_BLOCK_RSA_2048_SHA256_STRUCT.unpack_from(args.InputFileBuffer))
    args.InputFileBuffer = args.InputFileBuffer[EFI_CERT_BLOCK_RSA_2048_SHA256_STRUCT.size:]

    #
    # Verify that the Hash Type matches the expected SHA256 type
    #
    if uuid.UUID(bytes_le = Header.HashType) != EFI_HASH_ALGORITHM_SHA256_GUID:
      print('ERROR: unsupport hash GUID')
      sys.exit(1)

    #
    # Verify the public key
    #
    if Header.PublicKey != bytearray.fromhex(PublicKey):
      print('ERROR: Public key in input file does not match public key from private key file')
      sys.exit(1)

    FullInputFileBuffer = args.InputFileBuffer
    if args.MonotonicCountStr:
      format = "%dsQ" % len(args.InputFileBuffer)
      FullInputFileBuffer = struct.pack(format, args.InputFileBuffer, args.MonotonicCountValue)

    #
    # Write Signature to output file
    #
    open(args.OutputFileName, 'wb').write(Header.Signature)

    #
    # Verify signature
    #
    Process = subprocess.Popen('%s dgst -sha256 -prverify "%s" -signature %s' % (OpenSslCommand, args.PrivateKeyFileName, args.OutputFileName), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    Process.communicate(input=FullInputFileBuffer)
    if Process.returncode != 0:
      print('ERROR: Verification failed')
      os.remove (args.OutputFileName)
      sys.exit(Process.returncode)

    #
    # Save output file contents from input file
    #
    open(args.OutputFileName, 'wb').write(args.InputFileBuffer)
