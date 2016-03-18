/** @file
    Every thing you wanted to know about your compiler but didn't know whom to ask.

    COPYRIGHT(c) 1993-9 Steven Pemberton, CWI. All rights reserved.
    Steven Pemberton, CWI, Amsterdam; "Steven.Pemberton@cwi.nl"
    Used with permission.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */
  #pragma warning ( disable : 4018 )
  #pragma warning ( disable : 4055 )
  #pragma warning ( disable : 4116 )
  #pragma warning ( disable : 4130 )
  #pragma warning ( disable : 4189 )
  #pragma warning ( disable : 4244 )
  #pragma warning ( disable : 4723 )
#endif  /* defined(_MSC_VER) */

//#define NO_SC   1   // Compiler doesn't support signed char
//#define NO_UC   1   // Compiler doesn't support unsigned char
//#define NO_UI   1   // Compiler doesn't support unsigned short and long
//#define NO_VOID 1   // Compiler doesn't support void
//#define NO_SIG  1   // Compiler doesn't support signal() or setjmp/longjmp()

/*  Some compilers can't cope with "#ifdef __FILE__". Use
    either the FILENAME or BAD_CPP macro as described below.
*/
/*  If your C preprocessor doesn't have the predefined __FILE__
macro, and you don't want to call this file enquire.c but, say,
tell.c, uncomment the following and change enquire.c to tell.c.
*/
//#define FILENAME "enquire.c"

/*  Some compilers won't accept the line "#include FILENAME".  Uncomment
    the following macro. In that case, this file *must* be called enquire.c.
*/
//#define BAD_CPP     1

/*  Some naughty compilers define __STDC__, but don't really
    support it.  Some define it as 0, in which case we ignore it.
    But if your compiler defines it, and isn't really ANSI C,
    uncomment the BAD_STDC macro. (To those compiler writers: for shame).
*/
//#define BAD_STDC    1

/*  Some naughty compilers define __STDC__, but don't have the
    stddef.h include file. Uncomment the BAD_STDDEF macro. (Gcc needs this on
    some machines, due to clashes between stddef.h and other include files.)
*/
//#define BAD_STDDEF  1

/*  Some systems crash when you try to malloc all store. To save users of such
    defective systems too much grief, they may uncomment the BAD_MALLOC macro,
    which ignores that bit of the code.
*/
//#define BAD_MALLOC  1



#ifndef PROGRAM
#define PROGRAM enquire.c
#define VERSION "5.1a"
#define PURPOSE Everything you wanted to know about your machine and C compiler
#define BUT didnt know who to ask
#define FOR Any OS, any C compiler
#define AUTHOR  Steven Pemberton, CWI, Amsterdam; "Steven.Pemberton@cwi.nl"
#define COPYRIGHT(c) 1993-9 Steven Pemberton, CWI. All rights reserved.
#define NOTE  Improvements gratefully received. Please mention the version.
#define COMPILE On Unix compile with: "sh enquire.c"; see below for details
#define WEB "http://www.cwi.nl/~steven/enquire.html"
#endif

#ifdef NOTDEFINED /* This is how you compile it; see below for details */
  case $0 in
  *.c) ;;
  sh) echo 'Use "sh enquire.c", not "sh < enquire.c"' >&2; exit 1;;
  *) echo 'Filename must end in ".c"' >&2; exit 1;;
  esac
  if test -r test.c
  then echo Would overwrite test.c - try it somewhere safer >&2; exit 1
  fi
  CFLAGS=
  echo Testing for needed CFLAGS ...
  echo "main(){char c; c=0;}" > test.c
  if ${CC=cc} ${1+"$@"} -o enquire test.c $LIBS
  then :
  else
      echo '*** "'$CC ${1+"$@"} -o enquire test.c $LIBS'" failed'
      echo '*** Giving up'
      /bin/rm -f test.c
      exit 1
  fi
  echo "main(){signed char c; c=0;}" > test.c
  if $CC ${1+"$@"} -o enquire test.c $LIBS 2>/dev/null
  then echo "   Signed char ok"
  else
    CFLAGS=-DNO_SC
    echo "   Signed char not accepted; using $CFLAGS"
  fi
  echo "main(){unsigned char c; c=0;}" > test.c
  if $CC ${1+"$@"} -o enquire test.c $LIBS 2>/dev/null
  then echo "   Unsigned char ok"
  else
    CFLAGS="$CFLAGS -DNO_UC"
    echo "   Unsigned char not accepted; using $CFLAGS"
  fi
  echo "main(){unsigned short s;unsigned long l;s=0;l=0;}" > test.c
  if $CC ${1+"$@"} -o enquire test.c $LIBS 2>/dev/null
  then echo "   Unsigned short and long ok"
  else
    CFLAGS="$CFLAGS -DNO_UI"
    echo "   Unsigned short or long not accepted; using $CFLAGS"
  fi
  echo "void foo(){} main(){foo();}" > test.c
  if $CC ${1+"$@"} -o enquire test.c $LIBS 2>/dev/null
  then echo "   Void ok"
  else
    CFLAGS="$CFLAGS -DNO_VOID"
    echo "   Void not accepted; using $CFLAGS"
  fi
  /bin/rm -f test.c a.out

  echo Compiling $0 ...
  case $# in
  0) : check bug in interpreting "$@" in some shells, if no parameters
     case `echo 1 "$@" 2` in
     "1  2") echo '   *** There is a bug in your shell expanding "$@"!'
       echo '   *** Taking remedial action' ;;
     "1 2") ;;
     esac
  esac
  case $ID in
  "") echo "   $CC" $CFLAGS "$@" $0 -o enquire $LIBS
      $CC $CFLAGS ${1+"$@"} $0 -o enquire $LIBS ||
    { echo '***' Try setting some CFLAGS; exit 1; }
      ;;
  *)  echo "   $CC" $CFLAGS "$@" -DID="\"$ID\"" $0 -o enquire $LIBS
      $CC $CFLAGS ${1+"$@"} -DID="\"$ID\"" $0 -o enquire $LIBS ||
    { echo '***' Try setting some CFLAGS; exit 1; }
  esac
  echo "Producing enquire.out limits.h and float.h ..."
  echo "   enquire > enquire.out"
  ./enquire > enquire.out || echo '   *** Some problems: see enquire.out'
  echo "   enquire -l > limits.h"
  ./enquire -l > limits.h || echo '   *** Some problems: see limits.h'
  echo "   enquire -f > float.h"
  ./enquire -f > float.h  || echo '   *** Some problems: see float.h'
  echo "Verifying the contents of limits.h and float.h ..."
  echo "   $CC" -DVERIFY $CFLAGS "$@" $0 -o verify $LIBS
  $CC -DVERIFY $CFLAGS ${@+"$@"} $0 -o verify $LIBS ||
    { echo '***' Failed; exit 1; }
  echo "   verify -fl > verify.out"
  ./verify -fl > verify.out ||
    echo '   *** Some problems: see verify.out'
  echo Done
  exit 0
#endif

/*
 PURPOSE
    This is a program that determines many properties of the C
    compiler and machine that it is run on, such as minimum and
    maximum [un]signed char/int/long, many properties of float/ [long]
    double, and so on.

    As an option it produces the ANSI C float.h and limits.h files.

    As a further option, it even checks that the compiler reads the
    header files correctly.

    It is a good test-case for compilers, since it exercises them with
    many limiting values, such as the minimum and maximum floating-point
    numbers.

 COMPILING AND RUNNING ON UNIX MACHINES
    With luck and a following wind, on Unix systems just the following
    will work:
  sh enquire.c    (or whatever filename you chose).
    Any parameters are passed to the C compiler, so if the compilation
    fails for any reason curable as explained below, you can do the following:
  sh enquire.c -DBAD_CPP

    If you do get compilation errors, check the line in question.
    Very often there is a comment attached saying which define to set.

    You can use a different C compiler than the default cc by setting CC:
  CC=gcc sh enquire.c -ansi
    You can load extra libraries by setting LIBS:
  CC=gcc LIBS=-lflong sh enquire.c -ansi
    Add ID="string" for the string to be added to the output; for instance:
  ID="`hostname` cc -ansi" sh enquire.c -ansi

    Compiling may give messages about unreachable code. These you can ignore.

    Some compilers offer various flags for different floating point
    modes; it's worth trying all possible combinations of these.

    Don't say I haven't tried to make life easy for you...

 COMPILING AND RUNNING ON NON-UNIX SYSTEMS
    On non-Unix systems, you must say (the equivalent of):
  cc enquire.c -o enquire
  enquire > enquire.out
  enquire -l > limits.h
  enquire -f > float.h
  cc -DVERIFY enquire.c -o verify #this includes limits.h and float.h
  verify -fl > verify.out

    If your compiler doesn't support:   add flag:
  signed char (eg pcc)      -DNO_SC
  unsigned char       -DNO_UC
  unsigned short and long     -DNO_UI
  void          -DNO_VOID
  signal(), or setjmp/longjmp()   -DNO_SIG

    Try to compile first with no flags, and see if you get any errors
    - you might be surprised. (Most non-ANSI compilers need -DNO_SC,
    though.)  Some compilers need a -f flag for floating point.

    Don't use any optimisation flags: the program may not work if you
    do.  Though "while (a+1.0-a-1.0 == 0.0)" may look like "while(1)"
    to an optimiser, to a floating-point unit there's a world of difference.

    Compiling may give messages about unreachable code. These you can ignore.

    Some compilers offer various flags for different floating point
    modes; it's worth trying all possible combinations of these.

 FAULTY COMPILERS
    Because of bugs and/or inadequacies, some compilers need the following
    defines:

    -  If your C preprocessor doesn't have the predefined __FILE__
       macro, and you don't want to call this file enquire.c but, say,
       tell.c, add the flag -DFILENAME=\"tell.c\" .

    -  Some compilers won't accept the line "#include FILENAME".  Add
       flag -DBAD_CPP. In that case, this file *must* be called
       enquire.c.

    -  Some compilers can't cope with "#ifdef __FILE__". Use
       -DFILENAME= or -DBAD_CPP as above.

    -  Some naughty compilers define __STDC__, but don't really
       support it.  Some define it as 0, in which case we ignore it.
       But if your compiler defines it, and isn't really ANSI C, add
       flag -DBAD_STDC. (To those compiler writers: for shame).

    -  Some naughty compilers define __STDC__, but don't have the
       stddef.h include file. Add flag -DBAD_STDDEF. (Gcc needs this
       on some machines, due to clashes between stddef.h and other
       include files.)

    -  Some systems crash when you try to malloc all store. To save
       users of such defective systems too much grief, they may
       compile with -DBAD_MALLOC, which ignores that bit of the code.

    Summary of naughty-compiler flags:
    If your compiler doesn't support:    add flag:
  __FILE__ (and you changed the filename) -DFILENAME=\"name.c\"
  #ifdef __FILE__       -DBAD_CPP or -DFILENAME=...
  #include FILENAME     -DBAD_CPP
  __STDC__ (properly)     -DBAD_STDC
  stddef.h        -DBAD_STDDEF
  malloc(LOTS) == NULL      -DBAD_MALLOC

    While it is not our policy to support defective compilers, pity has been
    taken on people with compilers that can't produce object files bigger than
    32k (especially since it was an easy addition). Compile the program
    into separate parts like this:
  cc -c -DSEP -DPASS0 -o p0.o <other flags> enquire.c
  cc -c -DSEP -DPASS1 -o p1.o <other flags> enquire.c
  cc -c -DSEP -DPASS2 -o p2.o <other flags> enquire.c
  cc -c -DSEP -DPASS3 -o p3.o <other flags> enquire.c
  cc -o enquire p0.o p1.o p2.o p3.o

 SYSTEM DEPENDENCIES
    You may possibly need to add some calls to signal() for other sorts of
    exception on your machine than SIGFPE, SIGOVER, SIGBUS, and SIGSEGV.
    See lines beginning #ifdef SIGxxx (and communicate the differences to me!).

 OUTPUT
    Running without argument gives the information as English text. If run
    with argument -l (e.g. enquire -l), output is a series of #define's for
    the ANSI standard limits.h include file, excluding MB_MAX_CHAR. If run
    with argument -f, output is a series of #define's for the ANSI standard
    float.h include file (according to ANSI C Draft of Dec 7, 1988).
    Flag -v gives verbose output: output includes the English text above
    as C comments. The program exit(0)'s if everything went ok, otherwise
    it exits with a positive number, telling how many problems there were.

 VERIFYING THE COMPILER
    If, having produced the float.h and limits.h header files, you want to
    verify that the compiler reads them back correctly (there are a lot of
    boundary cases, of course, like minimum and maximum numbers), you can
    recompile enquire.c with -DVERIFY set (plus the other flags that you used
    when compiling the version that produced the header files). This then
    recompiles the program so that it #includes "limits.h" and "float.h",
    and checks that the constants it finds there are the same as the
    constants it produces. Run the resulting program with enquire -fl.
    Many compilers fail this test.
    NB: You *must* recompile with the same compiler and flags, otherwise
    you may get odd results.

    You can also use this option if your compiler already has both files,
    and you want to confirm that this program produces the right results.

 TROUBLESHOOTING.
    This program is now quite trustworthy, and suspicious and wrong output
    may well be caused by bugs in the compiler, not in the program (however
    of course, this is not guaranteed, and no responsibility can be
    accepted, etc.)

    The program only works if overflows are ignored by the C system or
    are catchable with signal().

    If the program fails to run to completion (often with the error message
    "Unexpected signal at point x"), this often turns out to be a bug in the
    C compiler's run-time system. Check what was about to be printed, and
    try to narrow the problem down.

    Another possible problem is that you have compiled the program to produce
    loss-of-precision arithmetic traps. The program cannot cope with these,
    and you should re-compile without them. (They should never be the default).

    Make sure you compiled with optimisation turned off.

    Output preceded by *** WARNING: identifies behaviour of the C system
    deemed incorrect by the program. Likely problems are that printf or
    scanf don't cope properly with certain boundary numbers: this program
    goes to a lot of trouble to calculate its values, and these values
    are mostly boundary numbers. Experience has shown that often printf
    cannot cope with these values, and so in an attempt to increase
    confidence in the output, for each float and double that is printed,
    the printed value is checked by using sscanf to read it back.
   Care is taken that numbers are printed with enough digits to uniquely
    identify them, and therefore that they can be read back identically.
    If the number read back is different, then there is probably a bug in
    printf or sscanf, and the program prints the warning message.
    If the two numbers in the warning look identical, then printf is more
    than likely rounding the last digit(s) incorrectly. To put you at ease
    that the two really are different, the bit patterns of the two numbers
    are also printed. The difference is very likely in the last bit.
   Many scanf's read the minimum double back as 0.0, and similarly cause
    overflow when reading the maximum double. This program quite ruthlessly
    declares all these behaviours faulty. The point is that if you get
    one of these warnings, the output may be wrong, so you should check
    the result carefully if you intend to use the results. Of course, printf
    and sscanf may both be wrong, and cancel each other out, so you should
    check the output carefully anyway.

    The warning that "a cast didn't work" refers to cases like this:

  float f;
  #define C 1.234567890123456789
  f= C;
  if (f != (float) C) printf ("Wrong!");

    A faulty compiler will widen f to double and ignore the cast to float,
    and because there is more accuracy in a double than a float, fail to
    recognise that they are the same. In the actual case in point, f and C
    are passed as parameters to a function that discovers they are not equal,
    so it's just possible that the error was in the parameter passing,
    not in the cast (see function Verify()).
    For ANSI C, which has float constants, the error message is "constant has
    wrong precision".

 REPORTING PROBLEMS
    If the program doesn't work for you for any reason that can't be
    narrowed down to a problem in the C compiler, or it has to be
    changed in order to get it to compile, or it produces suspicious
    output (like a very low maximum float, for instance), please mail
    the problem and an example of the incorrect output to
    Steven.Pemberton@cwi.nl so that improvements can be worked into
    future versions. Try to give as much information as possible;
    DON'T FORGET TO MENTION THE VERSION NUMBER!

    The program tries to catch and diagnose bugs in the compiler/run-time
    system. I would be especially pleased to have reports of failures so
    that I can improve this service.

    I apologise unreservedly for the contorted use of the preprocessor...
    but it was fun!

 NEW VERSIONS
    Worried that you may not have the latest version? Ftp to
    ftp.cwi.nl, and look in directory pub/steven/enquire
    for file enquireXX.c; XX is the version number. Or look at
    http://www.cwi.nl/~steven/enquire.html

 HOW DOES ENQUIRE WORK?
    There is an article that explains a lot of the workings: The
    Ergonomics of Portability; available from the above addresses as file
    enquire.ps.

 THE SMALL PRINT
    This is not a public domain program; nor is any other program that
    carries a copyright notice. It is however freely copyable under the
    following conditions:

       You may copy and distribute verbatim copies of this source file.
       You may modify this source file, and copy and distribute such
       modified versions, provided that you leave the copyright notice
       at the top of the file and also cause the modified file to carry
       prominent notices stating that you changed the files and the
       date of any change; and cause the whole of any work that you
       distribute or publish, that in whole or in part contains or is a
       derivative of this program or any part thereof, to be licensed
       at no charge to all third parties on terms identical to those
       here.

    While every effort has been taken to make this program as reliable as
    possible, no responsibility can be taken for the correctness of the
    output, nor suitability for any particular use.

    If you do have a fix to any problem, please send it to me, so that
    other people can have the benefits.

    This program is an offshoot of a project funded by public funds.
    If you use this program for research or commercial use (i.e. more
    than just for the fun of knowing about your compiler) mailing a short
    note of acknowledgement may help keep enquire.c supported.

 ACKNOWLEDGEMENTS
    Many people have given time and ideas to making this program what it is.
    To all of them thanks, and apologies for not mentioning them by name.

 HISTORY
    Originally started as a program to generate configuration constants
    for a large piece of software we were writing, which later took on
    a life of its own...
    1.0 Length 6658!; end 1984?
  Unix only. Only printed a dozen maximum int/double values.
    2.0 Length 10535; Spring 1985
  Prints values as #defines (about 20 of them)
  More extensive floating point, using Cody and Waite
  Handles signals better
  Programs around optimisations
  Handles Cybers
    3.0 Length 12648; Aug 1987; prints about 42 values
  Added PASS stuff, so treats float as well as double
    4.0 Length 33891; Feb 1989; prints around 85 values
  First GNU version (for gcc, where they called it hard-params.c)
  Generates float.h and limits.h files
  Handles long double
  Generates warnings for dubious output
    4.1 Length 47738; April 1989
  Added VERIFY and TEST
    4.2 Length 63442; Feb 1990
  Added SEP
  Fixed eps/epsneg
  Added check for pseudo-unsigned chars
  Added description for each #define output
  Added check for absence of defines during verify
  Added prototypes
  Added BAD_STDC and BAD_CPP
  Fixed alignments output
    4.3 Length 75000; Oct 1990; around 114 lines of output
  Function xmalloc defined, Richard Stallman, June 89.
  Alignments computed from member offsets rather than structure sizes,
      Richard Stallman, Oct 89
  Print whether char* and int* pointers have the same format;
      also char * and function *
  Update to Draft C version Dec 7, 1988
      - types of constants produced in limits.h
    (whether to put a U after unsigned shorts and chars and
     whether to output -1024 as (-1023-1))
      - values of SCHAR_MIN/MAX
      - values of *_EPSILON (not the smallest but the effective smallest)
  Added FILENAME, since ANSI C doesn't allow #define __FILE__
  Renamed from config.c to enquire.c
  Added size_t and ptrdiff_t enquiries
  Added promotion enquiries
  Added type checks of #defines
  Added BAD_STDDEF
  Changed endian to allow for cases where not all bits are used
  Sanity check for max integrals
  Fixed definition of setjmp for -DNO_SIG
  Moved #define ... 0.0L inside #ifdef STDC, in case some cpp's tokenize
  Added BAD_MALLOC
    5.0 Length 88228; February 1993; around 120 lines of output
         (depends on what you count)
  Added the 'sh enquire.c' horror/delight: thanks David Mankins@think
  Added checks for long names: thanks Steve Simon@leeds-poly
  Added FPE signal checks: thanks Leonid A. Broukhis
  Added check for dereferencing NULL
  Added TESTI
  Added LIBS, fixed showtype: thanks Rainer Orth@TechFak.Uni-Bielefeld.DE
  Added a free(): thanks nickc@perihelion.co.uk
  Added signal catching to the malloc part
  Renamed naughty-compiler defines to BAD_*
  Renamed and altered Verify() to better check faulty compilers
  Shut some compilers up from giving incorrect warnings.
  Fixed sign_of(): thanks Hugh Redelmeier@redvax
  Fixed USHRT_MAX for sizeof(short)=sizeof(int) but INT_MAX > SHRT_MAX
  Fixed NO_UI
  Fixed -DSEP: thanks Mike Black@seismo
  Fixed the case where stdio.h includes limits.h: thanks rms@gnu
  Fixed exponent(): thanks Christophe BINOT
    <chb%hpvpta.france.hp.com@hplb.hpl.hp.com>
   5.0a Aug 1997
  Made handling of ID= easier
  Improved the reporting of use of bits in Floating values.
   5.1  Length 88739; Sep 1998
  Fixed detection of infinity for machines with no overflow trap
  Speeded up search for max char (first 32 bit char machine turned up...)
   5.1a Length 88832; Feb 1999
  Changed identification message
   5.1b Length 88926; Oct 2002
        Fixed a missing \n in an output line; thanks Leonid Broukhis again
*/

/* Set FILENAME to the name of this file */
#ifndef FILENAME
#ifdef BAD_CPP
#define FILENAME "enquire.c"
#else
#ifdef __FILE__ /* It's a compiler bug if this fails. Define BAD_CPP */
#define FILENAME __FILE__
#else
#define FILENAME "enquire.c"
#endif /* __FILE__ */
#endif /* BAD_CPP */
#endif /* FILENAME */

/* This file is read three times (it #includes itself), to generate
   otherwise identical code for each of short+float, int+double,
   long+long double. If PASS isn't defined, then this is the first pass.
   Code bracketed by 'PASS0' is for stuff independent of all three,
   but is read during the first pass.
*/
#ifndef PASS
#ifdef SEP /* so we're only interested if this is pass 1 or not */
#ifdef PASS1
#define PASS 1
#else
#define PASS 0
#endif
#else /* no SEP, so this is the first pass */
#define PASS 1
#define PASS0 1
#define PASS1 1
#endif /* SEP */

/* Void just marks the functions that don't return a result */
#ifdef NO_VOID
#define Void int
#else
#define Void void
#endif

/* Set STDC to whether this is *really* an ANSI C compiler.
   Some bad compilers define __STDC__, when they don't support it.
   Compile with -DBAD_STDC to get round this.
*/
#ifndef BAD_STDC
#ifdef __STDC__
#if __STDC__ /* If __STDC__ is 0, assume it isn't supported */
#define STDC
#endif
#endif
#endif

/* Stuff different for ANSI C, and old C:
   ARGS and NOARGS are used for function prototypes.
   Volatile is used to reduce the chance of optimisation,
  and to prevent variables being put in registers (when setjmp/longjmp
  wouldn't work as we want)
   Long_double is the longest floating point type available.
   stdc is used in tests like "if (stdc)", which is less ugly than #ifdef.
   U is output after unsigned constants.
 */
#ifdef STDC

#define ARGS(x) x
#define NOARGS (void)
#define Volatile volatile
#define Long_double long double
#define stdc 1
#define U "U"

#else /* Old style C */

#define ARGS(x) ()
#define NOARGS ()
#define Volatile static
#define Long_double double
#define stdc 0
#define U ""

#endif /* STDC */

/* include files */
#include <stdio.h>
#include  <wchar.h>

#ifdef STDC
#ifndef BAD_STDDEF
#include <stddef.h> /* for size_t: if this fails, define BAD_STDDEF */
#endif
#endif

#ifdef NO_SIG
#define jmp_buf int
#else
#include <signal.h> /* if this fails, define NO_SIG */
#include <setjmp.h> /* if this fails, define NO_SIG */
#endif
//#ifndef NO_SIG
//#include <signal.h> /* if this fails, define NO_SIG */
//#include <setjmp.h> /* if this fails, define NO_SIG */
//#endif

/* Kludge around the possiblity that <stdio.h> includes <limits.h> */
#ifdef CHAR_BIT
#undef CHAR_BIT
#undef CHAR_MAX
#undef CHAR_MIN
#undef SCHAR_MAX
#undef SCHAR_MIN
#undef UCHAR_MAX
#undef UCHAR_MIN
#endif

#ifdef VERIFY
#include "limits.h"
#include "float.h"
#endif

/* The largest unsigned type */
#ifdef NO_UI
#define ulong unsigned int
#else
#define ulong unsigned long
#endif

/* Some shorthands */
#define Vprintf if (V) printf
#define Unexpected(place) if (setjmp(lab)!=0) croak(place)
#define fabs(x) (((x)<0.0)?(-x):(x))

#endif /* PASS */

/* A description of the ANSI constants */
#define D_CHAR_BIT "Number of bits in a storage unit"
#define D_CHAR_MAX "Maximum char"
#define D_CHAR_MIN "Minimum char"
#define D_SCHAR_MAX "Maximum signed char"
#define D_SCHAR_MIN "Minimum signed char"
#define D_UCHAR_MAX "Maximum unsigned char (minimum is always 0)"

#define D_INT_MAX "Maximum %s"
#define D_INT_MIN "Minimum %s"
#define D_UINT_MAX "Maximum unsigned %s (minimum is always 0)"

#define D_FLT_ROUNDS "Addition rounds to 0: zero, 1: nearest, 2: +inf, 3: -inf, -1: unknown"
#define D_FLT_RADIX "Radix of exponent representation"
#define D_MANT_DIG "Number of base-FLT_RADIX digits in the significand of a %s"
#define D_DIG "Number of decimal digits of precision in a %s"
#define D_MIN_EXP "Minimum int x such that FLT_RADIX**(x-1) is a normalised %s"
#define D_MIN_10_EXP "Minimum int x such that 10**x is a normalised %s"
#define D_MAX_EXP "Maximum int x such that FLT_RADIX**(x-1) is a representable %s"
#define D_MAX_10_EXP "Maximum int x such that 10**x is a representable %s"
#define D_MAX "Maximum %s"
#define D_EPSILON "Difference between 1.0 and the minimum %s greater than 1.0"
#define D_MIN "Minimum normalised %s"

#ifdef PASS0

/* Prototypes for what's to come: */

int false NOARGS;

#ifdef BAD_STDDEF
char *malloc (); /* Old style prototype, since we don't know what size_t is */
#else
char *malloc ARGS((size_t size));
#endif
Void free ARGS((char *p)); /* Syntax error here? Try -DNO_VOID */

Void exit ARGS((int status));

char *f_rep ARGS((int precision, Long_double val));

int maximum_int NOARGS;
int cprop NOARGS;
int basic NOARGS;
Void sprop NOARGS;
Void iprop NOARGS;
Void lprop NOARGS;
Void usprop NOARGS;
Void uiprop NOARGS;
Void ulprop NOARGS;
int fprop ARGS((int byte_size));
int dprop ARGS((int byte_size));
int ldprop ARGS((int byte_size));
Void efprop ARGS((int fprec, int dprec, int lprec));
Void edprop ARGS((int fprec, int dprec, int lprec));
Void eldprop ARGS((int fprec, int dprec, int lprec));

int setmode ARGS((char *s));
Void farewell ARGS((int bugs));
Void describe ARGS((char *description, char *extra));
Void missing ARGS((char *s));
Void fmissing ARGS((char *s));
Void check_defines NOARGS;
Void bitpattern ARGS((char *p, unsigned int size));
int ceil_log ARGS((int base, Long_double x));
Void croak ARGS((int place));
Void trap1 ARGS((int sig));
Void eek_a_bug ARGS((char *problem));
Void endian ARGS((int byte_size));
int exponent ARGS((Long_double x, Long_double *fract, int *exp));
int floor_log ARGS((int base, Long_double x));
Void f_define ARGS((char *desc, char *extra, char *sort, char *name,
       int prec, Long_double val, char *mark));
Void i_define ARGS((char *desc, char *extra, char *sort, char *name,
       long val, long lim, long req, char *mark));
Void u_define ARGS((char *desc, char *extra, char *sort, char *name,
       ulong val, ulong req, char *mark));

#ifdef NO_SIG  /* There's no signal(), or setjmp/longjmp() */

  /* Dummy routines instead */
  typedef int jmp_buf;

  int setjmp ARGS((jmp_buf lab));

  jmp_buf lab, mlab;
  int setjmp(jmp_buf lab) { return(0); }
  void  longjmp(jmp_buf lab, int val) { return; }

  Void signal(int i, void (*p)()) {}

#else
  jmp_buf lab, mlab;
  Void overflow(int sig)
  { /* what to do on over/underflow */
    signal(sig, overflow);
    longjmp(lab, 1);
  }

  Void address(int sig)
  { /* what to do on an address error */
    signal(sig, address);
    longjmp(mlab, 1);
  }

#endif /*NO_SIG*/

int V= 0, /* verbose */
    L= 0, /* produce limits.h */
    F= 0, /* produce float.h  */
    bugs=0; /* The number of (possible) bugs in the output */

char co[4], oc[4]; /* Comment starter and ender symbols */

int bits_per_byte; /* the number of bits per unit returned by sizeof() */
int flt_rounds;    /* The calculated value of FLT_ROUNDS */
int flt_radix;     /* The calculated value of FLT_RADIX */
Volatile int trapped; /* For testing FPE traps */

#ifdef TEST
/* Set the fp modes on a SUN with 68881 chip, to check that different
   rounding modes etc. get properly detected.
   Compile with -f68881 for cc, -m68881 for gcc, and with additional flag
   -DTEST. Run with additional parameter +hex-number, to set the 68881 mode
   register to hex-number
*/

/* Bits 0x30 = rounding mode */
#define ROUND_BITS  0x30
#define TO_NEAREST  0x00
#define TO_ZERO   0x10
#define TO_MINUS_INF  0x20
#define TO_PLUS_INF 0x30 /* The SUN FP user's guide seems to be wrong here */

/* Bits 0xc0 = extended rounding */
#define EXT_BITS  0xc0
#define ROUND_EXTENDED  0x00
#define ROUND_SINGLE  0x40
#define ROUND_DOUBLE  0x80

/* Enabled traps */
#define EXE_INEX1  0x100
#define EXE_INEX2  0x200
#define EXE_DZ     0x400
#define EXE_UNFL   0x800
#define EXE_OVFL  0x1000
#define EXE_OPERR 0x2000
#define EXE_SNAN  0x4000
#define EXE_BSUN  0x8000

/* Only used for testing, on a Sun with 68881 chip */
/* Print the FP mode */
printmode(new) unsigned new; {
  fpmode_(&new);
  printf("New fp mode:\n");
  printf("  Round toward ");
  switch (new & ROUND_BITS) {
        case TO_NEAREST:   printf("nearest"); break;
        case TO_ZERO:      printf("zero"); break;
        case TO_MINUS_INF: printf("minus infinity"); break;
        case TO_PLUS_INF:  printf("plus infinity"); break;
        default: printf("???"); break;
  }

  printf("\n  Extended rounding precision: ");

  switch (new & EXT_BITS) {
        case ROUND_EXTENDED: printf("extended"); break;
        case ROUND_SINGLE:   printf("single"); break;
        case ROUND_DOUBLE:   printf("double"); break;
        default: printf("???"); break;
  }

  printf("\n  Enabled exceptions:");
  if (new & (unsigned) EXE_INEX1) printf(" inex1");
  if (new & (unsigned) EXE_INEX2) printf(" inex2");
  if (new & (unsigned) EXE_DZ)    printf(" divz");
  if (new & (unsigned) EXE_UNFL)  printf(" unfl");
  if (new & (unsigned) EXE_OVFL)  printf(" ovfl");
  if (new & (unsigned) EXE_OPERR) printf(" operr");
  if (new & (unsigned) EXE_SNAN)  printf(" snan");
  if (new & (unsigned) EXE_BSUN)  printf(" bsun");
  printf("\n");
}

/* Only used for testing, on a Sun with 68881 chip */
/* Set the FP mode */
int setmode(s) char *s; {
  unsigned mode=0, dig;
  char c;

  while (*s) {
    c= *s++;
    if  (c>='0' && c<='9') dig= c-'0';
    else if (c>='a' && c<='f') dig= c-'a'+10;
    else if (c>='A' && c<='F') dig= c-'A'+10;
    else return 1;
    mode= mode<<4 | dig;
  }
  printmode(mode);
  return 0;
}
#define SETMODE
#endif

#ifdef TESTI /* Test mode using SunOs IEEE routines */

#include <sys/ieeefp.h>

int setmode(char *s) {
  char *dummy, c, *cmd, *val;
  while (*s) {
    switch (c= *s++) {
          case '=': cmd= "direction"; val= "nearest"; break;
          case '0': cmd= "direction"; val= "tozero"; break;
          case '+': cmd= "direction"; val= "positive"; break;
          case '-': cmd= "direction"; val= "negative"; break;
          case '1': cmd= "precision"; val= "single"; break;
          case '2': cmd= "precision"; val= "double"; break;
          case '3': cmd= "precision"; val= "extended"; break;
          case '~': cmd= "exception"; val= "inexact"; break;
          case '/': cmd= "exception"; val= "division"; break;
          case '>': cmd= "exception"; val= "overflow"; break;
          case '<': cmd= "exception"; val= "underflow"; break;
          default:
      printf("Bad setmode character: %c\n", c);
      return 1;
      break;
    }
    printf("Set %s %s", cmd, val);
    if (ieee_flags("set", cmd, val, &dummy)) {
      printf(": failed\n");
      return 1;
    }
    printf("\n");
  }
  return 0;
}
#define SETMODE
#endif

#ifndef SETMODE
/* ARGSUSED */
int
setmode(char *s)
{
  fprintf(stderr, "Can't set mode: not compiled with TEST\n");
  return(1);
}
#endif

int
memeq(
  char *p1,
  int size1,
  char *p2,
  int size2
  )
{
  /* See if two blocks of store are identical */
  int i;
  if (size1 != size2) return 0;
  for (i=1; i<=size1; i++) {
    if (*p1++ != *p2++) return 0;
  }
  return 1;
}

Void
farewell(int bugs)
{
  if (bugs == 0) exit(0);
  printf("\n%sFor hints on dealing with the ", co);
  if (bugs == 1) printf("problem");
  else printf("%d problems", bugs);
  printf(" above\n   see the section 'TROUBLESHOOTING' in the file ");
  printf("%s%s\n", FILENAME, oc);
  exit(bugs);
}

/* The program has received a signal where it wasn't expecting one */
Void
croak(int place)
{
  printf("*** Unexpected signal at point %d\n", place);
  farewell(bugs+1); /* An exit isn't essential here, but avoids loops */
}

/* This is here in case alloca.c is used, which calls this. */
char *
xmalloc(unsigned size)
{
  char *value = malloc(size);
  if (value == 0) {
    fprintf(stderr, "Virtual memory exceeded\n");
    exit(bugs+1);
  }
  return value;
}

int maxint;

int
maximum_int( void )
{
  /* Find the maximum integer */
  Volatile int newi, int_max, two=2;

  /* Calculate maxint ***********************************/
  /* Calculate 2**n-1 until overflow - then use the previous value  */

  newi=1; int_max=0;

  if (setjmp(lab)==0) { /* Yields int_max */
    while(newi>int_max) {
      int_max=newi;
      newi=newi*two+1;
    }
  }
  Unexpected(0);
  return int_max;
}

/* How long are my identifiers? I could go further here, but some compilers
   have line length limits that I don't want to break.
*/

int zzzzzzzzz1zzzzzzzzz2zzzzzzzzz3zzzzzzzzz4zzzzzzzzz5zzzzzzzzz6zzzz=64;

int
name_len( void )
{
   int zzzzzzzzz1zzzzzzzzz2zzzzzzzzz3zz=32;
   { int zzzzzzzzz1zzzzzz=16;
     { int zzzzzzzz=8;
       { int zzzzzzz=7;
   { int zzzzzz=6;
     return
       zzzzzzzzz1zzzzzzzzz2zzzzzzzzz3zzzzzzzzz4zzzzzzzzz5zzzzzzzzz6zzzz;
   }
       }
     }
   }
}

#define aaaaaaaaa1aaaaaaaaa2aaaaaaaaa3aaaaaaaaa4aaaaaaaaa5aaaaaaaaa6aaaa 64
#define LENGTH 64

#ifdef aaaaaaaaa1aaaaaaaaa2aaaaaaaaa3aa
#undef LENGTH
#define LENGTH 32
#endif

#ifdef aaaaaaaaa1aaaaaa
#undef LENGTH
#define LENGTH 16
#endif

#ifdef aaaaaaaa
#undef LENGTH
#define LENGTH 8
#endif

#undef aaaaaaaaa1aaaaaaaaa2aaaaaaaaa3aaaaaaaaa4aaaaaaaaa5aaaaaaaaa6aaaa

Void long_names()
{
  int l= name_len();
  Vprintf("Compiler names are at least %d chars long", l);
  if (l != 64)
    Vprintf(" (but less than %d)", l*2);
  Vprintf("\n");
  Vprintf("Preprocessor names are at least %d long", LENGTH);
  if (LENGTH != 64)
    Vprintf(" (but less than %d)", LENGTH*2);
  Vprintf("\n\n");
}

/* Used by FPROP to see if FP traps are generated, and if they may return */

Void
trap2(int sig)
{
  longjmp(lab, 1);
}

Void
trap1(int sig)
{
  trapped= 1; /* A global */
  signal(sig, trap2);
}

Void
setsignals( void )
{
#ifdef SIGFPE
  signal(SIGFPE, overflow);
#endif
#ifdef SIGOVER
  signal(SIGOVER, overflow);
#endif
#ifdef SIGBUS
  signal(SIGBUS, address);
#endif
#ifdef SIGSEGV
  signal(SIGSEGV, address);
#endif
  /* Add more calls as necessary */
}

#undef LENGTH

int
main(int argc, char *argv[])
{
  int dprec, fprec, lprec;
  int i;
  wchar_t *s;
  int bad;

  setsignals();
  Unexpected(1);

  bad=0;
  for (i=1; i < argc; i++) {
    s = (wchar_t *)(argv[i]);
    if (*s == L'-') {
      s++;
      while (*s) {
        switch (*(s++)) {
              case L'v': V=1; break;
              case L'l': L=1; break;
              case L'f': F=1; break;
              default: bad=1; break;
        }
      }
    } else if (*s == L'+') {
      s++;
      bad = setmode((char *)s);
    } else bad= 1;
  }
  if (bad) {
    fprintf(stderr,
      "Usage: %ls [-vlf]\n  v=Verbose l=Limits.h f=Float.h\n",
      (wchar_t *)(argv[0]));
    exit(1);
  }
  if (L || F) {
    co[0]= '/'; oc[0]= ' ';
    co[1]= '*'; oc[1]= '*';
    co[2]= ' '; oc[2]= '/';
    co[3]= '\0'; oc[3]= '\0';
  } else {
    co[0]= '\0'; oc[0]= '\0';
    V=1;
  }

  if (L) printf("%slimits.h%s\n", co, oc);
  if (F) printf("%sfloat.h%s\n", co, oc);
#ifdef ID
  printf("%sProduced by enquire version %s (%s), CWI, Amsterdam\n   %s\n",
         co, VERSION, ID, WEB, oc);
#else
  printf("%sProduced by enquire version %s, CWI, Amsterdam\n   %s %s\n",
         co, VERSION,     WEB, oc);
#endif

#ifdef VERIFY
  printf("%sVerification phase%s\n", co, oc);
#endif

#ifdef NO_SIG
  Vprintf("%sCompiled without signal(): %s%s\n",
    co,
    "there's nothing that can be done if overflow occurs",
    oc);
#endif
#ifdef NO_SC
  Vprintf("%sCompiled without signed char%s\n", co, oc);
#endif
#ifdef NO_UC
  Vprintf("%sCompiled without unsigned char%s\n", co, oc);
#endif
#ifdef NO_UI
  Vprintf("%sCompiled without unsigned short or long%s\n", co, oc);
#endif
#ifdef __STDC__
  Vprintf("%sCompiler claims to be ANSI C level %d%s\n",
    co, __STDC__, oc);
#else
  Vprintf("%sCompiler does not claim to be ANSI C%s\n", co, oc);
#endif
  printf("\n");

  long_names();
  check_defines();

  maxint= maximum_int();
  bits_per_byte= basic();
  Vprintf("\n");
  if (F||V) {
    fprec= fprop(bits_per_byte);
    dprec= dprop(bits_per_byte);
    lprec= ldprop(bits_per_byte);
    efprop(fprec, dprec, lprec);
    edprop(fprec, dprec, lprec);
    eldprop(fprec, dprec, lprec);
  }
#ifndef BAD_MALLOC
  if (V) {
    /* An extra goody: the approximate amount of data-space */
    /* Allocate store until no more available */
    /* Different implementations have a different argument type
       to malloc. Here we assume that it's the same type as
       that which sizeof() returns */
    unsigned int size;
    Volatile long total;
    char kmg, *ptr, *save;
    char **link;

    save= NULL;
    size=maximum_int()/4;
    total=0;
    while (size!=0) {
      if (setjmp(mlab) == 0) {
        while ((ptr= malloc((false()?sizeof(int):size))) != (char *)NULL) {
          //if (save == NULL) save= ptr; /* save the biggest chunk */
          link = (char **)ptr;
          if (save == NULL) {
            // Save pointer to first allocated chunk
            save= ptr;
            *link = NULL;
          }
          else {
            // Build list of all subsequently allocated chunks, LIFO
            *link = save;
            save = ptr;
          }
          total+=(size/2);
        }
      } else {
        eek_a_bug("Trying to malloc all store generates a trap");
      }
      size/=2;
    }
    if (setjmp(mlab)!=0) croak(-1);

    //if (save != NULL) free(save);
    while(save != NULL) {
      link = (char **)save;
      ptr = *link;
      free(save);
      save = ptr;
    }

    total= (total+511)/512; /* Sic */ kmg= 'K';
    if (total > 10000) {
      total= (total+999)/1000; kmg= 'M';
    }
    if (total > 10000) {
      total= (total+999)/1000; kmg= 'G';
    }
    if (total > 10000) {
      total= (total+999)/1000; kmg= 'T';
    }
    Vprintf("%sMemory mallocatable ~= %ld %cbytes%s\n",
      co, total, kmg, oc);
  }
#endif
  farewell(bugs);
  return bugs; /* To keep compilers and lint happy */
}

Void
eek_a_bug(char *problem)
{
  /* The program has discovered a problem */
  printf("\n%s*** WARNING: %s%s\n", co, problem, oc);
  bugs++;
}

Void
describe(char *description, char *extra)
{
  /* Produce the description for a #define */
  printf("   %s", co);
  printf(description, extra);
  printf("%s\n", oc);
}

Void
i_define(
   char *desc,
   char *extra,
   char *sort,
   char *name,
   long val,
   long lim,
   long req,
   char *mark
  )
{
  /* Produce a #define for a signed int type */
  describe(desc, extra);
  if (val >= 0) {
    printf("#define %s%s %ld%s\n", sort, name, val, mark);
  } else if (val + lim < 0) {
    /* We may not produce a constant like -1024 if the max
       allowable value is 1023. It has then to be output as
       -1023-1. lim is the max allowable value. */
    printf("#define %s%s (%ld%s%ld%s)\n",
           sort, name, -lim, mark, val+lim, mark);
  } else {
    printf("#define %s%s (%ld%s)\n", sort, name, val, mark);
  }
  /* If VERIFY is not set, val and req are just the same value;
     if it is set, val is the value as calculated, and req is
     the #defined constant
  */
  if (val != req) {
    printf("%s*** Verify failed for above #define!\n", co);
    printf("       Compiler has %ld for value%s\n\n", req, oc);
    bugs++;
  }
  Vprintf("\n");
}

Void
u_define(
  char *desc,
  char *extra,
  char *sort,
  char *name,
  ulong val,
  ulong req,
  char *mark
  )
{
  /* Produce a #define for an unsigned value */
  describe(desc, extra);
  printf("#define %s%s %lu%s%s\n", sort, name, val, U, mark);
  if (val != req) {
    printf("%s*** Verify failed for above #define!\n", co);
    printf("       Compiler has %lu for value%s\n\n", req, oc);
    bugs++;
  }
  Vprintf("\n");
}

Void f_define(
  char *desc,
  char *extra,
  char *sort,
  char *name,
  int precision,
  Long_double val,
  char *mark
  )
{
  /* Produce a #define for a float/double/long double */
  describe(desc, extra);
  if (stdc) {
    printf("#define %s%s %s%s\n",
           sort, name, f_rep(precision, val), mark);
  } else if (*mark == 'F') {
    /* non-ANSI C has no float constants, so cast the constant */
    printf("#define %s%s ((float)%s)\n",
           sort, name, f_rep(precision, val));
  } else {
    printf("#define %s%s %s\n", sort, name, f_rep(precision, val));
  }
  Vprintf("\n");
}

int
floor_log(int base, Long_double x)
{
  /* return floor(log base(x)) */
  int r=0;
  while (x>=base) { r++; x/=base; }
  return r;
}

int
ceil_log(int base, Long_double x)
{
  int r=0;
  while (x>1.0) { r++; x/=base; }
  return r;
}

int
exponent(Long_double x, Long_double *fract, int *exp)
{
  /* Split x into a fraction and a power of ten;
     returns 0 if x is unusable, 1 otherwise.
     Only used for error messages about faulty output.
  */
  int r=0, neg=0;
  Long_double old;
  *fract=0.0; *exp=0;
  if (x<0.0) {
    x= -x;
    neg= 1;
  }
  if (x==0.0) return 1;
  if (x>=10.0) {
    while (x>=10.0) {
      old=x; r++; x/=10.0;
      if (old==x) return 0;
    }
  } else {
    while (x<1.0) {
      old=x; r--; x*=10.0;
      if (old==x) return 0;
    }
  }
  if (neg) *fract= -x;
  else *fract= x;
  *exp=r;
  return 1;
}

char *
f_rep(int precision, Long_double val)
{
  /* Return the floating representation of val */
  static char buf[1024];
  char *f1;
  if (sizeof(double) == sizeof(Long_double)) {
    /* Assume they're the same, and use non-stdc format */
    /* This is for stdc compilers using non-stdc libraries */
    f1= "%.*e";
  } else {
    /* It had better support Le then */
    f1= "%.*Le";
  }
  sprintf(buf, f1, precision, val);
  return buf;
}

Void
bitpattern(char *p, unsigned int size)
{
  /* Printf the bit-pattern of p */
  char c;
  unsigned int i;
  int j;

  for (i=1; i<=size; i++) {
    c= *p;
    p++;
    for (j=bits_per_byte-1; j>=0; j--)
      printf("%c", (c>>j)&1 ? '1' : '0');
    if (i!=size) printf(" ");
  }
}

Void
fill(char *p, int size)
{
  char *ab= "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;

  for (i=0; i<size; i++)
    p[i]= ab[i];
}

#define Order(x, mode)\
   printf("%s%s ", co, mode); fill((char *)&x, sizeof(x)); \
   for (i=1; i<=sizeof(x); i++) { c=((x>>(byte_size*(sizeof(x)-i)))&mask);\
      putchar(c==0 ? '?' : (char)c); }\
   printf("%s\n", oc);

Void
endian(int byte_size)
{
  /* Printf the byte-order used on this machine */
  /*unsigned*/ short s=0;
  /*unsigned*/ int j=0;
  /*unsigned*/ long l=0;

  unsigned int mask, i, c;

  mask=0;
  for (i=1; i<=(unsigned)byte_size; i++) mask= (mask<<1)|1;

  if (V) {
    printf("%sCHARACTER ORDER%s\n", co, oc);
    Order(s, "short:");
    Order(j, "int:  ");
    Order(l, "long: ");
  }
}

Void
missing(char *s)
{
  printf("%s*** #define %s missing from limits.h%s\n", co, s, oc);
  bugs++;
}

Void
fmissing(char *s)
{
  printf("%s*** #define %s missing from float.h%s\n", co, s, oc);
  bugs++;
}

/* To try and fool optimisers */
int false( void ) { return 0; }

#define Promoted(x) (false()?(x):(-1))
#define is_signed(x) (Promoted(x) < 0)
#define sign_of(x) (is_signed(x)?"signed":"unsigned")
#define Signed 1
#define Unsigned 0
#define sgn(x) ((is_signed(x))?Signed:Unsigned)

#define showtype(t, x) Vprintf("%s%s %s %s%s\n", co, t, sign_of(x), type_of((int)sizeof(x)), oc)

char *type_of(int x)
{
  if (x == sizeof(char)) {
    if (sizeof(char) == sizeof(int)) return "char/short/int";
    if (sizeof(char) == sizeof(short)) return "char/short";
    return "char";
  }
  if (x == sizeof(short)) {
    if (sizeof(short) == sizeof(int)) return "short/int";
    return "short";
  }
  if (x == sizeof(int)) {
    if (sizeof(int) == sizeof(long)) return "int/long";
    return "int";
  }
  if (x == sizeof(long)) return "long";
  return "unknown-type";
}

char *ftype_of(int x)
{
  if (x == sizeof(float)) {
    return "float";
  }
  if (x == sizeof(double)) {
    if (sizeof(double) == sizeof(Long_double))
      return "(long)double";
    return "double";
  }
  if (x == sizeof(Long_double)) {
    return "long double";
  }
  return "unknown-type";
}

Void typerr(char *name, int esign, int esize, int sign, int size)
{
  Vprintf("*** %s has wrong type: expected %s %s, found %s %s\n",
    name, sign_of(esign), type_of(esize),
    sign_of(sign), type_of(size));
}

Void ftyperr(char *name, int esize, int size)
{
  Vprintf("*** %s has wrong type: expected %s, found %s\n",
    name, ftype_of(esize), ftype_of(size));
}

Void promotions( void )
{
  int si; long sl;
  unsigned int ui;
  short ss;

#ifndef NO_UI
  unsigned long ul;  /* if this fails, define NO_UI */
  unsigned short us; /* if this fails, define NO_UI */

  ul=0; us=0;
#endif
  /* Shut compiler warnings up: */
  si=0; sl=0; ui=0; ss=0;

  Vprintf("\n%sPROMOTIONS%s\n", co, oc);

  /* Sanity checks. Possible warnings here; should be no problem */
  if (is_signed(ui))
    eek_a_bug("unsigned int promotes to signed!\n");
  if (!is_signed(si))
    eek_a_bug("signed int promotes to unsigned!\n");
  if (!is_signed(sl))
    eek_a_bug("signed long promotes to unsigned!\n");
  if (sizeof(Promoted(si)) != sizeof(int))
    eek_a_bug("int doesn't promote to int!\n");
  if (sizeof(Promoted(sl)) != sizeof(long))
    eek_a_bug("long doesn't promote to long!\n");
  if (sizeof(Promoted(ss)) != sizeof(int))
    eek_a_bug("short doesn't promote to int!\n");
  if (sizeof(Promoted(ui)) != sizeof(int))
    eek_a_bug("unsigned int doesn't promote to int!\n");
#ifndef NO_UI
  if (sizeof(Promoted(ul)) != sizeof(long))
    eek_a_bug("unsigned long doesn't promote to long!\n");
  if (is_signed(ul))
    eek_a_bug("unsigned long promotes to signed!\n");
#endif

#ifndef NO_UI
  showtype("unsigned short promotes to", Promoted(us));
#endif
  showtype("long+unsigned gives", sl+ui);
}

#define checktype(x, n, s, t) if((sgn(x)!=s)||(sizeof(x)!=sizeof(t))) typerr(n, s, (int)sizeof(t), sgn(x), (int)sizeof(x));

#define fchecktype(x, n, t) if (sizeof(x) != sizeof(t)) ftyperr(n, (int)sizeof(x), (int)sizeof(t));

Void check_defines( void )
{
  /* ensure that all #defines are present and have the correct type */
#ifdef VERIFY
  int usign;

#ifdef NO_UI
  usign= Signed;
#else
  /* Implementations promote unsigned short differently */
  usign= is_signed((unsigned short)0);
#endif

  if (L) {
#ifdef CHAR_BIT
  checktype(CHAR_BIT, "CHAR_BIT", Signed, int);
#else
  missing("CHAR_BIT");
#endif
#ifdef CHAR_MAX
  checktype(CHAR_MAX, "CHAR_MAX", Signed, int);
#else
  missing("CHAR_MAX");
#endif
#ifdef CHAR_MIN
  checktype(CHAR_MIN, "CHAR_MIN", Signed, int);
#else
  missing("CHAR_MIN");
#endif
#ifdef SCHAR_MAX
  checktype(SCHAR_MAX, "SCHAR_MAX", Signed, int);
#else
  missing("SCHAR_MAX");
#endif
#ifdef SCHAR_MIN
  checktype(SCHAR_MIN, "SCHAR_MIN", Signed, int);
#else
  missing("SCHAR_MIN");
#endif
#ifdef UCHAR_MAX
  checktype(UCHAR_MAX, "UCHAR_MAX", Signed, int);
#else
  missing("UCHAR_MAX");
#endif
#ifdef SHRT_MAX
  checktype(SHRT_MAX, "SHRT_MAX", Signed, int);
#else
  missing("SHRT_MAX");
#endif
#ifdef SHRT_MIN
  checktype(SHRT_MIN, "SHRT_MIN", Signed, int);
#else
  missing("SHRT_MIN");
#endif
#ifdef INT_MAX
  checktype(INT_MAX, "INT_MAX", Signed, int);
#else
  missing("INT_MAX");
#endif
#ifdef INT_MIN
  checktype(INT_MIN, "INT_MIN", Signed, int);
#else
  missing("INT_MIN");
#endif
#ifdef LONG_MAX
  checktype(LONG_MAX, "LONG_MAX", Signed, long);
#else
  missing("LONG_MAX");
#endif
#ifdef LONG_MIN
  checktype(LONG_MIN, "LONG_MIN", Signed, long);
#else
  missing("LONG_MIN");
#endif
#ifdef USHRT_MAX
  checktype(USHRT_MAX, "USHRT_MAX", usign, int);
#else
  missing("USHRT_MAX");
#endif
#ifdef UINT_MAX
  checktype(UINT_MAX, "UINT_MAX", Unsigned, int);
#else
  missing("UINT_MAX");
#endif
#ifdef ULONG_MAX
  checktype(ULONG_MAX, "ULONG_MAX", Unsigned, long);
#else
  missing("ULONG_MAX");
#endif
  } /* if (L) */

  if (F) {
#ifdef FLT_RADIX
  checktype(FLT_RADIX, "FLT_RADIX", Signed, int);
#else
  fmissing("FLT_RADIX");
#endif
#ifdef FLT_MANT_DIG
  checktype(FLT_MANT_DIG, "FLT_MANT_DIG", Signed, int);
#else
  fmissing("FLT_MANT_DIG");
#endif
#ifdef FLT_DIG
  checktype(FLT_DIG, "FLT_DIG", Signed, int);
#else
  fmissing("FLT_DIG");
#endif
#ifdef FLT_ROUNDS
  checktype(FLT_ROUNDS, "FLT_ROUNDS", Signed, int);
#else
  fmissing("FLT_ROUNDS");
#endif
#ifdef FLT_EPSILON
  fchecktype(FLT_EPSILON, "FLT_EPSILON", float);
#else
  fmissing("FLT_EPSILON");
#endif
#ifdef FLT_MIN_EXP
  checktype(FLT_MIN_EXP, "FLT_MIN_EXP", Signed, int);
#else
  fmissing("FLT_MIN_EXP");
#endif
#ifdef FLT_MIN
  fchecktype(FLT_MIN, "FLT_MIN", float);
#else
  fmissing("FLT_MIN");
#endif
#ifdef FLT_MIN_10_EXP
  checktype(FLT_MIN_10_EXP, "FLT_MIN_10_EXP", Signed, int);
#else
  fmissing("FLT_MIN_10_EXP");
#endif
#ifdef FLT_MAX_EXP
  checktype(FLT_MAX_EXP, "FLT_MAX_EXP", Signed, int);
#else
  fmissing("FLT_MAX_EXP");
#endif
#ifdef FLT_MAX
  fchecktype(FLT_MAX, "FLT_MAX", float);
#else
  fmissing("FLT_MAX");
#endif
#ifdef FLT_MAX_10_EXP
  checktype(FLT_MAX_10_EXP, "FLT_MAX_10_EXP", Signed, int);
#else
  fmissing("FLT_MAX_10_EXP");
#endif
#ifdef DBL_MANT_DIG
  checktype(DBL_MANT_DIG, "DBL_MANT_DIG", Signed, int);
#else
  fmissing("DBL_MANT_DIG");
#endif
#ifdef DBL_DIG
  checktype(DBL_DIG, "DBL_DIG", Signed, int);
#else
  fmissing("DBL_DIG");
#endif
#ifdef DBL_EPSILON
  fchecktype(DBL_EPSILON, "DBL_EPSILON", double);
#else
  fmissing("DBL_EPSILON");
#endif
#ifdef DBL_MIN_EXP
  checktype(DBL_MIN_EXP, "DBL_MIN_EXP", Signed, int);
#else
  fmissing("DBL_MIN_EXP");
#endif
#ifdef DBL_MIN
  fchecktype(DBL_MIN, "DBL_MIN", double);
#else
  fmissing("DBL_MIN");
#endif
#ifdef DBL_MIN_10_EXP
  checktype(DBL_MIN_10_EXP, "DBL_MIN_10_EXP", Signed, int);
#else
  fmissing("DBL_MIN_10_EXP");
#endif
#ifdef DBL_MAX_EXP
  checktype(DBL_MAX_EXP, "DBL_MAX_EXP", Signed, int);
#else
  fmissing("DBL_MAX_EXP");
#endif
#ifdef DBL_MAX
  fchecktype(DBL_MAX, "DBL_MAX", double);
#else
  fmissing("DBL_MAX");
#endif
#ifdef DBL_MAX_10_EXP
  checktype(DBL_MAX_10_EXP, "DBL_MAX_10_EXP", Signed, int);
#else
  fmissing("DBL_MAX_10_EXP");
#endif
#ifdef STDC
#ifdef LDBL_MANT_DIG
  checktype(LDBL_MANT_DIG, "LDBL_MANT_DIG", Signed, int);
#else
  fmissing("LDBL_MANT_DIG");
#endif
#ifdef LDBL_DIG
  checktype(LDBL_DIG, "LDBL_DIG", Signed, int);
#else
  fmissing("LDBL_DIG");
#endif
#ifdef LDBL_EPSILON
  fchecktype(LDBL_EPSILON, "LDBL_EPSILON", long double);
#else
  fmissing("LDBL_EPSILON");
#endif
#ifdef LDBL_MIN_EXP
  checktype(LDBL_MIN_EXP, "LDBL_MIN_EXP", Signed, int);
#else
  fmissing("LDBL_MIN_EXP");
#endif
#ifdef LDBL_MIN
  fchecktype(LDBL_MIN, "LDBL_MIN", long double);
#else
  fmissing("LDBL_MIN");
#endif
#ifdef LDBL_MIN_10_EXP
  checktype(LDBL_MIN_10_EXP, "LDBL_MIN_10_EXP", Signed, int);
#else
  fmissing("LDBL_MIN_10_EXP");
#endif
#ifdef LDBL_MAX_EXP
  checktype(LDBL_MAX_EXP, "LDBL_MAX_EXP", Signed, int);
#else
  fmissing("LDBL_MAX_EXP");
#endif
#ifdef LDBL_MAX
  fchecktype(LDBL_MAX, "LDBL_MAX", long double);
#else
  fmissing("LDBL_MAX");
#endif
#ifdef LDBL_MAX_10_EXP
  checktype(LDBL_MAX_10_EXP, "LDBL_MAX_10_EXP", Signed, int);
#else
  fmissing("LDBL_MAX_10_EXP");
#endif
#endif /* STDC */
  } /* if (F) */
#endif /* VERIFY */
}

#ifdef VERIFY
#ifndef SCHAR_MAX
#define SCHAR_MAX char_max
#endif
#ifndef SCHAR_MIN
#define SCHAR_MIN char_min
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX char_max
#endif
#endif /* VERIFY */

#ifndef CHAR_BIT
#define CHAR_BIT  char_bit
#endif
#ifndef CHAR_MAX
#define CHAR_MAX  char_max
#endif
#ifndef CHAR_MIN
#define CHAR_MIN  char_min
#endif
#ifndef SCHAR_MAX
#define SCHAR_MAX char_max
#endif
#ifndef SCHAR_MIN
#define SCHAR_MIN char_min
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX char_max
#endif

int cprop( void )
{
  /* Properties of type char */
  Volatile char c, char_max, char_min;
  Volatile int byte_size, c_signed;
  long char_bit;

  Unexpected(2);

  /* Calculate number of bits per character *************************/
  c=1; byte_size=0;
  do { c=c<<1; byte_size++; } while(c!=0);
  c= (char)(-1);
  if (((int)c)<0) c_signed=1;
  else c_signed=0;
  Vprintf("%schar = %d bits, %ssigned%s\n",
    co, (int)sizeof(c)*byte_size, (c_signed?"":"un"), oc);
  char_bit=(long)(sizeof(c)*byte_size);
  if (L) i_define(D_CHAR_BIT, "", "CHAR", "_BIT",
      char_bit, 0L, (long) CHAR_BIT, "");

  c=0; char_max=0;
  c++;
  if (setjmp(lab)==0) { /* Yields char_max */
    while (c>char_max) {
      char_max=c;
      c=(char)(c*2+1);
    }
  } else {
    Vprintf("%sCharacter overflow generates a trap!%s\n", co, oc);
  }
  c=0; char_min=0;
  c--;
  if (c<char_min) /* then the min char < 0 */ {
     /* Minimum value: assume either two's or one's complement *********/
    char_min= -char_max;
    if (setjmp(lab)==0) { /* Yields char_min */
        if (char_min-1 < char_min) char_min--;
    }
  }
  Unexpected(8);
  if (c_signed && char_min == 0) {
    Vprintf("%sBEWARE! Chars are pseudo-unsigned:%s\n", co, oc);
    Vprintf("%s   %s%s%s\n",
      "They contain only nonnegative values, ",
      "but sign extend when used as integers.", co, oc);
  }
  Unexpected(3);

  if (L) {
    /* Because of the integer promotions, you must use a U after
       the MAX_CHARS in the following cases */
    if ((sizeof(char) == sizeof(int)) && !c_signed) {
      u_define(D_CHAR_MAX, "", "CHAR", "_MAX",
         (ulong) char_max,
         (ulong) CHAR_MAX, "");
    } else {
      i_define(D_CHAR_MAX, "", "CHAR", "_MAX",
         (long) char_max, 0L,
         (long) CHAR_MAX, "");
    }
    i_define(D_CHAR_MIN, "", "CHAR", "_MIN",
       (long) char_min, (long) maxint,
       (long) CHAR_MIN, "");
    if (c_signed) {
      i_define(D_SCHAR_MAX, "", "SCHAR", "_MAX",
         (long) char_max, 0L,
         (long) SCHAR_MAX, "");
      i_define(D_SCHAR_MIN, "", "SCHAR", "_MIN",
         (long) char_min, (long) maxint,
         (long) SCHAR_MIN, "");
    } else {
      if (sizeof(char) == sizeof(int)) {
        u_define(D_UCHAR_MAX, "", "UCHAR", "_MAX",
           (ulong) char_max,
           (ulong) UCHAR_MAX, "");
      } else {
        i_define(D_UCHAR_MAX, "", "UCHAR", "_MAX",
           (long) char_max, 0L,
           (long) UCHAR_MAX, "");
      }
    }

    if (c_signed) {
#ifndef NO_UC
/* Syntax error? Define NO_UC */ Volatile unsigned char c1, char_max;
      c1=0; char_max=0;
      c1++;
      if (setjmp(lab)==0) { /* Yields char_max */
        while (c1>char_max) {
          char_max=c1;
          c1++;
        }
      }
      Unexpected(4);
      if (sizeof(char) == sizeof(int)) {
        u_define(D_UCHAR_MAX, "", "UCHAR", "_MAX",
           (ulong) char_max,
           (ulong) UCHAR_MAX, "");
      } else {
        i_define(D_UCHAR_MAX, "", "UCHAR", "_MAX",
           (long) char_max, 0L,
           (long) UCHAR_MAX, "");
      }
#endif
    } else {
#ifndef NO_SC
/* Syntax error? Define NO_SC */ Volatile signed char c1, char_max, char_min;
      c1=0; char_max=0;
      c1++;
      if (setjmp(lab)==0) { /* Yields char_max */
        while (c1>char_max) {
          char_max=c1;
          c1++;
        }
      }
      c1=0; char_min=0;
      c1--;
      if (setjmp(lab)==0) { /* Yields char_min */
        while (c1<char_min) {
          char_min=c1;
          c1--;
        }
      }
      Unexpected(5);
      i_define(D_SCHAR_MIN, "", "SCHAR", "_MIN",
         (long) char_min, (long) maxint,
         (long) SCHAR_MIN, "");
      i_define(D_SCHAR_MAX, "", "SCHAR", "_MAX",
         (long) char_max, 0L,
         (long) SCHAR_MAX, "");
#endif /* NO_SC */
    }
  }
  return byte_size;
}

int basic( void )
{
  /* The properties of the basic types.
     Returns number of bits per sizeof unit */
  Volatile int byte_size;
  typedef int function ();
  int variable;
  char *cp; int *ip; function *fp;
  int *p, *q;

  Vprintf("%sSIZES%s\n", co, oc);
  byte_size= cprop();

  /* Shorts, ints and longs *****************************************/
  Vprintf("%sshort=%d int=%d long=%d float=%d double=%d bits %s\n",
    co,
    (int) sizeof(short)*byte_size,
    (int) sizeof(int)*byte_size,
    (int) sizeof(long)*byte_size,
    (int) sizeof(float)*byte_size,
    (int) sizeof(double)*byte_size, oc);
  if (stdc) {
    Vprintf("%slong double=%d bits%s\n",
      co, (int) sizeof(Long_double)*byte_size, oc);
  }
  Vprintf("%schar*=%d bits%s%s\n",
    co, (int)sizeof(char *)*byte_size,
    sizeof(char *)>sizeof(int)?" BEWARE! larger than int!":"",
    oc);
  Vprintf("%sint* =%d bits%s%s\n",
    co, (int)sizeof(int *)*byte_size,
    sizeof(int *)>sizeof(int)?" BEWARE! larger than int!":"",
    oc);
  Vprintf("%sfunc*=%d bits%s%s\n",
    co, (int)sizeof(function *)*byte_size,
    sizeof(function *)>sizeof(int)?" BEWARE! larger than int!":"",
    oc);

  showtype("Type size_t is", sizeof(0));
#ifdef STDC
  showtype("Type wchar_t is", L'x');
#endif

  /* Alignment constants ********************************************/

#define alignment(TYPE) \
  ((long)((char *)&((struct{char c; TYPE d;}*)0)->d - (char *)0))

  Vprintf("\n%sALIGNMENTS%s\n", co, oc);

  Vprintf("%schar=%ld short=%ld int=%ld long=%ld%s\n",
    co,
    alignment(char), alignment(short),
    alignment(int), alignment(long),
    oc);

  Vprintf("%sfloat=%ld double=%ld%s\n",
    co,
    alignment(float), alignment(double),
    oc);

  if (stdc) {
    Vprintf("%slong double=%ld%s\n",
      co,
      alignment(Long_double),
      oc);
  }
  Vprintf("%schar*=%ld int*=%ld func*=%ld%s\n",
    co,
    alignment(char *), alignment(int *), alignment(function *),
    oc);

  Vprintf("\n");

  /* Ten little endians *********************************************/

  endian(byte_size);

  /* Pointers *******************************************************/

  Vprintf("\n%sPROPERTIES OF POINTERS%s\n", co, oc);
  cp= (char *) &variable;
  ip= (int *) &variable;
  fp= (function *) &variable;

  Vprintf("%sChar and int pointer formats ", co);
  if (memeq((char *) &cp, sizeof(cp), (char *) &ip, sizeof(ip))) {
    Vprintf("seem identical%s\n", oc);
  } else {
    Vprintf("are different%s\n", oc);
  }
  Vprintf("%sChar and function pointer formats ", co);
  if (memeq((char *) &cp, sizeof(cp), (char *) &fp, sizeof(fp))) {
    Vprintf("seem identical%s\n", oc);
  } else {
    Vprintf("are different%s\n", oc);
  }

  if (V) {
    if ((VOID *)"abcd" == (VOID *)"abcd")
      printf("%sStrings are shared%s\n", co, oc);
    else printf("%sStrings are not shared%s\n", co, oc);
  }

  p=0; q=0;
  showtype("Type ptrdiff_t is", p-q);

  //if (setjmp(mlab) == 0) {
  //  variable= *p;
  //  Vprintf("%sBEWARE! Dereferencing NULL doesn't cause a trap%s\n",
  //    co, oc);
  //} else {
  //  Vprintf("%sDereferencing NULL causes a trap%s\n", co, oc);
  //}
  if (setjmp(mlab)!=0) croak(-2);

  Vprintf("\n%sPROPERTIES OF INTEGRAL TYPES%s\n", co, oc);

  sprop();
  iprop();
  lprop();
  usprop();
  uiprop();
  ulprop();

  promotions();

  Unexpected(6);

  return byte_size;
}

#else /* not PASS0 */

#ifdef SEP
/* The global variables used by several passes */
extern jmp_buf lab;
extern int V, L, F, bugs, bits_per_byte;
extern int maxint, flt_radix, flt_rounds;
extern Volatile int trapped;
extern char co[], oc[];
extern char *f_rep();
extern Void trap1();
#endif /* SEP */
#endif /* ifdef PASS0 */

/* As I said, I apologise for the contortions below. The functions are
   expanded by the preprocessor twice or three times (for float and double,
   and maybe for long double, and for short, int and long). That way,
   I never make a change to one that I forget to make to the other.
   You can look on it as C's fault for not supporting multi-line macros.
   This whole file is read 3 times by the preprocessor, with PASSn set for
   n=1, 2 or 3, to decide which parts to reprocess.
*/

/* #undef on an already undefined thing is (wrongly) flagged as an error
   by some compilers, therefore the #ifdef that follows:
*/
#ifdef Number
#undef Number
#undef THING
#undef Thing
#undef thing
#undef FPROP
#undef Fname
#undef Store
#undef Sum
#undef Diff
#undef Mul
#undef Div
#undef ZERO
#undef HALF
#undef ONE
#undef TWO
#undef THREE
#undef FOUR
#undef Self
#undef F_check
#undef Verify
#undef EPROP
#undef MARK

/* These are the float.h constants */
#undef F_RADIX
#undef F_MANT_DIG
#undef F_DIG
#undef F_ROUNDS
#undef F_EPSILON
#undef F_MIN_EXP
#undef F_MIN
#undef F_MIN_10_EXP
#undef F_MAX_EXP
#undef F_MAX
#undef F_MAX_10_EXP
#endif

#ifdef Integer
#undef Integer
#undef INT
#undef IPROP
#undef Iname
#undef UPROP
#undef Uname
#undef OK_UI
#undef IMARK

#undef I_MAX
#undef I_MIN
#undef U_MAX
#endif

#ifdef PASS1

/* Define the things we're going to use this pass */

#define Number  float
#define THING "FLOAT"
#define Thing "Float"
#define thing "float"
#define Fname "FLT"
#define FPROP fprop
#define Store fStore
#define Sum fSum
#define Diff  fDiff
#define Mul fMul
#define Div fDiv
#define ZERO  0.0
#define HALF  0.5
#define ONE 1.0
#define TWO 2.0
#define THREE 3.0
#define FOUR  4.0
#define Self  fSelf
#define F_check fCheck
#define MARK  "F"
#ifdef VERIFY
#define Verify fVerify
#endif

#define EPROP efprop

#define Integer short
#define INT "short"
#define IPROP sprop
#define Iname "SHRT"
#ifndef NO_UI
#define OK_UI 1
#endif
#define IMARK ""

#define UPROP usprop
#define Uname "USHRT"

#ifdef VERIFY
#ifdef SHRT_MAX
#define I_MAX   SHRT_MAX
#endif
#ifdef SHRT_MIN
#define I_MIN   SHRT_MIN
#endif
#ifdef USHRT_MAX
#define U_MAX   USHRT_MAX
#endif

#ifdef FLT_RADIX
#define F_RADIX   FLT_RADIX
#endif
#ifdef FLT_MANT_DIG
#define F_MANT_DIG  FLT_MANT_DIG
#endif
#ifdef FLT_DIG
#define F_DIG   FLT_DIG
#endif
#ifdef FLT_ROUNDS
#define F_ROUNDS  FLT_ROUNDS
#endif
#ifdef FLT_EPSILON
#define F_EPSILON FLT_EPSILON
#endif
#ifdef FLT_MIN_EXP
#define F_MIN_EXP FLT_MIN_EXP
#endif
#ifdef FLT_MIN
#define F_MIN   FLT_MIN
#endif
#ifdef FLT_MIN_10_EXP
#define F_MIN_10_EXP  FLT_MIN_10_EXP
#endif
#ifdef FLT_MAX_EXP
#define F_MAX_EXP FLT_MAX_EXP
#endif
#ifdef FLT_MAX
#define F_MAX   FLT_MAX
#endif
#ifdef FLT_MAX_10_EXP
#define F_MAX_10_EXP  FLT_MAX_10_EXP
#endif
#endif /* VERIFY */

#endif /* PASS1 */

#ifdef PASS2

#define Number  double
#define THING "DOUBLE"
#define Thing "Double"
#define thing "double"
#define Fname "DBL"
#define FPROP dprop
#define Store dStore
#define Sum dSum
#define Diff  dDiff
#define Mul dMul
#define Div dDiv
#define ZERO  0.0
#define HALF  0.5
#define ONE 1.0
#define TWO 2.0
#define THREE 3.0
#define FOUR  4.0
#define Self  dSelf
#define F_check dCheck
#define MARK  ""
#ifdef VERIFY
#define Verify dVerify
#endif

#define EPROP edprop

#define Integer int
#define INT "int"
#define IPROP iprop
#define Iname "INT"
#define OK_UI 1 /* Unsigned int is always possible */
#define IMARK ""

#define UPROP uiprop
#define Uname "UINT"

#ifdef VERIFY
#ifdef INT_MAX
#define I_MAX   INT_MAX
#endif
#ifdef INT_MIN
#define I_MIN   INT_MIN
#endif
#ifdef UINT_MAX
#define U_MAX   UINT_MAX
#endif

#ifdef DBL_MANT_DIG
#define F_MANT_DIG  DBL_MANT_DIG
#endif
#ifdef DBL_DIG
#define F_DIG   DBL_DIG
#endif
#ifdef DBL_EPSILON
#define F_EPSILON DBL_EPSILON
#endif
#ifdef DBL_MIN_EXP
#define F_MIN_EXP DBL_MIN_EXP
#endif
#ifdef DBL_MIN
#define F_MIN   DBL_MIN
#endif
#ifdef DBL_MIN_10_EXP
#define F_MIN_10_EXP  DBL_MIN_10_EXP
#endif
#ifdef DBL_MAX_EXP
#define F_MAX_EXP DBL_MAX_EXP
#endif
#ifdef DBL_MAX
#define F_MAX   DBL_MAX
#endif
#ifdef DBL_MAX_10_EXP
#define F_MAX_10_EXP  DBL_MAX_10_EXP
#endif
#endif /* VERIFY */

#endif /* PASS2 */

#ifdef PASS3

#ifdef STDC
#define Number  long double

#define ZERO  0.0L
#define HALF  0.5L
#define ONE 1.0L
#define TWO 2.0L
#define THREE 3.0L
#define FOUR  4.0L
#endif

#define THING "LONG DOUBLE"
#define Thing "Long double"
#define thing "long double"
#define Fname "LDBL"
#define FPROP ldprop
#define Store ldStore
#define Sum ldSum
#define Diff  ldDiff
#define Mul ldMul
#define Div ldDiv
#define Self  ldSelf
#define F_check ldCheck
#define MARK  "L"
#ifdef VERIFY
#define Verify ldVerify
#endif

#define EPROP eldprop

#define Integer long
#define INT "long"
#define IPROP lprop
#define Iname "LONG"
#ifndef NO_UI
#define OK_UI 1
#endif
#define IMARK "L"

#define UPROP ulprop
#define Uname "ULONG"

#ifdef VERIFY
#ifdef LONG_MAX
#define I_MAX LONG_MAX
#endif
#ifdef LONG_MIN
#define I_MIN LONG_MIN
#endif
#ifdef ULONG_MAX
#define U_MAX ULONG_MAX
#endif

#ifdef LDBL_MANT_DIG
#define F_MANT_DIG  LDBL_MANT_DIG
#endif
#ifdef LDBL_DIG
#define F_DIG   LDBL_DIG
#endif
#ifdef LDBL_EPSILON
#define F_EPSILON LDBL_EPSILON
#endif
#ifdef LDBL_MIN_EXP
#define F_MIN_EXP LDBL_MIN_EXP
#endif
#ifdef LDBL_MIN
#define F_MIN   LDBL_MIN
#endif
#ifdef LDBL_MIN_10_EXP
#define F_MIN_10_EXP  LDBL_MIN_10_EXP
#endif
#ifdef LDBL_MAX_EXP
#define F_MAX_EXP LDBL_MAX_EXP
#endif
#ifdef LDBL_MAX
#define F_MAX   LDBL_MAX
#endif
#ifdef LDBL_MAX_10_EXP
#define F_MAX_10_EXP  LDBL_MAX_10_EXP
#endif
#endif /* VERIFY */

#endif /* PASS3 */

/* The rest of the file gets read all three times;
   the differences are encoded in the things #defined above.
*/

#ifndef I_MAX
#define I_MAX int_max
#endif
#ifndef I_MIN
#define I_MIN int_min
#endif
#ifndef U_MAX
#define U_MAX u_max
#endif

#ifndef F_RADIX
#define F_RADIX   f_radix
#endif
#ifndef F_MANT_DIG
#define F_MANT_DIG  f_mant_dig
#endif
#ifndef F_DIG
#define F_DIG   f_dig
#endif
#ifndef F_ROUNDS
#define F_ROUNDS  f_rounds
#endif
#ifndef F_EPSILON
#define F_EPSILON f_epsilon
#endif
#ifndef F_MIN_EXP
#define F_MIN_EXP f_min_exp
#endif
#ifndef F_MIN
#define F_MIN   f_min
#endif
#ifndef F_MIN_10_EXP
#define F_MIN_10_EXP  f_min_10_exp
#endif
#ifndef F_MAX_EXP
#define F_MAX_EXP f_max_exp
#endif
#ifndef F_MAX
#define F_MAX   f_max
#endif
#ifndef F_MAX_10_EXP
#define F_MAX_10_EXP  f_max_10_exp
#endif

#ifndef VERIFY
#define Verify(prec, val, req, same, same1) {;}
#endif

#ifdef Integer

Void IPROP( void )
{
  /* the properties of short, int, and long */
  Volatile Integer newi, int_max, maxeri, int_min, minneri;
  Volatile int ibits, ipower, two=2;

  /* Calculate max short/int/long ***********************************/
  /* Calculate 2**n-1 until overflow - then use the previous value  */

  newi=1; int_max=0;

  if (setjmp(lab)==0) { /* Yields int_max */
    for(ipower=0; newi>int_max; ipower++) {
      int_max=newi;
      newi=newi*two+1;
    }
    Vprintf("%sOverflow of a%s %s does not generate a trap%s\n",
      co, INT[0]=='i'?"n":"", INT, oc);
  } else {
    Vprintf("%sOverflow of a%s %s generates a trap%s\n",
      co, INT[0]=='i'?"n":"", INT, oc);
  }
  Unexpected(7);

  /* Minimum value: assume either two's or one's complement *********/
  int_min= -int_max;
  if (setjmp(lab)==0) { /* Yields int_min */
    if (int_min-1 < int_min) int_min--;
  }
  Unexpected(8);

  /* Now for those daft Cybers */

  maxeri=0; newi=int_max;

  if (setjmp(lab)==0) { /* Yields maxeri */
    for(ibits=ipower; newi>maxeri; ibits++) {
      maxeri=newi;
      newi=newi+newi+1;
    }
  }
  Unexpected(9);

  minneri= -maxeri;
  if (setjmp(lab)==0) { /* Yields minneri */
    if (minneri-1 < minneri) minneri--;
  }
  Unexpected(10);

  Vprintf("%sMaximum %s = %ld (= 2**%d-1)%s\n",
    co, INT, (long)int_max, ipower, oc);
  Vprintf("%sMinimum %s = %ld%s\n", co, INT, (long)int_min, oc);

  if (L) i_define(D_INT_MAX, INT, Iname, "_MAX",
      (long) int_max, 0L,
      (long) I_MAX, IMARK);
  if (L) i_define(D_INT_MIN, INT, Iname, "_MIN",
      (long) int_min, (long) (PASS==1?maxint:int_max),
      (long) I_MIN, IMARK);

  if(int_max < 0) { /* It has happened (on a Cray) */
    eek_a_bug("signed integral comparison faulty?");
  }

  if (maxeri>int_max) {
    Vprintf("%sThere is a larger %s, %ld (= 2**%d-1), %s %s%s\n",
      co, INT, (long)maxeri, ibits,
      "but only for addition, not multiplication",
      "(I smell a Cyber!)",
      oc);
  }

  if (minneri<int_min) {
    Vprintf("%sThere is a smaller %s, %ld, %s %s%s\n",
      co, INT, (long)minneri,
      "but only for addition, not multiplication",
      "(I smell a Cyber!)",
      oc);
  }
}

Void UPROP ( void )
{
  /* The properties of unsigned short/int/long */
#ifdef OK_UI
  Volatile unsigned Integer u_max, newi, two;
  newi=1; u_max=0; two=2;

  if (setjmp(lab)==0) { /* Yields u_max */
    while(newi>u_max) {
      u_max=newi;
      newi=newi*two+1;
    }
  }
  Unexpected(11);
  Vprintf("%sMaximum unsigned %s = %lu%s\n",
    co, INT, (unsigned long) u_max, oc);

  /* Oh woe: new standard C defines value preserving promotions:
     3.2.1.1: "If an int can represent all values of the original type,
           the value is converted to an int;
         otherwise it is converted to an unsigned int."
  */
  if (L) {
    if (PASS == 1 /* we're dealing with short */
        && u_max <= maxint /* an int can represent all values */
        )
    { /* the value is converted to an int */
      i_define(D_UINT_MAX, INT, Uname, "_MAX",
         (long) u_max, 0L,
         (long) U_MAX, IMARK);
    } else { /* it is converted to an unsigned int */
      u_define(D_UINT_MAX, INT, Uname, "_MAX",
         (unsigned long) u_max,
         (unsigned long) U_MAX, IMARK);
    }
  }
#endif
}

#endif /* Integer */

#ifdef Number

/* The following routines are intended to defeat any attempt at optimisation
   or use of extended precision, and to defeat faulty narrowing casts.
   The weird prototypes are because of widening incompatibilities.
*/
#if defined(STDC) || defined(_MSC_VER)
#define ARGS1(A, a) (A a)
#define ARGS2(A, a, B, b) (A a, B b)
#define ARGS5(A, a, B, b, C, c, D, d, E, e) (A a, B b, C c, D d, E e)
#else
#define ARGS1(A, a) (a) A a;
#define ARGS2(A, a, B, b) (a, b) A a; B b;
#define ARGS5(A, a, B, b, C, c, D, d, E, e) (a,b,c,d,e)A a; B b; C c; D d; E e;
#endif

Void Store ARGS2(Number, a, Number *, b) { *b=a; }
Number Sum ARGS2(Number, a, Number, b) {Number r; Store(a+b, &r); return r; }
Number Diff ARGS2(Number, a, Number, b){Number r; Store(a-b, &r); return r; }
Number Mul ARGS2(Number, a, Number, b) {Number r; Store(a*b, &r); return r; }
Number Div ARGS2(Number, a, Number, b) {Number r; Store(a/b, &r); return r; }
Number Self ARGS1(Number, a)         {Number r; Store(a,   &r); return r; }

Void F_check ARGS((int precision, Long_double val1));

Void F_check(int precision, Long_double val1)
{
  /* You don't think I'm going to go to all the trouble of writing
     a program that works out what all sorts of values are, only to
     have printf go and print the wrong values out, do you?
     No, you're right, so this function tries to see if printf
     has written the right value, by reading it back again.
     This introduces a new problem of course: suppose printf writes
     the correct value, and scanf reads it back wrong... oh well.
     But I'm adamant about this: the precision given is enough
     to uniquely identify the printed number, therefore I insist
     that sscanf read the number back identically. Harsh yes, but
     sometimes you've got to be cruel to be kind.
  */
  Long_double new1, rem;
  Number val, new, diff;
  int e;
  char *rep, *f2;

  if (sizeof(double) == sizeof(Long_double)) {
    /* Assume they're the same, and use non-stdc format */
    /* This is for stdc compilers using non-stdc libraries */
    f2= "%le";   /* Input */
  } else {
    /* It had better support Le then */
    f2= "%Le";
  }
  val= (Number) val1;
  rep= f_rep(precision, (Long_double) val);
  if (setjmp(lab)==0) {
    sscanf(rep, f2, &new1);
  } else {
    eek_a_bug("sscanf caused a trap");
    printf("%s    scanning: %s format: %s%s\n\n", co, rep, f2, oc);
    Unexpected(12);
    return;
  }

  if (setjmp(lab)==0) { /* See if new is usable */
    new= new1;
    if (new != 0.0) {
      diff= val/new - 1.0;
      if (diff < 0.1) diff= 1.0;
      /* That should be enough to generate a trap */
    }
  } else {
    eek_a_bug("sscanf returned an unusable number");
    printf("%s    scanning: %s with format: %s%s\n\n",
           co, rep, f2, oc);
    Unexpected(13);
    return;
  }

  Unexpected(14);
  if (new != val) {
    eek_a_bug("Possibly bad output from printf above");
    if (!exponent((Long_double)val, &rem, &e)) {
      printf("%s    but value was an unusable number%s\n\n",
             co, oc);
      return;
    }
    printf("%s    expected value around ", co);
    //if (sizeof(double) == sizeof(Long_double)) {
      /* Assume they're the same, and use non-stdc format */
      /* This is for stdc compilers using non-stdc libraries */
      //printf("%.*fe%d, bit pattern:\n    ", precision, rem, e);
    //} else {
      /* It had better support Lfe then */
      printf("%.*Lfe%d, bit pattern:\n    ", precision, rem, e);
    //}
    bitpattern((char *) &val, (unsigned)sizeof(val));
    printf ("%s\n", oc);
    printf("%s    sscanf gave           %s, bit pattern:\n    ",
           co, f_rep(precision, (Long_double) new));
    bitpattern((char *) &new, (unsigned)sizeof(new));
    printf ("%s\n", oc);
    if (setjmp(lab) == 0) {
      diff= val-new;
      printf("%s    difference= %s%s\n\n",
             co, f_rep(precision, (Long_double) diff), oc);
    } /* else forget it */
    Unexpected(15);
  }
}

#ifdef VERIFY
Void Verify ARGS5(int, prec, Number, val, Number, req, int, same, int, same1)
{
  /* Check that the compiler has read a #define value correctly */
  Unexpected(16);
  if (!same) {
    printf("%s*** Verify failed for above #define!\n", co);
    if (setjmp(lab) == 0) { /* for the case that req == nan */
      printf("       Compiler has %s for value\n",
             f_rep(prec, req));
    } else {
      printf("       Compiler has %s for value\n",
             "an unusable number");
    }
    if (setjmp(lab) == 0) {
      F_check(prec, (Long_double) req);
    } /*else forget it*/
    if (setjmp(lab) == 0) {
      if (req > 0.0 && val > 0.0) {
        printf("       difference= %s\n",
               f_rep(prec, val-req));
      }
    } /*else forget it*/
    Unexpected(17);
    printf("%s\n", oc);
    bugs++;
  } else if (!same1) {
    if (stdc) eek_a_bug("constant has the wrong precision");
    else eek_a_bug("the cast didn't work");
    printf("\n");
  }
}
#endif /* VERIFY */

int FPROP(int byte_size)
{
  /* Properties of floating types, using algorithms by Cody and Waite
     from MA Malcolm, as modified by WM Gentleman and SB Marovich.
     Further extended by S Pemberton.

     Returns the number of digits in the fraction.
  */

  Volatile int
    i, f_radix, iexp, irnd, mrnd, f_rounds, f_mant_dig,
    iz, k, inf, machep, f_max_exp, f_min_exp, mx, negeps,
    mantbits, digs, f_dig, trap,
    hidden, normal, f_min_10_exp, f_max_10_exp;
  Volatile Number
    a, b, base, basein, basem1, f_epsilon, epsneg,
    eps, epsp1, etop, ebot,
    f_max, newxmax, f_min, xminner, y, y1, z, z1, z2;

  Unexpected(18);

  Vprintf("%sPROPERTIES OF %s%s\n", co, THING, oc);

  /* Base and size of significand **************************************/
  /* First repeatedly double until adding 1 has no effect.    */
  /* For instance, if base is 10, with 3 significant digits   */
  /* it will try 1, 2, 4, 8, ... 512, 1024, and stop there,   */
  /* since 1024 is only representable as 1020.        */
  a=1.0;
  if (setjmp(lab)==0) { /* inexact trap? */
    do { a=Sum(a, a); }
    while (Diff(Diff(Sum(a, ONE), a), ONE) == ZERO);
  } else {
    fprintf(stderr, "*** Program got loss-of-precision trap!\n");
    /* And supporting those is just TOO much trouble! */
    farewell(bugs+1);
  }
  Unexpected(19);
  /* Now double until you find a number that can be added to the    */
  /* above number. For 1020 this is 8 or 16, depending whether the  */
  /* result is rounded or truncated.          */
  /* In either case the result is 1030. 1030-1020= the base, 10.    */
  b=1.0;
  do { b=Sum(b, b); } while ((base=Diff(Sum(a, b), a)) == ZERO);
  f_radix=base;
  Vprintf("%sBase = %d%s\n", co, f_radix, oc);

  /* Sanity check; if base<2, I can't guarantee the rest will work  */
  if (f_radix < 2) {
    eek_a_bug("Function return or parameter passing faulty? (This is a guess.)");
    printf("\n");
    return(0);
  }

  if (PASS == 1) { /* only for FLT */
    flt_radix= f_radix;
    if (F) i_define(D_FLT_RADIX, "", "FLT", "_RADIX",
        (long) f_radix, 0L, (long) F_RADIX, "");
  } else if (f_radix != flt_radix) {
    printf("\n%s*** WARNING: %s %s (%d) %s%s\n",
           co, thing, "arithmetic has a different radix",
           f_radix, "from float", oc);
    bugs++;
  }

  /* Now the number of digits precision */
  f_mant_dig=0; b=1.0;
  do { f_mant_dig++; b=Mul(b, base); }
  while (Diff(Diff(Sum(b, ONE), b), ONE) == ZERO);
  f_dig=floor_log(10, (Long_double)(b/base)) + (base==10?1:0);
  Vprintf("%sSignificant base digits = %d %s %d %s%s\n",
    co, f_mant_dig, "(= at least", f_dig, "decimal digits)", oc);
  if (F) i_define(D_MANT_DIG, thing, Fname, "_MANT_DIG",
      (long) f_mant_dig, 0L, (long) F_MANT_DIG, "");
  if (F) i_define(D_DIG, thing, Fname, "_DIG",
      (long) f_dig, 0L, (long) F_DIG, "");
  digs= ceil_log(10, (Long_double)b); /* the number of digits to printf */

  /* Rounding *******************************************************/
  basem1=Diff(base, HALF);
  if (Diff(Sum(a, basem1), a) != ZERO) {
    if (f_radix == 2) basem1=0.375;
    else basem1=1.0;
    if (Diff(Sum(a, basem1), a) != ZERO) irnd=2; /* away from 0 */
    else irnd=1; /* to nearest */
  } else irnd=0; /* towards 0 */

  basem1=Diff(base, HALF);

  if (Diff(Diff(-a, basem1), -a) != ZERO) {
    if (f_radix == 2) basem1=0.375;
    else basem1=1.0;
    if (Diff(Diff(-a, basem1), -a) != ZERO) mrnd=2; /* away from 0*/
    else mrnd=1; /* to nearest */
  } else mrnd=0; /* towards 0 */

  f_rounds= -1; /* Unknown rounding */
  if (irnd==0 && mrnd==0) f_rounds=0; /* zero = chops */
  if (irnd==1 && mrnd==1) f_rounds=1; /* nearest */
  if (irnd==2 && mrnd==0) f_rounds=2; /* +inf */
  if (irnd==0 && mrnd==2) f_rounds=3; /* -inf */

  if (f_rounds != -1) {
    Vprintf("%sArithmetic rounds towards ", co);
    switch (f_rounds) {
          case 0: Vprintf("zero (i.e. it chops)"); break;
          case 1: Vprintf("nearest"); break;
          case 2: Vprintf("+infinity"); break;
          case 3: Vprintf("-infinity"); break;
          default: Vprintf("???"); break;
    }
    Vprintf("%s\n", oc);
  } else { /* Hmm, try to give some help here */
    Vprintf("%sArithmetic rounds oddly: %s\n", co, oc);
    Vprintf("%s    Negative numbers %s%s\n",
      co, mrnd==0 ? "towards zero" :
          mrnd==1 ? "to nearest" :
              "away from zero",
      oc);
    Vprintf("%s    Positive numbers %s%s\n",
      co, irnd==0 ? "towards zero" :
          irnd==1 ? "to nearest" :
              "away from zero",
      oc);
  }
  /* An extra goody */
  if (f_radix == 2 && f_rounds == 1) {
    if (Diff(Sum(a, ONE), a) != ZERO) {
      Vprintf("%s   Tie breaking rounds up%s\n", co, oc);
    } else if (Diff(Sum(a, THREE), a) == FOUR) {
      Vprintf("%s   Tie breaking rounds to even%s\n", co, oc);
    } else {
      Vprintf("%s   Tie breaking rounds down%s\n", co, oc);
    }
  }
  if (PASS == 1) { /* only for FLT */
    flt_rounds= f_rounds;
    if (F)
      i_define(D_FLT_ROUNDS, "", "FLT", "_ROUNDS",
         (long) f_rounds, 1L, (long) F_ROUNDS, "");
  } else if (f_rounds != flt_rounds) {
    printf("\n%s*** WARNING: %s %s (%d) %s%s\n\n",
           co, thing, "arithmetic rounds differently",
           f_rounds, "from float", oc);
    bugs++;
  }

  /* Various flavours of epsilon ************************************/
  negeps=f_mant_dig+f_mant_dig;
  basein=1.0/base;
  a=1.0;
  for(i=1; i<=negeps; i++) a*=basein;

  b=a;
  while (Diff(Diff(ONE, a), ONE) == ZERO) {
    a*=base;
    negeps--;
  }
  negeps= -negeps;
  Vprintf("%sSmallest x such that 1.0-base**x != 1.0 = %d%s\n",
    co, negeps, oc);

  etop = ONE;
  ebot = ZERO;
  eps = Sum(ebot, Div(Diff(etop, ebot), TWO));
  /* find the smallest epsneg (1-epsneg != 1) by binary search.
     ebot and etop are the current bounds */
  while (eps != ebot && eps != etop) {
    epsp1 = Diff(ONE, eps);
    if (epsp1 < ONE) etop = eps;
    else ebot = eps;
    eps = Sum(ebot, Div(Diff(etop, ebot), TWO));
  }
  eps= etop;
  /* Sanity check */
  if (Diff(ONE, etop) >= ONE || Diff(ONE, ebot) != ONE) {
    eek_a_bug("internal error calculating epsneg");
  }
  Vprintf("%sSmallest x such that 1.0-x != 1.0 = %s%s\n",
    co, f_rep(digs, (Long_double) eps), oc);
  if (V) F_check(digs, (Long_double) eps);

  epsneg=a;
  if ((f_radix!=2) && irnd) {
  /*  a=(a*(1.0+a))/(1.0+1.0); => */
    a=Div(Mul(a, Sum(ONE, a)), Sum(ONE, ONE));
  /*  if ((1.0-a)-1.0 != 0.0) epsneg=a; => */
    if (Diff(Diff(ONE, a), ONE) != ZERO) epsneg=a;
  }
  /* epsneg is used later */
  Unexpected(20);

  machep= -f_mant_dig-f_mant_dig;
  a=b;
  while (Diff(Sum(ONE, a), ONE) == ZERO) { a*=base; machep++; }
  Vprintf("%sSmallest x such that 1.0+base**x != 1.0 = %d%s\n",
    co, machep, oc);

  etop = ONE;
  ebot = ZERO;
  eps = Sum(ebot, Div(Diff(etop, ebot), TWO));
  /* find the smallest eps (1+eps != 1) by binary search.
     ebot and etop are the current bounds */
  while (eps != ebot && eps != etop) {
    epsp1 = Sum(ONE, eps);
    if (epsp1 > ONE) etop = eps;
    else ebot = eps;
    eps = Sum(ebot, Div(Diff(etop, ebot), TWO));
  }
  /* Sanity check */
  if (Sum(ONE, etop) <= ONE || Sum(ONE, ebot) != ONE) {
    eek_a_bug("internal error calculating eps");
  }
  f_epsilon=etop;

  Vprintf("%sSmallest x such that 1.0+x != 1.0 = %s%s\n",
    co, f_rep(digs, (Long_double) f_epsilon), oc);

  f_epsilon= Diff(Sum(ONE, f_epsilon), ONE); /* New C standard defn */
  Vprintf("%s(Above number + 1.0) - 1.0 = %s%s\n",
    co, f_rep(digs, (Long_double) (f_epsilon)), oc);

  /* Possible loss of precision warnings here from non-stdc compilers */
  if (F) f_define(D_EPSILON, thing,
      Fname, "_EPSILON", digs, (Long_double) f_epsilon, MARK);
  if (V || F) F_check(digs, (Long_double) f_epsilon);
  Unexpected(21);
  if (F) Verify(digs, f_epsilon, F_EPSILON,
          f_epsilon == Self(F_EPSILON),
          (Long_double) f_epsilon == (Long_double) F_EPSILON);
  Unexpected(22);

  /* Extra chop info *************************************************/
  if (f_rounds == 0) {
    if (Diff(Mul(Sum(ONE,f_epsilon),ONE),ONE) !=  ZERO) {
      Vprintf("%sAlthough arithmetic chops, it uses guard digits%s\n", co, oc);
    }
  }

  /* Size of and minimum normalised exponent ************************/
  y=0; i=0; k=1; z=basein; z1=(1.0+f_epsilon)/base;

  /* Coarse search for the largest power of two */
  if (setjmp(lab)==0) { /* for underflow trap */ /* Yields i, k, y, y1 */
    do {
      y=z; y1=z1;
      z=Mul(y,y); z1=Mul(z1, y);
      a=Mul(z,ONE);
      z2=Div(z1,y);
      if (z2 != y1) break;
      if ((Sum(a,a) == ZERO) || (fabs(z) >= y)) break;
      i++;
      k+=k;
    } while(1);
  } else {
    Vprintf("%s%s underflow generates a trap%s\n", co, Thing, oc);
  }
  Unexpected(23);

  if (f_radix != 10) {
    iexp=i+1; /* for the sign */
    mx=k+k;
  } else {
    iexp=2;
    iz=f_radix;
    while (k >= iz) { iz*=f_radix; iexp++; }
    mx=iz+iz-1;
  }

  /* Fine tune starting with y and y1 */
  if (setjmp(lab)==0) { /* for underflow trap */ /* Yields k, f_min */
    do {
      f_min=y; z1=y1;
      y=Div(y,base); y1=Div(y1,base);
      a=Mul(y,ONE);
      z2=Mul(y1,base);
      if (z2 != z1) break;
      if ((Sum(a,a) == ZERO) || (fabs(y) >= f_min)) break;
      k++;
    } while (1);
  }
  Unexpected(24);

  f_min_exp=(-k)+1;

  if ((mx <= k+k-3) && (f_radix != 10)) { mx+=mx; iexp+=1; }
  Vprintf("%sNumber of bits used for exponent = %d%s\n", co, iexp, oc);
  Vprintf("%sMinimum normalised exponent = %d%s\n", co, f_min_exp-1, oc);
  if (F)
    i_define(D_MIN_EXP, thing, Fname, "_MIN_EXP",
       (long) f_min_exp, (long) maxint, (long) F_MIN_EXP, "");

  if (setjmp(lab)==0) {
    Vprintf("%sMinimum normalised positive number = %s%s\n",
      co, f_rep(digs, (Long_double) f_min), oc);
  } else {
    eek_a_bug("printf can't print the smallest normalised number");
    printf("\n");
  }
  Unexpected(25);
  /* Possible loss of precision warnings here from non-stdc compilers */
  if (setjmp(lab) == 0) {
    if (F) f_define(D_MIN, thing,
        Fname, "_MIN", digs, (Long_double) f_min, MARK);
    if (V || F) F_check(digs, (Long_double) f_min);
  } else {
    eek_a_bug("xxx_MIN caused a trap");
    printf("\n");
  }

  if (setjmp(lab) == 0) {
    if (F) Verify(digs, f_min, F_MIN,
            f_min == Self(F_MIN),
            (Long_double) f_min == (Long_double) F_MIN);
  } else {
    printf("%s*** Verify failed for above #define!\n    %s %s\n\n",
           co, "Compiler has an unusable number for value", oc);
    bugs++;
  }
  Unexpected(26);

  a=1.0; f_min_10_exp=0;
  while (a > f_min*10.0) { a/=10.0; f_min_10_exp--; }
  if (F) i_define(D_MIN_10_EXP, thing, Fname, "_MIN_10_EXP",
      (long) f_min_10_exp, (long) maxint,
      (long) F_MIN_10_EXP, "");

  /* Minimum exponent ************************************************/
  if (setjmp(lab)==0) { /* for underflow trap */ /* Yields xminner */
    do {
      xminner=y;
      y=Div(y,base);
      a=Mul(y,ONE);
      if ((Sum(a,a) == ZERO) || (fabs(y) >= xminner)) break;
    } while (1);
  }
  Unexpected(27);

  if (xminner != 0.0 && xminner != f_min) {
    normal= 0;
    Vprintf("%sThe smallest numbers are not kept normalised%s\n",
      co, oc);
    if (setjmp(lab)==0) {
        Vprintf("%sSmallest unnormalised positive number = %s%s\n",
          co, f_rep(digs, (Long_double) xminner), oc);
        if (V) F_check(digs, (Long_double) xminner);
    } else {
      eek_a_bug("printf can't print the smallest unnormalised number.");
      printf("\n");
    }
    Unexpected(28);
  } else {
    normal= 1;
    Vprintf("%sThe smallest numbers are normalised%s\n", co, oc);
  }

  /* Maximum exponent ************************************************/
  f_max_exp=2; f_max=1.0; newxmax=base+1.0;
  inf=0; trap=0;
  while (f_max<newxmax) {
    f_max=newxmax;
    if (setjmp(lab) == 0) { /* Yields inf, f_max_exp */
      newxmax=Mul(newxmax, base);
    } else {
      trap=1;
      break;
    }
    if (Div(newxmax, base) != f_max) {
      if (newxmax > f_max) inf=1; /* ieee infinity */
      break;
    }
    f_max_exp++;
  }
  Unexpected(29);
  Vprintf("%sMaximum exponent = %d%s\n", co, f_max_exp, oc);
  if (F) i_define(D_MAX_EXP, thing, Fname, "_MAX_EXP",
      (long) f_max_exp, 0L, (long) F_MAX_EXP, "");

  /* Largest number ***************************************************/
  f_max=Diff(ONE, epsneg);
  if (Mul(f_max,ONE) != f_max) f_max=Diff(ONE, Mul(base,epsneg));
  for (i=1; i<=f_max_exp; i++) f_max=Mul(f_max, base);

  if (setjmp(lab)==0) {
    Vprintf("%sMaximum number = %s%s\n",
      co, f_rep(digs, (Long_double) f_max), oc);
  } else {
    eek_a_bug("printf can't print the largest double.");
    printf("\n");
  }
  if (setjmp(lab)==0) {
  /* Possible loss of precision warnings here from non-stdc compilers */
    if (F) f_define(D_MAX, thing,
        Fname, "_MAX", digs, (Long_double) f_max, MARK);
    if (V || F) F_check(digs, (Long_double) f_max);
  } else {
    eek_a_bug("xxx_MAX caused a trap");
    printf("\n");
  }
  if (setjmp(lab)==0) {
    if (F) Verify(digs, f_max, F_MAX,
            f_max == Self(F_MAX),
            (Long_double) f_max == (Long_double) F_MAX);
  } else {
    printf("%s*** Verify failed for above #define!\n    %s %s\n\n",
           co, "Compiler has an unusable number for value", oc);
    bugs++;
  }
  Unexpected(30);

  a=1.0; f_max_10_exp=0;
  while (a < f_max/10.0) { a*=10.0; f_max_10_exp++; }
  if (F) i_define(D_MAX_10_EXP, thing, Fname, "_MAX_10_EXP",
      (long) f_max_10_exp, 0L, (long) F_MAX_10_EXP, "");

  /* Traps and infinities ********************************************/
  if (trap) {
    Vprintf("%sOverflow generates a trap%s\n", co, oc);
  } else {
    Vprintf("%sOverflow doesn't seem to generate a trap%s\n",
      co, oc);
  }

  if (inf) { Vprintf("%sThere is an 'infinite' value%s\n", co, oc); }

#ifdef SIGFPE
  signal(SIGFPE, trap1);
#endif
  if (setjmp(lab) == 0) {
    trapped= 0; /* A global variable */
    b= 0.0;
    a= (1.0/b)/b;
    if (!trapped) {
      Vprintf("%sDivide by zero doesn't generate a trap%s\n",
        co, oc);
    } else {
      Vprintf("%sDivide by zero generates a trap%s\n",
        co, oc);
      Vprintf("%sFP signal handlers return safely%s\n",
        co, oc);
    }
  } else {
    Vprintf("%sDivide by zero generates a trap%s\n", co, oc);
    Vprintf("%sBEWARE! FP signal handlers can NOT return%s\n",
      co, oc);
  }
  setsignals();
  Unexpected(31);

  /* Hidden bit + sanity check ****************************************/
  if (f_radix != 10) {
    hidden=0;
    mantbits=floor_log(2, (Long_double)f_radix)*f_mant_dig;
    if (mantbits+iexp == (int)sizeof(Number)*byte_size) {
      hidden=1;
      Vprintf("%sArithmetic uses a hidden bit%s\n", co, oc);
    } else if (mantbits+iexp+1 == (int)sizeof(Number)*byte_size) {
      Vprintf("%sArithmetic doesn't use a hidden bit%s\n",
        co, oc);
    } else if (mantbits+iexp+1 < (int)sizeof(Number)*byte_size) {
      Vprintf("%sOnly %d of the %d bits of a %s %s%s\n",
        co,
        mantbits+iexp,
        (int)sizeof(Number)*byte_size,
        thing,
        "are actually used",
        oc);
    } else {
      printf("\n%s%s\n    %s (%d) %s (%d) %s %s (%d)!%s\n\n",
             co,
             "*** Something fishy here!",
             "Exponent size",
             iexp,
             "+ significand size",
             mantbits,
             "doesn't match with the size of a",
             thing,
             (int)sizeof(Number)*byte_size,
             oc);
    }
    if (hidden && f_radix == 2 && f_max_exp+f_min_exp==3) {
      Vprintf("%sIt looks like %s length IEEE format%s\n",
        co, f_mant_dig==24 ? "single" :
            f_mant_dig==53 ? "double" :
            f_mant_dig >53 ? "extended" :
            "some", oc);
      if (f_rounds != 1 || normal) {
        Vprintf("%s   though ", co);
        if (f_rounds != 1) {
          Vprintf("the rounding is unusual");
          if (normal) { Vprintf(" and "); }
        }
        if (normal) {
            Vprintf("the normalisation is unusual");
        }
        Vprintf("%s\n", oc);
      }
    } else {
      Vprintf("%sIt doesn't look like IEEE format%s\n",
        co, oc);
    }
  }
  printf("\n"); /* regardless of verbosity */
  return f_mant_dig;
}

Void EPROP(int fprec, int dprec, int lprec)
{
  /* See if expressions are evaluated in extended precision.
     Some compilers optimise even if you don't want it,
     and then this function fails to produce the right result.
     We try to diagnose this if it happens.
  */
  Volatile int eprec;
  Volatile double a, b, base, old;
  Volatile Number d, oldd, dbase, one, zero;
  Volatile int bad=0;

  /* Size of significand **************************************/
  a=1.0;
  if (setjmp(lab) == 0) { /* Yields nothing */
    do { old=a; a=a+a; }
    while ((((a+1.0)-a)-1.0) == 0.0 && a>old);
  } else bad=1;
  if (!bad && a <= old) bad=1;

  if (!bad) {
    b=1.0;
    if (setjmp(lab) == 0) { /* Yields nothing */
      do { old=b; b=b+b; }
      while ((base=((a+b)-a)) == 0.0 && b>old);
      if (b <= old) bad=1;
    } else bad=1;
  }

  if (!bad) {
    eprec=0; d=1.0; dbase=base; one=1.0; zero=0.0;
    if (setjmp(lab) == 0) { /* Yields nothing */
      do { eprec++; oldd=d; d=d*dbase; }
      while ((((d+one)-d)-one) == zero && d>oldd);
      if (d <= oldd) bad=1;
    } else bad=1;
  }

  Unexpected(32);

  if (bad) {
    Vprintf("%sCan't determine precision for %s expressions:\n%s%s\n",
     co, thing, "   check that you compiled without optimisation!",
     oc);
  } else if (eprec==dprec) {
    Vprintf("%s%s expressions are evaluated in double precision%s\n",
      co, Thing, oc);
  } else if (eprec==fprec) {
    Vprintf("%s%s expressions are evaluated in float precision%s\n",
      co, Thing, oc);
  } else if (eprec==lprec) {
    Vprintf("%s%s expressions are evaluated in long double precision%s\n",
      co, Thing, oc);
  } else {
    Vprintf("%s%s expressions are evaluated in a %s %s %d %s%s\n",
      co, Thing, eprec>dprec ? "higher" : "lower",
      "precision than double,\n   using",
      eprec, "base digits",
      oc);
  }
}

#else /* not Number */

#ifdef FPROP /* Then create dummy routines for long double */
/* ARGSUSED */
int FPROP(int byte_size) { return 0; }
#endif
#ifdef EPROP
/* ARGSUSED */
Void EPROP(int fprec, int dprec, int lprec) {}
#endif

#endif /* ifdef Number */

/* Increment the pass number */
#undef PASS

#ifdef PASS2
#undef PASS2
#define PASS 3
#define PASS3 1
#endif

#ifdef PASS1
#undef PASS1
#define PASS 2
#define PASS2 1
#endif

#ifdef PASS0
#undef PASS0
#endif

#ifndef SEP
#ifdef PASS /* then rescan this file */
#ifdef BAD_CPP
#include "enquire.c"
#else
#include FILENAME  /* if this line fails to compile, define BAD_CPP */
#endif
#endif /* PASS */
#endif /* SEP */
