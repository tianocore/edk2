## @file
#
# Generates Provisioning Data for a Confidential VM using certificates as input.
#
# Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import argparse
import datetime
from struct import *
import Common.EdkLogger as EdkLogger
from Common.BuildToolError import *
import sys
import os

__description__ = """
Generate a Provisioning Data file for a Confidential VM using certificates as input.
"""

## Parse the command line arguments.
#
# @retval A argparse.NameSpace instance, containing parsed values.
#
def ParseArgs():
    #Initialise the parser
    Parser = argparse.ArgumentParser(description = __description__)

    #Define the arguments
    Parser.add_argument("--pk",
                        required=True,
                        type=str,
                        help="the full path and filename of the PK certificate file")

    Parser.add_argument("--kek",
                        required=True,
                        type=str,
                        help="the full path and filename of the KEK certificate file")
    Parser.add_argument("--db",
                        required=True,
                        type=str,
                        help="the full path and filename of the DB certificate file")
    Parser.add_argument("--dbx",
                        required=True,
                        type=str,
                        help="the full path and filename of the DBX certificate file")
    Parser.add_argument("--dbt",
                        required=True,
                        type=str,
                        help="the full path and filename of the DBT certificate file")
    Parser.add_argument("--dbr",
                        required=True,
                        type=str,
                        help="the full path and filename of the DBR certificate file")
    Parser.add_argument("--out",
                       required=True,
                        type=str,
                        help = "the full path and filename of the output Provisioning Data file")
    Parser.add_argument("--fv",
                        action="store_true",
                        help="Enable emitting Efi Firmware Volume Header")
    Args = Parser.parse_args()

    #Check if certificate files exist
    if not os.path.exists(Args.pk):
        EdkLogger.error(__file__, FILE_OPEN_FAILURE, ExtraData = Args.pk)
        return None
    if not os.path.exists(Args.kek):
        EdkLogger.error(__file__, FILE_OPEN_FAILURE, ExtraData = Args.kek)
        return None
    if not os.path.exists(Args.db):
        EdkLogger.error(__file__, FILE_OPEN_FAILURE, ExtraData = Args.db)
        return None
    if not os.path.exists(Args.dbx):
        EdkLogger.error(__file__, FILE_OPEN_FAILURE, ExtraData = Args.dbx)
        return None
    if not os.path.exists(Args.dbt):
        EdkLogger.error(__file__, FILE_OPEN_FAILURE, ExtraData = Args.dbt)
        return None
    if not os.path.exists(Args.dbr):
        EdkLogger.error(__file__, FILE_OPEN_FAILURE, ExtraData = Args.dbr)
        return None
    # Certificate files checked as existing

    return Args

## Generate the EFI_FIRMWARE_VOLUME_HEADER to be emitted
#
# @param CheckSum   The 16 bit checksum for the Firmware Volume Header
# @param BlockMap   The BlockMap to be written, in the form of a tuple
#
# @retval A bytes object containing the formatted data for the EFI_FIRMWARE_VOLUME_HEADER
#
def GenEfiFvHeader(CheckSum,BlockMap):
    # The structure of an EFI_FIRMWARE_VOLUME_HEADER is defined in edk2/MdePkg/Include/Pi/PiFirmwareVolume.h
    # And it is as follows:
    # 16 bytes of zeroes
    # 128 bits, or 16 bytes for the FileSystemGuid
    # 8 bytes giving the length of the firmware volume including this header
    # 4 bytes with a specific signature value
    # 4 bytes of attribute data
    # 2 bytes for the length of the firmware volume header
    # 2 bytes for the 16 bit checksum of the firmware volume header
    # 2 bytes for the offset for EFI_FIRMWARE_VOLUME_EXT_HEADER (or zero if none)
    # 1 byte  set to zero as a reserved byte
    # 1 byte set to two as revision
    # 16 bytes for the blockmap

    # Create Correctly formatted Bytes objects for each field
    FvZeroVectorBytes = pack("<QQ",*(0,0))
    # the following signature is EFI_FIRMWARE_VOLUME_HEADER
    FvFileSysGuidBytes= pack("<IHHBBBBBBBB",
                            0xFFF12B8D,
                            0x7696,
                            0x4C8B,
                            0xA9,
                            0x85,
                            0x27,
                            0x47,
                            0x07,
                            0x5B,
                            0x4F,
                            0x50
                            )
    FvFvLengthBytes = pack("<Q",0xC0000)
    FvSignatureBytes = pack("<BBBB",0x5f,0x46,0x56,0x48)
    FvAttributesBytes = pack("<BBBB",0xff,0xf2,0x04,0x00)
    FvHeaderBytes = pack("<H",0x48)
    FvChecksumBytes = pack("<H",CheckSum)
    FvExtHeadOffBytes= pack("<H",0x0)
    FvReservedBytes = pack("<B",0x0)
    FvRevisionBytes = pack("<B",0x02)
    FvBlockmapBytes = pack("<IIQ",*BlockMap)

    OutBytes = FvZeroVectorBytes + FvFileSysGuidBytes +FvFvLengthBytes + \
                FvSignatureBytes + FvAttributesBytes + FvHeaderBytes + \
                FvChecksumBytes + FvExtHeadOffBytes + FvReservedBytes + \
                FvRevisionBytes +FvBlockmapBytes
    return OutBytes

## Generate the VARIABLE_STORE_HEADER to be emitted
#
# @param VsSize The size of the Variable Store
#
# @retval A bytes object containing the formatted data for the VARIABLE_STORE_HEADER
#
def GenVarStoreHeader(VsSize):
    # The Structure of a VARIABLE_STORE_HEADER is defined in edk2/MdeModulePkg/Include/Guid/VariableFormat.h
    # The sizes of the fields are as follows:
    # 16 Bytes for the signature
    # 4 Bytes for the size of the store
    # 1 Byte for the region format state
    # 1 Byte for te region healthy state
    # 2 Bytes reserved
    # 4 bytes reserved

    # Create correctly formatted Bytes objects for each field

    # the following signature is gEfiAuthenticatedVariableGuid
    VsSignatureBytes = pack("<IHHBBBBBBBB",
                            0xaaf32c78,
                            0x947b,
                            0x439a,0xa1,
                            0x80,
                            0x2e,
                            0x14,
                            0x4e,
                            0xc3,
                            0x77,
                            0x92
                            )
    VsSizeBytes = pack("<I",VsSize)
    VsRegionFormatStateBytes = pack("<B",0x5a)
    VsRegionHealthyStateBytes = pack("<B",0xFE)
    VsReserved2Bytes = pack("<H",0)
    VsReserved4Bytes = pack("<I",0)

    VsOutBytes = VsSignatureBytes + VsSizeBytes + \
                    VsRegionFormatStateBytes + VsRegionHealthyStateBytes + \
                    VsReserved2Bytes + VsReserved4Bytes

    return VsOutBytes

## Generates a tuple containing the current time as required for the EFI_TIME field in the
# AUTHENTICATED_VARIABLE_HEADER
#
# @retval A tuple with the correct values for the EFI_TIME struct in order, to be packed elsewhere
#
def GetTimeStamp():
    CurrentTime = datetime.datetime.now()
    TimeTuple = (CurrentTime.year,
                CurrentTime.month,
                CurrentTime.day,
                CurrentTime.hour,
                CurrentTime.minute,
                CurrentTime.second,
                0,0,0,0,0)
    return TimeTuple

## Generates the AUTHENTICATED_VARIABLE_HEADER to be emitted
#
# @param State          The state of the variable
# @param Attribute      Attributes of variable as defined in UEFI spec
# @param NameSize       Size of the null-terminated Unicode string name
# @param DataSize       Size of the Data without this header
# @param VendorGuid     A tuple containing the formatted unique identifer for the vendor that produces/consumes this variable
# @param CurrentTime    A tuple with the correct fields for the EFI_TIME variable
#
# @retval A bytes object containing the formatted data for the AUTHENTICATED_VARIABLE_HEADER
#
def GenAuthVarbHeader(State, Attribute,NameSize, DataSize, VendorGuid, CurrentTime):
    # The Structure of an AUTHENTICATED_VARIABLE_HEADER is as follows
    # 2 Bytes for StartID
    # 1 bytes for State
    # 1 byte reserved
    # 4 bytes of attributes
    # 8 Bytes for monotonic count
    # 16 Bytes for the EFI_TIME variable in a specific format
    # 4 Bytes for the publickey index
    # 4 Bytes for the size of the null-terminated unicode string name
    # 4 Bytes for the size of the variable data without this header
    # 16 Bytes for the Vendor Guid

    AvhStartIdBytes = pack("<H", 0x55AA)
    AvhStateBytes = pack("<B",State)
    AvhReservedBytes = pack("<B",0)
    AvhAttrBytes = pack("<I",Attribute)
    AvhMonCountBytes = pack("Q",0)

    AvhTimeBytes = pack("<HBBBBBBIhBB",
                        *CurrentTime)
    AvhKeyIndexBytes = pack("<I",0)
    AvhNameSizeBytes = pack("<I",NameSize)
    AvhDataSizeBytes = pack("<I",DataSize)
    AvhVendGuidBytes = pack("<IHHBBBBBBBB",*VendorGuid)

    AvhOutBytes = AvhStartIdBytes + AvhStateBytes + AvhReservedBytes + \
                    AvhAttrBytes + AvhMonCountBytes + AvhTimeBytes + \
                    AvhKeyIndexBytes + AvhNameSizeBytes + AvhDataSizeBytes + \
                    AvhVendGuidBytes

    return AvhOutBytes

## Generates the bytes for the variable name, including null terminator
#
# @param A string containing the name of the variable
#
# @retval A bytes object containing the formatted data for the name
#
def GenName(VarName):
    OutName = b''
    for c in VarName:
        CharBytes = c.encode("utf_16_le")
        OutName += CharBytes

    TerminatorBytes = pack("<BB",0,0)

    return OutName + TerminatorBytes

## Generates an EFI_SIGNATURE_LIST for an X.509 cert
#
# @param SignatureListSize  The size of the signature list
# @param CertificateSize    The size of the certificate
#
# @retvalue A bytes object containing the formatted data for the EFI_SIGNATURE_LIST
#
def GenEfiSigList(SignatureListSize, CertificateSize):
    # The Structure of an EFI_SIGNATURE_LIST is as follows
    # 16 Bytes for the GUID describing signature type
    # 4 Bytes for the total size of the signature list including this header
    # 4 bytes for the size of the signature header preceeding array of signatures
    # 4 bytes for the size of each signature

    #The following signature corresponds to an X.509 certificate
    EslSigTypeBytes = pack("<IHHBBBBBBBB",
                            0xA5C059A1,
                            0x94E4,
                            0x4AA7,
                            0x87, 0xB5,
                            0xAB, 0x15,
                            0x5C, 0x2B,
                            0xf0, 0x72)
    EslSigListSizeBytes = pack("<I",SignatureListSize)
    # Defined to be 0 for an x.509 cert
    EslSigHeaderSizeBytes = pack("<I",0)
    # For an X.509 Cert, the Sig Size is defined as 16 + Size of the certificate
    SigSize = 16 + CertificateSize
    EslSigSizeBytes = pack("<I",SigSize)

    EslOutBytes = EslSigTypeBytes + EslSigListSizeBytes + \
                    EslSigHeaderSizeBytes + EslSigSizeBytes
    return EslOutBytes

## Generates an EFI_SIGNATURE_DATA
# @param SignatureOwner A formatted tuple containing the GUID of the Signature Owner
#
# @retvalue A bytes object containing the correctly formatted data for the EFI_SIGNATURE_DATA
#
def GenEfiSigData(SignatureOwner):
    # In this context, the structure of an EFI_SIGNATURE_DATA is as follows:
    # 16 Bytes for the Guid of the signature owner
    # In theory there is one extra byte of information, however for the format
    # We are emitting that is the first byte of the signature, which will be
    # Handled by a separate function

    EsdSigOwnerBytes = pack("<IHHBBBBBBBB",*SignatureOwner)

    return EsdSigOwnerBytes

## Generates the Padding Bytes for a block of data to align it to 4 bytes as required
#
# @param DataSize   the size of any variable data in a variable such as the data or name
#
# @retval A bytes object consisting of the required padding
#
def GenPaddingBytes(DataSize):
    PaddingBytes = b''
    NumPaddingBytes = 4 - (DataSize % 4)
    if NumPaddingBytes == 4:
        return PaddingBytes
    else:
        for i in range (NumPaddingBytes):
            PaddingBytes += pack("<B",0xFF)
    return PaddingBytes

## Writes a complete Authenticated Variable for a certificate entry to the specified outfile,
# including all headers, names, data, and padding
#
# @param OutFile                The file object to be written to
# @param CertName               The name of the certificate as to be put in the blob as a string
# @param CertificatePath        The absolute path to the certificate
# @param AuthVarHeadAttributes  The attributes value to be passed to stored in the Auth Var Header
# @param VendorGuid             A tuple in the format (8,4,4,4,12) containing the VendorGuid
#
def WriteCertAuthVarb(OutFile, CertName, CertificatePath, AuthVarHeadAttributes, VendorGuid,CurrentTime):

    # The complete AuthVarb structure is as follows:
    # The Authenticated Variable Header
    # The Variable Name
    # The EFI SIGNATURE LIST
    # The EFT SIGNATURE DATA
    # The variable data
    # Paddding bytes to allign to 4 bytes wide

    # The size of the certificate in bytes, required

    CertificateSize = os.path.getsize(CertificatePath)

    #Each character in CertName takes up two bytes, and two bytes for null char
    CertNameLength = len(CertName) * 2 + 2
    # 28 is size of EFI_SIG_LIST, 16 size of EFI_SIG_DATA
    AuthVarHeadDataSize = CertificateSize + 28 + 16

    # This GUID is used for db, dbt, dbx, PK, and KEK. May need to be refactored
    EfiGlobVarGuid = (0x8BE4DF61, 0x93CA, 0x11d2, 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C)

    AuthVarHeadBytes = GenAuthVarbHeader(0x3f,AuthVarHeadAttributes,CertNameLength,AuthVarHeadDataSize,VendorGuid,CurrentTime)
    OutFile.write(AuthVarHeadBytes)
    NameBytes = GenName(CertName)
    OutFile.write(NameBytes)
    EfiSigListBytes = GenEfiSigList(AuthVarHeadDataSize,CertificateSize)
    OutFile.write(EfiSigListBytes)
    EfiSigDataBytes = GenEfiSigData(EfiGlobVarGuid)
    OutFile.write(EfiSigDataBytes)

    CertFile = open(CertificatePath,"rb")
    CertData = CertFile.read()
    CertFile.close()
    OutFile.write(CertData)
    # Padding
    PaddingBytes = GenPaddingBytes(CertificateSize + CertNameLength)
    OutFile.write(PaddingBytes)
    return


## Generate the bytes for an authenticated variable consisting of only a header, a name
# and a single byte of data
#
# @param AuthVariableName       The name of the Authenticated variable
# @param AuthVarState           The state field for the Authenticated Variable Header
# @param AuthVarHeadAttributes  The attributes field for the Authenticated Variable Header
# @param VendorGuid             The Vendor Guid for the Authenticated Variable header
# @param Data                   The Single Byte of Data
#
# @retval A bytes object containing the formated Authenticated Variable
def GenAuthVarbOnlyData(AuthVariableName, AuthVarState, AuthVarHeadAttributes, VendorGuid, Data):
    NameLength = len(AuthVariableName)*2 + 2
    CurrentTime = (0,0,0,0,0,0,0,0,0,0,0)

    AuthVarHeadBytes = GenAuthVarbHeader(AuthVarState,AuthVarHeadAttributes,NameLength,1,VendorGuid,CurrentTime)
    NameBytes = GenName(AuthVariableName)
    DataBytes = pack("<B",Data)
    # Do padding:

    VarSize = 1 + NameLength
    PaddingBytes = GenPaddingBytes(VarSize)

    return AuthVarHeadBytes + NameBytes + DataBytes + PaddingBytes

## Main method
#
# This method:
#  1- Initlialises an EdkLogger instance
#  2- Parses the input arguments
#  3- Generates a CFV file from the input certificates
#  4- Writes the CFV file
#
# @retval 0     Success.
# @retval 1     Error.
#
def Main():
    # Initialize an EdkLogger instance.
    EdkLogger.Initialize()
    try:

        CommandArgs = ParseArgs()
        if not CommandArgs:
            return 1
    except Exception as e:
        print(e)
        return 1

    # Generate Time Stamp Tuple for use in all AUTHENTICATED_VARIABLE_HEADER
    # Which from testing share all timestamp
    TimeStamp = GetTimeStamp()
    gEfiVendorKeysNvGuid = (0x9073e4e0, 0x60ec, 0x4b6e,  0x99, 0x3, 0x4c, 0x22, 0x3c, 0x26, 0xf, 0x3c)
    gEfiSecureBootEnableDisableGuid = (0xf0a30bc7, 0xaf08, 0x4556, 0x99, 0xc4, 0x0, 0x10, 0x9, 0xc9, 0x3a, 0x44)
    gEfiCustomModeEnableGuid = (0xc076ec0c, 0x7028, 0x4399, 0xa0, 0x72, 0x71, 0xee, 0x5c, 0x44, 0x8b, 0x9f)
    EfiImageSecurityDbGuid = (0xd719b2cb, 0x3d3a, 0x4596, 0xa3, 0xbc, 0xda, 0xd0, 0xe, 0x67, 0x65, 0x6f)
    EfiGlobVarGuid = (0x8BE4DF61, 0x93CA, 0x11d2, 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C)

    OutFile = open(CommandArgs.out,"wb")

    #Generate and write common headers
    if CommandArgs.fv:
        print ("Generate and emit FV Header")
        #Note: It may be good to add checks to verify the block maps.
        FvHeader = GenEfiFvHeader(0x928,(0x3,0x40000,0x0))
        OutFile.write(FvHeader)

    #Note: This value, may need dynamic setting.
    VsHeader = GenVarStoreHeader(0x10000)

    OutFile.write(VsHeader)

    # Write variables prior to certificates
    WriteCertAuthVarb(OutFile, "db",CommandArgs.db,0x27,EfiImageSecurityDbGuid,TimeStamp)
    WriteCertAuthVarb(OutFile,"dbx",CommandArgs.dbx,0x27,EfiImageSecurityDbGuid,TimeStamp)
    WriteCertAuthVarb(OutFile,"dbt",CommandArgs.dbt,0x27,EfiImageSecurityDbGuid,TimeStamp)
    WriteCertAuthVarb(OutFile,"KEK",CommandArgs.kek,0x27,EfiGlobVarGuid,TimeStamp)
    WriteCertAuthVarb(OutFile,"PK",CommandArgs.pk,0x27,EfiGlobVarGuid,TimeStamp)

    # Generate and then write Authenticated Vars at end
    VendorKeyBytes = GenAuthVarbOnlyData("VendorKeysNV",0x3f,0x23,gEfiVendorKeysNvGuid,1)
    OutFile.write(VendorKeyBytes)
    SecureBootEnableBytes = GenAuthVarbOnlyData("SecureBootEnable", 0x3f,0x03,gEfiSecureBootEnableDisableGuid,1)
    OutFile.write(SecureBootEnableBytes)
    CustomModeBytes = GenAuthVarbOnlyData("CustomMode",0x3f,0x03,gEfiCustomModeEnableGuid,0)
    OutFile.write(CustomModeBytes)
    OutFile.close()
    return 0

if __name__ == '__main__':
    r = Main()
    # 0-127 is a safe return range, and 1 is a standard default error
    if r < 0 or r > 127: r = 1
    sys.exit(r)
