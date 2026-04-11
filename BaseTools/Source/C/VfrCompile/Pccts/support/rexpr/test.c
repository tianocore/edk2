#include <stdio.h>
#include "rexpr.h"

/*
 * test for rexpr().
 * To make this test:
 *	cc -o rexpr test.c rexpr.c
 * Then from command line type:
 *	rexpr r string
 * where r is the regular expression that decribes a language
 * and string is the string to verify.
 */
main(argc,argv)
int argc;
char *argv[];
{
	if ( argc!=3 ) fprintf(stderr,"rexpr: expr s\n");
	else printf("%d\n", rexpr(argv[1], argv[2]));
}
