default rel
%define XMMWORD
%define YMMWORD
%define ZMMWORD
section .text code align=64


global  ossl_rsaz_amm52x30_x1_avxifma256

ALIGN   32
ossl_rsaz_amm52x30_x1_avxifma256:
        mov     QWORD[8+rsp],rdi        ;WIN64 prologue
        mov     QWORD[16+rsp],rsi
        mov     rax,rsp
$L$SEH_begin_ossl_rsaz_amm52x30_x1_avxifma256:
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
$L$ossl_rsaz_amm52x30_x1_avxifma256_body:

        vpxor   ymm0,ymm0,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0
        vmovapd ymm8,ymm0
        vmovapd ymm9,ymm0
        vmovapd ymm10,ymm0

        xor     r9d,r9d

        mov     r11,rdx
        mov     rax,0xfffffffffffff


        mov     ebx,7

ALIGN   32
$L$loop7:
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

        lea     rsp,[((-264))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]


        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        mov     QWORD[256+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]

        lea     rsp,[264+rsp]
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

        lea     rsp,[((-264))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]


        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        mov     QWORD[256+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]

        lea     rsp,[264+rsp]
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

        lea     rsp,[((-264))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]


        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        mov     QWORD[256+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]

        lea     rsp,[264+rsp]
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

        lea     rsp,[((-264))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]


        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        mov     QWORD[256+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]

        lea     rsp,[264+rsp]
        lea     r11,[32+r11]
        dec     ebx
        jne     NEAR $L$loop7
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

        lea     rsp,[((-264))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]


        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        mov     QWORD[256+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]

        lea     rsp,[264+rsp]
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

        lea     rsp,[((-264))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]


        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        mov     QWORD[256+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]

        lea     rsp,[264+rsp]

        vmovq   xmm0,r9
        vpbroadcastq    ymm0,xmm0
        vpblendd        ymm3,ymm3,ymm0,3



        vpsrlq  ymm0,ymm3,52
        vpsrlq  ymm1,ymm4,52
        vpsrlq  ymm2,ymm5,52
        vpsrlq  ymm11,ymm6,52
        vpsrlq  ymm12,ymm7,52
        vpsrlq  ymm13,ymm8,52
        vpsrlq  ymm14,ymm9,52
        vpsrlq  ymm15,ymm10,52

        lea     rsp,[((-32))+rsp]
        vmovupd YMMWORD[rsp],ymm3


        vpermq  ymm15,ymm15,144
        vpermq  ymm3,ymm14,3
        vblendpd        ymm15,ymm15,ymm3,1

        vpermq  ymm14,ymm14,144
        vpermq  ymm3,ymm13,3
        vblendpd        ymm14,ymm14,ymm3,1

        vpermq  ymm13,ymm13,144
        vpermq  ymm3,ymm12,3
        vblendpd        ymm13,ymm13,ymm3,1

        vpermq  ymm12,ymm12,144
        vpermq  ymm3,ymm11,3
        vblendpd        ymm12,ymm12,ymm3,1

        vpermq  ymm11,ymm11,144
        vpermq  ymm3,ymm2,3
        vblendpd        ymm11,ymm11,ymm3,1

        vpermq  ymm2,ymm2,144
        vpermq  ymm3,ymm1,3
        vblendpd        ymm2,ymm2,ymm3,1

        vpermq  ymm1,ymm1,144
        vpermq  ymm3,ymm0,3
        vblendpd        ymm1,ymm1,ymm3,1

        vpermq  ymm0,ymm0,144
        vpand   ymm0,ymm0,YMMWORD[$L$high64x3]

        vmovupd ymm3,YMMWORD[rsp]
        lea     rsp,[32+rsp]


        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]
        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]


        vpaddq  ymm3,ymm3,ymm0
        vpaddq  ymm4,ymm4,ymm1
        vpaddq  ymm5,ymm5,ymm2
        vpaddq  ymm6,ymm6,ymm11
        vpaddq  ymm7,ymm7,ymm12
        vpaddq  ymm8,ymm8,ymm13
        vpaddq  ymm9,ymm9,ymm14
        vpaddq  ymm10,ymm10,ymm15



        vpcmpgtq        ymm0,ymm3,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm1,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r14d,ymm0
        vmovmskpd       r13d,ymm1
        shl     r13b,4
        or      r14b,r13b

        vpcmpgtq        ymm2,ymm5,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm11,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm2
        vmovmskpd       r12d,ymm11
        shl     r12b,4
        or      r13b,r12b

        vpcmpgtq        ymm12,ymm7,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm12
        vmovmskpd       r11d,ymm13
        shl     r11b,4
        or      r12b,r11b

        vpcmpgtq        ymm14,ymm9,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm15,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm14
        vmovmskpd       r10d,ymm15
        shl     r10b,4
        or      r11b,r10b

        add     r14b,r14b
        adc     r13b,r13b
        adc     r12b,r12b
        adc     r11b,r11b


        vpcmpeqq        ymm0,ymm3,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm1,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm0
        vmovmskpd       r8d,ymm1
        shl     r8b,4
        or      r9b,r8b

        vpcmpeqq        ymm2,ymm5,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm11,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm2
        vmovmskpd       edx,ymm11
        shl     dl,4
        or      r8b,dl

        vpcmpeqq        ymm12,ymm7,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm12
        vmovmskpd       ecx,ymm13
        shl     cl,4
        or      dl,cl

        vpcmpeqq        ymm14,ymm9,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm15,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm14
        vmovmskpd       ebx,ymm15
        shl     bl,4
        or      cl,bl

        add     r14b,r9b
        adc     r13b,r8b
        adc     r12b,dl
        adc     r11b,cl

        xor     r14b,r9b
        xor     r13b,r8b
        xor     r12b,dl
        xor     r11b,cl

        lea     rdx,[$L$kmasklut]

        mov     r10b,r14b
        and     r14,0xf
        vpsubq  ymm0,ymm3,YMMWORD[$L$mask52x4]
        shl     r14,5
        vmovapd ymm2,YMMWORD[r14*1+rdx]
        vblendvpd       ymm3,ymm3,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm4,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm4,ymm4,ymm0,ymm2

        mov     r10b,r13b
        and     r13,0xf
        vpsubq  ymm0,ymm5,YMMWORD[$L$mask52x4]
        shl     r13,5
        vmovapd ymm2,YMMWORD[r13*1+rdx]
        vblendvpd       ymm5,ymm5,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm6,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm6,ymm6,ymm0,ymm2

        mov     r10b,r12b
        and     r12,0xf
        vpsubq  ymm0,ymm7,YMMWORD[$L$mask52x4]
        shl     r12,5
        vmovapd ymm2,YMMWORD[r12*1+rdx]
        vblendvpd       ymm7,ymm7,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm8,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm8,ymm8,ymm0,ymm2

        mov     r10b,r11b
        and     r11,0xf
        vpsubq  ymm0,ymm9,YMMWORD[$L$mask52x4]
        shl     r11,5
        vmovapd ymm2,YMMWORD[r11*1+rdx]
        vblendvpd       ymm9,ymm9,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm10,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm10,ymm10,ymm0,ymm2

        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]

        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]

        vmovdqu YMMWORD[rdi],ymm3
        vmovdqu YMMWORD[32+rdi],ymm4
        vmovdqu YMMWORD[64+rdi],ymm5
        vmovdqu YMMWORD[96+rdi],ymm6
        vmovdqu YMMWORD[128+rdi],ymm7
        vmovdqu YMMWORD[160+rdi],ymm8
        vmovdqu YMMWORD[192+rdi],ymm9
        vmovdqu YMMWORD[224+rdi],ymm10

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

$L$ossl_rsaz_amm52x30_x1_avxifma256_epilogue:
        mov     rdi,QWORD[8+rsp]        ;WIN64 epilogue
        mov     rsi,QWORD[16+rsp]
        DB      0F3h,0C3h               ;repret

$L$SEH_end_ossl_rsaz_amm52x30_x1_avxifma256:
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


global  ossl_rsaz_amm52x30_x2_avxifma256

ALIGN   32
ossl_rsaz_amm52x30_x2_avxifma256:
        mov     QWORD[8+rsp],rdi        ;WIN64 prologue
        mov     QWORD[16+rsp],rsi
        mov     rax,rsp
$L$SEH_begin_ossl_rsaz_amm52x30_x2_avxifma256:
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
$L$ossl_rsaz_amm52x30_x2_avxifma256_body:

        vpxor   ymm0,ymm0,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0
        vmovapd ymm8,ymm0
        vmovapd ymm9,ymm0
        vmovapd ymm10,ymm0

        xor     r9d,r9d

        mov     r11,rdx
        mov     rax,0xfffffffffffff

        mov     ebx,30

ALIGN   32
$L$loop30:
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

        lea     rsp,[((-264))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[224+rcx]


        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        mov     QWORD[256+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[32+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[64+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[96+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[128+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[160+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[192+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[224+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[32+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[64+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[96+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[128+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[160+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[192+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[224+rcx]

        lea     rsp,[264+rsp]
        lea     r11,[8+r11]
        dec     ebx
        jne     NEAR $L$loop30

        push    r11
        push    rsi
        push    rcx
        push    r8

        vmovq   xmm0,r9
        vpbroadcastq    ymm0,xmm0
        vpblendd        ymm3,ymm3,ymm0,3



        vpsrlq  ymm0,ymm3,52
        vpsrlq  ymm1,ymm4,52
        vpsrlq  ymm2,ymm5,52
        vpsrlq  ymm11,ymm6,52
        vpsrlq  ymm12,ymm7,52
        vpsrlq  ymm13,ymm8,52
        vpsrlq  ymm14,ymm9,52
        vpsrlq  ymm15,ymm10,52

        lea     rsp,[((-32))+rsp]
        vmovupd YMMWORD[rsp],ymm3


        vpermq  ymm15,ymm15,144
        vpermq  ymm3,ymm14,3
        vblendpd        ymm15,ymm15,ymm3,1

        vpermq  ymm14,ymm14,144
        vpermq  ymm3,ymm13,3
        vblendpd        ymm14,ymm14,ymm3,1

        vpermq  ymm13,ymm13,144
        vpermq  ymm3,ymm12,3
        vblendpd        ymm13,ymm13,ymm3,1

        vpermq  ymm12,ymm12,144
        vpermq  ymm3,ymm11,3
        vblendpd        ymm12,ymm12,ymm3,1

        vpermq  ymm11,ymm11,144
        vpermq  ymm3,ymm2,3
        vblendpd        ymm11,ymm11,ymm3,1

        vpermq  ymm2,ymm2,144
        vpermq  ymm3,ymm1,3
        vblendpd        ymm2,ymm2,ymm3,1

        vpermq  ymm1,ymm1,144
        vpermq  ymm3,ymm0,3
        vblendpd        ymm1,ymm1,ymm3,1

        vpermq  ymm0,ymm0,144
        vpand   ymm0,ymm0,YMMWORD[$L$high64x3]

        vmovupd ymm3,YMMWORD[rsp]
        lea     rsp,[32+rsp]


        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]
        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]


        vpaddq  ymm3,ymm3,ymm0
        vpaddq  ymm4,ymm4,ymm1
        vpaddq  ymm5,ymm5,ymm2
        vpaddq  ymm6,ymm6,ymm11
        vpaddq  ymm7,ymm7,ymm12
        vpaddq  ymm8,ymm8,ymm13
        vpaddq  ymm9,ymm9,ymm14
        vpaddq  ymm10,ymm10,ymm15



        vpcmpgtq        ymm0,ymm3,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm1,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r14d,ymm0
        vmovmskpd       r13d,ymm1
        shl     r13b,4
        or      r14b,r13b

        vpcmpgtq        ymm2,ymm5,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm11,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm2
        vmovmskpd       r12d,ymm11
        shl     r12b,4
        or      r13b,r12b

        vpcmpgtq        ymm12,ymm7,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm12
        vmovmskpd       r11d,ymm13
        shl     r11b,4
        or      r12b,r11b

        vpcmpgtq        ymm14,ymm9,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm15,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm14
        vmovmskpd       r10d,ymm15
        shl     r10b,4
        or      r11b,r10b

        add     r14b,r14b
        adc     r13b,r13b
        adc     r12b,r12b
        adc     r11b,r11b


        vpcmpeqq        ymm0,ymm3,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm1,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm0
        vmovmskpd       r8d,ymm1
        shl     r8b,4
        or      r9b,r8b

        vpcmpeqq        ymm2,ymm5,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm11,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm2
        vmovmskpd       edx,ymm11
        shl     dl,4
        or      r8b,dl

        vpcmpeqq        ymm12,ymm7,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm12
        vmovmskpd       ecx,ymm13
        shl     cl,4
        or      dl,cl

        vpcmpeqq        ymm14,ymm9,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm15,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm14
        vmovmskpd       ebx,ymm15
        shl     bl,4
        or      cl,bl

        add     r14b,r9b
        adc     r13b,r8b
        adc     r12b,dl
        adc     r11b,cl

        xor     r14b,r9b
        xor     r13b,r8b
        xor     r12b,dl
        xor     r11b,cl

        lea     rdx,[$L$kmasklut]

        mov     r10b,r14b
        and     r14,0xf
        vpsubq  ymm0,ymm3,YMMWORD[$L$mask52x4]
        shl     r14,5
        vmovapd ymm2,YMMWORD[r14*1+rdx]
        vblendvpd       ymm3,ymm3,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm4,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm4,ymm4,ymm0,ymm2

        mov     r10b,r13b
        and     r13,0xf
        vpsubq  ymm0,ymm5,YMMWORD[$L$mask52x4]
        shl     r13,5
        vmovapd ymm2,YMMWORD[r13*1+rdx]
        vblendvpd       ymm5,ymm5,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm6,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm6,ymm6,ymm0,ymm2

        mov     r10b,r12b
        and     r12,0xf
        vpsubq  ymm0,ymm7,YMMWORD[$L$mask52x4]
        shl     r12,5
        vmovapd ymm2,YMMWORD[r12*1+rdx]
        vblendvpd       ymm7,ymm7,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm8,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm8,ymm8,ymm0,ymm2

        mov     r10b,r11b
        and     r11,0xf
        vpsubq  ymm0,ymm9,YMMWORD[$L$mask52x4]
        shl     r11,5
        vmovapd ymm2,YMMWORD[r11*1+rdx]
        vblendvpd       ymm9,ymm9,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm10,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm10,ymm10,ymm0,ymm2

        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]

        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]
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

        xor     r15d,r15d

        lea     r11,[16+r11]
        mov     rax,0xfffffffffffff

        mov     ebx,30

        vpxor   ymm0,ymm0,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0
        vmovapd ymm8,ymm0
        vmovapd ymm9,ymm0
        vmovapd ymm10,ymm0
ALIGN   32
$L$loop40:
        mov     r13,QWORD[r11]

        vpbroadcastq    ymm1,QWORD[r11]
        mov     rdx,QWORD[256+rsi]
        mulx    r12,r13,r13
        add     r9,r13
        mov     r10,r12
        adc     r10,0

        mov     r13,QWORD[8+r8]
        imul    r13,r9
        and     r13,rax

        vmovq   xmm2,r13
        vpbroadcastq    ymm2,xmm2
        mov     rdx,QWORD[256+rcx]
        mulx    r12,r13,r13
        add     r9,r13
        adc     r10,r12

        shr     r9,52
        sal     r10,12
        or      r9,r10

        lea     rsp,[((-264))+rsp]

{vex}   vpmadd52luq     ymm3,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52luq     ymm4,ymm1,YMMWORD[288+rsi]
{vex}   vpmadd52luq     ymm5,ymm1,YMMWORD[320+rsi]
{vex}   vpmadd52luq     ymm6,ymm1,YMMWORD[352+rsi]
{vex}   vpmadd52luq     ymm7,ymm1,YMMWORD[384+rsi]
{vex}   vpmadd52luq     ymm8,ymm1,YMMWORD[416+rsi]
{vex}   vpmadd52luq     ymm9,ymm1,YMMWORD[448+rsi]
{vex}   vpmadd52luq     ymm10,ymm1,YMMWORD[480+rsi]

{vex}   vpmadd52luq     ymm3,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52luq     ymm4,ymm2,YMMWORD[288+rcx]
{vex}   vpmadd52luq     ymm5,ymm2,YMMWORD[320+rcx]
{vex}   vpmadd52luq     ymm6,ymm2,YMMWORD[352+rcx]
{vex}   vpmadd52luq     ymm7,ymm2,YMMWORD[384+rcx]
{vex}   vpmadd52luq     ymm8,ymm2,YMMWORD[416+rcx]
{vex}   vpmadd52luq     ymm9,ymm2,YMMWORD[448+rcx]
{vex}   vpmadd52luq     ymm10,ymm2,YMMWORD[480+rcx]


        vmovdqu YMMWORD[rsp],ymm3
        vmovdqu YMMWORD[32+rsp],ymm4
        vmovdqu YMMWORD[64+rsp],ymm5
        vmovdqu YMMWORD[96+rsp],ymm6
        vmovdqu YMMWORD[128+rsp],ymm7
        vmovdqu YMMWORD[160+rsp],ymm8
        vmovdqu YMMWORD[192+rsp],ymm9
        vmovdqu YMMWORD[224+rsp],ymm10
        mov     QWORD[256+rsp],0

        vmovdqu ymm3,YMMWORD[8+rsp]
        vmovdqu ymm4,YMMWORD[40+rsp]
        vmovdqu ymm5,YMMWORD[72+rsp]
        vmovdqu ymm6,YMMWORD[104+rsp]
        vmovdqu ymm7,YMMWORD[136+rsp]
        vmovdqu ymm8,YMMWORD[168+rsp]
        vmovdqu ymm9,YMMWORD[200+rsp]
        vmovdqu ymm10,YMMWORD[232+rsp]

        add     r9,QWORD[8+rsp]

{vex}   vpmadd52huq     ymm3,ymm1,YMMWORD[256+rsi]
{vex}   vpmadd52huq     ymm4,ymm1,YMMWORD[288+rsi]
{vex}   vpmadd52huq     ymm5,ymm1,YMMWORD[320+rsi]
{vex}   vpmadd52huq     ymm6,ymm1,YMMWORD[352+rsi]
{vex}   vpmadd52huq     ymm7,ymm1,YMMWORD[384+rsi]
{vex}   vpmadd52huq     ymm8,ymm1,YMMWORD[416+rsi]
{vex}   vpmadd52huq     ymm9,ymm1,YMMWORD[448+rsi]
{vex}   vpmadd52huq     ymm10,ymm1,YMMWORD[480+rsi]

{vex}   vpmadd52huq     ymm3,ymm2,YMMWORD[256+rcx]
{vex}   vpmadd52huq     ymm4,ymm2,YMMWORD[288+rcx]
{vex}   vpmadd52huq     ymm5,ymm2,YMMWORD[320+rcx]
{vex}   vpmadd52huq     ymm6,ymm2,YMMWORD[352+rcx]
{vex}   vpmadd52huq     ymm7,ymm2,YMMWORD[384+rcx]
{vex}   vpmadd52huq     ymm8,ymm2,YMMWORD[416+rcx]
{vex}   vpmadd52huq     ymm9,ymm2,YMMWORD[448+rcx]
{vex}   vpmadd52huq     ymm10,ymm2,YMMWORD[480+rcx]

        lea     rsp,[264+rsp]
        lea     r11,[8+r11]
        dec     ebx
        jne     NEAR $L$loop40

        vmovq   xmm0,r9
        vpbroadcastq    ymm0,xmm0
        vpblendd        ymm3,ymm3,ymm0,3



        vpsrlq  ymm0,ymm3,52
        vpsrlq  ymm1,ymm4,52
        vpsrlq  ymm2,ymm5,52
        vpsrlq  ymm11,ymm6,52
        vpsrlq  ymm12,ymm7,52
        vpsrlq  ymm13,ymm8,52
        vpsrlq  ymm14,ymm9,52
        vpsrlq  ymm15,ymm10,52

        lea     rsp,[((-32))+rsp]
        vmovupd YMMWORD[rsp],ymm3


        vpermq  ymm15,ymm15,144
        vpermq  ymm3,ymm14,3
        vblendpd        ymm15,ymm15,ymm3,1

        vpermq  ymm14,ymm14,144
        vpermq  ymm3,ymm13,3
        vblendpd        ymm14,ymm14,ymm3,1

        vpermq  ymm13,ymm13,144
        vpermq  ymm3,ymm12,3
        vblendpd        ymm13,ymm13,ymm3,1

        vpermq  ymm12,ymm12,144
        vpermq  ymm3,ymm11,3
        vblendpd        ymm12,ymm12,ymm3,1

        vpermq  ymm11,ymm11,144
        vpermq  ymm3,ymm2,3
        vblendpd        ymm11,ymm11,ymm3,1

        vpermq  ymm2,ymm2,144
        vpermq  ymm3,ymm1,3
        vblendpd        ymm2,ymm2,ymm3,1

        vpermq  ymm1,ymm1,144
        vpermq  ymm3,ymm0,3
        vblendpd        ymm1,ymm1,ymm3,1

        vpermq  ymm0,ymm0,144
        vpand   ymm0,ymm0,YMMWORD[$L$high64x3]

        vmovupd ymm3,YMMWORD[rsp]
        lea     rsp,[32+rsp]


        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]
        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]


        vpaddq  ymm3,ymm3,ymm0
        vpaddq  ymm4,ymm4,ymm1
        vpaddq  ymm5,ymm5,ymm2
        vpaddq  ymm6,ymm6,ymm11
        vpaddq  ymm7,ymm7,ymm12
        vpaddq  ymm8,ymm8,ymm13
        vpaddq  ymm9,ymm9,ymm14
        vpaddq  ymm10,ymm10,ymm15



        vpcmpgtq        ymm0,ymm3,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm1,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r14d,ymm0
        vmovmskpd       r13d,ymm1
        shl     r13b,4
        or      r14b,r13b

        vpcmpgtq        ymm2,ymm5,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm11,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r13d,ymm2
        vmovmskpd       r12d,ymm11
        shl     r12b,4
        or      r13b,r12b

        vpcmpgtq        ymm12,ymm7,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       r12d,ymm12
        vmovmskpd       r11d,ymm13
        shl     r11b,4
        or      r12b,r11b

        vpcmpgtq        ymm14,ymm9,YMMWORD[$L$mask52x4]
        vpcmpgtq        ymm15,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       r11d,ymm14
        vmovmskpd       r10d,ymm15
        shl     r10b,4
        or      r11b,r10b

        add     r14b,r14b
        adc     r13b,r13b
        adc     r12b,r12b
        adc     r11b,r11b


        vpcmpeqq        ymm0,ymm3,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm1,ymm4,YMMWORD[$L$mask52x4]
        vmovmskpd       r9d,ymm0
        vmovmskpd       r8d,ymm1
        shl     r8b,4
        or      r9b,r8b

        vpcmpeqq        ymm2,ymm5,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm11,ymm6,YMMWORD[$L$mask52x4]
        vmovmskpd       r8d,ymm2
        vmovmskpd       edx,ymm11
        shl     dl,4
        or      r8b,dl

        vpcmpeqq        ymm12,ymm7,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm13,ymm8,YMMWORD[$L$mask52x4]
        vmovmskpd       edx,ymm12
        vmovmskpd       ecx,ymm13
        shl     cl,4
        or      dl,cl

        vpcmpeqq        ymm14,ymm9,YMMWORD[$L$mask52x4]
        vpcmpeqq        ymm15,ymm10,YMMWORD[$L$mask52x4]
        vmovmskpd       ecx,ymm14
        vmovmskpd       ebx,ymm15
        shl     bl,4
        or      cl,bl

        add     r14b,r9b
        adc     r13b,r8b
        adc     r12b,dl
        adc     r11b,cl

        xor     r14b,r9b
        xor     r13b,r8b
        xor     r12b,dl
        xor     r11b,cl

        lea     rdx,[$L$kmasklut]

        mov     r10b,r14b
        and     r14,0xf
        vpsubq  ymm0,ymm3,YMMWORD[$L$mask52x4]
        shl     r14,5
        vmovapd ymm2,YMMWORD[r14*1+rdx]
        vblendvpd       ymm3,ymm3,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm4,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm4,ymm4,ymm0,ymm2

        mov     r10b,r13b
        and     r13,0xf
        vpsubq  ymm0,ymm5,YMMWORD[$L$mask52x4]
        shl     r13,5
        vmovapd ymm2,YMMWORD[r13*1+rdx]
        vblendvpd       ymm5,ymm5,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm6,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm6,ymm6,ymm0,ymm2

        mov     r10b,r12b
        and     r12,0xf
        vpsubq  ymm0,ymm7,YMMWORD[$L$mask52x4]
        shl     r12,5
        vmovapd ymm2,YMMWORD[r12*1+rdx]
        vblendvpd       ymm7,ymm7,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm8,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm8,ymm8,ymm0,ymm2

        mov     r10b,r11b
        and     r11,0xf
        vpsubq  ymm0,ymm9,YMMWORD[$L$mask52x4]
        shl     r11,5
        vmovapd ymm2,YMMWORD[r11*1+rdx]
        vblendvpd       ymm9,ymm9,ymm0,ymm2

        shr     r10b,4
        and     r10,0xf
        vpsubq  ymm0,ymm10,YMMWORD[$L$mask52x4]
        shl     r10,5
        vmovapd ymm2,YMMWORD[r10*1+rdx]
        vblendvpd       ymm10,ymm10,ymm0,ymm2

        vpand   ymm3,ymm3,YMMWORD[$L$mask52x4]
        vpand   ymm4,ymm4,YMMWORD[$L$mask52x4]
        vpand   ymm5,ymm5,YMMWORD[$L$mask52x4]
        vpand   ymm6,ymm6,YMMWORD[$L$mask52x4]
        vpand   ymm7,ymm7,YMMWORD[$L$mask52x4]
        vpand   ymm8,ymm8,YMMWORD[$L$mask52x4]
        vpand   ymm9,ymm9,YMMWORD[$L$mask52x4]

        vpand   ymm10,ymm10,YMMWORD[$L$mask52x4]

        vmovdqu YMMWORD[256+rdi],ymm3
        vmovdqu YMMWORD[288+rdi],ymm4
        vmovdqu YMMWORD[320+rdi],ymm5
        vmovdqu YMMWORD[352+rdi],ymm6
        vmovdqu YMMWORD[384+rdi],ymm7
        vmovdqu YMMWORD[416+rdi],ymm8
        vmovdqu YMMWORD[448+rdi],ymm9
        vmovdqu YMMWORD[480+rdi],ymm10

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

$L$ossl_rsaz_amm52x30_x2_avxifma256_epilogue:
        mov     rdi,QWORD[8+rsp]        ;WIN64 epilogue
        mov     rsi,QWORD[16+rsp]
        DB      0F3h,0C3h               ;repret

$L$SEH_end_ossl_rsaz_amm52x30_x2_avxifma256:
section .text code align=64


ALIGN   32
global  ossl_extract_multiplier_2x30_win5_avx

ossl_extract_multiplier_2x30_win5_avx:

DB      243,15,30,250
        vmovapd ymm12,YMMWORD[$L$ones]
        vmovq   xmm8,r8
        vpbroadcastq    ymm10,xmm8
        vmovq   xmm8,r9
        vpbroadcastq    ymm11,xmm8
        lea     rax,[16384+rdx]


        vpxor   xmm0,xmm0,xmm0
        vmovapd ymm9,ymm0
        vmovapd ymm1,ymm0
        vmovapd ymm2,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0

ALIGN   32
$L$loop:
        vpcmpeqq        ymm13,ymm10,ymm9
        vmovdqu ymm8,YMMWORD[rdx]

        vblendvpd       ymm0,ymm0,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[32+rdx]

        vblendvpd       ymm1,ymm1,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[64+rdx]

        vblendvpd       ymm2,ymm2,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[96+rdx]

        vblendvpd       ymm3,ymm3,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[128+rdx]

        vblendvpd       ymm4,ymm4,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[160+rdx]

        vblendvpd       ymm5,ymm5,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[192+rdx]

        vblendvpd       ymm6,ymm6,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[224+rdx]

        vblendvpd       ymm7,ymm7,ymm8,ymm13
        vpaddq  ymm9,ymm9,ymm12
        add     rdx,512
        cmp     rax,rdx
        jne     NEAR $L$loop
        vmovdqu YMMWORD[rcx],ymm0
        vmovdqu YMMWORD[32+rcx],ymm1
        vmovdqu YMMWORD[64+rcx],ymm2
        vmovdqu YMMWORD[96+rcx],ymm3
        vmovdqu YMMWORD[128+rcx],ymm4
        vmovdqu YMMWORD[160+rcx],ymm5
        vmovdqu YMMWORD[192+rcx],ymm6
        vmovdqu YMMWORD[224+rcx],ymm7
        lea     rdx,[((-16384))+rax]


        vpxor   xmm0,xmm0,xmm0
        vmovapd ymm9,ymm0
        vmovapd ymm0,ymm0
        vmovapd ymm1,ymm0
        vmovapd ymm2,ymm0
        vmovapd ymm3,ymm0
        vmovapd ymm4,ymm0
        vmovapd ymm5,ymm0
        vmovapd ymm6,ymm0
        vmovapd ymm7,ymm0

ALIGN   32
$L$loop_8_15:
        vpcmpeqq        ymm13,ymm11,ymm9
        vmovdqu ymm8,YMMWORD[256+rdx]

        vblendvpd       ymm0,ymm0,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[288+rdx]

        vblendvpd       ymm1,ymm1,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[320+rdx]

        vblendvpd       ymm2,ymm2,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[352+rdx]

        vblendvpd       ymm3,ymm3,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[384+rdx]

        vblendvpd       ymm4,ymm4,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[416+rdx]

        vblendvpd       ymm5,ymm5,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[448+rdx]

        vblendvpd       ymm6,ymm6,ymm8,ymm13
        vmovdqu ymm8,YMMWORD[480+rdx]

        vblendvpd       ymm7,ymm7,ymm8,ymm13
        vpaddq  ymm9,ymm9,ymm12
        add     rdx,512
        cmp     rax,rdx
        jne     NEAR $L$loop_8_15
        vmovdqu YMMWORD[256+rcx],ymm0
        vmovdqu YMMWORD[288+rcx],ymm1
        vmovdqu YMMWORD[320+rcx],ymm2
        vmovdqu YMMWORD[352+rcx],ymm3
        vmovdqu YMMWORD[384+rcx],ymm4
        vmovdqu YMMWORD[416+rcx],ymm5
        vmovdqu YMMWORD[448+rcx],ymm6
        vmovdqu YMMWORD[480+rcx],ymm7

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
        DD      $L$SEH_begin_ossl_rsaz_amm52x30_x1_avxifma256 wrt ..imagebase
        DD      $L$SEH_end_ossl_rsaz_amm52x30_x1_avxifma256 wrt ..imagebase
        DD      $L$SEH_info_ossl_rsaz_amm52x30_x1_avxifma256 wrt ..imagebase

        DD      $L$SEH_begin_ossl_rsaz_amm52x30_x2_avxifma256 wrt ..imagebase
        DD      $L$SEH_end_ossl_rsaz_amm52x30_x2_avxifma256 wrt ..imagebase
        DD      $L$SEH_info_ossl_rsaz_amm52x30_x2_avxifma256 wrt ..imagebase

section .xdata rdata align=8
ALIGN   8
$L$SEH_info_ossl_rsaz_amm52x30_x1_avxifma256:
DB      9,0,0,0
        DD      rsaz_avx_handler wrt ..imagebase
        DD      $L$ossl_rsaz_amm52x30_x1_avxifma256_body wrt ..imagebase,$L$ossl_rsaz_amm52x30_x1_avxifma256_epilogue wrt ..imagebase
$L$SEH_info_ossl_rsaz_amm52x30_x2_avxifma256:
DB      9,0,0,0
        DD      rsaz_avx_handler wrt ..imagebase
        DD      $L$ossl_rsaz_amm52x30_x2_avxifma256_body wrt ..imagebase,$L$ossl_rsaz_amm52x30_x2_avxifma256_epilogue wrt ..imagebase
