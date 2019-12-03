## @file
# Simple Rule object for generating FFS
#
#  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from . import Rule
from CommonDataClass.FdfClass import RuleSimpleFileClassObject

## simple rule
#
#
class RuleSimpleFile (RuleSimpleFileClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        RuleSimpleFileClassObject.__init__(self)
