#	$NetBSD: Makefile.inc,v 1.17 2014/01/30 19:11:54 matt Exp $

SOFTFLOAT_BITS?=64
.PATH:		${ARCHDIR}/softfloat \
		${.CURDIR}/softfloat/bits${SOFTFLOAT_BITS} ${.CURDIR}/softfloat

CPPFLAGS+=	-I${ARCHDIR}/softfloat -I${.CURDIR}/softfloat
CPPFLAGS+=	-DSOFTFLOAT_FOR_GCC

SRCS.softfloat= softfloat.c

SRCS.softfloat+=fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c \
		fpgetsticky.c fpsetsticky.c

.if !empty(LIBC_MACHINE_ARCH:Mearm*)
SRCS.softfloat+=__aeabi_dcmpeq.c __aeabi_fcmpeq.c
SRCS.softfloat+=__aeabi_dcmpge.c __aeabi_fcmpge.c
SRCS.softfloat+=__aeabi_dcmpgt.c __aeabi_fcmpgt.c
SRCS.softfloat+=__aeabi_dcmple.c __aeabi_fcmple.c
SRCS.softfloat+=__aeabi_dcmplt.c __aeabi_fcmplt.c
SRCS.softfloat+=__aeabi_dcmpun.c __aeabi_fcmpun.c
.else
SRCS.softfloat+=eqsf2.c nesf2.c gtsf2.c gesf2.c ltsf2.c lesf2.c negsf2.c \
		eqdf2.c nedf2.c gtdf2.c gedf2.c ltdf2.c ledf2.c negdf2.c \
		eqtf2.c netf2.c gttf2.c getf2.c lttf2.c letf2.c negtf2.c \
		nexf2.c gtxf2.c gexf2.c negxf2.c \
		unordsf2.c unorddf2.c unordtf2.c
.endif

SRCS+=		${SRCS.softfloat}

# XXX
.if defined(HAVE_GCC) && ${HAVE_GCC} >= 45
.if (${MACHINE_CPU} == "arm")
# See doc/HACKS for more information.
COPTS.softfloat.c+=	-Wno-enum-compare
COPTS.softfloat.c+=	${${ACTIVE_CXX} == "gcc":? -fno-tree-vrp :}
.elif (${MACHINE_CPU} == "mips" || \
     ${MACHINE_CPU} == "sh3")
COPTS.softfloat.c+=	-Wno-enum-compare
.endif
.endif
