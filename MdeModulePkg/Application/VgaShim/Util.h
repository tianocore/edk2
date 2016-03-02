#ifndef __UTIL_H__
#define __UTIL_H__


/**
  -----------------------------------------------------------------------------
  Constants.
  -----------------------------------------------------------------------------
**/

#define	MAX_DEBUG_MESSAGE_LENGTH	0x100
#define	DEBUG_MESSAGE_DELAY_MS		500


/**
  -----------------------------------------------------------------------------
  Includes.
  -----------------------------------------------------------------------------
**/

#include <Uefi.h>


/**
  -----------------------------------------------------------------------------
  Type definitions and enums.
  -----------------------------------------------------------------------------
**/

typedef	UINTN	DEBUG_LEVEL;


/**
  -----------------------------------------------------------------------------
  Exported method signatures.
  -----------------------------------------------------------------------------
**/

VOID StrToLowercase(
	IN	CHAR16					*String);

VOID PrintMessage(
	IN	DEBUG_LEVEL	MessageLevel,
	IN	CHAR16		*FormatString,
	IN	...	
	);


/**
  -----------------------------------------------------------------------------
  Constants.
  -----------------------------------------------------------------------------
**/

STATIC CONST	DEBUG_LEVEL	DEBUG_VERBOSE	= 0x00400000;
STATIC CONST	DEBUG_LEVEL	DEBUG_INFO		= 0x00000040;
STATIC CONST	DEBUG_LEVEL	DEBUG_WARN		= 0x00000002;
STATIC CONST	DEBUG_LEVEL	DEBUG_ERROR		= 0x80000000;


#endif