# WARNING: do not edit!
# Generated from openssl/crypto/modes/asm/aesni-gcm-x86_64.pl
#
# Copyright 2013-2020 The OpenSSL Project Authors. All Rights Reserved.
#
# Licensed under the OpenSSL license (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.openssl.org/source/license.html

.text

.globl  aesni_gcm_encrypt
.type   aesni_gcm_encrypt,@function
aesni_gcm_encrypt:
.cfi_startproc
        xorl    %eax,%eax
        .byte   0xf3,0xc3
.cfi_endproc
.size   aesni_gcm_encrypt,.-aesni_gcm_encrypt

.globl  aesni_gcm_decrypt
.type   aesni_gcm_decrypt,@function
aesni_gcm_decrypt:
.cfi_startproc
        xorl    %eax,%eax
        .byte   0xf3,0xc3
.cfi_endproc
.size   aesni_gcm_decrypt,.-aesni_gcm_decrypt
