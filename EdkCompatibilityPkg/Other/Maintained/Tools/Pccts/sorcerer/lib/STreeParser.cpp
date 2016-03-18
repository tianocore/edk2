#include <stdio.h>
#include "STreeParser.h"

void STreeParser::
MATCH(SORASTBase *_t,int tok)
{
	if ( _t->type()!=tok )
	{
		if ( guessing ) _GUESS_FAIL;
		else mismatched_token(tok, _t);
	}
}

void STreeParser::
MATCHRANGE(SORASTBase *_t,int tok,int tok2)
{
	if ( _t->type()<tok || _t->type()>tok2 )
	{
		if ( guessing ) _GUESS_FAIL;
		else mismatched_range(tok, tok2, _t);
	}
}

void STreeParser::
WILDCARD(SORASTBase *_t)
{
	if ( _t==NULL )
	{
		if ( guessing ) _GUESS_FAIL;
		else missing_wildcard();
	}
}

void STreeParser::
mismatched_range(int looking_for, int upper_token, SORASTBase *found)
{
	if ( found!=NULL ) {
		fprintf(stderr,
				"parse error: expected token range %d..%d found token %d\n",
				looking_for, upper_token,
				found->type());
	}
	else {
		fprintf(stderr,
				"parse error: expected token range %d..%d found NULL tree\n",
				looking_for, upper_token);
	}
}

void STreeParser::
missing_wildcard()
{
	fprintf(stderr, "parse error: expected any token/tree found found NULL tree\n");
}

void STreeParser::
mismatched_token(int looking_for, SORASTBase *found)
{
	if ( found!=NULL ) {
		fprintf(stderr,
				"parse error: expected token %d found token %d\n",
				looking_for,
				found->type());
	}
	else {
		fprintf(stderr,
				"parse error: expected token %d found NULL tree\n",
				looking_for);
	}
}

void STreeParser::
no_viable_alt(char *rulename, SORASTBase *root)
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

void STreeParser::
panic(char *err)
{
	fprintf(stderr, "panic: %s\n", err);
	exit(-1);
}

void STreeParser::
save_state(STreeParser *buf)
{
	buf->try_ok = this->try_ok;
	buf->sjrv = this->sjrv;
	buf->guessing = this->guessing;
	buf->startofguess = this->startofguess;
}

void STreeParser::
restore_state(STreeParser *buf)
{
	this->try_ok = buf->try_ok;
	this->sjrv = buf->sjrv;
	this->guessing = buf->guessing;
	this->startofguess = buf->startofguess;
}

void STreeParser::
_mkroot(SORASTBase **r, SORASTBase **s, SORASTBase **e, SORASTBase *t)
{
	*r = t;
}

void STreeParser::
_mkchild(SORASTBase **r, SORASTBase **s, SORASTBase **e, SORASTBase *t)
{
#ifdef BEFORE_GARYS_FIX
	/* if no sibling list, must attach to any existing root */
	if ( *s==NULL )
	{
		*s = *e = t;
		/* If r is NULL, then there was no root defined--must be sibling list */
		if ( *r==NULL ) *r = *s;
		else (*r)->setDown(t);
	}
	else { (*e)->setRight(t); *e = t; }
#endif
/*
	should do nothing if asked to add a NULL argument.  NULL's come up
	when a rule wants to return "nothing".
*/
	/* if no sibling list, must attach to any existing root */
	if (*s == NULL)
	{
		*s = *e = t;
		// If r is NULL then there was no root defined--must be sibling list
		if (*r == NULL)	*r = *s;
		else (*r)->setDown(t);
	}
	else if (*e != NULL)
	{
		(*e)->setRight(t);
		*e = t;
	}
	if (*e != NULL) {
		while ((*e)->right() != NULL) *e = (SORASTBase *)(*e)->right();
	}
}

