## @file
# This tool can be used to generate new RSA 2048 bit private/public key pairs
# in a PEM file format using OpenSSL command line utilities that are installed
# on the path specified by the system environment variable OPENSSL_PATH.
# This tool can also optionally write one or more SHA 256 hashes of 2048 bit
# public keys to a binary file, write one or more SHA 256 hashes of 2048 bit
# public keys to a file in a C structure format, and in verbose mode display
# one or more SHA 256 hashes of 2048 bit public keys in a C structure format
# on STDOUT.
# This tool has been tested with OpenSSL 1.0.1e 11 Feb 2013
#
# Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
Rsa2048Sha256GenerateKeys
'''
from __future__ import print_function

import os
import sys
import argparse
import subprocess
from Common.BuildVersion import gBUILD_VERSION

#
# Globals for help information
#
__prog__      = 'Rsa2048Sha256GenerateKeys'
__version__   = '%s Version %s' % (__prog__, '0.9 ' + gBUILD_VERSION)
__copyright__ = 'Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.'
__usage__     = '%s [options]' % (__prog__)


if __name__ == '__main__':
  #
  # Create command line argument parser object
  #
  parser = argparse.ArgumentParser(prog=__prog__, usage=__usage__, description=__copyright__, conflict_handler='resolve')
  group = parser.add_mutually_exclusive_group(required=True)
  group.add_argument("--version", action='version', version=__version__)
  group.add_argument("-o", "--output", dest='OutputFile', type=argparse.FileType('wb'), metavar='filename', nargs='*', help="specify the output private key filename in PEM format")
  group.add_argument("-i", "--input", dest='InputFile', type=argparse.FileType('rb'), metavar='filename', nargs='*', help="specify the input private key filename in PEM format")
  parser.add_argument("--public-key-hash", dest='PublicKeyHashFile', type=argparse.FileType('wb'), help="specify the public key hash filename that is SHA 256 hash of 2048 bit RSA public key in binary format")
  parser.add_argument("--public-key-hash-c", dest='PublicKeyHashCFile', type=argparse.FileType('wb'), help="specify the public key hash filename that is SHA 256 hash of 2048 bit RSA public key in C structure format")
  parser.add_argument("-v", "--verbose", dest='Verbose', action="store_true", help="increase output messages")
  parser.add_argument("-q", "--quiet", dest='Quiet', action="store_true", help="reduce output messages")
  parser.add_argument("--debug", dest='Debug', type=int, metavar='[0-9]', choices=range(0, 10), default=0, help="set debug level")

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
  print(Version[0].decode(encoding='utf-8', errors='ignore'))

  args.PemFileName = []

  #
  # Check for output file argument
  #
  if args.OutputFile is not None:
    for Item in args.OutputFile:
      #
      # Save PEM filename and close output file
      #
      args.PemFileName.append(Item.name)
      Item.close()

      #
      # Generate private key and save it to output file in a PEM file format
      #
      Process = subprocess.Popen('%s genrsa -out %s 2048' % (OpenSslCommand, Item.name), stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
      Process.communicate()
      if Process.returncode != 0:
        print('ERROR: RSA 2048 key generation failed')
        sys.exit(Process.returncode)

  #
  # Check for input file argument
  #
  if args.InputFile is not None:
    for Item in args.InputFile:
      #
      # Save PEM filename and close input file
      #
      args.PemFileName.append(Item.name)
      Item.close()

  PublicKeyHash = bytearray()
  for Item in args.PemFileName:
    #
    # Extract public key from private key into STDOUT
    #
    Process = subprocess.Popen('%s rsa -in %s -modulus -noout' % (OpenSslCommand, Item), stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    PublicKeyHexString = Process.communicate()[0].decode(encoding='utf-8', errors='ignore').split(b'=')[1].strip()
    if Process.returncode != 0:
      print('ERROR: Unable to extract public key from private key')
      sys.exit(Process.returncode)
    PublicKey = bytearray()
    for Index in range (0, len(PublicKeyHexString), 2):
      PublicKey = PublicKey + PublicKeyHexString[Index:Index + 2]

    #
    # Generate SHA 256 hash of RSA 2048 bit public key into STDOUT
    #
    Process = subprocess.Popen('%s dgst -sha256 -binary' % (OpenSslCommand), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    Process.stdin.write (PublicKey)
    PublicKeyHash = PublicKeyHash + Process.communicate()[0].decode(encoding='utf-8', errors='ignore')
    if Process.returncode != 0:
      print('ERROR: Unable to extract SHA 256 hash of public key')
      sys.exit(Process.returncode)

  #
  # Write SHA 256 hash of 2048 bit binary public key to public key hash file
  #
  try:
    args.PublicKeyHashFile.write (PublicKeyHash)
    args.PublicKeyHashFile.close ()
  except:
    pass

  #
  # Convert public key hash to a C structure string
  #
  PublicKeyHashC = '{'
  for Item in PublicKeyHash:
    PublicKeyHashC = PublicKeyHashC + '0x%02x, ' % (Item)
  PublicKeyHashC = PublicKeyHashC[:-2] + '}'

  #
  # Write SHA 256 of 2048 bit binary public key to public key hash C structure file
  #
  try:
    args.PublicKeyHashCFile.write (bytes(PublicKeyHashC))
    args.PublicKeyHashCFile.close ()
  except:
    pass

  #
  # If verbose is enabled display the public key in C structure format
  #
  if args.Verbose:
    print('PublicKeySha256 = ' + PublicKeyHashC)
