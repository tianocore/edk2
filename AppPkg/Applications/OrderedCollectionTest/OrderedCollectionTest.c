/** @file
  A simple "fuzzer" application for OrderedCollectionLib, reading commands from
  the standard input, and writing results to the standard output.

  Make sure you configure your platform so that the console stderr device is
  visible to the user (or else run the program from wherever stderr is visible
  per default, eg. serial line).

  Copyright (C) 2014, Red Hat, Inc.
  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <assert.h> // assert()
#include <errno.h>  // errno
#include <limits.h> // INT_MIN
#include <stdio.h>  // fgets()
#include <stdlib.h> // EXIT_FAILURE
#include <string.h> // strerror()
#include <unistd.h> // getopt()

#include <Library/OrderedCollectionLib.h>

//
// We allow the user to select between stdin+stdout and regular input+output
// files via command line options. We don't rely on shell redirection for two
// reasons:
//
// - The "old shell" doesn't support input redirection (<a, <);
//
// - The "new shell" supports input redirection (<a, <), but those redirections
//   break fgets(stdin). Both when redirecting stdin from an ASCII file (<a),
//   and when redirecting stdin from a UCS-2 file (<), the very first fgets()
//   spirals into an infinite loop, spewing ^@ on the serial console.
//
// Performing fopen() manually (made available to the user with the -i option),
// and reading from that stream with fgets() work under both old and new shell.
// Only ASCII encoded files are supported.
//
static FILE *Input, *Output;


//
// Define a (potentially aggregate) key type.
//
typedef struct {
  int Value;
} USER_KEY;

//
// The user structure includes the key as one of its fields. (There can be
// several, optionally differently typed, keys, if we link user structures into
// several collections, with different comparators.)
//
typedef struct {
  unsigned char  Supplementary1[4];
  USER_KEY       Key;
  unsigned short Supplementary2[2];
} USER_STRUCT;


/**
  Compare a standalone key against a user structure containing an embedded key.

  @param[in] StandaloneKey  Pointer to the bare key.

  @param[in] UserStruct     Pointer to the user structure with the embedded
                            key.

  @retval <0  If StandaloneKey compares less than UserStruct's key.

  @retval  0  If StandaloneKey compares equal to UserStruct's key.

  @retval >0  If StandaloneKey compares greater than UserStruct's key.
**/
static
INTN
EFIAPI
KeyCompare (
  IN CONST VOID *StandaloneKey,
  IN CONST VOID *UserStruct
  )
{
  const USER_KEY    *CmpKey;
  const USER_STRUCT *CmpStruct;

  CmpKey    = StandaloneKey;
  CmpStruct = UserStruct;

  return CmpKey->Value < CmpStruct->Key.Value ? -1 :
         CmpKey->Value > CmpStruct->Key.Value ?  1 :
         0;
}


/**
  Comparator function type for two user structures.

  @param[in] UserStruct1  Pointer to the first user structure.

  @param[in] UserStruct2  Pointer to the second user structure.

  @retval <0  If UserStruct1 compares less than UserStruct2.

  @retval  0  If UserStruct1 compares equal to UserStruct2.

  @retval >0  If UserStruct1 compares greater than UserStruct2.
**/
static
INTN
EFIAPI
UserStructCompare (
  IN CONST VOID *UserStruct1,
  IN CONST VOID *UserStruct2
  )
{
  const USER_STRUCT *CmpStruct1;

  CmpStruct1 = UserStruct1;
  return KeyCompare (&CmpStruct1->Key, UserStruct2);
}


/**
  Empty the collection by iterating forward through its entries.

  This function demonstrates that iterators different from the one being
  removed remain valid.

  @param[in,out] Collection  The collection to empty.
**/
static void
CmdForwardEmpty (
  IN OUT ORDERED_COLLECTION *Collection
  )
{
  ORDERED_COLLECTION_ENTRY *Entry;

  Entry = OrderedCollectionMin (Collection);
  while (Entry != NULL) {
    ORDERED_COLLECTION_ENTRY *Next;
    void                     *Ptr;
    USER_STRUCT              *UserStruct;

    Next = OrderedCollectionNext (Entry);
    OrderedCollectionDelete (Collection, Entry, &Ptr);

    UserStruct = Ptr;
    fprintf (Output, "%s: %d: removed\n", __FUNCTION__, UserStruct->Key.Value);
    free (UserStruct);

    Entry = Next;
  }
}


/**
  Empty the collection by iterating backward through its entries.

  This function demonstrates that iterators different from the one being
  removed remain valid.

  @param[in,out] Collection  The collection to empty.
**/
static void
CmdBackwardEmpty (
  IN OUT ORDERED_COLLECTION *Collection
  )
{
  ORDERED_COLLECTION_ENTRY *Entry;

  Entry = OrderedCollectionMax (Collection);
  while (Entry != NULL) {
    ORDERED_COLLECTION_ENTRY *Prev;
    void                     *Ptr;
    USER_STRUCT              *UserStruct;

    Prev = OrderedCollectionPrev (Entry);
    OrderedCollectionDelete (Collection, Entry, &Ptr);

    UserStruct = Ptr;
    fprintf (Output, "%s: %d: removed\n", __FUNCTION__, UserStruct->Key.Value);
    free (UserStruct);

    Entry = Prev;
  }
}


/**
  List the user structures linked into the collection, in increasing order.

  @param[in] Collection  The collection to list.
**/
static void
CmdForwardList (
  IN ORDERED_COLLECTION *Collection
  )
{
  ORDERED_COLLECTION_ENTRY *Entry;

  for (Entry = OrderedCollectionMin (Collection); Entry != NULL;
       Entry = OrderedCollectionNext (Entry)) {
    USER_STRUCT  *UserStruct;

    UserStruct = OrderedCollectionUserStruct (Entry);
    fprintf (Output, "%s: %d\n", __FUNCTION__, UserStruct->Key.Value);
  }
}


/**
  List the user structures linked into the collection, in decreasing order.

  @param[in] Collection  The collection to list.
**/
static void
CmdBackwardList (
  IN ORDERED_COLLECTION *Collection
  )
{
  ORDERED_COLLECTION_ENTRY *Entry;

  for (Entry = OrderedCollectionMax (Collection); Entry != NULL;
       Entry = OrderedCollectionPrev (Entry)) {
    USER_STRUCT  *UserStruct;

    UserStruct = OrderedCollectionUserStruct (Entry);
    fprintf (Output, "%s: %d\n", __FUNCTION__, UserStruct->Key.Value);
  }
}


/**
  Create a new user structure and attempt to insert it into the collection.

  @param[in]     Value       The key value of the user structure to create.

  @param[in,out] Collection  The collection to insert the new user structure
                             into.
**/
static void
CmdInsert (
  IN     int     Value,
  IN OUT ORDERED_COLLECTION *Collection
  )
{
  USER_STRUCT              *UserStruct, *UserStruct2;
  RETURN_STATUS            Status;
  ORDERED_COLLECTION_ENTRY *Entry;

  UserStruct = calloc (1, sizeof *UserStruct);
  if (UserStruct == NULL) {
    fprintf (Output, "%s: %d: calloc(): out of memory\n", __FUNCTION__, Value);
    return;
  }

  UserStruct->Key.Value = Value;
  Status = OrderedCollectionInsert (Collection, &Entry, UserStruct);

  switch (Status) {
  case RETURN_OUT_OF_RESOURCES:
    fprintf (Output, "%s: %d: OrderedCollectionInsert(): out of memory\n",
      __FUNCTION__, Value);
    goto ReleaseUserStruct;

  case RETURN_ALREADY_STARTED:
    UserStruct2 = OrderedCollectionUserStruct (Entry);
    assert (UserStruct != UserStruct2);
    assert (UserStruct2->Key.Value == Value);
    fprintf (Output, "%s: %d: already exists\n", __FUNCTION__,
      UserStruct2->Key.Value);
    goto ReleaseUserStruct;

  default:
    assert (Status == RETURN_SUCCESS);
    break;
  }

  assert (OrderedCollectionUserStruct (Entry) == UserStruct);
  fprintf (Output, "%s: %d: inserted\n", __FUNCTION__, Value);
  return;

ReleaseUserStruct:
  free (UserStruct);
}


/**
  Look up a user structure by key in the collection and print it.

  @param[in] Value       The key of the user structure to find.

  @param[in] Collection  The collection to search for the user structure with
                         the key.
**/
static void
CmdFind (
  IN int                Value,
  IN ORDERED_COLLECTION *Collection
  )
{
  USER_KEY                 StandaloneKey;
  ORDERED_COLLECTION_ENTRY *Entry;
  USER_STRUCT              *UserStruct;

  StandaloneKey.Value = Value;
  Entry = OrderedCollectionFind (Collection, &StandaloneKey);

  if (Entry == NULL) {
    fprintf (Output, "%s: %d: not found\n", __FUNCTION__, Value);
    return;
  }

  UserStruct = OrderedCollectionUserStruct (Entry);
  assert (UserStruct->Key.Value == StandaloneKey.Value);
  fprintf (Output, "%s: %d: found\n", __FUNCTION__, UserStruct->Key.Value);
}


/**
  Look up a user structure by key in the collection and delete it.

  @param[in] Value       The key of the user structure to find.

  @param[in] Collection  The collection to search for the user structure with
                         the key.
**/
static void
CmdDelete (
  IN int                Value,
  IN ORDERED_COLLECTION *Collection
  )
{
  USER_KEY                 StandaloneKey;
  ORDERED_COLLECTION_ENTRY *Entry;
  void                     *Ptr;
  USER_STRUCT              *UserStruct;

  StandaloneKey.Value = Value;
  Entry = OrderedCollectionFind (Collection, &StandaloneKey);

  if (Entry == NULL) {
    fprintf (Output, "%s: %d: not found\n", __FUNCTION__, Value);
    return;
  }

  OrderedCollectionDelete (Collection, Entry, &Ptr);

  UserStruct = Ptr;
  assert (UserStruct->Key.Value == StandaloneKey.Value);
  fprintf (Output, "%s: %d: removed\n", __FUNCTION__, UserStruct->Key.Value);
  free (UserStruct);
}


typedef struct {
  const char *Command;
  void       (*Function) (ORDERED_COLLECTION *Collection);
  const char *Description;
} KEYLESS_COMMAND;

typedef struct {
  const char *Command;
  void       (*Function) (int Value, ORDERED_COLLECTION *Collection);
  const char *Description;
} KEYED_COMMAND;

static const KEYLESS_COMMAND KeylessCommands[] = {
  { "forward-empty",  CmdForwardEmpty,
                      "empty the collection iterating forward" },
  { "fe",             CmdForwardEmpty,
                      "shorthand for forward-empty" },
  { "backward-empty", CmdBackwardEmpty,
                      "empty the collection iterating backward" },
  { "be",             CmdBackwardEmpty,
                      "shorthand for backward-empty" },
  { "forward-list",   CmdForwardList,
                      "list contents, iterating forward" },
  { "fl",             CmdForwardList,
                      "shorthand for forward-list" },
  { "backward-list",  CmdBackwardList,
                      "list contents, iterating backward" },
  { "bl",             CmdBackwardList,
                      "shorthand for backward-list" },
  { NULL }
};

static const KEYED_COMMAND KeyedCommands[] = {
  { "insert ", CmdInsert, "insert value into collection" },
  { "i ",      CmdInsert, "shorthand for insert"         },
  { "find ",   CmdFind,   "find value in collection"     },
  { "f ",      CmdFind,   "shorthand for find"           },
  { "delete ", CmdDelete, "delete value from collection" },
  { "d ",      CmdDelete, "shorthand for delete"         },
  { NULL }
};


/**
  List the supported commands on stderr.
**/
static void
ListCommands (
  void
  )
{
  const KEYLESS_COMMAND *KeylessCmd;
  const KEYED_COMMAND   *KeyedCmd;

  fprintf (stderr, "Supported commands:\n\n");
  for (KeylessCmd = KeylessCommands; KeylessCmd->Command != NULL;
       ++KeylessCmd) {
    fprintf (stderr, "%-14s: %s\n", KeylessCmd->Command,
      KeylessCmd->Description);
  }
  for (KeyedCmd = KeyedCommands; KeyedCmd->Command != NULL; ++KeyedCmd) {
    fprintf (stderr, "%-9s<int>: %s\n", KeyedCmd->Command,
      KeyedCmd->Description);
  }
}


/**
  Configure stdio FILEs that we'll use for input and output.

  @param[in] ArgC  The number of elements in ArgV, from main(). The environment
                   is required to ensure ArgC >= 1 (ie. that the program name,
                   ArgV[0], is available).

  @param[in] ArgV  Command line argument list, from main().
**/
static void
SetupInputOutput (
  IN int  ArgC,
  IN char **ArgV
  )
{
  char *InputName, *OutputName;
  int  Loop;

  assert (ArgC >= 1);

  InputName  = NULL;
  OutputName = NULL;
  Loop       = 1;

  while (Loop) {
    switch (getopt (ArgC, ArgV, ":i:o:h")) {
    case 'i':
      InputName = optarg;
      break;

    case 'o':
      OutputName = optarg;
      break;

    case 'h':
      fprintf (stderr,
        "%1$s: simple OrderedCollectionLib tester\n"
        "\n"
        "Usage: 1. %1$s [-i InputFile] [-o OutputFile]\n"
        "       2. %1$s -h\n"
        "\n"
        "Options:\n"
        "  -i InputFile : read commands from InputFile\n"
        "                 (will read from stdin if absent)\n"
        "  -o OutputFile: write command responses to OutputFile\n"
        "                 (will write to stdout if absent)\n"
        "  -h           : print this help and exit\n"
        "\n", ArgV[0]);
      ListCommands ();
      exit (EXIT_SUCCESS);

//
// The current "compatibility" getopt() implementation doesn't support optopt,
// but it gracefully degrades these branches to the others (one of the optarg
// ones or the excess operands one).
//
#if 0
    case ':':
      fprintf (stderr, "%s: option -%c requires an argument; pass -h for "
        "help\n", ArgV[0], optopt);
      exit (EXIT_FAILURE);

    case '?':
      fprintf (stderr, "%s: unknown option -%c; pass -h for help\n", ArgV[0],
        optopt);
      exit (EXIT_FAILURE);
#endif

    case -1:
      if (optind != ArgC) {
        fprintf (stderr, "%s: excess operands on command line; pass -h for "
          "help\n", ArgV[0]);
        exit (EXIT_FAILURE);
      }
      Loop = 0;
      break;

    default:
      assert (0);
    }
  }

  if (InputName == NULL) {
    Input = stdin;
  } else {
    Input = fopen (InputName, "r");
    if (Input == NULL) {
      fprintf (stderr, "%s: fopen(\"%s\", \"r\"): %s\n", ArgV[0], InputName,
        strerror (errno));
      exit (EXIT_FAILURE);
    }
  }

  if (OutputName == NULL) {
    Output = stdout;
  } else {
    Output = fopen (OutputName, "w");
    if (Output == NULL) {
      fprintf (stderr, "%s: fopen(\"%s\", \"w\"): %s\n", ArgV[0], OutputName,
        strerror (errno));
      exit (EXIT_FAILURE);
    }
  }

  //
  // When reading commands from the standard input, assume interactive mode,
  // and list the supported commands. However, delay this until both streams
  // are set up.
  //
  if (InputName == NULL) {
    ListCommands ();
  }
}


int
main (
  IN int  ArgC,
  IN char **ArgV
  )
{
  int                RetVal;
  ORDERED_COLLECTION *Collection;
  char               Line[256];

  SetupInputOutput (ArgC, ArgV);

  Collection = OrderedCollectionInit (UserStructCompare, KeyCompare);
  if (Collection == NULL) {
    fprintf (stderr, "%s: OrderedCollectionInit(): out of memory\n",
      __FUNCTION__);
      return EXIT_FAILURE;
  }

  RetVal = EXIT_SUCCESS;
  while (fgets (Line, sizeof Line, Input) != NULL) {
    size_t                Length;
    const KEYLESS_COMMAND *KeylessCmd;
    const KEYED_COMMAND   *KeyedCmd;

    Length = strlen (Line);
    assert (Length > 0);
    if (Line[Length - 1] != '\n') {
      fprintf (stderr, "%s: overlong line\n", __FUNCTION__);
      RetVal = EXIT_FAILURE;
      break;
    }

    //
    // Strip [\r]\n.
    //
    Line[Length - 1] = '\0';
    if (Length >= 2 && Line[Length - 2] == '\r') {
      Line[Length - 2] = '\0';
    }
    //
    // Ignore empty lines and comments.
    //
    if (Line[0] == '\0' || Line[0] == '#') {
      if (Input != stdin) {
        //
        // ... but echo them back in non-interactive mode.
        //
        fprintf (Output, "%s\n", Line);
      }
      continue;
    }

    //
    // Ironically, this is the kind of loop that should be replaced with an
    // ORDERED_COLLECTION.
    //
    for (KeylessCmd = KeylessCommands; KeylessCmd->Command != NULL;
         ++KeylessCmd) {
      if (strcmp (KeylessCmd->Command, Line) == 0) {
        KeylessCmd->Function (Collection);
        break;
      }
    }
    if (KeylessCmd->Command != NULL) {
      continue;
    }

    for (KeyedCmd = KeyedCommands; KeyedCmd->Command != NULL; ++KeyedCmd) {
      size_t CmdLength;

      CmdLength = strlen (KeyedCmd->Command);
      assert (CmdLength >= 2);
      if (strncmp (KeyedCmd->Command, Line, CmdLength) == 0) {
        char *CommandArg, *EndPtr;
        long Value;

        CommandArg = Line + CmdLength;
        errno = 0;
        Value = strtol (CommandArg, &EndPtr, 10);
        if (EndPtr == CommandArg ||            // no conversion performed
            errno != 0 ||                      // not in long's range, etc
            *EndPtr != '\0' ||                 // final string not empty
            Value < INT_MIN || Value > INT_MAX // parsed long not in int range
            ) {
          fprintf (stderr, "%s: %.*s: \"%s\": not an int\n", __FUNCTION__,
            (int)(CmdLength - 1), Line, CommandArg);
        } else {
          KeyedCmd->Function (Value, Collection);
        }

        break;
      }
    }
    if (KeyedCmd->Command != NULL) {
      continue;
    }

    fprintf (stderr, "%s: \"%s\": unknown command\n", __FUNCTION__, Line);
  }

  if (RetVal == EXIT_SUCCESS && ferror (Input)) {
    fprintf (stderr, "%s: fgets(): %s\n", __FUNCTION__, strerror (errno));
    RetVal = EXIT_FAILURE;
  }

  CmdForwardEmpty (Collection);
  OrderedCollectionUninit (Collection);
  return RetVal;
}
