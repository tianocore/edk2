/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DepexParser.c

Abstract:

  Validate Dependency Expression syntax
  recursive descent Algorithm

  The original BNF grammar(taken from "Pre EFI Initialization Core Interface Specification 
  draft review 0.9") is thus:
       <depex>    ::= BEFORE <guid> END
                    | AFTER <guid> END
                    | SOR <bool> END
                    | <bool> END    
       <bool>     ::= <bool> AND <term> 
                    | <bool> OR <term>  
                    | <term>            
       <term>     ::= NOT <factor>      
                    | <factor>          
       <factor>   ::= <bool>            
                    | TRUE 
                    | FALSE 
                    | GUID

       <guid>     ::= '{' <hex32> ',' <hex16> ',' <hex16> ','
                      <hex8> ',' <hex8> ',' <hex8> ',' <hex8> ',' 
                      <hex8> ',' <hex8> ',' <hex8> ',' <hex8> '}'
       <hex32>    ::= <hexprefix> <hexvalue>
       <hex16>    ::= <hexprefix> <hexvalue>
       <hex8>     ::= <hexprefix> <hexvalue>
       <hexprefix>::= '0' 'x'
                    | '0' 'X'
       <hexvalue> ::= <hexdigit> <hexvalue>
                    | <hexdigit>
       <hexdigit> ::= [0-9]
                    | [a-f]
                    | [A-F]

  After cleaning left recursive and parentheses supported, the BNF grammar used in this module is thus:
       <depex>    ::= BEFORE <guid>
                    | AFTER <guid>
                    | SOR <bool>
                    | <bool>
       <bool>     ::= <term><rightbool>
       <rightbool>::= AND <term><rightbool>
                    | OR <term><rightbool>
                    | ''
       <term>     ::= NOT <factor>
                    | <factor>
       <factor>   ::= '('<bool>')'<rightfactor>
                    | NOT <factor> <rightbool> <rightfactor>
                    | TRUE <rightfactor>
                    | FALSE <rightfactor>
                    | END <rightfactor>
                    | <guid> <rightfactor>                    
       <rightfactor> ::=AND <term><rightbool> <rightfactor>   
                    | OR <term><rightbool> <rightfactor>                 
                    | ''
       <guid>     ::= '{' <hex32> ',' <hex16> ',' <hex16> ','
                      <hex8> ',' <hex8> ',' <hex8> ',' <hex8> ',' 
                      <hex8> ',' <hex8> ',' <hex8> ',' <hex8> '}'
       <hex32>    ::= <hexprefix> <hexvalue>
       <hex16>    ::= <hexprefix> <hexvalue>
       <hex8>     ::= <hexprefix> <hexvalue>
       <hexprefix>::= '0' 'x'
                    | '0' 'X'
       <hexvalue> ::= <hexdigit> <hexvalue>
                    | <hexdigit>
       <hexdigit> ::= [0-9]
                    | [a-f]
                    | [A-F]
 
  Note: 1. There's no precedence in operators except parentheses;
        2. For hex32, less and equal than 8 bits is valid, more than 8 bits is invalid.
           Same constraint for hex16 is 4, hex8 is 2. All hex should contains at least 1 bit.
        3. "<factor>   ::= '('<bool>')'<rightfactor>" is added to support parentheses;
        4. "<factor>   ::= GUID" is changed to "<factor>   ::= <guid>";
        5. "DEPENDENCY_END" is the terminal of the expression. But it has been filtered by caller. 
           During parsing, "DEPENDENCY_END" will be treated as illegal factor;
    
  This code should build in any environment that supports a standard C-library w/ string
  operations and File I/O services.

  As an example of usage, consider the following:

  The input string could be something like: 
    
      NOT ({ 0xce345171, 0xba0b, 0x11d2, 0x8e, 0x4f, 0x0, 0xa0, 0xc9, 0x69, 0x72,
        0x3b } AND { 0x964e5b22, 0x6459, 0x11d2, 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69,
        0x72, 0x3b }) OR { 0x03c4e603, 0xac28, 0x11d3, 0x9a, 0x2d, 0x00, 0x90, 0x27,
        0x3f, 0xc1, 0x4d } AND

  It's invalid for an extra "AND" in the end.

  Complies with Tiano C Coding Standards Document, version 0.33, 16 Aug 2001.

--*/

#include "DepexParser.h"

BOOLEAN
ParseBool (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  );

BOOLEAN
ParseTerm (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  );

BOOLEAN
ParseRightBool (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  );

BOOLEAN
ParseFactor (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  );

VOID
LeftTrim (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Left trim the space, '\n' and '\r' character in string.
  The space at the end does not need trim.


Arguments:

  Pbegin    The pointer to the string  
  length    length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  None


--*/
{
  while
  (
    ((*Pindex) < (Pbegin + length)) &&
    ((strncmp (*Pindex, " ", 1) == 0) || (strncmp (*Pindex, "\n", 1) == 0) || (strncmp (*Pindex, "\r", 1) == 0))
  ) {
    (*Pindex)++;
  }
}

BOOLEAN
ParseHexdigit (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse Hex bit in dependency expression.  

Arguments:

  Pbegin    The pointer to the string  
  length    Length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If parses a valid hex bit, return TRUE, otherwise FALSE


--*/
{
  //
  // <hexdigit> ::= [0-9] | [a-f] | [A-F]
  //
  if (((**Pindex) >= '0' && (**Pindex) <= '9') ||
      ((**Pindex) >= 'a' && (**Pindex) <= 'f') ||
      ((**Pindex) >= 'A' && (**Pindex) <= 'F')
      ) {
    (*Pindex)++;
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
ParseHex32 (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse Hex32 in dependency expression.  

Arguments:

  Pbegin    The pointer to the string  
  length    Length of the string
  Pindex    The pointer of point to the next parse character in the string

Returns:

  BOOLEAN   If parses a valid hex32, return TRUE, otherwise FALSE


--*/
{
  INT32 Index;
  INT8  *Pin;

  Index = 0;
  Pin   = *Pindex;
  LeftTrim (Pbegin, length, Pindex);

  if ((strncmp (*Pindex, "0x", 2) != 0) && (strncmp (*Pindex, "0X", 2) != 0)) {
    return FALSE;
  }
  (*Pindex) += 2;

  while (ParseHexdigit (Pbegin, length, Pindex)) {
    Index++;
  }

  if (Index > 0 && Index <= 8) {
    return TRUE;
  } else {
    *Pindex = Pin;
    return FALSE;
  }
}

BOOLEAN
ParseHex16 (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse Hex16 in dependency expression.  

Arguments:

  Pbegin    The pointer to the string  
  length    Length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If parses a valid hex16, return TRUE, otherwise FALSE


--*/
{
  int   Index;
  INT8  *Pin;

  Index = 0;
  Pin   = *Pindex;
  LeftTrim (Pbegin, length, Pindex);

  if ((strncmp (*Pindex, "0x", 2) != 0) && (strncmp (*Pindex, "0X", 2) != 0)) {
    return FALSE;
  }
  (*Pindex) += 2;

  while (ParseHexdigit (Pbegin, length, Pindex)) {
    Index++;
  }

  if (Index > 0 && Index <= 4) {
    return TRUE;
  } else {
    *Pindex = Pin;
    return FALSE;
  }
}

BOOLEAN
ParseHex8 (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse Hex8 in dependency expression.  

Arguments:

  Pbegin    The pointer to the string  
  length    Length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If parses a valid hex8, return TRUE, otherwise FALSE


--*/
{
  int   Index;
  INT8  *Pin;

  Index = 0;
  Pin   = *Pindex;
  LeftTrim (Pbegin, length, Pindex);

  if ((strncmp (*Pindex, "0x", 2) != 0) && (strncmp (*Pindex, "0X", 2) != 0)) {
    return FALSE;
  }
  (*Pindex) += 2;

  while (ParseHexdigit (Pbegin, length, Pindex)) {
    Index++;
  }

  if (Index > 0 && Index <= 2) {
    return TRUE;
  } else {
    *Pindex = Pin;
    return FALSE;
  }
}

BOOLEAN
ParseGuid (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse guid in dependency expression.
  There can be any number of spaces between '{' and hexword, ',' and hexword, 
  hexword and ',', hexword and '}'. The hexword include hex32, hex16 and hex8.

Arguments:

  Pbegin    The pointer to the string  
  length    length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If parses a valid guid, return TRUE, otherwise FALSE


--*/
{
  INT32 Index;
  INT8  *Pin;
  Pin = *Pindex;
  LeftTrim (Pbegin, length, Pindex);
  if (strncmp (*Pindex, "{", 1) != 0) {
    return FALSE;
  }
  (*Pindex)++;

  LeftTrim (Pbegin, length, Pindex);
  if (!ParseHex32 (Pbegin, length, Pindex)) {
    *Pindex = Pin;
    return FALSE;
  }

  LeftTrim (Pbegin, length, Pindex);
  if (strncmp (*Pindex, ",", 1) != 0) {
    return FALSE;
  } else {
    (*Pindex)++;
  }

  for (Index = 0; Index < 2; Index++) {
    LeftTrim (Pbegin, length, Pindex);
    if (!ParseHex16 (Pbegin, length, Pindex)) {
      *Pindex = Pin;
      return FALSE;
    }

    LeftTrim (Pbegin, length, Pindex);
    if (strncmp (*Pindex, ",", 1) != 0) {
      return FALSE;
    } else {
      (*Pindex)++;
    }
  }

  for (Index = 0; Index < 7; Index++) {
    LeftTrim (Pbegin, length, Pindex);
    if (!ParseHex8 (Pbegin, length, Pindex)) {
      *Pindex = Pin;
      return FALSE;
    }

    LeftTrim (Pbegin, length, Pindex);
    if (strncmp (*Pindex, ",", 1) != 0) {
      return FALSE;
    } else {
      (*Pindex)++;
    }
  }

  LeftTrim (Pbegin, length, Pindex);
  if (!ParseHex8 (Pbegin, length, Pindex)) {
    *Pindex = Pin;
    return FALSE;
  }

  LeftTrim (Pbegin, length, Pindex);
  if (strncmp (*Pindex, "}", 1) != 0) {
    return FALSE;
  } else {
    (*Pindex)++;
  }

  return TRUE;
}

BOOLEAN
ParseRightFactor (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse rightfactor in bool expression.

Arguments:

  Pbegin    The pointer to the string  
  length    length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If string is a valid rightfactor expression, return TRUE, otherwise FALSE


--*/
{
  INT8  *Pin;

  Pin = *Pindex;
  LeftTrim (Pbegin, length, Pindex);

  //
  // <rightfactor> ::=AND <term> <rightbool> <rightfactor>
  //
  if (strncmp (*Pindex, OPERATOR_AND, strlen (OPERATOR_AND)) == 0) {
    *Pindex += strlen (OPERATOR_AND);
    LeftTrim (Pbegin, length, Pindex);

    if (ParseTerm (Pbegin, length, Pindex)) {
      LeftTrim (Pbegin, length, Pindex);

      if (ParseRightBool (Pbegin, length, Pindex)) {
        LeftTrim (Pbegin, length, Pindex);
        if (ParseRightFactor (Pbegin, length, Pindex)) {
          return TRUE;
        } else {
          *Pindex = Pin;
        }
      } else {
        *Pindex = Pin;
      }
    } else {
      *Pindex = Pin;
    }
  }
  //
  // <rightfactor> ::=OR <term> <rightbool> <rightfactor>
  //
  if (strncmp (*Pindex, OPERATOR_OR, strlen (OPERATOR_OR)) == 0) {
    *Pindex += strlen (OPERATOR_OR);
    LeftTrim (Pbegin, length, Pindex);

    if (ParseTerm (Pbegin, length, Pindex)) {
      LeftTrim (Pbegin, length, Pindex);

      if (ParseRightBool (Pbegin, length, Pindex)) {
        LeftTrim (Pbegin, length, Pindex);
        if (ParseRightFactor (Pbegin, length, Pindex)) {
          return TRUE;
        } else {
          *Pindex = Pin;
        }
      } else {
        *Pindex = Pin;
      }
    } else {
      *Pindex = Pin;
    }
  }
  //
  // <rightfactor> ::= ''
  //
  *Pindex = Pin;
  return TRUE;
}

BOOLEAN
ParseRightBool (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse rightbool in bool expression.

Arguments:

  Pbegin    The pointer to the string  
  length    length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If string is a valid rightbool expression, return TRUE, otherwise FALSE


--*/
{
  INT8  *Pin;

  Pin = *Pindex;
  LeftTrim (Pbegin, length, Pindex);

  //
  // <rightbool>::= AND <term><rightbool>
  //
  if (strncmp (*Pindex, OPERATOR_AND, strlen (OPERATOR_AND)) == 0) {
    *Pindex += strlen (OPERATOR_AND);
    LeftTrim (Pbegin, length, Pindex);

    if (ParseTerm (Pbegin, length, Pindex)) {
      LeftTrim (Pbegin, length, Pindex);

      if (ParseRightBool (Pbegin, length, Pindex)) {
        return TRUE;
      } else {
        *Pindex = Pin;
      }
    } else {
      *Pindex = Pin;
    }
  }
  //
  // <rightbool>::=  OR <term><rightbool>
  //
  if (strncmp (*Pindex, OPERATOR_OR, strlen (OPERATOR_OR)) == 0) {
    *Pindex += strlen (OPERATOR_OR);
    LeftTrim (Pbegin, length, Pindex);

    if (ParseTerm (Pbegin, length, Pindex)) {
      LeftTrim (Pbegin, length, Pindex);

      if (ParseRightBool (Pbegin, length, Pindex)) {
        return TRUE;
      } else {
        *Pindex = Pin;
      }
    } else {
      *Pindex = Pin;
    }
  }
  //
  // <rightbool>::= ''
  //
  *Pindex = Pin;
  return TRUE;
}

BOOLEAN
ParseFactor (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse factor in bool expression.

Arguments:

  Pbegin    The pointer to the string  
  length    length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If string is a valid factor, return TRUE, otherwise FALSE


--*/
{
  INT8  *Pin;

  Pin = *Pindex;
  LeftTrim (Pbegin, length, Pindex);

  //
  // <factor>   ::= '('<bool>')'<rightfactor>
  //
  if (strncmp (*Pindex, OPERATOR_LEFT_PARENTHESIS, strlen (OPERATOR_LEFT_PARENTHESIS)) == 0) {
    *Pindex += strlen (OPERATOR_LEFT_PARENTHESIS);
    LeftTrim (Pbegin, length, Pindex);

    if (!ParseBool (Pbegin, length, Pindex)) {
      *Pindex = Pin;
    } else {
      LeftTrim (Pbegin, length, Pindex);

      if (strncmp (*Pindex, OPERATOR_RIGHT_PARENTHESIS, strlen (OPERATOR_RIGHT_PARENTHESIS)) == 0) {
        *Pindex += strlen (OPERATOR_RIGHT_PARENTHESIS);
        LeftTrim (Pbegin, length, Pindex);

        if (ParseRightFactor (Pbegin, length, Pindex)) {
          return TRUE;
        } else {
          *Pindex = Pin;
        }
      }
    }
  }
  //
  // <factor>   ::= NOT <factor> <rightbool> <rightfactor>
  //
  if (strncmp (*Pindex, OPERATOR_NOT, strlen (OPERATOR_NOT)) == 0) {
    *Pindex += strlen (OPERATOR_NOT);
    LeftTrim (Pbegin, length, Pindex);

    if (ParseFactor (Pbegin, length, Pindex)) {
      LeftTrim (Pbegin, length, Pindex);

      if (ParseRightBool (Pbegin, length, Pindex)) {
        LeftTrim (Pbegin, length, Pindex);

        if (ParseRightFactor (Pbegin, length, Pindex)) {
          return TRUE;
        } else {
          *Pindex = Pin;
        }
      } else {
        *Pindex = Pin;
      }
    } else {
      *Pindex = Pin;
    }
  }
  //
  // <factor>   ::= TRUE <rightfactor>
  //
  if (strncmp (*Pindex, OPERATOR_TRUE, strlen (OPERATOR_TRUE)) == 0) {
    *Pindex += strlen (OPERATOR_TRUE);
    LeftTrim (Pbegin, length, Pindex);

    if (ParseRightFactor (Pbegin, length, Pindex)) {
      return TRUE;
    } else {
      *Pindex = Pin;
    }
  }
  //
  // <factor>   ::= FALSE <rightfactor>
  //
  if (strncmp (*Pindex, OPERATOR_FALSE, strlen (OPERATOR_FALSE)) == 0) {
    *Pindex += strlen (OPERATOR_FALSE);
    LeftTrim (Pbegin, length, Pindex);

    if (ParseRightFactor (Pbegin, length, Pindex)) {
      return TRUE;
    } else {
      *Pindex = Pin;
    }
  }
  //
  // <factor>   ::= <guid> <rightfactor>
  //
  if (ParseGuid (Pbegin, length, Pindex)) {
    LeftTrim (Pbegin, length, Pindex);

    if (ParseRightFactor (Pbegin, length, Pindex)) {
      return TRUE;
    } else {
      *Pindex = Pin;
      return FALSE;
    }
  } else {
    *Pindex = Pin;
    return FALSE;
  }
}

BOOLEAN
ParseTerm (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse term in bool expression.

Arguments:

  Pbegin    The pointer to the string  
  length    length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If string is a valid term, return TRUE, otherwise FALSE


--*/
{
  INT8  *Pin;

  Pin = *Pindex;
  LeftTrim (Pbegin, length, Pindex);

  //
  // <term>     ::= NOT <factor>
  //
  if (strncmp (*Pindex, OPERATOR_NOT, strlen (OPERATOR_NOT)) == 0) {
    *Pindex += strlen (OPERATOR_NOT);
    LeftTrim (Pbegin, length, Pindex);

    if (!ParseFactor (Pbegin, length, Pindex)) {
      *Pindex = Pin;
    } else {
      return TRUE;
    }
  }
  //
  // <term>     ::=<factor>
  //
  if (ParseFactor (Pbegin, length, Pindex)) {
    return TRUE;
  } else {
    *Pindex = Pin;
    return FALSE;
  }
}

BOOLEAN
ParseBool (
  IN      INT8      *Pbegin,
  IN      UINT32    length,
  IN OUT  INT8      **Pindex
  )
/*++

Routine Description:

  Parse bool expression.

Arguments:

  Pbegin    The pointer to the string  
  length    length of the string
  Pindex    The pointer of pointer to the next parse character in the string

Returns:

  BOOLEAN   If string is a valid bool expression, return TRUE, otherwise FALSE


--*/
{
  INT8  *Pin;
  Pin = *Pindex;
  LeftTrim (Pbegin, length, Pindex);

  if (ParseTerm (Pbegin, length, Pindex)) {
    LeftTrim (Pbegin, length, Pindex);

    if (!ParseRightBool (Pbegin, length, Pindex)) {
      *Pindex = Pin;
      return FALSE;
    } else {
      return TRUE;
    }
  } else {
    *Pindex = Pin;
    return FALSE;
  }
}

BOOLEAN
ParseDepex (
  IN      INT8      *Pbegin,
  IN      UINT32    length
  )
/*++

Routine Description:

  Parse whole dependency expression.

Arguments:

  Pbegin    The pointer to the string  
  length    length of the string

Returns:

  BOOLEAN   If string is a valid dependency expression, return TRUE, otherwise FALSE


--*/
{
  BOOLEAN Result;
  INT8    **Pindex;
  INT8    *temp;

  Result  = FALSE;
  temp    = Pbegin;
  Pindex  = &temp;

  LeftTrim (Pbegin, length, Pindex);
  if (strncmp (*Pindex, OPERATOR_BEFORE, strlen (OPERATOR_BEFORE)) == 0) {
    (*Pindex) += strlen (OPERATOR_BEFORE);
    Result = ParseGuid (Pbegin, length, Pindex);

  } else if (strncmp (*Pindex, OPERATOR_AFTER, strlen (OPERATOR_AFTER)) == 0) {
    (*Pindex) += strlen (OPERATOR_AFTER);
    Result = ParseGuid (Pbegin, length, Pindex);

  } else if (strncmp (*Pindex, OPERATOR_SOR, strlen (OPERATOR_SOR)) == 0) {
    (*Pindex) += strlen (OPERATOR_SOR);
    Result = ParseBool (Pbegin, length, Pindex);

  } else {
    Result = ParseBool (Pbegin, length, Pindex);

  }

  LeftTrim (Pbegin, length, Pindex);
  return (BOOLEAN) (Result && (*Pindex) >= (Pbegin + length));
}
