## @file
# This file is used to check PCD logical expression
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

'''
ExpressionValidate
'''

##
# Import Modules
#
import re
from Logger import StringTable as ST

## IsValidBareCString
#
# Check if String is comprised by whitespace(0x20), !(0x21), 0x23 - 0x7E
# or '\n', '\t', '\f', '\r', '\b', '\0', '\\'
#
# @param String: string to be checked
#
def IsValidBareCString(String):
    EscapeList = ['n', 't', 'f', 'r', 'b', '0', '\\', '"']
    PreChar = ''
    LastChar = ''
    for Char in String:
        LastChar = Char
        if PreChar == '\\':
            if Char not in EscapeList:
                return False
            if Char == '\\':
                PreChar = ''
                continue
        else:
            IntChar = ord(Char)
            if IntChar != 0x20 and IntChar != 0x09 and IntChar != 0x21 \
                and (IntChar < 0x23 or IntChar > 0x7e):
                return False
        PreChar = Char
    
    # Last char cannot be \ if PreChar is not \
    if LastChar == '\\' and PreChar == LastChar:
        return False
    return True

def _ValidateToken(Token):
    Token = Token.strip()
    Index = Token.find("\"")
    if Index != -1:
        return IsValidBareCString(Token[Index+1:-1])
    return True

## _ExprError
#
# @param      Exception:    Exception
#
class _ExprError(Exception):
    def __init__(self, Error = ''):
        Exception.__init__(self)
        self.Error = Error

## _ExprBase
#
class _ExprBase:
    HEX_PATTERN = '[\t\s]*0[xX][a-fA-F0-9]+'
    INT_PATTERN = '[\t\s]*[0-9]+'
    MACRO_PATTERN = '[\t\s]*\$\(([A-Z][_A-Z0-9]*)\)'
    PCD_PATTERN = \
    '[\t\s]*[_a-zA-Z][a-zA-Z0-9_]*[\t\s]*\.[\t\s]*[_a-zA-Z][a-zA-Z0-9_]*'
    QUOTED_PATTERN = '[\t\s]*L?"[^"]*"'
    BOOL_PATTERN = '[\t\s]*(true|True|TRUE|false|False|FALSE)'
    def __init__(self, Token):
        self.Token = Token
        self.Index = 0
        self.Len = len(Token)
    
    ## SkipWhitespace
    #
    def SkipWhitespace(self):
        for Char in self.Token[self.Index:]:
            if Char not in ' \t':
                break
            self.Index += 1
    
    ## IsCurrentOp
    #
    # @param      OpList:   option list 
    #    
    def IsCurrentOp(self, OpList):
        self.SkipWhitespace()
        LetterOp = ["EQ", "NE", "GE", "LE", "GT", "LT", "NOT", "and", "AND", 
                    "or", "OR", "XOR"]
        OpMap = {
            '|' : '|',
            '&' : '&',
            '!' : '=',
            '>' : '=',
            '<' : '='
        }
        for Operator in OpList:
            if not self.Token[self.Index:].startswith(Operator):
                continue
            self.Index += len(Operator)
            Char = self.Token[self.Index : self.Index + 1]
            if (Operator in LetterOp and (Char == '_' or Char.isalnum())) \
                or (Operator in OpMap and OpMap[Operator] == Char):
                self.Index -= len(Operator)
                break
            return True
        return False

## _LogicalExpressionParser
#
# @param      _ExprBase:   _ExprBase object
#    
class _LogicalExpressionParser(_ExprBase):
    #
    # STRINGITEM can only be logical field according to spec
    #
    STRINGITEM = -1
    
    #
    # Evaluate to True or False
    #
    LOGICAL = 0
    REALLOGICAL = 2
    
    #
    # Just arithmetic expression
    #
    ARITH = 1
    
    def __init__(self, Token):
        _ExprBase.__init__(self, Token)
        self.Parens = 0
    
    def _CheckToken(self, MatchList):
        for Match in MatchList:
            if Match and Match.start() == 0:
                if not _ValidateToken(
                            self.Token[self.Index:self.Index+Match.end()]
                        ):
                    return False
                
                self.Index += Match.end()
                if self.Token[self.Index - 1] == '"':
                    return True
                if self.Token[self.Index:self.Index+1] == '_' or \
                    self.Token[self.Index:self.Index+1].isalnum():
                    self.Index -= Match.end()
                    return False
                
                Token = self.Token[self.Index - Match.end():self.Index]
                if Token.strip() in ["EQ", "NE", "GE", "LE", "GT", "LT",
                    "NOT", "and", "AND", "or", "OR", "XOR"]:
                    self.Index -= Match.end()
                    return False
                
                return True
        return False
    
    def IsAtomicNumVal(self):
        #
        # Hex number
        #
        Match1 = re.compile(self.HEX_PATTERN).match(self.Token[self.Index:])
        
        #
        # Number
        #
        Match2 = re.compile(self.INT_PATTERN).match(self.Token[self.Index:])
        
        #
        # Macro
        #
        Match3 = re.compile(self.MACRO_PATTERN).match(self.Token[self.Index:])
        
        #
        # PcdName
        #
        Match4 = re.compile(self.PCD_PATTERN).match(self.Token[self.Index:])
        
        return self._CheckToken([Match1, Match2, Match3, Match4])
    

    def IsAtomicItem(self):
        #
        # Macro
        #
        Match1 = re.compile(self.MACRO_PATTERN).match(self.Token[self.Index:])
        
        #
        # PcdName
        #
        Match2 = re.compile(self.PCD_PATTERN).match(self.Token[self.Index:])
        
        #
        # Quoted string
        #
        Match3 = re.compile(self.QUOTED_PATTERN).\
            match(self.Token[self.Index:].replace('\\\\', '//').\
                  replace('\\\"', '\\\''))
        
        return self._CheckToken([Match1, Match2, Match3])
    
    ## A || B
    #
    def LogicalExpression(self):
        Ret = self.SpecNot()
        while self.IsCurrentOp(['||', 'OR', 'or', '&&', 'AND', 'and', 'XOR']):
            if self.Token[self.Index-1] == '|' and self.Parens <= 0:
                raise  _ExprError(ST.ERR_EXPR_OR)
            if Ret == self.ARITH:
                raise _ExprError(ST.ERR_EXPR_LOGICAL % self.Token)
            Ret = self.SpecNot()
            if Ret == self.ARITH:
                raise _ExprError(ST.ERR_EXPR_LOGICAL % self.Token)
            Ret = self.REALLOGICAL
        return Ret
    
    def SpecNot(self):
        if self.IsCurrentOp(["NOT", "!"]):
            return self.SpecNot()
        return self.Rel()
    
    ## A < B, A > B, A <= B, A >= b
    #
    def Rel(self):
        Ret = self.Expr()
        if self.IsCurrentOp(["<=", ">=", ">", "<", "GT", "LT", "GE", "LE",
                             "==", "EQ", "!=", "NE"]):
            if Ret == self.STRINGITEM or Ret == self.REALLOGICAL:
                raise _ExprError(ST.ERR_EXPR_LOGICAL % self.Token)
            Ret = self.Expr()
            if Ret == self.STRINGITEM or Ret == self.REALLOGICAL:
                raise _ExprError(ST.ERR_EXPR_LOGICAL % self.Token)
            Ret = self.REALLOGICAL
        return Ret
    
    ## A + B, A - B
    #
    def Expr(self):
        Ret = self.Factor()
        while self.IsCurrentOp(["+", "-", "&", "|", "^"]):
            if self.Token[self.Index-1] == '|' and self.Parens <= 0:
                raise  _ExprError(ST.ERR_EXPR_OR)
            if Ret == self.STRINGITEM or Ret == self.REALLOGICAL:
                raise _ExprError(ST.ERR_EXPR_LOGICAL % self.Token)
            Ret = self.Factor()
            if Ret == self.STRINGITEM or Ret == self.REALLOGICAL:
                raise _ExprError(ST.ERR_EXPR_LOGICAL % self.Token)
            Ret = self.ARITH
        return Ret

    ## Factor
    #    
    def Factor(self):
        if self.IsCurrentOp(["("]):
            self.Parens += 1
            Ret = self.LogicalExpression()
            if not self.IsCurrentOp([")"]):
                raise _ExprError(ST.ERR_EXPR_RIGHT_PAREN % \
                                 (self.Token, self.Token[self.Index:]))
            self.Parens -= 1
            return Ret
        
        if self.IsAtomicItem():
            if self.Token[self.Index - 1] == '"':
                return self.STRINGITEM
            return self.LOGICAL
        elif self.IsAtomicNumVal():
            return self.ARITH
        else:
            raise _ExprError(ST.ERR_EXPR_FACTOR % \
                             (self.Token, self.Token[self.Index:]))
            
    ## IsValidLogicalExpression
    #
    def IsValidLogicalExpression(self):
        if self.Len == 0:
            return False, ST.ERR_EXPR_EMPTY
        try:
            if self.LogicalExpression() == self.ARITH:
                return False, ST.ERR_EXPR_LOGICAL % self.Token
        except _ExprError, XExcept:
            return False, XExcept.Error
        self.SkipWhitespace()
        if self.Index != self.Len:
            return False, (ST.ERR_EXPR_BOOLEAN % \
                           (self.Token[self.Index:], self.Token))
        return True, ''

## _ValidRangeExpressionParser
#
class _ValidRangeExpressionParser(_ExprBase):
    INT_RANGE_PATTERN = '[\t\s]*[0-9]+[\t\s]*-[\t\s]*[0-9]+'
    HEX_RANGE_PATTERN = \
        '[\t\s]*0[xX][a-fA-F0-9]+[\t\s]*-[\t\s]*0[xX][a-fA-F0-9]+'
    def __init__(self, Token):
        _ExprBase.__init__(self, Token)
    
    ## IsValidRangeExpression
    #
    def IsValidRangeExpression(self):
        if self.Len == 0:
            return False
        try:
            self.RangeExpression()
        except _ExprError:
            return False
        self.SkipWhitespace()
        if self.Index != self.Len:
            return False
        return True
    
    ## RangeExpression
    #
    def RangeExpression(self):
        self.Unary()
        while self.IsCurrentOp(['OR', 'AND', 'XOR']):
            self.Unary()
    
    ## Unary
    #
    def Unary(self):
        if self.IsCurrentOp(["NOT", "-"]):
            return self.Unary()
        return self.ValidRange()
    
    ## ValidRange
    #    
    def ValidRange(self):
        if self.IsCurrentOp(["("]):
            self.RangeExpression()
            if not self.IsCurrentOp([")"]):
                raise _ExprError('')
            return
        
        if self.IsCurrentOp(["LT", "GT", "LE", "GE", "EQ"]):
            IntMatch = \
                re.compile(self.INT_PATTERN).match(self.Token[self.Index:])
            HexMatch = \
                re.compile(self.HEX_PATTERN).match(self.Token[self.Index:])
            if HexMatch and HexMatch.start() == 0:
                self.Index += HexMatch.end()
            elif IntMatch and IntMatch.start() == 0:
                self.Index += IntMatch.end()
            else:
                raise _ExprError('')
        else:
            IntRangeMatch = re.compile(
                self.INT_RANGE_PATTERN).match(self.Token[self.Index:]
            )
            HexRangeMatch = re.compile(
                self.HEX_RANGE_PATTERN).match(self.Token[self.Index:]
            )
            if HexRangeMatch and HexRangeMatch.start() == 0:
                self.Index += HexRangeMatch.end()
            elif IntRangeMatch and IntRangeMatch.start() == 0:
                self.Index += IntRangeMatch.end()
            else:
                raise _ExprError('')
        
        if self.Token[self.Index:self.Index+1] == '_' or \
            self.Token[self.Index:self.Index+1].isalnum():
            raise _ExprError('')

## _StringTestParser
#
class _StringTestParser(_ExprBase):
    def __init__(self, Token):
        _ExprBase.__init__(self, Token)

    ## IsValidStringTest
    #        
    def IsValidStringTest(self):
        if self.Len == 0:
            return False, ST.ERR_EXPR_EMPTY
        try:
            self.StringTest()
        except _ExprError, XExcept:
            return False, XExcept.Error
        return True, ''

    ## StringItem
    #        
    def StringItem(self):
        Match1 = re.compile(self.QUOTED_PATTERN)\
            .match(self.Token[self.Index:].replace('\\\\', '//')\
                   .replace('\\\"', '\\\''))
        Match2 = re.compile(self.MACRO_PATTERN).match(self.Token[self.Index:])
        Match3 = re.compile(self.PCD_PATTERN).match(self.Token[self.Index:])
        MatchList = [Match1, Match2, Match3]
        for Match in MatchList:
            if Match and Match.start() == 0:
                if not _ValidateToken(
                            self.Token[self.Index:self.Index+Match.end()]
                        ):
                    raise _ExprError(ST.ERR_EXPR_STRING_ITEM % \
                                     (self.Token, self.Token[self.Index:]))
                self.Index += Match.end()
                Token = self.Token[self.Index - Match.end():self.Index]
                if Token.strip() in ["EQ", "NE"]:
                    raise _ExprError(ST.ERR_EXPR_STRING_ITEM % \
                             (self.Token, self.Token[self.Index:]))
                return
        else:
            raise _ExprError(ST.ERR_EXPR_STRING_ITEM % \
                             (self.Token, self.Token[self.Index:]))

    ## StringTest
    #        
    def StringTest(self):
        self.StringItem()
        if not self.IsCurrentOp(["==", "EQ", "!=", "NE"]):
            raise _ExprError(ST.ERR_EXPR_EQUALITY % \
                             (self.Token, self.Token[self.Index:]))
        self.StringItem()
        if self.Index != self.Len:
            raise _ExprError(ST.ERR_EXPR_BOOLEAN % \
                             (self.Token[self.Index:], self.Token))

##
# Check syntax of logical expression
#
# @param Token: expression token
#
def IsValidLogicalExpr(Token, Flag=False):
    #
    # Not do the check right now, keep the implementation for future enhancement.
    #
    if not Flag:
        return True, ""
    return _LogicalExpressionParser(Token).IsValidLogicalExpression()

##
# Check syntax of string test
#
# @param Token: string test token
#
def IsValidStringTest(Token, Flag=False):
    #
    # Not do the check right now, keep the implementation for future enhancement.
    #
    if not Flag:
        return True, ""
    return _StringTestParser(Token).IsValidStringTest()

##
# Check syntax of range expression
#
# @param Token: range expression token
#
def IsValidRangeExpr(Token):
    return _ValidRangeExpressionParser(Token).IsValidRangeExpression()

##
# Check whether the feature flag expression is valid or not
#
# @param Token: feature flag expression
#
def IsValidFeatureFlagExp(Token, Flag=False):
    #
    # Not do the check right now, keep the implementation for future enhancement.
    #
    if not Flag:
        return True, "", Token
    else:
        if Token in ['TRUE', 'FALSE', 'true', 'false', 'True', 'False',
                     '0x1', '0x01', '0x0', '0x00']:
            return True, ""
        Valid, Cause = IsValidStringTest(Token, Flag)
        if not Valid:
            Valid, Cause = IsValidLogicalExpr(Token, Flag)
        if not Valid:
            return False, Cause   
        return True, ""

if __name__ == '__main__':
    print _LogicalExpressionParser('a ^ b > a + b').IsValidLogicalExpression()
