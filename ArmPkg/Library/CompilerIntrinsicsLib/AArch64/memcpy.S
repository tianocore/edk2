/*
 * Copyright (c) 2011 - 2013, ARM Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


.text
.align 2


GCC_ASM_EXPORT(memcpy)


// Taken from Newlib BSD implementation.
ASM_PFX(memcpy):
        // Copy dst to x6, so we can preserve return value.
        mov     x6, x0

        // NOTE: although size_t is unsigned, this code uses signed
        // comparisons on x2 so relies on nb never having its top bit
        // set. In practice this is not going to be a real problem.

        // Require at least 64 bytes to be worth aligning.
        cmp     x2, #64
        blt     qwordcopy

        // Compute offset to align destination to 16 bytes.
        neg     x3, x0
        and     x3, x3, 15

        cbz     x3, blockcopy           // offset == 0 is likely

        // We know there is at least 64 bytes to be done, so we
        // do a 16 byte misaligned copy at first and then later do
        // all 16-byte aligned copies.  Some bytes will be copied
        // twice, but there's no harm in that since memcpy does not
        // guarantee correctness on overlap.

        sub     x2, x2, x3              // nb -= offset
        ldp     x4, x5, [x1]
        add     x1, x1, x3
        stp     x4, x5, [x6]
        add     x6, x6, x3

        // The destination pointer is now qword (16 byte) aligned.
        // (The src pointer might be.)

blockcopy:
        // Copy 64 bytes at a time.
        subs    x2, x2, #64
        blt     3f
2:      subs    x2, x2, #64
        ldp     x4, x5, [x1,#0]
        ldp     x8, x9, [x1,#16]
        ldp     x10,x11,[x1,#32]
        ldp     x12,x13,[x1,#48]
        add     x1, x1, #64
        stp     x4, x5, [x6,#0]
        stp     x8, x9, [x6,#16]
        stp     x10,x11,[x6,#32]
        stp     x12,x13,[x6,#48]
        add     x6, x6, #64
        bge     2b

        // Unwind pre-decrement
3:      add     x2, x2, #64

qwordcopy:
        // Copy 0-48 bytes, 16 bytes at a time.
        subs    x2, x2, #16
        blt     tailcopy
2:      ldp     x4, x5, [x1],#16
        subs    x2, x2, #16
        stp     x4, x5, [x6],#16
        bge     2b

        // No need to unwind the pre-decrement, it would not change
        // the low 4 bits of the count. But how likely is it for the
        // byte count to be multiple of 16? Is it worth the overhead
        // of testing for x2 == -16?

tailcopy:
        // Copy trailing 0-15 bytes.
        tbz     x2, #3, 1f
        ldr     x4, [x1],#8             // copy 8 bytes
        str     x4, [x6],#8
1:
        tbz     x2, #2, 1f
        ldr     w4, [x1],#4             // copy 4 bytes
        str     w4, [x6],#4
1:
        tbz     x2, #1, 1f
        ldrh    w4, [x1],#2             // copy 2 bytes
        strh    w4, [x6],#2
1:
        tbz     x2, #0, return
        ldrb    w4, [x1]                // copy 1 byte
        strb    w4, [x6]

return:
        // This is the only return point of memcpy.
        ret
