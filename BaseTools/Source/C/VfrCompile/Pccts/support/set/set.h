#ifndef __GATE_SET_H
#define __GATE_SET_H

/*	set.h

	The following is a general-purpose set library originally developed
	by Hank Dietz and enhanced by Terence Parr to allow dynamic sets.
	
	Sets are now structs containing the #words in the set and
	a pointer to the actual set words.

	1987 by Hank Dietz
	
	Modified by:
		Terence Parr
		Purdue University
		October 1989

		Added ANSI prototyping Dec. 1992 -- TJP
*/

#include "pcctscfg.h"

#ifdef NOT_USED /* SEE config.h */
/* Define usable bits per unsigned int word */
#ifdef PC
#define WORDSIZE 16
#define LogWordSize	4
#else
#define	WORDSIZE 32
#define LogWordSize 5
#endif
#define BytesPerWord	sizeof(unsigned)
#endif

#define	SETSIZE(a) ((a).n<<LogWordSize)		/* Maximum items per set */
#define	MODWORD(x) ((x) & (WORDSIZE-1))		/* x % WORDSIZE */
#define	DIVWORD(x) ((x) >> LogWordSize)		/* x / WORDSIZE */
#define	nil	(~((unsigned) 0))	/* An impossible set member all bits on (big!) */

typedef struct _set {
			unsigned int n;		/* Number of words in set */
			unsigned *setword;
		} set;

#define set_init	{0, NULL}
#define set_null(a)	((a).setword==NULL)

#define	NumBytes(x)		(((x)>>3)+1)						/* Num bytes to hold x */
#define	NumWords(x)		((((unsigned)(x))>>LogWordSize)+1)	/* Num words to hold x */


/* M a c r o s */

/* make arg1 a set big enough to hold max elem # of arg2 */
#define set_new(a,_max) \
if (((a).setword=(unsigned *)calloc(NumWords(_max),BytesPerWord))==NULL) \
        fprintf(stderr, "set_new: Cannot allocate set with max of %d\n", _max); \
        (a).n = NumWords(_max);

#define set_free(a)									\
	{if ( (a).setword != NULL ) free((char *)((a).setword));	\
	(a) = empty;}

#ifdef __USE_PROTOS
extern void set_size( unsigned );
extern unsigned int set_deg( set );
extern set set_or( set, set );
extern set set_and( set, set );
extern set set_dif( set, set );
extern set set_of( unsigned );
extern void set_ext( set *, unsigned int );
extern set set_not( set );
extern int set_equ( set, set );
extern int set_sub( set, set );
extern unsigned set_int( set );
extern int set_el( unsigned, set );
extern int set_nil( set );
extern char * set_str( set );
extern set set_val( register char * );
extern void set_orel( unsigned, set * );
extern void set_orin( set *, set );
extern void set_andin( set *, set );
extern void set_rm( unsigned, set );
extern void set_clr( set );
extern set set_dup( set );
extern void set_PDQ( set, register unsigned * );
extern unsigned *set_pdq( set );
extern void _set_pdq( set a, register unsigned *q );
extern unsigned int set_hash( set, register unsigned int );
#else
extern void set_size();
extern unsigned int set_deg();
extern set set_or();
extern set set_and();
extern set set_dif();
extern set set_of();
extern void set_ext();
extern set set_not();
extern int set_equ();
extern int set_sub();
extern unsigned set_int();
extern int set_el();
extern int set_nil();
extern char * set_str();
extern set set_val();
extern void set_orel();
extern void set_orin();
extern void set_andin();
extern void set_rm();
extern void set_clr();
extern set set_dup();
extern void set_PDQ();
extern unsigned *set_pdq();
extern void _set_pdq();
extern unsigned int set_hash();
#endif

extern set empty;

#endif
