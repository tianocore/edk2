//
// Quick hack to work around not having sed, or any other reasonable 
// way to edit a file from a script on Windows......
//
// Copyright (c) 2010, Apple Inc. All rights reserved.
//  
//  All rights reserved. This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define TRUE  1
#define FALSE 0

typedef struct {
  char  *Match;
  int   MatchSize;
  char  *Replace;
} MATCH_PAIR;

//
// argv[1] - Old File
// argv[2] - New File
// argv[3+n] - Match String
// argv[4+n] - Replace string 
int
main (int argc, char **argv)
{
  FILE *In, *Out;
  char *Key, *Replace;
  int  c, i, n, Len, MaxLenKey = 0, MinLenKey = INT_MAX;
  unsigned long  InFileSize, InFilePos;
  MATCH_PAIR *Match;
  int MaxMatch;
  int ReadCount;
  int Found;

  if (argc < 5) {
    // Need at least two files and two strings
    return -1;
  } else if ((argc % 2) == 0) {
    // Match and Replace string must come in pairs
    return -4;
  }

  In  = fopen (argv[1], "r");
  fseek (In, 0, SEEK_END);
  InFileSize = ftell (In);
  if (InFileSize == 0) {
    return -6;
  }
  fseek (In, 0, SEEK_SET);


  Out = fopen (argv[2], "w+");
  if ((In == NULL) || (Out == NULL)) {
    return -2;
  }

  MaxMatch = (argc - 2)/2;
  printf ("\nMaxMatch = %d:%d\n", MaxMatch, argc);
  Match = calloc (MaxMatch, sizeof (MATCH_PAIR));
  if (Match == NULL) {
    return -7;
  }

  for (n=0; n < MaxMatch; n++) {
    Match[n].Match   = argv[3 + n*2];
    Match[n].MatchSize = strlen (argv[3 + n*2]);
    Match[n].Replace = argv[3 + n*2 + 1];
printf ("%s > %s\n", Match[n].Match, Match[n].Replace);
    if (Match[n].MatchSize > MaxLenKey) {
      // Max size of match/replace string pair
      MaxLenKey = Match[n].MatchSize;
    }
    if (Match[n].MatchSize < MinLenKey) {
      MinLenKey = Match[n].MatchSize;
    }
  }

  Key = malloc (MaxLenKey);
  if (Key == NULL) {
    return -5;
  }

  InFilePos = 0;
  while (InFilePos < (InFileSize - MinLenKey)) {
    fseek (In, InFilePos, SEEK_SET);
    ReadCount = fread (Key, 1, MaxLenKey, In);
    for (i = 0, Found = FALSE;i < MaxMatch; i++) {
      if (ReadCount >= Match[i].MatchSize) {
        if (!memcmp (Key, Match[i].Match, Match[i].MatchSize)) {
          printf ("Found [%s] @ %u\n", Match[i].Match, InFilePos);
          InFilePos += (Match[i].MatchSize - 1);
          printf ("InFilePos = %u", InFilePos);
          fputs (Match[i].Replace, Out);
          Found = TRUE;
          break;
        }
      }
    }
    if (!Found) {
      fputc (Key[0], Out);
    }
 
    InFilePos++;
  }

 
  fclose (In);
  fclose (Out);
  free (Key);
  return 0;
}

