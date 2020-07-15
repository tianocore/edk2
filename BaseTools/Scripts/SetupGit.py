## @file
#  Set up the git configuration for contributing to TianoCore projects
#
#  Copyright (c) 2019, Linaro Ltd. All rights reserved.<BR>
#  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function
import argparse
import os.path
import re
import sys

try:
    import git
except ImportError:
    print('Unable to load gitpython module - please install and try again.')
    sys.exit(1)

try:
    # Try Python 2 'ConfigParser' module first since helpful lib2to3 will
    # otherwise automagically load it with the name 'configparser'
    import ConfigParser
except ImportError:
    # Otherwise, try loading the Python 3 'configparser' under an alias
    try:
        import configparser as ConfigParser
    except ImportError:
        print("Unable to load configparser/ConfigParser module - please install and try again!")
        sys.exit(1)


# Assumptions: Script is in edk2/BaseTools/Scripts,
#              templates in edk2/BaseTools/Conf
CONFDIR = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                       'Conf')

UPSTREAMS = [
    {'name': 'edk2',
     'repo': 'https://github.com/tianocore/edk2.git',
     'list': 'devel@edk2.groups.io'},
    {'name': 'edk2-platforms',
     'repo': 'https://github.com/tianocore/edk2-platforms.git',
     'list': 'devel@edk2.groups.io', 'prefix': 'edk2-platforms'},
    {'name': 'edk2-non-osi',
     'repo': 'https://github.com/tianocore/edk2-non-osi.git',
     'list': 'devel@edk2.groups.io', 'prefix': 'edk2-non-osi'}
    ]

# The minimum version required for all of the below options to work
MIN_GIT_VERSION = (1, 9, 0)

# Set of options to be set identically for all repositories
OPTIONS = [
    {'section': 'am',          'option': 'keepcr',            'value': True},
    {'section': 'am',          'option': 'signoff',           'value': True},
    {'section': 'cherry-pick', 'option': 'signoff',           'value': True},
    {'section': 'color',       'option': 'diff',              'value': True},
    {'section': 'color',       'option': 'grep',              'value': 'auto'},
    {'section': 'commit',      'option': 'signoff',           'value': True},
    {'section': 'core',        'option': 'abbrev',            'value': 12},
    {'section': 'core',        'option': 'attributesFile',
     'value': os.path.join(CONFDIR, 'gitattributes').replace('\\', '/')},
    {'section': 'core',        'option': 'whitespace',        'value': 'cr-at-eol'},
    {'section': 'diff',        'option': 'algorithm',         'value': 'patience'},
    {'section': 'diff',        'option': 'orderFile',
     'value': os.path.join(CONFDIR, 'diff.order').replace('\\', '/')},
    {'section': 'diff',        'option': 'renames',           'value': 'copies'},
    {'section': 'diff',        'option': 'statGraphWidth',    'value': '20'},
    {'section': 'diff "ini"',  'option': 'xfuncname',
     'value': '^\\\\[[A-Za-z0-9_., ]+]'},
    {'section': 'format',      'option': 'coverLetter',       'value': True},
    {'section': 'format',      'option': 'numbered',          'value': True},
    {'section': 'format',      'option': 'signoff',           'value': False},
    {'section': 'log',         'option': 'mailmap',           'value': True},
    {'section': 'notes',       'option': 'rewriteRef',        'value': 'refs/notes/commits'},
    {'section': 'sendemail',   'option': 'chainreplyto',      'value': False},
    {'section': 'sendemail',   'option': 'thread',            'value': True},
    {'section': 'sendemail',   'option': 'transferEncoding',  'value': '8bit'},
    ]


def locate_repo():
    """Opens a Repo object for the current tree, searching upwards in the directory hierarchy."""
    try:
        repo = git.Repo(path='.', search_parent_directories=True)
    except (git.InvalidGitRepositoryError, git.NoSuchPathError):
        print("It doesn't look like we're inside a git repository - aborting.")
        sys.exit(2)
    return repo


def fuzzy_match_repo_url(one, other):
    """Compares two repository URLs, ignoring protocol and optional trailing '.git'."""
    oneresult   = re.match(r'.*://(?P<oneresult>.*?)(\.git)*$', one)
    otherresult = re.match(r'.*://(?P<otherresult>.*?)(\.git)*$', other)

    if oneresult and otherresult:
        onestring = oneresult.group('oneresult')
        otherstring = otherresult.group('otherresult')
        if onestring == otherstring:
            return True

    return False


def get_upstream(url, name):
    """Extracts the dict for the current repo origin."""
    for upstream in UPSTREAMS:
        if (fuzzy_match_repo_url(upstream['repo'], url) or
                upstream['name'] == name):
            return upstream
    print("Unknown upstream '%s' - aborting!" % url)
    sys.exit(3)


def check_versions():
    """Checks versions of dependencies."""
    version = git.cmd.Git().version_info

    if version < MIN_GIT_VERSION:
        print('Need git version %d.%d or later!' % (version[0], version[1]))
        sys.exit(4)


def write_config_value(repo, section, option, data):
    """."""
    with repo.config_writer(config_level='repository') as configwriter:
        configwriter.set_value(section, option, data)


if __name__ == '__main__':
    check_versions()

    PARSER = argparse.ArgumentParser(
        description='Sets up a git repository according to TianoCore rules.')
    PARSER.add_argument('-c', '--check',
                        help='check current config only, printing what would be changed',
                        action='store_true',
                        required=False)
    PARSER.add_argument('-f', '--force',
                        help='overwrite existing settings conflicting with program defaults',
                        action='store_true',
                        required=False)
    PARSER.add_argument('-n', '--name', type=str, metavar='repo',
                        choices=['edk2', 'edk2-platforms', 'edk2-non-osi'],
                        help='set the repo name to configure for, if not '
                             'detected automatically',
                        required=False)
    PARSER.add_argument('-v', '--verbose',
                        help='enable more detailed output',
                        action='store_true',
                        required=False)
    ARGS = PARSER.parse_args()

    REPO = locate_repo()
    if REPO.bare:
        print('Bare repo - please check out an upstream one!')
        sys.exit(6)

    URL = REPO.remotes.origin.url

    UPSTREAM = get_upstream(URL, ARGS.name)
    if not UPSTREAM:
        print("Upstream '%s' unknown, aborting!" % URL)
        sys.exit(7)

    # Set a list email address if our upstream wants it
    if 'list' in UPSTREAM:
        OPTIONS.append({'section': 'sendemail', 'option': 'to',
                        'value': UPSTREAM['list']})
    # Append a subject prefix entry to OPTIONS if our upstream wants it
    if 'prefix' in UPSTREAM:
        OPTIONS.append({'section': 'format', 'option': 'subjectPrefix',
                        'value': "PATCH " + UPSTREAM['prefix']})

    CONFIG = REPO.config_reader(config_level='repository')

    for entry in OPTIONS:
        exists = False
        try:
            # Make sure to read boolean/int settings as real type rather than strings
            if isinstance(entry['value'], bool):
                value = CONFIG.getboolean(entry['section'], entry['option'])
            elif isinstance(entry['value'], int):
                value = CONFIG.getint(entry['section'], entry['option'])
            else:
                value = CONFIG.get(entry['section'], entry['option'])

            exists = True
        # Don't bail out from options not already being set
        except (ConfigParser.NoSectionError, ConfigParser.NoOptionError):
            pass

        if exists:
            if value == entry['value']:
                if ARGS.verbose:
                    print("%s.%s already set (to '%s')" % (entry['section'],
                                                           entry['option'], value))
            else:
                if ARGS.force:
                    write_config_value(REPO, entry['section'], entry['option'], entry['value'])
                else:
                    print("Not overwriting existing %s.%s value:" % (entry['section'],
                                                                     entry['option']))
                    print("  '%s' != '%s'" % (value, entry['value']))
                    print("  add '-f' to command line to force overwriting existing settings")
        else:
            print("%s.%s => '%s'" % (entry['section'], entry['option'], entry['value']))
            if not ARGS.check:
                write_config_value(REPO, entry['section'], entry['option'], entry['value'])
