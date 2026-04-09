#!/usr/bin/env python3
#
# Pull Request Formatting Validator
#
# This script validates basic pull request formatting requirements and can
# optionally post validation comments.
#
# Validation Checks Performed:
#
# 1. PR Title Validation:
#    - Ensures the PR title is not empty or whitespace-only
#    - Failure comment: "⚠️ Pull request title cannot be empty."
#
# 2. PR Body Validation:
#    - Ensures the PR body is not empty, null, or whitespace-only
#    - Ensures the PR body meets minimum length requirement (147 characters)
#      - The minimum length is based on the PR template with empty sections
#    - Empty body failure comment: "⚠️ Add a meaningful pull request
#      description using the PR template."
#    - Short body failure comment: "⚠️ Provide a more detailed pull request
#      description using the PR template (current: X characters)."
#
# 3. PR Template Line Validation:
#    - Checks that template placeholder lines are removed from the PR
#      description
#       - Based on present template lines in the PR template file
#    - Failure comment: "⚠️ Remove the following template lines from your
#      PR description:" followed by list of remaining template lines
#
# The same comment (based on a body content hash) will not be posted multiple
# times to avoid spamming the PR with duplicate messages.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import base64
import hashlib
import os
import re
import sys
from typing import List, Optional

import GitHub


class PRValidator:
    """Logic for validating pull request formatting."""

    VALIDATION_HEADER = "Pull Request Formatting Issues"
    MIN_BODY_LENGTH = 147
    TEMPLATE_LINE_PATTERN = r'^<_.*_>$'
    GITHUB_PR_TEMPLATE_PATH = ".github/pull_request_template.md"

    @property
    def GITHUB_PR_TEMPLATE_URL(self) -> str:
        """The PR template URL path for the given PR validator instance."""
        return (f"https://github.com/{self.owner}/{self.repo}/blob/master/"
                f"{self.GITHUB_PR_TEMPLATE_PATH}")

    def __init__(self, github_token: str, owner: str, repo: str,
                 pr_number: int):
        """Initialize the validator with GitHub context."""
        self.github_token = github_token
        self.owner = owner
        self.repo = repo
        self.pr_number = pr_number
        self.validation_messages: List[str] = []
        self.pr = None

    def fetch_pr_data(self) -> tuple[str, str]:
        """Fetch PR title and body."""
        self.pr = GitHub._get_pr(
            self.github_token, self.owner, self.repo, self.pr_number
        )

        if not self.pr:
            print("::error title=Failed to get PR data!::"
                  "Could not fetch PR information")
            sys.exit(1)

        title = self.pr.title or ''
        body = self.pr.body or ''

        return title, body

    def fetch_template_content(self) -> Optional[str]:
        """Fetch the PR template content if it exists."""
        try:
            gh = GitHub._authenticate(self.github_token)
            repo = gh.get_repo(f"{self.owner}/{self.repo}")
            template_file = repo.get_contents(self.GITHUB_PR_TEMPLATE_PATH)

            # Decode from base64
            if hasattr(template_file, 'content'):
                return base64.b64decode(template_file.content).decode('utf-8')

        except Exception:
            # Template doesn't exist, so there is nothing to return
            pass

        return None

    def validate_title(self, title: str) -> bool:
        """Validate that the PR title is not empty."""
        if not title or not title.strip():
            self.validation_messages.append(
                "⚠️ Pull request title cannot be empty.")
            return False
        return True

    def validate_body(self, body: str) -> bool:
        """Validate PR body meets minimum requirements."""
        is_valid = True

        # Check if body is empty or just whitespace
        if not body or body == "null" or not body.strip():
            self.validation_messages.append(
                "⚠️ Add a meaningful pull request description using the "
                f"[PR template]({self.GITHUB_PR_TEMPLATE_URL})."
            )
            is_valid = False
        elif len(body) < self.MIN_BODY_LENGTH:
            self.validation_messages.append(
                "⚠️ Provide a more detailed pull request description "
                f"using the [PR template]({self.GITHUB_PR_TEMPLATE_URL}) "
                f"(current: {len(body)} characters)."
            )
            is_valid = False

        return is_valid

    def validate_template_lines(self, body: str,
                                template_content: str) -> bool:
        """Check for leftover template lines in the PR body."""
        if not template_content:
            return True

        # Find template lines that match the pattern
        template_lines = []
        for line in template_content.splitlines():
            if re.match(self.TEMPLATE_LINE_PATTERN, line):
                template_lines.append(line)

        if not template_lines:
            return True

        # Check if any template lines are still in the PR body
        found_template_lines = []
        body_lines = body.splitlines()

        for template_line in template_lines:
            if template_line in body_lines:
                found_template_lines.append(f"`{template_line}`")

        if found_template_lines:
            self.validation_messages.append(
                "⚠️ Remove the following template lines from your PR "
                "description:\n" + "\n".join(found_template_lines)
            )
            return False

        return True

    def validate_pr(self) -> bool:
        """Run all validation checks and return whether PR is valid."""
        print("Fetching PR data...")
        title, body = self.fetch_pr_data()

        print("Fetching template content...")
        template_content = self.fetch_template_content()

        print("Running validation checks...")
        title_valid = self.validate_title(title)
        body_valid = self.validate_body(body)
        template_valid = self.validate_template_lines(
            body, template_content or '')

        return title_valid and body_valid and template_valid

    def post_validation_comment_with_dedup(self) -> bool:
        """Post a validation comment if there are errors, avoiding duplicates.

        This function uses the existing GitHub.py infrastructure and adds
        duplicate detection based on validation hash.

        Returns:
            bool: True if comment was posted, False if skipped or failed
        """
        if not self.validation_messages:
            return False

        # Create comment body
        messages_text = "\n".join(self.validation_messages)
        comment_body = (
            f"## {self.VALIDATION_HEADER}\n\n"
            f"{messages_text}\n\n"
            f"Address these issues and the validation will automatically "
            f"re-run when you update your pull request."
        )

        # Create an issue comment validation hash to detect duplicate messages
        validation_hash = hashlib.sha256(messages_text.encode()).hexdigest()
        comment_body_with_hash = (
            f"{comment_body}\n\n"
            f"<!-- validation-hash: {validation_hash} -->"
        )

        # Check if any existing comments have the same hash
        try:
            if not self.pr:
                self.pr = GitHub._get_pr(
                    self.github_token, self.owner, self.repo, self.pr_number
                )

            if not self.pr:
                print("::error title=Failed to get PR!::"
                      "Could not fetch PR for comment checking")
                return False

            for comment in self.pr.get_issue_comments():
                if (self.VALIDATION_HEADER in comment.body and
                        f"validation-hash: {validation_hash}" in comment.body):
                    print(f"::notice title=Skipped Duplicate Comment::"
                          f"Validation comment already exists (hash: "
                          f"{validation_hash[:8]}...)")
                    return False

        except Exception as e:
            print(f"::warning title=Comment Check Failed::"
                  f"Could not check for existing comments: {e}. "
                  f"Proceeding to post comment.")

        try:
            GitHub.leave_pr_comment(
                self.github_token, self.owner, self.repo, self.pr_number,
                comment_body_with_hash
            )
            print(f"::notice title=Posted Validation Comment::"
                  f"Comment posted with hash: {validation_hash[:8]}...")
            return True

        except Exception as e:
            print(f"::error title=Failed to Post Comment::"
                  f"Error posting validation comment: {e}")
            return False


def post_validation_comment_standalone():
    """Standalone function for posting validation comments from env vars.

    This function can be called independently to post validation comments
    when validation has already been performed and environment variables
    are set with the validation results.
    """
    # Get required environment variables
    github_token = os.environ.get('GITHUB_TOKEN')
    pr_number_str = os.environ.get('PR_NUMBER')
    validation_header = os.environ.get('VALIDATION_HEADER')
    validation_messages = os.environ.get('VALIDATION_MESSAGES')
    repo_full_name = os.environ.get('GITHUB_REPOSITORY')

    if not all([github_token, pr_number_str, validation_header,
                validation_messages, repo_full_name]):
        print("::error title=Missing Environment Variables!::"
              "Required: GITHUB_TOKEN, PR_NUMBER, VALIDATION_HEADER, "
              "VALIDATION_MESSAGES, GITHUB_REPOSITORY")
        return False

    try:
        pr_number = int(pr_number_str)
    except ValueError:
        print(f"::error title=Invalid PR Number!::"
              f"PR_NUMBER is invalid, got '{pr_number_str}'")
        return False

    # Get the current owner/repo from GITHUB_REPOSITORY
    try:
        owner, repo = repo_full_name.split('/', 1)
    except ValueError:
        print(f"::error title=Invalid Repository Format!::"
              f"GITHUB_REPOSITORY format is invalid: '{repo_full_name}'")
        return False

    print(f"::debug title=Posting Validation Comment::"
          f"PR #{pr_number} in {owner}/{repo}")

    validator = PRValidator(github_token, owner, repo, pr_number)
    validator.validation_messages = validation_messages.split('\n')

    return validator.post_validation_comment_with_dedup()


def main():
    """Main entry point for the PR validation script."""
    # Check if we're being called to just post a comment
    if len(sys.argv) > 1 and sys.argv[1] == '--post-comment':
        success = post_validation_comment_standalone()
        return 0 if success else 1

    # Get required environment variables for validation
    github_token = os.environ.get('GITHUB_TOKEN')
    owner = os.environ.get('OWNER')
    repo = os.environ.get('REPO')
    pr_number_str = os.environ.get('PR_NUMBER')

    if not all([github_token, owner, repo, pr_number_str]):
        print("::error title=Missing Environment Variables!::"
              "Required: GITHUB_TOKEN, OWNER, REPO, PR_NUMBER")
        sys.exit(1)

    try:
        pr_number = int(pr_number_str)
    except ValueError:
        print(f"::error title=Invalid PR Number!::"
              f"PR_NUMBER must be an integer, got '{pr_number_str}'")
        sys.exit(1)

    validator = PRValidator(github_token, owner, repo, pr_number)

    print(f"Validating PR #{pr_number} in {owner}/{repo}")

    is_valid = validator.validate_pr()

    GitHub.set_github_output(
        'validation_error', 'false' if is_valid else 'true'
    )

    if not is_valid:
        messages_text = "\n".join(validator.validation_messages)
        GitHub.set_github_env('VALIDATION_MESSAGES', messages_text)
        GitHub.set_github_env(
            'VALIDATION_HEADER', validator.VALIDATION_HEADER
        )

        print("\nPR formatting validation failed with the following issues:")
        for message in validator.validation_messages:
            print(f"  {message}")
    else:
        print("\nPR formatting validation passed!")

    # Always return success (0) so outputs are set properly
    # The workflow will check the validation_error output to determine failure
    return 0


if __name__ == '__main__':
    sys.exit(main())
