/** @file

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _UNISTD_H_
#define _UNISTD_H_

//#include <machine/ansi.h>
//#include <machine/int_types.h>
//#include <sys/featuretest.h>
//#include <sys/types.h>
#include  <sys/unistd.h>
#include  <sys/EfiSysCall.h>

#define F_ULOCK   0
#define F_LOCK    1
#define F_TLOCK   2
#define F_TEST    3


__BEGIN_DECLS
int             dup(int);
int             rename(const char *, const char *);

/* Functions implemented for compatibility. */
int             getopt(int, char * const [], const char *);
extern   char  *optarg;     /* getopt(3) external variables */
extern   int    optind;
pid_t           getpgrp(void);
pid_t           tcgetpgrp(int);
char           *getpass(const char *);
int             usleep(useconds_t);
unsigned int    sleep(unsigned int);
char           *basename(char *path);

// Networking
long            gethostid(void);
int             gethostname(char *, size_t);
int             getdomainname(char *, size_t);
int             setdomainname(const char *, size_t);
int             sethostid(long);
int             sethostname(const char *, size_t);

/*  Stub functions implemented for porting ease.
    These functions always fail or return NULL.
*/
__aconst char  *getlogin(void);
pid_t           fork(void);
uid_t           getuid(void);

// For Future implementation
ssize_t         pread(int, void *, size_t, off_t);
ssize_t         pwrite(int, const void *, size_t, off_t);
int             syscall(int, ...);

#if 0   // The following functions are not implemented
__dead   void   _exit(int) __attribute__((__noreturn__));
unsigned int    alarm(unsigned int);
int             chown(const char *, uid_t, gid_t);
size_t          confstr(int, char *, size_t);
int             execl(const char *, const char *, ...);
int             execle(const char *, const char *, ...);
int             execlp(const char *, const char *, ...);
int             execv(const char *, char * const *);
int             execve(const char *, char * const *, char * const *);
int             execvp(const char *, char * const *);
long            fpathconf(int, int);
gid_t           getegid(void);
uid_t           geteuid(void);
gid_t           getgid(void);
int             getgroups(int, gid_t []);
pid_t           getpid(void);
pid_t           getppid(void);
int             link(const char *, const char *);
long            pathconf(const char *, int);
int             pause(void);
int             pipe(int *);
int             setgid(gid_t);
int             setpgid(pid_t, pid_t);
pid_t           setsid(void);
int             setuid(uid_t);
long            sysconf(int);

int             tcsetpgrp(int, pid_t);
__aconst char  *ttyname(int);

extern   int    opterr;
extern   int    optopt;
extern   int    optreset;
extern   char  *suboptarg;

int             setegid(gid_t);
int             seteuid(uid_t);
int             fdatasync(int);
int             fsync(int);
int             ttyname_r(int, char *, size_t);
int             chroot(const char *);
int             nice(int);
__aconst char *crypt(const char *, const char *);
int             encrypt(char *, int);
pid_t           getsid(pid_t);

#ifndef intptr_t
typedef __intptr_t  intptr_t;
#define intptr_t  __intptr_t
#endif

int             brk(void *);
int             fchdir(int);
int             fchown(int, uid_t, gid_t);
int             getdtablesize(void);
__pure int      getpagesize(void);    /* legacy */
pid_t           getpgid(pid_t);
int             lchown(const char *, uid_t, gid_t);
int             lockf(int, int, off_t);
ssize_t         readlink(const char * __restrict, char * __restrict, size_t);
void           *sbrk(intptr_t);
int             setregid(gid_t, gid_t);
int             setreuid(uid_t, uid_t);
void            swab(const void *, void *, size_t);
int             symlink(const char *, const char *);
void            sync(void);
useconds_t      ualarm(useconds_t, useconds_t);
pid_t           vfork(void) __RENAME(__vfork14);

/*
 * Implementation-defined extensions
 */
int             acct(const char *);
int             closefrom(int);
int             des_cipher(const char *, char *, long, int);
int             des_setkey(const char *);
void            endusershell(void);
int             exect(const char *, char * const *, char * const *);
int             fchroot(int);
int             fsync_range(int, int, off_t, off_t);
int             getgrouplist(const char *, gid_t, gid_t *, int *);
int             getgroupmembership(const char *, gid_t, gid_t *, int, int *);
mode_t          getmode(const void *, mode_t);
int             getsubopt(char **, char * const *, char **);
__aconst char  *getusershell(void);
int             initgroups(const char *, gid_t);
int             iruserok(uint32_t, int, const char *, const char *);
int             issetugid(void);
int             nfssvc(int, void *);
int             profil(char *, size_t, u_long, u_int);
void            psignal(unsigned int, const char *);
int             rcmd(char **, int, const char *, const char *, const char *, int *);
int             revoke(const char *);
int             rresvport(int *);
int             ruserok(const char *, int, const char *, const char *);
int             setgroups(int, const gid_t *);
int             setlogin(const char *);
void           *setmode(const char *);
int             setrgid(gid_t);
int             setruid(uid_t);
void            setusershell(void);
void            strmode(mode_t, char *);
__aconst char  *strsignal(int);
int             swapctl(int, void *, int);
quad_t          __syscall(quad_t, ...);
int             undelete(const char *);
int             rcmd_af(char **, int, const char *, const char *, const char *, int *, int);
int             rresvport_af(int *, int);
int             iruserok_sa(const void *, int, int, const char *, const char *);
#endif  /* Unimplemented functions. */

__END_DECLS

#endif /* !_UNISTD_H_ */
