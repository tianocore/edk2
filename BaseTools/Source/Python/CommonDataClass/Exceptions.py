## @file
# This file is used to define common Exceptions class used in python tools
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

## Exceptions used in Expression
class EvaluationException(Exception):
    pass

class BadExpression(EvaluationException):
    pass

class WrnExpression(Exception):
    pass

## Exceptions used in macro replacements
class MacroException(Exception):
    pass

class SymbolNotFound(MacroException):
    pass

