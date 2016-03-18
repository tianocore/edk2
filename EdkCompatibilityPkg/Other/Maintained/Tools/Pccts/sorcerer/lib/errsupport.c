/*
 * errsupport.c -- error support code for SORCERER output
 *
 * Define your own or compile and link this in.
 *
 * Terence Parr
 * U of MN, AHPCRC
 * February 1994
 */
#include "sorcerer.h"

void
#ifdef __USE_PROTOS
mismatched_range( STreeParser *_parser, int looking_for, int upper_token, SORAST *found )
#else
mismatched_range( _parser, looking_for, upper_token, found )
int looking_for;
int upper_token;
SORAST *found;
STreeParser *_parser;
#endif
{
  if ( found!=NULL ) {
    fprintf(stderr,
        "parse error: expected token range %d..%d found token %d\n",
        looking_for, upper_token,
        found->token);
  }
  else {
    fprintf(stderr,
        "parse error: expected token range %d..%d found NULL tree\n",
        looking_for, upper_token);
  }
}

void
#ifdef __USE_PROTOS
missing_wildcard(STreeParser *_parser)
#else
missing_wildcard(_parser)
STreeParser *_parser;
#endif
{
  fprintf(stderr, "parse error: expected any token/tree found found NULL tree\n");
}

void
#ifdef __USE_PROTOS
mismatched_token( STreeParser *_parser, int looking_for, SORAST *found )
#else
mismatched_token( _parser, looking_for, found )
int looking_for;
SORAST *found;
STreeParser *_parser;
#endif
{
  if ( found!=NULL ) {
    fprintf(stderr,
        "parse error: expected token %d found token %d\n",
        looking_for,
        found->token);
  }
  else {
    fprintf(stderr,
        "parse error: expected token %d found NULL tree\n",
        looking_for);
  }
}

void
#ifdef __USE_PROTOS
no_viable_alt( STreeParser *_parser, char *rulename, SORAST *root )
#else
no_viable_alt( _parser, rulename, root )
char *rulename;
SORAST *root;
STreeParser *_parser;
#endif
{
  if ( root==NULL )
    fprintf(stderr,
        "parse error: in rule %s, no viable alternative for NULL tree\n",
        rulename);
  else
    fprintf(stderr,
        "parse error: in rule %s, no viable alternative for tree\n",
        rulename);
}

void
#ifdef __USE_PROTOS
sorcerer_panic(char *err)
#else
sorcerer_panic(err)
char *err;
#endif
{
  fprintf(stderr, "panic: %s\n", err);
  exit(-1);
}
