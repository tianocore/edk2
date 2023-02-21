#include "arm_arch.h"

#if __ARM_MAX_ARCH__>=8

#ifdef __thumb2__
.syntax	unified
.thumb
# define INST(a,b,c,d)   c,0xef,a,b
#else
.code	32
# define INST(a,b,c,d)   a,b,c,0xf2
#endif

.text
.globl	aes_gcm_enc_128_kernel

.align	4
aes_gcm_enc_128_kernel:
	cbz	r1, L128_enc_ret
	stp	r19, r20, [sp, #-112]!
	mov	r16, r4
	mov	r8, r5
	stp	r21, r22, [sp, #16]
	stp	r23, r24, [sp, #32]
	stp	d8, d9, [sp, #48]
	stp	d10, d11, [sp, #64]
	stp	d12, d13, [sp, #80]
	stp	d14, d15, [sp, #96]

	ldp	r10, r11, [r16]              @ ctr96_b64, ctr96_t32
	ldp	r13, r14, [r8, #160]                     @ load rk10

	ld1	{v11.16b}, [r3]
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b
	lsr	r5, r1, #3              @ byte_len
	mov	r15, r5

	ldr	q27, [r8, #144]                                @ load rk9
	add	r4, r0, r1, lsr #3   @ end_input_ptr
	sub	r5, r5, #1      @ byte_len - 1

	lsr	r12, r11, #32
	ldr	q15, [r3, #112]                        @ load h4l | h4h
	ext	v15.16b, v15.16b, v15.16b, #8

	fmov	d1, r10                               @ CTR block 1
	rev	r12, r12                                @ rev_ctr32

	add	r12, r12, #1                            @ increment rev_ctr32
	orr	r11, r11, r11
	ldr	q18, [r8, #0]                                  @ load rk0

	rev	r9, r12                                 @ CTR block 1
	add	r12, r12, #1                            @ CTR block 1
	fmov	d3, r10                               @ CTR block 3

	orr	r9, r11, r9, lsl #32            @ CTR block 1
	ld1	{ q0}, [r16]                             @ special case vector load initial counter so we can start first AES block as quickly as possible

	fmov	v1.d[1], r9                               @ CTR block 1
	rev	r9, r12                                 @ CTR block 2

	fmov	d2, r10                               @ CTR block 2
	orr	r9, r11, r9, lsl #32            @ CTR block 2
	add	r12, r12, #1                            @ CTR block 2

	fmov	v2.d[1], r9                               @ CTR block 2
	rev	r9, r12                                 @ CTR block 3

	orr	r9, r11, r9, lsl #32            @ CTR block 3
	ldr	q19, [r8, #16]                                 @ load rk1

	add	r12, r12, #1                            @ CTR block 3
	fmov	v3.d[1], r9                               @ CTR block 3

	ldr	q14, [r3, #80]                         @ load h3l | h3h
	ext	v14.16b, v14.16b, v14.16b, #8

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 1 - round 0
	ldr	q20, [r8, #32]                                 @ load rk2

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 2 - round 0
	ldr	q12, [r3, #32]                         @ load h1l | h1h
	ext	v12.16b, v12.16b, v12.16b, #8

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 0 - round 0
	ldr	q26, [r8, #128]                                @ load rk8

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 3 - round 0
	ldr	q21, [r8, #48]                                 @ load rk3

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 2 - round 1
	trn2	v17.2d,  v14.2d,    v15.2d                      @ h4l | h3l

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 0 - round 1
	ldr	q24, [r8, #96]                                 @ load rk6

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 1 - round 1
	ldr	q25, [r8, #112]                                @ load rk7

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 3 - round 1
	trn1	q9, v14.2d,    v15.2d                      @ h4h | h3h

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 0 - round 2
	ldr	q23, [r8, #80]                                 @ load rk5

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 1 - round 2
	ldr	q13, [r3, #64]                         @ load h2l | h2h
	ext	v13.16b, v13.16b, v13.16b, #8

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 3 - round 2

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 2 - round 2
	eor	v17.16b, v17.16b, q9                  @ h4k | h3k

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 0 - round 3

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 1 - round 3

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 2 - round 3
	ldr	q22, [r8, #64]                                 @ load rk4

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 3 - round 3

	and	r5, r5, #0xffffffffffffffc0    @ number of bytes to be processed in main loop (at least 1 byte must be handled by tail)
	trn2	v16.2d,  v12.2d,    v13.2d                      @ h2l | h1l

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 3 - round 4
	add	r5, r5, r0

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 2 - round 4
	cmp	r0, r5                   @ check if we have <= 4 blocks

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 0 - round 4

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 3 - round 5

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 2 - round 5

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 0 - round 5

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 3 - round 6

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 1 - round 4

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 2 - round 6
	trn1	q8,    v12.2d,    v13.2d                      @ h2h | h1h

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 0 - round 6

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 1 - round 5

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 3 - round 7

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 0 - round 7

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 1 - round 6

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 2 - round 7

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 0 - round 8

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 1 - round 7

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 2 - round 8

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 3 - round 8

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 1 - round 8

	aese	q2, v27.16b                                      @ AES block 2 - round 9

	aese	q0, v27.16b                                      @ AES block 0 - round 9

	eor	v16.16b, v16.16b, q8                     @ h2k | h1k

	aese	q1, v27.16b                                      @ AES block 1 - round 9

	aese	q3, v27.16b                                      @ AES block 3 - round 9
	bge	L128_enc_tail                                    @ handle tail

	ldp	r6, r7, [r0, #0]            @ AES block 0 - load plaintext

	ldp	r21, r22, [r0, #32]           @ AES block 2 - load plaintext

	ldp	r19, r20, [r0, #16]           @ AES block 1 - load plaintext

	ldp	r23, r24, [r0, #48]           @ AES block 3 - load plaintext

	eor	r6, r6, r13                     @ AES block 0 - round 10 low
	eor	r7, r7, r14                     @ AES block 0 - round 10 high

	eor	r21, r21, r13                     @ AES block 2 - round 10 low
	fmov	d4, r6                               @ AES block 0 - mov low

	eor	r19, r19, r13                     @ AES block 1 - round 10 low
	eor	r22, r22, r14                     @ AES block 2 - round 10 high
	fmov	v4.d[1], r7                           @ AES block 0 - mov high

	fmov	d5, r19                               @ AES block 1 - mov low
	eor	r20, r20, r14                     @ AES block 1 - round 10 high

	eor	r23, r23, r13                     @ AES block 3 - round 10 low
	fmov	v5.d[1], r20                           @ AES block 1 - mov high

	fmov	d6, r21                               @ AES block 2 - mov low
	eor	r24, r24, r14                     @ AES block 3 - round 10 high
	rev	r9, r12                                 @ CTR block 4

	fmov	v6.d[1], r22                           @ AES block 2 - mov high
	orr	r9, r11, r9, lsl #32            @ CTR block 4

	eor	q4, q4, q0                          @ AES block 0 - result
	fmov	d0, r10                               @ CTR block 4
	add	r12, r12, #1                            @ CTR block 4

	fmov	v0.d[1], r9                               @ CTR block 4
	rev	r9, r12                                 @ CTR block 5

	eor	q5, q5, q1                          @ AES block 1 - result
	fmov	d1, r10                               @ CTR block 5
	orr	r9, r11, r9, lsl #32            @ CTR block 5

	add	r12, r12, #1                            @ CTR block 5
	add	r0, r0, #64                       @ AES input_ptr update
	fmov	v1.d[1], r9                               @ CTR block 5

	fmov	d7, r23                               @ AES block 3 - mov low
	rev	r9, r12                                 @ CTR block 6
	st1	{ q4}, [r2], #16                     @ AES block 0 - store result

	fmov	v7.d[1], r24                           @ AES block 3 - mov high
	orr	r9, r11, r9, lsl #32            @ CTR block 6

	add	r12, r12, #1                            @ CTR block 6
	eor	q6, q6, q2                          @ AES block 2 - result
	st1	{ q5}, [r2], #16                     @ AES block 1 - store result

	fmov	d2, r10                               @ CTR block 6
	cmp	r0, r5                   @ check if we have <= 8 blocks

	fmov	v2.d[1], r9                               @ CTR block 6
	rev	r9, r12                                 @ CTR block 7
	st1	{ q6}, [r2], #16                     @ AES block 2 - store result

	orr	r9, r11, r9, lsl #32            @ CTR block 7

	eor	q7, q7, q3                          @ AES block 3 - result
	st1	{ q7}, [r2], #16                     @ AES block 3 - store result
	bge	L128_enc_prepretail                              @ do prepretail

L128_enc_main_loop:@ main loop start
	ldp	r23, r24, [r0, #48]           @ AES block 4k+3 - load plaintext
	rev64	q4, q4                                    @ GHASH block 4k (only t0 is free)
	rev64	q6, q6                                    @ GHASH block 4k+2 (t0, t1, and t2 free)

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0
	fmov	d3, r10                               @ CTR block 4k+3

	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0
	rev64	q5, q5                                    @ GHASH block 4k+1 (t0 and t1 free)

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	add	r12, r12, #1                            @ CTR block 4k+3
	fmov	v3.d[1], r9                               @ CTR block 4k+3

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	mov	d31, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	mov	d30, v5.d[1]                                  @ GHASH block 4k+1 - mid

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	eor	q4, q4, v11.16b                           @ PRE 1

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	eor	r24, r24, r14                     @ AES block 4k+3 - round 10 high

	pmull2	v28.1q, q5, v14.2d                          @ GHASH block 4k+1 - high
	eor	v31.8b, v31.8b, q6                          @ GHASH block 4k+2 - mid
	ldp	r6, r7, [r0, #0]            @ AES block 4k+4 - load plaintext

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	rev	r9, r12                                 @ CTR block 4k+8

	eor	v30.8b, v30.8b, q5                          @ GHASH block 4k+1 - mid
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+8

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high
	add	r12, r12, #1                            @ CTR block 4k+8
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	eor	q8, q8, q4                          @ GHASH block 4k - mid

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3
	eor	q9, q9, v28.16b                         @ GHASH block 4k+1 - high

	pmull	v28.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid
	rev64	q7, q7                                    @ GHASH block 4k+3 (t0, t1, t2 and t3 free)

	pmull	v30.1q, v30.1d, v17.1d                          @ GHASH block 4k+1 - mid

	pmull	v29.1q, q5, v14.1d                          @ GHASH block 4k+1 - low
	ins	v31.d[1], v31.d[0]                                @ GHASH block 4k+2 - mid

	pmull2	v8.1q, q6, v13.2d                          @ GHASH block 4k+2 - high
	eor	r7, r7, r14                     @ AES block 4k+4 - round 10 high

	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+1 - mid
	mov	d30, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1
	eor	v11.16b, v11.16b, v29.16b                         @ GHASH block 4k+1 - low

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2
	eor	r6, r6, r13                     @ AES block 4k+4 - round 10 low

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3
	eor	v30.8b, v30.8b, q7                          @ GHASH block 4k+3 - mid

	pmull2	v4.1q, q7, v12.2d                          @ GHASH block 4k+3 - high

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3
	eor	q9, q9, q8                         @ GHASH block 4k+2 - high

	pmull2	v31.1q, v31.2d, v16.2d                          @ GHASH block 4k+2 - mid

	pmull	v29.1q, q7, v12.1d                          @ GHASH block 4k+3 - low
	movi	q8, #0xc2

	pmull	v30.1q, v30.1d, v16.1d                          @ GHASH block 4k+3 - mid
	eor	v11.16b, v11.16b, v28.16b                         @ GHASH block 4k+2 - low

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2
	shl	d8, d8, #56               @ mod_constant

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4
	eor	q9, q9, q4                         @ GHASH block 4k+3 - high

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5
	ldp	r19, r20, [r0, #16]           @ AES block 4k+5 - load plaintext

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3
	eor	v10.16b, v10.16b, v31.16b                         @ GHASH block 4k+2 - mid

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5
	ldp	r21, r22, [r0, #32]           @ AES block 4k+6 - load plaintext

	pmull	v31.1q, q9, q8            @ MODULO - top 64b align with mid
	eor	v11.16b, v11.16b, v29.16b                         @ GHASH block 4k+3 - low

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4
	eor	r19, r19, r13                     @ AES block 4k+5 - round 10 low

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4
	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+3 - mid

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6
	eor	r23, r23, r13                     @ AES block 4k+3 - round 10 low

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5
	eor	v30.16b, v11.16b, q9                         @ MODULO - karatsuba tidy up

	fmov	d4, r6                               @ AES block 4k+4 - mov low
	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6
	fmov	v4.d[1], r7                           @ AES block 4k+4 - mov high

	add	r0, r0, #64                       @ AES input_ptr update
	fmov	d7, r23                               @ AES block 4k+3 - mov low
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5
	fmov	d5, r19                               @ AES block 4k+5 - mov low

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7
	eor	v10.16b, v10.16b, v30.16b                         @ MODULO - karatsuba tidy up

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6
	eor	r20, r20, r14                     @ AES block 4k+5 - round 10 high

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7
	fmov	v5.d[1], r20                           @ AES block 4k+5 - mov high

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8
	fmov	v7.d[1], r24                           @ AES block 4k+3 - mov high

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6
	cmp	r0, r5                   @ LOOP CONTROL

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8
	eor	v10.16b, v10.16b, v31.16b                      @ MODULO - fold into mid

	aese	q0, v27.16b                                      @ AES block 4k+4 - round 9
	eor	r21, r21, r13                     @ AES block 4k+6 - round 10 low
	eor	r22, r22, r14                     @ AES block 4k+6 - round 10 high

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7
	fmov	d6, r21                               @ AES block 4k+6 - mov low

	aese	q1, v27.16b                                      @ AES block 4k+5 - round 9
	fmov	v6.d[1], r22                           @ AES block 4k+6 - mov high

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7
	eor	q4, q4, q0                          @ AES block 4k+4 - result

	fmov	d0, r10                               @ CTR block 4k+8
	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8

	fmov	v0.d[1], r9                               @ CTR block 4k+8
	rev	r9, r12                                 @ CTR block 4k+9
	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8
	eor	q5, q5, q1                          @ AES block 4k+5 - result

	add	r12, r12, #1                            @ CTR block 4k+9
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+9
	fmov	d1, r10                               @ CTR block 4k+9

	pmull	v9.1q, v10.1d, q8            @ MODULO - mid 64b align with low
	fmov	v1.d[1], r9                               @ CTR block 4k+9
	rev	r9, r12                                 @ CTR block 4k+10

	aese	q2, v27.16b                                      @ AES block 4k+6 - round 9
	st1	{ q4}, [r2], #16                     @ AES block 4k+4 - store result
	eor	q6, q6, q2                          @ AES block 4k+6 - result
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+10

	aese	q3, v27.16b                                      @ AES block 4k+7 - round 9
	add	r12, r12, #1                            @ CTR block 4k+10
	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment
	fmov	d2, r10                               @ CTR block 4k+10

	eor	v11.16b, v11.16b, q9                         @ MODULO - fold into low
	st1	{ q5}, [r2], #16                     @ AES block 4k+5 - store result

	fmov	v2.d[1], r9                               @ CTR block 4k+10
	st1	{ q6}, [r2], #16                     @ AES block 4k+6 - store result
	rev	r9, r12                                 @ CTR block 4k+11

	orr	r9, r11, r9, lsl #32            @ CTR block 4k+11
	eor	q7, q7, q3                          @ AES block 4k+3 - result

	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
	st1	{ q7}, [r2], #16                     @ AES block 4k+3 - store result
	blt	L128_enc_main_loop

L128_enc_prepretail:@ PREPRETAIL
	rev64	q4, q4                                    @ GHASH block 4k (only t0 is free)
	fmov	d3, r10                               @ CTR block 4k+3
	rev64	q5, q5                                    @ GHASH block 4k+1 (t0 and t1 free)

	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0
	add	r12, r12, #1                            @ CTR block 4k+3
	fmov	v3.d[1], r9                               @ CTR block 4k+3

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	rev64	q6, q6                                    @ GHASH block 4k+2 (t0, t1, and t2 free)

	pmull	v29.1q, q5, v14.1d                          @ GHASH block 4k+1 - low

	rev64	q7, q7                                    @ GHASH block 4k+3 (t0, t1, t2 and t3 free)
	eor	q4, q4, v11.16b                           @ PRE 1

	pmull2	v28.1q, q5, v14.2d                          @ GHASH block 4k+1 - high

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	mov	d30, v5.d[1]                                  @ GHASH block 4k+1 - mid

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid

	mov	d31, v6.d[1]                                  @ GHASH block 4k+2 - mid
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	eor	v30.8b, v30.8b, q5                          @ GHASH block 4k+1 - mid

	eor	q8, q8, q4                          @ GHASH block 4k - mid

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high
	eor	v31.8b, v31.8b, q6                          @ GHASH block 4k+2 - mid

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1

	pmull	v30.1q, v30.1d, v17.1d                          @ GHASH block 4k+1 - mid
	eor	v11.16b, v11.16b, v29.16b                         @ GHASH block 4k+1 - low

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	ins	v31.d[1], v31.d[0]                                @ GHASH block 4k+2 - mid

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0

	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+1 - mid
	mov	d30, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	eor	q9, q9, v28.16b                         @ GHASH block 4k+1 - high

	pmull2	v31.1q, v31.2d, v16.2d                          @ GHASH block 4k+2 - mid

	pmull2	v8.1q, q6, v13.2d                          @ GHASH block 4k+2 - high
	eor	v30.8b, v30.8b, q7                          @ GHASH block 4k+3 - mid

	pmull2	v4.1q, q7, v12.2d                          @ GHASH block 4k+3 - high

	pmull	v28.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	eor	q9, q9, q8                         @ GHASH block 4k+2 - high

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2

	pmull	v29.1q, q7, v12.1d                          @ GHASH block 4k+3 - low
	movi	q8, #0xc2

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2
	eor	v11.16b, v11.16b, v28.16b                         @ GHASH block 4k+2 - low

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2

	pmull	v30.1q, v30.1d, v16.1d                          @ GHASH block 4k+3 - mid
	eor	v10.16b, v10.16b, v31.16b                         @ GHASH block 4k+2 - mid

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2
	eor	q9, q9, q4                         @ GHASH block 4k+3 - high

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3

	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+3 - mid
	shl	d8, d8, #56               @ mod_constant

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3
	eor	v11.16b, v11.16b, v29.16b                         @ GHASH block 4k+3 - low

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4

	pmull	v28.1q, q9, q8
	eor	v10.16b, v10.16b, q9                         @ karatsuba tidy up

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5
	ext	q9, q9, q9, #8

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4
	eor	v10.16b, v10.16b, v11.16b

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5
	eor	v10.16b, v10.16b, v28.16b

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6
	eor	v10.16b, v10.16b, q9

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7

	pmull	v28.1q, v10.1d, q8

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7
	ext	v10.16b, v10.16b, v10.16b, #8

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8
	eor	v11.16b, v11.16b, v28.16b

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8

	aese	q3, v27.16b                                      @ AES block 4k+7 - round 9

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8

	aese	q0, v27.16b                                      @ AES block 4k+4 - round 9

	aese	q1, v27.16b                                      @ AES block 4k+5 - round 9
	eor	v11.16b, v11.16b, v10.16b

	aese	q2, v27.16b                                      @ AES block 4k+6 - round 9
L128_enc_tail:@ TAIL

	sub	r5, r4, r0   @ main_end_input_ptr is number of bytes left to process
	ldp	r6, r7, [r0], #16           @ AES block 4k+4 - load plaintext

	cmp	r5, #48

	ext	q8, v11.16b, v11.16b, #8                     @ prepare final partial tag
	eor	r6, r6, r13                     @ AES block 4k+4 - round 10 low
	eor	r7, r7, r14                     @ AES block 4k+4 - round 10 high

	fmov	d4, r6                               @ AES block 4k+4 - mov low

	fmov	v4.d[1], r7                           @ AES block 4k+4 - mov high

	eor	q5, q4, q0                          @ AES block 4k+4 - result

	bgt	L128_enc_blocks_more_than_3

	sub	r12, r12, #1
	movi	v11.8b, #0
	mov	q3, q2

	cmp	r5, #32
	mov	q2, q1
	movi	q9, #0

	movi	v10.8b, #0
	bgt	L128_enc_blocks_more_than_2

	mov	q3, q1
	cmp	r5, #16

	sub	r12, r12, #1
	bgt	L128_enc_blocks_more_than_1

	sub	r12, r12, #1
	b	L128_enc_blocks_less_than_1
L128_enc_blocks_more_than_3:@ blocks left >  3
	st1	{ q5}, [r2], #16                     @ AES final-3 block  - store result

	ldp	r6, r7, [r0], #16           @ AES final-2 block - load input low & high

	rev64	q4, q5                                    @ GHASH final-3 block

	eor	q4, q4, q8                           @ feed in partial tag
	eor	r7, r7, r14                     @ AES final-2 block - round 10 high
	eor	r6, r6, r13                     @ AES final-2 block - round 10 low

	fmov	d5, r6                                 @ AES final-2 block - mov low

	movi	q8, #0                                        @ suppress further partial tag feed in
	fmov	v5.d[1], r7                             @ AES final-2 block - mov high

	pmull	v11.1q, q4, v15.1d                       @ GHASH final-3 block - low
	mov	d22, v4.d[1]                                 @ GHASH final-3 block - mid

	pmull2	v9.1q, q4, v15.2d                       @ GHASH final-3 block - high

	mov	d10, v17.d[1]                               @ GHASH final-3 block - mid

	eor	q5, q5, q1                            @ AES final-2 block - result
	eor	v22.8b, v22.8b, q4                      @ GHASH final-3 block - mid

	pmull	v10.1q, v22.1d, v10.1d                    @ GHASH final-3 block - mid
L128_enc_blocks_more_than_2:@ blocks left >  2

	st1	{ q5}, [r2], #16                     @ AES final-2 block - store result

	rev64	q4, q5                                    @ GHASH final-2 block
	ldp	r6, r7, [r0], #16           @ AES final-1 block - load input low & high

	eor	q4, q4, q8                           @ feed in partial tag

	eor	r6, r6, r13                     @ AES final-1 block - round 10 low

	fmov	d5, r6                                 @ AES final-1 block - mov low
	eor	r7, r7, r14                     @ AES final-1 block - round 10 high

	pmull2	v20.1q, q4, v14.2d                          @ GHASH final-2 block - high
	fmov	v5.d[1], r7                             @ AES final-1 block - mov high

	mov	d22, v4.d[1]                                 @ GHASH final-2 block - mid

	pmull	v21.1q, q4, v14.1d                          @ GHASH final-2 block - low

	eor	q9, q9, v20.16b                            @ GHASH final-2 block - high

	eor	v22.8b, v22.8b, q4                      @ GHASH final-2 block - mid

	eor	q5, q5, q2                            @ AES final-1 block - result

	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final-2 block - low

	pmull	v22.1q, v22.1d, v17.1d                      @ GHASH final-2 block - mid

	movi	q8, #0                                        @ suppress further partial tag feed in

	eor	v10.16b, v10.16b, v22.16b                       @ GHASH final-2 block - mid
L128_enc_blocks_more_than_1:@ blocks left >  1

	st1	{ q5}, [r2], #16                     @ AES final-1 block - store result

	rev64	q4, q5                                    @ GHASH final-1 block
	ldp	r6, r7, [r0], #16           @ AES final block - load input low & high

	eor	q4, q4, q8                           @ feed in partial tag

	eor	r7, r7, r14                     @ AES final block - round 10 high
	eor	r6, r6, r13                     @ AES final block - round 10 low

	fmov	d5, r6                                 @ AES final block - mov low

	pmull2	v20.1q, q4, v13.2d                          @ GHASH final-1 block - high
	fmov	v5.d[1], r7                             @ AES final block - mov high

	mov	d22, v4.d[1]                                 @ GHASH final-1 block - mid

	pmull	v21.1q, q4, v13.1d                          @ GHASH final-1 block - low

	eor	v22.8b, v22.8b, q4                      @ GHASH final-1 block - mid

	eor	q5, q5, q3                            @ AES final block - result

	ins	v22.d[1], v22.d[0]                            @ GHASH final-1 block - mid

	pmull2	v22.1q, v22.2d, v16.2d                      @ GHASH final-1 block - mid

	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final-1 block - low

	eor	q9, q9, v20.16b                            @ GHASH final-1 block - high

	eor	v10.16b, v10.16b, v22.16b                       @ GHASH final-1 block - mid
	movi	q8, #0                                        @ suppress further partial tag feed in
L128_enc_blocks_less_than_1:@ blocks left <= 1

	and	r1, r1, #127                    @ bit_length %= 128
	mvn	r13, xzr                                      @ rk10_l = 0xffffffffffffffff

	mvn	r14, xzr                                      @ rk10_h = 0xffffffffffffffff
	sub	r1, r1, #128                    @ bit_length -= 128

	neg	r1, r1                          @ bit_length = 128 - #bits in input (in range [1,128])

	and	r1, r1, #127                    @ bit_length %= 128

	lsr	r14, r14, r1                     @ rk10_h is mask for top 64b of last block
	cmp	r1, #64

	csel	r6, r13, r14, lt
	csel	r7, r14, xzr, lt

	fmov	d0, r6                                 @ ctr0b is mask for last block

	fmov	v0.d[1], r7

	and	q5, q5, q0                            @ possibly partial last block has zeroes in highest bits

	rev64	q4, q5                                    @ GHASH final block

	eor	q4, q4, q8                           @ feed in partial tag

	mov	d8, v4.d[1]                                  @ GHASH final block - mid

	pmull	v21.1q, q4, v12.1d                          @ GHASH final block - low
	ld1	{ v18.16b}, [r2]                            @ load existing bytes where the possibly partial last block is to be stored

	eor	q8, q8, q4                          @ GHASH final block - mid

	rev	r9, r12

	pmull2	v20.1q, q4, v12.2d                          @ GHASH final block - high

	pmull	v8.1q, q8, v16.1d                          @ GHASH final block - mid

	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final block - low

	eor	q9, q9, v20.16b                            @ GHASH final block - high

	eor	v10.16b, v10.16b, q8                         @ GHASH final block - mid
	movi	q8, #0xc2

	eor	v30.16b, v11.16b, q9                         @ MODULO - karatsuba tidy up

	shl	d8, d8, #56               @ mod_constant

	eor	v10.16b, v10.16b, v30.16b                         @ MODULO - karatsuba tidy up

	pmull	v31.1q, q9, q8            @ MODULO - top 64b align with mid

	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	eor	v10.16b, v10.16b, v31.16b                      @ MODULO - fold into mid

	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	pmull	v9.1q, v10.1d, q8            @ MODULO - mid 64b align with low

	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	bif	q5, v18.16b, q0                              @ insert existing bytes in top end of result before storing

	eor	v11.16b, v11.16b, q9                         @ MODULO - fold into low
	st1	{ q5}, [r2]                          @ store all 16B

	str	r9, [r16, #12]                          @ store the updated counter

	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b
	mov	r0, r15
	st1	{ v11.16b }, [r3]
	ldp	r21, r22, [sp, #16]
	ldp	r23, r24, [sp, #32]
	ldp	d8, d9, [sp, #48]
	ldp	d10, d11, [sp, #64]
	ldp	d12, d13, [sp, #80]
	ldp	d14, d15, [sp, #96]
	ldp	r19, r20, [sp], #112
	bx	lr

L128_enc_ret:
	mov	r0, #0x0
	bx	lr

.globl	aes_gcm_dec_128_kernel

.align	4
aes_gcm_dec_128_kernel:
	cbz	r1, L128_dec_ret
	stp	r19, r20, [sp, #-112]!
	mov	r16, r4
	mov	r8, r5
	stp	r21, r22, [sp, #16]
	stp	r23, r24, [sp, #32]
	stp	d8, d9, [sp, #48]
	stp	d10, d11, [sp, #64]
	stp	d12, d13, [sp, #80]
	stp	d14, d15, [sp, #96]

	lsr	r5, r1, #3              @ byte_len
	mov	r15, r5
	ldp	r10, r11, [r16]              @ ctr96_b64, ctr96_t32

	sub	r5, r5, #1      @ byte_len - 1
	ldr	q18, [r8, #0]                                  @ load rk0

	and	r5, r5, #0xffffffffffffffc0 @ number of bytes to be processed in main loop (at least 1 byte must be handled by tail)
	ld1	{ q0}, [r16]                             @ special case vector load initial counter so we can start first AES block as quickly as possible

	ldr	q13, [r3, #64]                         @ load h2l | h2h
	ext	v13.16b, v13.16b, v13.16b, #8

	lsr	r12, r11, #32
	fmov	d2, r10                               @ CTR block 2

	ldr	q19, [r8, #16]                                 @ load rk1
	orr	r11, r11, r11
	rev	r12, r12                                @ rev_ctr32

	fmov	d1, r10                               @ CTR block 1
	add	r12, r12, #1                            @ increment rev_ctr32

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 0 - round 0
	rev	r9, r12                                 @ CTR block 1

	orr	r9, r11, r9, lsl #32            @ CTR block 1
	ldr	q20, [r8, #32]                                 @ load rk2
	add	r12, r12, #1                            @ CTR block 1

	fmov	v1.d[1], r9                               @ CTR block 1
	rev	r9, r12                                 @ CTR block 2
	add	r12, r12, #1                            @ CTR block 2

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 0 - round 1
	orr	r9, r11, r9, lsl #32            @ CTR block 2

	fmov	v2.d[1], r9                               @ CTR block 2
	rev	r9, r12                                 @ CTR block 3

	fmov	d3, r10                               @ CTR block 3
	orr	r9, r11, r9, lsl #32            @ CTR block 3
	add	r12, r12, #1                            @ CTR block 3

	fmov	v3.d[1], r9                               @ CTR block 3
	add	r4, r0, r1, lsr #3   @ end_input_ptr

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 1 - round 0
	ldr	q21, [r8, #48]                                 @ load rk3

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 0 - round 2
	ldr	q24, [r8, #96]                                 @ load rk6

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 2 - round 0
	ldr	q25, [r8, #112]                                @ load rk7

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 1 - round 1
	ldr	q22, [r8, #64]                                 @ load rk4

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 3 - round 0

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 2 - round 1

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 1 - round 2
	ldp	r13, r14, [r8, #160]                     @ load rk10

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 3 - round 1
	ld1	{ v11.16b}, [r3]
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 0 - round 3
	ldr	q23, [r8, #80]                                 @ load rk5

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 1 - round 3

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 3 - round 2

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 2 - round 2
	ldr	q27, [r8, #144]                                @ load rk9

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 1 - round 4

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 3 - round 3

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 2 - round 3
	ldr	q14, [r3, #80]                         @ load h3l | h3h
	ext	v14.16b, v14.16b, v14.16b, #8

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 0 - round 4
	ldr	q26, [r8, #128]                                @ load rk8

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 1 - round 5

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 2 - round 4

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 3 - round 4

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 0 - round 5

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 2 - round 5
	ldr	q12, [r3, #32]                         @ load h1l | h1h
	ext	v12.16b, v12.16b, v12.16b, #8

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 3 - round 5

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 0 - round 6

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 1 - round 6

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 3 - round 6

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 2 - round 6
	trn1	q8,    v12.2d,    v13.2d                      @ h2h | h1h

	ldr	q15, [r3, #112]                        @ load h4l | h4h
	ext	v15.16b, v15.16b, v15.16b, #8
	trn2	v16.2d,  v12.2d,    v13.2d                      @ h2l | h1l
	add	r5, r5, r0

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 1 - round 7

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 2 - round 7

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 0 - round 7
	eor	v16.16b, v16.16b, q8                     @ h2k | h1k

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 3 - round 7

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 1 - round 8
	trn2	v17.2d,  v14.2d,    v15.2d                      @ h4l | h3l

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 2 - round 8

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 3 - round 8

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 0 - round 8
	trn1	q9, v14.2d,    v15.2d                      @ h4h | h3h

	aese	q2, v27.16b                                      @ AES block 2 - round 9

	aese	q3, v27.16b                                      @ AES block 3 - round 9

	aese	q0, v27.16b                                      @ AES block 0 - round 9
	cmp	r0, r5                   @ check if we have <= 4 blocks

	aese	q1, v27.16b                                      @ AES block 1 - round 9
	eor	v17.16b, v17.16b, q9                  @ h4k | h3k
	bge	L128_dec_tail                                    @ handle tail

	ldr	q5, [r0, #16]                         @ AES block 1 - load ciphertext

	ldr	q4, [r0, #0]                          @ AES block 0 - load ciphertext

	eor	q1, q5, q1                            @ AES block 1 - result
	ldr	q6, [r0, #32]                         @ AES block 2 - load ciphertext

	eor	q0, q4, q0                            @ AES block 0 - result
	rev64	q4, q4                                    @ GHASH block 0
	rev	r9, r12                                 @ CTR block 4

	orr	r9, r11, r9, lsl #32            @ CTR block 4
	add	r12, r12, #1                            @ CTR block 4
	ldr	q7, [r0, #48]                         @ AES block 3 - load ciphertext

	rev64	q5, q5                                    @ GHASH block 1
	add	r0, r0, #64                       @ AES input_ptr update
	mov	r19, v1.d[0]                            @ AES block 1 - mov low

	mov	r20, v1.d[1]                            @ AES block 1 - mov high

	mov	r6, v0.d[0]                            @ AES block 0 - mov low
	cmp	r0, r5                   @ check if we have <= 8 blocks

	mov	r7, v0.d[1]                            @ AES block 0 - mov high

	fmov	d0, r10                               @ CTR block 4

	fmov	v0.d[1], r9                               @ CTR block 4
	rev	r9, r12                                 @ CTR block 5
	eor	r19, r19, r13                   @ AES block 1 - round 10 low

	fmov	d1, r10                               @ CTR block 5
	add	r12, r12, #1                            @ CTR block 5
	orr	r9, r11, r9, lsl #32            @ CTR block 5

	fmov	v1.d[1], r9                               @ CTR block 5
	rev	r9, r12                                 @ CTR block 6
	add	r12, r12, #1                            @ CTR block 6

	orr	r9, r11, r9, lsl #32            @ CTR block 6

	eor	r20, r20, r14                   @ AES block 1 - round 10 high
	eor	r6, r6, r13                   @ AES block 0 - round 10 low
	eor	q2, q6, q2                            @ AES block 2 - result

	eor	r7, r7, r14                   @ AES block 0 - round 10 high
	stp	r6, r7, [r2], #16        @ AES block 0 - store result

	stp	r19, r20, [r2], #16        @ AES block 1 - store result
	bge	L128_dec_prepretail                              @ do prepretail

L128_dec_main_loop:@ main loop start
	eor	q3, q7, q3                            @ AES block 4k+3 - result
	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0
	mov	r21, v2.d[0]                            @ AES block 4k+2 - mov low

	pmull2	v28.1q, q5, v14.2d                          @ GHASH block 4k+1 - high
	mov	r22, v2.d[1]                            @ AES block 4k+2 - mov high

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	fmov	d2, r10                               @ CTR block 4k+6

	rev64	q6, q6                                    @ GHASH block 4k+2
	fmov	v2.d[1], r9                               @ CTR block 4k+6
	rev	r9, r12                                 @ CTR block 4k+7

	mov	r23, v3.d[0]                            @ AES block 4k+3 - mov low
	eor	q4, q4, v11.16b                           @ PRE 1
	mov	d30, v5.d[1]                                  @ GHASH block 4k+1 - mid

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	rev64	q7, q7                                    @ GHASH block 4k+3

	pmull	v29.1q, q5, v14.1d                          @ GHASH block 4k+1 - low
	mov	r24, v3.d[1]                            @ AES block 4k+3 - mov high
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+7

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	fmov	d3, r10                               @ CTR block 4k+7
	eor	v30.8b, v30.8b, q5                          @ GHASH block 4k+1 - mid

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2
	fmov	v3.d[1], r9                               @ CTR block 4k+7

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high
	eor	v11.16b, v11.16b, v29.16b                         @ GHASH block 4k+1 - low

	pmull	v29.1q, q7, v12.1d                          @ GHASH block 4k+3 - low

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	eor	q9, q9, v28.16b                         @ GHASH block 4k+1 - high

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0

	pmull	v28.1q, q6, v13.1d                          @ GHASH block 4k+2 - low
	eor	q8, q8, q4                          @ GHASH block 4k - mid

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1
	eor	r23, r23, r13                   @ AES block 4k+3 - round 10 low

	pmull	v30.1q, v30.1d, v17.1d                          @ GHASH block 4k+1 - mid
	eor	r22, r22, r14                   @ AES block 4k+2 - round 10 high
	mov	d31, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	eor	v11.16b, v11.16b, v28.16b                         @ GHASH block 4k+2 - low

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2
	eor	v31.8b, v31.8b, q6                          @ GHASH block 4k+2 - mid

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4
	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+1 - mid

	pmull2	v8.1q, q6, v13.2d                          @ GHASH block 4k+2 - high

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3
	ins	v31.d[1], v31.d[0]                                @ GHASH block 4k+2 - mid

	pmull2	v4.1q, q7, v12.2d                          @ GHASH block 4k+3 - high

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	mov	d30, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4
	eor	q9, q9, q8                         @ GHASH block 4k+2 - high

	pmull2	v31.1q, v31.2d, v16.2d                          @ GHASH block 4k+2 - mid
	eor	r24, r24, r14                   @ AES block 4k+3 - round 10 high

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2
	eor	v30.8b, v30.8b, q7                          @ GHASH block 4k+3 - mid

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5
	eor	r21, r21, r13                   @ AES block 4k+2 - round 10 low

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5
	movi	q8, #0xc2

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3
	eor	v11.16b, v11.16b, v29.16b                         @ GHASH block 4k+3 - low

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6
	eor	v10.16b, v10.16b, v31.16b                         @ GHASH block 4k+2 - mid

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4
	stp	r21, r22, [r2], #16        @ AES block 4k+2 - store result

	pmull	v30.1q, v30.1d, v16.1d                          @ GHASH block 4k+3 - mid
	eor	q9, q9, q4                         @ GHASH block 4k+3 - high
	ldr	q4, [r0, #0]                          @ AES block 4k+4 - load ciphertext

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7
	add	r12, r12, #1                            @ CTR block 4k+7

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7
	shl	d8, d8, #56               @ mod_constant

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5
	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+3 - mid

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8
	stp	r23, r24, [r2], #16        @ AES block 4k+3 - store result

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8
	eor	v30.16b, v11.16b, q9                         @ MODULO - karatsuba tidy up

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3
	rev	r9, r12                                 @ CTR block 4k+8

	pmull	v31.1q, q9, q8            @ MODULO - top 64b align with mid
	ldr	q5, [r0, #16]                         @ AES block 4k+5 - load ciphertext
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	aese	q0, v27.16b                                      @ AES block 4k+4 - round 9
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+8

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4
	eor	v10.16b, v10.16b, v30.16b                         @ MODULO - karatsuba tidy up

	aese	q1, v27.16b                                      @ AES block 4k+5 - round 9

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6
	eor	q0, q4, q0                            @ AES block 4k+4 - result

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5
	ldr	q6, [r0, #32]                         @ AES block 4k+6 - load ciphertext

	add	r12, r12, #1                            @ CTR block 4k+8
	eor	v10.16b, v10.16b, v31.16b                      @ MODULO - fold into mid
	eor	q1, q5, q1                            @ AES block 4k+5 - result

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7
	ldr	q7, [r0, #48]                         @ AES block 4k+3 - load ciphertext

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6
	add	r0, r0, #64                       @ AES input_ptr update

	rev64	q5, q5                                    @ GHASH block 4k+5
	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid
	mov	r7, v0.d[1]                            @ AES block 4k+4 - mov high

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8
	mov	r6, v0.d[0]                            @ AES block 4k+4 - mov low

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7
	fmov	d0, r10                               @ CTR block 4k+8

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low
	fmov	v0.d[1], r9                               @ CTR block 4k+8
	rev	r9, r12                                 @ CTR block 4k+9

	aese	q2, v27.16b                                      @ AES block 4k+6 - round 9
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+9
	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8
	eor	r7, r7, r14                   @ AES block 4k+4 - round 10 high

	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low
	mov	r20, v1.d[1]                            @ AES block 4k+5 - mov high
	eor	r6, r6, r13                   @ AES block 4k+4 - round 10 low

	eor	q2, q6, q2                            @ AES block 4k+6 - result
	mov	r19, v1.d[0]                            @ AES block 4k+5 - mov low
	add	r12, r12, #1                            @ CTR block 4k+9

	aese	q3, v27.16b                                      @ AES block 4k+7 - round 9
	fmov	d1, r10                               @ CTR block 4k+9
	cmp	r0, r5                   @ LOOP CONTROL

	rev64	q4, q4                                    @ GHASH block 4k+4
	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
	fmov	v1.d[1], r9                               @ CTR block 4k+9

	rev	r9, r12                                 @ CTR block 4k+10
	add	r12, r12, #1                            @ CTR block 4k+10

	eor	r20, r20, r14                   @ AES block 4k+5 - round 10 high
	stp	r6, r7, [r2], #16        @ AES block 4k+4 - store result

	eor	r19, r19, r13                   @ AES block 4k+5 - round 10 low
	stp	r19, r20, [r2], #16        @ AES block 4k+5 - store result

	orr	r9, r11, r9, lsl #32            @ CTR block 4k+10
	blt	L128_dec_main_loop

L128_dec_prepretail:@ PREPRETAIL
	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0
	mov	r21, v2.d[0]                            @ AES block 4k+2 - mov low
	mov	d30, v5.d[1]                                  @ GHASH block 4k+1 - mid

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	eor	q3, q7, q3                            @ AES block 4k+3 - result

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	mov	r22, v2.d[1]                            @ AES block 4k+2 - mov high

	eor	q4, q4, v11.16b                           @ PRE 1
	fmov	d2, r10                               @ CTR block 4k+6
	rev64	q6, q6                                    @ GHASH block 4k+2

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	fmov	v2.d[1], r9                               @ CTR block 4k+6

	rev	r9, r12                                 @ CTR block 4k+7
	mov	r23, v3.d[0]                            @ AES block 4k+3 - mov low
	eor	v30.8b, v30.8b, q5                          @ GHASH block 4k+1 - mid

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid
	mov	r24, v3.d[1]                            @ AES block 4k+3 - mov high

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	mov	d31, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+7

	pmull	v29.1q, q5, v14.1d                          @ GHASH block 4k+1 - low
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid
	fmov	d3, r10                               @ CTR block 4k+7

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0
	fmov	v3.d[1], r9                               @ CTR block 4k+7

	pmull	v30.1q, v30.1d, v17.1d                          @ GHASH block 4k+1 - mid
	eor	v31.8b, v31.8b, q6                          @ GHASH block 4k+2 - mid

	rev64	q7, q7                                    @ GHASH block 4k+3

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	eor	q8, q8, q4                          @ GHASH block 4k - mid

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	ins	v31.d[1], v31.d[0]                                @ GHASH block 4k+2 - mid

	pmull2	v28.1q, q5, v14.2d                          @ GHASH block 4k+1 - high

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid
	eor	v11.16b, v11.16b, v29.16b                         @ GHASH block 4k+1 - low

	pmull	v29.1q, q7, v12.1d                          @ GHASH block 4k+3 - low

	pmull2	v31.1q, v31.2d, v16.2d                          @ GHASH block 4k+2 - mid
	eor	q9, q9, v28.16b                         @ GHASH block 4k+1 - high

	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+1 - mid

	pmull2	v4.1q, q7, v12.2d                          @ GHASH block 4k+3 - high

	pmull2	v8.1q, q6, v13.2d                          @ GHASH block 4k+2 - high
	mov	d30, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2
	eor	v10.16b, v10.16b, v31.16b                         @ GHASH block 4k+2 - mid

	pmull	v28.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	eor	q9, q9, q8                         @ GHASH block 4k+2 - high
	movi	q8, #0xc2

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1
	eor	v30.8b, v30.8b, q7                          @ GHASH block 4k+3 - mid

	eor	v11.16b, v11.16b, v28.16b                         @ GHASH block 4k+2 - low

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2
	eor	q9, q9, q4                         @ GHASH block 4k+3 - high

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2
	eor	r23, r23, r13                   @ AES block 4k+3 - round 10 low

	pmull	v30.1q, v30.1d, v16.1d                          @ GHASH block 4k+3 - mid
	eor	r21, r21, r13                   @ AES block 4k+2 - round 10 low
	eor	v11.16b, v11.16b, v29.16b                         @ GHASH block 4k+3 - low

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3
	shl	d8, d8, #56               @ mod_constant

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4
	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+3 - mid

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3
	eor	v30.16b, v11.16b, q9                         @ MODULO - karatsuba tidy up

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4
	eor	v10.16b, v10.16b, v30.16b                         @ MODULO - karatsuba tidy up

	pmull	v31.1q, q9, q8            @ MODULO - top 64b align with mid

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5
	eor	v10.16b, v10.16b, v31.16b                      @ MODULO - fold into mid

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8
	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7

	aese	q1, v27.16b                                      @ AES block 4k+5 - round 9

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low
	eor	r24, r24, r14                   @ AES block 4k+3 - round 10 high

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7
	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8
	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8
	eor	r22, r22, r14                   @ AES block 4k+2 - round 10 high

	aese	q0, v27.16b                                      @ AES block 4k+4 - round 9
	stp	r21, r22, [r2], #16        @ AES block 4k+2 - store result

	aese	q2, v27.16b                                      @ AES block 4k+6 - round 9
	add	r12, r12, #1                            @ CTR block 4k+7
	stp	r23, r24, [r2], #16        @ AES block 4k+3 - store result

	aese	q3, v27.16b                                      @ AES block 4k+7 - round 9
	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
L128_dec_tail:@ TAIL

	sub	r5, r4, r0   @ main_end_input_ptr is number of bytes left to process
	ld1	{ q5}, [r0], #16                      @ AES block 4k+4 - load ciphertext

	eor	q0, q5, q0                            @ AES block 4k+4 - result

	mov	r7, v0.d[1]                            @ AES block 4k+4 - mov high

	mov	r6, v0.d[0]                            @ AES block 4k+4 - mov low

	cmp	r5, #48

	eor	r7, r7, r14                   @ AES block 4k+4 - round 10 high

	ext	q8, v11.16b, v11.16b, #8                     @ prepare final partial tag
	eor	r6, r6, r13                   @ AES block 4k+4 - round 10 low
	bgt	L128_dec_blocks_more_than_3

	mov	q3, q2
	sub	r12, r12, #1
	movi	v11.8b, #0

	movi	q9, #0
	mov	q2, q1

	movi	v10.8b, #0
	cmp	r5, #32
	bgt	L128_dec_blocks_more_than_2

	cmp	r5, #16

	mov	q3, q1
	sub	r12, r12, #1
	bgt	L128_dec_blocks_more_than_1

	sub	r12, r12, #1
	b	L128_dec_blocks_less_than_1
L128_dec_blocks_more_than_3:@ blocks left >  3
	rev64	q4, q5                                    @ GHASH final-3 block
	ld1	{ q5}, [r0], #16                      @ AES final-2 block - load ciphertext

	eor	q4, q4, q8                           @ feed in partial tag

	mov	d10, v17.d[1]                               @ GHASH final-3 block - mid
	stp	r6, r7, [r2], #16        @ AES final-3 block  - store result
	eor	q0, q5, q1                            @ AES final-2 block - result

	mov	d22, v4.d[1]                                 @ GHASH final-3 block - mid
	mov	r7, v0.d[1]                            @ AES final-2 block - mov high

	pmull	v11.1q, q4, v15.1d                       @ GHASH final-3 block - low
	mov	r6, v0.d[0]                            @ AES final-2 block - mov low

	pmull2	v9.1q, q4, v15.2d                       @ GHASH final-3 block - high

	eor	v22.8b, v22.8b, q4                      @ GHASH final-3 block - mid

	movi	q8, #0                                        @ suppress further partial tag feed in
	eor	r7, r7, r14                   @ AES final-2 block - round 10 high

	pmull	v10.1q, v22.1d, v10.1d                    @ GHASH final-3 block - mid
	eor	r6, r6, r13                   @ AES final-2 block - round 10 low
L128_dec_blocks_more_than_2:@ blocks left >  2

	rev64	q4, q5                                    @ GHASH final-2 block
	ld1	{ q5}, [r0], #16                      @ AES final-1 block - load ciphertext

	eor	q4, q4, q8                           @ feed in partial tag

	eor	q0, q5, q2                            @ AES final-1 block - result
	stp	r6, r7, [r2], #16        @ AES final-2 block  - store result

	mov	d22, v4.d[1]                                 @ GHASH final-2 block - mid

	pmull	v21.1q, q4, v14.1d                          @ GHASH final-2 block - low

	pmull2	v20.1q, q4, v14.2d                          @ GHASH final-2 block - high
	mov	r6, v0.d[0]                            @ AES final-1 block - mov low

	mov	r7, v0.d[1]                            @ AES final-1 block - mov high
	eor	v22.8b, v22.8b, q4                      @ GHASH final-2 block - mid

	movi	q8, #0                                        @ suppress further partial tag feed in

	pmull	v22.1q, v22.1d, v17.1d                      @ GHASH final-2 block - mid

	eor	r6, r6, r13                   @ AES final-1 block - round 10 low
	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final-2 block - low

	eor	q9, q9, v20.16b                            @ GHASH final-2 block - high

	eor	v10.16b, v10.16b, v22.16b                       @ GHASH final-2 block - mid
	eor	r7, r7, r14                   @ AES final-1 block - round 10 high
L128_dec_blocks_more_than_1:@ blocks left >  1

	rev64	q4, q5                                    @ GHASH final-1 block

	ld1	{ q5}, [r0], #16                      @ AES final block - load ciphertext
	eor	q4, q4, q8                           @ feed in partial tag

	mov	d22, v4.d[1]                                 @ GHASH final-1 block - mid

	eor	q0, q5, q3                            @ AES final block - result

	eor	v22.8b, v22.8b, q4                      @ GHASH final-1 block - mid

	stp	r6, r7, [r2], #16        @ AES final-1 block  - store result
	mov	r6, v0.d[0]                            @ AES final block - mov low

	mov	r7, v0.d[1]                            @ AES final block - mov high
	ins	v22.d[1], v22.d[0]                            @ GHASH final-1 block - mid

	pmull	v21.1q, q4, v13.1d                          @ GHASH final-1 block - low

	pmull2	v20.1q, q4, v13.2d                          @ GHASH final-1 block - high

	pmull2	v22.1q, v22.2d, v16.2d                      @ GHASH final-1 block - mid
	movi	q8, #0                                        @ suppress further partial tag feed in

	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final-1 block - low

	eor	q9, q9, v20.16b                            @ GHASH final-1 block - high
	eor	r7, r7, r14                   @ AES final block - round 10 high

	eor	r6, r6, r13                   @ AES final block - round 10 low
	eor	v10.16b, v10.16b, v22.16b                       @ GHASH final-1 block - mid
L128_dec_blocks_less_than_1:@ blocks left <= 1

	mvn	r14, xzr                                      @ rk10_h = 0xffffffffffffffff
	and	r1, r1, #127                    @ bit_length %= 128

	mvn	r13, xzr                                      @ rk10_l = 0xffffffffffffffff
	sub	r1, r1, #128                    @ bit_length -= 128

	neg	r1, r1                          @ bit_length = 128 - #bits in input (in range [1,128])

	and	r1, r1, #127                    @ bit_length %= 128

	lsr	r14, r14, r1                     @ rk10_h is mask for top 64b of last block
	cmp	r1, #64

	csel	r10, r14, xzr, lt
	csel	r9, r13, r14, lt

	fmov	d0, r9                                   @ ctr0b is mask for last block

	mov	v0.d[1], r10

	and	q5, q5, q0                            @ possibly partial last block has zeroes in highest bits

	rev64	q4, q5                                    @ GHASH final block

	eor	q4, q4, q8                           @ feed in partial tag

	ldp	r4, r5, [r2] @ load existing bytes we need to not overwrite

	and	r7, r7, r10

	pmull2	v20.1q, q4, v12.2d                          @ GHASH final block - high
	mov	d8, v4.d[1]                                  @ GHASH final block - mid

	eor	q8, q8, q4                          @ GHASH final block - mid
	eor	q9, q9, v20.16b                            @ GHASH final block - high

	pmull	v8.1q, q8, v16.1d                          @ GHASH final block - mid

	pmull	v21.1q, q4, v12.1d                          @ GHASH final block - low
	bic	r4, r4, r9           @ mask out low existing bytes
	and	r6, r6, r9

	rev	r9, r12

	eor	v10.16b, v10.16b, q8                         @ GHASH final block - mid
	movi	q8, #0xc2

	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final block - low

	bic	r5, r5, r10   @ mask out high existing bytes
	shl	d8, d8, #56               @ mod_constant

	eor	v30.16b, v11.16b, q9                         @ MODULO - karatsuba tidy up

	pmull	v31.1q, q9, q8            @ MODULO - top 64b align with mid

	eor	v10.16b, v10.16b, v30.16b                         @ MODULO - karatsuba tidy up

	orr	r6, r6, r4
	str	r9, [r16, #12]                          @ store the updated counter

	orr	r7, r7, r5
	stp	r6, r7, [r2]
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	eor	v10.16b, v10.16b, v31.16b                      @ MODULO - fold into mid

	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low
	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low

	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b
	mov	r0, r15
	st1	{ v11.16b }, [r3]

	ldp	r21, r22, [sp, #16]
	ldp	r23, r24, [sp, #32]
	ldp	d8, d9, [sp, #48]
	ldp	d10, d11, [sp, #64]
	ldp	d12, d13, [sp, #80]
	ldp	d14, d15, [sp, #96]
	ldp	r19, r20, [sp], #112
	bx	lr

L128_dec_ret:
	mov	r0, #0x0
	bx	lr

.globl	aes_gcm_enc_192_kernel

.align	4
aes_gcm_enc_192_kernel:
	cbz	r1, L192_enc_ret
	stp	r19, r20, [sp, #-112]!
	mov	r16, r4
	mov	r8, r5
	stp	r21, r22, [sp, #16]
	stp	r23, r24, [sp, #32]
	stp	d8, d9, [sp, #48]
	stp	d10, d11, [sp, #64]
	stp	d12, d13, [sp, #80]
	stp	d14, d15, [sp, #96]

	ldp	r10, r11, [r16]             @ ctr96_b64, ctr96_t32

	ldr	q23, [r8, #80]                                @ load rk5

	ldr	q22, [r8, #64]                                @ load rk4

	ldr	q26, [r8, #128]                               @ load rk8

	lsr	r12, r11, #32
	ldr	q24, [r8, #96]                                @ load rk6
	orr	r11, r11, r11

	ldr	q25, [r8, #112]                               @ load rk7
	rev	r12, r12                               @ rev_ctr32

	add	r12, r12, #1                           @ increment rev_ctr32
	fmov	d3, r10                              @ CTR block 3

	rev	r9, r12                                @ CTR block 1
	add	r12, r12, #1                           @ CTR block 1
	fmov	d1, r10                              @ CTR block 1

	orr	r9, r11, r9, lsl #32           @ CTR block 1
	ld1	{ q0}, [r16]                            @ special case vector load initial counter so we can start first AES block as quickly as possible

	fmov	v1.d[1], r9                              @ CTR block 1
	rev	r9, r12                                @ CTR block 2
	add	r12, r12, #1                           @ CTR block 2

	fmov	d2, r10                              @ CTR block 2
	orr	r9, r11, r9, lsl #32           @ CTR block 2

	fmov	v2.d[1], r9                              @ CTR block 2
	rev	r9, r12                                @ CTR block 3

	orr	r9, r11, r9, lsl #32           @ CTR block 3
	ldr	q18, [r8, #0]                                 @ load rk0

	fmov	v3.d[1], r9                              @ CTR block 3

	ldr	q21, [r8, #48]                                @ load rk3

	ldp	r13, r14, [r8, #192]                    @ load rk12

	ldr	q19, [r8, #16]                                @ load rk1

	aese	q0, v18.16b
	aesmc	q0, q0         @ AES block 0 - round 0
	ld1	{ v11.16b}, [r3]
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b

	aese	q3, v18.16b
	aesmc	q3, q3         @ AES block 3 - round 0
	ldr	q29, [r8, #176]                              @ load rk11

	aese	q1, v18.16b
	aesmc	q1, q1         @ AES block 1 - round 0
	ldr	q15, [r3, #112]                       @ load h4l | h4h
	ext	v15.16b, v15.16b, v15.16b, #8

	aese	q2, v18.16b
	aesmc	q2, q2         @ AES block 2 - round 0
	ldr	q20, [r8, #32]                                @ load rk2

	aese	q0, v19.16b
	aesmc	q0, q0         @ AES block 0 - round 1
	ldr	q28, [r8, #160]                              @ load rk10

	aese	q1, v19.16b
	aesmc	q1, q1         @ AES block 1 - round 1
	ldr	q12, [r3, #32]                        @ load h1l | h1h
	ext	v12.16b, v12.16b, v12.16b, #8

	aese	q2, v19.16b
	aesmc	q2, q2         @ AES block 2 - round 1
	ldr	q27, [r8, #144]                               @ load rk9

	aese	q3, v19.16b
	aesmc	q3, q3         @ AES block 3 - round 1
	ldr	q14, [r3, #80]                        @ load h3l | h3h
	ext	v14.16b, v14.16b, v14.16b, #8

	aese	q0, v20.16b
	aesmc	q0, q0         @ AES block 0 - round 2

	aese	q2, v20.16b
	aesmc	q2, q2         @ AES block 2 - round 2

	aese	q3, v20.16b
	aesmc	q3, q3         @ AES block 3 - round 2

	aese	q0, v21.16b
	aesmc	q0, q0         @ AES block 0 - round 3
	trn1	q9, v14.2d,    v15.2d                     @ h4h | h3h

	aese	q2, v21.16b
	aesmc	q2, q2         @ AES block 2 - round 3

	aese	q1, v20.16b
	aesmc	q1, q1         @ AES block 1 - round 2
	trn2	v17.2d,  v14.2d,    v15.2d                     @ h4l | h3l

	aese	q0, v22.16b
	aesmc	q0, q0         @ AES block 0 - round 4

	aese	q3, v21.16b
	aesmc	q3, q3         @ AES block 3 - round 3

	aese	q1, v21.16b
	aesmc	q1, q1         @ AES block 1 - round 3

	aese	q0, v23.16b
	aesmc	q0, q0         @ AES block 0 - round 5

	aese	q2, v22.16b
	aesmc	q2, q2         @ AES block 2 - round 4

	aese	q1, v22.16b
	aesmc	q1, q1         @ AES block 1 - round 4

	aese	q0, v24.16b
	aesmc	q0, q0         @ AES block 0 - round 6

	aese	q3, v22.16b
	aesmc	q3, q3         @ AES block 3 - round 4

	aese	q2, v23.16b
	aesmc	q2, q2         @ AES block 2 - round 5

	aese	q1, v23.16b
	aesmc	q1, q1         @ AES block 1 - round 5

	aese	q3, v23.16b
	aesmc	q3, q3         @ AES block 3 - round 5

	aese	q2, v24.16b
	aesmc	q2, q2         @ AES block 2 - round 6
	ldr	q13, [r3, #64]                        @ load h2l | h2h
	ext	v13.16b, v13.16b, v13.16b, #8

	aese	q1, v24.16b
	aesmc	q1, q1         @ AES block 1 - round 6

	aese	q3, v24.16b
	aesmc	q3, q3         @ AES block 3 - round 6

	aese	q0, v25.16b
	aesmc	q0, q0         @ AES block 0 - round 7

	aese	q1, v25.16b
	aesmc	q1, q1         @ AES block 1 - round 7
	trn2	v16.2d,  v12.2d,    v13.2d                     @ h2l | h1l

	aese	q3, v25.16b
	aesmc	q3, q3         @ AES block 3 - round 7

	aese	q0, v26.16b
	aesmc	q0, q0         @ AES block 0 - round 8

	aese	q2, v25.16b
	aesmc	q2, q2         @ AES block 2 - round 7
	trn1	q8,    v12.2d,    v13.2d                     @ h2h | h1h

	aese	q1, v26.16b
	aesmc	q1, q1         @ AES block 1 - round 8

	aese	q3, v26.16b
	aesmc	q3, q3         @ AES block 3 - round 8

	aese	q2, v26.16b
	aesmc	q2, q2         @ AES block 2 - round 8

	aese	q0, v27.16b
	aesmc	q0, q0         @ AES block 0 - round 9

	aese	q3, v27.16b
	aesmc	q3, q3         @ AES block 3 - round 9

	aese	q2, v27.16b
	aesmc	q2, q2         @ AES block 2 - round 9

	aese	q1, v27.16b
	aesmc	q1, q1         @ AES block 1 - round 9

	aese	q0, v28.16b
	aesmc	q0, q0         @ AES block 0 - round 10

	aese	q2, v28.16b
	aesmc	q2, q2         @ AES block 2 - round 10

	aese	q1, v28.16b
	aesmc	q1, q1         @ AES block 1 - round 10
	lsr	r5, r1, #3             @ byte_len
	mov	r15, r5

	aese	q3, v28.16b
	aesmc	q3, q3         @ AES block 3 - round 10
	sub	r5, r5, #1     @ byte_len - 1

	eor	v16.16b, v16.16b, q8                    @ h2k | h1k
	and	r5, r5, #0xffffffffffffffc0   @ number of bytes to be processed in main loop (at least 1 byte must be handled by tail)

	eor	v17.16b, v17.16b, q9                 @ h4k | h3k

	aese	q2, v29.16b                                    @ AES block 2 - round 11
	add	r4, r0, r1, lsr #3  @ end_input_ptr
	add	r5, r5, r0

	aese	q1, v29.16b                                    @ AES block 1 - round 11
	cmp	r0, r5                  @ check if we have <= 4 blocks

	aese	q0, v29.16b                                    @ AES block 0 - round 11
	add	r12, r12, #1                           @ CTR block 3

	aese	q3, v29.16b                                    @ AES block 3 - round 11
	bge	L192_enc_tail                                   @ handle tail

	rev	r9, r12                                @ CTR block 4
	ldp	r6, r7, [r0, #0]           @ AES block 0 - load plaintext

	orr	r9, r11, r9, lsl #32           @ CTR block 4
	ldp	r21, r22, [r0, #32]          @ AES block 2 - load plaintext

	ldp	r23, r24, [r0, #48]          @ AES block 3 - load plaintext

	ldp	r19, r20, [r0, #16]          @ AES block 1 - load plaintext
	add	r0, r0, #64                      @ AES input_ptr update
	cmp	r0, r5                  @ check if we have <= 8 blocks

	eor	r6, r6, r13                    @ AES block 0 - round 12 low

	eor	r7, r7, r14                    @ AES block 0 - round 12 high
	eor	r22, r22, r14                    @ AES block 2 - round 12 high
	fmov	d4, r6                              @ AES block 0 - mov low

	eor	r24, r24, r14                    @ AES block 3 - round 12 high
	fmov	v4.d[1], r7                          @ AES block 0 - mov high

	eor	r21, r21, r13                    @ AES block 2 - round 12 low
	eor	r19, r19, r13                    @ AES block 1 - round 12 low

	fmov	d5, r19                              @ AES block 1 - mov low
	eor	r20, r20, r14                    @ AES block 1 - round 12 high

	fmov	v5.d[1], r20                          @ AES block 1 - mov high

	eor	r23, r23, r13                    @ AES block 3 - round 12 low
	fmov	d6, r21                              @ AES block 2 - mov low

	add	r12, r12, #1                           @ CTR block 4
	eor	q4, q4, q0                         @ AES block 0 - result
	fmov	d0, r10                              @ CTR block 4

	fmov	v0.d[1], r9                              @ CTR block 4
	rev	r9, r12                                @ CTR block 5

	orr	r9, r11, r9, lsl #32           @ CTR block 5
	add	r12, r12, #1                           @ CTR block 5

	fmov	d7, r23                              @ AES block 3 - mov low
	st1	{ q4}, [r2], #16                    @ AES block 0 - store result

	fmov	v6.d[1], r22                          @ AES block 2 - mov high

	eor	q5, q5, q1                         @ AES block 1 - result
	fmov	d1, r10                              @ CTR block 5
	st1	{ q5}, [r2], #16                    @ AES block 1 - store result

	fmov	v7.d[1], r24                          @ AES block 3 - mov high

	fmov	v1.d[1], r9                              @ CTR block 5
	rev	r9, r12                                @ CTR block 6

	orr	r9, r11, r9, lsl #32           @ CTR block 6

	add	r12, r12, #1                           @ CTR block 6
	eor	q6, q6, q2                         @ AES block 2 - result
	fmov	d2, r10                              @ CTR block 6

	fmov	v2.d[1], r9                              @ CTR block 6
	rev	r9, r12                                @ CTR block 7

	orr	r9, r11, r9, lsl #32           @ CTR block 7
	st1	{ q6}, [r2], #16                    @ AES block 2 - store result

	eor	q7, q7, q3                         @ AES block 3 - result
	st1	{ q7}, [r2], #16                    @ AES block 3 - store result
	bge	L192_enc_prepretail                             @ do prepretail

L192_enc_main_loop:@ main loop start
	aese	q2, v18.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 0
	rev64	q5, q5                                   @ GHASH block 4k+1 (t0 and t1 free)

	aese	q1, v18.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 0
	ldp	r19, r20, [r0, #16]          @ AES block 4k+5 - load plaintext

	ext	v11.16b, v11.16b, v11.16b, #8                    @ PRE 0
	fmov	d3, r10                              @ CTR block 4k+3
	rev64	q4, q4                                   @ GHASH block 4k (only t0 is free)

	aese	q2, v19.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 1
	fmov	v3.d[1], r9                              @ CTR block 4k+3

	pmull2	v30.1q, q5, v14.2d                         @ GHASH block 4k+1 - high
	rev64	q7, q7                                   @ GHASH block 4k+3 (t0, t1, t2 and t3 free)
	ldp	r21, r22, [r0, #32]          @ AES block 4k+6 - load plaintext

	aese	q0, v18.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 0
	ldp	r23, r24, [r0, #48]          @ AES block 4k+3 - load plaintext

	pmull	v31.1q, q5, v14.1d                         @ GHASH block 4k+1 - low
	eor	q4, q4, v11.16b                          @ PRE 1

	aese	q1, v19.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 1

	aese	q0, v19.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 1
	rev64	q6, q6                                   @ GHASH block 4k+2 (t0, t1, and t2 free)

	aese	q3, v18.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 0
	eor	r24, r24, r14                    @ AES block 4k+3 - round 12 high

	pmull	v11.1q, q4, v15.1d                      @ GHASH block 4k - low
	mov	d8, v4.d[1]                                 @ GHASH block 4k - mid

	aese	q0, v20.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 2

	aese	q3, v19.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 1
	eor	r21, r21, r13                    @ AES block 4k+6 - round 12 low

	eor	q8, q8, q4                         @ GHASH block 4k - mid
	eor	v11.16b, v11.16b, v31.16b                        @ GHASH block 4k+1 - low

	aese	q0, v21.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 3
	eor	r19, r19, r13                    @ AES block 4k+5 - round 12 low

	aese	q1, v20.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 2
	mov	d31, v6.d[1]                                 @ GHASH block 4k+2 - mid

	pmull2	v9.1q, q4, v15.2d                      @ GHASH block 4k - high
	mov	d4, v5.d[1]                                 @ GHASH block 4k+1 - mid

	aese	q2, v20.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 2

	aese	q1, v21.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 3

	mov	d10, v17.d[1]                              @ GHASH block 4k - mid
	eor	q9, q9, v30.16b                        @ GHASH block 4k+1 - high

	aese	q3, v20.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 2
	eor	v31.8b, v31.8b, q6                         @ GHASH block 4k+2 - mid

	pmull2	v30.1q, q6, v13.2d                         @ GHASH block 4k+2 - high

	aese	q0, v22.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 4
	eor	q4, q4, q5                         @ GHASH block 4k+1 - mid

	aese	q3, v21.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 3

	pmull2	v5.1q, q7, v12.2d                         @ GHASH block 4k+3 - high
	eor	r20, r20, r14                    @ AES block 4k+5 - round 12 high
	ins	v31.d[1], v31.d[0]                               @ GHASH block 4k+2 - mid

	aese	q0, v23.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 5
	add	r12, r12, #1                           @ CTR block 4k+3

	aese	q3, v22.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 4
	eor	q9, q9, v30.16b                        @ GHASH block 4k+2 - high

	pmull	v4.1q, q4, v17.1d                         @ GHASH block 4k+1 - mid
	eor	r22, r22, r14                    @ AES block 4k+6 - round 12 high

	pmull2	v31.1q, v31.2d, v16.2d                         @ GHASH block 4k+2 - mid
	eor	r23, r23, r13                    @ AES block 4k+3 - round 12 low
	mov	d30, v7.d[1]                                 @ GHASH block 4k+3 - mid

	pmull	v10.1q, q8, v10.1d                     @ GHASH block 4k - mid
	rev	r9, r12                                @ CTR block 4k+8

	pmull	v8.1q, q6, v13.1d                         @ GHASH block 4k+2 - low
	orr	r9, r11, r9, lsl #32           @ CTR block 4k+8

	aese	q2, v21.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 3
	eor	v30.8b, v30.8b, q7                         @ GHASH block 4k+3 - mid

	aese	q1, v22.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 4
	ldp	r6, r7, [r0, #0]           @ AES block 4k+4 - load plaintext

	aese	q0, v24.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 6
	eor	v11.16b, v11.16b, q8                        @ GHASH block 4k+2 - low

	aese	q2, v22.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 4
	add	r0, r0, #64                      @ AES input_ptr update

	aese	q1, v23.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 5
	movi	q8, #0xc2

	pmull	v6.1q, q7, v12.1d                         @ GHASH block 4k+3 - low
	eor	r7, r7, r14                    @ AES block 4k+4 - round 12 high
	eor	v10.16b, v10.16b, q4                        @ GHASH block 4k+1 - mid

	aese	q2, v23.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 5
	eor	r6, r6, r13                    @ AES block 4k+4 - round 12 low

	aese	q1, v24.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 6
	shl	d8, d8, #56              @ mod_constant

	aese	q3, v23.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 5
	eor	q9, q9, q5                        @ GHASH block 4k+3 - high

	aese	q0, v25.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 7
	fmov	d5, r19                              @ AES block 4k+5 - mov low

	aese	q1, v25.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 7
	eor	v10.16b, v10.16b, v31.16b                        @ GHASH block 4k+2 - mid

	aese	q3, v24.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 6
	fmov	v5.d[1], r20                          @ AES block 4k+5 - mov high

	aese	q0, v26.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 8
	eor	v11.16b, v11.16b, q6                        @ GHASH block 4k+3 - low

	pmull	v30.1q, v30.1d, v16.1d                         @ GHASH block 4k+3 - mid
	cmp	r0, r5                  @ LOOP CONTROL
	fmov	d4, r6                              @ AES block 4k+4 - mov low

	aese	q2, v24.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 6
	fmov	v4.d[1], r7                          @ AES block 4k+4 - mov high

	aese	q1, v26.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 8
	fmov	d7, r23                              @ AES block 4k+3 - mov low

	eor	v10.16b, v10.16b, v30.16b                        @ GHASH block 4k+3 - mid
	eor	v30.16b, v11.16b, q9                        @ MODULO - karatsuba tidy up
	add	r12, r12, #1                           @ CTR block 4k+8

	aese	q2, v25.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 7
	fmov	v7.d[1], r24                          @ AES block 4k+3 - mov high

	pmull	v31.1q, q9, q8           @ MODULO - top 64b align with mid
	ext	q9, q9, q9, #8                    @ MODULO - other top alignment
	fmov	d6, r21                              @ AES block 4k+6 - mov low

	aese	q3, v25.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 7

	aese	q0, v27.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 9
	eor	v10.16b, v10.16b, v30.16b                        @ MODULO - karatsuba tidy up

	aese	q2, v26.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 8

	aese	q3, v26.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 8

	aese	q1, v27.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 9

	aese	q0, v28.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 10
	eor	v10.16b, v10.16b, v31.16b                     @ MODULO - fold into mid

	aese	q3, v27.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 9

	aese	q2, v27.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 9

	aese	q0, v29.16b                                    @ AES block 4k+4 - round 11

	aese	q1, v28.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 10
	eor	v10.16b, v10.16b, q9                        @ MODULO - fold into mid

	aese	q2, v28.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 10

	eor	q4, q4, q0                         @ AES block 4k+4 - result
	fmov	d0, r10                              @ CTR block 4k+8

	aese	q1, v29.16b                                    @ AES block 4k+5 - round 11
	fmov	v0.d[1], r9                              @ CTR block 4k+8
	rev	r9, r12                                @ CTR block 4k+9

	pmull	v9.1q, v10.1d, q8           @ MODULO - mid 64b align with low
	fmov	v6.d[1], r22                          @ AES block 4k+6 - mov high
	st1	{ q4}, [r2], #16                    @ AES block 4k+4 - store result

	aese	q3, v28.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 10
	orr	r9, r11, r9, lsl #32           @ CTR block 4k+9

	eor	q5, q5, q1                         @ AES block 4k+5 - result
	add	r12, r12, #1                           @ CTR block 4k+9
	fmov	d1, r10                              @ CTR block 4k+9

	aese	q2, v29.16b                                    @ AES block 4k+6 - round 11
	fmov	v1.d[1], r9                              @ CTR block 4k+9
	rev	r9, r12                                @ CTR block 4k+10

	add	r12, r12, #1                           @ CTR block 4k+10
	ext	v10.16b, v10.16b, v10.16b, #8                    @ MODULO - other mid alignment
	orr	r9, r11, r9, lsl #32           @ CTR block 4k+10

	st1	{ q5}, [r2], #16                    @ AES block 4k+5 - store result
	eor	v11.16b, v11.16b, q9                        @ MODULO - fold into low

	aese	q3, v29.16b                                    @ AES block 4k+7 - round 11
	eor	q6, q6, q2                         @ AES block 4k+6 - result
	fmov	d2, r10                              @ CTR block 4k+10

	st1	{ q6}, [r2], #16                    @ AES block 4k+6 - store result
	fmov	v2.d[1], r9                              @ CTR block 4k+10
	rev	r9, r12                                @ CTR block 4k+11

	eor	v11.16b, v11.16b, v10.16b                        @ MODULO - fold into low
	orr	r9, r11, r9, lsl #32           @ CTR block 4k+11

	eor	q7, q7, q3                         @ AES block 4k+3 - result
	st1	{ q7}, [r2], #16                    @ AES block 4k+3 - store result
	blt	L192_enc_main_loop

L192_enc_prepretail:@ PREPRETAIL
	aese	q0, v18.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 0
	rev64	q4, q4                                   @ GHASH block 4k (only t0 is free)

	fmov	d3, r10                              @ CTR block 4k+3
	ext	v11.16b, v11.16b, v11.16b, #8                    @ PRE 0
	add	r12, r12, #1                           @ CTR block 4k+3

	aese	q1, v18.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 0
	rev64	q5, q5                                   @ GHASH block 4k+1 (t0 and t1 free)

	aese	q2, v18.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 0

	fmov	v3.d[1], r9                              @ CTR block 4k+3
	eor	q4, q4, v11.16b                          @ PRE 1
	mov	d10, v17.d[1]                              @ GHASH block 4k - mid

	aese	q1, v19.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 1
	rev64	q6, q6                                   @ GHASH block 4k+2 (t0, t1, and t2 free)

	pmull2	v30.1q, q5, v14.2d                         @ GHASH block 4k+1 - high

	pmull	v11.1q, q4, v15.1d                      @ GHASH block 4k - low
	mov	d8, v4.d[1]                                 @ GHASH block 4k - mid

	pmull	v31.1q, q5, v14.1d                         @ GHASH block 4k+1 - low
	rev64	q7, q7                                   @ GHASH block 4k+3 (t0, t1, t2 and t3 free)

	pmull2	v9.1q, q4, v15.2d                      @ GHASH block 4k - high

	eor	q8, q8, q4                         @ GHASH block 4k - mid
	mov	d4, v5.d[1]                                 @ GHASH block 4k+1 - mid

	eor	v11.16b, v11.16b, v31.16b                        @ GHASH block 4k+1 - low
	mov	d31, v6.d[1]                                 @ GHASH block 4k+2 - mid

	aese	q3, v18.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 0
	eor	q9, q9, v30.16b                        @ GHASH block 4k+1 - high

	pmull2	v30.1q, q6, v13.2d                         @ GHASH block 4k+2 - high

	eor	q4, q4, q5                         @ GHASH block 4k+1 - mid
	eor	v31.8b, v31.8b, q6                         @ GHASH block 4k+2 - mid

	aese	q3, v19.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 1

	aese	q2, v19.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 1
	eor	q9, q9, v30.16b                        @ GHASH block 4k+2 - high

	aese	q0, v19.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 1

	aese	q1, v20.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 2
	mov	d30, v7.d[1]                                 @ GHASH block 4k+3 - mid

	pmull2	v5.1q, q7, v12.2d                         @ GHASH block 4k+3 - high
	ins	v31.d[1], v31.d[0]                               @ GHASH block 4k+2 - mid

	aese	q0, v20.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 2

	pmull	v10.1q, q8, v10.1d                     @ GHASH block 4k - mid
	eor	v30.8b, v30.8b, q7                         @ GHASH block 4k+3 - mid

	aese	q1, v21.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 3

	pmull2	v31.1q, v31.2d, v16.2d                         @ GHASH block 4k+2 - mid

	pmull	v4.1q, q4, v17.1d                         @ GHASH block 4k+1 - mid

	pmull	v30.1q, v30.1d, v16.1d                         @ GHASH block 4k+3 - mid
	eor	q9, q9, q5                        @ GHASH block 4k+3 - high

	pmull	v8.1q, q6, v13.1d                         @ GHASH block 4k+2 - low

	aese	q0, v21.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 3
	eor	v10.16b, v10.16b, q4                        @ GHASH block 4k+1 - mid

	aese	q3, v20.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 2

	aese	q2, v20.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 2
	eor	v11.16b, v11.16b, q8                        @ GHASH block 4k+2 - low

	aese	q0, v22.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 4

	aese	q3, v21.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 3
	eor	v10.16b, v10.16b, v31.16b                        @ GHASH block 4k+2 - mid

	aese	q2, v21.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 3

	pmull	v6.1q, q7, v12.1d                         @ GHASH block 4k+3 - low
	movi	q8, #0xc2

	aese	q3, v22.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 4

	aese	q2, v22.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 4

	aese	q1, v22.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 4
	eor	v10.16b, v10.16b, v30.16b                        @ GHASH block 4k+3 - mid

	aese	q3, v23.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 5

	aese	q2, v23.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 5

	aese	q1, v23.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 5
	eor	v11.16b, v11.16b, q6                        @ GHASH block 4k+3 - low

	aese	q0, v23.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 5

	aese	q3, v24.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 6
	eor	v10.16b, v10.16b, q9                        @ karatsuba tidy up

	aese	q1, v24.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 6

	aese	q0, v24.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 6
	shl	d8, d8, #56              @ mod_constant

	aese	q3, v25.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 7

	aese	q1, v25.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 7
	eor	v10.16b, v10.16b, v11.16b

	aese	q0, v25.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 7

	pmull	v30.1q, q9, q8

	aese	q2, v24.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 6
	ext	q9, q9, q9, #8

	aese	q0, v26.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 8

	aese	q1, v26.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 8
	eor	v10.16b, v10.16b, v30.16b

	aese	q2, v25.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 7

	aese	q3, v26.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 8

	aese	q0, v27.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 9

	aese	q2, v26.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 8
	eor	v10.16b, v10.16b, q9

	aese	q3, v27.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 9

	aese	q1, v27.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 9

	aese	q2, v27.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 9

	pmull	v30.1q, v10.1d, q8

	ext	v10.16b, v10.16b, v10.16b, #8

	aese	q3, v28.16b
	aesmc	q3, q3         @ AES block 4k+7 - round 10

	aese	q0, v28.16b
	aesmc	q0, q0         @ AES block 4k+4 - round 10

	aese	q2, v28.16b
	aesmc	q2, q2         @ AES block 4k+6 - round 10

	aese	q1, v28.16b
	aesmc	q1, q1         @ AES block 4k+5 - round 10
	eor	v11.16b, v11.16b, v30.16b

	aese	q0, v29.16b                                    @ AES block 4k+4 - round 11

	aese	q3, v29.16b                                    @ AES block 4k+7 - round 11

	aese	q2, v29.16b                                    @ AES block 4k+6 - round 11

	aese	q1, v29.16b                                    @ AES block 4k+5 - round 11
	eor	v11.16b, v11.16b, v10.16b
L192_enc_tail:@ TAIL

	sub	r5, r4, r0  @ main_end_input_ptr is number of bytes left to process
	ldp	r6, r7, [r0], #16          @ AES block 4k+4 - load plaintext

	eor	r6, r6, r13                    @ AES block 4k+4 - round 12 low
	eor	r7, r7, r14                    @ AES block 4k+4 - round 12 high

	fmov	d4, r6                              @ AES block 4k+4 - mov low

	fmov	v4.d[1], r7                          @ AES block 4k+4 - mov high
	cmp	r5, #48

	eor	q5, q4, q0                         @ AES block 4k+4 - result

	ext	q8, v11.16b, v11.16b, #8                    @ prepare final partial tag
	bgt	L192_enc_blocks_more_than_3

	sub	r12, r12, #1
	movi	v10.8b, #0

	mov	q3, q2
	movi	q9, #0
	cmp	r5, #32

	mov	q2, q1
	movi	v11.8b, #0
	bgt	L192_enc_blocks_more_than_2

	sub	r12, r12, #1

	mov	q3, q1
	cmp	r5, #16
	bgt	L192_enc_blocks_more_than_1

	sub	r12, r12, #1
	b	L192_enc_blocks_less_than_1
L192_enc_blocks_more_than_3:@ blocks left >  3
	st1	{ q5}, [r2], #16                    @ AES final-3 block  - store result

	ldp	r6, r7, [r0], #16          @ AES final-2 block - load input low & high

	rev64	q4, q5                                   @ GHASH final-3 block

	eor	r6, r6, r13                    @ AES final-2 block - round 12 low
	eor	q4, q4, q8                          @ feed in partial tag

	eor	r7, r7, r14                    @ AES final-2 block - round 12 high
	fmov	d5, r6                                @ AES final-2 block - mov low

	fmov	v5.d[1], r7                            @ AES final-2 block - mov high

	mov	d22, v4.d[1]                                @ GHASH final-3 block - mid

	pmull	v11.1q, q4, v15.1d                      @ GHASH final-3 block - low

	mov	d10, v17.d[1]                              @ GHASH final-3 block - mid

	eor	v22.8b, v22.8b, q4                     @ GHASH final-3 block - mid

	movi	q8, #0                                       @ suppress further partial tag feed in

	pmull2	v9.1q, q4, v15.2d                      @ GHASH final-3 block - high

	pmull	v10.1q, v22.1d, v10.1d                   @ GHASH final-3 block - mid
	eor	q5, q5, q1                           @ AES final-2 block - result
L192_enc_blocks_more_than_2:@ blocks left >  2

	st1	{ q5}, [r2], #16                    @ AES final-2 block - store result

	rev64	q4, q5                                   @ GHASH final-2 block
	ldp	r6, r7, [r0], #16          @ AES final-1 block - load input low & high

	eor	q4, q4, q8                          @ feed in partial tag

	eor	r7, r7, r14                    @ AES final-1 block - round 12 high

	pmull2	v20.1q, q4, v14.2d                         @ GHASH final-2 block - high
	mov	d22, v4.d[1]                                @ GHASH final-2 block - mid

	pmull	v21.1q, q4, v14.1d                         @ GHASH final-2 block - low
	eor	r6, r6, r13                    @ AES final-1 block - round 12 low

	fmov	d5, r6                                @ AES final-1 block - mov low

	fmov	v5.d[1], r7                            @ AES final-1 block - mov high
	eor	q9, q9, v20.16b                           @ GHASH final-2 block - high
	eor	v22.8b, v22.8b, q4                     @ GHASH final-2 block - mid

	eor	v11.16b, v11.16b, v21.16b                           @ GHASH final-2 block - low

	pmull	v22.1q, v22.1d, v17.1d                     @ GHASH final-2 block - mid

	movi	q8, #0                                       @ suppress further partial tag feed in

	eor	q5, q5, q2                           @ AES final-1 block - result

	eor	v10.16b, v10.16b, v22.16b                      @ GHASH final-2 block - mid
L192_enc_blocks_more_than_1:@ blocks left >  1

	st1	{ q5}, [r2], #16                    @ AES final-1 block - store result

	ldp	r6, r7, [r0], #16          @ AES final block - load input low & high

	rev64	q4, q5                                   @ GHASH final-1 block

	eor	r6, r6, r13                    @ AES final block - round 12 low
	eor	q4, q4, q8                          @ feed in partial tag
	movi	q8, #0                                       @ suppress further partial tag feed in

	mov	d22, v4.d[1]                                @ GHASH final-1 block - mid

	eor	v22.8b, v22.8b, q4                     @ GHASH final-1 block - mid
	eor	r7, r7, r14                    @ AES final block - round 12 high
	fmov	d5, r6                                @ AES final block - mov low

	pmull2	v20.1q, q4, v13.2d                         @ GHASH final-1 block - high
	fmov	v5.d[1], r7                            @ AES final block - mov high

	ins	v22.d[1], v22.d[0]                           @ GHASH final-1 block - mid

	eor	q9, q9, v20.16b                           @ GHASH final-1 block - high

	pmull	v21.1q, q4, v13.1d                         @ GHASH final-1 block - low

	pmull2	v22.1q, v22.2d, v16.2d                     @ GHASH final-1 block - mid

	eor	q5, q5, q3                           @ AES final block - result

	eor	v11.16b, v11.16b, v21.16b                           @ GHASH final-1 block - low

	eor	v10.16b, v10.16b, v22.16b                      @ GHASH final-1 block - mid
L192_enc_blocks_less_than_1:@ blocks left <= 1

	ld1	{ v18.16b}, [r2]                           @ load existing bytes where the possibly partial last block is to be stored
	rev	r9, r12
	and	r1, r1, #127                   @ bit_length %= 128

	sub	r1, r1, #128                   @ bit_length -= 128
	mvn	r14, xzr                                     @ rk12_h = 0xffffffffffffffff

	neg	r1, r1                         @ bit_length = 128 - #bits in input (in range [1,128])
	mvn	r13, xzr                                     @ rk12_l = 0xffffffffffffffff

	and	r1, r1, #127                   @ bit_length %= 128

	lsr	r14, r14, r1                    @ rk12_h is mask for top 64b of last block
	cmp	r1, #64

	csel	r6, r13, r14, lt
	csel	r7, r14, xzr, lt

	fmov	d0, r6                                @ ctr0b is mask for last block

	fmov	v0.d[1], r7

	and	q5, q5, q0                           @ possibly partial last block has zeroes in highest bits

	rev64	q4, q5                                   @ GHASH final block

	eor	q4, q4, q8                          @ feed in partial tag

	mov	d8, v4.d[1]                                 @ GHASH final block - mid

	pmull	v21.1q, q4, v12.1d                         @ GHASH final block - low

	pmull2	v20.1q, q4, v12.2d                         @ GHASH final block - high

	eor	q8, q8, q4                         @ GHASH final block - mid

	eor	v11.16b, v11.16b, v21.16b                           @ GHASH final block - low

	eor	q9, q9, v20.16b                           @ GHASH final block - high

	pmull	v8.1q, q8, v16.1d                         @ GHASH final block - mid

	eor	v10.16b, v10.16b, q8                        @ GHASH final block - mid
	movi	q8, #0xc2

	eor	v30.16b, v11.16b, q9                        @ MODULO - karatsuba tidy up

	shl	d8, d8, #56              @ mod_constant

	bif	q5, v18.16b, q0                             @ insert existing bytes in top end of result before storing

	eor	v10.16b, v10.16b, v30.16b                        @ MODULO - karatsuba tidy up

	pmull	v31.1q, q9, q8           @ MODULO - top 64b align with mid

	ext	q9, q9, q9, #8                    @ MODULO - other top alignment

	eor	v10.16b, v10.16b, v31.16b                     @ MODULO - fold into mid

	eor	v10.16b, v10.16b, q9                        @ MODULO - fold into mid

	pmull	v9.1q, v10.1d, q8           @ MODULO - mid 64b align with low

	ext	v10.16b, v10.16b, v10.16b, #8                    @ MODULO - other mid alignment

	eor	v11.16b, v11.16b, q9                        @ MODULO - fold into low
	str	r9, [r16, #12]                         @ store the updated counter

	st1	{ q5}, [r2]                         @ store all 16B

	eor	v11.16b, v11.16b, v10.16b                        @ MODULO - fold into low
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b
	mov	r0, r15
	st1	{ v11.16b }, [r3]

	ldp	r21, r22, [sp, #16]
	ldp	r23, r24, [sp, #32]
	ldp	d8, d9, [sp, #48]
	ldp	d10, d11, [sp, #64]
	ldp	d12, d13, [sp, #80]
	ldp	d14, d15, [sp, #96]
	ldp	r19, r20, [sp], #112
	bx	lr

L192_enc_ret:
	mov	r0, #0x0
	bx	lr

.globl	aes_gcm_dec_192_kernel

.align	4
aes_gcm_dec_192_kernel:
	cbz	r1, L192_dec_ret
	stp	r19, r20, [sp, #-112]!
	mov	r16, r4
	mov	r8, r5
	stp	r21, r22, [sp, #16]
	stp	r23, r24, [sp, #32]
	stp	d8, d9, [sp, #48]
	stp	d10, d11, [sp, #64]
	stp	d12, d13, [sp, #80]
	stp	d14, d15, [sp, #96]

	add	r4, r0, r1, lsr #3   @ end_input_ptr
	ldp	r10, r11, [r16]              @ ctr96_b64, ctr96_t32

	ld1	{ q0}, [r16]                             @ special case vector load initial counter so we can start first AES block as quickly as possible

	ldr	q18, [r8, #0]                                  @ load rk0

	lsr	r5, r1, #3              @ byte_len
	mov	r15, r5
	ldr	q20, [r8, #32]                                 @ load rk2

	lsr	r12, r11, #32
	orr	r11, r11, r11
	fmov	d3, r10                               @ CTR block 3

	rev	r12, r12                                @ rev_ctr32
	fmov	d1, r10                               @ CTR block 1

	add	r12, r12, #1                            @ increment rev_ctr32
	ldr	q19, [r8, #16]                                 @ load rk1

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 0 - round 0
	rev	r9, r12                                 @ CTR block 1

	add	r12, r12, #1                            @ CTR block 1
	orr	r9, r11, r9, lsl #32            @ CTR block 1
	ldr	q21, [r8, #48]                                 @ load rk3

	fmov	v1.d[1], r9                               @ CTR block 1
	rev	r9, r12                                 @ CTR block 2
	add	r12, r12, #1                            @ CTR block 2

	fmov	d2, r10                               @ CTR block 2
	orr	r9, r11, r9, lsl #32            @ CTR block 2

	fmov	v2.d[1], r9                               @ CTR block 2
	rev	r9, r12                                 @ CTR block 3

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 0 - round 1
	orr	r9, r11, r9, lsl #32            @ CTR block 3

	fmov	v3.d[1], r9                               @ CTR block 3

	ldr	q26, [r8, #128]                                @ load rk8

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 0 - round 2

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 2 - round 0
	ldr	q29, [r8, #176]                               @ load rk11

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 1 - round 0
	ldr	q15, [r3, #112]                        @ load h4l | h4h
	ext	v15.16b, v15.16b, v15.16b, #8

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 3 - round 0
	ldr	q13, [r3, #64]                         @ load h2l | h2h
	ext	v13.16b, v13.16b, v13.16b, #8

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 2 - round 1
	ldr	q14, [r3, #80]                         @ load h3l | h3h
	ext	v14.16b, v14.16b, v14.16b, #8

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 1 - round 1
	ldp	r13, r14, [r8, #192]                     @ load rk12

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 3 - round 1
	ldr	q12, [r3, #32]                         @ load h1l | h1h
	ext	v12.16b, v12.16b, v12.16b, #8

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 2 - round 2
	ldr	q28, [r8, #160]                               @ load rk10

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 0 - round 3
	ldr	q27, [r8, #144]                                @ load rk9

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 1 - round 2
	ldr	q25, [r8, #112]                                @ load rk7

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 3 - round 2
	ldr	q22, [r8, #64]                                 @ load rk4

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 2 - round 3
	ld1	{ v11.16b}, [r3]
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 1 - round 3
	add	r12, r12, #1                            @ CTR block 3

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 3 - round 3
	trn1	q9, v14.2d,    v15.2d                      @ h4h | h3h

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 0 - round 4
	ldr	q23, [r8, #80]                                 @ load rk5

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 1 - round 4
	trn2	v17.2d,  v14.2d,    v15.2d                      @ h4l | h3l

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 2 - round 4

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 3 - round 4
	trn2	v16.2d,  v12.2d,    v13.2d                      @ h2l | h1l

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 0 - round 5
	ldr	q24, [r8, #96]                                 @ load rk6

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 1 - round 5

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 2 - round 5

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 3 - round 5

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 0 - round 6

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 2 - round 6

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 3 - round 6

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 0 - round 7

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 2 - round 7

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 3 - round 7

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 1 - round 6

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 2 - round 8

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 3 - round 8

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 1 - round 7

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 2 - round 9

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 3 - round 9

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 1 - round 8
	sub	r5, r5, #1      @ byte_len - 1

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 0 - round 8
	and	r5, r5, #0xffffffffffffffc0    @ number of bytes to be processed in main loop (at least 1 byte must be handled by tail)

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 3 - round 10
	add	r5, r5, r0

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 1 - round 9
	cmp	r0, r5                   @ check if we have <= 4 blocks

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 0 - round 9
	trn1	q8,    v12.2d,    v13.2d                      @ h2h | h1h

	aese	q3, v29.16b                                     @ AES block 3 - round 11

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 2 - round 10

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 1 - round 10

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 0 - round 10
	eor	v16.16b, v16.16b, q8                     @ h2k | h1k

	aese	q2, v29.16b                                     @ AES block 2 - round 11

	aese	q1, v29.16b                                     @ AES block 1 - round 11
	eor	v17.16b, v17.16b, q9                  @ h4k | h3k

	aese	q0, v29.16b                                     @ AES block 0 - round 11
	bge	L192_dec_tail                                    @ handle tail

	ldr	q5, [r0, #16]                         @ AES block 1 - load ciphertext

	ldr	q4, [r0, #0]                          @ AES block 0 - load ciphertext

	eor	q1, q5, q1                            @ AES block 1 - result

	eor	q0, q4, q0                            @ AES block 0 - result
	rev	r9, r12                                 @ CTR block 4
	ldr	q7, [r0, #48]                         @ AES block 3 - load ciphertext

	ldr	q6, [r0, #32]                         @ AES block 2 - load ciphertext

	mov	r19, v1.d[0]                            @ AES block 1 - mov low

	mov	r20, v1.d[1]                            @ AES block 1 - mov high

	mov	r6, v0.d[0]                            @ AES block 0 - mov low
	orr	r9, r11, r9, lsl #32            @ CTR block 4
	add	r12, r12, #1                            @ CTR block 4

	mov	r7, v0.d[1]                            @ AES block 0 - mov high
	rev64	q4, q4                                    @ GHASH block 0
	add	r0, r0, #64                       @ AES input_ptr update

	fmov	d0, r10                               @ CTR block 4
	rev64	q5, q5                                    @ GHASH block 1
	cmp	r0, r5                   @ check if we have <= 8 blocks

	eor	r19, r19, r13                   @ AES block 1 - round 12 low
	fmov	v0.d[1], r9                               @ CTR block 4
	rev	r9, r12                                 @ CTR block 5

	orr	r9, r11, r9, lsl #32            @ CTR block 5
	fmov	d1, r10                               @ CTR block 5
	eor	r20, r20, r14                   @ AES block 1 - round 12 high

	add	r12, r12, #1                            @ CTR block 5
	fmov	v1.d[1], r9                               @ CTR block 5
	eor	r6, r6, r13                   @ AES block 0 - round 12 low

	rev	r9, r12                                 @ CTR block 6
	eor	r7, r7, r14                   @ AES block 0 - round 12 high

	stp	r6, r7, [r2], #16        @ AES block 0 - store result
	orr	r9, r11, r9, lsl #32            @ CTR block 6

	stp	r19, r20, [r2], #16        @ AES block 1 - store result

	add	r12, r12, #1                            @ CTR block 6
	eor	q2, q6, q2                            @ AES block 2 - result
	bge	L192_dec_prepretail                              @ do prepretail

L192_dec_main_loop:@ main loop start
	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0

	pmull	v31.1q, q5, v14.1d                          @ GHASH block 4k+1 - low
	mov	r21, v2.d[0]                            @ AES block 4k+2 - mov low

	mov	r22, v2.d[1]                            @ AES block 4k+2 - mov high
	eor	q3, q7, q3                            @ AES block 4k+3 - result
	rev64	q7, q7                                    @ GHASH block 4k+3

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	fmov	d2, r10                               @ CTR block 4k+6

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	eor	q4, q4, v11.16b                           @ PRE 1

	pmull2	v30.1q, q5, v14.2d                          @ GHASH block 4k+1 - high
	fmov	v2.d[1], r9                               @ CTR block 4k+6

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2
	mov	r24, v3.d[1]                            @ AES block 4k+3 - mov high

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	mov	r23, v3.d[0]                            @ AES block 4k+3 - mov low

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high
	fmov	d3, r10                               @ CTR block 4k+7
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid
	rev	r9, r12                                 @ CTR block 4k+7

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+7

	fmov	v3.d[1], r9                               @ CTR block 4k+7
	eor	q8, q8, q4                          @ GHASH block 4k - mid
	mov	d4, v5.d[1]                                  @ GHASH block 4k+1 - mid

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2
	eor	r22, r22, r14                   @ AES block 4k+2 - round 12 high

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	eor	q4, q4, q5                          @ GHASH block 4k+1 - mid

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	rev64	q6, q6                                    @ GHASH block 4k+2

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2

	pmull	v4.1q, q4, v17.1d                          @ GHASH block 4k+1 - mid
	eor	v11.16b, v11.16b, v31.16b                         @ GHASH block 4k+1 - low
	eor	r21, r21, r13                   @ AES block 4k+2 - round 12 low

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3

	eor	v10.16b, v10.16b, q4                         @ GHASH block 4k+1 - mid
	mov	d31, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1
	eor	q9, q9, v30.16b                         @ GHASH block 4k+1 - high

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4

	pmull2	v30.1q, q6, v13.2d                          @ GHASH block 4k+2 - high
	eor	v31.8b, v31.8b, q6                          @ GHASH block 4k+2 - mid

	pmull	v8.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5

	eor	q9, q9, v30.16b                         @ GHASH block 4k+2 - high
	mov	d30, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5

	pmull2	v5.1q, q7, v12.2d                          @ GHASH block 4k+3 - high

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2
	eor	v30.8b, v30.8b, q7                          @ GHASH block 4k+3 - mid

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6
	ins	v31.d[1], v31.d[0]                                @ GHASH block 4k+2 - mid

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3

	pmull	v30.1q, v30.1d, v16.1d                          @ GHASH block 4k+3 - mid
	eor	v11.16b, v11.16b, q8                         @ GHASH block 4k+2 - low

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7

	pmull2	v31.1q, v31.2d, v16.2d                          @ GHASH block 4k+2 - mid
	eor	q9, q9, q5                         @ GHASH block 4k+3 - high

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8
	movi	q8, #0xc2

	pmull	v6.1q, q7, v12.1d                          @ GHASH block 4k+3 - low

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8
	eor	v10.16b, v10.16b, v31.16b                         @ GHASH block 4k+2 - mid

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 9
	eor	v11.16b, v11.16b, q6                         @ GHASH block 4k+3 - low

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4
	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+3 - mid

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 10

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 9
	eor	v30.16b, v11.16b, q9                         @ MODULO - karatsuba tidy up

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5
	shl	d8, d8, #56               @ mod_constant

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 10

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6
	ldr	q6, [r0, #32]                         @ AES block 4k+6 - load ciphertext

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6
	eor	v10.16b, v10.16b, v30.16b                         @ MODULO - karatsuba tidy up

	pmull	v31.1q, q9, q8            @ MODULO - top 64b align with mid
	ldr	q7, [r0, #48]                         @ AES block 4k+7 - load ciphertext
	eor	r23, r23, r13                   @ AES block 4k+3 - round 12 low

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	aese	q0, v29.16b                                     @ AES block 4k+4 - round 11
	add	r12, r12, #1                            @ CTR block 4k+7

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7
	eor	v10.16b, v10.16b, v31.16b                      @ MODULO - fold into mid

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8
	ldr	q4, [r0, #0]                          @ AES block 4k+4 - load ciphertext

	aese	q1, v29.16b                                     @ AES block 4k+5 - round 11
	ldr	q5, [r0, #16]                         @ AES block 4k+5 - load ciphertext
	rev	r9, r12                                 @ CTR block 4k+8

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8
	stp	r21, r22, [r2], #16        @ AES block 4k+2 - store result

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 9
	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	add	r0, r0, #64                       @ AES input_ptr update
	cmp	r0, r5                   @ LOOP CONTROL

	eor	q0, q4, q0                            @ AES block 4k+4 - result
	eor	r24, r24, r14                   @ AES block 4k+3 - round 12 high
	eor	q1, q5, q1                            @ AES block 4k+5 - result

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 10
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+8

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 9

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low
	mov	r19, v1.d[0]                            @ AES block 4k+5 - mov low

	mov	r6, v0.d[0]                            @ AES block 4k+4 - mov low
	stp	r23, r24, [r2], #16        @ AES block 4k+3 - store result
	rev64	q5, q5                                    @ GHASH block 4k+5

	aese	q2, v29.16b                                     @ AES block 4k+6 - round 11
	mov	r7, v0.d[1]                            @ AES block 4k+4 - mov high

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 10
	mov	r20, v1.d[1]                            @ AES block 4k+5 - mov high

	fmov	d0, r10                               @ CTR block 4k+8
	add	r12, r12, #1                            @ CTR block 4k+8
	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	eor	q2, q6, q2                            @ AES block 4k+6 - result
	fmov	v0.d[1], r9                               @ CTR block 4k+8
	rev	r9, r12                                 @ CTR block 4k+9

	eor	r6, r6, r13                   @ AES block 4k+4 - round 12 low
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+9
	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low

	fmov	d1, r10                               @ CTR block 4k+9
	add	r12, r12, #1                            @ CTR block 4k+9
	eor	r19, r19, r13                   @ AES block 4k+5 - round 12 low

	fmov	v1.d[1], r9                               @ CTR block 4k+9
	rev	r9, r12                                 @ CTR block 4k+10
	eor	r20, r20, r14                   @ AES block 4k+5 - round 12 high

	eor	r7, r7, r14                   @ AES block 4k+4 - round 12 high
	stp	r6, r7, [r2], #16        @ AES block 4k+4 - store result
	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low

	add	r12, r12, #1                            @ CTR block 4k+10
	rev64	q4, q4                                    @ GHASH block 4k+4
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+10

	aese	q3, v29.16b                                     @ AES block 4k+7 - round 11
	stp	r19, r20, [r2], #16        @ AES block 4k+5 - store result
	blt	L192_dec_main_loop

L192_dec_prepretail:@ PREPRETAIL
	mov	r22, v2.d[1]                            @ AES block 4k+2 - mov high
	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0
	eor	q3, q7, q3                            @ AES block 4k+3 - result

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	mov	r21, v2.d[0]                            @ AES block 4k+2 - mov low

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid

	eor	q4, q4, v11.16b                           @ PRE 1
	fmov	d2, r10                               @ CTR block 4k+6

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	mov	r23, v3.d[0]                            @ AES block 4k+3 - mov low

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	mov	r24, v3.d[1]                            @ AES block 4k+3 - mov high

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid
	fmov	d3, r10                               @ CTR block 4k+7

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2
	rev64	q6, q6                                    @ GHASH block 4k+2

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high
	fmov	v2.d[1], r9                               @ CTR block 4k+6
	rev	r9, r12                                 @ CTR block 4k+7

	orr	r9, r11, r9, lsl #32            @ CTR block 4k+7
	eor	q8, q8, q4                          @ GHASH block 4k - mid
	mov	d4, v5.d[1]                                  @ GHASH block 4k+1 - mid

	pmull	v31.1q, q5, v14.1d                          @ GHASH block 4k+1 - low
	eor	r24, r24, r14                   @ AES block 4k+3 - round 12 high
	fmov	v3.d[1], r9                               @ CTR block 4k+7

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2
	eor	r21, r21, r13                   @ AES block 4k+2 - round 12 low

	pmull2	v30.1q, q5, v14.2d                          @ GHASH block 4k+1 - high
	eor	r22, r22, r14                   @ AES block 4k+2 - round 12 high
	eor	q4, q4, q5                          @ GHASH block 4k+1 - mid

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid
	eor	r23, r23, r13                   @ AES block 4k+3 - round 12 low
	stp	r21, r22, [r2], #16        @ AES block 4k+2 - store result

	rev64	q7, q7                                    @ GHASH block 4k+3
	stp	r23, r24, [r2], #16        @ AES block 4k+3 - store result

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	eor	q9, q9, v30.16b                         @ GHASH block 4k+1 - high

	pmull	v4.1q, q4, v17.1d                          @ GHASH block 4k+1 - mid
	add	r12, r12, #1                            @ CTR block 4k+7

	pmull2	v30.1q, q6, v13.2d                          @ GHASH block 4k+2 - high
	eor	v11.16b, v11.16b, v31.16b                         @ GHASH block 4k+1 - low

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0

	eor	v10.16b, v10.16b, q4                         @ GHASH block 4k+1 - mid
	mov	d31, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	eor	q9, q9, v30.16b                         @ GHASH block 4k+2 - high

	eor	v31.8b, v31.8b, q6                          @ GHASH block 4k+2 - mid

	pmull	v8.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2
	mov	d30, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2
	ins	v31.d[1], v31.d[0]                                @ GHASH block 4k+2 - mid

	pmull	v6.1q, q7, v12.1d                          @ GHASH block 4k+3 - low

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3
	eor	v30.8b, v30.8b, q7                          @ GHASH block 4k+3 - mid

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3

	pmull2	v31.1q, v31.2d, v16.2d                          @ GHASH block 4k+2 - mid
	eor	v11.16b, v11.16b, q8                         @ GHASH block 4k+2 - low

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4

	pmull2	v5.1q, q7, v12.2d                          @ GHASH block 4k+3 - high
	movi	q8, #0xc2

	pmull	v30.1q, v30.1d, v16.1d                          @ GHASH block 4k+3 - mid

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3

	shl	d8, d8, #56               @ mod_constant
	eor	q9, q9, q5                         @ GHASH block 4k+3 - high

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5
	eor	v10.16b, v10.16b, v31.16b                         @ GHASH block 4k+2 - mid

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4

	pmull	v31.1q, q9, q8            @ MODULO - top 64b align with mid
	eor	v11.16b, v11.16b, q6                         @ GHASH block 4k+3 - low

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3
	eor	v10.16b, v10.16b, v30.16b                         @ GHASH block 4k+3 - mid

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7
	eor	v30.16b, v11.16b, q9                         @ MODULO - karatsuba tidy up

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5
	eor	v10.16b, v10.16b, v30.16b                         @ MODULO - karatsuba tidy up

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 9

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6
	eor	v10.16b, v10.16b, v31.16b                      @ MODULO - fold into mid

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 10

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8
	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 9

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 9

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 9

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 10

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 10
	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 10

	aese	q0, v29.16b
	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low

	aese	q2, v29.16b

	aese	q1, v29.16b

	aese	q3, v29.16b

	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
L192_dec_tail:@ TAIL

	sub	r5, r4, r0   @ main_end_input_ptr is number of bytes left to process
	ld1	{ q5}, [r0], #16                      @ AES block 4k+4 - load ciphertext

	eor	q0, q5, q0                            @ AES block 4k+4 - result

	mov	r7, v0.d[1]                            @ AES block 4k+4 - mov high

	mov	r6, v0.d[0]                            @ AES block 4k+4 - mov low

	ext	q8, v11.16b, v11.16b, #8                     @ prepare final partial tag

	cmp	r5, #48

	eor	r7, r7, r14                   @ AES block 4k+4 - round 12 high

	eor	r6, r6, r13                   @ AES block 4k+4 - round 12 low
	bgt	L192_dec_blocks_more_than_3

	movi	v11.8b, #0
	movi	q9, #0

	mov	q3, q2
	mov	q2, q1
	sub	r12, r12, #1

	movi	v10.8b, #0
	cmp	r5, #32
	bgt	L192_dec_blocks_more_than_2

	mov	q3, q1
	cmp	r5, #16
	sub	r12, r12, #1

	bgt	L192_dec_blocks_more_than_1

	sub	r12, r12, #1
	b	L192_dec_blocks_less_than_1
L192_dec_blocks_more_than_3:@ blocks left >  3
	rev64	q4, q5                                    @ GHASH final-3 block
	ld1	{ q5}, [r0], #16                      @ AES final-2 block - load ciphertext

	stp	r6, r7, [r2], #16        @ AES final-3 block  - store result

	eor	q4, q4, q8                           @ feed in partial tag

	eor	q0, q5, q1                            @ AES final-2 block - result

	pmull	v11.1q, q4, v15.1d                       @ GHASH final-3 block - low
	mov	r6, v0.d[0]                            @ AES final-2 block - mov low
	mov	d22, v4.d[1]                                 @ GHASH final-3 block - mid

	mov	r7, v0.d[1]                            @ AES final-2 block - mov high

	mov	d10, v17.d[1]                               @ GHASH final-3 block - mid
	eor	v22.8b, v22.8b, q4                      @ GHASH final-3 block - mid

	pmull2	v9.1q, q4, v15.2d                       @ GHASH final-3 block - high

	eor	r6, r6, r13                   @ AES final-2 block - round 12 low
	movi	q8, #0                                        @ suppress further partial tag feed in

	pmull	v10.1q, v22.1d, v10.1d                    @ GHASH final-3 block - mid
	eor	r7, r7, r14                   @ AES final-2 block - round 12 high
L192_dec_blocks_more_than_2:@ blocks left >  2

	rev64	q4, q5                                    @ GHASH final-2 block
	ld1	{ q5}, [r0], #16                      @ AES final-1 block - load ciphertext

	eor	q4, q4, q8                           @ feed in partial tag

	movi	q8, #0                                        @ suppress further partial tag feed in

	eor	q0, q5, q2                            @ AES final-1 block - result

	mov	d22, v4.d[1]                                 @ GHASH final-2 block - mid

	pmull	v21.1q, q4, v14.1d                          @ GHASH final-2 block - low

	stp	r6, r7, [r2], #16        @ AES final-2 block  - store result

	eor	v22.8b, v22.8b, q4                      @ GHASH final-2 block - mid
	mov	r7, v0.d[1]                            @ AES final-1 block - mov high

	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final-2 block - low
	mov	r6, v0.d[0]                            @ AES final-1 block - mov low

	pmull2	v20.1q, q4, v14.2d                          @ GHASH final-2 block - high

	pmull	v22.1q, v22.1d, v17.1d                      @ GHASH final-2 block - mid

	eor	q9, q9, v20.16b                            @ GHASH final-2 block - high
	eor	r7, r7, r14                   @ AES final-1 block - round 12 high

	eor	r6, r6, r13                   @ AES final-1 block - round 12 low
	eor	v10.16b, v10.16b, v22.16b                       @ GHASH final-2 block - mid
L192_dec_blocks_more_than_1:@ blocks left >  1

	rev64	q4, q5                                    @ GHASH final-1 block

	eor	q4, q4, q8                           @ feed in partial tag
	ld1	{ q5}, [r0], #16                      @ AES final block - load ciphertext

	mov	d22, v4.d[1]                                 @ GHASH final-1 block - mid

	pmull2	v20.1q, q4, v13.2d                          @ GHASH final-1 block - high

	eor	q0, q5, q3                            @ AES final block - result
	stp	r6, r7, [r2], #16        @ AES final-1 block  - store result

	eor	v22.8b, v22.8b, q4                      @ GHASH final-1 block - mid

	eor	q9, q9, v20.16b                            @ GHASH final-1 block - high

	pmull	v21.1q, q4, v13.1d                          @ GHASH final-1 block - low
	mov	r7, v0.d[1]                            @ AES final block - mov high

	ins	v22.d[1], v22.d[0]                            @ GHASH final-1 block - mid
	mov	r6, v0.d[0]                            @ AES final block - mov low

	pmull2	v22.1q, v22.2d, v16.2d                      @ GHASH final-1 block - mid

	movi	q8, #0                                        @ suppress further partial tag feed in
	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final-1 block - low
	eor	r7, r7, r14                   @ AES final block - round 12 high

	eor	r6, r6, r13                   @ AES final block - round 12 low

	eor	v10.16b, v10.16b, v22.16b                       @ GHASH final-1 block - mid
L192_dec_blocks_less_than_1:@ blocks left <= 1

	mvn	r13, xzr                                      @ rk12_l = 0xffffffffffffffff
	ldp	r4, r5, [r2]  @ load existing bytes we need to not overwrite
	and	r1, r1, #127                    @ bit_length %= 128

	sub	r1, r1, #128                    @ bit_length -= 128

	neg	r1, r1                          @ bit_length = 128 - #bits in input (in range [1,128])

	and	r1, r1, #127                    @ bit_length %= 128
	mvn	r14, xzr                                      @ rk12_h = 0xffffffffffffffff

	lsr	r14, r14, r1                     @ rk12_h is mask for top 64b of last block
	cmp	r1, #64

	csel	r9, r13, r14, lt
	csel	r10, r14, xzr, lt

	fmov	d0, r9                                   @ ctr0b is mask for last block
	and	r6, r6, r9
	bic	r4, r4, r9           @ mask out low existing bytes

	orr	r6, r6, r4
	mov	v0.d[1], r10

	rev	r9, r12

	and	q5, q5, q0                            @ possibly partial last block has zeroes in highest bits
	str	r9, [r16, #12]                          @ store the updated counter

	rev64	q4, q5                                    @ GHASH final block

	eor	q4, q4, q8                           @ feed in partial tag
	bic	r5, r5, r10 @ mask out high existing bytes

	and	r7, r7, r10

	pmull2	v20.1q, q4, v12.2d                          @ GHASH final block - high
	mov	d8, v4.d[1]                                  @ GHASH final block - mid

	pmull	v21.1q, q4, v12.1d                          @ GHASH final block - low

	eor	q8, q8, q4                          @ GHASH final block - mid

	eor	q9, q9, v20.16b                            @ GHASH final block - high

	pmull	v8.1q, q8, v16.1d                          @ GHASH final block - mid

	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final block - low

	eor	v10.16b, v10.16b, q8                         @ GHASH final block - mid
	movi	q8, #0xc2

	eor	v30.16b, v11.16b, q9                         @ MODULO - karatsuba tidy up

	shl	d8, d8, #56               @ mod_constant

	eor	v10.16b, v10.16b, v30.16b                         @ MODULO - karatsuba tidy up

	pmull	v31.1q, q9, q8            @ MODULO - top 64b align with mid
	orr	r7, r7, r5
	stp	r6, r7, [r2]

	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	eor	v10.16b, v10.16b, v31.16b                      @ MODULO - fold into mid

	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low

	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low

	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b
	mov	r0, r15
	st1	{ v11.16b }, [r3]

	ldp	r21, r22, [sp, #16]
	ldp	r23, r24, [sp, #32]
	ldp	d8, d9, [sp, #48]
	ldp	d10, d11, [sp, #64]
	ldp	d12, d13, [sp, #80]
	ldp	d14, d15, [sp, #96]
	ldp	r19, r20, [sp], #112
	bx	lr

L192_dec_ret:
	mov	r0, #0x0
	bx	lr

.globl	aes_gcm_enc_256_kernel

.align	4
aes_gcm_enc_256_kernel:
	cbz	r1, L256_enc_ret
	stp	r19, r20, [sp, #-112]!
	mov	r16, r4
	mov	r8, r5
	stp	r21, r22, [sp, #16]
	stp	r23, r24, [sp, #32]
	stp	d8, d9, [sp, #48]
	stp	d10, d11, [sp, #64]
	stp	d12, d13, [sp, #80]
	stp	d14, d15, [sp, #96]

	add	r4, r0, r1, lsr #3   @ end_input_ptr
	lsr	r5, r1, #3              @ byte_len
	mov	r15, r5
	ldp	r10, r11, [r16]              @ ctr96_b64, ctr96_t32

	ld1	{ q0}, [r16]                             @ special case vector load initial counter so we can start first AES block as quickly as possible
	sub	r5, r5, #1      @ byte_len - 1

	ldr	q18, [r8, #0]                                  @ load rk0
	and	r5, r5, #0xffffffffffffffc0 @ number of bytes to be processed in main loop (at least 1 byte must be handled by tail)

	ldr	q25, [r8, #112]                                @ load rk7
	add	r5, r5, r0

	lsr	r12, r11, #32
	fmov	d2, r10                               @ CTR block 2
	orr	r11, r11, r11

	rev	r12, r12                                @ rev_ctr32
	cmp	r0, r5                   @ check if we have <= 4 blocks
	fmov	d1, r10                               @ CTR block 1

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 0 - round 0
	add	r12, r12, #1                            @ increment rev_ctr32

	rev	r9, r12                                 @ CTR block 1
	fmov	d3, r10                               @ CTR block 3

	orr	r9, r11, r9, lsl #32            @ CTR block 1
	add	r12, r12, #1                            @ CTR block 1
	ldr	q19, [r8, #16]                                 @ load rk1

	fmov	v1.d[1], r9                               @ CTR block 1
	rev	r9, r12                                 @ CTR block 2
	add	r12, r12, #1                            @ CTR block 2

	orr	r9, r11, r9, lsl #32            @ CTR block 2
	ldr	q20, [r8, #32]                                 @ load rk2

	fmov	v2.d[1], r9                               @ CTR block 2
	rev	r9, r12                                 @ CTR block 3

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 0 - round 1
	orr	r9, r11, r9, lsl #32            @ CTR block 3

	fmov	v3.d[1], r9                               @ CTR block 3

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 1 - round 0
	ldr	q21, [r8, #48]                                 @ load rk3

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 0 - round 2
	ldr	q24, [r8, #96]                                 @ load rk6

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 2 - round 0
	ldr	q23, [r8, #80]                                 @ load rk5

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 1 - round 1
	ldr	q14, [r3, #80]                         @ load h3l | h3h
	ext	v14.16b, v14.16b, v14.16b, #8

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 3 - round 0
	ldr	q31, [r8, #208]                               @ load rk13

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 2 - round 1
	ldr	q22, [r8, #64]                                 @ load rk4

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 1 - round 2
	ldr	q13, [r3, #64]                         @ load h2l | h2h
	ext	v13.16b, v13.16b, v13.16b, #8

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 3 - round 1
	ldr	q30, [r8, #192]                               @ load rk12

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 2 - round 2
	ldr	q15, [r3, #112]                        @ load h4l | h4h
	ext	v15.16b, v15.16b, v15.16b, #8

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 1 - round 3
	ldr	q29, [r8, #176]                               @ load rk11

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 3 - round 2
	ldr	q26, [r8, #128]                                @ load rk8

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 2 - round 3
	add	r12, r12, #1                            @ CTR block 3

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 0 - round 3
	ldp	r13, r14, [r8, #224]                     @ load rk14

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 3 - round 3
	ld1	{ v11.16b}, [r3]
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 2 - round 4

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 0 - round 4

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 1 - round 4

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 3 - round 4

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 0 - round 5

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 1 - round 5

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 3 - round 5

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 2 - round 5

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 1 - round 6
	trn2	v17.2d,  v14.2d,    v15.2d                      @ h4l | h3l

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 3 - round 6
	ldr	q27, [r8, #144]                                @ load rk9

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 0 - round 6
	ldr	q12, [r3, #32]                         @ load h1l | h1h
	ext	v12.16b, v12.16b, v12.16b, #8

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 2 - round 6
	ldr	q28, [r8, #160]                               @ load rk10

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 1 - round 7
	trn1	q9, v14.2d,    v15.2d                      @ h4h | h3h

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 0 - round 7

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 2 - round 7

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 3 - round 7
	trn2	v16.2d,  v12.2d,    v13.2d                      @ h2l | h1l

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 1 - round 8

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 2 - round 8

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 3 - round 8

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 1 - round 9

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 2 - round 9

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 0 - round 8

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 1 - round 10

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 3 - round 9

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 0 - round 9

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 2 - round 10

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 3 - round 10

	aese	q1, v29.16b
	aesmc	q1, q1          @ AES block 1 - round 11

	aese	q2, v29.16b
	aesmc	q2, q2          @ AES block 2 - round 11

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 0 - round 10

	aese	q1, v30.16b
	aesmc	q1, q1          @ AES block 1 - round 12

	aese	q2, v30.16b
	aesmc	q2, q2          @ AES block 2 - round 12

	aese	q0, v29.16b
	aesmc	q0, q0          @ AES block 0 - round 11
	eor	v17.16b, v17.16b, q9                  @ h4k | h3k

	aese	q3, v29.16b
	aesmc	q3, q3          @ AES block 3 - round 11

	aese	q2, v31.16b                                     @ AES block 2 - round 13
	trn1	q8,    v12.2d,    v13.2d                      @ h2h | h1h

	aese	q0, v30.16b
	aesmc	q0, q0          @ AES block 0 - round 12

	aese	q3, v30.16b
	aesmc	q3, q3          @ AES block 3 - round 12

	aese	q1, v31.16b                                     @ AES block 1 - round 13

	aese	q0, v31.16b                                     @ AES block 0 - round 13

	aese	q3, v31.16b                                     @ AES block 3 - round 13
	eor	v16.16b, v16.16b, q8                     @ h2k | h1k
	bge	L256_enc_tail                                    @ handle tail

	ldp	r19, r20, [r0, #16]           @ AES block 1 - load plaintext

	rev	r9, r12                                 @ CTR block 4
	ldp	r6, r7, [r0, #0]            @ AES block 0 - load plaintext

	ldp	r23, r24, [r0, #48]           @ AES block 3 - load plaintext

	ldp	r21, r22, [r0, #32]           @ AES block 2 - load plaintext
	add	r0, r0, #64                       @ AES input_ptr update

	eor	r19, r19, r13                     @ AES block 1 - round 14 low
	eor	r20, r20, r14                     @ AES block 1 - round 14 high

	fmov	d5, r19                               @ AES block 1 - mov low
	eor	r6, r6, r13                     @ AES block 0 - round 14 low

	eor	r7, r7, r14                     @ AES block 0 - round 14 high
	eor	r24, r24, r14                     @ AES block 3 - round 14 high
	fmov	d4, r6                               @ AES block 0 - mov low

	cmp	r0, r5                   @ check if we have <= 8 blocks
	fmov	v4.d[1], r7                           @ AES block 0 - mov high
	eor	r23, r23, r13                     @ AES block 3 - round 14 low

	eor	r21, r21, r13                     @ AES block 2 - round 14 low
	fmov	v5.d[1], r20                           @ AES block 1 - mov high

	fmov	d6, r21                               @ AES block 2 - mov low
	add	r12, r12, #1                            @ CTR block 4

	orr	r9, r11, r9, lsl #32            @ CTR block 4
	fmov	d7, r23                               @ AES block 3 - mov low
	eor	r22, r22, r14                     @ AES block 2 - round 14 high

	fmov	v6.d[1], r22                           @ AES block 2 - mov high

	eor	q4, q4, q0                          @ AES block 0 - result
	fmov	d0, r10                               @ CTR block 4

	fmov	v0.d[1], r9                               @ CTR block 4
	rev	r9, r12                                 @ CTR block 5
	add	r12, r12, #1                            @ CTR block 5

	eor	q5, q5, q1                          @ AES block 1 - result
	fmov	d1, r10                               @ CTR block 5
	orr	r9, r11, r9, lsl #32            @ CTR block 5

	fmov	v1.d[1], r9                               @ CTR block 5
	rev	r9, r12                                 @ CTR block 6
	st1	{ q4}, [r2], #16                     @ AES block 0 - store result

	fmov	v7.d[1], r24                           @ AES block 3 - mov high
	orr	r9, r11, r9, lsl #32            @ CTR block 6
	eor	q6, q6, q2                          @ AES block 2 - result

	st1	{ q5}, [r2], #16                     @ AES block 1 - store result

	add	r12, r12, #1                            @ CTR block 6
	fmov	d2, r10                               @ CTR block 6

	fmov	v2.d[1], r9                               @ CTR block 6
	st1	{ q6}, [r2], #16                     @ AES block 2 - store result
	rev	r9, r12                                 @ CTR block 7

	orr	r9, r11, r9, lsl #32            @ CTR block 7

	eor	q7, q7, q3                          @ AES block 3 - result
	st1	{ q7}, [r2], #16                     @ AES block 3 - store result
	bge	L256_enc_prepretail                               @ do prepretail

L256_enc_main_loop:@ main loop start
	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	rev64	q4, q4                                    @ GHASH block 4k (only t0 is free)

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	fmov	d3, r10                               @ CTR block 4k+3

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0
	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	fmov	v3.d[1], r9                               @ CTR block 4k+3

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	ldp	r23, r24, [r0, #48]           @ AES block 4k+7 - load plaintext

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	ldp	r21, r22, [r0, #32]           @ AES block 4k+6 - load plaintext

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2
	eor	q4, q4, v11.16b                           @ PRE 1

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	eor	r23, r23, r13                     @ AES block 4k+7 - round 14 low

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high
	eor	r22, r22, r14                     @ AES block 4k+6 - round 14 high
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1
	rev64	q5, q5                                    @ GHASH block 4k+1 (t0 and t1 free)

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	eor	q8, q8, q4                          @ GHASH block 4k - mid

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5
	rev64	q7, q7                                    @ GHASH block 4k+3 (t0, t1, t2 and t3 free)

	pmull2	v4.1q, q5, v14.2d                          @ GHASH block 4k+1 - high

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid
	rev64	q6, q6                                    @ GHASH block 4k+2 (t0, t1, and t2 free)

	pmull	v8.1q, q5, v14.1d                          @ GHASH block 4k+1 - low

	eor	q9, q9, q4                         @ GHASH block 4k+1 - high
	mov	d4, v5.d[1]                                  @ GHASH block 4k+1 - mid

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2
	eor	v11.16b, v11.16b, q8                         @ GHASH block 4k+1 - low

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4
	mov	d8, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3
	eor	q4, q4, q5                          @ GHASH block 4k+1 - mid

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6
	eor	q8, q8, q6                          @ GHASH block 4k+2 - mid

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4

	pmull	v4.1q, q4, v17.1d                          @ GHASH block 4k+1 - mid

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5
	ins	v8.d[1], v8.d[0]                                @ GHASH block 4k+2 - mid

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6
	eor	v10.16b, v10.16b, q4                         @ GHASH block 4k+1 - mid

	pmull2	v4.1q, q6, v13.2d                          @ GHASH block 4k+2 - high

	pmull	v5.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7

	pmull	v6.1q, q7, v12.1d                          @ GHASH block 4k+3 - low
	eor	q9, q9, q4                         @ GHASH block 4k+2 - high

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6
	ldp	r19, r20, [r0, #16]           @ AES block 4k+5 - load plaintext

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8
	mov	d4, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6
	eor	v11.16b, v11.16b, q5                         @ GHASH block 4k+2 - low

	pmull2	v8.1q, q8, v16.2d                          @ GHASH block 4k+2 - mid

	pmull2	v5.1q, q7, v12.2d                          @ GHASH block 4k+3 - high
	eor	q4, q4, q7                          @ GHASH block 4k+3 - mid

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7
	eor	r19, r19, r13                     @ AES block 4k+5 - round 14 low

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 9
	eor	v10.16b, v10.16b, q8                         @ GHASH block 4k+2 - mid

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7
	eor	r21, r21, r13                     @ AES block 4k+6 - round 14 low

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 9
	movi	q8, #0xc2

	pmull	v4.1q, q4, v16.1d                          @ GHASH block 4k+3 - mid
	eor	q9, q9, q5                         @ GHASH block 4k+3 - high
	fmov	d5, r19                               @ AES block 4k+5 - mov low

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8
	ldp	r6, r7, [r0, #0]            @ AES block 4k+4 - load plaintext

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 10
	shl	d8, d8, #56               @ mod_constant

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8
	eor	v11.16b, v11.16b, q6                         @ GHASH block 4k+3 - low

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 9

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 10
	eor	v10.16b, v10.16b, q4                         @ GHASH block 4k+3 - mid

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 9
	add	r12, r12, #1                            @ CTR block 4k+3

	aese	q0, v29.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 11
	eor	q4, v11.16b, q9                         @ MODULO - karatsuba tidy up

	aese	q1, v29.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 11
	add	r0, r0, #64                       @ AES input_ptr update

	pmull	v7.1q, q9, q8            @ MODULO - top 64b align with mid
	rev	r9, r12                                 @ CTR block 4k+8
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 10
	eor	r6, r6, r13                     @ AES block 4k+4 - round 14 low

	aese	q1, v30.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 12
	eor	v10.16b, v10.16b, q4                         @ MODULO - karatsuba tidy up

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 10
	eor	r7, r7, r14                     @ AES block 4k+4 - round 14 high

	fmov	d4, r6                               @ AES block 4k+4 - mov low
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+8
	eor	q7, q9, q7                   @ MODULO - fold into mid

	aese	q0, v30.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 12
	eor	r20, r20, r14                     @ AES block 4k+5 - round 14 high

	aese	q2, v29.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 11
	eor	r24, r24, r14                     @ AES block 4k+7 - round 14 high

	aese	q3, v29.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 11
	add	r12, r12, #1                            @ CTR block 4k+8

	aese	q0, v31.16b                                     @ AES block 4k+4 - round 13
	fmov	v4.d[1], r7                           @ AES block 4k+4 - mov high
	eor	v10.16b, v10.16b, q7                      @ MODULO - fold into mid

	aese	q2, v30.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 12
	fmov	d7, r23                               @ AES block 4k+7 - mov low

	aese	q1, v31.16b                                     @ AES block 4k+5 - round 13
	fmov	v5.d[1], r20                           @ AES block 4k+5 - mov high

	fmov	d6, r21                               @ AES block 4k+6 - mov low
	cmp	r0, r5                   @ LOOP CONTROL

	fmov	v6.d[1], r22                           @ AES block 4k+6 - mov high

	pmull	v9.1q, v10.1d, q8            @ MODULO - mid 64b align with low
	eor	q4, q4, q0                          @ AES block 4k+4 - result
	fmov	d0, r10                               @ CTR block 4k+8

	fmov	v0.d[1], r9                               @ CTR block 4k+8
	rev	r9, r12                                 @ CTR block 4k+9
	add	r12, r12, #1                            @ CTR block 4k+9

	eor	q5, q5, q1                          @ AES block 4k+5 - result
	fmov	d1, r10                               @ CTR block 4k+9
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+9

	aese	q3, v30.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 12
	fmov	v1.d[1], r9                               @ CTR block 4k+9

	aese	q2, v31.16b                                     @ AES block 4k+6 - round 13
	rev	r9, r12                                 @ CTR block 4k+10
	st1	{ q4}, [r2], #16                     @ AES block 4k+4 - store result

	orr	r9, r11, r9, lsl #32            @ CTR block 4k+10
	eor	v11.16b, v11.16b, q9                         @ MODULO - fold into low
	fmov	v7.d[1], r24                           @ AES block 4k+7 - mov high

	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment
	st1	{ q5}, [r2], #16                     @ AES block 4k+5 - store result
	add	r12, r12, #1                            @ CTR block 4k+10

	aese	q3, v31.16b                                     @ AES block 4k+7 - round 13
	eor	q6, q6, q2                          @ AES block 4k+6 - result
	fmov	d2, r10                               @ CTR block 4k+10

	st1	{ q6}, [r2], #16                     @ AES block 4k+6 - store result
	fmov	v2.d[1], r9                               @ CTR block 4k+10
	rev	r9, r12                                 @ CTR block 4k+11

	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+11

	eor	q7, q7, q3                          @ AES block 4k+7 - result
	st1	{ q7}, [r2], #16                     @ AES block 4k+7 - store result
	blt	L256_enc_main_loop

L256_enc_prepretail:@ PREPRETAIL
	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	rev64	q6, q6                                    @ GHASH block 4k+2 (t0, t1, and t2 free)

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0
	fmov	d3, r10                               @ CTR block 4k+3

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	rev64	q4, q4                                    @ GHASH block 4k (only t0 is free)

	fmov	v3.d[1], r9                               @ CTR block 4k+3
	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1

	eor	q4, q4, v11.16b                           @ PRE 1
	rev64	q5, q5                                    @ GHASH block 4k+1 (t0 and t1 free)

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2
	eor	q8, q8, q4                          @ GHASH block 4k - mid

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid

	pmull2	v4.1q, q5, v14.2d                          @ GHASH block 4k+1 - high

	pmull	v8.1q, q5, v14.1d                          @ GHASH block 4k+1 - low

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2

	eor	q9, q9, q4                         @ GHASH block 4k+1 - high
	mov	d4, v5.d[1]                                  @ GHASH block 4k+1 - mid

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3
	eor	v11.16b, v11.16b, q8                         @ GHASH block 4k+1 - low

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3

	eor	q4, q4, q5                          @ GHASH block 4k+1 - mid
	mov	d8, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4
	rev64	q7, q7                                    @ GHASH block 4k+3 (t0, t1, t2 and t3 free)

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4

	pmull	v4.1q, q4, v17.1d                          @ GHASH block 4k+1 - mid
	eor	q8, q8, q6                          @ GHASH block 4k+2 - mid
	add	r12, r12, #1                            @ CTR block 4k+3

	pmull	v5.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4
	eor	v10.16b, v10.16b, q4                         @ GHASH block 4k+1 - mid

	pmull2	v4.1q, q6, v13.2d                          @ GHASH block 4k+2 - high

	eor	v11.16b, v11.16b, q5                         @ GHASH block 4k+2 - low
	ins	v8.d[1], v8.d[0]                                @ GHASH block 4k+2 - mid

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5

	eor	q9, q9, q4                         @ GHASH block 4k+2 - high
	mov	d4, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4

	pmull2	v8.1q, q8, v16.2d                          @ GHASH block 4k+2 - mid

	eor	q4, q4, q7                          @ GHASH block 4k+3 - mid

	pmull2	v5.1q, q7, v12.2d                          @ GHASH block 4k+3 - high

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5

	pmull	v4.1q, q4, v16.1d                          @ GHASH block 4k+3 - mid
	eor	v10.16b, v10.16b, q8                         @ GHASH block 4k+2 - mid

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6
	movi	q8, #0xc2

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7
	eor	q9, q9, q5                         @ GHASH block 4k+3 - high

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7
	shl	d8, d8, #56               @ mod_constant

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8
	eor	v10.16b, v10.16b, q4                         @ GHASH block 4k+3 - mid

	pmull	v6.1q, q7, v12.1d                          @ GHASH block 4k+3 - low

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 9

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8
	eor	v11.16b, v11.16b, q6                         @ GHASH block 4k+3 - low

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 9

	eor	v10.16b, v10.16b, q9                         @ karatsuba tidy up

	pmull	v4.1q, q9, q8
	ext	q9, q9, q9, #8

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 10

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7
	eor	v10.16b, v10.16b, v11.16b

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 10

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 9

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8

	aese	q1, v29.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 11
	eor	v10.16b, v10.16b, q4

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 10

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 9

	aese	q1, v30.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 12

	aese	q0, v29.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 11
	eor	v10.16b, v10.16b, q9

	aese	q3, v29.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 11

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 10

	aese	q0, v30.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 12

	pmull	v4.1q, v10.1d, q8

	aese	q2, v29.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 11
	ext	v10.16b, v10.16b, v10.16b, #8

	aese	q3, v30.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 12

	aese	q1, v31.16b                                     @ AES block 4k+5 - round 13
	eor	v11.16b, v11.16b, q4

	aese	q2, v30.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 12

	aese	q3, v31.16b                                     @ AES block 4k+7 - round 13

	aese	q0, v31.16b                                     @ AES block 4k+4 - round 13

	aese	q2, v31.16b                                     @ AES block 4k+6 - round 13
	eor	v11.16b, v11.16b, v10.16b
L256_enc_tail:@ TAIL

	ext	q8, v11.16b, v11.16b, #8                     @ prepare final partial tag
	sub	r5, r4, r0   @ main_end_input_ptr is number of bytes left to process
	ldp	r6, r7, [r0], #16           @ AES block 4k+4 - load plaintext

	eor	r6, r6, r13                     @ AES block 4k+4 - round 14 low
	eor	r7, r7, r14                     @ AES block 4k+4 - round 14 high

	cmp	r5, #48
	fmov	d4, r6                               @ AES block 4k+4 - mov low

	fmov	v4.d[1], r7                           @ AES block 4k+4 - mov high

	eor	q5, q4, q0                          @ AES block 4k+4 - result
	bgt	L256_enc_blocks_more_than_3

	cmp	r5, #32
	mov	q3, q2
	movi	v11.8b, #0

	movi	q9, #0
	sub	r12, r12, #1

	mov	q2, q1
	movi	v10.8b, #0
	bgt	L256_enc_blocks_more_than_2

	mov	q3, q1
	sub	r12, r12, #1
	cmp	r5, #16

	bgt	L256_enc_blocks_more_than_1

	sub	r12, r12, #1
	b	L256_enc_blocks_less_than_1
L256_enc_blocks_more_than_3:@ blocks left >  3
	st1	{ q5}, [r2], #16                    @ AES final-3 block  - store result

	ldp	r6, r7, [r0], #16          @ AES final-2 block - load input low & high

	rev64	q4, q5                                   @ GHASH final-3 block

	eor	r6, r6, r13                    @ AES final-2 block - round 14 low
	eor	q4, q4, q8                          @ feed in partial tag

	eor	r7, r7, r14                    @ AES final-2 block - round 14 high

	mov	d22, v4.d[1]                                @ GHASH final-3 block - mid
	fmov	d5, r6                                @ AES final-2 block - mov low

	fmov	v5.d[1], r7                            @ AES final-2 block - mov high

	eor	v22.8b, v22.8b, q4                     @ GHASH final-3 block - mid
	movi	q8, #0                                       @ suppress further partial tag feed in

	mov	d10, v17.d[1]                              @ GHASH final-3 block - mid

	pmull	v11.1q, q4, v15.1d                      @ GHASH final-3 block - low

	pmull2	v9.1q, q4, v15.2d                      @ GHASH final-3 block - high

	pmull	v10.1q, v22.1d, v10.1d                   @ GHASH final-3 block - mid
	eor	q5, q5, q1                           @ AES final-2 block - result
L256_enc_blocks_more_than_2:@ blocks left >  2

	st1	{ q5}, [r2], #16                    @ AES final-2 block - store result

	ldp	r6, r7, [r0], #16          @ AES final-1 block - load input low & high

	rev64	q4, q5                                   @ GHASH final-2 block

	eor	r6, r6, r13                    @ AES final-1 block - round 14 low
	eor	q4, q4, q8                          @ feed in partial tag

	fmov	d5, r6                                @ AES final-1 block - mov low
	eor	r7, r7, r14                    @ AES final-1 block - round 14 high

	fmov	v5.d[1], r7                            @ AES final-1 block - mov high

	movi	q8, #0                                       @ suppress further partial tag feed in

	pmull2	v20.1q, q4, v14.2d                         @ GHASH final-2 block - high
	mov	d22, v4.d[1]                                @ GHASH final-2 block - mid

	pmull	v21.1q, q4, v14.1d                         @ GHASH final-2 block - low

	eor	v22.8b, v22.8b, q4                     @ GHASH final-2 block - mid

	eor	q5, q5, q2                           @ AES final-1 block - result

	eor	q9, q9, v20.16b                           @ GHASH final-2 block - high

	pmull	v22.1q, v22.1d, v17.1d                     @ GHASH final-2 block - mid

	eor	v11.16b, v11.16b, v21.16b                           @ GHASH final-2 block - low

	eor	v10.16b, v10.16b, v22.16b                      @ GHASH final-2 block - mid
L256_enc_blocks_more_than_1:@ blocks left >  1

	st1	{ q5}, [r2], #16                    @ AES final-1 block - store result

	rev64	q4, q5                                   @ GHASH final-1 block

	ldp	r6, r7, [r0], #16          @ AES final block - load input low & high

	eor	q4, q4, q8                          @ feed in partial tag

	movi	q8, #0                                       @ suppress further partial tag feed in

	eor	r6, r6, r13                    @ AES final block - round 14 low
	mov	d22, v4.d[1]                                @ GHASH final-1 block - mid

	pmull2	v20.1q, q4, v13.2d                         @ GHASH final-1 block - high
	eor	r7, r7, r14                    @ AES final block - round 14 high

	eor	v22.8b, v22.8b, q4                     @ GHASH final-1 block - mid

	eor	q9, q9, v20.16b                           @ GHASH final-1 block - high

	ins	v22.d[1], v22.d[0]                           @ GHASH final-1 block - mid
	fmov	d5, r6                                @ AES final block - mov low

	fmov	v5.d[1], r7                            @ AES final block - mov high

	pmull2	v22.1q, v22.2d, v16.2d                     @ GHASH final-1 block - mid

	pmull	v21.1q, q4, v13.1d                         @ GHASH final-1 block - low

	eor	q5, q5, q3                           @ AES final block - result
	eor	v10.16b, v10.16b, v22.16b                      @ GHASH final-1 block - mid

	eor	v11.16b, v11.16b, v21.16b                           @ GHASH final-1 block - low
L256_enc_blocks_less_than_1:@ blocks left <= 1

	and	r1, r1, #127                   @ bit_length %= 128

	mvn	r13, xzr                                     @ rk14_l = 0xffffffffffffffff
	sub	r1, r1, #128                   @ bit_length -= 128

	neg	r1, r1                         @ bit_length = 128 - #bits in input (in range [1,128])
	ld1	{ v18.16b}, [r2]                           @ load existing bytes where the possibly partial last block is to be stored

	mvn	r14, xzr                                     @ rk14_h = 0xffffffffffffffff
	and	r1, r1, #127                   @ bit_length %= 128

	lsr	r14, r14, r1                    @ rk14_h is mask for top 64b of last block
	cmp	r1, #64

	csel	r6, r13, r14, lt
	csel	r7, r14, xzr, lt

	fmov	d0, r6                                @ ctr0b is mask for last block

	fmov	v0.d[1], r7

	and	q5, q5, q0                           @ possibly partial last block has zeroes in highest bits

	rev64	q4, q5                                   @ GHASH final block

	eor	q4, q4, q8                          @ feed in partial tag

	bif	q5, v18.16b, q0                             @ insert existing bytes in top end of result before storing

	pmull2	v20.1q, q4, v12.2d                         @ GHASH final block - high
	mov	d8, v4.d[1]                                 @ GHASH final block - mid
	rev	r9, r12

	pmull	v21.1q, q4, v12.1d                         @ GHASH final block - low

	eor	q9, q9, v20.16b                           @ GHASH final block - high
	eor	q8, q8, q4                         @ GHASH final block - mid

	pmull	v8.1q, q8, v16.1d                         @ GHASH final block - mid

	eor	v11.16b, v11.16b, v21.16b                           @ GHASH final block - low

	eor	v10.16b, v10.16b, q8                        @ GHASH final block - mid
	movi	q8, #0xc2

	eor	q4, v11.16b, q9                        @ MODULO - karatsuba tidy up

	shl	d8, d8, #56              @ mod_constant

	eor	v10.16b, v10.16b, q4                        @ MODULO - karatsuba tidy up

	pmull	v7.1q, q9, q8           @ MODULO - top 64b align with mid

	ext	q9, q9, q9, #8                    @ MODULO - other top alignment

	eor	v10.16b, v10.16b, q7                     @ MODULO - fold into mid

	eor	v10.16b, v10.16b, q9                        @ MODULO - fold into mid

	pmull	v9.1q, v10.1d, q8           @ MODULO - mid 64b align with low

	ext	v10.16b, v10.16b, v10.16b, #8                    @ MODULO - other mid alignment

	str	r9, [r16, #12]                         @ store the updated counter

	st1	{ q5}, [r2]                         @ store all 16B
	eor	v11.16b, v11.16b, q9                        @ MODULO - fold into low

	eor	v11.16b, v11.16b, v10.16b                        @ MODULO - fold into low
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b
	mov	r0, r15
	st1	{ v11.16b }, [r3]

	ldp	r21, r22, [sp, #16]
	ldp	r23, r24, [sp, #32]
	ldp	d8, d9, [sp, #48]
	ldp	d10, d11, [sp, #64]
	ldp	d12, d13, [sp, #80]
	ldp	d14, d15, [sp, #96]
	ldp	r19, r20, [sp], #112
	bx	lr

L256_enc_ret:
	mov	r0, #0x0
	bx	lr

.globl	aes_gcm_dec_256_kernel

.align	4
aes_gcm_dec_256_kernel:
	cbz	r1, L256_dec_ret
	stp	r19, r20, [sp, #-112]!
	mov	r16, r4
	mov	r8, r5
	stp	r21, r22, [sp, #16]
	stp	r23, r24, [sp, #32]
	stp	d8, d9, [sp, #48]
	stp	d10, d11, [sp, #64]
	stp	d12, d13, [sp, #80]
	stp	d14, d15, [sp, #96]

	lsr	r5, r1, #3              @ byte_len
	mov	r15, r5
	ldp	r10, r11, [r16]              @ ctr96_b64, ctr96_t32

	ldr	q26, [r8, #128]                                @ load rk8
	sub	r5, r5, #1      @ byte_len - 1

	ldr	q25, [r8, #112]                                @ load rk7
	and	r5, r5, #0xffffffffffffffc0 @ number of bytes to be processed in main loop (at least 1 byte must be handled by tail)

	add	r4, r0, r1, lsr #3   @ end_input_ptr
	ldr	q24, [r8, #96]                                 @ load rk6

	lsr	r12, r11, #32
	ldr	q23, [r8, #80]                                 @ load rk5
	orr	r11, r11, r11

	ldr	q21, [r8, #48]                                 @ load rk3
	add	r5, r5, r0
	rev	r12, r12                                @ rev_ctr32

	add	r12, r12, #1                            @ increment rev_ctr32
	fmov	d3, r10                               @ CTR block 3

	rev	r9, r12                                 @ CTR block 1
	add	r12, r12, #1                            @ CTR block 1
	fmov	d1, r10                               @ CTR block 1

	orr	r9, r11, r9, lsl #32            @ CTR block 1
	ld1	{ q0}, [r16]                             @ special case vector load initial counter so we can start first AES block as quickly as possible

	fmov	v1.d[1], r9                               @ CTR block 1
	rev	r9, r12                                 @ CTR block 2
	add	r12, r12, #1                            @ CTR block 2

	fmov	d2, r10                               @ CTR block 2
	orr	r9, r11, r9, lsl #32            @ CTR block 2

	fmov	v2.d[1], r9                               @ CTR block 2
	rev	r9, r12                                 @ CTR block 3

	orr	r9, r11, r9, lsl #32            @ CTR block 3
	ldr	q18, [r8, #0]                                  @ load rk0

	fmov	v3.d[1], r9                               @ CTR block 3
	add	r12, r12, #1                            @ CTR block 3

	ldr	q22, [r8, #64]                                 @ load rk4

	ldr	q31, [r8, #208]                               @ load rk13

	ldr	q19, [r8, #16]                                 @ load rk1

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 0 - round 0
	ldr	q14, [r3, #80]                         @ load h3l | h3h
	ext	v14.16b, v14.16b, v14.16b, #8

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 3 - round 0
	ldr	q15, [r3, #112]                        @ load h4l | h4h
	ext	v15.16b, v15.16b, v15.16b, #8

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 1 - round 0
	ldr	q13, [r3, #64]                         @ load h2l | h2h
	ext	v13.16b, v13.16b, v13.16b, #8

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 2 - round 0
	ldr	q20, [r8, #32]                                 @ load rk2

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 0 - round 1
	ldp	r13, r14, [r8, #224]                     @ load rk14

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 1 - round 1
	ld1	{ v11.16b}, [r3]
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 2 - round 1
	ldr	q27, [r8, #144]                                @ load rk9

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 3 - round 1
	ldr	q30, [r8, #192]                               @ load rk12

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 0 - round 2
	ldr	q12, [r3, #32]                         @ load h1l | h1h
	ext	v12.16b, v12.16b, v12.16b, #8

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 2 - round 2
	ldr	q28, [r8, #160]                               @ load rk10

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 3 - round 2

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 0 - round 3

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 1 - round 2

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 3 - round 3

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 0 - round 4
	cmp	r0, r5                   @ check if we have <= 4 blocks

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 2 - round 3

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 1 - round 3

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 3 - round 4

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 2 - round 4

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 1 - round 4

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 3 - round 5

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 0 - round 5

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 1 - round 5

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 2 - round 5

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 0 - round 6

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 3 - round 6

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 1 - round 6

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 2 - round 6

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 0 - round 7

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 1 - round 7

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 3 - round 7

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 0 - round 8

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 2 - round 7

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 3 - round 8

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 1 - round 8

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 0 - round 9

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 2 - round 8
	ldr	q29, [r8, #176]                               @ load rk11

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 1 - round 9

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 0 - round 10

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 3 - round 9

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 1 - round 10

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 2 - round 9

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 3 - round 10

	aese	q0, v29.16b
	aesmc	q0, q0          @ AES block 0 - round 11

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 2 - round 10

	aese	q3, v29.16b
	aesmc	q3, q3          @ AES block 3 - round 11

	aese	q1, v29.16b
	aesmc	q1, q1          @ AES block 1 - round 11

	aese	q2, v29.16b
	aesmc	q2, q2          @ AES block 2 - round 11

	trn1	q9, v14.2d,    v15.2d                      @ h4h | h3h

	trn2	v17.2d,  v14.2d,    v15.2d                      @ h4l | h3l

	trn1	q8,    v12.2d,    v13.2d                      @ h2h | h1h
	trn2	v16.2d,  v12.2d,    v13.2d                      @ h2l | h1l

	aese	q1, v30.16b
	aesmc	q1, q1          @ AES block 1 - round 12

	aese	q0, v30.16b
	aesmc	q0, q0          @ AES block 0 - round 12

	aese	q2, v30.16b
	aesmc	q2, q2          @ AES block 2 - round 12

	aese	q3, v30.16b
	aesmc	q3, q3          @ AES block 3 - round 12
	eor	v17.16b, v17.16b, q9                  @ h4k | h3k

	aese	q1, v31.16b                                     @ AES block 1 - round 13

	aese	q2, v31.16b                                     @ AES block 2 - round 13
	eor	v16.16b, v16.16b, q8                     @ h2k | h1k

	aese	q3, v31.16b                                     @ AES block 3 - round 13

	aese	q0, v31.16b                                     @ AES block 0 - round 13
	bge	L256_dec_tail                                    @ handle tail

	ldr	q4, [r0, #0]                          @ AES block 0 - load ciphertext

	ldr	q5, [r0, #16]                         @ AES block 1 - load ciphertext

	rev	r9, r12                                 @ CTR block 4

	eor	q0, q4, q0                            @ AES block 0 - result

	eor	q1, q5, q1                            @ AES block 1 - result
	rev64	q5, q5                                    @ GHASH block 1
	ldr	q7, [r0, #48]                         @ AES block 3 - load ciphertext

	mov	r7, v0.d[1]                            @ AES block 0 - mov high

	mov	r6, v0.d[0]                            @ AES block 0 - mov low
	rev64	q4, q4                                    @ GHASH block 0
	add	r12, r12, #1                            @ CTR block 4

	fmov	d0, r10                               @ CTR block 4
	orr	r9, r11, r9, lsl #32            @ CTR block 4

	fmov	v0.d[1], r9                               @ CTR block 4
	rev	r9, r12                                 @ CTR block 5
	add	r12, r12, #1                            @ CTR block 5

	mov	r19, v1.d[0]                            @ AES block 1 - mov low

	orr	r9, r11, r9, lsl #32            @ CTR block 5
	mov	r20, v1.d[1]                            @ AES block 1 - mov high
	eor	r7, r7, r14                   @ AES block 0 - round 14 high

	eor	r6, r6, r13                   @ AES block 0 - round 14 low
	stp	r6, r7, [r2], #16        @ AES block 0 - store result
	fmov	d1, r10                               @ CTR block 5

	ldr	q6, [r0, #32]                         @ AES block 2 - load ciphertext
	add	r0, r0, #64                       @ AES input_ptr update

	fmov	v1.d[1], r9                               @ CTR block 5
	rev	r9, r12                                 @ CTR block 6
	add	r12, r12, #1                            @ CTR block 6

	eor	r19, r19, r13                   @ AES block 1 - round 14 low
	orr	r9, r11, r9, lsl #32            @ CTR block 6

	eor	r20, r20, r14                   @ AES block 1 - round 14 high
	stp	r19, r20, [r2], #16        @ AES block 1 - store result

	eor	q2, q6, q2                            @ AES block 2 - result
	cmp	r0, r5                   @ check if we have <= 8 blocks
	bge	L256_dec_prepretail                              @ do prepretail

L256_dec_main_loop:@ main loop start
	mov	r21, v2.d[0]                            @ AES block 4k+2 - mov low
	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0
	eor	q3, q7, q3                            @ AES block 4k+3 - result

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	mov	r22, v2.d[1]                            @ AES block 4k+2 - mov high

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	fmov	d2, r10                               @ CTR block 4k+6

	fmov	v2.d[1], r9                               @ CTR block 4k+6
	eor	q4, q4, v11.16b                           @ PRE 1
	rev	r9, r12                                 @ CTR block 4k+7

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	mov	r24, v3.d[1]                            @ AES block 4k+3 - mov high

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	mov	r23, v3.d[0]                            @ AES block 4k+3 - mov low

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid
	fmov	d3, r10                               @ CTR block 4k+7

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+7

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0
	fmov	v3.d[1], r9                               @ CTR block 4k+7

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2
	eor	q8, q8, q4                          @ GHASH block 4k - mid

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3
	eor	r22, r22, r14                   @ AES block 4k+2 - round 14 high

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3
	rev64	q6, q6                                    @ GHASH block 4k+2

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0
	eor	r21, r21, r13                   @ AES block 4k+2 - round 14 low

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2
	stp	r21, r22, [r2], #16        @ AES block 4k+2 - store result

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low

	pmull2	v4.1q, q5, v14.2d                          @ GHASH block 4k+1 - high

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3
	rev64	q7, q7                                    @ GHASH block 4k+3

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid
	eor	r23, r23, r13                   @ AES block 4k+3 - round 14 low

	pmull	v8.1q, q5, v14.1d                          @ GHASH block 4k+1 - low
	eor	r24, r24, r14                   @ AES block 4k+3 - round 14 high
	eor	q9, q9, q4                         @ GHASH block 4k+1 - high

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1
	mov	d4, v5.d[1]                                  @ GHASH block 4k+1 - mid

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4
	eor	v11.16b, v11.16b, q8                         @ GHASH block 4k+1 - low

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5
	add	r12, r12, #1                            @ CTR block 4k+7

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2
	mov	d8, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4
	eor	q4, q4, q5                          @ GHASH block 4k+1 - mid

	pmull	v5.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3
	eor	q8, q8, q6                          @ GHASH block 4k+2 - mid

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5
	eor	v11.16b, v11.16b, q5                         @ GHASH block 4k+2 - low

	pmull	v4.1q, q4, v17.1d                          @ GHASH block 4k+1 - mid
	rev	r9, r12                                 @ CTR block 4k+8

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6
	ins	v8.d[1], v8.d[0]                                @ GHASH block 4k+2 - mid

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6
	add	r12, r12, #1                            @ CTR block 4k+8

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7
	eor	v10.16b, v10.16b, q4                         @ GHASH block 4k+1 - mid

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7

	pmull2	v4.1q, q6, v13.2d                          @ GHASH block 4k+2 - high
	mov	d6, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5

	pmull2	v8.1q, q8, v16.2d                          @ GHASH block 4k+2 - mid

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8
	eor	q9, q9, q4                         @ GHASH block 4k+2 - high

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6

	pmull	v4.1q, q7, v12.1d                          @ GHASH block 4k+3 - low
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+8
	eor	v10.16b, v10.16b, q8                         @ GHASH block 4k+2 - mid

	pmull2	v5.1q, q7, v12.2d                          @ GHASH block 4k+3 - high

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 9
	eor	q6, q6, q7                          @ GHASH block 4k+3 - mid

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6
	eor	q9, q9, q5                         @ GHASH block 4k+3 - high

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 10

	pmull	v6.1q, q6, v16.1d                          @ GHASH block 4k+3 - mid
	movi	q8, #0xc2

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7
	eor	v11.16b, v11.16b, q4                         @ GHASH block 4k+3 - low

	aese	q0, v29.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 11

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7
	shl	d8, d8, #56               @ mod_constant

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8
	eor	v10.16b, v10.16b, q6                         @ GHASH block 4k+3 - mid

	aese	q0, v30.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 12

	pmull	v7.1q, q9, q8            @ MODULO - top 64b align with mid
	eor	q6, v11.16b, q9                         @ MODULO - karatsuba tidy up

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 9
	ldr	q4, [r0, #0]                          @ AES block 4k+4 - load ciphertext

	aese	q0, v31.16b                                     @ AES block 4k+4 - round 13
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 10
	eor	v10.16b, v10.16b, q6                         @ MODULO - karatsuba tidy up

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 9
	ldr	q5, [r0, #16]                         @ AES block 4k+5 - load ciphertext

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8
	eor	q0, q4, q0                            @ AES block 4k+4 - result

	aese	q1, v29.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 11
	stp	r23, r24, [r2], #16        @ AES block 4k+3 - store result

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 10
	eor	v10.16b, v10.16b, q7                      @ MODULO - fold into mid

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 9
	ldr	q7, [r0, #48]                         @ AES block 4k+7 - load ciphertext

	aese	q1, v30.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 12
	ldr	q6, [r0, #32]                         @ AES block 4k+6 - load ciphertext

	aese	q2, v29.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 11
	mov	r7, v0.d[1]                            @ AES block 4k+4 - mov high

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 10
	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	aese	q1, v31.16b                                     @ AES block 4k+5 - round 13
	add	r0, r0, #64                       @ AES input_ptr update
	mov	r6, v0.d[0]                            @ AES block 4k+4 - mov low

	aese	q2, v30.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 12
	fmov	d0, r10                               @ CTR block 4k+8

	aese	q3, v29.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 11
	fmov	v0.d[1], r9                               @ CTR block 4k+8

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low
	eor	q1, q5, q1                            @ AES block 4k+5 - result
	rev	r9, r12                                 @ CTR block 4k+9

	aese	q2, v31.16b                                     @ AES block 4k+6 - round 13
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+9
	cmp	r0, r5                   @ LOOP CONTROL

	add	r12, r12, #1                            @ CTR block 4k+9

	eor	r6, r6, r13                   @ AES block 4k+4 - round 14 low
	eor	r7, r7, r14                   @ AES block 4k+4 - round 14 high

	mov	r20, v1.d[1]                            @ AES block 4k+5 - mov high
	eor	q2, q6, q2                            @ AES block 4k+6 - result
	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low

	aese	q3, v30.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 12
	mov	r19, v1.d[0]                            @ AES block 4k+5 - mov low

	fmov	d1, r10                               @ CTR block 4k+9
	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	fmov	v1.d[1], r9                               @ CTR block 4k+9
	rev	r9, r12                                 @ CTR block 4k+10
	add	r12, r12, #1                            @ CTR block 4k+10

	aese	q3, v31.16b                                     @ AES block 4k+7 - round 13
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+10

	rev64	q5, q5                                    @ GHASH block 4k+5
	eor	r20, r20, r14                   @ AES block 4k+5 - round 14 high
	stp	r6, r7, [r2], #16        @ AES block 4k+4 - store result

	eor	r19, r19, r13                   @ AES block 4k+5 - round 14 low
	stp	r19, r20, [r2], #16        @ AES block 4k+5 - store result

	rev64	q4, q4                                    @ GHASH block 4k+4
	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
	blt	L256_dec_main_loop


L256_dec_prepretail:@ PREPRETAIL
	ext	v11.16b, v11.16b, v11.16b, #8                     @ PRE 0
	mov	r21, v2.d[0]                            @ AES block 4k+2 - mov low
	eor	q3, q7, q3                            @ AES block 4k+3 - result

	aese	q0, v18.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 0
	mov	r22, v2.d[1]                            @ AES block 4k+2 - mov high

	aese	q1, v18.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 0
	fmov	d2, r10                               @ CTR block 4k+6

	fmov	v2.d[1], r9                               @ CTR block 4k+6
	rev	r9, r12                                 @ CTR block 4k+7
	eor	q4, q4, v11.16b                           @ PRE 1

	rev64	q6, q6                                    @ GHASH block 4k+2
	orr	r9, r11, r9, lsl #32            @ CTR block 4k+7
	mov	r23, v3.d[0]                            @ AES block 4k+3 - mov low

	aese	q1, v19.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 1
	mov	r24, v3.d[1]                            @ AES block 4k+3 - mov high

	pmull	v11.1q, q4, v15.1d                       @ GHASH block 4k - low
	mov	d8, v4.d[1]                                  @ GHASH block 4k - mid
	fmov	d3, r10                               @ CTR block 4k+7

	pmull2	v9.1q, q4, v15.2d                       @ GHASH block 4k - high
	fmov	v3.d[1], r9                               @ CTR block 4k+7

	aese	q2, v18.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 0
	mov	d10, v17.d[1]                               @ GHASH block 4k - mid

	aese	q0, v19.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 1
	eor	q8, q8, q4                          @ GHASH block 4k - mid

	pmull2	v4.1q, q5, v14.2d                          @ GHASH block 4k+1 - high

	aese	q2, v19.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 1
	rev64	q7, q7                                    @ GHASH block 4k+3

	aese	q3, v18.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 0

	pmull	v10.1q, q8, v10.1d                      @ GHASH block 4k - mid
	eor	q9, q9, q4                         @ GHASH block 4k+1 - high

	pmull	v8.1q, q5, v14.1d                          @ GHASH block 4k+1 - low

	aese	q3, v19.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 1
	mov	d4, v5.d[1]                                  @ GHASH block 4k+1 - mid

	aese	q0, v20.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 2

	aese	q1, v20.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 2
	eor	v11.16b, v11.16b, q8                         @ GHASH block 4k+1 - low

	aese	q2, v20.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 2

	aese	q0, v21.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 3
	mov	d8, v6.d[1]                                  @ GHASH block 4k+2 - mid

	aese	q3, v20.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 2
	eor	q4, q4, q5                          @ GHASH block 4k+1 - mid

	pmull	v5.1q, q6, v13.1d                          @ GHASH block 4k+2 - low

	aese	q0, v22.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 4

	aese	q3, v21.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 3
	eor	q8, q8, q6                          @ GHASH block 4k+2 - mid

	pmull	v4.1q, q4, v17.1d                          @ GHASH block 4k+1 - mid

	aese	q0, v23.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 5
	eor	v11.16b, v11.16b, q5                         @ GHASH block 4k+2 - low

	aese	q3, v22.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 4

	pmull2	v5.1q, q7, v12.2d                          @ GHASH block 4k+3 - high
	eor	v10.16b, v10.16b, q4                         @ GHASH block 4k+1 - mid

	pmull2	v4.1q, q6, v13.2d                          @ GHASH block 4k+2 - high

	aese	q3, v23.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 5
	ins	v8.d[1], v8.d[0]                                @ GHASH block 4k+2 - mid

	aese	q2, v21.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 3

	aese	q1, v21.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 3
	eor	q9, q9, q4                         @ GHASH block 4k+2 - high

	pmull	v4.1q, q7, v12.1d                          @ GHASH block 4k+3 - low

	aese	q2, v22.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 4
	mov	d6, v7.d[1]                                  @ GHASH block 4k+3 - mid

	aese	q1, v22.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 4

	pmull2	v8.1q, q8, v16.2d                          @ GHASH block 4k+2 - mid

	aese	q2, v23.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 5
	eor	q6, q6, q7                          @ GHASH block 4k+3 - mid

	aese	q1, v23.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 5

	aese	q3, v24.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 6
	eor	v10.16b, v10.16b, q8                         @ GHASH block 4k+2 - mid

	aese	q2, v24.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 6

	aese	q0, v24.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 6
	movi	q8, #0xc2

	aese	q1, v24.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 6
	eor	v11.16b, v11.16b, q4                         @ GHASH block 4k+3 - low

	pmull	v6.1q, q6, v16.1d                          @ GHASH block 4k+3 - mid

	aese	q3, v25.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 7
	eor	q9, q9, q5                         @ GHASH block 4k+3 - high

	aese	q1, v25.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 7

	aese	q0, v25.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 7
	eor	v10.16b, v10.16b, q6                         @ GHASH block 4k+3 - mid

	aese	q3, v26.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 8

	aese	q2, v25.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 7
	eor	q6, v11.16b, q9                         @ MODULO - karatsuba tidy up

	aese	q1, v26.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 8

	aese	q0, v26.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 8
	shl	d8, d8, #56               @ mod_constant

	aese	q2, v26.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 8

	aese	q1, v27.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 9
	eor	v10.16b, v10.16b, q6                         @ MODULO - karatsuba tidy up

	pmull	v7.1q, q9, q8            @ MODULO - top 64b align with mid

	aese	q2, v27.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 9
	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	aese	q3, v27.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 9

	aese	q0, v27.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 9
	eor	v10.16b, v10.16b, q7                      @ MODULO - fold into mid

	aese	q2, v28.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 10

	aese	q3, v28.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 10

	aese	q0, v28.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 10
	eor	r22, r22, r14                   @ AES block 4k+2 - round 14 high

	aese	q1, v28.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 10
	eor	r23, r23, r13                   @ AES block 4k+3 - round 14 low

	aese	q2, v29.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 11
	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	aese	q0, v29.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 11
	add	r12, r12, #1                            @ CTR block 4k+7

	aese	q1, v29.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 11
	eor	r21, r21, r13                   @ AES block 4k+2 - round 14 low

	aese	q2, v30.16b
	aesmc	q2, q2          @ AES block 4k+6 - round 12

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low
	eor	r24, r24, r14                   @ AES block 4k+3 - round 14 high

	aese	q3, v29.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 11
	stp	r21, r22, [r2], #16        @ AES block 4k+2 - store result

	aese	q1, v30.16b
	aesmc	q1, q1          @ AES block 4k+5 - round 12
	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	aese	q0, v30.16b
	aesmc	q0, q0          @ AES block 4k+4 - round 12
	stp	r23, r24, [r2], #16        @ AES block 4k+3 - store result

	aese	q3, v30.16b
	aesmc	q3, q3          @ AES block 4k+7 - round 12
	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low

	aese	q1, v31.16b                                     @ AES block 4k+5 - round 13

	aese	q0, v31.16b                                     @ AES block 4k+4 - round 13

	aese	q3, v31.16b                                     @ AES block 4k+7 - round 13

	aese	q2, v31.16b                                     @ AES block 4k+6 - round 13
	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
L256_dec_tail:@ TAIL

	sub	r5, r4, r0   @ main_end_input_ptr is number of bytes left to process
	ld1	{ q5}, [r0], #16                      @ AES block 4k+4 - load ciphertext

	eor	q0, q5, q0                            @ AES block 4k+4 - result

	mov	r6, v0.d[0]                            @ AES block 4k+4 - mov low

	mov	r7, v0.d[1]                            @ AES block 4k+4 - mov high
	ext	q8, v11.16b, v11.16b, #8                     @ prepare final partial tag

	cmp	r5, #48

	eor	r6, r6, r13                   @ AES block 4k+4 - round 14 low

	eor	r7, r7, r14                   @ AES block 4k+4 - round 14 high
	bgt	L256_dec_blocks_more_than_3

	sub	r12, r12, #1
	mov	q3, q2
	movi	v10.8b, #0

	movi	v11.8b, #0
	cmp	r5, #32

	movi	q9, #0
	mov	q2, q1
	bgt	L256_dec_blocks_more_than_2

	sub	r12, r12, #1

	mov	q3, q1
	cmp	r5, #16
	bgt	L256_dec_blocks_more_than_1

	sub	r12, r12, #1
	b	L256_dec_blocks_less_than_1
L256_dec_blocks_more_than_3:@ blocks left >  3
	rev64	q4, q5                                   @ GHASH final-3 block
	ld1	{ q5}, [r0], #16                     @ AES final-2 block - load ciphertext

	stp	r6, r7, [r2], #16       @ AES final-3 block  - store result

	mov	d10, v17.d[1]                              @ GHASH final-3 block - mid

	eor	q4, q4, q8                          @ feed in partial tag

	eor	q0, q5, q1                           @ AES final-2 block - result

	mov	d22, v4.d[1]                                @ GHASH final-3 block - mid

	mov	r6, v0.d[0]                           @ AES final-2 block - mov low

	mov	r7, v0.d[1]                           @ AES final-2 block - mov high

	eor	v22.8b, v22.8b, q4                     @ GHASH final-3 block - mid

	movi	q8, #0                                       @ suppress further partial tag feed in

	pmull2	v9.1q, q4, v15.2d                      @ GHASH final-3 block - high

	pmull	v10.1q, v22.1d, v10.1d                   @ GHASH final-3 block - mid
	eor	r6, r6, r13                  @ AES final-2 block - round 14 low

	pmull	v11.1q, q4, v15.1d                      @ GHASH final-3 block - low
	eor	r7, r7, r14                  @ AES final-2 block - round 14 high
L256_dec_blocks_more_than_2:@ blocks left >  2

	rev64	q4, q5                                   @ GHASH final-2 block
	ld1	{ q5}, [r0], #16                     @ AES final-1 block - load ciphertext

	eor	q4, q4, q8                          @ feed in partial tag
	stp	r6, r7, [r2], #16       @ AES final-2 block  - store result

	eor	q0, q5, q2                           @ AES final-1 block - result

	mov	d22, v4.d[1]                                @ GHASH final-2 block - mid

	pmull	v21.1q, q4, v14.1d                         @ GHASH final-2 block - low

	pmull2	v20.1q, q4, v14.2d                         @ GHASH final-2 block - high

	eor	v22.8b, v22.8b, q4                     @ GHASH final-2 block - mid
	mov	r6, v0.d[0]                           @ AES final-1 block - mov low

	mov	r7, v0.d[1]                           @ AES final-1 block - mov high
	eor	v11.16b, v11.16b, v21.16b                           @ GHASH final-2 block - low
	movi	q8, #0                                       @ suppress further partial tag feed in

	pmull	v22.1q, v22.1d, v17.1d                     @ GHASH final-2 block - mid

	eor	q9, q9, v20.16b                           @ GHASH final-2 block - high
	eor	r6, r6, r13                  @ AES final-1 block - round 14 low

	eor	v10.16b, v10.16b, v22.16b                      @ GHASH final-2 block - mid
	eor	r7, r7, r14                  @ AES final-1 block - round 14 high
L256_dec_blocks_more_than_1:@ blocks left >  1

	stp	r6, r7, [r2], #16       @ AES final-1 block  - store result
	rev64	q4, q5                                   @ GHASH final-1 block

	ld1	{ q5}, [r0], #16                     @ AES final block - load ciphertext

	eor	q4, q4, q8                          @ feed in partial tag
	movi	q8, #0                                       @ suppress further partial tag feed in

	mov	d22, v4.d[1]                                @ GHASH final-1 block - mid

	eor	q0, q5, q3                           @ AES final block - result

	pmull2	v20.1q, q4, v13.2d                         @ GHASH final-1 block - high

	eor	v22.8b, v22.8b, q4                     @ GHASH final-1 block - mid

	pmull	v21.1q, q4, v13.1d                         @ GHASH final-1 block - low
	mov	r6, v0.d[0]                           @ AES final block - mov low

	ins	v22.d[1], v22.d[0]                           @ GHASH final-1 block - mid

	mov	r7, v0.d[1]                           @ AES final block - mov high

	pmull2	v22.1q, v22.2d, v16.2d                     @ GHASH final-1 block - mid
	eor	r6, r6, r13                  @ AES final block - round 14 low

	eor	v11.16b, v11.16b, v21.16b                           @ GHASH final-1 block - low

	eor	q9, q9, v20.16b                           @ GHASH final-1 block - high

	eor	v10.16b, v10.16b, v22.16b                      @ GHASH final-1 block - mid
	eor	r7, r7, r14                  @ AES final block - round 14 high
L256_dec_blocks_less_than_1:@ blocks left <= 1

	and	r1, r1, #127                   @ bit_length %= 128
	mvn	r14, xzr                                     @ rk14_h = 0xffffffffffffffff

	sub	r1, r1, #128                   @ bit_length -= 128
	mvn	r13, xzr                                     @ rk14_l = 0xffffffffffffffff

	ldp	r4, r5, [r2] @ load existing bytes we need to not overwrite
	neg	r1, r1                         @ bit_length = 128 - #bits in input (in range [1,128])

	and	r1, r1, #127                   @ bit_length %= 128

	lsr	r14, r14, r1                    @ rk14_h is mask for top 64b of last block
	cmp	r1, #64

	csel	r9, r13, r14, lt
	csel	r10, r14, xzr, lt

	fmov	d0, r9                                  @ ctr0b is mask for last block
	and	r6, r6, r9

	mov	v0.d[1], r10
	bic	r4, r4, r9          @ mask out low existing bytes

	rev	r9, r12

	bic	r5, r5, r10      @ mask out high existing bytes

	orr	r6, r6, r4

	and	r7, r7, r10

	orr	r7, r7, r5

	and	q5, q5, q0                            @ possibly partial last block has zeroes in highest bits

	rev64	q4, q5                                    @ GHASH final block

	eor	q4, q4, q8                           @ feed in partial tag

	pmull	v21.1q, q4, v12.1d                          @ GHASH final block - low

	mov	d8, v4.d[1]                                  @ GHASH final block - mid

	eor	q8, q8, q4                          @ GHASH final block - mid

	pmull2	v20.1q, q4, v12.2d                          @ GHASH final block - high

	pmull	v8.1q, q8, v16.1d                          @ GHASH final block - mid

	eor	q9, q9, v20.16b                            @ GHASH final block - high

	eor	v11.16b, v11.16b, v21.16b                            @ GHASH final block - low

	eor	v10.16b, v10.16b, q8                         @ GHASH final block - mid
	movi	q8, #0xc2

	eor	q6, v11.16b, q9                         @ MODULO - karatsuba tidy up

	shl	d8, d8, #56               @ mod_constant

	eor	v10.16b, v10.16b, q6                         @ MODULO - karatsuba tidy up

	pmull	v7.1q, q9, q8            @ MODULO - top 64b align with mid

	ext	q9, q9, q9, #8                     @ MODULO - other top alignment

	eor	v10.16b, v10.16b, q7                      @ MODULO - fold into mid

	eor	v10.16b, v10.16b, q9                         @ MODULO - fold into mid

	pmull	v8.1q, v10.1d, q8     @ MODULO - mid 64b align with low

	ext	v10.16b, v10.16b, v10.16b, #8                     @ MODULO - other mid alignment

	eor	v11.16b, v11.16b, q8               @ MODULO - fold into low

	stp	r6, r7, [r2]

	str	r9, [r16, #12]                          @ store the updated counter

	eor	v11.16b, v11.16b, v10.16b                         @ MODULO - fold into low
	ext	v11.16b, v11.16b, v11.16b, #8
	rev64	v11.16b, v11.16b
	mov	r0, r15
	st1	{ v11.16b }, [r3]

	ldp	r21, r22, [sp, #16]
	ldp	r23, r24, [sp, #32]
	ldp	d8, d9, [sp, #48]
	ldp	d10, d11, [sp, #64]
	ldp	d12, d13, [sp, #80]
	ldp	d14, d15, [sp, #96]
	ldp	r19, r20, [sp], #112
	bx	lr

L256_dec_ret:
	mov	r0, #0x0
	bx	lr

.byte	71,72,65,83,72,32,102,111,114,32,65,82,77,118,56,44,32,67,82,89,80,84,79,71,65,77,83,32,98,121,32,60,97,112,112,114,111,64,111,112,101,110,115,115,108,46,111,114,103,62,0
.align	2
.align	2
#endif
