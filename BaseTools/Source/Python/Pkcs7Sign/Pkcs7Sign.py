## @file
# This tool adds EFI_FIRMWARE_IMAGE_AUTHENTICATION for a binary.
#
# This tool only support CertType - EFI_CERT_TYPE_PKCS7_GUID
#   {0x4aafd29d, 0x68df, 0x49ee, {0x8a, 0xa9, 0x34, 0x7d, 0x37, 0x56, 0x65, 0xa7}}
#
# This tool has been tested with OpenSSL.
#
# Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
Pkcs7Sign
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
__prog__      = 'Pkcs7Sign'
__version__   = '%s Version %s' % (__prog__, '0.9 ' + gBUILD_VERSION)
__copyright__ = 'Copyright (c) 2016, Intel Corporation. All rights reserved.'
__usage__     = '%s -e|-d [options] <input_file>' % (__prog__)

#
# GUID for PKCS7 from UEFI Specification
#
WIN_CERT_REVISION      = 0x0200
WIN_CERT_TYPE_EFI_GUID = 0x0EF1
EFI_CERT_TYPE_PKCS7_GUID = uuid.UUID('{4aafd29d-68df-49ee-8aa9-347d375665a7}')

#
# typedef struct _WIN_CERTIFICATE {
#   UINT32 dwLength;
#   UINT16 wRevision;
#   UINT16 wCertificateType;
# //UINT8 bCertificate[ANYSIZE_ARRAY];
# } WIN_CERTIFICATE;
#
# typedef struct _WIN_CERTIFICATE_UEFI_GUID {
#   WIN_CERTIFICATE Hdr;
#   EFI_GUID        CertType;
# //UINT8 CertData[ANYSIZE_ARRAY];
# } WIN_CERTIFICATE_UEFI_GUID;
#
# typedef struct {
#   UINT64                    MonotonicCount;
#   WIN_CERTIFICATE_UEFI_GUID AuthInfo;
# } EFI_FIRMWARE_IMAGE_AUTHENTICATION;
#

#
# Filename of test signing private cert that is stored in same directory as this tool
#
TEST_SIGNER_PRIVATE_CERT_FILENAME = 'TestCert.pem'
TEST_OTHER_PUBLIC_CERT_FILENAME = 'TestSub.pub.pem'
TEST_TRUSTED_PUBLIC_CERT_FILENAME = 'TestRoot.pub.pem'

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
  parser.add_argument("--signer-private-cert", dest='SignerPrivateCertFile', type=argparse.FileType('rb'), help="specify the signer private cert filename.  If not specified, a test signer private cert is used.")
  parser.add_argument("--other-public-cert", dest='OtherPublicCertFile', type=argparse.FileType('rb'), help="specify the other public cert filename.  If not specified, a test other public cert is used.")
  parser.add_argument("--trusted-public-cert", dest='TrustedPublicCertFile', type=argparse.FileType('rb'), help="specify the trusted public cert filename.  If not specified, a test trusted public cert is used.")
  parser.add_argument("--monotonic-count", dest='MonotonicCountStr', type=str, help="specify the MonotonicCount in FMP capsule.  If not specified, 0 is used.")
  parser.add_argument("--signature-size", dest='SignatureSizeStr', type=str, help="specify the signature size for decode process.")
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
  print(Version[0].decode())

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

  try:
    if args.MonotonicCountStr.upper().startswith('0X'):
      args.MonotonicCountValue = int(args.MonotonicCountStr, 16)
    else:
      args.MonotonicCountValue = int(args.MonotonicCountStr)
  except:
    args.MonotonicCountValue = int(0)

  if args.Encode:
    #
    # Save signer private cert filename and close private cert file
    #
    try:
      args.SignerPrivateCertFileName = args.SignerPrivateCertFile.name
      args.SignerPrivateCertFile.close()
    except:
      try:
        #
        # Get path to currently executing script or executable
        #
        if hasattr(sys, 'frozen'):
            Pkcs7ToolPath = sys.executable
        else:
            Pkcs7ToolPath = sys.argv[0]
        if Pkcs7ToolPath.startswith('"'):
            Pkcs7ToolPath = Pkcs7ToolPath[1:]
        if Pkcs7ToolPath.endswith('"'):
            Pkcs7ToolPath = RsaToolPath[:-1]
        args.SignerPrivateCertFileName = os.path.join(os.path.dirname(os.path.realpath(Pkcs7ToolPath)), TEST_SIGNER_PRIVATE_CERT_FILENAME)
        args.SignerPrivateCertFile = open(args.SignerPrivateCertFileName, 'rb')
        args.SignerPrivateCertFile.close()
      except:
        print('ERROR: test signer private cert file %s missing' % (args.SignerPrivateCertFileName))
        sys.exit(1)

    #
    # Save other public cert filename and close public cert file
    #
    try:
      args.OtherPublicCertFileName = args.OtherPublicCertFile.name
      args.OtherPublicCertFile.close()
    except:
      try:
        #
        # Get path to currently executing script or executable
        #
        if hasattr(sys, 'frozen'):
            Pkcs7ToolPath = sys.executable
        else:
            Pkcs7ToolPath = sys.argv[0]
        if Pkcs7ToolPath.startswith('"'):
            Pkcs7ToolPath = Pkcs7ToolPath[1:]
        if Pkcs7ToolPath.endswith('"'):
            Pkcs7ToolPath = RsaToolPath[:-1]
        args.OtherPublicCertFileName = os.path.join(os.path.dirname(os.path.realpath(Pkcs7ToolPath)), TEST_OTHER_PUBLIC_CERT_FILENAME)
        args.OtherPublicCertFile = open(args.OtherPublicCertFileName, 'rb')
        args.OtherPublicCertFile.close()
      except:
        print('ERROR: test other public cert file %s missing' % (args.OtherPublicCertFileName))
        sys.exit(1)

    format = "%dsQ" % len(args.InputFileBuffer)
    FullInputFileBuffer = struct.pack(format, args.InputFileBuffer, args.MonotonicCountValue)

    #
    # Sign the input file using the specified private key and capture signature from STDOUT
    #
    Process = subprocess.Popen('%s smime -sign -binary -signer "%s" -outform DER -md sha256 -certfile "%s"' % (OpenSslCommand, args.SignerPrivateCertFileName, args.OtherPublicCertFileName), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    Signature = Process.communicate(input=FullInputFileBuffer)[0]
    if Process.returncode != 0:
      sys.exit(Process.returncode)

    #
    # Write output file that contains Signature, and Input data
    #
    args.OutputFile = open(args.OutputFileName, 'wb')
    args.OutputFile.write(Signature)
    args.OutputFile.write(args.InputFileBuffer)
    args.OutputFile.close()

  if args.Decode:
    #
    # Save trusted public cert filename and close public cert file
    #
    try:
      args.TrustedPublicCertFileName = args.TrustedPublicCertFile.name
      args.TrustedPublicCertFile.close()
    except:
      try:
        #
        # Get path to currently executing script or executable
        #
        if hasattr(sys, 'frozen'):
            Pkcs7ToolPath = sys.executable
        else:
            Pkcs7ToolPath = sys.argv[0]
        if Pkcs7ToolPath.startswith('"'):
            Pkcs7ToolPath = Pkcs7ToolPath[1:]
        if Pkcs7ToolPath.endswith('"'):
            Pkcs7ToolPath = RsaToolPath[:-1]
        args.TrustedPublicCertFileName = os.path.join(os.path.dirname(os.path.realpath(Pkcs7ToolPath)), TEST_TRUSTED_PUBLIC_CERT_FILENAME)
        args.TrustedPublicCertFile = open(args.TrustedPublicCertFileName, 'rb')
        args.TrustedPublicCertFile.close()
      except:
        print('ERROR: test trusted public cert file %s missing' % (args.TrustedPublicCertFileName))
        sys.exit(1)

    if not args.SignatureSizeStr:
      print("ERROR: please use the option --signature-size to specify the size of the signature data!")
      sys.exit(1)
    else:
      if args.SignatureSizeStr.upper().startswith('0X'):
        SignatureSize = int(args.SignatureSizeStr, 16)
      else:
        SignatureSize = int(args.SignatureSizeStr)
    if SignatureSize < 0:
        print("ERROR: The value of option --signature-size can't be set to negative value!")
        sys.exit(1)
    elif SignatureSize > len(args.InputFileBuffer):
        print("ERROR: The value of option --signature-size is exceed the size of the input file !")
        sys.exit(1)

    args.SignatureBuffer = args.InputFileBuffer[0:SignatureSize]
    args.InputFileBuffer = args.InputFileBuffer[SignatureSize:]

    format = "%dsQ" % len(args.InputFileBuffer)
    FullInputFileBuffer = struct.pack(format, args.InputFileBuffer, args.MonotonicCountValue)

    #
    # Save output file contents from input file
    #
    open(args.OutputFileName, 'wb').write(FullInputFileBuffer)

    #
    # Verify signature
    #
    Process = subprocess.Popen('%s smime -verify -inform DER -content %s -CAfile %s' % (OpenSslCommand, args.OutputFileName, args.TrustedPublicCertFileName), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    Process.communicate(input=args.SignatureBuffer)[0]
    if Process.returncode != 0:
      print('ERROR: Verification failed')
      os.remove (args.OutputFileName)
      sys.exit(Process.returncode)

    open(args.OutputFileName, 'wb').write(args.InputFileBuffer)
