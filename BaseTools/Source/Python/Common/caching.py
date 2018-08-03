## @file
# help with caching in BaseTools
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

## Import Modules
#

# for class function
class cached_class_function(object):
    def __init__(self, function):
        self._function = function
    def __get__(self, obj, cls):
        def CallMeHere(*args,**kwargs):
            Value = self._function(obj, *args,**kwargs)
            obj.__dict__[self._function.__name__] = lambda *args,**kwargs:Value
            return Value
        return CallMeHere

# for class property
class cached_property(object):
    def __init__(self, function):
        self._function = function
    def __get__(self, obj, cls):
        Value = obj.__dict__[self._function.__name__] = self._function(obj)
        return Value

# for non-class function
class cached_basic_function(object):
    def __init__(self, function):
        self._function = function
    # wrapper to call _do since <class>.__dict__ doesn't support changing __call__
    def __call__(self,*args,**kwargs):
        return self._do(*args,**kwargs)
    def _do(self,*args,**kwargs):
        Value = self._function(*args,**kwargs)
        self.__dict__['_do'] = lambda self,*args,**kwargs:Value
        return Value
