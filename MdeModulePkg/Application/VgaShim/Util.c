#include "Util.h"

VOID
StrToLowercase(
	IN	CHAR16	*String)
{
	CHAR16      *TmpStr;

	for (TmpStr = String; *TmpStr != L'\0'; TmpStr++) {
		if (*TmpStr >= L'A' && *TmpStr <= L'Z') {
			*TmpStr = (CHAR16)(*TmpStr - L'A' + L'a');
		}
	}
}