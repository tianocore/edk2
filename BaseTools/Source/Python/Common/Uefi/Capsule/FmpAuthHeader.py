## @file
# Module that encodes and decodes a EFI_FIRMWARE_IMAGE_AUTHENTICATION with
# certificate data and payload data.
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
FmpAuthHeader
'''

import struct
import uuid

class FmpAuthHeaderClass (object):
    # ///
    # /// Image Attribute -Authentication Required
    # ///
    # typedef struct {
    #   ///
    #   /// It is included in the signature of AuthInfo. It is used to ensure freshness/no replay.
    #   /// It is incremented during each firmware image operation.
    #   ///
    #   UINT64                                  MonotonicCount;
    #   ///
    #   /// Provides the authorization for the firmware image operations. It is a signature across
    #   /// the image data and the Monotonic Count value. Caller uses the private key that is
    #   /// associated with a public key that has been provisioned via the key exchange.
    #   /// Because this is defined as a signature, WIN_CERTIFICATE_UEFI_GUID.CertType must
    #   /// be EFI_CERT_TYPE_PKCS7_GUID.
    #   ///
    #   WIN_CERTIFICATE_UEFI_GUID               AuthInfo;
    # } EFI_FIRMWARE_IMAGE_AUTHENTICATION;
    #
    # ///
    # /// Certificate which encapsulates a GUID-specific digital signature
    # ///
    # typedef struct {
    #   ///
    #   /// This is the standard WIN_CERTIFICATE header, where
    #   /// wCertificateType is set to WIN_CERT_TYPE_EFI_GUID.
    #   ///
    #   WIN_CERTIFICATE   Hdr;
    #   ///
    #   /// This is the unique id which determines the
    #   /// format of the CertData. .
    #   ///
    #   EFI_GUID          CertType;
    #   ///
    #   /// The following is the certificate data. The format of
    #   /// the data is determined by the CertType.
    #   /// If CertType is EFI_CERT_TYPE_RSA2048_SHA256_GUID,
    #   /// the CertData will be EFI_CERT_BLOCK_RSA_2048_SHA256 structure.
    #   ///
    #   UINT8            CertData[1];
    # } WIN_CERTIFICATE_UEFI_GUID;
    #
    # ///
    # /// The WIN_CERTIFICATE structure is part of the PE/COFF specification.
    # ///
    # typedef struct {
    #   ///
    #   /// The length of the entire certificate,
    #   /// including the length of the header, in bytes.
    #   ///
    #   UINT32  dwLength;
    #   ///
    #   /// The revision level of the WIN_CERTIFICATE
    #   /// structure. The current revision level is 0x0200.
    #   ///
    #   UINT16  wRevision;
    #   ///
    #   /// The certificate type. See WIN_CERT_TYPE_xxx for the UEFI
    #   /// certificate types. The UEFI specification reserves the range of
    #   /// certificate type values from 0x0EF0 to 0x0EFF.
    #   ///
    #   UINT16  wCertificateType;
    #   ///
    #   /// The following is the actual certificate. The format of
    #   /// the certificate depends on wCertificateType.
    #   ///
    #   /// UINT8 bCertificate[ANYSIZE_ARRAY];
    #   ///
    # } WIN_CERTIFICATE;
    #
    # #define WIN_CERT_TYPE_EFI_GUID         0x0EF1
    #
    # ///
    # /// This identifies a signature containing a DER-encoded PKCS #7 version 1.5 [RFC2315]
    # /// SignedData value.
    # ///
    # #define EFI_CERT_TYPE_PKCS7_GUID \
    #   { \
    #     0x4aafd29d, 0x68df, 0x49ee, {0x8a, 0xa9, 0x34, 0x7d, 0x37, 0x56, 0x65, 0xa7} \
    #   }

    _StructFormat = '<QIHH16s'
    _StructSize   = struct.calcsize (_StructFormat)

    _MonotonicCountFormat = '<Q'
    _MonotonicCountSize   = struct.calcsize (_MonotonicCountFormat)

    _StructAuthInfoFormat = '<IHH16s'
    _StructAuthInfoSize   = struct.calcsize (_StructAuthInfoFormat)

    _WIN_CERT_REVISION        = 0x0200
    _WIN_CERT_TYPE_EFI_GUID   = 0x0EF1
    _EFI_CERT_TYPE_PKCS7_GUID = uuid.UUID ('4aafd29d-68df-49ee-8aa9-347d375665a7')

    def __init__ (self):
        self._Valid              = False
        self.MonotonicCount      = 0
        self.dwLength            = self._StructAuthInfoSize
        self.wRevision           = self._WIN_CERT_REVISION
        self.wCertificateType    = self._WIN_CERT_TYPE_EFI_GUID
        self.CertType            = self._EFI_CERT_TYPE_PKCS7_GUID
        self.CertData            = b''
        self.Payload             = b''


    def Encode (self):
        if self.wRevision != self._WIN_CERT_REVISION:
            raise ValueError
        if self.wCertificateType != self._WIN_CERT_TYPE_EFI_GUID:
            raise ValueError
        if self.CertType != self._EFI_CERT_TYPE_PKCS7_GUID:
            raise ValueError
        self.dwLength = self._StructAuthInfoSize + len (self.CertData)

        FmpAuthHeader = struct.pack (
                                 self._StructFormat,
                                 self.MonotonicCount,
                                 self.dwLength,
                                 self.wRevision,
                                 self.wCertificateType,
                                 self.CertType.bytes_le
                                 )
        self._Valid = True

        return FmpAuthHeader + self.CertData + self.Payload

    def Decode (self, Buffer):
        if len (Buffer) < self._StructSize:
            raise ValueError
        (MonotonicCount, dwLength, wRevision, wCertificateType, CertType) = \
            struct.unpack (
                     self._StructFormat,
                     Buffer[0:self._StructSize]
                     )
        if dwLength < self._StructAuthInfoSize:
            raise ValueError
        if wRevision != self._WIN_CERT_REVISION:
            raise ValueError
        if wCertificateType != self._WIN_CERT_TYPE_EFI_GUID:
            raise ValueError
        if CertType != self._EFI_CERT_TYPE_PKCS7_GUID.bytes_le:
            raise ValueError
        self.MonotonicCount   = MonotonicCount
        self.dwLength         = dwLength
        self.wRevision        = wRevision
        self.wCertificateType = wCertificateType
        self.CertType         = uuid.UUID (bytes_le = CertType)
        self.CertData         = Buffer[self._StructSize:self._MonotonicCountSize + self.dwLength]
        self.Payload          = Buffer[self._MonotonicCountSize + self.dwLength:]
        self._Valid           = True
        return self.Payload

    def DumpInfo (self):
        if not self._Valid:
            raise ValueError
        print ('EFI_FIRMWARE_IMAGE_AUTHENTICATION.MonotonicCount                = {MonotonicCount:016X}'.format (MonotonicCount = self.MonotonicCount))
        print ('EFI_FIRMWARE_IMAGE_AUTHENTICATION.AuthInfo.Hdr.dwLength         = {dwLength:08X}'.format (dwLength = self.dwLength))
        print ('EFI_FIRMWARE_IMAGE_AUTHENTICATION.AuthInfo.Hdr.wRevision        = {wRevision:04X}'.format (wRevision = self.wRevision))
        print ('EFI_FIRMWARE_IMAGE_AUTHENTICATION.AuthInfo.Hdr.wCertificateType = {wCertificateType:04X}'.format (wCertificateType = self.wCertificateType))
        print ('EFI_FIRMWARE_IMAGE_AUTHENTICATION.AuthInfo.CertType             = {Guid}'.format (Guid = str(self.CertType).upper()))
        print ('sizeof (EFI_FIRMWARE_IMAGE_AUTHENTICATION.AuthInfo.CertData)    = {Size:08X}'.format (Size = len (self.CertData)))
        print ('sizeof (Payload)                                                = {Size:08X}'.format (Size = len (self.Payload)))
