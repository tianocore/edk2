; WARNING: do not edit!
; Generated from openssl/crypto/modes/asm/aesni-gcm-x86_64.pl
;
; Copyright 2013-2020 The OpenSSL Project Authors. All Rights Reserved.
;
; Licensed under the OpenSSL license (the "License").  You may not use
; this file except in compliance with the License.  You can obtain a copy
; in the file LICENSE in the source distribution or at
; https://www.openssl.org/source/license.html

default rel
%define XMMWORD
%define YMMWORD
%define ZMMWORD
section .text code align=64


global  aesni_gcm_encrypt

aesni_gcm_encrypt:

        xor     eax,eax
        DB      0F3h,0C3h               ;repret



global  aesni_gcm_decrypt

aesni_gcm_decrypt:

        xor     eax,eax
        DB      0F3h,0C3h               ;repret


