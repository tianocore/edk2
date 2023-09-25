# @file globber.py
#
# Provides global functionality for use by the CodeQL plugin.
#
# Copyright 2019 Jaakko Kangasharju
#
#            Apache License
#      Version 2.0, January 2004
#   http://www.apache.org/licenses/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file has been altered from its original form. Based on code in:
#   https://github.com/advanced-security/filter-sarif
#
# Specifically:
#   https://github.com/advanced-security/filter-sarif/blob/main/filter_sarif.py
#
# It primarily contains modifications made to integrate with the CodeQL plugin.
#
# SPDX-License-Identifier: Apache-2.0
##

import re

_double_star_after_invalid_regex = re.compile(r'[^/\\]\*\*')
_double_star_first_before_invalid_regex = re.compile('^\\*\\*[^/]')
_double_star_middle_before_invalid_regex = re.compile(r'[^\\]\*\*[^/]')


def _match_component(pattern_component, file_name_component):
    if len(pattern_component) == 0 and len(file_name_component) == 0:
        return True
    elif len(pattern_component) == 0:
        return False
    elif len(file_name_component) == 0:
        return pattern_component == '*'
    elif pattern_component[0] == '*':
        return (_match_component(pattern_component, file_name_component[1:]) or
                _match_component(pattern_component[1:], file_name_component))
    elif pattern_component[0] == '?':
        return _match_component(pattern_component[1:], file_name_component[1:])
    elif pattern_component[0] == '\\':
        return (len(pattern_component) >= 2 and
                pattern_component[1] == file_name_component[0] and
                _match_component(
                    pattern_component[2:], file_name_component[1:]))
    elif pattern_component[0] != file_name_component[0]:
        return False
    else:
        return _match_component(pattern_component[1:], file_name_component[1:])


def _match_components(pattern_components, file_name_components):
    if len(pattern_components) == 0 and len(file_name_components) == 0:
        return True
    if len(pattern_components) == 0:
        return False
    if len(file_name_components) == 0:
        return len(pattern_components) == 1 and pattern_components[0] == '**'
    if pattern_components[0] == '**':
        return (_match_components(pattern_components, file_name_components[1:])
                or _match_components(
                    pattern_components[1:], file_name_components))
    else:
        return (
            _match_component(
                pattern_components[0], file_name_components[0]) and
            _match_components(
                pattern_components[1:], file_name_components[1:]))


def match(pattern: str, file_name: str):
    """Match a glob pattern against a file name.

    Glob pattern matching is for file names, which do not need to exist as
    files on the file system.

    A file name is a sequence of directory names, possibly followed by the name
    of a file, with the components separated by a path separator. A glob
    pattern is similar, except it may contain special characters: A '?' matches
    any character in a name. A '*' matches any sequence of characters (possibly
    empty) in a name. Both of these match only within a single component, i.e.,
    they will not match a path separator. A component in a pattern may also be
    a literal '**', which matches zero or more components in the complete file
    name. A backslash '\\' in a pattern acts as an escape character, and
    indicates that the following character is to be matched literally, even if
    it is a special character.

    Args:
        pattern (str): The pattern to match. The path separator in patterns is
                       always '/'.
        file_name (str): The file name to match against. The path separator in
                         file names is the platform separator

    Returns:
        bool: True if the pattern matches, False otherwise.
    """
    if (_double_star_after_invalid_regex.search(pattern) is not None or
        _double_star_first_before_invalid_regex.search(
            pattern) is not None or
        _double_star_middle_before_invalid_regex.search(pattern) is not None):
        raise ValueError(
            '** in {} not alone between path separators'.format(pattern))

    pattern = pattern.rstrip('/')
    file_name = file_name.rstrip('/')

    while '**/**' in pattern:
        pattern = pattern.replace('**/**', '**')

    pattern_components = pattern.split('/')

    # We split on '\' as well as '/' to support unix and windows-style paths
    file_name_components = re.split(r'[\\/]', file_name)

    return _match_components(pattern_components, file_name_components)
