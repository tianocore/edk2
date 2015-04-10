# # @file
# This file is used to parse and evaluate range expression in Pcd declaration.
#
# Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

# # Import Modules
#
from Common.GlobalData import *
from CommonDataClass.Exceptions import BadExpression
from CommonDataClass.Exceptions import WrnExpression
import uuid

ERR_STRING_EXPR = 'This operator cannot be used in string expression: [%s].'
ERR_SNYTAX = 'Syntax error, the rest of expression cannot be evaluated: [%s].'
ERR_MATCH = 'No matching right parenthesis.'
ERR_STRING_TOKEN = 'Bad string token: [%s].'
ERR_MACRO_TOKEN = 'Bad macro token: [%s].'
ERR_EMPTY_TOKEN = 'Empty token is not allowed.'
ERR_PCD_RESOLVE = 'PCD token cannot be resolved: [%s].'
ERR_VALID_TOKEN = 'No more valid token found from rest of string: [%s].'
ERR_EXPR_TYPE = 'Different types found in expression.'
ERR_OPERATOR_UNSUPPORT = 'Unsupported operator: [%s]'
ERR_REL_NOT_IN = 'Expect "IN" after "not" operator.'
WRN_BOOL_EXPR = 'Operand of boolean type cannot be used in arithmetic expression.'
WRN_EQCMP_STR_OTHERS = '== Comparison between Operand of string type and Boolean/Number Type always return False.'
WRN_NECMP_STR_OTHERS = '!= Comparison between Operand of string type and Boolean/Number Type always return True.'
ERR_RELCMP_STR_OTHERS = 'Operator taking Operand of string type and Boolean/Number Type is not allowed: [%s].'
ERR_STRING_CMP = 'Unicode string and general string cannot be compared: [%s %s %s]'
ERR_ARRAY_TOKEN = 'Bad C array or C format GUID token: [%s].'
ERR_ARRAY_ELE = 'This must be HEX value for NList or Array: [%s].'
ERR_EMPTY_EXPR = 'Empty expression is not allowed.'
ERR_IN_OPERAND = 'Macro after IN operator can only be: $(FAMILY), $(ARCH), $(TOOL_CHAIN_TAG) and $(TARGET).'

def MaxOfType(DataType):
    if DataType == 'UINT8':
        return int('0xFF', 16)
    if DataType == 'UINT16':
        return int('0xFFFF', 16)
    if DataType == 'UINT32':
        return int('0xFFFFFFFF', 16)
    if DataType == 'UINT64':
        return int('0xFFFFFFFFFFFFFFFF', 16)

class RangeObject(object):
    def __init__(self, start, end, empty = False):
        
        if int(start) < int(end):
            self.start = int(start)
            self.end = int(end)
        else:
            self.start = int(end)
            self.end = int(start)
        self.empty = empty

class RangeContainer(object):
    def __init__(self):
        self.rangelist = []
        
    def push(self, RangeObject):
        self.rangelist.append(RangeObject)
        self.rangelist = sorted(self.rangelist, key = lambda rangeobj : rangeobj.start)
        self.merge()
        
    def pop(self):
        for item in self.rangelist:
            yield item
   
    def __clean__(self):   
        newrangelist = []
        for rangeobj in self.rangelist:
            if rangeobj.empty == True:
                continue
            else:
                newrangelist.append(rangeobj)
        self.rangelist = newrangelist      
    def merge(self):
        self.__clean__()
        for i in range(0, len(self.rangelist) - 1):
            if self.rangelist[i + 1].start > self.rangelist[i].end:
                continue
            else:
                self.rangelist[i + 1].start = self.rangelist[i].start
                self.rangelist[i + 1].end = self.rangelist[i + 1].end > self.rangelist[i].end and self.rangelist[i + 1].end or self.rangelist[i].end 
                self.rangelist[i].empty = True

        self.__clean__()
        
    def dump(self):
        print "----------------------"
        rangelist = ""
        for object in self.rangelist:
            rangelist = rangelist + "[%d , %d]" % (object.start, object.end)
        print rangelist
        
        
class XOROperatorObject(object):   
    def __init__(self):     
        pass
    def Calculate(self, Operand, DataType, SymbolTable): 
        if type(Operand) == type('') and not Operand.isalnum():
            Expr = "XOR ..."
            raise BadExpression(ERR_SNYTAX % Expr)
        rangeId = str(uuid.uuid1())
        rangeContainer = RangeContainer()
        rangeContainer.push(RangeObject(0, int(Operand) - 1))
        rangeContainer.push(RangeObject(int(Operand) + 1, MaxOfType(DataType)))
        SymbolTable[rangeId] = rangeContainer
        return rangeId

class LEOperatorObject(object):
    def __init__(self):     
        pass
    def Calculate(self, Operand, DataType, SymbolTable): 
        if type(Operand) == type('') and not Operand.isalnum():
            Expr = "LE ..."
            raise BadExpression(ERR_SNYTAX % Expr)
        rangeId1 = str(uuid.uuid1())
        rangeContainer = RangeContainer()
        rangeContainer.push(RangeObject(0, int(Operand)))
        SymbolTable[rangeId1] = rangeContainer
        return rangeId1
class LTOperatorObject(object):
    def __init__(self):     
        pass
    def Calculate(self, Operand, DataType, SymbolTable):
        if type(Operand) == type('') and not Operand.isalnum():
            Expr = "LT ..." 
            raise BadExpression(ERR_SNYTAX % Expr) 
        rangeId1 = str(uuid.uuid1())
        rangeContainer = RangeContainer()
        rangeContainer.push(RangeObject(0, int(Operand) - 1))
        SymbolTable[rangeId1] = rangeContainer
        return rangeId1   

class GEOperatorObject(object):
    def __init__(self):     
        pass
    def Calculate(self, Operand, DataType, SymbolTable): 
        if type(Operand) == type('') and not Operand.isalnum():
            Expr = "GE ..."
            raise BadExpression(ERR_SNYTAX % Expr)
        rangeId1 = str(uuid.uuid1())
        rangeContainer = RangeContainer()
        rangeContainer.push(RangeObject(int(Operand), MaxOfType(DataType)))
        SymbolTable[rangeId1] = rangeContainer
        return rangeId1   
      
class GTOperatorObject(object):
    def __init__(self):     
        pass
    def Calculate(self, Operand, DataType, SymbolTable): 
        if type(Operand) == type('') and not Operand.isalnum():
            Expr = "GT ..."
            raise BadExpression(ERR_SNYTAX % Expr)
        rangeId1 = str(uuid.uuid1())
        rangeContainer = RangeContainer()
        rangeContainer.push(RangeObject(int(Operand) + 1, MaxOfType(DataType)))
        SymbolTable[rangeId1] = rangeContainer
        return rangeId1   
    
class EQOperatorObject(object):
    def __init__(self):     
        pass
    def Calculate(self, Operand, DataType, SymbolTable): 
        if type(Operand) == type('') and not Operand.isalnum():
            Expr = "EQ ..."
            raise BadExpression(ERR_SNYTAX % Expr)
        rangeId1 = str(uuid.uuid1())
        rangeContainer = RangeContainer()
        rangeContainer.push(RangeObject(int(Operand) , int(Operand)))
        SymbolTable[rangeId1] = rangeContainer
        return rangeId1   
    
def GetOperatorObject(Operator):
    if Operator == '>':
        return GTOperatorObject()
    elif Operator == '>=':
        return GEOperatorObject()
    elif Operator == '<':
        return LTOperatorObject()
    elif Operator == '<=':
        return LEOperatorObject()
    elif Operator == '==':
        return EQOperatorObject()
    elif Operator == '^':
        return XOROperatorObject()
    else:
        raise BadExpression("Bad Operator")

class RangeExpression(object):
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

    NonLetterOpLst = ['+', '-', '&', '|', '^', '!', '=', '>', '<']

    PcdPattern = re.compile(r'[_a-zA-Z][0-9A-Za-z_]*\.[_a-zA-Z][0-9A-Za-z_]*$')
    HexPattern = re.compile(r'0[xX][0-9a-fA-F]+')
    RegGuidPattern = re.compile(r'[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}')
    ExRegGuidPattern = re.compile(r'[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$')
    
    SymbolPattern = re.compile("("
                                 "\$\([A-Z][A-Z0-9_]*\)|\$\(\w+\.\w+\)|\w+\.\w+|"
                                 "&&|\|\||!(?!=)|"
                                 "(?<=\W)AND(?=\W)|(?<=\W)OR(?=\W)|(?<=\W)NOT(?=\W)|(?<=\W)XOR(?=\W)|"
                                 "(?<=\W)EQ(?=\W)|(?<=\W)NE(?=\W)|(?<=\W)GT(?=\W)|(?<=\W)LT(?=\W)|(?<=\W)GE(?=\W)|(?<=\W)LE(?=\W)"
                               ")")
    
    RangePattern = re.compile(r'[0-9]+ - [0-9]+')

    def preProcessRangeExpr(self, expr):
        # convert hex to int
        # convert interval to object index. ex. 1 - 10 to a GUID
        expr = expr.strip()
        NumberDict = {}
        for HexNumber in self.HexPattern.findall(expr):
            Number = str(int(HexNumber, 16))
            NumberDict[HexNumber] = Number
        for HexNum in NumberDict:
            expr = expr.replace(HexNum, NumberDict[HexNum])
        
        rangedict = {}    
        for validrange in self.RangePattern.findall(expr):
            start, end = validrange.split(" - ")
            start = start.strip()
            end = end.strip()
            rangeid = str(uuid.uuid1())
            rangeContainer = RangeContainer()
            rangeContainer.push(RangeObject(start, end))
            self.operanddict[str(rangeid)] = rangeContainer
            rangedict[validrange] = str(rangeid)
            
        for validrange in rangedict:
            expr = expr.replace(validrange, rangedict[validrange])
         
        self._Expr = expr    
        return expr
            
        
    def EvalRange(self, Operator, Oprand):

        operatorobj = GetOperatorObject(Operator)
        return operatorobj.Calculate(Oprand, self.PcdDataType, self.operanddict)
        
    def Rangeintersection(self, Oprand1, Oprand2):
        rangeContainer1 = self.operanddict[Oprand1]
        rangeContainer2 = self.operanddict[Oprand2]
        rangeContainer = RangeContainer()
        for range1 in rangeContainer1.pop():
            for range2 in rangeContainer2.pop():
                if range1.start >= range2.start:
                    start = range1.start
                    end = range1.end
                    range1.start = range2.start
                    range1.end = range2.end
                    range2.start = start
                    range2.end = end
                if range1.empty:
                    rangeid = str(uuid.uuid1())
                    rangeContainer.push(RangeObject(0, 0, True))
                if range1.end < range2.start:
                    rangeid = str(uuid.uuid1())
                    rangeContainer.push(RangeObject(0, 0, True))
                elif range1.end == range2.start:
                    rangeid = str(uuid.uuid1())
                    rangeContainer.push(RangeObject(range1.end, range1.end))
                elif range1.end <= range2.end and range1.end > range2.start:
                    rangeid = str(uuid.uuid1())
                    rangeContainer.push(RangeObject(range2.start, range1.end))
                elif range1.end >= range2.end:
                    rangeid = str(uuid.uuid1())
                    rangeContainer.push(RangeObject(range2.start, range2.end))
        
        self.operanddict[rangeid] = rangeContainer
#        rangeContainer.dump()
        return rangeid
            
    def Rangecollections(self, Oprand1, Oprand2):

        rangeContainer1 = self.operanddict[Oprand1]
        rangeContainer2 = self.operanddict[Oprand2]
        rangeContainer = RangeContainer()
        
        for rangeobj in rangeContainer2.pop():
            rangeContainer.push(rangeobj)
        for rangeobj in rangeContainer1.pop():
            rangeContainer.push(rangeobj)
        
        rangeid = str(uuid.uuid1())
        self.operanddict[rangeid] = rangeContainer
        
#        rangeContainer.dump()
        return rangeid
        
            
    def NegtiveRange(self, Oprand1):
        rangeContainer1 = self.operanddict[Oprand1]
        
        
        rangeids = []
        
        for rangeobj in rangeContainer1.pop():
            rangeContainer = RangeContainer()
            rangeid = str(uuid.uuid1())
            if rangeobj.empty:
                rangeContainer.push(RangeObject(0, MaxOfType(self.PcdDataType)))
            else:
                if rangeobj.start > 0:
                    rangeContainer.push(RangeObject(0, rangeobj.start - 1))
                if rangeobj.end < MaxOfType(self.PcdDataType):
                    rangeContainer.push(RangeObject(rangeobj.end + 1, MaxOfType(self.PcdDataType)))
            self.operanddict[rangeid] = rangeContainer
            rangeids.append(rangeid)

        if len(rangeids) == 0:
            rangeContainer = RangeContainer()
            rangeContainer.push(RangeObject(0, MaxOfType(self.PcdDataType)))
            rangeid = str(uuid.uuid1())
            self.operanddict[rangeid] = rangeContainer
            return rangeid

        if len(rangeids) == 1:
            return rangeids[0]

        re = self.Rangeintersection(rangeids[0], rangeids[1])
        for i in range(2, len(rangeids)):
            re = self.Rangeintersection(re, rangeids[i])
            
        rangeid2 = str(uuid.uuid1())
        self.operanddict[rangeid2] = self.operanddict[re]
        return rangeid2
        
    def Eval(self, Operator, Oprand1, Oprand2 = None):
        
        if Operator in ["!", "NOT", "not"]:
            if not self.RegGuidPattern.match(Oprand1.strip()):
                raise BadExpression(ERR_STRING_EXPR % Operator)
            return self.NegtiveRange(Oprand1)
        else:
            if Operator in ["==", ">=", "<=", ">", "<", '^']:
                return self.EvalRange(Operator, Oprand1)
            elif Operator == 'and' :
                if not self.ExRegGuidPattern.match(Oprand1.strip()) or not self.ExRegGuidPattern.match(Oprand2.strip()):
                    raise BadExpression(ERR_STRING_EXPR % Operator)
                return self.Rangeintersection(Oprand1, Oprand2)    
            elif Operator == 'or':
                if not self.ExRegGuidPattern.match(Oprand1.strip()) or not self.ExRegGuidPattern.match(Oprand2.strip()):
                    raise BadExpression(ERR_STRING_EXPR % Operator)
                return self.Rangecollections(Oprand1, Oprand2)
            else:
                raise BadExpression(ERR_STRING_EXPR % Operator)


    def __init__(self, Expression, PcdDataType, SymbolTable = {}):
        self._NoProcess = False
        if type(Expression) != type(''):
            self._Expr = Expression
            self._NoProcess = True
            return

        self._Expr = Expression.strip()

        if not self._Expr.strip():
            raise BadExpression(ERR_EMPTY_EXPR)

        #
        # The symbol table including PCD and macro mapping
        #
        self._Symb = SymbolTable
        self._Symb.update(self.LogicalOperators)
        self._Idx = 0
        self._Len = len(self._Expr)
        self._Token = ''
        self._WarnExcept = None
        

        # Literal token without any conversion
        self._LiteralToken = ''
        
        # store the operand object
        self.operanddict = {}
        # The Pcd max value depends on PcdDataType
        self.PcdDataType = PcdDataType

    # Public entry for this class
    #   @param RealValue: False: only evaluate if the expression is true or false, used for conditional expression
    #                     True : return the evaluated str(value), used for PCD value
    #
    #   @return: True or False if RealValue is False
    #            Evaluated value of string format if RealValue is True
    #
    def __call__(self, RealValue = False, Depth = 0):
        if self._NoProcess:
            return self._Expr

        self._Depth = Depth

        self._Expr = self._Expr.strip()
        
        self.preProcessRangeExpr(self._Expr)
        
        # check if the expression does not need to evaluate
        if RealValue and Depth == 0:
            self._Token = self._Expr
            if self.ExRegGuidPattern.match(self._Expr):
                return [self.operanddict[self._Expr] ]

            self._Idx = 0
            self._Token = ''

        Val = self._OrExpr()
        RealVal = Val
        
        RangeIdList = RealVal.split("or")
        RangeList = []
        for rangeid in RangeIdList:
            RangeList.append(self.operanddict[rangeid.strip()])
            
        return RangeList

    # Template function to parse binary operators which have same precedence
    # Expr [Operator Expr]*
    def _ExprFuncTemplate(self, EvalFunc, OpLst):
        Val = EvalFunc()
        while self._IsOperator(OpLst):
            Op = self._Token
            try:
                Val = self.Eval(Op, Val, EvalFunc())
            except WrnExpression, Warn:
                self._WarnExcept = Warn
                Val = Warn.result
        return Val

    # A [|| B]*
    def _OrExpr(self):
        return self._ExprFuncTemplate(self._AndExpr, ["OR", "or"])

    # A [&& B]*
    def _AndExpr(self):
        return self._ExprFuncTemplate(self._NeExpr, ["AND", "and"])

    def _NeExpr(self):
        Val = self._RelExpr()
        while self._IsOperator([ "!=", "NOT", "not"]):
            Op = self._Token
            if Op in ["!", "NOT", "not"]:
                if not self._IsOperator(["IN", "in"]):
                    raise BadExpression(ERR_REL_NOT_IN)
                Op += ' ' + self._Token
            try:
                Val = self.Eval(Op, Val, self._RelExpr())
            except WrnExpression, Warn:
                self._WarnExcept = Warn
                Val = Warn.result
        return Val

    # [!]*A
    def _RelExpr(self):
        if self._IsOperator(["NOT" , "LE", "GE", "LT", "GT", "EQ", "XOR"]):
            Token = self._Token
            Val = self._NeExpr()
            try:
                return self.Eval(Token, Val)
            except WrnExpression, Warn:
                self._WarnExcept = Warn
                return Warn.result
        return self._IdenExpr()

    # Parse identifier or encapsulated expression
    def _IdenExpr(self):
        Tk = self._GetToken()
        if Tk == '(':
            Val = self._OrExpr()
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
            HexList = Token.split(',')
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
        Expr = self._Expr[self._Idx:].replace('\\\\', '//').replace('\\\"', '\\\'')
        for Ch in Expr:
            self._Idx += 1
            if Ch == '"':
                break
        self._Token = self._LiteralToken = self._Expr[Idx:self._Idx]
        if not self._Token.endswith('"'):
            raise BadExpression(ERR_STRING_TOKEN % self._Token)
        self._Token = self._Token[1:-1]
        return self._Token

    # Get token that is comprised by alphanumeric, underscore or dot(used by PCD)
    # @param IsAlphaOp: Indicate if parsing general token or script operator(EQ, NE...)
    def __GetIdToken(self, IsAlphaOp = False):
        IdToken = ''
        for Ch in self._Expr[self._Idx:]:
            if not self.__IsIdChar(Ch):
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
        if self.PcdPattern.match(self._Token):
            if self._Token not in self._Symb:
                Ex = BadExpression(ERR_PCD_RESOLVE % self._Token)
                Ex.Pcd = self._Token
                raise Ex
            self._Token = RangeExpression(self._Symb[self._Token], self._Symb)(True, self._Depth + 1)
            if type(self._Token) != type(''):
                self._LiteralToken = hex(self._Token)
                return

        if self._Token.startswith('"'):
            self._Token = self._Token[1:-1]
        elif self._Token in ["FALSE", "false", "False"]:
            self._Token = False
        elif self._Token in ["TRUE", "true", "True"]:
            self._Token = True
        else:
            self.__IsNumberToken()

    def __GetNList(self, InArray = False):
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

        if self.HexPattern.match(self._LiteralToken):
            Token = self._LiteralToken[2:]
            Token = Token.lstrip('0')
            if not Token:
                self._LiteralToken = '0x0'
            else:
                self._LiteralToken = '0x' + Token.lower()
            return True
        return False

    def _GetToken(self):
        return self.__GetNList()

    @staticmethod
    def __IsIdChar(Ch):
        return Ch in '._/:' or Ch.isalnum()

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

        self._Token = ''
        if Expr:
            Ch = Expr[0]
            Match = self.RegGuidPattern.match(Expr)
            if Match and not Expr[Match.end():Match.end() + 1].isalnum() \
                and Expr[Match.end():Match.end() + 1] != '_':
                self._Idx += Match.end()
                self._Token = Expr[0:Match.end()]
                return self._Token
            elif self.__IsIdChar(Ch):
                return self.__GetIdToken()
            elif Ch == '(' or Ch == ')':
                self._Idx += 1
                self._Token = Ch
                return self._Token

        raise BadExpression(ERR_VALID_TOKEN % Expr)

    # Parse operator
    def _GetOperator(self):
        self.__SkipWS()
        LegalOpLst = ['&&', '||', '!=', '==', '>=', '<='] + self.NonLetterOpLst

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
                if '!' == Ch and OpToken:
                    break
                self._Idx += 1
                OpToken += Ch
            else:
                break

        if OpToken not in LegalOpLst:
            raise BadExpression(ERR_OPERATOR_UNSUPPORT % OpToken)
        self._Token = OpToken
        return OpToken

    # Check if current token matches the operators given from OpList
    def _IsOperator(self, OpList):
        Idx = self._Idx
        self._GetOperator()
        if self._Token in OpList:
            if self._Token in self.LogicalOperators:
                self._Token = self.LogicalOperators[self._Token]
            return True
        self._Idx = Idx
        return False


    
    
    
    




#    UTRangeList()
