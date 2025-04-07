default rel
%define XMMWORD
%define YMMWORD
%define ZMMWORD
section .text code align=64


global  ossl_rsaz_amm52x40_x1_avxifma256

ALIGN   32
ossl_rsaz_amm52x40_x1_avxifma256:
        mov     QWORD[8+rsp],rdi        ;WIN64 prologue
        mov     QWORD[16+rsp],rsi
        mov     rax,rsp
$L$SEH_begin_ossl_rsaz_amm52x40_x1_avxifma256:
        mov     rdi,rcx
        mov     rsi,rdx
        mov     rdx,r8
        mov     rcx,r9
        mov     r8,QWORD[40+rsp]



DB      243,15,30,250
        push    rbx

        push    rbp

        push    r12

        push    r13

        push    r14

        push    r15

        lea     rsp,[((-168))+rsp]
        vmovapd XMMWORD[rsp],xmm6
        vmovapd XMMWORD[16+rsp],xmm7
        vmovapd XMMWORD[32+rsp],xmm8
        vmovapd XMMWORD[48+rsp],xmm9
        vmovapd XMMWORD[64+rsp],xmm10
        vmovapd XMMWORD[80+rsp],xmm11
        vmovapd XMMWORD[96+rsp],xmm12
        vmovapd XMMWORD[112+rsp],xmm13
        vmovapd XMMWORD[128+rsp],xmm14
        vmovapd XMMWORD[144+rsp],xmm15
$L$ossl_rsaz_amm52x40_x1_avxifma256_body:

        vpxor   ymm0,ymm0,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0
        vmovapd ymm8,ymm0
        vmovapd ymm9,ymm0
        vmovapd ymm10,ymm0
        vmovapd ymm11,ymm0
        vmovapd ymm12,ymm0

        xor     r9d,r9d

        mov     r11,rdx
        mov     rax,0xfffffffffffff


        mov     ebx,10

ALIGN   32
$L$loop10:
        mov     r13,QWORD[r11]

        vpbroadcastq    ymm1,QWORD[r11]
        mov     rdx,QWORD[rsi]
        mulx    r12,r13,r13
        add     r9,r13
        mov     r10,r12
        adc     r10,0

        mov     r13,r8
        imul    r13,r9
        and     r13,rax

        vmovq   xmm2,r13
        vpbroadcastq    ymm2,xmm2
        mov     rdx,QWORD[rcx]
        mulx    r12,r13,r13
        add     r9,r13
        adc     r10,r12

        shr     r9,52
        sal     r10,12
        or      r9,r10

        lea     rsp,[((-328))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52luq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52luq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52luq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52luq     ymm12,ymm2,YMMWORD[288+rcx]
        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        vmovdqu YMMWORD[256+rsp],ymm11
        vmovdqu YMMWORD[288+rsp],ymm12
        mov     QWORD[320+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]
        vmovdqu ymm11,YMMWORD[264+rsp]
        vmovdqu ymm12,YMMWORD[296+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52huq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52huq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52huq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52huq     ymm12,ymm2,YMMWORD[288+rcx]
        lea     rsp,[328+rsp]
        mov     r13,QWORD[8+r11]

        vpbroadcastq    ymm1,QWORD[8+r11]
        mov     rdx,QWORD[rsi]
        mulx    r12,r13,r13
        add     r9,r13
        mov     r10,r12
        adc     r10,0

        mov     r13,r8
        imul    r13,r9
        and     r13,rax

        vmovq   xmm2,r13
        vpbroadcastq    ymm2,xmm2
        mov     rdx,QWORD[rcx]
        mulx    r12,r13,r13
        add     r9,r13
        adc     r10,r12

        shr     r9,52
        sal     r10,12
        or      r9,r10

        lea     rsp,[((-328))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52luq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52luq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52luq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52luq     ymm12,ymm2,YMMWORD[288+rcx]
        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        vmovdqu YMMWORD[256+rsp],ymm11
        vmovdqu YMMWORD[288+rsp],ymm12
        mov     QWORD[320+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]
        vmovdqu ymm11,YMMWORD[264+rsp]
        vmovdqu ymm12,YMMWORD[296+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52huq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52huq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52huq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52huq     ymm12,ymm2,YMMWORD[288+rcx]
        lea     rsp,[328+rsp]
        mov     r13,QWORD[16+r11]

        vpbroadcastq    ymm1,QWORD[16+r11]
        mov     rdx,QWORD[rsi]
        mulx    r12,r13,r13
        add     r9,r13
        mov     r10,r12
        adc     r10,0

        mov     r13,r8
        imul    r13,r9
        and     r13,rax

        vmovq   xmm2,r13
        vpbroadcastq    ymm2,xmm2
        mov     rdx,QWORD[rcx]
        mulx    r12,r13,r13
        add     r9,r13
        adc     r10,r12

        shr     r9,52
        sal     r10,12
        or      r9,r10

        lea     rsp,[((-328))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52luq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52luq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52luq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52luq     ymm12,ymm2,YMMWORD[288+rcx]
        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        vmovdqu YMMWORD[256+rsp],ymm11
        vmovdqu YMMWORD[288+rsp],ymm12
        mov     QWORD[320+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]
        vmovdqu ymm11,YMMWORD[264+rsp]
        vmovdqu ymm12,YMMWORD[296+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52huq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52huq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52huq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52huq     ymm12,ymm2,YMMWORD[288+rcx]
        lea     rsp,[328+rsp]
        mov     r13,QWORD[24+r11]

        vpbroadcastq    ymm1,QWORD[24+r11]
        mov     rdx,QWORD[rsi]
        mulx    r12,r13,r13
        add     r9,r13
        mov     r10,r12
        adc     r10,0

        mov     r13,r8
        imul    r13,r9
        and     r13,rax

        vmovq   xmm2,r13
        vpbroadcastq    ymm2,xmm2
        mov     rdx,QWORD[rcx]
        mulx    r12,r13,r13
        add     r9,r13
        adc     r10,r12

        shr     r9,52
        sal     r10,12
        or      r9,r10

        lea     rsp,[((-328))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52luq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52luq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52luq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52luq     ymm12,ymm2,YMMWORD[288+rcx]
        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        vmovdqu YMMWORD[256+rsp],ymm11
        vmovdqu YMMWORD[288+rsp],ymm12
        mov     QWORD[320+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]
        vmovdqu ymm11,YMMWORD[264+rsp]
        vmovdqu ymm12,YMMWORD[296+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52huq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52huq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52huq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52huq     ymm12,ymm2,YMMWORD[288+rcx]
        lea     rsp,[328+rsp]
        lea     r11,[32+r11]
        dec     ebx
        jne     NEAR $L$loop10

        vmovq   xmm0,r9
        vpbroadcastq    ymm0,xmm0
        vpblendd        ymm3,ymm3,ymm0,3

        lea     rsp,[((-640))+rsp]
        vmovupd YMMWORD[rsp],ymm3
        vmovupd YMMWORD[32+rsp],ymm4
        vmovupd YMMWORD[64+rsp],ymm5
        vmovupd YMMWORD[96+rsp],ymm6
        vmovupd YMMWORD[128+rsp],ymm7
        vmovupd YMMWORD[160+rsp],ymm8
        vmovupd YMMWORD[192+rsp],ymm9
        vmovupd YMMWORD[224+rsp],ymm10
        vmovupd YMMWORD[256+rsp],ymm11
        vmovupd YMMWORD[288+rsp],ymm12



        vpsrlq  ymm3,ymm3,52
        vpsrlq  ymm4,ymm4,52
        vpsrlq  ymm5,ymm5,52
        vpsrlq  ymm6,ymm6,52
        vpsrlq  ymm7,ymm7,52
        vpsrlq  ymm8,ymm8,52
        vpsrlq  ymm9,ymm9,52
        vpsrlq  ymm10,ymm10,52
        vpsrlq  ymm11,ymm11,52
        vpsrlq  ymm12,ymm12,52


        vpermq  ymm12,ymm12,144
        vpermq  ymm13,ymm11,3
        vblendpd        ymm12,ymm12,ymm13,1

        vpermq  ymm11,ymm11,144
        vpermq  ymm13,ymm10,3
        vblendpd        ymm11,ymm11,ymm13,1

        vpermq  ymm10,ymm10,144
        vpermq  ymm13,ymm9,3
        vblendpd        ymm10,ymm10,ymm13,1

        vpermq  ymm9,ymm9,144
        vpermq  ymm13,ymm8,3
        vblendpd        ymm9,ymm9,ymm13,1

        vpermq  ymm8,ymm8,144
        vpermq  ymm13,ymm7,3
        vblendpd        ymm8,ymm8,ymm13,1

        vpermq  ymm7,ymm7,144
        vpermq  ymm13,ymm6,3
        vblendpd        ymm7,ymm7,ymm13,1

        vpermq  ymm6,ymm6,144
        vpermq  ymm13,ymm5,3
        vblendpd        ymm6,ymm6,ymm13,1

        vpermq  ymm5,ymm5,144
        vpermq  ymm13,ymm4,3
        vblendpd        ymm5,ymm5,ymm13,1

        vpermq  ymm4,ymm4,144
        vpermq  ymm13,ymm3,3
        vblendpd        ymm4,ymm4,ymm13,1

        vpermq  ymm3,ymm3,144
        vpand   ymm3,ymm3,YMMWORD[$L$high64x3]

        vmovupd YMMWORD[320+rsp],ymm3
        vmovupd YMMWORD[352+rsp],ymm4
        vmovupd YMMWORD[384+rsp],ymm5
        vmovupd YMMWORD[416+rsp],ymm6
        vmovupd YMMWORD[448+rsp],ymm7
        vmovupd YMMWORD[480+rsp],ymm8
        vmovupd YMMWORD[512+rsp],ymm9
        vmovupd YMMWORD[544+rsp],ymm10
        vmovupd YMMWORD[576+rsp],ymm11
        vmovupd YMMWORD[608+rsp],ymm12

        vmovupd ymm3,YMMWORD[rsp]
        vmovupd ymm4,YMMWORD[32+rsp]
        vmovupd ymm5,YMMWORD[64+rsp]
        vmovupd ymm6,YMMWORD[96+rsp]
        vmovupd ymm7,YMMWORD[128+rsp]
        vmovupd ymm8,YMMWORD[160+rsp]
        vmovupd ymm9,YMMWORD[192+rsp]
        vmovupd ymm10,YMMWORD[224+rsp]
        vmovupd ymm11,YMMWORD[256+rsp]
        vmovupd ymm12,YMMWORD[288+rsp]


        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]
        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]
        vpand   ymm11,ymm11,YMMWORD[$L$mask52x4]
        vpand   ymm12,ymm12,YMMWORD[$L$mask52x4]


        vpaddq  ymm3,ymm3,YMMWORD[320+rsp]
        vpaddq  ymm4,ymm4,YMMWORD[352+rsp]
        vpaddq  ymm5,ymm5,YMMWORD[384+rsp]
        vpaddq  ymm6,ymm6,YMMWORD[416+rsp]
        vpaddq  ymm7,ymm7,YMMWORD[448+rsp]
        vpaddq  ymm8,ymm8,YMMWORD[480+rsp]
        vpaddq  ymm9,ymm9,YMMWORD[512+rsp]
        vpaddq  ymm10,ymm10,YMMWORD[544+rsp]
        vpaddq  ymm11,ymm11,YMMWORD[576+rsp]
        vpaddq  ymm12,ymm12,YMMWORD[608+rsp]

        lea     rsp,[640+rsp]



        vpcmpgtq        ymm13,ymm3,YMMWORD[$L$mask52x4]
        vmovmskpd       r14d,ymm13
        vpcmpgtq        ymm13,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm13
        shl     r13b,4
        or      r14b,r13b

        vpcmpgtq        ymm13,ymm5,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm13
        vpcmpgtq        ymm13,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm13
        shl     r12b,4
        or      r13b,r12b

        vpcmpgtq        ymm13,ymm7,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm13
        vpcmpgtq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm13
        shl     r11b,4
        or      r12b,r11b

        vpcmpgtq        ymm13,ymm9,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm13
        vpcmpgtq        ymm13,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       r10d,ymm13
        shl     r10b,4
        or      r11b,r10b

        vpcmpgtq        ymm13,ymm11,YMMWORD[$L$mask52x4]
        vmovmskpd       r10d,ymm13
        vpcmpgtq        ymm13,ymm12,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm13
        shl     r9b,4
        or      r10b,r9b

        add     r14b,r14b
        adc     r13b,r13b
        adc     r12b,r12b
        adc     r11b,r11b
        adc     r10b,r10b


        vpcmpeqq        ymm13,ymm3,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm13
        vpcmpeqq        ymm13,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm13
        shl     r8b,4
        or      r9b,r8b

        vpcmpeqq        ymm13,ymm5,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm13
        vpcmpeqq        ymm13,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm13
        shl     dl,4
        or      r8b,dl

        vpcmpeqq        ymm13,ymm7,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm13
        vpcmpeqq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm13
        shl     cl,4
        or      dl,cl

        vpcmpeqq        ymm13,ymm9,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm13
        vpcmpeqq        ymm13,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       ebx,ymm13
        shl     bl,4
        or      cl,bl

        vpcmpeqq        ymm13,ymm11,YMMWORD[$L$mask52x4]
        vmovmskpd       ebx,ymm13
        vpcmpeqq        ymm13,ymm12,YMMWORD[$L$mask52x4]
        vmovmskpd       eax,ymm13
        shl     al,4
        or      bl,al

        add     r14b,r9b
        adc     r13b,r8b
        adc     r12b,dl
        adc     r11b,cl
        adc     r10b,bl

        xor     r14b,r9b
        xor     r13b,r8b
        xor     r12b,dl
        xor     r11b,cl
        xor     r10b,bl

        push    r9
        push    r8

        lea     r8,[$L$kmasklut]

        mov     r9b,r14b
        and     r14,0xf
        vpsubq  ymm13,ymm3,YMMWORD[$L$mask52x4]
        shl     r14,5
        vmovapd ymm14,YMMWORD[r14*1+r8]
        vblendvpd       ymm3,ymm3,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm4,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm4,ymm4,ymm13,ymm14

        mov     r9b,r13b
        and     r13,0xf
        vpsubq  ymm13,ymm5,YMMWORD[$L$mask52x4]
        shl     r13,5
        vmovapd ymm14,YMMWORD[r13*1+r8]
        vblendvpd       ymm5,ymm5,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm6,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm6,ymm6,ymm13,ymm14

        mov     r9b,r12b
        and     r12,0xf
        vpsubq  ymm13,ymm7,YMMWORD[$L$mask52x4]
        shl     r12,5
        vmovapd ymm14,YMMWORD[r12*1+r8]
        vblendvpd       ymm7,ymm7,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm8,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm8,ymm8,ymm13,ymm14

        mov     r9b,r11b
        and     r11,0xf
        vpsubq  ymm13,ymm9,YMMWORD[$L$mask52x4]
        shl     r11,5
        vmovapd ymm14,YMMWORD[r11*1+r8]
        vblendvpd       ymm9,ymm9,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm10,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm10,ymm10,ymm13,ymm14

        mov     r9b,r10b
        and     r10,0xf
        vpsubq  ymm13,ymm11,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm14,YMMWORD[r10*1+r8]
        vblendvpd       ymm11,ymm11,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm12,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm12,ymm12,ymm13,ymm14

        pop     r8
        pop     r9

        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]

        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]
        vpand   ymm11,ymm11,YMMWORD[$L$mask52x4]
        vpand   ymm12,ymm12,YMMWORD[$L$mask52x4]

        vmovdqu YMMWORD[rdi],ymm3
        vmovdqu YMMWORD[32+rdi],ymm4
        vmovdqu YMMWORD[64+rdi],ymm5
        vmovdqu YMMWORD[96+rdi],ymm6
        vmovdqu YMMWORD[128+rdi],ymm7
        vmovdqu YMMWORD[160+rdi],ymm8
        vmovdqu YMMWORD[192+rdi],ymm9
        vmovdqu YMMWORD[224+rdi],ymm10
        vmovdqu YMMWORD[256+rdi],ymm11
        vmovdqu YMMWORD[288+rdi],ymm12

        vzeroupper
        lea     rax,[rsp]

        vmovapd xmm6,XMMWORD[rax]
        vmovapd xmm7,XMMWORD[16+rax]
        vmovapd xmm8,XMMWORD[32+rax]
        vmovapd xmm9,XMMWORD[48+rax]
        vmovapd xmm10,XMMWORD[64+rax]
        vmovapd xmm11,XMMWORD[80+rax]
        vmovapd xmm12,XMMWORD[96+rax]
        vmovapd xmm13,XMMWORD[112+rax]
        vmovapd xmm14,XMMWORD[128+rax]
        vmovapd xmm15,XMMWORD[144+rax]
        lea     rax,[168+rsp]
        mov     r15,QWORD[rax]

        mov     r14,QWORD[8+rax]

        mov     r13,QWORD[16+rax]

        mov     r12,QWORD[24+rax]

        mov     rbp,QWORD[32+rax]

        mov     rbx,QWORD[40+rax]

        lea     rsp,[48+rax]

$L$ossl_rsaz_amm52x40_x1_avxifma256_epilogue:

        mov     rdi,QWORD[8+rsp]        ;WIN64 epilogue
        mov     rsi,QWORD[16+rsp]
        DB      0F3h,0C3h               ;repret

$L$SEH_end_ossl_rsaz_amm52x40_x1_avxifma256:
section .rdata rdata align=32
ALIGN   32
$L$mask52x4:
        DQ      0xfffffffffffff
        DQ      0xfffffffffffff
        DQ      0xfffffffffffff
        DQ      0xfffffffffffff
$L$high64x3:
        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
$L$kmasklut:

        DQ      0x0
        DQ      0x0
        DQ      0x0
        DQ      0x0

        DQ      0xffffffffffffffff
        DQ      0x0
        DQ      0x0
        DQ      0x0

        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0x0
        DQ      0x0

        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0x0
        DQ      0x0

        DQ      0x0
        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0x0

        DQ      0xffffffffffffffff
        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0x0

        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0x0

        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0x0

        DQ      0x0
        DQ      0x0
        DQ      0x0
        DQ      0xffffffffffffffff

        DQ      0xffffffffffffffff
        DQ      0x0
        DQ      0x0
        DQ      0xffffffffffffffff

        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0x0
        DQ      0xffffffffffffffff

        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0x0
        DQ      0xffffffffffffffff

        DQ      0x0
        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff

        DQ      0xffffffffffffffff
        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff

        DQ      0x0
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff

        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
        DQ      0xffffffffffffffff
section .text code align=64


global  ossl_rsaz_amm52x40_x2_avxifma256

ALIGN   32
ossl_rsaz_amm52x40_x2_avxifma256:
        mov     QWORD[8+rsp],rdi        ;WIN64 prologue
        mov     QWORD[16+rsp],rsi
        mov     rax,rsp
$L$SEH_begin_ossl_rsaz_amm52x40_x2_avxifma256:
        mov     rdi,rcx
        mov     rsi,rdx
        mov     rdx,r8
        mov     rcx,r9
        mov     r8,QWORD[40+rsp]



DB      243,15,30,250
        push    rbx

        push    rbp

        push    r12

        push    r13

        push    r14

        push    r15

        lea     rsp,[((-168))+rsp]
        vmovapd XMMWORD[rsp],xmm6
        vmovapd XMMWORD[16+rsp],xmm7
        vmovapd XMMWORD[32+rsp],xmm8
        vmovapd XMMWORD[48+rsp],xmm9
        vmovapd XMMWORD[64+rsp],xmm10
        vmovapd XMMWORD[80+rsp],xmm11
        vmovapd XMMWORD[96+rsp],xmm12
        vmovapd XMMWORD[112+rsp],xmm13
        vmovapd XMMWORD[128+rsp],xmm14
        vmovapd XMMWORD[144+rsp],xmm15
$L$ossl_rsaz_amm52x40_x2_avxifma256_body:

        vpxor   ymm0,ymm0,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0
        vmovapd ymm8,ymm0
        vmovapd ymm9,ymm0
        vmovapd ymm10,ymm0
        vmovapd ymm11,ymm0
        vmovapd ymm12,ymm0

        xor     r9d,r9d

        mov     r11,rdx
        mov     rax,0xfffffffffffff

        mov     ebx,40

ALIGN   32
$L$loop40:
        mov     r13,QWORD[r11]

        vpbroadcastq    ymm1,QWORD[r11]
        mov     rdx,QWORD[rsi]
        mulx    r12,r13,r13
        add     r9,r13
        mov     r10,r12
        adc     r10,0

        mov     r13,QWORD[r8]
        imul    r13,r9
        and     r13,rax

        vmovq   xmm2,r13
        vpbroadcastq    ymm2,xmm2
        mov     rdx,QWORD[rcx]
        mulx    r12,r13,r13
        add     r9,r13
        adc     r10,r12

        shr     r9,52
        sal     r10,12
        or      r9,r10

        lea     rsp,[((-328))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52luq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52luq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52luq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52luq     ymm12,ymm2,YMMWORD[288+rcx]
        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        vmovdqu YMMWORD[256+rsp],ymm11
        vmovdqu YMMWORD[288+rsp],ymm12
        mov     QWORD[320+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]
        vmovdqu ymm11,YMMWORD[264+rsp]
        vmovdqu ymm12,YMMWORD[296+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]
{vex}   vpmadd52huq     ymm11,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52huq     ymm12,ymm1,YMMWORD[288+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]
{vex}   vpmadd52huq     ymm11,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52huq     ymm12,ymm2,YMMWORD[288+rcx]
        lea     rsp,[328+rsp]
        lea     r11,[8+r11]
        dec     ebx
        jne     NEAR $L$loop40

        push    r11
        push    rsi
        push    rcx
        push    r8

        vmovq   xmm0,r9
        vpbroadcastq    ymm0,xmm0
        vpblendd        ymm3,ymm3,ymm0,3

        lea     rsp,[((-640))+rsp]
        vmovupd YMMWORD[rsp],ymm3
        vmovupd YMMWORD[32+rsp],ymm4
        vmovupd YMMWORD[64+rsp],ymm5
        vmovupd YMMWORD[96+rsp],ymm6
        vmovupd YMMWORD[128+rsp],ymm7
        vmovupd YMMWORD[160+rsp],ymm8
        vmovupd YMMWORD[192+rsp],ymm9
        vmovupd YMMWORD[224+rsp],ymm10
        vmovupd YMMWORD[256+rsp],ymm11
        vmovupd YMMWORD[288+rsp],ymm12



        vpsrlq  ymm3,ymm3,52
        vpsrlq  ymm4,ymm4,52
        vpsrlq  ymm5,ymm5,52
        vpsrlq  ymm6,ymm6,52
        vpsrlq  ymm7,ymm7,52
        vpsrlq  ymm8,ymm8,52
        vpsrlq  ymm9,ymm9,52
        vpsrlq  ymm10,ymm10,52
        vpsrlq  ymm11,ymm11,52
        vpsrlq  ymm12,ymm12,52


        vpermq  ymm12,ymm12,144
        vpermq  ymm13,ymm11,3
        vblendpd        ymm12,ymm12,ymm13,1

        vpermq  ymm11,ymm11,144
        vpermq  ymm13,ymm10,3
        vblendpd        ymm11,ymm11,ymm13,1

        vpermq  ymm10,ymm10,144
        vpermq  ymm13,ymm9,3
        vblendpd        ymm10,ymm10,ymm13,1

        vpermq  ymm9,ymm9,144
        vpermq  ymm13,ymm8,3
        vblendpd        ymm9,ymm9,ymm13,1

        vpermq  ymm8,ymm8,144
        vpermq  ymm13,ymm7,3
        vblendpd        ymm8,ymm8,ymm13,1

        vpermq  ymm7,ymm7,144
        vpermq  ymm13,ymm6,3
        vblendpd        ymm7,ymm7,ymm13,1

        vpermq  ymm6,ymm6,144
        vpermq  ymm13,ymm5,3
        vblendpd        ymm6,ymm6,ymm13,1

        vpermq  ymm5,ymm5,144
        vpermq  ymm13,ymm4,3
        vblendpd        ymm5,ymm5,ymm13,1

        vpermq  ymm4,ymm4,144
        vpermq  ymm13,ymm3,3
        vblendpd        ymm4,ymm4,ymm13,1

        vpermq  ymm3,ymm3,144
        vpand   ymm3,ymm3,YMMWORD[$L$high64x3]

        vmovupd YMMWORD[320+rsp],ymm3
        vmovupd YMMWORD[352+rsp],ymm4
        vmovupd YMMWORD[384+rsp],ymm5
        vmovupd YMMWORD[416+rsp],ymm6
        vmovupd YMMWORD[448+rsp],ymm7
        vmovupd YMMWORD[480+rsp],ymm8
        vmovupd YMMWORD[512+rsp],ymm9
        vmovupd YMMWORD[544+rsp],ymm10
        vmovupd YMMWORD[576+rsp],ymm11
        vmovupd YMMWORD[608+rsp],ymm12

        vmovupd ymm3,YMMWORD[rsp]
        vmovupd ymm4,YMMWORD[32+rsp]
        vmovupd ymm5,YMMWORD[64+rsp]
        vmovupd ymm6,YMMWORD[96+rsp]
        vmovupd ymm7,YMMWORD[128+rsp]
        vmovupd ymm8,YMMWORD[160+rsp]
        vmovupd ymm9,YMMWORD[192+rsp]
        vmovupd ymm10,YMMWORD[224+rsp]
        vmovupd ymm11,YMMWORD[256+rsp]
        vmovupd ymm12,YMMWORD[288+rsp]


        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]
        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]
        vpand   ymm11,ymm11,YMMWORD[$L$mask52x4]
        vpand   ymm12,ymm12,YMMWORD[$L$mask52x4]


        vpaddq  ymm3,ymm3,YMMWORD[320+rsp]
        vpaddq  ymm4,ymm4,YMMWORD[352+rsp]
        vpaddq  ymm5,ymm5,YMMWORD[384+rsp]
        vpaddq  ymm6,ymm6,YMMWORD[416+rsp]
        vpaddq  ymm7,ymm7,YMMWORD[448+rsp]
        vpaddq  ymm8,ymm8,YMMWORD[480+rsp]
        vpaddq  ymm9,ymm9,YMMWORD[512+rsp]
        vpaddq  ymm10,ymm10,YMMWORD[544+rsp]
        vpaddq  ymm11,ymm11,YMMWORD[576+rsp]
        vpaddq  ymm12,ymm12,YMMWORD[608+rsp]

        lea     rsp,[640+rsp]



        vpcmpgtq        ymm13,ymm3,YMMWORD[$L$mask52x4]
        vmovmskpd       r14d,ymm13
        vpcmpgtq        ymm13,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm13
        shl     r13b,4
        or      r14b,r13b

        vpcmpgtq        ymm13,ymm5,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm13
        vpcmpgtq        ymm13,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm13
        shl     r12b,4
        or      r13b,r12b

        vpcmpgtq        ymm13,ymm7,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm13
        vpcmpgtq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm13
        shl     r11b,4
        or      r12b,r11b

        vpcmpgtq        ymm13,ymm9,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm13
        vpcmpgtq        ymm13,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       r10d,ymm13
        shl     r10b,4
        or      r11b,r10b

        vpcmpgtq        ymm13,ymm11,YMMWORD[$L$mask52x4]
        vmovmskpd       r10d,ymm13
        vpcmpgtq        ymm13,ymm12,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm13
        shl     r9b,4
        or      r10b,r9b

        add     r14b,r14b
        adc     r13b,r13b
        adc     r12b,r12b
        adc     r11b,r11b
        adc     r10b,r10b


        vpcmpeqq        ymm13,ymm3,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm13
        vpcmpeqq        ymm13,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm13
        shl     r8b,4
        or      r9b,r8b

        vpcmpeqq        ymm13,ymm5,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm13
        vpcmpeqq        ymm13,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm13
        shl     dl,4
        or      r8b,dl

        vpcmpeqq        ymm13,ymm7,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm13
        vpcmpeqq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm13
        shl     cl,4
        or      dl,cl

        vpcmpeqq        ymm13,ymm9,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm13
        vpcmpeqq        ymm13,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       ebx,ymm13
        shl     bl,4
        or      cl,bl

        vpcmpeqq        ymm13,ymm11,YMMWORD[$L$mask52x4]
        vmovmskpd       ebx,ymm13
        vpcmpeqq        ymm13,ymm12,YMMWORD[$L$mask52x4]
        vmovmskpd       eax,ymm13
        shl     al,4
        or      bl,al

        add     r14b,r9b
        adc     r13b,r8b
        adc     r12b,dl
        adc     r11b,cl
        adc     r10b,bl

        xor     r14b,r9b
        xor     r13b,r8b
        xor     r12b,dl
        xor     r11b,cl
        xor     r10b,bl

        push    r9
        push    r8

        lea     r8,[$L$kmasklut]

        mov     r9b,r14b
        and     r14,0xf
        vpsubq  ymm13,ymm3,YMMWORD[$L$mask52x4]
        shl     r14,5
        vmovapd ymm14,YMMWORD[r14*1+r8]
        vblendvpd       ymm3,ymm3,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm4,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm4,ymm4,ymm13,ymm14

        mov     r9b,r13b
        and     r13,0xf
        vpsubq  ymm13,ymm5,YMMWORD[$L$mask52x4]
        shl     r13,5
        vmovapd ymm14,YMMWORD[r13*1+r8]
        vblendvpd       ymm5,ymm5,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm6,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm6,ymm6,ymm13,ymm14

        mov     r9b,r12b
        and     r12,0xf
        vpsubq  ymm13,ymm7,YMMWORD[$L$mask52x4]
        shl     r12,5
        vmovapd ymm14,YMMWORD[r12*1+r8]
        vblendvpd       ymm7,ymm7,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm8,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm8,ymm8,ymm13,ymm14

        mov     r9b,r11b
        and     r11,0xf
        vpsubq  ymm13,ymm9,YMMWORD[$L$mask52x4]
        shl     r11,5
        vmovapd ymm14,YMMWORD[r11*1+r8]
        vblendvpd       ymm9,ymm9,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm10,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm10,ymm10,ymm13,ymm14

        mov     r9b,r10b
        and     r10,0xf
        vpsubq  ymm13,ymm11,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm14,YMMWORD[r10*1+r8]
        vblendvpd       ymm11,ymm11,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm12,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm12,ymm12,ymm13,ymm14

        pop     r8
        pop     r9

        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]

        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]
        vpand   ymm11,ymm11,YMMWORD[$L$mask52x4]
        vpand   ymm12,ymm12,YMMWORD[$L$mask52x4]

        pop     r8
        pop     rcx
        pop     rsi
        pop     r11

        vmovdqu YMMWORD[rdi],ymm3
        vmovdqu YMMWORD[32+rdi],ymm4
        vmovdqu YMMWORD[64+rdi],ymm5
        vmovdqu YMMWORD[96+rdi],ymm6
        vmovdqu YMMWORD[128+rdi],ymm7
        vmovdqu YMMWORD[160+rdi],ymm8
        vmovdqu YMMWORD[192+rdi],ymm9
        vmovdqu YMMWORD[224+rdi],ymm10
        vmovdqu YMMWORD[256+rdi],ymm11
        vmovdqu YMMWORD[288+rdi],ymm12

        xor     r15d,r15d

        mov     rax,0xfffffffffffff

        mov     ebx,40

        vpxor   ymm0,ymm0,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0
        vmovapd ymm8,ymm0
        vmovapd ymm9,ymm0
        vmovapd ymm10,ymm0
        vmovapd ymm11,ymm0
        vmovapd ymm12,ymm0
ALIGN   32
$L$loop40_1:
        mov     r13,QWORD[r11]

        vpbroadcastq    ymm1,QWORD[r11]
        mov     rdx,QWORD[320+rsi]
        mulx    r12,r13,r13
        add     r9,r13
        mov     r10,r12
        adc     r10,0

        mov     r13,QWORD[8+r8]
        imul    r13,r9
        and     r13,rax

        vmovq   xmm2,r13
        vpbroadcastq    ymm2,xmm2
        mov     rdx,QWORD[320+rcx]
        mulx    r12,r13,r13
        add     r9,r13
        adc     r10,r12

        shr     r9,52
        sal     r10,12
        or      r9,r10

        lea     rsp,[((-328))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[320+rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[352+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[384+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[416+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[448+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[480+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[512+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[544+rsi]
{vex}   vpmadd52luq     ymm11,ymm1,YMMWORD[576+rsi]
{vex}   vpmadd52luq     ymm12,ymm1,YMMWORD[608+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[320+rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[352+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[384+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[416+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[448+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[480+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[512+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[544+rcx]
{vex}   vpmadd52luq     ymm11,ymm2,YMMWORD[576+rcx]
{vex}   vpmadd52luq     ymm12,ymm2,YMMWORD[608+rcx]
        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        vmovdqu YMMWORD[256+rsp],ymm11
        vmovdqu YMMWORD[288+rsp],ymm12
        mov     QWORD[320+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]
        vmovdqu ymm11,YMMWORD[264+rsp]
        vmovdqu ymm12,YMMWORD[296+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[320+rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[352+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[384+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[416+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[448+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[480+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[512+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[544+rsi]
{vex}   vpmadd52huq     ymm11,ymm1,YMMWORD[576+rsi]
{vex}   vpmadd52huq     ymm12,ymm1,YMMWORD[608+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[320+rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[352+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[384+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[416+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[448+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[480+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[512+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[544+rcx]
{vex}   vpmadd52huq     ymm11,ymm2,YMMWORD[576+rcx]
{vex}   vpmadd52huq     ymm12,ymm2,YMMWORD[608+rcx]
        lea     rsp,[328+rsp]
        lea     r11,[8+r11]
        dec     ebx
        jne     NEAR $L$loop40_1

        vmovq   xmm0,r9
        vpbroadcastq    ymm0,xmm0
        vpblendd        ymm3,ymm3,ymm0,3

        lea     rsp,[((-640))+rsp]
        vmovupd YMMWORD[rsp],ymm3
        vmovupd YMMWORD[32+rsp],ymm4
        vmovupd YMMWORD[64+rsp],ymm5
        vmovupd YMMWORD[96+rsp],ymm6
        vmovupd YMMWORD[128+rsp],ymm7
        vmovupd YMMWORD[160+rsp],ymm8
        vmovupd YMMWORD[192+rsp],ymm9
        vmovupd YMMWORD[224+rsp],ymm10
        vmovupd YMMWORD[256+rsp],ymm11
        vmovupd YMMWORD[288+rsp],ymm12



        vpsrlq  ymm3,ymm3,52
        vpsrlq  ymm4,ymm4,52
        vpsrlq  ymm5,ymm5,52
        vpsrlq  ymm6,ymm6,52
        vpsrlq  ymm7,ymm7,52
        vpsrlq  ymm8,ymm8,52
        vpsrlq  ymm9,ymm9,52
        vpsrlq  ymm10,ymm10,52
        vpsrlq  ymm11,ymm11,52
        vpsrlq  ymm12,ymm12,52


        vpermq  ymm12,ymm12,144
        vpermq  ymm13,ymm11,3
        vblendpd        ymm12,ymm12,ymm13,1

        vpermq  ymm11,ymm11,144
        vpermq  ymm13,ymm10,3
        vblendpd        ymm11,ymm11,ymm13,1

        vpermq  ymm10,ymm10,144
        vpermq  ymm13,ymm9,3
        vblendpd        ymm10,ymm10,ymm13,1

        vpermq  ymm9,ymm9,144
        vpermq  ymm13,ymm8,3
        vblendpd        ymm9,ymm9,ymm13,1

        vpermq  ymm8,ymm8,144
        vpermq  ymm13,ymm7,3
        vblendpd        ymm8,ymm8,ymm13,1

        vpermq  ymm7,ymm7,144
        vpermq  ymm13,ymm6,3
        vblendpd        ymm7,ymm7,ymm13,1

        vpermq  ymm6,ymm6,144
        vpermq  ymm13,ymm5,3
        vblendpd        ymm6,ymm6,ymm13,1

        vpermq  ymm5,ymm5,144
        vpermq  ymm13,ymm4,3
        vblendpd        ymm5,ymm5,ymm13,1

        vpermq  ymm4,ymm4,144
        vpermq  ymm13,ymm3,3
        vblendpd        ymm4,ymm4,ymm13,1

        vpermq  ymm3,ymm3,144
        vpand   ymm3,ymm3,YMMWORD[$L$high64x3]

        vmovupd YMMWORD[320+rsp],ymm3
        vmovupd YMMWORD[352+rsp],ymm4
        vmovupd YMMWORD[384+rsp],ymm5
        vmovupd YMMWORD[416+rsp],ymm6
        vmovupd YMMWORD[448+rsp],ymm7
        vmovupd YMMWORD[480+rsp],ymm8
        vmovupd YMMWORD[512+rsp],ymm9
        vmovupd YMMWORD[544+rsp],ymm10
        vmovupd YMMWORD[576+rsp],ymm11
        vmovupd YMMWORD[608+rsp],ymm12

        vmovupd ymm3,YMMWORD[rsp]
        vmovupd ymm4,YMMWORD[32+rsp]
        vmovupd ymm5,YMMWORD[64+rsp]
        vmovupd ymm6,YMMWORD[96+rsp]
        vmovupd ymm7,YMMWORD[128+rsp]
        vmovupd ymm8,YMMWORD[160+rsp]
        vmovupd ymm9,YMMWORD[192+rsp]
        vmovupd ymm10,YMMWORD[224+rsp]
        vmovupd ymm11,YMMWORD[256+rsp]
        vmovupd ymm12,YMMWORD[288+rsp]


        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]
        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]
        vpand   ymm11,ymm11,YMMWORD[$L$mask52x4]
        vpand   ymm12,ymm12,YMMWORD[$L$mask52x4]


        vpaddq  ymm3,ymm3,YMMWORD[320+rsp]
        vpaddq  ymm4,ymm4,YMMWORD[352+rsp]
        vpaddq  ymm5,ymm5,YMMWORD[384+rsp]
        vpaddq  ymm6,ymm6,YMMWORD[416+rsp]
        vpaddq  ymm7,ymm7,YMMWORD[448+rsp]
        vpaddq  ymm8,ymm8,YMMWORD[480+rsp]
        vpaddq  ymm9,ymm9,YMMWORD[512+rsp]
        vpaddq  ymm10,ymm10,YMMWORD[544+rsp]
        vpaddq  ymm11,ymm11,YMMWORD[576+rsp]
        vpaddq  ymm12,ymm12,YMMWORD[608+rsp]

        lea     rsp,[640+rsp]



        vpcmpgtq        ymm13,ymm3,YMMWORD[$L$mask52x4]
        vmovmskpd       r14d,ymm13
        vpcmpgtq        ymm13,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm13
        shl     r13b,4
        or      r14b,r13b

        vpcmpgtq        ymm13,ymm5,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm13
        vpcmpgtq        ymm13,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm13
        shl     r12b,4
        or      r13b,r12b

        vpcmpgtq        ymm13,ymm7,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm13
        vpcmpgtq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm13
        shl     r11b,4
        or      r12b,r11b

        vpcmpgtq        ymm13,ymm9,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm13
        vpcmpgtq        ymm13,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       r10d,ymm13
        shl     r10b,4
        or      r11b,r10b

        vpcmpgtq        ymm13,ymm11,YMMWORD[$L$mask52x4]
        vmovmskpd       r10d,ymm13
        vpcmpgtq        ymm13,ymm12,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm13
        shl     r9b,4
        or      r10b,r9b

        add     r14b,r14b
        adc     r13b,r13b
        adc     r12b,r12b
        adc     r11b,r11b
        adc     r10b,r10b


        vpcmpeqq        ymm13,ymm3,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm13
        vpcmpeqq        ymm13,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm13
        shl     r8b,4
        or      r9b,r8b

        vpcmpeqq        ymm13,ymm5,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm13
        vpcmpeqq        ymm13,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm13
        shl     dl,4
        or      r8b,dl

        vpcmpeqq        ymm13,ymm7,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm13
        vpcmpeqq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm13
        shl     cl,4
        or      dl,cl

        vpcmpeqq        ymm13,ymm9,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm13
        vpcmpeqq        ymm13,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       ebx,ymm13
        shl     bl,4
        or      cl,bl

        vpcmpeqq        ymm13,ymm11,YMMWORD[$L$mask52x4]
        vmovmskpd       ebx,ymm13
        vpcmpeqq        ymm13,ymm12,YMMWORD[$L$mask52x4]
        vmovmskpd       eax,ymm13
        shl     al,4
        or      bl,al

        add     r14b,r9b
        adc     r13b,r8b
        adc     r12b,dl
        adc     r11b,cl
        adc     r10b,bl

        xor     r14b,r9b
        xor     r13b,r8b
        xor     r12b,dl
        xor     r11b,cl
        xor     r10b,bl

        push    r9
        push    r8

        lea     r8,[$L$kmasklut]

        mov     r9b,r14b
        and     r14,0xf
        vpsubq  ymm13,ymm3,YMMWORD[$L$mask52x4]
        shl     r14,5
        vmovapd ymm14,YMMWORD[r14*1+r8]
        vblendvpd       ymm3,ymm3,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm4,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm4,ymm4,ymm13,ymm14

        mov     r9b,r13b
        and     r13,0xf
        vpsubq  ymm13,ymm5,YMMWORD[$L$mask52x4]
        shl     r13,5
        vmovapd ymm14,YMMWORD[r13*1+r8]
        vblendvpd       ymm5,ymm5,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm6,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm6,ymm6,ymm13,ymm14

        mov     r9b,r12b
        and     r12,0xf
        vpsubq  ymm13,ymm7,YMMWORD[$L$mask52x4]
        shl     r12,5
        vmovapd ymm14,YMMWORD[r12*1+r8]
        vblendvpd       ymm7,ymm7,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm8,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm8,ymm8,ymm13,ymm14

        mov     r9b,r11b
        and     r11,0xf
        vpsubq  ymm13,ymm9,YMMWORD[$L$mask52x4]
        shl     r11,5
        vmovapd ymm14,YMMWORD[r11*1+r8]
        vblendvpd       ymm9,ymm9,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm10,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm10,ymm10,ymm13,ymm14

        mov     r9b,r10b
        and     r10,0xf
        vpsubq  ymm13,ymm11,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm14,YMMWORD[r10*1+r8]
        vblendvpd       ymm11,ymm11,ymm13,ymm14

        shr     r9b,4
        and     r9,0xf
        vpsubq  ymm13,ymm12,YMMWORD[$L$mask52x4]
        shl     r9,5
        vmovapd ymm14,YMMWORD[r9*1+r8]
        vblendvpd       ymm12,ymm12,ymm13,ymm14

        pop     r8
        pop     r9

        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]

        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]
        vpand   ymm11,ymm11,YMMWORD[$L$mask52x4]
        vpand   ymm12,ymm12,YMMWORD[$L$mask52x4]

        vmovdqu YMMWORD[320+rdi],ymm3
        vmovdqu YMMWORD[352+rdi],ymm4
        vmovdqu YMMWORD[384+rdi],ymm5
        vmovdqu YMMWORD[416+rdi],ymm6
        vmovdqu YMMWORD[448+rdi],ymm7
        vmovdqu YMMWORD[480+rdi],ymm8
        vmovdqu YMMWORD[512+rdi],ymm9
        vmovdqu YMMWORD[544+rdi],ymm10
        vmovdqu YMMWORD[576+rdi],ymm11
        vmovdqu YMMWORD[608+rdi],ymm12

        vzeroupper
        lea     rax,[rsp]

        vmovapd xmm6,XMMWORD[rax]
        vmovapd xmm7,XMMWORD[16+rax]
        vmovapd xmm8,XMMWORD[32+rax]
        vmovapd xmm9,XMMWORD[48+rax]
        vmovapd xmm10,XMMWORD[64+rax]
        vmovapd xmm11,XMMWORD[80+rax]
        vmovapd xmm12,XMMWORD[96+rax]
        vmovapd xmm13,XMMWORD[112+rax]
        vmovapd xmm14,XMMWORD[128+rax]
        vmovapd xmm15,XMMWORD[144+rax]
        lea     rax,[168+rsp]
        mov     r15,QWORD[rax]

        mov     r14,QWORD[8+rax]

        mov     r13,QWORD[16+rax]

        mov     r12,QWORD[24+rax]

        mov     rbp,QWORD[32+rax]

        mov     rbx,QWORD[40+rax]

        lea     rsp,[48+rax]

$L$ossl_rsaz_amm52x40_x2_avxifma256_epilogue:
        mov     rdi,QWORD[8+rsp]        ;WIN64 epilogue
        mov     rsi,QWORD[16+rsp]
        DB      0F3h,0C3h               ;repret

$L$SEH_end_ossl_rsaz_amm52x40_x2_avxifma256:
section .text code align=64


ALIGN   32
global  ossl_extract_multiplier_2x40_win5_avx

ossl_extract_multiplier_2x40_win5_avx:

DB      243,15,30,250
        vmovapd ymm14,YMMWORD[$L$ones]
        vmovq   xmm10,r8
        vpbroadcastq    ymm12,xmm10
        vmovq   xmm10,r9
        vpbroadcastq    ymm13,xmm10
        lea     rax,[20480+rdx]


        mov     r10,rdx


        vpxor   xmm0,xmm0,xmm0
        vmovapd ymm1,ymm0
        vmovapd ymm2,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0
        vmovapd ymm8,ymm0
        vmovapd ymm9,ymm0
        vpxor   ymm11,ymm11,ymm11
ALIGN   32
$L$loop_0:
        vpcmpeqq        ymm15,ymm12,ymm11
        vmovdqu ymm10,YMMWORD[rdx]

        vblendvpd       ymm0,ymm0,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[32+rdx]

        vblendvpd       ymm1,ymm1,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[64+rdx]

        vblendvpd       ymm2,ymm2,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[96+rdx]

        vblendvpd       ymm3,ymm3,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[128+rdx]

        vblendvpd       ymm4,ymm4,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[160+rdx]

        vblendvpd       ymm5,ymm5,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[192+rdx]

        vblendvpd       ymm6,ymm6,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[224+rdx]

        vblendvpd       ymm7,ymm7,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[256+rdx]

        vblendvpd       ymm8,ymm8,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[288+rdx]

        vblendvpd       ymm9,ymm9,ymm10,ymm15
        vpaddq  ymm11,ymm11,ymm14
        add     rdx,640
        cmp     rax,rdx
        jne     NEAR $L$loop_0
        vmovdqu YMMWORD[rcx],ymm0
        vmovdqu YMMWORD[32+rcx],ymm1
        vmovdqu YMMWORD[64+rcx],ymm2
        vmovdqu YMMWORD[96+rcx],ymm3
        vmovdqu YMMWORD[128+rcx],ymm4
        vmovdqu YMMWORD[160+rcx],ymm5
        vmovdqu YMMWORD[192+rcx],ymm6
        vmovdqu YMMWORD[224+rcx],ymm7
        vmovdqu YMMWORD[256+rcx],ymm8
        vmovdqu YMMWORD[288+rcx],ymm9
        mov     rdx,r10
        vpxor   ymm11,ymm11,ymm11
ALIGN   32
$L$loop_320:
        vpcmpeqq        ymm15,ymm13,ymm11
        vmovdqu ymm10,YMMWORD[320+rdx]

        vblendvpd       ymm0,ymm0,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[352+rdx]

        vblendvpd       ymm1,ymm1,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[384+rdx]

        vblendvpd       ymm2,ymm2,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[416+rdx]

        vblendvpd       ymm3,ymm3,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[448+rdx]

        vblendvpd       ymm4,ymm4,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[480+rdx]

        vblendvpd       ymm5,ymm5,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[512+rdx]

        vblendvpd       ymm6,ymm6,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[544+rdx]

        vblendvpd       ymm7,ymm7,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[576+rdx]

        vblendvpd       ymm8,ymm8,ymm10,ymm15
        vmovdqu ymm10,YMMWORD[608+rdx]

        vblendvpd       ymm9,ymm9,ymm10,ymm15
        vpaddq  ymm11,ymm11,ymm14
        add     rdx,640
        cmp     rax,rdx
        jne     NEAR $L$loop_320
        vmovdqu YMMWORD[320+rcx],ymm0
        vmovdqu YMMWORD[352+rcx],ymm1
        vmovdqu YMMWORD[384+rcx],ymm2
        vmovdqu YMMWORD[416+rcx],ymm3
        vmovdqu YMMWORD[448+rcx],ymm4
        vmovdqu YMMWORD[480+rcx],ymm5
        vmovdqu YMMWORD[512+rcx],ymm6
        vmovdqu YMMWORD[544+rcx],ymm7
        vmovdqu YMMWORD[576+rcx],ymm8
        vmovdqu YMMWORD[608+rcx],ymm9

        DB      0F3h,0C3h               ;repret


section .rdata rdata align=32
ALIGN   32
$L$ones:
        DQ      1,1,1,1
$L$zeros:
        DQ      0,0,0,0
EXTERN  __imp_RtlVirtualUnwind

ALIGN   16
rsaz_avx_handler:
        push    rsi
        push    rdi
        push    rbx
        push    rbp
        push    r12
        push    r13
        push    r14
        push    r15
        pushfq
        sub     rsp,64

        mov     rax,QWORD[120+r8]
        mov     rbx,QWORD[248+r8]

        mov     rsi,QWORD[8+r9]
        mov     r11,QWORD[56+r9]

        mov     r10d,DWORD[r11]
        lea     r10,[r10*1+rsi]
        cmp     rbx,r10
        jb      NEAR $L$common_seh_tail

        mov     r10d,DWORD[4+r11]
        lea     r10,[r10*1+rsi]
        cmp     rbx,r10
        jae     NEAR $L$common_seh_tail

        mov     rax,QWORD[152+r8]

        lea     rsi,[rax]
        lea     rdi,[512+r8]
        mov     ecx,20
        DD      0xa548f3fc

        lea     rax,[216+rax]

        mov     rbx,QWORD[((-8))+rax]
        mov     rbp,QWORD[((-16))+rax]
        mov     r12,QWORD[((-24))+rax]
        mov     r13,QWORD[((-32))+rax]
        mov     r14,QWORD[((-40))+rax]
        mov     r15,QWORD[((-48))+rax]
        mov     QWORD[144+r8],rbx
        mov     QWORD[160+r8],rbp
        mov     QWORD[216+r8],r12
        mov     QWORD[224+r8],r13
        mov     QWORD[232+r8],r14
        mov     QWORD[240+r8],r15

$L$common_seh_tail:
        mov     rdi,QWORD[8+rax]
        mov     rsi,QWORD[16+rax]
        mov     QWORD[152+r8],rax
        mov     QWORD[168+r8],rsi
        mov     QWORD[176+r8],rdi

        mov     rdi,QWORD[40+r9]
        mov     rsi,r8
        mov     ecx,154
        DD      0xa548f3fc

        mov     rsi,r9
        xor     rcx,rcx
        mov     rdx,QWORD[8+rsi]
        mov     r8,QWORD[rsi]
        mov     r9,QWORD[16+rsi]
        mov     r10,QWORD[40+rsi]
        lea     r11,[56+rsi]
        lea     r12,[24+rsi]
        mov     QWORD[32+rsp],r10
        mov     QWORD[40+rsp],r11
        mov     QWORD[48+rsp],r12
        mov     QWORD[56+rsp],rcx
        call    QWORD[__imp_RtlVirtualUnwind]

        mov     eax,1
        add     rsp,64
        popfq
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     rbp
        pop     rbx
        pop     rdi
        pop     rsi
        DB      0F3h,0C3h               ;repret


section .pdata rdata align=4
ALIGN   4
        DD      $L$SEH_begin_ossl_rsaz_amm52x40_x1_avxifma256 wrt ..imagebase
        DD      $L$SEH_end_ossl_rsaz_amm52x40_x1_avxifma256 wrt ..imagebase
        DD      $L$SEH_info_ossl_rsaz_amm52x40_x1_avxifma256 wrt ..imagebase

        DD      $L$SEH_begin_ossl_rsaz_amm52x40_x2_avxifma256 wrt ..imagebase
        DD      $L$SEH_end_ossl_rsaz_amm52x40_x2_avxifma256 wrt ..imagebase
        DD      $L$SEH_info_ossl_rsaz_amm52x40_x2_avxifma256 wrt ..imagebase

section .xdata rdata align=8
ALIGN   8
$L$SEH_info_ossl_rsaz_amm52x40_x1_avxifma256:
DB      9,0,0,0
        DD      rsaz_avx_handler wrt ..imagebase
        DD      $L$ossl_rsaz_amm52x40_x1_avxifma256_body wrt ..imagebase,$L$ossl_rsaz_amm52x40_x1_avxifma256_epilogue wrt ..imagebase
$L$SEH_info_ossl_rsaz_amm52x40_x2_avxifma256:
DB      9,0,0,0
        DD      rsaz_avx_handler wrt ..imagebase
        DD      $L$ossl_rsaz_amm52x40_x2_avxifma256_body wrt ..imagebase,$L$ossl_rsaz_amm52x40_x2_avxifma256_epilogue wrt ..imagebase
