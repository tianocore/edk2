/** @file
  Core Primitive Implementation of the Advanced Encryption Standard (AES) algorithm.
  Refer to FIPS PUB 197 ("Advanced Encryption Standard (AES)") for detailed algorithm 
  description of AES. 

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AesCore.h"

//
// Number of columns (32-bit words) comprising the State.
// AES_NB is a constant (value = 4) for NIST FIPS-197.
//
#define AES_NB                     4

//
// Pre-computed AES Forward Table: AesForwardTable[t] = AES_SBOX[t].[02, 01, 01, 03]
// AES_SBOX (AES S-box) is defined in sec 5.1.1 of FIPS PUB 197.
// This is to speed up execution of the cipher by combining SubBytes and
// ShiftRows with MixColumns steps and transforming them into table lookups.
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT32 AesForwardTable[] = {
  0xc66363a5, 0xf87c7c84, 0xee777799, 0xf67b7b8d, 0xfff2f20d, 0xd66b6bbd, 
  0xde6f6fb1, 0x91c5c554, 0x60303050, 0x02010103, 0xce6767a9, 0x562b2b7d,
  0xe7fefe19, 0xb5d7d762, 0x4dababe6, 0xec76769a, 0x8fcaca45, 0x1f82829d, 
  0x89c9c940, 0xfa7d7d87, 0xeffafa15, 0xb25959eb, 0x8e4747c9, 0xfbf0f00b,
  0x41adadec, 0xb3d4d467, 0x5fa2a2fd, 0x45afafea, 0x239c9cbf, 0x53a4a4f7, 
  0xe4727296, 0x9bc0c05b, 0x75b7b7c2, 0xe1fdfd1c, 0x3d9393ae, 0x4c26266a,
  0x6c36365a, 0x7e3f3f41, 0xf5f7f702, 0x83cccc4f, 0x6834345c, 0x51a5a5f4, 
  0xd1e5e534, 0xf9f1f108, 0xe2717193, 0xabd8d873, 0x62313153, 0x2a15153f,
  0x0804040c, 0x95c7c752, 0x46232365, 0x9dc3c35e, 0x30181828, 0x379696a1, 
  0x0a05050f, 0x2f9a9ab5, 0x0e070709, 0x24121236, 0x1b80809b, 0xdfe2e23d,
  0xcdebeb26, 0x4e272769, 0x7fb2b2cd, 0xea75759f, 0x1209091b, 0x1d83839e, 
  0x582c2c74, 0x341a1a2e, 0x361b1b2d, 0xdc6e6eb2, 0xb45a5aee, 0x5ba0a0fb,
  0xa45252f6, 0x763b3b4d, 0xb7d6d661, 0x7db3b3ce, 0x5229297b, 0xdde3e33e, 
  0x5e2f2f71, 0x13848497, 0xa65353f5, 0xb9d1d168, 0x00000000, 0xc1eded2c,
  0x40202060, 0xe3fcfc1f, 0x79b1b1c8, 0xb65b5bed, 0xd46a6abe, 0x8dcbcb46, 
  0x67bebed9, 0x7239394b, 0x944a4ade, 0x984c4cd4, 0xb05858e8, 0x85cfcf4a,
  0xbbd0d06b, 0xc5efef2a, 0x4faaaae5, 0xedfbfb16, 0x864343c5, 0x9a4d4dd7, 
  0x66333355, 0x11858594, 0x8a4545cf, 0xe9f9f910, 0x04020206, 0xfe7f7f81,
  0xa05050f0, 0x783c3c44, 0x259f9fba, 0x4ba8a8e3, 0xa25151f3, 0x5da3a3fe, 
  0x804040c0, 0x058f8f8a, 0x3f9292ad, 0x219d9dbc, 0x70383848, 0xf1f5f504,
  0x63bcbcdf, 0x77b6b6c1, 0xafdada75, 0x42212163, 0x20101030, 0xe5ffff1a, 
  0xfdf3f30e, 0xbfd2d26d, 0x81cdcd4c, 0x180c0c14, 0x26131335, 0xc3ecec2f,
  0xbe5f5fe1, 0x359797a2, 0x884444cc, 0x2e171739, 0x93c4c457, 0x55a7a7f2, 
  0xfc7e7e82, 0x7a3d3d47, 0xc86464ac, 0xba5d5de7, 0x3219192b, 0xe6737395,
  0xc06060a0, 0x19818198, 0x9e4f4fd1, 0xa3dcdc7f, 0x44222266, 0x542a2a7e, 
  0x3b9090ab, 0x0b888883, 0x8c4646ca, 0xc7eeee29, 0x6bb8b8d3, 0x2814143c,
  0xa7dede79, 0xbc5e5ee2, 0x160b0b1d, 0xaddbdb76, 0xdbe0e03b, 0x64323256, 
  0x743a3a4e, 0x140a0a1e, 0x924949db, 0x0c06060a, 0x4824246c, 0xb85c5ce4,
  0x9fc2c25d, 0xbdd3d36e, 0x43acacef, 0xc46262a6, 0x399191a8, 0x319595a4, 
  0xd3e4e437, 0xf279798b, 0xd5e7e732, 0x8bc8c843, 0x6e373759, 0xda6d6db7,
  0x018d8d8c, 0xb1d5d564, 0x9c4e4ed2, 0x49a9a9e0, 0xd86c6cb4, 0xac5656fa, 
  0xf3f4f407, 0xcfeaea25, 0xca6565af, 0xf47a7a8e, 0x47aeaee9, 0x10080818,
  0x6fbabad5, 0xf0787888, 0x4a25256f, 0x5c2e2e72, 0x381c1c24, 0x57a6a6f1, 
  0x73b4b4c7, 0x97c6c651, 0xcbe8e823, 0xa1dddd7c, 0xe874749c, 0x3e1f1f21,
  0x964b4bdd, 0x61bdbddc, 0x0d8b8b86, 0x0f8a8a85, 0xe0707090, 0x7c3e3e42, 
  0x71b5b5c4, 0xcc6666aa, 0x904848d8, 0x06030305, 0xf7f6f601, 0x1c0e0e12,
  0xc26161a3, 0x6a35355f, 0xae5757f9, 0x69b9b9d0, 0x17868691, 0x99c1c158, 
  0x3a1d1d27, 0x279e9eb9, 0xd9e1e138, 0xebf8f813, 0x2b9898b3, 0x22111133,
  0xd26969bb, 0xa9d9d970, 0x078e8e89, 0x339494a7, 0x2d9b9bb6, 0x3c1e1e22, 
  0x15878792, 0xc9e9e920, 0x87cece49, 0xaa5555ff, 0x50282878, 0xa5dfdf7a,
  0x038c8c8f, 0x59a1a1f8, 0x09898980, 0x1a0d0d17, 0x65bfbfda, 0xd7e6e631, 
  0x844242c6, 0xd06868b8, 0x824141c3, 0x299999b0, 0x5a2d2d77, 0x1e0f0f11,
  0x7bb0b0cb, 0xa85454fc, 0x6dbbbbd6, 0x2c16163a
};

//
// Round constant word array used in AES key expansion.
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT32 Rcon[] = {
  0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000,
  0x20000000, 0x40000000, 0x80000000, 0x1B000000, 0x36000000
};

//
// Rotates x right n bits (circular right shift operation)
//
#define ROTATE_RIGHT32(x, n)    (((x) >> (n)) | ((x) << (32-(n))))

//
// Loading & Storing 32-bit words in big-endian format: y[3..0] --> x; x --> y[3..0];
//
#define LOAD32H(x, y)    { x = ((UINT32)((y)[0] & 0xFF) << 24) | ((UINT32)((y)[1] & 0xFF) << 16) |  \
                               ((UINT32)((y)[2] & 0xFF) <<  8) | ((UINT32)((y)[3] & 0xFF)); }
#define STORE32H(x, y)   { (y)[0] = (UINT8)(((x) >> 24) & 0xFF); (y)[1] = (UINT8)(((x) >> 16) & 0xFF); \
                           (y)[2] = (UINT8)(((x) >>  8) & 0xFF); (y)[3] = (UINT8)((x)         & 0xFF); }

//
// Wrap macros for AES forward tables lookups
//
#define AES_FT0(x)  AesForwardTable[x]
#define AES_FT1(x)  ROTATE_RIGHT32(AesForwardTable[x],  8)
#define AES_FT2(x)  ROTATE_RIGHT32(AesForwardTable[x], 16)
#define AES_FT3(x)  ROTATE_RIGHT32(AesForwardTable[x], 24)

///
/// AES Key Schedule which is expanded from symmetric key [Size 60 = 4 * ((Max AES Round, 14) + 1)].
///
typedef struct {
  UINTN     Nk;            // Number of Cipher Key (in 32-bit words);
  UINT32    EncKey[60];    // Expanded AES encryption key
  UINT32    DecKey[60];    // Expanded AES decryption key (Not used here)
} AES_KEY;

/**
  AES Key Expansion. 
  This function expands the cipher key into encryption schedule.

  @param[in]  Key                AES symmetric key buffer.
  @param[in]  KeyLenInBits       Key length in bits (128, 192, or 256).
  @param[out] AesKey             Expanded AES Key schedule for encryption.

  @retval EFI_SUCCESS            AES key expansion succeeded.
  @retval EFI_INVALID_PARAMETER  Unsupported key length.

**/
EFI_STATUS
EFIAPI
AesExpandKey (
  IN UINT8         *Key,
  IN UINTN         KeyLenInBits,
  OUT AES_KEY      *AesKey
  )
{
  UINTN       Nk;
  UINTN       Nr;
  UINTN       Nw;
  UINTN       Index1;
  UINTN       Index2;
  UINTN       Index3;
  UINT32      *Ek;
  UINT32      Temp;

  //
  // Nk - Number of 32-bit words comprising the cipher key. (Nk = 4, 6 or 8)
  // Nr - Number of rounds. (Nr = 10, 12, or 14), which is dependent on the key size.
  //
  Nk = KeyLenInBits >> 5;
  if (Nk != 4 && Nk != 6 && Nk != 8) {
    return EFI_INVALID_PARAMETER;
  }
  Nr = Nk + 6;
  Nw = AES_NB * (Nr + 1);    // Key Expansion generates a total of Nb * (Nr + 1) words
  AesKey->Nk = Nk;

  //
  // Load initial symmetric AES key;
  // Note that AES was designed on big-endian systems.
  //
  Ek = AesKey->EncKey;
  for (Index1 = Index2 = 0; Index1 < Nk; Index1++, Index2 += 4) {
    LOAD32H (Ek[Index1], Key + Index2);
  }
  
  //
  // Initialize the encryption key scheduler
  //
  for (Index2 = Nk, Index3 = 0; Index2 < Nw; Index2 += Nk, Index3++) {
    Temp       = Ek[Index2 - 1];
    Ek[Index2] = Ek[Index2 - Nk] ^ (AES_FT2((Temp >> 16) & 0xFF) & 0xFF000000) ^
                                   (AES_FT3((Temp >>  8) & 0xFF) & 0x00FF0000) ^
                                   (AES_FT0((Temp)       & 0xFF) & 0x0000FF00) ^
                                   (AES_FT1((Temp >> 24) & 0xFF) & 0x000000FF) ^
                                   Rcon[Index3];
    if (Nk <= 6) {
      //
      // If AES Cipher Key is 128 or 192 bits
      //
      for (Index1 = 1; Index1 < Nk && (Index1 + Index2) < Nw; Index1++) {
        Ek [Index1 + Index2] = Ek [Index1 + Index2 - Nk] ^ Ek[Index1 + Index2 - 1];
      }
    } else {
      //
      // Different routine for key expansion If Cipher Key is 256 bits, 
      //
      for (Index1 = 1; Index1 < 4 && (Index1 + Index2) < Nw; Index1++) {
        Ek [Index1 + Index2] = Ek[Index1 + Index2 - Nk] ^ Ek[Index1 + Index2 - 1];
      }
      if (Index2 + 4 < Nw) {
        Temp           = Ek[Index2 + 3];
        Ek[Index2 + 4] = Ek[Index2 + 4 - Nk] ^ (AES_FT2((Temp >> 24) & 0xFF) & 0xFF000000) ^
                                               (AES_FT3((Temp >> 16) & 0xFF) & 0x00FF0000) ^
                                               (AES_FT0((Temp >>  8) & 0xFF) & 0x0000FF00) ^
                                               (AES_FT1((Temp)       & 0xFF) & 0x000000FF);
      }
      
      for (Index1 = 5; Index1 < Nk && (Index1 + Index2) < Nw; Index1++) {
        Ek[Index1 + Index2] = Ek[Index1 + Index2 - Nk] ^ Ek[Index1 + Index2 - 1];
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Encrypts one single block data (128 bits) with AES algorithm.

  @param[in]  Key                AES symmetric key buffer.
  @param[in]  InData             One block of input plaintext to be encrypted.
  @param[out] OutData            Encrypted output ciphertext.

  @retval EFI_SUCCESS            AES Block Encryption succeeded.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
AesEncrypt (
  IN  UINT8        *Key,
  IN  UINT8        *InData,
  OUT UINT8        *OutData
  )
{
  AES_KEY  AesKey;
  UINTN    Nr;
  UINT32   *Ek;
  UINT32   State[4];
  UINT32   TempState[4];
  UINT32   *StateX;
  UINT32   *StateY;
  UINT32   *Temp;
  UINTN    Index;
  UINTN    NbIndex;
  UINTN    Round;

  if ((Key == NULL) || (InData == NULL) || (OutData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Expands AES Key for encryption.
  //
  AesExpandKey (Key, 128, &AesKey);

  Nr = AesKey.Nk + 6;
  Ek = AesKey.EncKey;

  //
  // Initialize the cipher State array with the initial round key
  //
  for (Index = 0; Index < AES_NB; Index++) {
    LOAD32H (State[Index], InData + 4 * Index);
    State[Index] ^= Ek[Index];
  }

  NbIndex = AES_NB;
  StateX  = State;
  StateY  = TempState;

  //
  // AES Cipher transformation rounds (Nr - 1 rounds), in which SubBytes(), 
  // ShiftRows() and MixColumns() operations were combined by a sequence of 
  // table lookups to speed up the execution.
  //
  for (Round = 1; Round < Nr; Round++) {
    StateY[0] = AES_FT0 ((StateX[0] >> 24)       ) ^ AES_FT1 ((StateX[1] >> 16) & 0xFF) ^
                AES_FT2 ((StateX[2] >>  8) & 0xFF) ^ AES_FT3 ((StateX[3]      ) & 0xFF) ^ Ek[NbIndex];
    StateY[1] = AES_FT0 ((StateX[1] >> 24)       ) ^ AES_FT1 ((StateX[2] >> 16) & 0xFF) ^
                AES_FT2 ((StateX[3] >>  8) & 0xFF) ^ AES_FT3 ((StateX[0]      ) & 0xFF) ^ Ek[NbIndex + 1];
    StateY[2] = AES_FT0 ((StateX[2] >> 24)       ) ^ AES_FT1 ((StateX[3] >> 16) & 0xFF) ^
                AES_FT2 ((StateX[0] >>  8) & 0xFF) ^ AES_FT3 ((StateX[1]      ) & 0xFF) ^ Ek[NbIndex + 2];
    StateY[3] = AES_FT0 ((StateX[3] >> 24)       ) ^ AES_FT1 ((StateX[0] >> 16) & 0xFF) ^
                AES_FT2 ((StateX[1] >>  8) & 0xFF) ^ AES_FT3 ((StateX[2]      ) & 0xFF) ^ Ek[NbIndex + 3];

    NbIndex += 4;
    Temp = StateX; StateX = StateY; StateY = Temp;
  }

  //
  // Apply the final round, which does not include MixColumns() transformation
  //
  StateY[0] = (AES_FT2 ((StateX[0] >> 24)       ) & 0xFF000000) ^ (AES_FT3 ((StateX[1] >> 16) & 0xFF) & 0x00FF0000) ^
              (AES_FT0 ((StateX[2] >>  8) & 0xFF) & 0x0000FF00) ^ (AES_FT1 ((StateX[3]      ) & 0xFF) & 0x000000FF) ^
              Ek[NbIndex];
  StateY[1] = (AES_FT2 ((StateX[1] >> 24)       ) & 0xFF000000) ^ (AES_FT3 ((StateX[2] >> 16) & 0xFF) & 0x00FF0000) ^
              (AES_FT0 ((StateX[3] >>  8) & 0xFF) & 0x0000FF00) ^ (AES_FT1 ((StateX[0]      ) & 0xFF) & 0x000000FF) ^
              Ek[NbIndex + 1];
  StateY[2] = (AES_FT2 ((StateX[2] >> 24)       ) & 0xFF000000) ^ (AES_FT3 ((StateX[3] >> 16) & 0xFF) & 0x00FF0000) ^
              (AES_FT0 ((StateX[0] >>  8) & 0xFF) & 0x0000FF00) ^ (AES_FT1 ((StateX[1]      ) & 0xFF) & 0x000000FF) ^
              Ek[NbIndex + 2];
  StateY[3] = (AES_FT2 ((StateX[3] >> 24)       ) & 0xFF000000) ^ (AES_FT3 ((StateX[0] >> 16) & 0xFF) & 0x00FF0000) ^
              (AES_FT0 ((StateX[1] >>  8) & 0xFF) & 0x0000FF00) ^ (AES_FT1 ((StateX[2]      ) & 0xFF) & 0x000000FF) ^
              Ek[NbIndex + 3];

  //
  // Output the transformed result;
  //
  for (Index = 0; Index < AES_NB; Index++) {
    STORE32H (StateY[Index], OutData + 4 * Index);
  }

  return EFI_SUCCESS;
}