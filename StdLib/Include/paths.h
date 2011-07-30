/*  $NetBSD: paths.h,v 1.30 2004/12/11 06:01:33 christos Exp $  */

/*
 * Copyright (c) 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)paths.h 8.1 (Berkeley) 6/2/93
 */

#ifndef _PATHS_H_
#define _PATHS_H_

///*
// * Default user search path.
// * set by login(1), rshd(8), rexecd(8)
// * used by execvp(3) absent PATH from the environ(7)
// */
//#ifdef RESCUEDIR
//#define _PATH_DEFPATH RESCUEDIR ":/usr/bin:/bin:/usr/pkg/bin:/usr/local/bin"
//#else
//#define _PATH_DEFPATH "/usr/bin:/bin:/usr/pkg/bin:/usr/local/bin"
//#endif

/*
 * Provide trailing slash, since mostly used for building pathnames.
 * see the __CONCAT() macro from <sys/EfiCdefs.h> for cpp examples.
 */
#define _PATH_DEV "/dev/"
#define _PATH_ETC "/Efi/etc/"
#define _PATH_TMP "/Efi/Temp/"
//#define _PATH_DEV_PTS "/dev/pts/"
//#define _PATH_EMUL_AOUT "/emul/aout/"
//#define _PATH_VARDB "/var/db/"
//#define _PATH_VARRUN  "/var/run/"
//#define _PATH_VARTMP  "/var/tmp/"

///*
// * All standard utilities path.
// * set by init(8) for system programs & scripts (e.g. /etc/rc)
// * used by ttyaction(3), whereis(1)
// */
//#define _PATH_STDPATH   "/usr/bin:/bin:/usr/sbin:/sbin:/usr/pkg/bin:/usr/pkg/sbin:/usr/local/bin:/usr/local/sbin"

//#define _PATH_AUDIO "/dev/audio"
//#define _PATH_AUDIO0  "/dev/audio0"
//#define _PATH_AUDIOCTL  "/dev/audioctl"
//#define _PATH_AUDIOCTL0 "/dev/audioctl0"
//#define _PATH_BPF "/dev/bpf"
//#define _PATH_CLOCKCTL  "/dev/clockctl"
//#define _PATH_CSMAPPER  "/usr/share/i18n/csmapper"
//#define _PATH_DEFTAPE "/dev/nrst0"
//#define _PATH_DEVDB "/var/run/dev.db"
//#define _PATH_DRUM  "/dev/drum"
//#define _PATH_ESDB  "/usr/share/i18n/esdb"
//#define _PATH_FTPUSERS  "/etc/ftpusers"
//#define _PATH_I18NMODULE "/usr/lib/i18n"
//#define _PATH_ICONV "/usr/share/i18n/iconv"
//#define _PATH_KMEM  "/dev/kmem"
//#define _PATH_KSYMS "/dev/ksyms"
//#define _PATH_KVMDB "/var/db/kvm.db"
//#define _PATH_MAILDIR "/var/mail"
//#define _PATH_MAN "/usr/share/man"
//#define _PATH_MEM "/dev/mem"
//#define _PATH_MIXER "/dev/mixer"
//#define _PATH_MIXER0  "/dev/mixer0"
//#define _PATH_NOLOGIN "/etc/nologin"
//#define _PATH_RANDOM  "/dev/random"
//#define _PATH_SENDMAIL  "/usr/sbin/sendmail"
//#define _PATH_SHELLS  "/etc/shells"
//#define _PATH_SKEYKEYS  "/etc/skeykeys"
//#define _PATH_SOUND "/dev/sound"
//#define _PATH_SOUND0  "/dev/sound0"
//#define _PATH_SYSMON  "/dev/sysmon"
//#define _PATH_UNIX  "/netbsd"
//#define _PATH_URANDOM "/dev/urandom"
//#define _PATH_VI  "/usr/bin/vi"

// DOS style device paths
#define _PATH_TTYDEV "tty:"
#define _PATH_NULLDEV "null:"
#define _PATH_CONSOLE "console:"
#define _PATH_CONSTTY "constty:"
#define _PATH_STDIN   "stdin:"
#define _PATH_STDOUT  "stdout:"
#define _PATH_STDERR  "stderr:"
#define _PATH_SOCKET  "socket:"

// *nix style device paths
#define _PATH_DEVTTY      _PATH_DEV "tty"
#define _PATH_DEVNULL     _PATH_DEV "null"
#define _PATH_DEVCONSOLE  _PATH_DEV "console"
#define _PATH_DEVCONSTTY  _PATH_DEV "constty"
#define _PATH_DEVSTDIN    _PATH_DEV "stdin"
#define _PATH_DEVSTDOUT   _PATH_DEV "stdout"
#define _PATH_DEVSTDERR   _PATH_DEV "stderr"
#define _PATH_DEVSOCKET   _PATH_DEV "socket"

// Special files and locations
#define _PATH_FSTAB       _PATH_ETC "fstab"
////#define _PATH_HEQUIV      _PATH_ETC "hosts.equiv"
#define _PATH_HOSTNAME    _PATH_ETC "hostname"
#define _PATH_HOSTS       _PATH_ETC "hosts"
#define _PATH_HOSTCONF    _PATH_ETC "host.conf"
#define _PATH_LOCALE      _PATH_ETC "Locale"
#define _PATH_NETCONF     _PATH_ETC "host.conf"
#define _PATH_NETWORKS    _PATH_ETC "networks"
#define _PATH_PROTOCOLS   _PATH_ETC "protocols"

/*
 * Resolver configuration file.
 * Normally not present, but may contain the address of the
 * inital name server(s) to query and the domain search list.
 */
#define _PATH_RESCONF     _PATH_ETC "resolv.conf"
#define _PATH_SERVICES    _PATH_ETC "services"
////#define _PATH_SERVICES_DB "/Efi/var/db/services.db"

//#define _PATH_BSHELL  RESCUEDIR "/sh"
//#define _PATH_CSHELL  RESCUEDIR "/csh"

#endif /* !_PATHS_H_ */

