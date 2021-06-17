## @file
#  Check a patch for various format issues
#
#  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
#  Copyright (C) 2020, Red Hat, Inc.<BR>
#  Copyright (c) 2020, ARM Ltd. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function

VersionNumber = '0.1'
__copyright__ = "Copyright (c) 2015 - 2016, Intel Corporation  All rights reserved."

import email
import argparse
import os
import re
import subprocess
import sys

import email.header

class Verbose:
    SILENT, ONELINE, NORMAL = range(3)
    level = NORMAL

class EmailAddressCheck:
    """Checks an email address."""

    def __init__(self, email, description):
        self.ok = True

        if email is None:
            self.error('Email address is missing!')
            return
        if description is None:
            self.error('Email description is missing!')
            return

        self.description = "'" + description + "'"
        self.check_email_address(email)

    def error(self, *err):
        if self.ok and Verbose.level > Verbose.ONELINE:
            print('The ' + self.description + ' email address is not valid:')
        self.ok = False
        if Verbose.level < Verbose.NORMAL:
            return
        count = 0
        for line in err:
            prefix = (' *', '  ')[count > 0]
            print(prefix, line)
            count += 1

    email_re1 = re.compile(r'(?:\s*)(.*?)(\s*)<(.+)>\s*$',
                           re.MULTILINE|re.IGNORECASE)

    def check_email_address(self, email):
        email = email.strip()
        mo = self.email_re1.match(email)
        if mo is None:
            self.error("Email format is invalid: " + email.strip())
            return

        name = mo.group(1).strip()
        if name == '':
            self.error("Name is not provided with email address: " +
                       email)
        else:
            quoted = len(name) > 2 and name[0] == '"' and name[-1] == '"'
            if name.find(',') >= 0 and not quoted:
                self.error('Add quotes (") around name with a comma: ' +
                           name)

        if mo.group(2) == '':
            self.error("There should be a space between the name and " +
                       "email address: " + email)

        if mo.group(3).find(' ') >= 0:
            self.error("The email address cannot contain a space: " +
                       mo.group(3))

        if ' via Groups.Io' in name and mo.group(3).endswith('@groups.io'):
            self.error("Email rewritten by lists DMARC / DKIM / SPF: " +
                       email)

class CommitMessageCheck:
    """Checks the contents of a git commit message."""

    def __init__(self, subject, message):
        self.ok = True

        if subject is None and  message is None:
            self.error('Commit message is missing!')
            return

        self.subject = subject
        self.msg = message

        print (subject)

        self.check_contributed_under()
        self.check_signed_off_by()
        self.check_misc_signatures()
        self.check_overall_format()
        self.report_message_result()

    url = 'https://github.com/tianocore/tianocore.github.io/wiki/Commit-Message-Format'

    def report_message_result(self):
        if Verbose.level < Verbose.NORMAL:
            return
        if self.ok:
            # All checks passed
            return_code = 0
            print('The commit message format passed all checks.')
        else:
            return_code = 1
        if not self.ok:
            print(self.url)

    def error(self, *err):
        if self.ok and Verbose.level > Verbose.ONELINE:
            print('The commit message format is not valid:')
        self.ok = False
        if Verbose.level < Verbose.NORMAL:
            return
        count = 0
        for line in err:
            prefix = (' *', '  ')[count > 0]
            print(prefix, line)
            count += 1

    # Find 'contributed-under:' at the start of a line ignoring case and
    # requires ':' to be present.  Matches if there is white space before
    # the tag or between the tag and the ':'.
    contributed_under_re = \
        re.compile(r'^\s*contributed-under\s*:', re.MULTILINE|re.IGNORECASE)

    def check_contributed_under(self):
        match = self.contributed_under_re.search(self.msg)
        if match is not None:
            self.error('Contributed-under! (Note: this must be ' +
                       'removed by the code contributor!)')

    @staticmethod
    def make_signature_re(sig, re_input=False):
        if re_input:
            sub_re = sig
        else:
            sub_re = sig.replace('-', r'[-\s]+')
        re_str = (r'^(?P<tag>' + sub_re +
                  r')(\s*):(\s*)(?P<value>\S.*?)(?:\s*)$')
        try:
            return re.compile(re_str, re.MULTILINE|re.IGNORECASE)
        except Exception:
            print("Tried to compile re:", re_str)
            raise

    sig_block_re = \
        re.compile(r'''^
                        (?: (?P<tag>[^:]+) \s* : \s*
                            (?P<value>\S.*?) )
                            |
                        (?: \[ (?P<updater>[^:]+) \s* : \s*
                               (?P<note>.+?) \s* \] )
                    \s* $''',
                   re.VERBOSE | re.MULTILINE)

    def find_signatures(self, sig):
        if not sig.endswith('-by') and sig != 'Cc':
            sig += '-by'
        regex = self.make_signature_re(sig)

        sigs = regex.findall(self.msg)

        bad_case_sigs = filter(lambda m: m[0] != sig, sigs)
        for s in bad_case_sigs:
            self.error("'" +s[0] + "' should be '" + sig + "'")

        for s in sigs:
            if s[1] != '':
                self.error('There should be no spaces between ' + sig +
                           " and the ':'")
            if s[2] != ' ':
                self.error("There should be a space after '" + sig + ":'")

            EmailAddressCheck(s[3], sig)

        return sigs

    def check_signed_off_by(self):
        sob='Signed-off-by'
        if self.msg.find(sob) < 0:
            self.error('Missing Signed-off-by! (Note: this must be ' +
                       'added by the code contributor!)')
            return

        sobs = self.find_signatures('Signed-off')

        if len(sobs) == 0:
            self.error('Invalid Signed-off-by format!')
            return

    sig_types = (
        'Reviewed',
        'Reported',
        'Tested',
        'Suggested',
        'Acked',
        'Cc'
        )

    def check_misc_signatures(self):
        for sig in self.sig_types:
            self.find_signatures(sig)

    cve_re = re.compile('CVE-[0-9]{4}-[0-9]{5}[^0-9]')

    def check_overall_format(self):
        lines = self.msg.splitlines()

        if len(lines) >= 1 and lines[0].endswith('\r\n'):
            empty_line = '\r\n'
        else:
            empty_line = '\n'

        lines.insert(0, empty_line)
        lines.insert(0, self.subject + empty_line)

        count = len(lines)

        if count <= 0:
            self.error('Empty commit message!')
            return

        if count >= 1 and re.search(self.cve_re, lines[0]):
            #
            # If CVE-xxxx-xxxxx is present in subject line, then limit length of
            # subject line to 92 characters
            #
            if len(lines[0].rstrip()) >= 93:
                self.error(
                    'First line of commit message (subject line) is too long (%d >= 93).' %
                    (len(lines[0].rstrip()))
                    )
        else:
            #
            # If CVE-xxxx-xxxxx is not present in subject line, then limit
            # length of subject line to 75 characters
            #
            if len(lines[0].rstrip()) >= 76:
                self.error(
                    'First line of commit message (subject line) is too long (%d >= 76).' %
                    (len(lines[0].rstrip()))
                    )

        if count >= 1 and len(lines[0].strip()) == 0:
            self.error('First line of commit message (subject line) ' +
                       'is empty.')

        if count >= 2 and lines[1].strip() != '':
            self.error('Second line of commit message should be ' +
                       'empty.')

        for i in range(2, count):
            if (len(lines[i]) >= 76 and
                len(lines[i].split()) > 1 and
                not lines[i].startswith('git-svn-id:') and
                not lines[i].startswith('Reviewed-by') and
                not lines[i].startswith('Acked-by:') and
                not lines[i].startswith('Tested-by:') and
                not lines[i].startswith('Reported-by:') and
                not lines[i].startswith('Suggested-by:') and
                not lines[i].startswith('Signed-off-by:') and
                not lines[i].startswith('Cc:')):
                #
                # Print a warning if body line is longer than 75 characters
                #
                print(
                    'WARNING - Line %d of commit message is too long (%d >= 76).' %
                    (i + 1, len(lines[i]))
                    )
                print(lines[i])

        last_sig_line = None
        for i in range(count - 1, 0, -1):
            line = lines[i]
            mo = self.sig_block_re.match(line)
            if mo is None:
                if line.strip() == '':
                    break
                elif last_sig_line is not None:
                    err2 = 'Add empty line before "%s"?' % last_sig_line
                    self.error('The line before the signature block ' +
                               'should be empty', err2)
                else:
                    self.error('The signature block was not found')
                break
            last_sig_line = line.strip()

(START, PRE_PATCH, PATCH) = range(3)

class GitDiffCheck:
    """Checks the contents of a git diff."""

    def __init__(self, diff):
        self.ok = True
        self.format_ok = True
        self.lines = diff.splitlines(True)
        self.count = len(self.lines)
        self.line_num = 0
        self.state = START
        self.new_bin = []
        while self.line_num < self.count and self.format_ok:
            line_num = self.line_num
            self.run()
            assert(self.line_num > line_num)
        self.report_message_result()

    def report_message_result(self):
        if Verbose.level < Verbose.NORMAL:
            return
        if self.ok:
            print('The code passed all checks.')
        if self.new_bin:
            print('\nWARNING - The following binary files will be added ' +
                  'into the repository:')
            for binary in self.new_bin:
                print('  ' + binary)

    def run(self):
        line = self.lines[self.line_num]

        if self.state in (PRE_PATCH, PATCH):
            if line.startswith('diff --git'):
                self.state = START
        if self.state == PATCH:
            if line.startswith('@@ '):
                self.state = PRE_PATCH
            elif len(line) >= 1 and line[0] not in ' -+' and \
                 not line.startswith('\r\n') and  \
                 not line.startswith(r'\ No newline ') and not self.binary:
                for line in self.lines[self.line_num + 1:]:
                    if line.startswith('diff --git'):
                        self.format_error('diff found after end of patch')
                        break
                self.line_num = self.count
                return

        if self.state == START:
            if line.startswith('diff --git'):
                self.state = PRE_PATCH
                self.filename = line[13:].split(' ', 1)[0]
                self.is_newfile = False
                self.force_crlf = True
                self.force_notabs = True
                if self.filename.endswith('.sh') or \
                    self.filename.startswith('BaseTools/BinWrappers/PosixLike/') or \
                    self.filename.startswith('BaseTools/BinPipWrappers/PosixLike/') or \
                    self.filename.startswith('BaseTools/Bin/CYGWIN_NT-5.1-i686/') or \
                    self.filename == 'BaseTools/BuildEnv':
                    #
                    # Do not enforce CR/LF line endings for linux shell scripts.
                    # Some linux shell scripts don't end with the ".sh" extension,
                    # they are identified by their path.
                    #
                    self.force_crlf = False
                if self.filename == '.gitmodules' or \
                   self.filename == 'BaseTools/Conf/diff.order':
                    #
                    # .gitmodules and diff orderfiles are used internally by git
                    # use tabs and LF line endings.  Do not enforce no tabs and
                    # do not enforce CR/LF line endings.
                    #
                    self.force_crlf = False
                    self.force_notabs = False
            elif len(line.rstrip()) != 0:
                self.format_error("didn't find diff command")
            self.line_num += 1
        elif self.state == PRE_PATCH:
            if line.startswith('@@ '):
                self.state = PATCH
                self.binary = False
            elif line.startswith('GIT binary patch') or \
                 line.startswith('Binary files'):
                self.state = PATCH
                self.binary = True
                if self.is_newfile:
                    self.new_bin.append(self.filename)
            elif line.startswith('new file mode 160000'):
                #
                # New submodule.  Do not enforce CR/LF line endings
                #
                self.force_crlf = False
            else:
                ok = False
                self.is_newfile = self.newfile_prefix_re.match(line)
                for pfx in self.pre_patch_prefixes:
                    if line.startswith(pfx):
                        ok = True
                if not ok:
                    self.format_error("didn't find diff hunk marker (@@)")
            self.line_num += 1
        elif self.state == PATCH:
            if self.binary:
                pass
            elif line.startswith('-'):
                pass
            elif line.startswith('+'):
                self.check_added_line(line[1:])
            elif line.startswith('\r\n'):
                pass
            elif line.startswith(r'\ No newline '):
                pass
            elif not line.startswith(' '):
                self.format_error("unexpected patch line")
            self.line_num += 1

    pre_patch_prefixes = (
        '--- ',
        '+++ ',
        'index ',
        'new file ',
        'deleted file ',
        'old mode ',
        'new mode ',
        'similarity index ',
        'copy from ',
        'copy to ',
        'rename ',
        )

    line_endings = ('\r\n', '\n\r', '\n', '\r')

    newfile_prefix_re = \
        re.compile(r'''^
                       index\ 0+\.\.
                   ''',
                   re.VERBOSE)

    def added_line_error(self, msg, line):
        lines = [ msg ]
        if self.filename is not None:
            lines.append('File: ' + self.filename)
        lines.append('Line: ' + line)

        self.error(*lines)

    old_debug_re = \
        re.compile(r'''
                        DEBUG \s* \( \s* \( \s*
                        (?: DEBUG_[A-Z_]+ \s* \| \s*)*
                        EFI_D_ ([A-Z_]+)
                   ''',
                   re.VERBOSE)

    def check_added_line(self, line):
        eol = ''
        for an_eol in self.line_endings:
            if line.endswith(an_eol):
                eol = an_eol
                line = line[:-len(eol)]

        stripped = line.rstrip()

        if self.force_crlf and eol != '\r\n' and (line.find('Subproject commit') == -1):
            self.added_line_error('Line ending (%s) is not CRLF' % repr(eol),
                                  line)
        if self.force_notabs and '\t' in line:
            self.added_line_error('Tab character used', line)
        if len(stripped) < len(line):
            self.added_line_error('Trailing whitespace found', line)

        mo = self.old_debug_re.search(line)
        if mo is not None:
            self.added_line_error('EFI_D_' + mo.group(1) + ' was used, '
                                  'but DEBUG_' + mo.group(1) +
                                  ' is now recommended', line)

    split_diff_re = re.compile(r'''
                                   (?P<cmd>
                                       ^ diff \s+ --git \s+ a/.+ \s+ b/.+ $
                                   )
                                   (?P<index>
                                       ^ index \s+ .+ $
                                   )
                               ''',
                               re.IGNORECASE | re.VERBOSE | re.MULTILINE)

    def format_error(self, err):
        self.format_ok = False
        err = 'Patch format error: ' + err
        err2 = 'Line: ' + self.lines[self.line_num].rstrip()
        self.error(err, err2)

    def error(self, *err):
        if self.ok and Verbose.level > Verbose.ONELINE:
            print('Code format is not valid:')
        self.ok = False
        if Verbose.level < Verbose.NORMAL:
            return
        count = 0
        for line in err:
            prefix = (' *', '  ')[count > 0]
            print(prefix, line)
            count += 1

class CheckOnePatch:
    """Checks the contents of a git email formatted patch.

    Various checks are performed on both the commit message and the
    patch content.
    """

    def __init__(self, name, patch):
        self.patch = patch
        self.find_patch_pieces()

        email_check = EmailAddressCheck(self.author_email, 'Author')
        email_ok = email_check.ok

        msg_check = CommitMessageCheck(self.commit_subject, self.commit_msg)
        msg_ok = msg_check.ok

        diff_ok = True
        if self.diff is not None:
            diff_check = GitDiffCheck(self.diff)
            diff_ok = diff_check.ok

        self.ok = email_ok and msg_ok and diff_ok

        if Verbose.level == Verbose.ONELINE:
            if self.ok:
                result = 'ok'
            else:
                result = list()
                if not msg_ok:
                    result.append('commit message')
                if not diff_ok:
                    result.append('diff content')
                result = 'bad ' + ' and '.join(result)
            print(name, result)


    git_diff_re = re.compile(r'''
                                 ^ diff \s+ --git \s+ a/.+ \s+ b/.+ $
                             ''',
                             re.IGNORECASE | re.VERBOSE | re.MULTILINE)

    stat_re = \
        re.compile(r'''
                       (?P<commit_message> [\s\S\r\n]* )
                       (?P<stat>
                           ^ --- $ [\r\n]+
                           (?: ^ \s+ .+ \s+ \| \s+ \d+ \s+ \+* \-*
                               $ [\r\n]+ )+
                           [\s\S\r\n]+
                       )
                   ''',
                   re.IGNORECASE | re.VERBOSE | re.MULTILINE)

    subject_prefix_re = \
        re.compile(r'''^
                       \s* (\[
                        [^\[\]]* # Allow all non-brackets
                       \])* \s*
                   ''',
                   re.VERBOSE)

    def find_patch_pieces(self):
        if sys.version_info < (3, 0):
            patch = self.patch.encode('ascii', 'ignore')
        else:
            patch = self.patch

        self.commit_msg = None
        self.stat = None
        self.commit_subject = None
        self.commit_prefix = None
        self.diff = None

        if patch.startswith('diff --git'):
            self.diff = patch
            return

        pmail = email.message_from_string(patch)
        parts = list(pmail.walk())
        assert(len(parts) == 1)
        assert(parts[0].get_content_type() == 'text/plain')
        content = parts[0].get_payload(decode=True).decode('utf-8', 'ignore')

        mo = self.git_diff_re.search(content)
        if mo is not None:
            self.diff = content[mo.start():]
            content = content[:mo.start()]

        mo = self.stat_re.search(content)
        if mo is None:
            self.commit_msg = content
        else:
            self.stat = mo.group('stat')
            self.commit_msg = mo.group('commit_message')
        #
        # Parse subject line from email header.  The subject line may be
        # composed of multiple parts with different encodings.  Decode and
        # combine all the parts to produce a single string with the contents of
        # the decoded subject line.
        #
        parts = email.header.decode_header(pmail.get('subject'))
        subject = ''
        for (part, encoding) in parts:
            if encoding:
                part = part.decode(encoding)
            else:
                try:
                    part = part.decode()
                except:
                    pass
            subject = subject + part

        self.commit_subject = subject.replace('\r\n', '')
        self.commit_subject = self.commit_subject.replace('\n', '')
        self.commit_subject = self.subject_prefix_re.sub('', self.commit_subject, 1)

        self.author_email = pmail['from']

class CheckGitCommits:
    """Reads patches from git based on the specified git revision range.

    The patches are read from git, and then checked.
    """

    def __init__(self, rev_spec, max_count):
        commits = self.read_commit_list_from_git(rev_spec, max_count)
        if len(commits) == 1 and Verbose.level > Verbose.ONELINE:
            commits = [ rev_spec ]
        self.ok = True
        blank_line = False
        for commit in commits:
            if Verbose.level > Verbose.ONELINE:
                if blank_line:
                    print()
                else:
                    blank_line = True
                print('Checking git commit:', commit)
            email = self.read_committer_email_address_from_git(commit)
            self.ok &= EmailAddressCheck(email, 'Committer').ok
            patch = self.read_patch_from_git(commit)
            self.ok &= CheckOnePatch(commit, patch).ok
        if not commits:
            print("Couldn't find commit matching: '{}'".format(rev_spec))

    def read_commit_list_from_git(self, rev_spec, max_count):
        # Run git to get the commit patch
        cmd = [ 'rev-list', '--abbrev-commit', '--no-walk' ]
        if max_count is not None:
            cmd.append('--max-count=' + str(max_count))
        cmd.append(rev_spec)
        out = self.run_git(*cmd)
        return out.split() if out else []

    def read_patch_from_git(self, commit):
        # Run git to get the commit patch
        return self.run_git('show', '--pretty=email', '--no-textconv',
                            '--no-use-mailmap', commit)

    def read_committer_email_address_from_git(self, commit):
        # Run git to get the committer email
        return self.run_git('show', '--pretty=%cn <%ce>', '--no-patch',
                            '--no-use-mailmap', commit)

    def run_git(self, *args):
        cmd = [ 'git' ]
        cmd += args
        p = subprocess.Popen(cmd,
                     stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT)
        Result = p.communicate()
        return Result[0].decode('utf-8', 'ignore') if Result[0] and Result[0].find(b"fatal")!=0 else None

class CheckOnePatchFile:
    """Performs a patch check for a single file.

    stdin is used when the filename is '-'.
    """

    def __init__(self, patch_filename):
        if patch_filename == '-':
            patch = sys.stdin.read()
            patch_filename = 'stdin'
        else:
            f = open(patch_filename, 'rb')
            patch = f.read().decode('utf-8', 'ignore')
            f.close()
        if Verbose.level > Verbose.ONELINE:
            print('Checking patch file:', patch_filename)
        self.ok = CheckOnePatch(patch_filename, patch).ok

class CheckOneArg:
    """Performs a patch check for a single command line argument.

    The argument will be handed off to a file or git-commit based
    checker.
    """

    def __init__(self, param, max_count=None):
        self.ok = True
        if param == '-' or os.path.exists(param):
            checker = CheckOnePatchFile(param)
        else:
            checker = CheckGitCommits(param, max_count)
        self.ok = checker.ok

class PatchCheckApp:
    """Checks patches based on the command line arguments."""

    def __init__(self):
        self.parse_options()
        patches = self.args.patches

        if len(patches) == 0:
            patches = [ 'HEAD' ]

        self.ok = True
        self.count = None
        for patch in patches:
            self.process_one_arg(patch)

        if self.count is not None:
            self.process_one_arg('HEAD')

        if self.ok:
            self.retval = 0
        else:
            self.retval = -1

    def process_one_arg(self, arg):
        if len(arg) >= 2 and arg[0] == '-':
            try:
                self.count = int(arg[1:])
                return
            except ValueError:
                pass
        self.ok &= CheckOneArg(arg, self.count).ok
        self.count = None

    def parse_options(self):
        parser = argparse.ArgumentParser(description=__copyright__)
        parser.add_argument('--version', action='version',
                            version='%(prog)s ' + VersionNumber)
        parser.add_argument('patches', nargs='*',
                            help='[patch file | git rev list]')
        group = parser.add_mutually_exclusive_group()
        group.add_argument("--oneline",
                           action="store_true",
                           help="Print one result per line")
        group.add_argument("--silent",
                           action="store_true",
                           help="Print nothing")
        self.args = parser.parse_args()
        if self.args.oneline:
            Verbose.level = Verbose.ONELINE
        if self.args.silent:
            Verbose.level = Verbose.SILENT

if __name__ == "__main__":
    sys.exit(PatchCheckApp().retval)
