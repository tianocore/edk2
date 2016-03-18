/*
 * sorcerer.c -- support code for SORCERER output
 *
 * Define your own or compile and link this in.
 *
 * Terence Parr
 * U of MN, AHPCRC
 * February 1994
 */

/***********************************************************************
  2-Oct-97  The routine ast_dup() appeared to have a bug in it.  Instead
            of providing a deep copy of its argument it made a bushy copy
            of its argument - by duplicating the nodes pointed to by
            its right link.  This is certainly not deliberate and does
            not match code in PCCTSAST.cpp (which had its own bug).  This
            has been changed to do a deep copy in the traditional sense.
***********************************************************************/

#ifdef OLD
/* Given a result pointer, return the same one if *t is NULL,
 * else find the end of the sibling list and return the address
 * the 'next[write]' field in that last node.
 */
AST **
#ifdef __USE_PROTOS
_nextresult(STreeParser *_parser, AST **t)
#else
_nextresult(_parser, t)
AST **t;
STreeParser *_parser;
#endif
{
  AST *p = *t;

  if ( p==NULL ) return t;
  while ( p->ast_right(_parser->write) != NULL )
  {
    p = p->ast_right(_parser->write);
  }
  return &(p->ast_right(_parser->write));
}

/*
 * Copy the read pointers to the write pointers for a node or entire subtree
 */
void
#ifdef __USE_PROTOS
_copy_wildcard(STreeParser *_parser, AST *t, int root)
#else
_copy_wildcard(_parser, t, root)
STreeParser *_parser;
AST *t;
int root;
#endif
{
  while ( t!=NULL )
  {
    if ( !root ) t->ast_right(_parser->write) = t->ast_right(_parser->read);
    t->ast_down(_parser->write) = t->ast_down(_parser->read);
    if ( t->ast_down(_parser->read)!=NULL )
      _copy_wildcard(_parser, t->ast_down(_parser->read), 0);
    if ( root ) return;
    else root=0;
    t = t->ast_right(_parser->read);
  }
}
#endif

void
#ifdef __USE_PROTOS
_mkroot(SORAST **r, SORAST **s, SORAST **e, SORAST *t)
#else
_mkroot(r,s,e,t)
SORAST **r, **s, **e, *t;
#endif
{
  *r = t;
}

void
#ifdef __USE_PROTOS
_mkchild(SORAST **r, SORAST **s, SORAST **e, SORAST *t)
#else
_mkchild(r,s,e,t)
SORAST **r, **s, **e, *t;
#endif
{
  /* if no sibling list, must attach to any existing root */
  if ( *s==NULL )
  {
    *s = *e = t;
    /* If r is NULL, then there was no root defined--must be sibling list */
    if ( *r==NULL ) *r = *s;
    else (*r)->ast_down = t;
  }
  else { (*e)->ast_right = t; *e = t; }
}

/* THESE FUNCS HAVE TO GO HERE BECAUSE THEY ARE SENSITIVE TO USER'S SORAST DEF */
SORAST *
#ifdef __USE_PROTOS
ast_alloc(void)
#else
ast_alloc()
#endif
{
  SORAST *t = (SORAST *)calloc(1, sizeof(SORAST));
  if ( t==NULL ) sorcerer_panic("out of memory");
  return t;
}

SORAST *
#ifdef __USE_PROTOS
ast_dup_bushy(SORAST *t)
#else
ast_dup_bushy(t)
SORAST *t;
#endif
{
  SORAST *u;
  
  if ( t == NULL ) return NULL;
  u = ast_alloc();
  *u = *t;  /* copy contents */
  u->ast_down = ast_dup_bushy(t->ast_down);    /* copy the rest of the tree */
  u->ast_right = ast_dup_bushy(t->ast_right);
  return u;
}


/* Assume t is a root node of a tree--duplicate that node and what's below */

SORAST *
#ifdef __USE_PROTOS
ast_dup(SORAST *t)
#else
ast_dup(t)
SORAST *t;
#endif
{
  SORAST *u;
  
  if ( t == NULL ) return NULL;
  u = ast_alloc();
  *u = *t;  /* copy contents */
  u->ast_down = ast_dup_bushy(t->ast_down);    /* copy the rest of the tree */
  u->ast_right = NULL;
  return u;
}

/* Assume t is a root node of a tree--duplicate that node and what's below */
SORAST *
#ifdef __USE_PROTOS
ast_dup_node(SORAST *t)
#else
ast_dup_node(t)
SORAST *t;
#endif
{
  SORAST *u;
  
  if ( t == NULL ) return NULL;
  u = ast_alloc();
  *u = *t;  /* copy contents */
  u->down = NULL;
  u->right = NULL;
  return u;
}
