
#ifndef _GASKET_H_
#define _GASKET_H_

#include <Library/PeCoffLib.h>

#include <Protocol/UgaDraw.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/UnixUgaIo.h>


//
// Gasket functions for EFI_UNIX_THUNK_PROTOCOL
//

void GasketmsSleep (unsigned long Milliseconds);
void Gasketexit (int status);
void GasketSetTimer (UINT64 PeriodMs, VOID (*CallBack)(UINT64 DeltaMs));
void GasketGetLocalTime (EFI_TIME *Time);
struct tm *Gasketgmtime (const time_t *clock);
long GasketGetTimeZone (void);
int GasketGetDayLight (void);
int Gasketpoll (struct pollfd *pfd, int nfds, int timeout);
int Gasketread (int fd, void *buf, int count);
int Gasketwrite (int fd, const void *buf, int count);
char *Gasketgetenv (const char *name);
int Gasketopen (const char *name, int flags, int mode);
off_t Gasketlseek (int fd, off_t off, int whence);
int Gasketftruncate (int fd, long int len);
int Gasketclose (int fd);
int Gasketmkdir (const char *pathname, mode_t mode);
int Gasketrmdir (const char *pathname);
int Gasketunlink (const char *pathname);
int GasketGetErrno (void);
DIR *Gasketopendir (const char *pathname);
void *Gasketrewinddir (DIR *dir);
struct dirent *Gasketreaddir (DIR *dir);
int Gasketclosedir (DIR *dir);
int Gasketstat (const char *path, STAT_FIX *buf);
int Gasketstatfs (const char *path, struct statfs *buf);
int Gasketrename (const char *oldpath, const char *newpath);
time_t Gasketmktime (struct tm *tm);
int Gasketfsync (int fd);
int Gasketchmod (const char *path, mode_t mode);
int Gasketutime (const char *filename, const struct utimbuf *buf);
int Gaskettcflush (int fildes, int queue_selector);
EFI_STATUS GasketUgaCreate(struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo, CONST CHAR16 *Title);
void Gasketperror (__const char *__s);

//
// ... is always an int or pointer to device specific data structure
//
int Gasketioctl (int fd, unsigned long int __request, ...);
int Gasketfcntl (int __fd, int __cmd, ...);

int Gasketcfsetispeed (struct termios *__termios_p, speed_t __speed);
int Gasketcfsetospeed (struct termios *__termios_p, speed_t __speed);
int Gaskettcgetattr (int __fd, struct termios *__termios_p); 
int Gaskettcsetattr (int __fd, int __optional_actions, __const struct termios *__termios_p);
int Gasketsigaction (int sig, const struct sigaction *act, struct sigaction *oact);
int Gasketsetcontext (const ucontext_t *ucp);
int Gasketgetcontext (ucontext_t *ucp);
int Gasketsigemptyset (sigset_t *set);
int Gasketsigaltstack (const stack_t *ss, stack_t *oss);

RETURN_STATUS
GasketUnixPeCoffGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  );

VOID
GasketUnixPeCoffRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

VOID
GasketPeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );
  

int GasketVoid (void *api);
int GasketUintn (void *api, UINTN a);
int GasketUintnUintn (void *api, UINTN a, UINTN b);
int GasketUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c);
int GasketUintnUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c, UINTN d);
int GasketUintn10Args (void *api, UINTN a, UINTN b, UINTN c, UINTN d, UINTN e, UINTN f, UINTN g, UINTN h, UINTN i, UINTN j);
int GasketUint64Uintn (void *api, UINT64 a, UINTN b);
UINT64 GasketUintnUint64Uintn (void *api, UINTN a, UINT64 b, UINTN c);
int GasketUintnUint16 (void *api, UINTN a, UINT16 b);


//
// Gasket functions for EFI_UNIX_UGA_IO_PROTOCOL
//

EFI_STATUS EFIAPI GasketUgaClose (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo);
EFI_STATUS EFIAPI GasketUgaSize (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, UINT32 Width, UINT32 Height);
EFI_STATUS EFIAPI GasketUgaCheckKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo);
EFI_STATUS EFIAPI GasketUgaGetKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, EFI_INPUT_KEY *key);
EFI_STATUS EFIAPI GasketUgaBlt (
   EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
   IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
   IN  EFI_UGA_BLT_OPERATION                   BltOperation,
   IN  UINTN                                   SourceX,
   IN  UINTN                                   SourceY,
   IN  UINTN                                   DestinationX,
   IN  UINTN                                   DestinationY,
   IN  UINTN                                   Width,
   IN  UINTN                                   Height,
   IN  UINTN                                   Delta OPTIONAL
   );

EFI_STATUS UgaCreate (EFI_UNIX_UGA_IO_PROTOCOL **Uga, CONST CHAR16 *Title);


//
// Gasket functions for EFI_UNIX_UGA_IO_PROTOCOL
//
EFI_STATUS UgaClose (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo);
EFI_STATUS UgaSize(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, UINT32 Width, UINT32 Height);
EFI_STATUS UgaCheckKey(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo);
EFI_STATUS UgaGetKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, EFI_INPUT_KEY *key);
EFI_STATUS UgaBlt (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
       IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
       IN  EFI_UGA_BLT_OPERATION                   BltOperation,
       IN  UINTN                                   SourceX,
       IN  UINTN                                   SourceY,
       IN  UINTN                                   DestinationX,
       IN  UINTN                                   DestinationY,
       IN  UINTN                                   Width,
       IN  UINTN                                   Height,
       IN  UINTN                                   Delta OPTIONAL
  );



#endif


