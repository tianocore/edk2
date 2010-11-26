/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  SetStamp.c

Abstract:
  Set Date/Time Stamp of Portable Executable (PE) format file

--*/

#include <stdio.h>
#include <string.h>
#include <time.h>

#define LINE_MAXLEN 80
#define UTILITY_NAME    "SetStamp"
#define UTILITY_VERSION "v1.0"
void
PrintUsage (
  void
  )
/*++
Routine Description:
  print usage of setstamp command

Arguments:
  void

Returns:
  None
--*/
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Set Time Stamp Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" PEFILE TIMEFILE",
    "Description:",
    "  Set Date/Time Stamp of Portable Executable (PE) format file",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
  
}

int
GetDateTime (
  FILE      *fp,
  time_t    *ltime
  )
/*++
Routine Description:
  Read the date and time from TIME file. If the date/time string is
"NOW NOW", write the current date and time to TIME file and set it to
ltime. Else, set the date and time of TIME file to ltime.

Arguments:
  fp              - The pointer of TIME file
  ltime           - Date and time

Returns:
  =  0            - Success
  = -1            - Failed
--*/
{
  char      buffer[LINE_MAXLEN];
  struct tm stime;
  struct tm *now;

  if (fgets (buffer, LINE_MAXLEN, fp) == NULL) {
    printf ("Error: Cannot read TIME file.\n");
    return -1;
  }
  //
  // compare the value with "NOW NOW", write TIME file if equal
  //
  if (strncmp (buffer, "NOW NOW", 7) == 0) {
    //
    // get system current time and date
    //
    time (ltime);

    now = localtime (ltime);
    if (now == NULL) {
      printf ("Error: Cannot get local time.\n");
      return -1;
    }

    if (strftime (buffer, LINE_MAXLEN, "%Y-%m-%d %H:%M:%S", now) == 0) {
      printf ("Error: Cannot format time string.\n");
      return -1;
    }
    //
    // write TIME file
    //
    if (fseek (fp, 0, SEEK_SET) != 0) {
      printf ("Error: Cannot move location of TIME file.\n");
      return -1;
    }

    if (fputs (buffer, fp) == EOF) {
      printf ("Error: Cannot write time string to TIME file.\n");
      return -1;
    }
    //
    // ltime has been set as current time and date, return
    //
    return 0;
  }
  //
  // get the date and time from buffer
  //
  if (6 != sscanf (
            buffer,
            "%d-%d-%d %d:%d:%d",
            &stime.tm_year,
            &stime.tm_mon,
            &stime.tm_mday,
            &stime.tm_hour,
            &stime.tm_min,
            &stime.tm_sec
            )) {
    printf ("Error: Invaild date or time!\n");
    return -1;
  }
  //
  // in struct, Month (0 - 11; Jan = 0). So decrease 1 from it
  //
  stime.tm_mon -= 1;

  //
  // in struct, Year (current year minus 1900)
  // and only the dates can be handled from Jan 1, 1970 to Jan 18, 2038
  //
  //
  // convert 0 -> 100 (2000), 1 -> 101 (2001), ..., 38 -> 138 (2038)
  //
  if (stime.tm_year <= 38) {
    stime.tm_year += 100;
  }
  //
  // convert 1970 -> 70, 2000 -> 100, ...
  //
  else if (stime.tm_year >= 1970) {
    stime.tm_year -= 1900;
  }
  //
  // convert the date and time to time_t format
  //
  *ltime = mktime (&stime);
  if (*ltime == (time_t) - 1) {
    printf ("Error: Invalid date or time!\n");
    return -1;
  }

  return 0;
}

int
ReadFromFile (
  FILE      *fp,
  long      offset,
  void      *buffer,
  int       size
  )
/*++
Routine Description:
  read data from a specified location of file

Arguments:
  fp              - file pointer
  offset          - number of bytes from beginning of file
  buffer          - buffer used to store data
  size            - size of buffer

Returns:
  =  0            - Success
  = -1            - Failed
--*/
{
  //
  // set file pointer to the specified location of file
  //
  if (fseek (fp, offset, SEEK_SET) != 0) {
    printf ("Error: Cannot move the current location of the file.\n");
    return -1;
  }
  //
  // read data from the file
  //
  if (fread (buffer, size, 1, fp) != 1) {
    printf ("Error: Cannot read data from the file.\n");
    return -1;
  }

  return 0;
}

int
WriteToFile (
  FILE      *fp,
  long      offset,
  void      *buffer,
  int       size
  )
/*++
Routine Description:
  write data to a specified location of file

Arguments:
  fp              - file pointer
  offset          - number of bytes from beginning of file
  buffer          - buffer used to store data
  size            - size of buffer

Returns:
  =  0            - Success
  = -1            - Failed
--*/
{
  //
  // set file pointer to the specified location of file
  //
  if (fseek (fp, offset, SEEK_SET) != 0) {
    printf ("Error: Cannot move the current location of the file.\n");
    return -1;
  }
  //
  // write data to the file
  //
  if (fwrite (buffer, size, 1, fp) != 1) {
    perror ("Error: Cannot write data to the file.\n");
    return -1;
  }

  return 0;
}

int
SetStamp (
  FILE      *fp,
  time_t    ltime
  )
/*++
Routine Description:
  set Date/Time Stamp of the file

Arguments:
  fp              - file pointer
  ltime           - time and date

Returns:
  =  0            - Success
  = -1            - Failed
--*/
{
  unsigned char header[4];
  unsigned long offset;
  unsigned long NumberOfRvaAndSizes;
  unsigned int  nvalue;
  unsigned long lvalue;

  //
  // read the header of file
  //
  if (ReadFromFile (fp, 0, header, 2) != 0) {
    return -1;
  }
  //
  // "MZ" -- the header of image file (PE)
  //
  if (strncmp ((char *) header, "MZ", 2) != 0) {
    printf ("Error: Invalid Image file.\n");
    return -1;
  }
  //
  // At location 0x3C, the stub has the file offset to the
  // PE signature.
  //
  if (ReadFromFile (fp, 0x3C, &offset, 4) != 0) {
    return -1;
  }
  //
  // read the header of optional
  //
  if (ReadFromFile (fp, offset, header, 4) != 0) {
    return -1;
  }
  //
  // "PE\0\0" -- the signature of optional header
  //
  if (strncmp ((char *) header, "PE\0\0", 4) != 0) {
    printf ("Error: Invalid PE format file.\n");
    return -1;
  }
  //
  // Add 8 to skip PE signature (4-byte), Machine (2-byte) and
  // NumberOfSection (2-byte)
  //
  offset += 8;

  if (WriteToFile (fp, offset, &ltime, 4) != 0) {
    return -1;
  }
  //
  // Add 16 to skip COFF file header, and get to optional header.
  //
  offset += 16;

  //
  // Check the magic field, 0x10B for PE32 and 0x20B for PE32+
  //
  if (ReadFromFile (fp, offset, &nvalue, 2) != 0) {
    return -1;
  }
  //
  // If this is PE32 image file, offset of NumberOfRvaAndSizes is 92.
  // Else it is 108.
  //
  switch (nvalue & 0xFFFF) {
  case 0x10B:
    offset += 92;
    break;

  case 0x20B:
    offset += 108;
    break;

  default:
    printf ("Error: Sorry! The Magic value is unknown.\n");
    return -1;
  }
  //
  // get the value of NumberOfRvaAndSizes
  //
  if (ReadFromFile (fp, offset, &NumberOfRvaAndSizes, 4) != 0) {
    return -1;
  }
  //
  // Date/time stamp exists in Export Table, Import Table, Resource Table,
  // Debug Table and Delay Import Table. And in Import Table and Delay Import
  // Table, it will be set when bound. So here only set the date/time stamp
  // of Export Table, Resource Table and Debug Table.
  //
  //
  // change date/time stamp of Export Table, the offset of Export Table
  // is 4 + 0 * 8 = 4. And the offset of stamp is 4.
  //
  if (NumberOfRvaAndSizes >= 1) {
    if (ReadFromFile (fp, offset + 4, &lvalue, 4) != 0) {
      return -1;
    }

    if (lvalue != 0) {
      if (WriteToFile (fp, lvalue + 4, &ltime, 4) != 0) {
        return -1;
      }
    }
  }
  //
  // change date/time stamp of Resource Table, the offset of Resource Table
  // is 4 + 2 * 8 = 20. And the offset of stamp is 4.
  //
  if (NumberOfRvaAndSizes >= 3) {
    if (ReadFromFile (fp, offset + 20, &lvalue, 4) != 0) {
      return -1;
    }

    if (lvalue != 0) {
      if (WriteToFile (fp, lvalue + 4, &ltime, 4) != 0) {
        return -1;
      }
    }
  }
  //
  // change date/time stamp of Debug Table, offset of Debug Table
  // is 4 + 6 * 8 = 52. And the offset of stamp is 4.
  //
  if (NumberOfRvaAndSizes >= 7) {
    if (ReadFromFile (fp, offset + 52, &lvalue, 4) != 0) {
      return -1;
    }

    if (lvalue != 0) {
      if (WriteToFile (fp, lvalue + 4, &ltime, 4) != 0) {
        return -1;
      }
    }
    //
    // change the date/time stamp of Debug Data
    //
    if (ReadFromFile (fp, lvalue + 24, &lvalue, 4) != 0) {
      return -1;
    }
    //
    // get the signature of debug data
    //
    if (ReadFromFile (fp, lvalue, header, 2) != 0) {
      return -1;
    }
    //
    // "NB" - the signature of Debug Data
    // Need Review: (From Spec. is "NB05", From .dll is "NB10")
    //
    if (strncmp ((char *) header, "NB", 2) == 0) {
      if (WriteToFile (fp, lvalue + 8, &ltime, 4) != 0) {
        return -1;
      }
    }
  }

  return 0;
}

int
main (
  int       argc,
  char      *argv[]
  )
{
  FILE    *fp;
  time_t  ltime;

  //
  // check the number of parameters
  //
  if (argc != 3) {
    PrintUsage ();
    return -1;
  }
  //
  // open the TIME file, if not exists, return
  //
  fp = fopen (argv[2], "r+");
  if (fp == NULL) {
    return 0;
  }
  //
  // get time and date from file
  //
  if (GetDateTime (fp, &ltime) != 0) {
    fclose (fp);
    return -1;
  }
  //
  // close the TIME file
  //
  fclose (fp);

  //
  // open the PE file
  //
  fp = fopen (argv[1], "r+b");
  if (fp == NULL) {
    printf ("Error: Cannot open the PE file!\n");
    return -1;
  }
  //
  // set time and date stamp to the PE file
  //
  if (SetStamp (fp, ltime) != 0) {
    fclose (fp);
    return -1;
  }

  printf ("Set Date/Time Stamp to %s", ctime (&ltime));

  //
  // close the PE file
  //
  fclose (fp);

  return 0;
}
