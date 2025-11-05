#!/usr/bin/env python
# @ CommonUtility.py
# Common utility script
#
# Copyright (c) 2016 - 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import os
import sys
import shutil
import subprocess
import string
from ctypes import ARRAY, c_char, c_uint16, c_uint32, \
        c_uint8, Structure, sizeof
from importlib.machinery import SourceFileLoader
from SingleSign import single_sign_gen_pub_key


# Key types  defined should match with cryptolib.h
PUB_KEY_TYPE = {
    "RSA": 1,
    "ECC": 2,
    "DSA": 3,
    }

# Signing type schemes  defined should match with cryptolib.h
SIGN_TYPE_SCHEME = {
    "RSA_PKCS1": 1,
    "RSA_PSS": 2,
    "ECC": 3,
    "DSA": 4,
    }

# Hash values defined should match with cryptolib.h
HASH_TYPE_VALUE = {
    "SHA2_256": 1,
    "SHA2_384": 2,
    "SHA2_512": 3,
    "SM3_256": 4,
    }

# Hash values defined should match with cryptolib.h
HASH_VAL_STRING = dict(map(reversed, HASH_TYPE_VALUE.items()))

AUTH_TYPE_HASH_VALUE = {
    "SHA2_256": 1,
    "SHA2_384": 2,
    "SHA2_512": 3,
    "SM3_256": 4,
    "RSA2048SHA256": 1,
    "RSA3072SHA384": 2,
    }

HASH_DIGEST_SIZE = {
    "SHA2_256": 32,
    "SHA2_384": 48,
    "SHA2_512": 64,
    "SM3_256": 32,
    }


class PUB_KEY_HDR (Structure):
    _pack_ = 1
    _fields_ = [
        ('Identifier', ARRAY(c_char, 4)),      # signature ('P', 'U', 'B', 'K')
        ('KeySize',    c_uint16),              # Length of Public Key
        ('KeyType',    c_uint8),               # RSA or ECC
        ('Reserved',   ARRAY(c_uint8, 1)),
        ('KeyData',    ARRAY(c_uint8, 0)),
        ]

    def __init__(self):
        self.Identifier = b'PUBK'


class SIGNATURE_HDR (Structure):
    _pack_ = 1
    _fields_ = [
        ('Identifier', ARRAY(c_char, 4)),
        ('SigSize',    c_uint16),
        ('SigType',    c_uint8),
        ('HashAlg',    c_uint8),
        ('Signature',  ARRAY(c_uint8, 0)),
        ]

    def __init__(self):
        self.Identifier = b'SIGN'


class LZ_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('signature',       ARRAY(c_char, 4)),
        ('compressed_len',  c_uint32),
        ('length',          c_uint32),
        ('version',         c_uint16),
        ('svn',             c_uint8),
        ('attribute',       c_uint8)
    ]
    _compress_alg = {
        b'LZDM': 'Dummy',
        b'LZ4 ': 'Lz4',
        b'LZMA': 'Lzma',
    }


def print_bytes(data, indent=0, offset=0, show_ascii=False):
    bytes_per_line = 16
    printable = ' ' + string.ascii_letters + string.digits + string.punctuation
    str_fmt = '{:s}{:04x}: {:%ds} {:s}' % (bytes_per_line * 3)
    bytes_per_line
    data_array = bytearray(data)
    for idx in range(0, len(data_array), bytes_per_line):
        hex_str = ' '.join(
            '%02X' % val for val in data_array[idx:idx + bytes_per_line])
        asc_str = ''.join('%c' % (val if (chr(val) in printable) else '.')
                          for val in data_array[idx:idx + bytes_per_line])
        print(str_fmt.format(
            indent * ' ',
            offset + idx, hex_str,
            ' ' + asc_str if show_ascii else ''))


def get_bits_from_bytes(bytes, start, length):
    if length == 0:
        return 0
    byte_start = (start) // 8
    byte_end = (start + length - 1) // 8
    bit_start = start & 7
    mask = (1 << length) - 1
    val = bytes_to_value(bytes[byte_start:byte_end + 1])
    val = (val >> bit_start) & mask
    return val


def set_bits_to_bytes(bytes, start, length, bvalue):
    if length == 0:
        return
    byte_start = (start) // 8
    byte_end = (start + length - 1) // 8
    bit_start = start & 7
    mask = (1 << length) - 1
    val = bytes_to_value(bytes[byte_start:byte_end + 1])
    val &= ~(mask << bit_start)
    val |= ((bvalue & mask) << bit_start)
    bytes[byte_start:byte_end+1] = value_to_bytearray(
        val,
        byte_end + 1 - byte_start)


def value_to_bytes(value, length):
    return value.to_bytes(length, 'little')


def bytes_to_value(bytes):
    return int.from_bytes(bytes, 'little')


def value_to_bytearray(value, length):
    return bytearray(value_to_bytes(value, length))

# def value_to_bytearray (value, length):
    return bytearray(value_to_bytes(value, length))


def get_aligned_value(value, alignment=4):
    if alignment != (1 << (alignment.bit_length() - 1)):
        raise Exception(
            'Alignment (0x%x) should to be power of 2 !' % alignment)
    value = (value + (alignment - 1)) & ~(alignment - 1)
    return value


def get_padding_length(data_len, alignment=4):
    new_data_len = get_aligned_value(data_len, alignment)
    return new_data_len - data_len


def get_file_data(file, mode='rb'):
    return open(file, mode).read()


def gen_file_from_object(file, object):
    open(file, 'wb').write(object)


def gen_file_with_size(file, size):
    open(file, 'wb').write(b'\xFF' * size)


def check_files_exist(base_name_list, dir='', ext=''):
    for each in base_name_list:
        if not os.path.exists(os.path.join(dir, each + ext)):
            return False
    return True


def load_source(name, filepath):
    mod = SourceFileLoader(name, filepath).load_module()
    return mod


def get_openssl_path():
    if os.name == 'nt':
        if 'OPENSSL_PATH' not in os.environ:
            openssl_dir = "C:\\Openssl\\bin\\"
            if os.path.exists(openssl_dir):
                os.environ['OPENSSL_PATH'] = openssl_dir
            else:
                os.environ['OPENSSL_PATH'] = "C:\\Openssl\\"
                if 'OPENSSL_CONF' not in os.environ:
                    openssl_cfg = "C:\\Openssl\\openssl.cfg"
                    if os.path.exists(openssl_cfg):
                        os.environ['OPENSSL_CONF'] = openssl_cfg
        openssl = os.path.join(
            os.environ.get('OPENSSL_PATH', ''),
            'openssl.exe')
    else:
        # Get openssl path for Linux cases
        openssl = shutil.which('openssl')

    return openssl


def run_process(arg_list, print_cmd=False, capture_out=False):
    sys.stdout.flush()
    if os.name == 'nt' and os.path.splitext(arg_list[0])[1] == '' and \
       os.path.exists(arg_list[0] + '.exe'):
        arg_list[0] += '.exe'
    if print_cmd:
        print(' '.join(arg_list))

    exc = None
    result = 0
    output = ''
    try:
        if capture_out:
            output = subprocess.check_output(arg_list).decode()
        else:
            result = subprocess.call(arg_list)
    except Exception as ex:
        result = 1
        exc = ex

    if result:
        if not print_cmd:
            print('Error in running process:\n  %s' % ' '.join(arg_list))
        if exc is None:
            sys.exit(1)
        else:
            raise exc

    return output


# Adjust hash type algorithm based on Public key file
def adjust_hash_type(pub_key_file):
    key_type = get_key_type(pub_key_file)
    if key_type == 'RSA2048':
        hash_type = 'SHA2_256'
    elif key_type == 'RSA3072':
        hash_type = 'SHA2_384'
    else:
        hash_type = None

    return hash_type


def rsa_sign_file(
      priv_key, pub_key, hash_type, sign_scheme,
      in_file, out_file, inc_dat=False, inc_key=False):

    bins = bytearray()
    if inc_dat:
        bins.extend(get_file_data(in_file))


# def single_sign_file(priv_key, hash_type, sign_scheme, in_file, out_file):

    out_data = get_file_data(out_file)

    sign = SIGNATURE_HDR()
    sign.SigSize = len(out_data)
    sign.SigType = SIGN_TYPE_SCHEME[sign_scheme]
    sign.HashAlg = HASH_TYPE_VALUE[hash_type]

    bins.extend(bytearray(sign) + out_data)
    if inc_key:
        key = gen_pub_key(priv_key, pub_key)
        bins.extend(key)

    if len(bins) != len(out_data):
        gen_file_from_object(out_file, bins)


def get_key_type(in_key):

    # Check in_key is file or key Id
    if not os.path.exists(in_key):
        key = bytearray(gen_pub_key(in_key))
    else:
        # Check for public key in binary format.
        key = bytearray(get_file_data(in_key))

    pub_key_hdr = PUB_KEY_HDR.from_buffer(key)
    if pub_key_hdr.Identifier != b'PUBK':
        pub_key = gen_pub_key(in_key)
        pub_key_hdr = PUB_KEY_HDR.from_buffer(pub_key)

    key_type = next(
        (key for key,
            value in PUB_KEY_TYPE.items() if value == pub_key_hdr.KeyType))
    return '%s%d' % (key_type, (pub_key_hdr.KeySize - 4) * 8)


def get_auth_hash_type(key_type, sign_scheme):
    if key_type == "RSA2048" and sign_scheme == "RSA_PKCS1":
        hash_type = 'SHA2_256'
        auth_type = 'RSA2048_PKCS1_SHA2_256'
    elif key_type == "RSA3072" and sign_scheme == "RSA_PKCS1":
        hash_type = 'SHA2_384'
        auth_type = 'RSA3072_PKCS1_SHA2_384'
    elif key_type == "RSA2048" and sign_scheme == "RSA_PSS":
        hash_type = 'SHA2_256'
        auth_type = 'RSA2048_PSS_SHA2_256'
    elif key_type == "RSA3072" and sign_scheme == "RSA_PSS":
        hash_type = 'SHA2_384'
        auth_type = 'RSA3072_PSS_SHA2_384'
    else:
        hash_type = ''
        auth_type = ''
    return auth_type, hash_type


# def single_sign_gen_pub_key(in_key, pub_key_file=None):


def gen_pub_key(in_key, pub_key=None):

    keydata = single_sign_gen_pub_key(in_key, pub_key)

    publickey = PUB_KEY_HDR()
    publickey.KeySize = len(keydata)
    publickey.KeyType = PUB_KEY_TYPE['RSA']

    key = bytearray(publickey) + keydata

    if pub_key:
        gen_file_from_object(pub_key, key)

    return key


def decompress(in_file, out_file, tool_dir=''):
    if not os.path.isfile(in_file):
        raise Exception("Invalid input file '%s' !" % in_file)

    # Remove the Lz Header
    fi = open(in_file, 'rb')
    di = bytearray(fi.read())
    fi.close()

    lz_hdr = LZ_HEADER.from_buffer(di)
    offset = sizeof(lz_hdr)
    if lz_hdr.signature == b"LZDM" or lz_hdr.compressed_len == 0:
        fo = open(out_file, 'wb')
        fo.write(di[offset:offset + lz_hdr.compressed_len])
        fo.close()
        return

    temp = os.path.splitext(out_file)[0] + '.tmp'
    if lz_hdr.signature == b"LZMA":
        alg = "Lzma"
    elif lz_hdr.signature == b"LZ4 ":
        alg = "Lz4"
    else:
        raise Exception("Unsupported compression '%s' !" % lz_hdr.signature)

    fo = open(temp, 'wb')
    fo.write(di[offset:offset + lz_hdr.compressed_len])
    fo.close()

    compress_tool = "%sCompress" % alg
    if alg == "Lz4":
        try:
            cmdline = [
                os.path.join(tool_dir, compress_tool),
                "-d",
                "-o", out_file,
                temp]
            run_process(cmdline, False, True)
        except Exception:
            msg_string = "Could not find/use CompressLz4 tool, " \
                        "trying with python lz4..."
            print(msg_string)
            try:
                import lz4.block
                if lz4.VERSION != '3.1.1':
                    msg_string = "Recommended lz4 module version " \
                                "is '3.1.1'," + lz4.VERSION \
                                + " is currently installed."
                    print(msg_string)
            except ImportError:
                msg_string = "Could not import lz4, use " \
                            "'python -m pip install lz4==3.1.1' " \
                            "to install it."
                print(msg_string)
                exit(1)
            decompress_data = lz4.block.decompress(get_file_data(temp))
            with open(out_file, "wb") as lz4bin:
                lz4bin.write(decompress_data)
    else:
        cmdline = [
            os.path.join(tool_dir, compress_tool),
            "-d",
            "-o", out_file,
            temp]
        run_process(cmdline, False, True)
    os.remove(temp)


def compress(in_file, alg, svn=0, out_path='', tool_dir=''):
    if not os.path.isfile(in_file):
        raise Exception("Invalid input file '%s' !" % in_file)

    basename, ext = os.path.splitext(os.path.basename(in_file))
    if out_path:
        if os.path.isdir(out_path):
            out_file = os.path.join(out_path, basename + '.lz')
        else:
            out_file = os.path.join(out_path)
    else:
        out_file = os.path.splitext(in_file)[0] + '.lz'

    if alg == "Lzma":
        sig = "LZMA"
    elif alg == "Tiano":
        sig = "LZUF"
    elif alg == "Lz4":
        sig = "LZ4 "
    elif alg == "Dummy":
        sig = "LZDM"
    else:
        raise Exception("Unsupported compression '%s' !" % alg)

    in_len = os.path.getsize(in_file)
    if in_len > 0:
        compress_tool = "%sCompress" % alg
        if sig == "LZDM":
            shutil.copy(in_file, out_file)
            compress_data = get_file_data(out_file)
        elif sig == "LZ4 ":
            try:
                cmdline = [
                    os.path.join(tool_dir, compress_tool),
                    "-e",
                    "-o", out_file,
                    in_file]
                run_process(cmdline, False, True)
                compress_data = get_file_data(out_file)
            except Exception:
                msg_string = "Could not find/use CompressLz4 tool, " \
                            "trying with python lz4..."
                print(msg_string)
                try:
                    import lz4.block
                    if lz4.VERSION != '3.1.1':
                        msg_string = "Recommended lz4 module version " \
                                    "is '3.1.1', " + lz4.VERSION \
                                    + " is currently installed."
                        print(msg_string)
                except ImportError:
                    msg_string = "Could not import lz4, use " \
                                "'python -m pip install lz4==3.1.1' " \
                                "to install it."
                    print(msg_string)
                    exit(1)
                compress_data = lz4.block.compress(
                    get_file_data(in_file),
                    mode='high_compression')
        elif sig == "LZMA":
            cmdline = [
                os.path.join(tool_dir, compress_tool),
                "-e",
                "-o", out_file,
                in_file]
            run_process(cmdline, False, True)
            compress_data = get_file_data(out_file)
    else:
        compress_data = bytearray()

    lz_hdr = LZ_HEADER()
    lz_hdr.signature = sig.encode()
    lz_hdr.svn = svn
    lz_hdr.compressed_len = len(compress_data)
    lz_hdr.length = os.path.getsize(in_file)
    data = bytearray()
    data.extend(lz_hdr)
    data.extend(compress_data)
    gen_file_from_object(out_file, data)

    return out_file
