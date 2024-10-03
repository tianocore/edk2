/*
 * Copyright 2017-2021 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright 2017 Ribose Inc. All Rights Reserved.
 * Ported from Ribose contributions from Botan.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 *
 * Taken from OpenSSL release 3.0.9.
 */

#include "Sm3Core.h"

#define ROTATE(X, N)  LRotU32 (X, N)

#define P0(X)  (X ^ ROTATE(X, 9) ^ ROTATE(X, 17))
#define P1(X)  (X ^ ROTATE(X, 15) ^ ROTATE(X, 23))

#define FF0(X, Y, Z)  (X ^ Y ^ Z)
#define GG0(X, Y, Z)  (X ^ Y ^ Z)

#define FF1(X, Y, Z)  ((X & Y) | ((X | Y) & Z))
#define GG1(X, Y, Z)  ((Z ^ (X & (Y ^ Z))))

#define EXPAND(W0, W7, W13, W3, W10) \
   (P1(W0 ^ W7 ^ ROTATE(W13, 15)) ^ ROTATE(W3, 7) ^ W10)

#define RND(A, B, C, D, E, F, G, H, TJ, Wi, Wj, FF, GG)           \
     do {                                                         \
       const SM3_WORD A12 = ROTATE(A, 12);                        \
       const SM3_WORD A12_SM = A12 + E + TJ;                      \
       const SM3_WORD SS1 = ROTATE(A12_SM, 7);                    \
       const SM3_WORD TT1 = FF(A, B, C) + D + (SS1 ^ A12) + (Wj); \
       const SM3_WORD TT2 = GG(E, F, G) + H + SS1 + Wi;           \
       B = ROTATE(B, 9);                                          \
       D = TT1;                                                   \
       F = ROTATE(F, 19);                                         \
       H = P0(TT2);                                               \
     } while(0)

#define R1(A, B, C, D, E, F, G, H, TJ, Wi, Wj) \
   RND(A,B,C,D,E,F,G,H,TJ,Wi,Wj,FF0,GG0)

#define R2(A, B, C, D, E, F, G, H, TJ, Wi, Wj) \
   RND(A,B,C,D,E,F,G,H,TJ,Wi,Wj,FF1,GG1)

#define SM3_A  0x7380166fUL
#define SM3_B  0x4914b2b9UL
#define SM3_C  0x172442d7UL
#define SM3_D  0xda8a0600UL
#define SM3_E  0xa96f30bcUL
#define SM3_F  0x163138aaUL
#define SM3_G  0xe38dee4dUL
#define SM3_H  0xb0fb0e4eUL

int
ossl_sm3_init (
  SM3_CTX  *c
  )
{
  ZeroMem (c, sizeof (*c));
  c->A = SM3_A;
  c->B = SM3_B;
  c->C = SM3_C;
  c->D = SM3_D;
  c->E = SM3_E;
  c->F = SM3_F;
  c->G = SM3_G;
  c->H = SM3_H;
  return 1;
}

void
ossl_sm3_block_data_order (
  SM3_CTX     *ctx,
  const void  *p,
  size_t      num
  )
{
  const UINT32  *Data;
  UINT32        A, B, C, D, E, F, G, H;
  UINT32        W00, W01, W02, W03, W04, W05, W06, W07,
                W08, W09, W10, W11, W12, W13, W14, W15;

  Data = p;

  for ( ; num--;) {
    A = ctx->A;
    B = ctx->B;
    C = ctx->C;
    D = ctx->D;
    E = ctx->E;
    F = ctx->F;
    G = ctx->G;
    H = ctx->H;

    /*
     * We have to load all message bytes immediately since SM3 reads
     * them slightly out of order.
     */
    W00 = SwapBytes32 (ReadUnaligned32 (Data++));
    W01 = SwapBytes32 (ReadUnaligned32 (Data++));
    W02 = SwapBytes32 (ReadUnaligned32 (Data++));
    W03 = SwapBytes32 (ReadUnaligned32 (Data++));
    W04 = SwapBytes32 (ReadUnaligned32 (Data++));
    W05 = SwapBytes32 (ReadUnaligned32 (Data++));
    W06 = SwapBytes32 (ReadUnaligned32 (Data++));
    W07 = SwapBytes32 (ReadUnaligned32 (Data++));
    W08 = SwapBytes32 (ReadUnaligned32 (Data++));
    W09 = SwapBytes32 (ReadUnaligned32 (Data++));
    W10 = SwapBytes32 (ReadUnaligned32 (Data++));
    W11 = SwapBytes32 (ReadUnaligned32 (Data++));
    W12 = SwapBytes32 (ReadUnaligned32 (Data++));
    W13 = SwapBytes32 (ReadUnaligned32 (Data++));
    W14 = SwapBytes32 (ReadUnaligned32 (Data++));
    W15 = SwapBytes32 (ReadUnaligned32 (Data++));

    R1 (A, B, C, D, E, F, G, H, 0x79CC4519, W00, W00 ^ W04);
    W00 = EXPAND (W00, W07, W13, W03, W10);
    R1 (D, A, B, C, H, E, F, G, 0xF3988A32, W01, W01 ^ W05);
    W01 = EXPAND (W01, W08, W14, W04, W11);
    R1 (C, D, A, B, G, H, E, F, 0xE7311465, W02, W02 ^ W06);
    W02 = EXPAND (W02, W09, W15, W05, W12);
    R1 (B, C, D, A, F, G, H, E, 0xCE6228CB, W03, W03 ^ W07);
    W03 = EXPAND (W03, W10, W00, W06, W13);
    R1 (A, B, C, D, E, F, G, H, 0x9CC45197, W04, W04 ^ W08);
    W04 = EXPAND (W04, W11, W01, W07, W14);
    R1 (D, A, B, C, H, E, F, G, 0x3988A32F, W05, W05 ^ W09);
    W05 = EXPAND (W05, W12, W02, W08, W15);
    R1 (C, D, A, B, G, H, E, F, 0x7311465E, W06, W06 ^ W10);
    W06 = EXPAND (W06, W13, W03, W09, W00);
    R1 (B, C, D, A, F, G, H, E, 0xE6228CBC, W07, W07 ^ W11);
    W07 = EXPAND (W07, W14, W04, W10, W01);
    R1 (A, B, C, D, E, F, G, H, 0xCC451979, W08, W08 ^ W12);
    W08 = EXPAND (W08, W15, W05, W11, W02);
    R1 (D, A, B, C, H, E, F, G, 0x988A32F3, W09, W09 ^ W13);
    W09 = EXPAND (W09, W00, W06, W12, W03);
    R1 (C, D, A, B, G, H, E, F, 0x311465E7, W10, W10 ^ W14);
    W10 = EXPAND (W10, W01, W07, W13, W04);
    R1 (B, C, D, A, F, G, H, E, 0x6228CBCE, W11, W11 ^ W15);
    W11 = EXPAND (W11, W02, W08, W14, W05);
    R1 (A, B, C, D, E, F, G, H, 0xC451979C, W12, W12 ^ W00);
    W12 = EXPAND (W12, W03, W09, W15, W06);
    R1 (D, A, B, C, H, E, F, G, 0x88A32F39, W13, W13 ^ W01);
    W13 = EXPAND (W13, W04, W10, W00, W07);
    R1 (C, D, A, B, G, H, E, F, 0x11465E73, W14, W14 ^ W02);
    W14 = EXPAND (W14, W05, W11, W01, W08);
    R1 (B, C, D, A, F, G, H, E, 0x228CBCE6, W15, W15 ^ W03);
    W15 = EXPAND (W15, W06, W12, W02, W09);
    R2 (A, B, C, D, E, F, G, H, 0x9D8A7A87, W00, W00 ^ W04);
    W00 = EXPAND (W00, W07, W13, W03, W10);
    R2 (D, A, B, C, H, E, F, G, 0x3B14F50F, W01, W01 ^ W05);
    W01 = EXPAND (W01, W08, W14, W04, W11);
    R2 (C, D, A, B, G, H, E, F, 0x7629EA1E, W02, W02 ^ W06);
    W02 = EXPAND (W02, W09, W15, W05, W12);
    R2 (B, C, D, A, F, G, H, E, 0xEC53D43C, W03, W03 ^ W07);
    W03 = EXPAND (W03, W10, W00, W06, W13);
    R2 (A, B, C, D, E, F, G, H, 0xD8A7A879, W04, W04 ^ W08);
    W04 = EXPAND (W04, W11, W01, W07, W14);
    R2 (D, A, B, C, H, E, F, G, 0xB14F50F3, W05, W05 ^ W09);
    W05 = EXPAND (W05, W12, W02, W08, W15);
    R2 (C, D, A, B, G, H, E, F, 0x629EA1E7, W06, W06 ^ W10);
    W06 = EXPAND (W06, W13, W03, W09, W00);
    R2 (B, C, D, A, F, G, H, E, 0xC53D43CE, W07, W07 ^ W11);
    W07 = EXPAND (W07, W14, W04, W10, W01);
    R2 (A, B, C, D, E, F, G, H, 0x8A7A879D, W08, W08 ^ W12);
    W08 = EXPAND (W08, W15, W05, W11, W02);
    R2 (D, A, B, C, H, E, F, G, 0x14F50F3B, W09, W09 ^ W13);
    W09 = EXPAND (W09, W00, W06, W12, W03);
    R2 (C, D, A, B, G, H, E, F, 0x29EA1E76, W10, W10 ^ W14);
    W10 = EXPAND (W10, W01, W07, W13, W04);
    R2 (B, C, D, A, F, G, H, E, 0x53D43CEC, W11, W11 ^ W15);
    W11 = EXPAND (W11, W02, W08, W14, W05);
    R2 (A, B, C, D, E, F, G, H, 0xA7A879D8, W12, W12 ^ W00);
    W12 = EXPAND (W12, W03, W09, W15, W06);
    R2 (D, A, B, C, H, E, F, G, 0x4F50F3B1, W13, W13 ^ W01);
    W13 = EXPAND (W13, W04, W10, W00, W07);
    R2 (C, D, A, B, G, H, E, F, 0x9EA1E762, W14, W14 ^ W02);
    W14 = EXPAND (W14, W05, W11, W01, W08);
    R2 (B, C, D, A, F, G, H, E, 0x3D43CEC5, W15, W15 ^ W03);
    W15 = EXPAND (W15, W06, W12, W02, W09);
    R2 (A, B, C, D, E, F, G, H, 0x7A879D8A, W00, W00 ^ W04);
    W00 = EXPAND (W00, W07, W13, W03, W10);
    R2 (D, A, B, C, H, E, F, G, 0xF50F3B14, W01, W01 ^ W05);
    W01 = EXPAND (W01, W08, W14, W04, W11);
    R2 (C, D, A, B, G, H, E, F, 0xEA1E7629, W02, W02 ^ W06);
    W02 = EXPAND (W02, W09, W15, W05, W12);
    R2 (B, C, D, A, F, G, H, E, 0xD43CEC53, W03, W03 ^ W07);
    W03 = EXPAND (W03, W10, W00, W06, W13);
    R2 (A, B, C, D, E, F, G, H, 0xA879D8A7, W04, W04 ^ W08);
    W04 = EXPAND (W04, W11, W01, W07, W14);
    R2 (D, A, B, C, H, E, F, G, 0x50F3B14F, W05, W05 ^ W09);
    W05 = EXPAND (W05, W12, W02, W08, W15);
    R2 (C, D, A, B, G, H, E, F, 0xA1E7629E, W06, W06 ^ W10);
    W06 = EXPAND (W06, W13, W03, W09, W00);
    R2 (B, C, D, A, F, G, H, E, 0x43CEC53D, W07, W07 ^ W11);
    W07 = EXPAND (W07, W14, W04, W10, W01);
    R2 (A, B, C, D, E, F, G, H, 0x879D8A7A, W08, W08 ^ W12);
    W08 = EXPAND (W08, W15, W05, W11, W02);
    R2 (D, A, B, C, H, E, F, G, 0x0F3B14F5, W09, W09 ^ W13);
    W09 = EXPAND (W09, W00, W06, W12, W03);
    R2 (C, D, A, B, G, H, E, F, 0x1E7629EA, W10, W10 ^ W14);
    W10 = EXPAND (W10, W01, W07, W13, W04);
    R2 (B, C, D, A, F, G, H, E, 0x3CEC53D4, W11, W11 ^ W15);
    W11 = EXPAND (W11, W02, W08, W14, W05);
    R2 (A, B, C, D, E, F, G, H, 0x79D8A7A8, W12, W12 ^ W00);
    W12 = EXPAND (W12, W03, W09, W15, W06);
    R2 (D, A, B, C, H, E, F, G, 0xF3B14F50, W13, W13 ^ W01);
    W13 = EXPAND (W13, W04, W10, W00, W07);
    R2 (C, D, A, B, G, H, E, F, 0xE7629EA1, W14, W14 ^ W02);
    W14 = EXPAND (W14, W05, W11, W01, W08);
    R2 (B, C, D, A, F, G, H, E, 0xCEC53D43, W15, W15 ^ W03);
    W15 = EXPAND (W15, W06, W12, W02, W09);
    R2 (A, B, C, D, E, F, G, H, 0x9D8A7A87, W00, W00 ^ W04);
    W00 = EXPAND (W00, W07, W13, W03, W10);
    R2 (D, A, B, C, H, E, F, G, 0x3B14F50F, W01, W01 ^ W05);
    W01 = EXPAND (W01, W08, W14, W04, W11);
    R2 (C, D, A, B, G, H, E, F, 0x7629EA1E, W02, W02 ^ W06);
    W02 = EXPAND (W02, W09, W15, W05, W12);
    R2 (B, C, D, A, F, G, H, E, 0xEC53D43C, W03, W03 ^ W07);
    W03 = EXPAND (W03, W10, W00, W06, W13);
    R2 (A, B, C, D, E, F, G, H, 0xD8A7A879, W04, W04 ^ W08);
    R2 (D, A, B, C, H, E, F, G, 0xB14F50F3, W05, W05 ^ W09);
    R2 (C, D, A, B, G, H, E, F, 0x629EA1E7, W06, W06 ^ W10);
    R2 (B, C, D, A, F, G, H, E, 0xC53D43CE, W07, W07 ^ W11);
    R2 (A, B, C, D, E, F, G, H, 0x8A7A879D, W08, W08 ^ W12);
    R2 (D, A, B, C, H, E, F, G, 0x14F50F3B, W09, W09 ^ W13);
    R2 (C, D, A, B, G, H, E, F, 0x29EA1E76, W10, W10 ^ W14);
    R2 (B, C, D, A, F, G, H, E, 0x53D43CEC, W11, W11 ^ W15);
    R2 (A, B, C, D, E, F, G, H, 0xA7A879D8, W12, W12 ^ W00);
    R2 (D, A, B, C, H, E, F, G, 0x4F50F3B1, W13, W13 ^ W01);
    R2 (C, D, A, B, G, H, E, F, 0x9EA1E762, W14, W14 ^ W02);
    R2 (B, C, D, A, F, G, H, E, 0x3D43CEC5, W15, W15 ^ W03);

    ctx->A ^= A;
    ctx->B ^= B;
    ctx->C ^= C;
    ctx->D ^= D;
    ctx->E ^= E;
    ctx->F ^= F;
    ctx->G ^= G;
    ctx->H ^= H;
  }
}
