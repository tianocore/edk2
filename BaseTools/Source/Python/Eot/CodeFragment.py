## @file
# fragments of source file
#
#  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#


## The description of comment contents and start & end position
#
#
class Comment :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #   @param  CommentType The type of comment (T_COMMENT_TWO_SLASH or T_COMMENT_SLASH_STAR).
    #
    def __init__(self, Str, Begin, End, CommentType):
        self.Content = Str
        self.StartPos = Begin
        self.EndPos = End
        self.Type = CommentType

## The description of preprocess directives and start & end position
#
#
class PP_Directive :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #
    def __init__(self, Str, Begin, End):
        self.Content = Str
        self.StartPos = Begin
        self.EndPos = End

## The description of assignment expression and start & end position
#
#
class AssignmentExpression :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #
    def __init__(self, Lvalue, Op, Exp, Begin, End):
        self.Name = Lvalue
        self.Operator = Op
        self.Value = Exp
        self.StartPos = Begin
        self.EndPos = End

## The description of predicate expression and start & end position
#
#
class PredicateExpression :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #
    def __init__(self, Str, Begin, End):
        self.Content = Str
        self.StartPos = Begin
        self.EndPos = End

## The description of function definition and start & end position
#
#
class FunctionDefinition :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #   @param  LBPos       The left brace position tuple.
    #
    def __init__(self, ModifierStr, DeclStr, Begin, End, LBPos, NamePos):
        self.Modifier = ModifierStr
        self.Declarator = DeclStr
        self.StartPos = Begin
        self.EndPos = End
        self.LeftBracePos = LBPos
        self.NamePos = NamePos

## The description of variable declaration and start & end position
#
#
class VariableDeclaration :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #
    def __init__(self, ModifierStr, DeclStr, Begin, End):
        self.Modifier = ModifierStr
        self.Declarator = DeclStr
        self.StartPos = Begin
        self.EndPos = End

## The description of enum definition and start & end position
#
#
class EnumerationDefinition :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #
    def __init__(self, Str, Begin, End):
        self.Content = Str
        self.StartPos = Begin
        self.EndPos = End

## The description of struct/union definition and start & end position
#
#
class StructUnionDefinition :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #
    def __init__(self, Str, Begin, End):
        self.Content = Str
        self.StartPos = Begin
        self.EndPos = End

## The description of 'Typedef' definition and start & end position
#
#
class TypedefDefinition :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #
    def __init__(self, FromStr, ToStr, Begin, End):
        self.FromType = FromStr
        self.ToType = ToStr
        self.StartPos = Begin
        self.EndPos = End

## The description of function calling definition and start & end position
#
#
class FunctionCalling:
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  Begin       The start position tuple.
    #   @param  End         The end position tuple.
    #
    def __init__(self, Name, Param, Begin, End):
        self.FuncName = Name
        self.ParamList = Param
        self.StartPos = Begin
        self.EndPos = End
