; WARNING: do not edit!
; Generated from openssl/crypto/modes/asm/ghash-x86.pl
;
; Copyright 2010-2020 The OpenSSL Project Authors. All Rights Reserved.
;
; Licensed under the OpenSSL license (the "License").  You may not use
; this file except in compliance with the License.  You can obtain a copy
; in the file LICENSE in the source distribution or at
; https://www.openssl.org/source/license.html

%ifidn __OUTPUT_FORMAT__,obj
section code    use32 class=code align=64
%elifidn __OUTPUT_FORMAT__,win32
$@feat.00 equ 1
section .text   code align=64
%else
section .text   code
%endif
global  _gcm_gmult_4bit_x86
align   16
_gcm_gmult_4bit_x86:
L$_gcm_gmult_4bit_x86_begin:
        push    ebp
        push    ebx
        push    esi
        push    edi
        sub     esp,84
        mov     edi,DWORD [104+esp]
        mov     esi,DWORD [108+esp]
        mov     ebp,DWORD [edi]
        mov     edx,DWORD [4+edi]
        mov     ecx,DWORD [8+edi]
        mov     ebx,DWORD [12+edi]
        mov     DWORD [16+esp],0
        mov     DWORD [20+esp],471859200
        mov     DWORD [24+esp],943718400
        mov     DWORD [28+esp],610271232
        mov     DWORD [32+esp],1887436800
        mov     DWORD [36+esp],1822425088
        mov     DWORD [40+esp],1220542464
        mov     DWORD [44+esp],1423966208
        mov     DWORD [48+esp],3774873600
        mov     DWORD [52+esp],4246732800
        mov     DWORD [56+esp],3644850176
        mov     DWORD [60+esp],3311403008
        mov     DWORD [64+esp],2441084928
        mov     DWORD [68+esp],2376073216
        mov     DWORD [72+esp],2847932416
        mov     DWORD [76+esp],3051356160
        mov     DWORD [esp],ebp
        mov     DWORD [4+esp],edx
        mov     DWORD [8+esp],ecx
        mov     DWORD [12+esp],ebx
        shr     ebx,20
        and     ebx,240
        mov     ebp,DWORD [4+ebx*1+esi]
        mov     edx,DWORD [ebx*1+esi]
        mov     ecx,DWORD [12+ebx*1+esi]
        mov     ebx,DWORD [8+ebx*1+esi]
        xor     eax,eax
        mov     edi,15
        jmp     NEAR L$000x86_loop
align   16
L$000x86_loop:
        mov     al,bl
        shrd    ebx,ecx,4
        and     al,15
        shrd    ecx,edx,4
        shrd    edx,ebp,4
        shr     ebp,4
        xor     ebp,DWORD [16+eax*4+esp]
        mov     al,BYTE [edi*1+esp]
        and     al,240
        xor     ebx,DWORD [8+eax*1+esi]
        xor     ecx,DWORD [12+eax*1+esi]
        xor     edx,DWORD [eax*1+esi]
        xor     ebp,DWORD [4+eax*1+esi]
        dec     edi
        js      NEAR L$001x86_break
        mov     al,bl
        shrd    ebx,ecx,4
        and     al,15
        shrd    ecx,edx,4
        shrd    edx,ebp,4
        shr     ebp,4
        xor     ebp,DWORD [16+eax*4+esp]
        mov     al,BYTE [edi*1+esp]
        shl     al,4
        xor     ebx,DWORD [8+eax*1+esi]
        xor     ecx,DWORD [12+eax*1+esi]
        xor     edx,DWORD [eax*1+esi]
        xor     ebp,DWORD [4+eax*1+esi]
        jmp     NEAR L$000x86_loop
align   16
L$001x86_break:
        bswap   ebx
        bswap   ecx
        bswap   edx
        bswap   ebp
        mov     edi,DWORD [104+esp]
        mov     DWORD [12+edi],ebx
        mov     DWORD [8+edi],ecx
        mov     DWORD [4+edi],edx
        mov     DWORD [edi],ebp
        add     esp,84
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        ret
global  _gcm_ghash_4bit_x86
align   16
_gcm_ghash_4bit_x86:
L$_gcm_ghash_4bit_x86_begin:
        push    ebp
        push    ebx
        push    esi
        push    edi
        sub     esp,84
        mov     ebx,DWORD [104+esp]
        mov     esi,DWORD [108+esp]
        mov     edi,DWORD [112+esp]
        mov     ecx,DWORD [116+esp]
        add     ecx,edi
        mov     DWORD [116+esp],ecx
        mov     ebp,DWORD [ebx]
        mov     edx,DWORD [4+ebx]
        mov     ecx,DWORD [8+ebx]
        mov     ebx,DWORD [12+ebx]
        mov     DWORD [16+esp],0
        mov     DWORD [20+esp],471859200
        mov     DWORD [24+esp],943718400
        mov     DWORD [28+esp],610271232
        mov     DWORD [32+esp],1887436800
        mov     DWORD [36+esp],1822425088
        mov     DWORD [40+esp],1220542464
        mov     DWORD [44+esp],1423966208
        mov     DWORD [48+esp],3774873600
        mov     DWORD [52+esp],4246732800
        mov     DWORD [56+esp],3644850176
        mov     DWORD [60+esp],3311403008
        mov     DWORD [64+esp],2441084928
        mov     DWORD [68+esp],2376073216
        mov     DWORD [72+esp],2847932416
        mov     DWORD [76+esp],3051356160
align   16
L$002x86_outer_loop:
        xor     ebx,DWORD [12+edi]
        xor     ecx,DWORD [8+edi]
        xor     edx,DWORD [4+edi]
        xor     ebp,DWORD [edi]
        mov     DWORD [12+esp],ebx
        mov     DWORD [8+esp],ecx
        mov     DWORD [4+esp],edx
        mov     DWORD [esp],ebp
        shr     ebx,20
        and     ebx,240
        mov     ebp,DWORD [4+ebx*1+esi]
        mov     edx,DWORD [ebx*1+esi]
        mov     ecx,DWORD [12+ebx*1+esi]
        mov     ebx,DWORD [8+ebx*1+esi]
        xor     eax,eax
        mov     edi,15
        jmp     NEAR L$003x86_loop
align   16
L$003x86_loop:
        mov     al,bl
        shrd    ebx,ecx,4
        and     al,15
        shrd    ecx,edx,4
        shrd    edx,ebp,4
        shr     ebp,4
        xor     ebp,DWORD [16+eax*4+esp]
        mov     al,BYTE [edi*1+esp]
        and     al,240
        xor     ebx,DWORD [8+eax*1+esi]
        xor     ecx,DWORD [12+eax*1+esi]
        xor     edx,DWORD [eax*1+esi]
        xor     ebp,DWORD [4+eax*1+esi]
        dec     edi
        js      NEAR L$004x86_break
        mov     al,bl
        shrd    ebx,ecx,4
        and     al,15
        shrd    ecx,edx,4
        shrd    edx,ebp,4
        shr     ebp,4
        xor     ebp,DWORD [16+eax*4+esp]
        mov     al,BYTE [edi*1+esp]
        shl     al,4
        xor     ebx,DWORD [8+eax*1+esi]
        xor     ecx,DWORD [12+eax*1+esi]
        xor     edx,DWORD [eax*1+esi]
        xor     ebp,DWORD [4+eax*1+esi]
        jmp     NEAR L$003x86_loop
align   16
L$004x86_break:
        bswap   ebx
        bswap   ecx
        bswap   edx
        bswap   ebp
        mov     edi,DWORD [112+esp]
        lea     edi,[16+edi]
        cmp     edi,DWORD [116+esp]
        mov     DWORD [112+esp],edi
        jb      NEAR L$002x86_outer_loop
        mov     edi,DWORD [104+esp]
        mov     DWORD [12+edi],ebx
        mov     DWORD [8+edi],ecx
        mov     DWORD [4+edi],edx
        mov     DWORD [edi],ebp
        add     esp,84
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        ret
align   16
__mmx_gmult_4bit_inner:
        xor     ecx,ecx
        mov     edx,ebx
        mov     cl,dl
        shl     cl,4
        and     edx,240
        movq    mm0,[8+ecx*1+esi]
        movq    mm1,[ecx*1+esi]
        movd    ebp,mm0
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [14+edi]
        psllq   mm2,60
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [13+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [12+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [11+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [10+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [9+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [8+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [7+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [6+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [5+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [4+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [3+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [2+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [1+edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        mov     cl,BYTE [edi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        mov     edx,ecx
        movd    ebx,mm0
        pxor    mm0,mm2
        shl     cl,4
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+ecx*1+esi]
        psllq   mm2,60
        and     edx,240
        pxor    mm1,[ebp*8+eax]
        and     ebx,15
        pxor    mm1,[ecx*1+esi]
        movd    ebp,mm0
        pxor    mm0,mm2
        psrlq   mm0,4
        movq    mm2,mm1
        psrlq   mm1,4
        pxor    mm0,[8+edx*1+esi]
        psllq   mm2,60
        pxor    mm1,[ebx*8+eax]
        and     ebp,15
        pxor    mm1,[edx*1+esi]
        movd    ebx,mm0
        pxor    mm0,mm2
        mov     edi,DWORD [4+ebp*8+eax]
        psrlq   mm0,32
        movd    edx,mm1
        psrlq   mm1,32
        movd    ecx,mm0
        movd    ebp,mm1
        shl     edi,4
        bswap   ebx
        bswap   edx
        bswap   ecx
        xor     ebp,edi
        bswap   ebp
        ret
global  _gcm_gmult_4bit_mmx
align   16
_gcm_gmult_4bit_mmx:
L$_gcm_gmult_4bit_mmx_begin:
        push    ebp
        push    ebx
        push    esi
        push    edi
        mov     edi,DWORD [20+esp]
        mov     esi,DWORD [24+esp]
        call    L$005pic_point
L$005pic_point:
        pop     eax
        lea     eax,[(L$rem_4bit-L$005pic_point)+eax]
        movzx   ebx,BYTE [15+edi]
        call    __mmx_gmult_4bit_inner
        mov     edi,DWORD [20+esp]
        emms
        mov     DWORD [12+edi],ebx
        mov     DWORD [4+edi],edx
        mov     DWORD [8+edi],ecx
        mov     DWORD [edi],ebp
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        ret
global  _gcm_ghash_4bit_mmx
align   16
_gcm_ghash_4bit_mmx:
L$_gcm_ghash_4bit_mmx_begin:
        push    ebp
        push    ebx
        push    esi
        push    edi
        mov     ebp,DWORD [20+esp]
        mov     esi,DWORD [24+esp]
        mov     edi,DWORD [28+esp]
        mov     ecx,DWORD [32+esp]
        call    L$006pic_point
L$006pic_point:
        pop     eax
        lea     eax,[(L$rem_4bit-L$006pic_point)+eax]
        add     ecx,edi
        mov     DWORD [32+esp],ecx
        sub     esp,20
        mov     ebx,DWORD [12+ebp]
        mov     edx,DWORD [4+ebp]
        mov     ecx,DWORD [8+ebp]
        mov     ebp,DWORD [ebp]
        jmp     NEAR L$007mmx_outer_loop
align   16
L$007mmx_outer_loop:
        xor     ebx,DWORD [12+edi]
        xor     edx,DWORD [4+edi]
        xor     ecx,DWORD [8+edi]
        xor     ebp,DWORD [edi]
        mov     DWORD [48+esp],edi
        mov     DWORD [12+esp],ebx
        mov     DWORD [4+esp],edx
        mov     DWORD [8+esp],ecx
        mov     DWORD [esp],ebp
        mov     edi,esp
        shr     ebx,24
        call    __mmx_gmult_4bit_inner
        mov     edi,DWORD [48+esp]
        lea     edi,[16+edi]
        cmp     edi,DWORD [52+esp]
        jb      NEAR L$007mmx_outer_loop
        mov     edi,DWORD [40+esp]
        emms
        mov     DWORD [12+edi],ebx
        mov     DWORD [4+edi],edx
        mov     DWORD [8+edi],ecx
        mov     DWORD [edi],ebp
        add     esp,20
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        ret
align   64
L$rem_4bit:
dd      0,0,0,29491200,0,58982400,0,38141952
dd      0,117964800,0,113901568,0,76283904,0,88997888
dd      0,235929600,0,265420800,0,227803136,0,206962688
dd      0,152567808,0,148504576,0,177995776,0,190709760
db      71,72,65,83,72,32,102,111,114,32,120,56,54,44,32,67
db      82,89,80,84,79,71,65,77,83,32,98,121,32,60,97,112
db      112,114,111,64,111,112,101,110,115,115,108,46,111,114,103,62
db      0
