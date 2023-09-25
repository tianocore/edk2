# @file analyze_filter.py
#
# Filters results in a SARIF file.
#
#            Apache License
#      Version 2.0, January 2004
#   http://www.apache.org/licenses/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
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
# It primarily contains modifications made to integrate with the CodeQL plugin.
#
# Specifically:
#   https://github.com/advanced-security/filter-sarif/blob/main/filter_sarif.py
#
# View the full and complete license as provided by that repository here:
#   https://github.com/advanced-security/filter-sarif/blob/main/LICENSE
#
# SPDX-License-Identifier: Apache-2.0
##

import json
import logging
import re
from os import PathLike
from typing import Iterable, List, Tuple

from analyze.globber import match


def _match_path_and_rule(
    path: str, rule: str, patterns: Iterable[str]) -> bool:
    """Returns whether a given path matches a given rule.

    Args:
        path (str): A file path string.
        rule (str): A rule file path string.
        patterns (Iterable[str]): An iterable of pattern strings.

    Returns:
        bool: True if the path matches a rule. Otherwise, False.
    """
    result = True
    for s, fp, rp in patterns:
        if match(rp, rule) and match(fp, path):
            result = s
    return result


def _parse_pattern(line: str) -> Tuple[str]:
    """Parses a given pattern line.

    Args:
        line (str): The line string that contains the rule.

    Returns:
        Tuple[str]: The parsed sign, file pattern, and rule pattern from the
                    line.
    """
    sep_char = ':'
    esc_char = '\\'
    file_pattern = ''
    rule_pattern = ''
    seen_separator = False
    sign = True

    # inclusion or exclusion pattern?
    u_line = line
    if line:
        if line[0] == '-':
            sign = False
            u_line = line[1:]
        elif line[0] == '+':
            u_line = line[1:]

    i = 0
    while i < len(u_line):
        c = u_line[i]
        i = i + 1
        if c == sep_char:
            if seen_separator:
                raise Exception(
                    'Invalid pattern: "' + line + '" Contains more than one '
                    'separator!')
            seen_separator = True
            continue
        elif c == esc_char:
            next_c = u_line[i] if (i < len(u_line)) else None
            if next_c in ['+' , '-', esc_char, sep_char]:
                i = i + 1
                c = next_c
        if seen_separator:
            rule_pattern = rule_pattern + c
        else:
            file_pattern = file_pattern + c

    if not rule_pattern:
        rule_pattern = '**'

    return sign, file_pattern, rule_pattern


def filter_sarif(input_sarif: PathLike,
                 output_sarif: PathLike,
                 patterns: List[str],
                 split_lines: bool) -> None:
    """Filters a SARIF file with a given set of filter patterns.

    Args:
        input_sarif (PathLike): Input SARIF file path.
        output_sarif (PathLike): Output SARIF file path.
        patterns (PathLike): List of filter pattern strings.
        split_lines (PathLike): Whether to split lines in individual patterns.
    """
    if split_lines:
        tmp = []
        for p in patterns:
            tmp = tmp + re.split('\r?\n', p)
        patterns = tmp

    patterns = [_parse_pattern(p) for p in patterns if p]

    logging.debug('Given patterns:')
    for s, fp, rp in patterns:
        logging.debug(
            'files: {file_pattern}    rules: {rule_pattern} ({sign})'.format(
                file_pattern=fp,
                rule_pattern=rp,
                sign='positive' if s else 'negative'))

    with open(input_sarif, 'r') as f:
        s = json.load(f)

    for run in s.get('runs', []):
        if run.get('results', []):
            new_results = []
            for r in run['results']:
                if r.get('locations', []):
                    new_locations = []
                    for l in r['locations']:
                        # TODO: The uri field is optional. We might have to
                        #       fetch the actual uri from "artifacts" via
                        #       "index"
                        # (see https://github.com/microsoft/sarif-tutorials/blob/main/docs/2-Basics.md#-linking-results-to-artifacts)
                        uri = l.get(
                                    'physicalLocation', {}).get(
                                        'artifactLocation', {}).get(
                                            'uri', None)

                        # TODO: The ruleId field is optional and potentially
                        #       ambiguous. We might have to fetch the actual
                        #       ruleId from the rule metadata via the ruleIndex
                        #       field.
                        # (see https://github.com/microsoft/sarif-tutorials/blob/main/docs/2-Basics.md#rule-metadata)
                        ruleId = r['ruleId']

                        if (uri is None or
                            _match_path_and_rule(uri, ruleId, patterns)):
                            new_locations.append(l)
                    r['locations'] = new_locations
                    if new_locations:
                        new_results.append(r)
                else:
                    # locations array doesn't exist or is empty, so we can't
                    # match on anything. Therefore, we include the result in
                    # the output.
                    new_results.append(r)
            run['results'] = new_results

    with open(output_sarif, 'w') as f:
        json.dump(s, f, indent=2)
