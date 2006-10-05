/*
 * Simple symbol table manager using coalesced chaining to resolve collisions
 *
 * Doubly-linked lists are used for fast removal of entries.
 *
 * 'sym.h' must have a definition for typedef "Sym".  Sym must include at
 * minimum the following fields:
 *
 *		...
 *		char *symbol;
 *		struct ... *next, *prev, **head, *scope;
 *		unsigned int hash;
 *		...
 *
 * 'template.h' can be used as a template to create a 'sym.h'.
 *
 * 'head' is &(table[hash(itself)]).
 * The hash table is not resizable at run-time.
 * The scope field is used to link all symbols of a current scope together.
 * Scope() sets the current scope (linked list) to add symbols to.
 * Any number of scopes can be handled.  The user passes the address of
 * a pointer to a symbol table
 * entry (INITIALIZED TO NULL first time).
 *
 * Available Functions:
 *
 *	zzs_init(s1,s2)	--	Create hash table with size s1, string table size s2.
 *	zzs_done()		--	Free hash and string table created with zzs_init().
 *	zzs_add(key,rec)--	Add 'rec' with key 'key' to the symbol table.
 *	zzs_newadd(key)	--	create entry; add using 'key' to the symbol table.
 *	zzs_get(key)	--	Return pointer to last record entered under 'key'
 *						Else return NULL
 *	zzs_del(p)		--	Unlink the entry associated with p.  This does
 *						NOT free 'p' and DOES NOT remove it from a scope
 *						list.  If it was a part of your intermediate code
 *						tree or another structure.  It will still be there.
 *			  			It is only removed from further consideration
 *						by the symbol table.
 *	zzs_keydel(s)	--	Unlink the entry associated with key s.
 *						Calls zzs_del(p) to unlink.
 *	zzs_scope(sc)	--	Specifies that everything added to the symbol
 *			   			table with zzs_add() is added to the list (scope)
 *						'sc'.  'sc' is of 'Sym **sc' type and must be
 *						initialized to NULL before trying to add anything
 *						to it (passing it to zzs_scope()).  Scopes can be
 *					    switched at any time and merely links a set of
 *						symbol table entries.  If a NULL pointer is
 *						passed, the current scope is returned.
 *	zzs_rmscope(sc)	--	Remove (zzs_del()) all elements of scope 'sc'
 *						from the symbol table.  The entries are NOT
 *						free()'d.  A pointer to the first
 *			   			element in the "scope" is returned.  The user
 *			   			can then manipulate the list as he/she chooses
 *			   			(such as freeing them all). NOTE that this
 *			   			function sets your scope pointer to NULL,
 *			   			but returns a pointer to the list for you to use.
 *	zzs_stat()		--	Print out the symbol table and some relevant stats.
 *	zzs_new(key)	--	Create a new record with calloc() of type Sym.
 *			   			Add 'key' to the string table and make the new
 *			   			records 'symbol' pointer point to it.
 *	zzs_strdup(s)	--	Add s to the string table and return a pointer
 *			   			to it.  Very fast allocation routine
 *						and does not require strlen() nor calloc().
 *
 * Example:
 *
 *	#include <stdio.h>
 *	#include "sym.h"
 *
 *	main()
 *	{
 *	    Sym *scope1=NULL, *scope2=NULL, *a, *p;
 *	
 *	    zzs_init(101, 100);
 *	
 *	    a = zzs_new("Apple");	zzs_add(a->symbol, a);	-- No scope
 *	    zzs_scope( &scope1 );	-- enter scope 1
 *	    a = zzs_new("Plum");	zzs_add(a->symbol, a);
 *	    zzs_scope( &scope2 );	-- enter scope 2
 *	    a = zzs_new("Truck");	zzs_add(a->symbol, a);
 *	
 *    	p = zzs_get("Plum");
 *    	if ( p == NULL ) fprintf(stderr, "Hmmm...Can't find 'Plum'\n");
 *	
 *    	p = zzs_rmscope(&scope1)
 *    	for (; p!=NULL; p=p->scope) {printf("Scope1:  %s\n", p->symbol);}
 *    	p = zzs_rmscope(&scope2)
 *    	for (; p!=NULL; p=p->scope) {printf("Scope2:  %s\n", p->symbol);}
 * }
 *
 * Terence Parr
 * Purdue University
 * February 1990
 *
 * CHANGES
 *
 *	Terence Parr
 *	May 1991
 *		Renamed functions to be consistent with ANTLR
 *		Made HASH macro
 *		Added zzs_keydel()
 *		Added zzs_newadd()
 *		Fixed up zzs_stat()
 *
 *	July 1991
 *		Made symbol table entry save its hash code for fast comparison
 *			during searching etc...
 */

#include <stdio.h>
#if defined(__STDC__) || defined(__USE_PROTOS)
#include <string.h>
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include "sym.h"

#define StrSame		0

static Sym **CurScope = NULL;
static unsigned size = 0;
static Sym **table=NULL;
static char *strings;
static char *strp;
static int strsize = 0;

#ifdef __USE_PROTOS
void zzs_init(int sz,int strs)
#else
void zzs_init(sz, strs)
int sz, strs;
#endif
{
	if ( sz <= 0 || strs <= 0 ) return;
	table = (Sym **) calloc(sz, sizeof(Sym *));
	if ( table == NULL )
	{
		fprintf(stderr, "Cannot allocate table of size %d\n", sz);
		exit(1);
	}
	strings = (char *) calloc(strs, sizeof(char));
	if ( strings == NULL )
	{
		fprintf(stderr, "Cannot allocate string table of size %d\n", strs);
		exit(1);
	}
	size = sz;
	strsize = strs;
	strp = strings;
}

#ifdef __USE_PROTOS
void zzs_done(void)
#else
void zzs_done()
#endif
{
	if ( table != NULL ) free( table );
	if ( strings != NULL ) free( strings );
}

#ifdef __USE_PROTOS
void zzs_add(char *key,Sym rec)
#else
void zzs_add(key, rec)
char *key;
register Sym *rec;
#endif
{
	register unsigned int h=0;
	register char *p=key;
	
	HASH(p, h);
	rec->hash = h;					/* save hash code for fast comp later */
	h %= size;
	
	if ( CurScope != NULL ) {rec->scope = *CurScope; *CurScope = rec;}
	rec->next = table[h];			/* Add to doubly-linked list */
	rec->prev = NULL;
	if ( rec->next != NULL ) (rec->next)->prev = rec;
	table[h] = rec;
	rec->head = &(table[h]);
}

#ifdef __USE_PROTOS
Sym * zzs_get(char *key)
#else
Sym * zzs_get(key)
char *key;
#endif
{
	register unsigned int h=0;
	register char *p=key;
	register Sym *q;
	
	HASH(p, h);
	
	for (q = table[h%size]; q != NULL; q = q->next)
	{
		if ( q->hash == h )		/* do we even have a chance of matching? */
			if ( strcmp(key, q->symbol) == StrSame ) return( q );
	}
	return( NULL );
}

/*
 * Unlink p from the symbol table.  Hopefully, it's actually in the
 * symbol table.
 *
 * If p is not part of a bucket chain of the symbol table, bad things
 * will happen.
 *
 * Will do nothing if all list pointers are NULL
 */
#ifdef __USE_PROTOS
void zzs_del(Sym *p)
#else
void zzs_del(p)
register Sym *p;
#endif
{
	if ( p == NULL ) {fprintf(stderr, "zzs_del(NULL)\n"); exit(1);}
	if ( p->prev == NULL )	/* Head of list */
	{
		register Sym **t = p->head;
		
		if ( t == NULL ) return;	/* not part of symbol table */
		(*t) = p->next;
		if ( (*t) != NULL ) (*t)->prev = NULL;
	}
	else
	{
		(p->prev)->next = p->next;
		if ( p->next != NULL ) (p->next)->prev = p->prev;
	}
	p->next = p->prev = NULL;	/* not part of symbol table anymore */
	p->head = NULL;
}

#ifdef __USE_PROTOS
void zzs_keydel(char *key)
#else
void zzs_keydel(key)
char *key;
#endif
{
	Sym *p = zzs_get(key);

	if ( p != NULL ) zzs_del( p );
}

/* S c o p e  S t u f f */

/* Set current scope to 'scope'; return current scope if 'scope' == NULL */

#ifdef __USE_PROTOS
Sym ** zzs_scope(Sym **scope)
#else
Sym ** zzs_scope(scope)
Sym **scope;
#endif
{
	if ( scope == NULL ) return( CurScope );
	CurScope = scope;
	return( scope );
}

/* Remove a scope described by 'scope'.  Return pointer to 1st element in scope */

#ifdef __USE_PROTOS
Sym * zzs_rmscope(Sym **scope)
#else
Sym * zzs_rmscope(scope)
register Sym **scope;
#endif
{
	register Sym *p;
	Sym *start;

	if ( scope == NULL ) return(NULL);
	start = p = *scope;
	for (; p != NULL; p=p->scope) { zzs_del( p ); }
	*scope = NULL;
	return( start );
}

#ifdef __USE_PROTOS
void zzs_stat(void)
#else
void zzs_stat()
#endif
{
	static unsigned short count[20];
	unsigned int i,n=0,low=0, hi=0;
	register Sym **p;
	float avg=0.0;
	
	for (i=0; i<20; i++) count[i] = 0;
	for (p=table; p<&(table[size]); p++)
	{
		register Sym *q = *p;
		unsigned int len;
		
		if ( q != NULL && low==0 ) low = p-table;
		len = 0;
		if ( q != NULL ) printf("[%d]", p-table);
		while ( q != NULL )
		{
			len++;
			n++;
			printf(" %s", q->symbol);
			q = q->next;
			if ( q == NULL ) printf("\n");
		}
		if ( len>=20 ) printf("zzs_stat: count table too small\n");
		else count[len]++;
		if ( *p != NULL ) hi = p-table;
	}

	printf("Storing %d recs used %d hash positions out of %d\n",
			n, size-count[0], size);
	printf("%f %% utilization\n",
			((float)(size-count[0]))/((float)size));
	for (i=0; i<20; i++)
	{
		if ( count[i] != 0 )
		{
			avg += (((float)(i*count[i]))/((float)n)) * i;
			printf("Buckets of len %d == %d (%f %% of recs)\n",
					i, count[i], 100.0*((float)(i*count[i]))/((float)n));
		}
	}
	printf("Avg bucket length %f\n", avg);
	printf("Range of hash function: %d..%d\n", low, hi);
}

/*
 * Given a string, this function allocates and returns a pointer to a
 * symbol table record whose "symbol" pointer is reset to a position
 * in the string table.
 */

#ifdef __USE_PROTOS
Sym * zzs_new(char *text)
#else
Sym * zzs_new(text)
char *text;
#endif
{
	Sym *p;
	
	if ( (p = (Sym *) calloc(1,sizeof(Sym))) == 0 )
	{
		fprintf(stderr,"Out of memory\n");
		exit(1);
	}
	p->symbol = zzs_strdup(text);
	
	return p;
}

/* create a new symbol table entry and add it to the symbol table */

#ifdef __USE_PROTOS
Sym * zzs_newadd(char *text)
#else
Sym * zzs_newadd(text)
char *text;
#endif
{
	Sym *p = zzs_new(text);
	if ( p != NULL ) zzs_add(text, p);
	return p;
}

/* Add a string to the string table and return a pointer to it.
 * Bump the pointer into the string table to next avail position.
 */

#ifdef __USE_PROTOS
char * zzs_strdup(char *s)
#else
char * zzs_strdup(s)
register char *s;
#endif
{
	register char *start=strp;

	while ( *s != '\0' )
	{
		if ( strp >= &(strings[strsize-2]) )
		{
			fprintf(stderr, "sym: string table overflow (%d chars)\n", strsize);
			exit(-1);
		}
		*strp++ = *s++;
	}
	*strp++ = '\0';

	return( start );
}
