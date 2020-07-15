## @file
#  Retrieves the people to request review from on submission of a commit.
#
#  Copyright (c) 2019, Linaro Ltd. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function
from collections import defaultdict
from collections import OrderedDict
import argparse
import os
import re
import SetupGit

EXPRESSIONS = {
    'exclude':    re.compile(r'^X:\s*(?P<exclude>.*?)\r*$'),
    'file':       re.compile(r'^F:\s*(?P<file>.*?)\r*$'),
    'list':       re.compile(r'^L:\s*(?P<list>.*?)\r*$'),
    'maintainer': re.compile(r'^M:\s*(?P<maintainer>.*<.*?>)\r*$'),
    'reviewer':   re.compile(r'^R:\s*(?P<reviewer>.*?)\r*$'),
    'status':     re.compile(r'^S:\s*(?P<status>.*?)\r*$'),
    'tree':       re.compile(r'^T:\s*(?P<tree>.*?)\r*$'),
    'webpage':    re.compile(r'^W:\s*(?P<webpage>.*?)\r*$')
}

def printsection(section):
    """Prints out the dictionary describing a Maintainers.txt section."""
    print('===')
    for key in section.keys():
        print("Key: %s" % key)
        for item in section[key]:
            print('  %s' % item)

def pattern_to_regex(pattern):
    """Takes a string containing regular UNIX path wildcards
       and returns a string suitable for matching with regex."""

    pattern = pattern.replace('.', r'\.')
    pattern = pattern.replace('?', r'.')
    pattern = pattern.replace('*', r'.*')

    if pattern.endswith('/'):
        pattern += r'.*'
    elif pattern.endswith('.*'):
        pattern = pattern[:-2]
        pattern += r'(?!.*?/.*?)'

    return pattern

def path_in_section(path, section):
    """Returns True of False indicating whether the path is covered by
       the current section."""
    if not 'file' in section:
        return False

    for pattern in section['file']:
        regex = pattern_to_regex(pattern)

        match = re.match(regex, path)
        if match:
            # Check if there is an exclude pattern that applies
            for pattern in section['exclude']:
                regex = pattern_to_regex(pattern)

                match = re.match(regex, path)
                if match:
                    return False

            return True

    return False

def get_section_maintainers(path, section):
    """Returns a list with email addresses to any M: and R: entries
       matching the provided path in the provided section."""
    maintainers = []
    lists = []
    nowarn_status = ['Supported', 'Maintained']

    if path_in_section(path, section):
        for status in section['status']:
            if status not in nowarn_status:
                print('WARNING: Maintained status for "%s" is \'%s\'!' % (path, status))
        for address in section['maintainer'], section['reviewer']:
            # Convert to list if necessary
            if isinstance(address, list):
                maintainers += address
            else:
                lists += [address]
        for address in section['list']:
            # Convert to list if necessary
            if isinstance(address, list):
                lists += address
            else:
                lists += [address]

    return maintainers, lists

def get_maintainers(path, sections, level=0):
    """For 'path', iterates over all sections, returning maintainers
       for matching ones."""
    maintainers = []
    lists = []
    for section in sections:
        tmp_maint, tmp_lists = get_section_maintainers(path, section)
        if tmp_maint:
            maintainers += tmp_maint
        if tmp_lists:
            lists += tmp_lists

    if not maintainers:
        # If no match found, look for match for (nonexistent) file
        # REPO.working_dir/<default>
        print('"%s": no maintainers found, looking for default' % path)
        if level == 0:
            maintainers = get_maintainers('<default>', sections, level=level + 1)
        else:
            print("No <default> maintainers set for project.")
        if not maintainers:
            return None

    return maintainers + lists

def parse_maintainers_line(line):
    """Parse one line of Maintainers.txt, returning any match group and its key."""
    for key, expression in EXPRESSIONS.items():
        match = expression.match(line)
        if match:
            return key, match.group(key)
    return None, None

def parse_maintainers_file(filename):
    """Parse the Maintainers.txt from top-level of repo and
       return a list containing dictionaries of all sections."""
    with open(filename, 'r') as text:
        line = text.readline()
        sectionlist = []
        section = defaultdict(list)
        while line:
            key, value = parse_maintainers_line(line)
            if key and value:
                section[key].append(value)

            line = text.readline()
            # If end of section (end of file, or non-tag line encountered)...
            if not key or not value or not line:
                # ...if non-empty, append section to list.
                if section:
                    sectionlist.append(section.copy())
                    section.clear()

        return sectionlist

def get_modified_files(repo, args):
    """Returns a list of the files modified by the commit specified in 'args'."""
    commit = repo.commit(args.commit)
    return commit.stats.files

if __name__ == '__main__':
    PARSER = argparse.ArgumentParser(
        description='Retrieves information on who to cc for review on a given commit')
    PARSER.add_argument('commit',
                        action="store",
                        help='git revision to examine (default: HEAD)',
                        nargs='?',
                        default='HEAD')
    PARSER.add_argument('-l', '--lookup',
                        help='Find section matches for path LOOKUP',
                        required=False)
    ARGS = PARSER.parse_args()

    REPO = SetupGit.locate_repo()

    CONFIG_FILE = os.path.join(REPO.working_dir, 'Maintainers.txt')

    SECTIONS = parse_maintainers_file(CONFIG_FILE)

    if ARGS.lookup:
        FILES = [ARGS.lookup]
    else:
        FILES = get_modified_files(REPO, ARGS)

    ADDRESSES = []

    for file in FILES:
        print(file)
        addresslist = get_maintainers(file, SECTIONS)
        if addresslist:
            ADDRESSES += addresslist

    for address in list(OrderedDict.fromkeys(ADDRESSES)):
        print('  %s' % address)
