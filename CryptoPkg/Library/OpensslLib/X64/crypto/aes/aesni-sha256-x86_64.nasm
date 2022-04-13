; WARNING: do not edit!
; Generated from openssl/crypto/aes/asm/aesni-sha256-x86_64.pl
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


EXTERN  OPENSSL_ia32cap_P
global  aesni_cbc_sha256_enc

ALIGN   16
aesni_cbc_sha256_enc:

        xor     eax,eax
        cmp     rcx,0
        je      NEAR $L$probe
        ud2
$L$probe:
        DB      0F3h,0C3h               ;repret



ALIGN   64

K256:
        DD      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5
        DD      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5
        DD      0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5
        DD      0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5
        DD      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3
        DD      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3
        DD      0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174
        DD      0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174
        DD      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc
        DD      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc
        DD      0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da
        DD      0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da
        DD      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7
        DD      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7
        DD      0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967
        DD      0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967
        DD      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13
        DD      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13
        DD      0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85
        DD      0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85
        DD      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
        DD      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
        DD      0xd192e819,0xd6990624,0xf40e3585,0x106aa070
        DD      0xd192e819,0xd6990624,0xf40e3585,0x106aa070
        DD      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5
        DD      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5
        DD      0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
        DD      0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
        DD      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208
        DD      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208
        DD      0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        DD      0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2

        DD      0x00010203,0x04050607,0x08090a0b,0x0c0d0e0f
        DD      0x00010203,0x04050607,0x08090a0b,0x0c0d0e0f
        DD      0,0,0,0,0,0,0,0,-1,-1,-1,-1
        DD      0,0,0,0,0,0,0,0
DB      65,69,83,78,73,45,67,66,67,43,83,72,65,50,53,54
DB      32,115,116,105,116,99,104,32,102,111,114,32,120,56,54,95
DB      54,52,44,32,67,82,89,80,84,79,71,65,77,83,32,98
DB      121,32,60,97,112,112,114,111,64,111,112,101,110,115,115,108
DB      46,111,114,103,62,0
ALIGN   64
