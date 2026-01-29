## @file
# Rule object for generating FFS
#
#  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from CommonDataClass.FdfClass import RuleClassObject

## Rule base class
#
#
class Rule(RuleClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        RuleClassObject.__init__(self)
