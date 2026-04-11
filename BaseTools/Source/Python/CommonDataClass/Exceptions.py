## @file
# This file is used to define common Exceptions class used in python tools
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

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

