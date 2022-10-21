## @file
# This file is used to parse and evaluate expression in directive or PCD value.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

## Import Modules
#
from __future__ import print_function
from __future__ import absolute_import
from Common.GlobalData import *
from CommonDataClass.Exceptions import BadExpression
from CommonDataClass.Exceptions import WrnExpression
from .Misc import GuidStringToGuidStructureString, ParseFieldValue,CopyDict
import Common.EdkLogger as EdkLogger
import copy
from Common.DataType import *
import sys
from random import sample
import string

ERR_STRING_EXPR         = 'This operator cannot be used in string expression: [%s].'
ERR_SNYTAX              = 'Syntax error, the rest of expression cannot be evaluated: [%s].'
ERR_MATCH               = 'No matching right parenthesis.'
ERR_STRING_TOKEN        = 'Bad string token: [%s].'
ERR_MACRO_TOKEN         = 'Bad macro token: [%s].'
ERR_EMPTY_TOKEN         = 'Empty token is not allowed.'
ERR_PCD_RESOLVE         = 'The PCD should be FeatureFlag type or FixedAtBuild type: [%s].'
ERR_VALID_TOKEN         = 'No more valid token found from rest of string: [%s].'
ERR_EXPR_TYPE           = 'Different types found in expression.'
ERR_OPERATOR_UNSUPPORT  = 'Unsupported operator: [%s]'
ERR_REL_NOT_IN          = 'Expect "IN" after "not" operator.'
WRN_BOOL_EXPR           = 'Operand of boolean type cannot be used in arithmetic expression.'
WRN_EQCMP_STR_OTHERS    = '== Comparison between Operand of string type and Boolean/Number Type always return False.'
WRN_NECMP_STR_OTHERS    = '!= Comparison between Operand of string type and Boolean/Number Type always return True.'
ERR_RELCMP_STR_OTHERS   = 'Operator taking Operand of string type and Boolean/Number Type is not allowed: [%s].'
ERR_STRING_CMP          = 'Unicode string and general string cannot be compared: [%s %s %s]'
ERR_ARRAY_TOKEN         = 'Bad C array or C format GUID token: [%s].'
ERR_ARRAY_ELE           = 'This must be HEX value for NList or Array: [%s].'
ERR_EMPTY_EXPR          = 'Empty expression is not allowed.'
ERR_IN_OPERAND          = 'Macro after IN operator can only be: $(FAMILY), $(ARCH), $(TOOL_CHAIN_TAG) and $(TARGET).'

__ValidString = re.compile(r'[_a-zA-Z][_0-9a-zA-Z]*$')
_ReLabel = re.compile('LABEL\((\w+)\)')
_ReOffset = re.compile('OFFSET_OF\((\w+)\)')
PcdPattern = re.compile(r'^[_a-zA-Z][0-9A-Za-z_]*\.[_a-zA-Z][0-9A-Za-z_]*$')

## SplitString
#  Split string to list according double quote
#  For example: abc"de\"f"ghi"jkl"mn will be: ['abc', '"de\"f"', 'ghi', '"jkl"', 'mn']
#
def SplitString(String):
    # There might be escaped quote: "abc\"def\\\"ghi", 'abc\'def\\\'ghi'
    RanStr = ''.join(sample(string.ascii_letters + string.digits, 8))
    String = String.replace('\\\\', RanStr).strip()
    RetList = []
    InSingleQuote = False
    InDoubleQuote = False
    Item = ''
    for i, ch in enumerate(String):
        if ch == '"' and not InSingleQuote:
            if String[i - 1] != '\\':
                InDoubleQuote = not InDoubleQuote
            if not InDoubleQuote:
                Item += String[i]
                RetList.append(Item)
                Item = ''
                continue
            if Item:
                RetList.append(Item)
                Item = ''
        elif ch == "'" and not InDoubleQuote:
            if String[i - 1] != '\\':
                InSingleQuote = not InSingleQuote
            if not InSingleQuote:
                Item += String[i]
                RetList.append(Item)
                Item = ''
                continue
            if Item:
                RetList.append(Item)
                Item = ''
        Item += String[i]
    if InSingleQuote or InDoubleQuote:
        raise BadExpression(ERR_STRING_TOKEN % Item)
    if Item:
        RetList.append(Item)
    for i, ch in enumerate(RetList):
        if RanStr in ch:
            RetList[i] = ch.replace(RanStr,'\\\\')
    return RetList

def SplitPcdValueString(String):
    # There might be escaped comma in GUID() or DEVICE_PATH() or " "
    # or ' ' or L' ' or L" "
    RanStr = ''.join(sample(string.ascii_letters + string.digits, 8))
    String = String.replace('\\\\', RanStr).strip()
    RetList = []
    InParenthesis = 0
    InSingleQuote = False
    InDoubleQuote = False
    Item = ''
    for i, ch in enumerate(String):
        if ch == '(':
            InParenthesis += 1
        elif ch == ')':
            if InParenthesis:
                InParenthesis -= 1
            else:
                raise BadExpression(ERR_STRING_TOKEN % Item)
        elif ch == '"' and not InSingleQuote:
            if String[i-1] != '\\':
                InDoubleQuote = not InDoubleQuote
        elif ch == "'" and not InDoubleQuote:
            if String[i-1] != '\\':
                InSingleQuote = not InSingleQuote
        elif ch == ',':
            if InParenthesis or InSingleQuote or InDoubleQuote:
                Item += String[i]
                continue
            elif Item:
                RetList.append(Item)
                Item = ''
            continue
        Item += String[i]
    if InSingleQuote or InDoubleQuote or InParenthesis:
        raise BadExpression(ERR_STRING_TOKEN % Item)
    if Item:
        RetList.append(Item)
    for i, ch in enumerate(RetList):
        if RanStr in ch:
            RetList[i] = ch.replace(RanStr,'\\\\')
    return RetList

def IsValidCName(Str):
    return True if __ValidString.match(Str) else False

def BuildOptionValue(PcdValue, GuidDict):
    if PcdValue.startswith('H'):
        InputValue = PcdValue[1:]
    elif PcdValue.startswith("L'") or PcdValue.startswith("'"):
        InputValue = PcdValue
    elif PcdValue.startswith('L'):
        InputValue = 'L"' + PcdValue[1:] + '"'
    else:
        InputValue = PcdValue
    try:
        PcdValue = ValueExpressionEx(InputValue, TAB_VOID, GuidDict)(True)
    except:
        pass

    return PcdValue

## ReplaceExprMacro
#
def ReplaceExprMacro(String, Macros, ExceptionList = None):
    StrList = SplitString(String)
    for i, String in enumerate(StrList):
        InQuote = False
        if String.startswith('"'):
            InQuote = True
        MacroStartPos = String.find('$(')
        if MacroStartPos < 0:
            for Pcd in gPlatformPcds:
                if Pcd in String:
                    if Pcd not in gConditionalPcds:
                        gConditionalPcds.append(Pcd)
            continue
        RetStr = ''
        while MacroStartPos >= 0:
            RetStr = String[0:MacroStartPos]
            MacroEndPos = String.find(')', MacroStartPos)
            if MacroEndPos < 0:
                raise BadExpression(ERR_MACRO_TOKEN % String[MacroStartPos:])
            Macro = String[MacroStartPos+2:MacroEndPos]
            if Macro not in Macros:
                # From C reference manual:
                # If an undefined macro name appears in the constant-expression of
                # !if or !elif, it is replaced by the integer constant 0.
                RetStr += '0'
            elif not InQuote:
                Tklst = RetStr.split()
                if Tklst and Tklst[-1] in {'IN', 'in'} and ExceptionList and Macro not in ExceptionList:
                    raise BadExpression(ERR_IN_OPERAND)
                # Make sure the macro in exception list is encapsulated by double quote
                # For example: DEFINE ARCH = IA32 X64
                # $(ARCH) is replaced with "IA32 X64"
                if ExceptionList and Macro in ExceptionList:
                    RetStr += '"' + Macros[Macro] + '"'
                elif Macros[Macro].strip():
                    RetStr += Macros[Macro]
                else:
                    RetStr += '""'
            else:
                RetStr += Macros[Macro]
            RetStr += String[MacroEndPos+1:]
            String = RetStr
            MacroStartPos = String.find('$(')
        StrList[i] = RetStr
    return ''.join(StrList)

# transfer int to string for in/not in expression
def IntToStr(Value):
    StrList = []
    while Value > 0:
        StrList.append(chr(Value & 0xff))
        Value = Value >> 8
    Value = '"' + ''.join(StrList) + '"'
    return Value

SupportedInMacroList = ['TARGET', 'TOOL_CHAIN_TAG', 'ARCH', 'FAMILY']

class BaseExpression(object):
    def __init__(self, *args, **kwargs):
        super(BaseExpression, self).__init__()

    # Check if current token matches the operators given from parameter
    def _IsOperator(self, OpSet):
        Idx = self._Idx
        self._GetOperator()
        if self._Token in OpSet:
            if self._Token in self.LogicalOperators:
                self._Token = self.LogicalOperators[self._Token]
            return True
        self._Idx = Idx
        return False

class ValueExpression(BaseExpression):
    # Logical operator mapping
    LogicalOperators = {
        '&&' : 'and', '||' : 'or',
        '!'  : 'not', 'AND': 'and',
        'OR' : 'or' , 'NOT': 'not',
        'XOR': '^'  , 'xor': '^',
        'EQ' : '==' , 'NE' : '!=',
        'GT' : '>'  , 'LT' : '<',
        'GE' : '>=' , 'LE' : '<=',
        'IN' : 'in'
    }

    NonLetterOpLst = ['+', '-', TAB_STAR, '/', '%', '&', '|', '^', '~', '<<', '>>', '!', '=', '>', '<', '?', ':']


    SymbolPattern = re.compile("("
                                 "\$\([A-Z][A-Z0-9_]*\)|\$\(\w+\.\w+\)|\w+\.\w+|"
                                 "&&|\|\||!(?!=)|"
                                 "(?<=\W)AND(?=\W)|(?<=\W)OR(?=\W)|(?<=\W)NOT(?=\W)|(?<=\W)XOR(?=\W)|"
                                 "(?<=\W)EQ(?=\W)|(?<=\W)NE(?=\W)|(?<=\W)GT(?=\W)|(?<=\W)LT(?=\W)|(?<=\W)GE(?=\W)|(?<=\W)LE(?=\W)"
                               ")")

    @staticmethod
    def Eval(Operator, Oprand1, Oprand2 = None):
        WrnExp = None

        if Operator not in {"==", "!=", ">=", "<=", ">", "<", "in", "not in"} and \
            (isinstance(Oprand1, type('')) or isinstance(Oprand2, type(''))):
            raise BadExpression(ERR_STRING_EXPR % Operator)
        if Operator in {'in', 'not in'}:
            if not isinstance(Oprand1, type('')):
                Oprand1 = IntToStr(Oprand1)
            if not isinstance(Oprand2, type('')):
                Oprand2 = IntToStr(Oprand2)
        TypeDict = {
            type(0)  : 0,
            # For python2 long type
            type(sys.maxsize + 1) : 0,
            type('') : 1,
            type(True) : 2
        }

        EvalStr = ''
        if Operator in {"!", "NOT", "not"}:
            if isinstance(Oprand1, type('')):
                raise BadExpression(ERR_STRING_EXPR % Operator)
            EvalStr = 'not Oprand1'
        elif Operator in {"~"}:
            if isinstance(Oprand1, type('')):
                raise BadExpression(ERR_STRING_EXPR % Operator)
            EvalStr = '~ Oprand1'
        else:
            if Operator in {"+", "-"} and (type(True) in {type(Oprand1), type(Oprand2)}):
                # Boolean in '+'/'-' will be evaluated but raise warning
                WrnExp = WrnExpression(WRN_BOOL_EXPR)
            elif type('') in {type(Oprand1), type(Oprand2)} and not isinstance(Oprand1, type(Oprand2)):
                # == between string and number/boolean will always return False, != return True
                if Operator == "==":
                    WrnExp = WrnExpression(WRN_EQCMP_STR_OTHERS)
                    WrnExp.result = False
                    raise WrnExp
                elif Operator == "!=":
                    WrnExp = WrnExpression(WRN_NECMP_STR_OTHERS)
                    WrnExp.result = True
                    raise WrnExp
                else:
                    raise BadExpression(ERR_RELCMP_STR_OTHERS % Operator)
            elif TypeDict[type(Oprand1)] != TypeDict[type(Oprand2)]:
                if Operator in {"==", "!=", ">=", "<=", ">", "<"} and set((TypeDict[type(Oprand1)], TypeDict[type(Oprand2)])) == set((TypeDict[type(True)], TypeDict[type(0)])):
                    # comparison between number and boolean is allowed
                    pass
                elif Operator in {'&', '|', '^', "and", "or"} and set((TypeDict[type(Oprand1)], TypeDict[type(Oprand2)])) == set((TypeDict[type(True)], TypeDict[type(0)])):
                    # bitwise and logical operation between number and boolean is allowed
                    pass
                else:
                    raise BadExpression(ERR_EXPR_TYPE)
            if isinstance(Oprand1, type('')) and isinstance(Oprand2, type('')):
                if ((Oprand1.startswith('L"') or Oprand1.startswith("L'")) and (not Oprand2.startswith('L"')) and (not Oprand2.startswith("L'"))) or \
                        (((not Oprand1.startswith('L"')) and (not Oprand1.startswith("L'"))) and (Oprand2.startswith('L"') or Oprand2.startswith("L'"))):
                    raise BadExpression(ERR_STRING_CMP % (Oprand1, Operator, Oprand2))
            if 'in' in Operator and isinstance(Oprand2, type('')):
                Oprand2 = Oprand2.split()
            EvalStr = 'Oprand1 ' + Operator + ' Oprand2'

        # Local symbols used by built in eval function
        Dict = {
            'Oprand1' : Oprand1,
            'Oprand2' : Oprand2
        }
        try:
            Val = eval(EvalStr, {}, Dict)
        except Exception as Excpt:
            raise BadExpression(str(Excpt))

        if Operator in {'and', 'or'}:
            if Val:
                Val = True
            else:
                Val = False

        if WrnExp:
            WrnExp.result = Val
            raise WrnExp
        return Val

    def __init__(self, Expression, SymbolTable={}):
        super(ValueExpression, self).__init__(self, Expression, SymbolTable)
        self._NoProcess = False
        if not isinstance(Expression, type('')):
            self._Expr = Expression
            self._NoProcess = True
            return

        self._Expr = ReplaceExprMacro(Expression.strip(),
                                  SymbolTable,
                                  SupportedInMacroList)

        if not self._Expr.strip():
            raise BadExpression(ERR_EMPTY_EXPR)

        #
        # The symbol table including PCD and macro mapping
        #
        self._Symb = CopyDict(SymbolTable)
        self._Symb.update(self.LogicalOperators)
        self._Idx = 0
        self._Len = len(self._Expr)
        self._Token = ''
        self._WarnExcept = None

        # Literal token without any conversion
        self._LiteralToken = ''

    # Public entry for this class
    #   @param RealValue: False: only evaluate if the expression is true or false, used for conditional expression
    #                     True : return the evaluated str(value), used for PCD value
    #
    #   @return: True or False if RealValue is False
    #            Evaluated value of string format if RealValue is True
    #
    def __call__(self, RealValue=False, Depth=0):
        if self._NoProcess:
            return self._Expr

        self._Depth = Depth

        self._Expr = self._Expr.strip()
        if RealValue and Depth == 0:
            self._Token = self._Expr
            if self.__IsNumberToken():
                return self._Expr
            Token = ''
            try:
                Token = self._GetToken()
            except BadExpression:
                pass
            if isinstance(Token, type('')) and Token.startswith('{') and Token.endswith('}') and self._Idx >= self._Len:
                return self._Expr

            self._Idx = 0
            self._Token = ''

        Val = self._ConExpr()
        RealVal = Val
        if isinstance(Val, type('')):
            if Val == 'L""':
                Val = False
            elif not Val:
                Val = False
                RealVal = '""'
            elif not Val.startswith('L"') and not Val.startswith('{') and not Val.startswith("L'") and not Val.startswith("'"):
                Val = True
                RealVal = '"' + RealVal + '"'

        # The expression has been parsed, but the end of expression is not reached
        # It means the rest does not comply EBNF of <Expression>
        if self._Idx != self._Len:
            raise BadExpression(ERR_SNYTAX % self._Expr[self._Idx:])

        if RealValue:
            RetVal = str(RealVal)
        elif Val:
            RetVal = True
        else:
            RetVal = False

        if self._WarnExcept:
            self._WarnExcept.result = RetVal
            raise self._WarnExcept
        else:
            return RetVal

    # Template function to parse binary operators which have same precedence
    # Expr [Operator Expr]*
    def _ExprFuncTemplate(self, EvalFunc, OpSet):
        Val = EvalFunc()
        while self._IsOperator(OpSet):
            Op = self._Token
            if Op == '?':
                Val2 = EvalFunc()
                if self._IsOperator({':'}):
                    Val3 = EvalFunc()
                if Val:
                    Val = Val2
                else:
                    Val = Val3
                continue
            #
            # PEP 238 -- Changing the Division Operator
            # x/y to return a reasonable approximation of the mathematical result of the division ("true division")
            # x//y to return the floor ("floor division")
            #
            if Op == '/':
                Op = '//'
            try:
                Val = self.Eval(Op, Val, EvalFunc())
            except WrnExpression as Warn:
                self._WarnExcept = Warn
                Val = Warn.result
        return Val
    # A [? B]*
    def _ConExpr(self):
        return self._ExprFuncTemplate(self._OrExpr, {'?', ':'})

    # A [|| B]*
    def _OrExpr(self):
        return self._ExprFuncTemplate(self._AndExpr, {"OR", "or", "||"})

    # A [&& B]*
    def _AndExpr(self):
        return self._ExprFuncTemplate(self._BitOr, {"AND", "and", "&&"})

    # A [ | B]*
    def _BitOr(self):
        return self._ExprFuncTemplate(self._BitXor, {"|"})

    # A [ ^ B]*
    def _BitXor(self):
        return self._ExprFuncTemplate(self._BitAnd, {"XOR", "xor", "^"})

    # A [ & B]*
    def _BitAnd(self):
        return self._ExprFuncTemplate(self._EqExpr, {"&"})

    # A [ == B]*
    def _EqExpr(self):
        Val = self._RelExpr()
        while self._IsOperator({"==", "!=", "EQ", "NE", "IN", "in", "!", "NOT", "not"}):
            Op = self._Token
            if Op in {"!", "NOT", "not"}:
                if not self._IsOperator({"IN", "in"}):
                    raise BadExpression(ERR_REL_NOT_IN)
                Op += ' ' + self._Token
            try:
                Val = self.Eval(Op, Val, self._RelExpr())
            except WrnExpression as Warn:
                self._WarnExcept = Warn
                Val = Warn.result
        return Val

    # A [ > B]*
    def _RelExpr(self):
        return self._ExprFuncTemplate(self._ShiftExpr, {"<=", ">=", "<", ">", "LE", "GE", "LT", "GT"})

    def _ShiftExpr(self):
        return self._ExprFuncTemplate(self._AddExpr, {"<<", ">>"})

    # A [ + B]*
    def _AddExpr(self):
        return self._ExprFuncTemplate(self._MulExpr, {"+", "-"})

    # A [ * B]*
    def _MulExpr(self):
        return self._ExprFuncTemplate(self._UnaryExpr, {TAB_STAR, "/", "%"})

    # [!]*A
    def _UnaryExpr(self):
        if self._IsOperator({"!", "NOT", "not"}):
            Val = self._UnaryExpr()
            try:
                return self.Eval('not', Val)
            except WrnExpression as Warn:
                self._WarnExcept = Warn
                return Warn.result
        if self._IsOperator({"~"}):
            Val = self._UnaryExpr()
            try:
                return self.Eval('~', Val)
            except WrnExpression as Warn:
                self._WarnExcept = Warn
                return Warn.result
        return self._IdenExpr()

    # Parse identifier or encapsulated expression
    def _IdenExpr(self):
        Tk = self._GetToken()
        if Tk == '(':
            Val = self._ConExpr()
            try:
                # _GetToken may also raise BadExpression
                if self._GetToken() != ')':
                    raise BadExpression(ERR_MATCH)
            except BadExpression:
                raise BadExpression(ERR_MATCH)
            return Val
        return Tk

    # Skip whitespace or tab
    def __SkipWS(self):
        for Char in self._Expr[self._Idx:]:
            if Char not in ' \t':
                break
            self._Idx += 1

    # Try to convert string to number
    def __IsNumberToken(self):
        Radix = 10
        if self._Token.lower()[0:2] == '0x' and len(self._Token) > 2:
            Radix = 16
        if self._Token.startswith('"') or self._Token.startswith('L"'):
            Flag = 0
            for Index in range(len(self._Token)):
                if self._Token[Index] in {'"'}:
                    if self._Token[Index - 1] == '\\':
                        continue
                    Flag += 1
            if Flag == 2 and self._Token.endswith('"'):
                return True
        if self._Token.startswith("'") or self._Token.startswith("L'"):
            Flag = 0
            for Index in range(len(self._Token)):
                if self._Token[Index] in {"'"}:
                    if self._Token[Index - 1] == '\\':
                        continue
                    Flag += 1
            if Flag == 2 and self._Token.endswith("'"):
                return True
        try:
            self._Token = int(self._Token, Radix)
            return True
        except ValueError:
            return False
        except TypeError:
            return False

    # Parse array: {...}
    def __GetArray(self):
        Token = '{'
        self._Idx += 1
        self.__GetNList(True)
        Token += self._LiteralToken
        if self._Idx >= self._Len or self._Expr[self._Idx] != '}':
            raise BadExpression(ERR_ARRAY_TOKEN % Token)
        Token += '}'

        # All whitespace and tabs in array are already stripped.
        IsArray = IsGuid = False
        if len(Token.split(',')) == 11 and len(Token.split(',{')) == 2 \
            and len(Token.split('},')) == 1:
            HexLen = [11, 6, 6, 5, 4, 4, 4, 4, 4, 4, 6]
            HexList= Token.split(',')
            if HexList[3].startswith('{') and \
                not [Index for Index, Hex in enumerate(HexList) if len(Hex) > HexLen[Index]]:
                IsGuid = True
        if Token.lstrip('{').rstrip('}').find('{') == -1:
            if not [Hex for Hex in Token.lstrip('{').rstrip('}').split(',') if len(Hex) > 4]:
                IsArray = True
        if not IsArray and not IsGuid:
            raise BadExpression(ERR_ARRAY_TOKEN % Token)
        self._Idx += 1
        self._Token = self._LiteralToken = Token
        return self._Token

    # Parse string, the format must be: "..."
    def __GetString(self):
        Idx = self._Idx

        # Skip left quote
        self._Idx += 1

        # Replace escape \\\", \"
        if self._Expr[Idx] == '"':
            Expr = self._Expr[self._Idx:].replace('\\\\', '//').replace('\\\"', '\\\'')
            for Ch in Expr:
                self._Idx += 1
                if Ch == '"':
                    break
            self._Token = self._LiteralToken = self._Expr[Idx:self._Idx]
            if not self._Token.endswith('"'):
                raise BadExpression(ERR_STRING_TOKEN % self._Token)
        #Replace escape \\\', \'
        elif self._Expr[Idx] == "'":
            Expr = self._Expr[self._Idx:].replace('\\\\', '//').replace("\\\'", "\\\"")
            for Ch in Expr:
                self._Idx += 1
                if Ch == "'":
                    break
            self._Token = self._LiteralToken = self._Expr[Idx:self._Idx]
            if not self._Token.endswith("'"):
                raise BadExpression(ERR_STRING_TOKEN % self._Token)
        self._Token = self._Token[1:-1]
        return self._Token

    # Get token that is comprised by alphanumeric, underscore or dot(used by PCD)
    # @param IsAlphaOp: Indicate if parsing general token or script operator(EQ, NE...)
    def __GetIdToken(self, IsAlphaOp = False):
        IdToken = ''
        for Ch in self._Expr[self._Idx:]:
            if not self.__IsIdChar(Ch) or ('?' in self._Expr and Ch == ':'):
                break
            self._Idx += 1
            IdToken += Ch

        self._Token = self._LiteralToken = IdToken
        if not IsAlphaOp:
            self.__ResolveToken()
        return self._Token

    # Try to resolve token
    def __ResolveToken(self):
        if not self._Token:
            raise BadExpression(ERR_EMPTY_TOKEN)

        # PCD token
        if PcdPattern.match(self._Token):
            if self._Token not in self._Symb:
                Ex = BadExpression(ERR_PCD_RESOLVE % self._Token)
                Ex.Pcd = self._Token
                raise Ex
            self._Token = ValueExpression(self._Symb[self._Token], self._Symb)(True, self._Depth+1)
            if not isinstance(self._Token, type('')):
                self._LiteralToken = hex(self._Token)
                return

        if self._Token.startswith('"'):
            self._Token = self._Token[1:-1]
        elif self._Token in {"FALSE", "false", "False"}:
            self._Token = False
        elif self._Token in {"TRUE", "true", "True"}:
            self._Token = True
        else:
            self.__IsNumberToken()

    def __GetNList(self, InArray=False):
        self._GetSingleToken()
        if not self.__IsHexLiteral():
            if InArray:
                raise BadExpression(ERR_ARRAY_ELE % self._Token)
            return self._Token

        self.__SkipWS()
        Expr = self._Expr[self._Idx:]
        if not Expr.startswith(','):
            return self._Token

        NList = self._LiteralToken
        while Expr.startswith(','):
            NList += ','
            self._Idx += 1
            self.__SkipWS()
            self._GetSingleToken()
            if not self.__IsHexLiteral():
                raise BadExpression(ERR_ARRAY_ELE % self._Token)
            NList += self._LiteralToken
            self.__SkipWS()
            Expr = self._Expr[self._Idx:]
        self._Token = self._LiteralToken = NList
        return self._Token

    def __IsHexLiteral(self):
        if self._LiteralToken.startswith('{') and \
            self._LiteralToken.endswith('}'):
            return True

        if gHexPattern.match(self._LiteralToken):
            Token = self._LiteralToken[2:]
            if not Token:
                self._LiteralToken = '0x0'
            else:
                self._LiteralToken = '0x' + Token
            return True
        return False

    def _GetToken(self):
        return self.__GetNList()

    @staticmethod
    def __IsIdChar(Ch):
        return Ch in '._:' or Ch.isalnum()

    # Parse operand
    def _GetSingleToken(self):
        self.__SkipWS()
        Expr = self._Expr[self._Idx:]
        if Expr.startswith('L"'):
            # Skip L
            self._Idx += 1
            UStr = self.__GetString()
            self._Token = 'L"' + UStr + '"'
            return self._Token
        elif Expr.startswith("L'"):
            # Skip L
            self._Idx += 1
            UStr = self.__GetString()
            self._Token = "L'" + UStr + "'"
            return self._Token
        elif Expr.startswith("'"):
            UStr = self.__GetString()
            self._Token = "'" + UStr + "'"
            return self._Token
        elif Expr.startswith('UINT'):
            Re = re.compile('(?:UINT8|UINT16|UINT32|UINT64)\((.+)\)')
            try:
                RetValue = Re.search(Expr).group(1)
            except:
                 raise BadExpression('Invalid Expression %s' % Expr)
            Idx = self._Idx
            for Ch in Expr:
                self._Idx += 1
                if Ch == '(':
                    Prefix = self._Expr[Idx:self._Idx - 1]
                    Idx = self._Idx
                if Ch == ')':
                    TmpValue = self._Expr[Idx :self._Idx - 1]
                    TmpValue = ValueExpression(TmpValue)(True)
                    TmpValue = '0x%x' % int(TmpValue) if not isinstance(TmpValue, type('')) else TmpValue
                    break
            self._Token, Size = ParseFieldValue(Prefix + '(' + TmpValue + ')')
            return  self._Token

        self._Token = ''
        if Expr:
            Ch = Expr[0]
            Match = gGuidPattern.match(Expr)
            if Match and not Expr[Match.end():Match.end()+1].isalnum() \
                and Expr[Match.end():Match.end()+1] != '_':
                self._Idx += Match.end()
                self._Token = ValueExpression(GuidStringToGuidStructureString(Expr[0:Match.end()]))(True, self._Depth+1)
                return self._Token
            elif self.__IsIdChar(Ch):
                return self.__GetIdToken()
            elif Ch == '"':
                return self.__GetString()
            elif Ch == '{':
                return self.__GetArray()
            elif Ch == '(' or Ch == ')':
                self._Idx += 1
                self._Token = Ch
                return self._Token

        raise BadExpression(ERR_VALID_TOKEN % Expr)

    # Parse operator
    def _GetOperator(self):
        self.__SkipWS()
        LegalOpLst = ['&&', '||', '!=', '==', '>=', '<='] + self.NonLetterOpLst + ['?', ':']

        self._Token = ''
        Expr = self._Expr[self._Idx:]

        # Reach end of expression
        if not Expr:
            return ''

        # Script operator: LT, GT, LE, GE, EQ, NE, and, or, xor, not
        if Expr[0].isalpha():
            return self.__GetIdToken(True)

        # Start to get regular operator: +, -, <, > ...
        if Expr[0] not in self.NonLetterOpLst:
            return ''

        OpToken = ''
        for Ch in Expr:
            if Ch in self.NonLetterOpLst:
                if Ch in ['!', '~'] and OpToken:
                    break
                self._Idx += 1
                OpToken += Ch
            else:
                break

        if OpToken not in LegalOpLst:
            raise BadExpression(ERR_OPERATOR_UNSUPPORT % OpToken)
        self._Token = OpToken
        return OpToken

class ValueExpressionEx(ValueExpression):
    def __init__(self, PcdValue, PcdType, SymbolTable={}):
        ValueExpression.__init__(self, PcdValue, SymbolTable)
        self.PcdValue = PcdValue
        self.PcdType = PcdType

    def __call__(self, RealValue=False, Depth=0):
        PcdValue = self.PcdValue
        if "{CODE(" not in PcdValue:
            try:
                PcdValue = ValueExpression.__call__(self, RealValue, Depth)
                if self.PcdType == TAB_VOID and (PcdValue.startswith("'") or PcdValue.startswith("L'")):
                    PcdValue, Size = ParseFieldValue(PcdValue)
                    PcdValueList = []
                    for I in range(Size):
                        PcdValueList.append('0x%02X'%(PcdValue & 0xff))
                        PcdValue = PcdValue >> 8
                    PcdValue = '{' + ','.join(PcdValueList) + '}'
                elif self.PcdType in TAB_PCD_NUMERIC_TYPES and (PcdValue.startswith("'") or \
                          PcdValue.startswith('"') or PcdValue.startswith("L'") or PcdValue.startswith('L"') or PcdValue.startswith('{')):
                    raise BadExpression
            except WrnExpression as Value:
                PcdValue = Value.result
            except BadExpression as Value:
                if self.PcdType in TAB_PCD_NUMERIC_TYPES:
                    PcdValue = PcdValue.strip()
                    if PcdValue.startswith('{') and PcdValue.endswith('}'):
                        PcdValue = SplitPcdValueString(PcdValue[1:-1])
                    if isinstance(PcdValue, type([])):
                        TmpValue = 0
                        Size = 0
                        ValueType = ''
                        for Item in PcdValue:
                            Item = Item.strip()
                            if Item.startswith(TAB_UINT8):
                                ItemSize = 1
                                ValueType = TAB_UINT8
                            elif Item.startswith(TAB_UINT16):
                                ItemSize = 2
                                ValueType = TAB_UINT16
                            elif Item.startswith(TAB_UINT32):
                                ItemSize = 4
                                ValueType = TAB_UINT32
                            elif Item.startswith(TAB_UINT64):
                                ItemSize = 8
                                ValueType = TAB_UINT64
                            elif Item[0] in {'"', "'", 'L'}:
                                ItemSize = 0
                                ValueType = TAB_VOID
                            else:
                                ItemSize = 0
                                ValueType = TAB_UINT8
                            Item = ValueExpressionEx(Item, ValueType, self._Symb)(True)
                            if ItemSize == 0:
                                try:
                                    tmpValue = int(Item, 0)
                                    if tmpValue > 255:
                                        raise BadExpression("Byte  array number %s should less than 0xFF." % Item)
                                except BadExpression as Value:
                                    raise BadExpression(Value)
                                except ValueError:
                                    pass
                                ItemValue, ItemSize = ParseFieldValue(Item)
                            else:
                                ItemValue = ParseFieldValue(Item)[0]

                            if isinstance(ItemValue, type('')):
                                ItemValue = int(ItemValue, 0)

                            TmpValue = (ItemValue << (Size * 8)) | TmpValue
                            Size = Size + ItemSize
                    else:
                        try:
                            TmpValue, Size = ParseFieldValue(PcdValue)
                        except BadExpression as Value:
                            raise BadExpression("Type: %s, Value: %s, %s" % (self.PcdType, PcdValue, Value))
                    if isinstance(TmpValue, type('')):
                        try:
                            TmpValue = int(TmpValue)
                        except:
                            raise  BadExpression(Value)
                    else:
                        PcdValue = '0x%0{}X'.format(Size) % (TmpValue)
                    if TmpValue < 0:
                        raise  BadExpression('Type %s PCD Value is negative' % self.PcdType)
                    if self.PcdType == TAB_UINT8 and Size > 1:
                        raise BadExpression('Type %s PCD Value Size is Larger than 1 byte' % self.PcdType)
                    if self.PcdType == TAB_UINT16 and Size > 2:
                        raise BadExpression('Type %s PCD Value Size is Larger than 2 byte' % self.PcdType)
                    if self.PcdType == TAB_UINT32 and Size > 4:
                        raise BadExpression('Type %s PCD Value Size is Larger than 4 byte' % self.PcdType)
                    if self.PcdType == TAB_UINT64 and Size > 8:
                        raise BadExpression('Type %s PCD Value Size is Larger than 8 byte' % self.PcdType)
                else:
                    try:
                        TmpValue = int(PcdValue)
                        TmpList = []
                        if TmpValue.bit_length() == 0:
                            PcdValue = '{0x00}'
                        else:
                            for I in range((TmpValue.bit_length() + 7) // 8):
                                TmpList.append('0x%02x' % ((TmpValue >> I * 8) & 0xff))
                            PcdValue = '{' + ', '.join(TmpList) + '}'
                    except:
                        if PcdValue.strip().startswith('{'):
                            PcdValueList = SplitPcdValueString(PcdValue.strip()[1:-1])
                            LabelDict = {}
                            NewPcdValueList = []
                            LabelOffset = 0
                            for Item in PcdValueList:
                                # compute byte offset of every LABEL
                                LabelList = _ReLabel.findall(Item)
                                Item = _ReLabel.sub('', Item)
                                Item = Item.strip()
                                if LabelList:
                                    for Label in LabelList:
                                        if not IsValidCName(Label):
                                            raise BadExpression('%s is not a valid c variable name' % Label)
                                        if Label not in LabelDict:
                                            LabelDict[Label] = str(LabelOffset)
                                if Item.startswith(TAB_UINT8):
                                    LabelOffset = LabelOffset + 1
                                elif Item.startswith(TAB_UINT16):
                                    LabelOffset = LabelOffset + 2
                                elif Item.startswith(TAB_UINT32):
                                    LabelOffset = LabelOffset + 4
                                elif Item.startswith(TAB_UINT64):
                                    LabelOffset = LabelOffset + 8
                                else:
                                    try:
                                        ItemValue, ItemSize = ParseFieldValue(Item)
                                        LabelOffset = LabelOffset + ItemSize
                                    except:
                                        LabelOffset = LabelOffset + 1

                            for Item in PcdValueList:
                                # for LABEL parse
                                Item = Item.strip()
                                try:
                                    Item = _ReLabel.sub('', Item)
                                except:
                                    pass
                                try:
                                    OffsetList = _ReOffset.findall(Item)
                                except:
                                    pass
                                # replace each offset, except errors
                                for Offset in OffsetList:
                                    try:
                                        Item = Item.replace('OFFSET_OF({})'.format(Offset), LabelDict[Offset])
                                    except:
                                        raise BadExpression('%s not defined' % Offset)

                                NewPcdValueList.append(Item)

                            AllPcdValueList = []
                            for Item in NewPcdValueList:
                                Size = 0
                                ValueStr = ''
                                TokenSpaceGuidName = ''
                                if Item.startswith(TAB_GUID) and Item.endswith(')'):
                                    try:
                                        TokenSpaceGuidName = re.search('GUID\((\w+)\)', Item).group(1)
                                    except:
                                        pass
                                    if TokenSpaceGuidName and TokenSpaceGuidName in self._Symb:
                                        Item = 'GUID(' + self._Symb[TokenSpaceGuidName] + ')'
                                    elif TokenSpaceGuidName:
                                        raise BadExpression('%s not found in DEC file' % TokenSpaceGuidName)
                                    Item, Size = ParseFieldValue(Item)
                                    for Index in range(0, Size):
                                        ValueStr = '0x%02X' % (int(Item) & 255)
                                        Item >>= 8
                                        AllPcdValueList.append(ValueStr)
                                    continue
                                elif Item.startswith('DEVICE_PATH') and Item.endswith(')'):
                                    Item, Size = ParseFieldValue(Item)
                                    AllPcdValueList.append(Item[1:-1])
                                    continue
                                else:
                                    ValueType = ""
                                    if Item.startswith(TAB_UINT8):
                                        ItemSize = 1
                                        ValueType = TAB_UINT8
                                    elif Item.startswith(TAB_UINT16):
                                        ItemSize = 2
                                        ValueType = TAB_UINT16
                                    elif Item.startswith(TAB_UINT32):
                                        ItemSize = 4
                                        ValueType = TAB_UINT32
                                    elif Item.startswith(TAB_UINT64):
                                        ItemSize = 8
                                        ValueType = TAB_UINT64
                                    else:
                                        ItemSize = 0
                                    if ValueType:
                                        TmpValue = ValueExpressionEx(Item, ValueType, self._Symb)(True)
                                    else:
                                        TmpValue = ValueExpressionEx(Item, self.PcdType, self._Symb)(True)
                                    Item = '0x%x' % TmpValue if not isinstance(TmpValue, type('')) else TmpValue
                                    if ItemSize == 0:
                                        ItemValue, ItemSize = ParseFieldValue(Item)
                                        if Item[0] not in {'"', 'L', '{'} and ItemSize > 1:
                                            raise BadExpression("Byte  array number %s should less than 0xFF." % Item)
                                    else:
                                        ItemValue = ParseFieldValue(Item)[0]
                                    for I in range(0, ItemSize):
                                        ValueStr = '0x%02X' % (int(ItemValue) & 255)
                                        ItemValue >>= 8
                                        AllPcdValueList.append(ValueStr)
                                    Size += ItemSize

                            if Size > 0:
                                PcdValue = '{' + ', '.join(AllPcdValueList) + '}'
                        else:
                            raise  BadExpression("Type: %s, Value: %s, %s"%(self.PcdType, PcdValue, Value))

            if PcdValue == 'True':
                PcdValue = '1'
            if PcdValue == 'False':
                PcdValue = '0'

        if RealValue:
            return PcdValue

if __name__ == '__main__':
    pass
    while True:
        input = raw_input('Input expr: ')
        if input in 'qQ':
            break
        try:
            print(ValueExpression(input)(True))
            print(ValueExpression(input)(False))
        except WrnExpression as Ex:
            print(Ex.result)
            print(str(Ex))
        except Exception as Ex:
            print(str(Ex))
