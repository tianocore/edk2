//
// Quick hack to work around not having sed, or any other reasonable
// way to edit a file from a script on Windows......
//
// Copyright (c) 2010, Apple Inc. All rights reserved.<BR>
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
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

void
Usage (char *Name)
{
  printf ("\n%s OldFile NewFile MatchString ReplaceString [MatchString2 ReplaceString2]*\n", Name);
  printf ("    OldFile - Must be arg[1] File to search for MatchStrings\n");
  printf ("    NewFile - Must be arg[2] File where MatchString has been replaced with ReplaceString\n");
  printf ("    MatchString & ReplaceString. Required arguments.\n");
  printf ("    More MatchString/ReplaceString pairs are supported.\n");
}

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
    fprintf (stderr, "Need at least two files and one Match/Replacement string pair\n");
    Usage (argv[0]);
    return -1;
  } else if ((argc % 2) == 0) {
    fprintf (stderr, "Match and Replace string must come in pairs\n");
    return -4;
  }

  In  = fopen (argv[1], "r");
  fseek (In, 0, SEEK_END);
  InFileSize = ftell (In);
  if (InFileSize == 0) {
    fprintf (stderr, "Could not open %s\n", argv[1]);
    return -6;
  }
  fseek (In, 0, SEEK_SET);


  Out = fopen (argv[2], "w+");
  if ((In == NULL) || (Out == NULL)) {
    fprintf (stderr, "Could not open %s\n", argv[2]);
    return -2;
  }

  MaxMatch = (argc - 2)/2;
  Match = calloc (MaxMatch, sizeof (MATCH_PAIR));
  if (Match == NULL) {
    return -7;
  }

  for (n=0; n < MaxMatch; n++) {
    Match[n].Match   = argv[3 + n*2];
    Match[n].MatchSize = strlen (argv[3 + n*2]);
    Match[n].Replace = argv[3 + n*2 + 1];
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

  // Search for a match by reading every possition of the file
  // into a buffer that is as big as the maximum search key size.
  // Then we can search the keys for a match. If no match
  // copy the old file character to the new file. If it is a match
  // then copy the replacement string into the output file.
  // This code assumes the file system is smart and caches the
  // file in a buffer. So all the reads don't really hit the disk.
  InFilePos = 0;
  while (InFilePos < (InFileSize - MinLenKey)) {
    fseek (In, InFilePos, SEEK_SET);
    ReadCount = fread (Key, 1, MaxLenKey, In);
    for (i = 0, Found = FALSE;i < MaxMatch; i++) {
      if (ReadCount >= Match[i].MatchSize) {
        if (!memcmp (Key, Match[i].Match, Match[i].MatchSize)) {
          InFilePos += (Match[i].MatchSize - 1);
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

  // We stoped searching when we got to the point that we could no longer match.
  // So the last few bytes of the file are not copied in the privous loop
  fseek (In, InFilePos, SEEK_SET);
  while ((c = fgetc (In)) != EOF) {
    fputc (c, Out);
  }

  fclose (In);
  fclose (Out);
  free (Key);
  free (Match);
  return 0;
}

