## @file
#  GitHub API helper functions.
#
#  Copyright (c) Microsoft Corporation.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import git
import logging
import re
import requests

from collections import OrderedDict
from edk2toollib.utility_functions import RunPythonScript
from github import Auth, Github, GithubException
from io import StringIO
from typing import List


"""GitHub API helper functions."""


def _authenticate(token: str):
    """Authenticate to GitHub using a token.

    Returns a GitHub instance that is authenticated using the provided
    token.

    Args:
        token (str): The GitHub token to use for authentication.

    Returns:
        Github: A GitHub instance.
    """
    auth = Auth.Token(token)
    return Github(auth=auth)


def _get_pr(token: str, owner: str, repo: str, pr_number: int):
    """Get the PR object from GitHub.

    Args:
        token (str): The GitHub token to use for authentication.
        owner (str): The GitHub owner (organization) name.
        repo (str): The GitHub repository name (e.g. 'edk2').
        pr_number (int): The pull request number.

    Returns:
        PullRequest: A PyGithub PullRequest object for the given pull request
                     or None if the attempt to get the PR fails.
    """
    try:
        g = _authenticate(token)
        return g.get_repo(f"{owner}/{repo}").get_pull(pr_number)
    except GithubException as ge:
        print(
            f"::error title=Error Getting PR {pr_number} Info!::"
            f"{ge.data['message']}"
        )
        return None


def leave_pr_comment(
    token: str, owner: str, repo: str, pr_number: int, comment_body: str
):
    """Leaves a comment on a PR.

    Args:
        token (str): The GitHub token to use for authentication.
        owner (str): The GitHub owner (organization) name.
        repo (str): The GitHub repository name (e.g. 'edk2').
        pr_number (int): The pull request number.
        comment_body (str): The comment text. Markdown is supported.
    """
    if pr := _get_pr(token, owner, repo, pr_number):
        try:
            pr.create_issue_comment(comment_body)
        except GithubException as ge:
            print(
                f"::error title=Error Commenting on PR {pr_number}!::"
                f"{ge.data['message']}"
            )


def get_reviewers_for_range(
    workspace_path: str,
    maintainer_file_path: str,
    range_start: str = "master",
    range_end: str = "HEAD",
) -> List[str]:
    """Get the reviewers for the current branch.

       To get the reviewers for a single commit, set `range_start` and
       `range_end` to the commit SHA.

    Args:
        workspace_path (str): The workspace path.
        maintainer_file_path (str): The maintainer file path.
        range_start (str, optional): The range start ref. Defaults to "master".
        range_end (str, optional): The range end ref. Defaults to "HEAD".

    Returns:
        List[str]: A list of GitHub usernames.
    """
    if range_start == range_end:
        commits = [range_start]
    else:
        commits = [
            c.hexsha
            for c in git.Repo(workspace_path).iter_commits(
                f"{range_start}..{range_end}"
            )
        ]

    raw_reviewers = []
    for commit_sha in commits:
        reviewer_stream_buffer = StringIO()
        cmd_ret = RunPythonScript(
            maintainer_file_path,
            f"-g {commit_sha}",
            workingdir=workspace_path,
            outstream=reviewer_stream_buffer,
            logging_level=logging.INFO,
        )
        if cmd_ret != 0:
            print(
                f"::error title=Reviewer Lookup Error!::Error calling GetMaintainer.py: [{cmd_ret}]: {reviewer_stream_buffer.getvalue()}"
            )
            return []

        commit_reviewers = reviewer_stream_buffer.getvalue()

        pattern = r"\[(.*?)\]"
        matches = re.findall(pattern, commit_reviewers)
        if not matches:
            return []

        print(
            f"::debug title=Commit {commit_sha[:7]} Reviewer(s)::{', '.join(matches)}"
        )

        raw_reviewers.extend(matches)

    reviewers = list(OrderedDict.fromkeys([r.strip() for r in raw_reviewers]))

    print(f"::debug title=Total Reviewer Set::{', '.join(reviewers)}")

    return reviewers


def get_pr_sha(token: str, owner: str, repo: str, pr_number: int) -> str:
    """Returns the commit SHA of given PR branch.

       This returns the SHA of the merge commit that GitHub creates from a
       PR branch. This commit contains all of the files in the PR branch in
       a single commit.

    Args:
        token (str): The GitHub token to use for authentication.
        owner (str): The GitHub owner (organization) name.
        repo (str): The GitHub repository name (e.g. 'edk2').
        pr_number (int): The pull request number.

    Returns:
        str: The commit SHA of the PR branch. An empty string is returned
             if the request fails.
    """
    if pr := _get_pr(token, owner, repo, pr_number):
        merge_commit_sha = pr.merge_commit_sha
        print(f"::debug title=PR {pr_number} Merge Commit SHA::{merge_commit_sha}")
        return merge_commit_sha

    return ""


def download_gh_file(github_url: str, local_path: str, token=None):
    """Downloads a file from GitHub.

    Args:
        github_url (str): The GitHub raw file URL.
        local_path (str): A local path to write the file contents to.
        token (_type_, optional): A GitHub authentication token.
            Only needed for a private repo. Defaults to None.
    """
    headers = {}
    if token:
        headers["Authorization"] = f"Bearer {token}"

    try:
        response = requests.get(github_url, headers=headers)
        response.raise_for_status()
    except requests.exceptions.HTTPError:
        print(
            f"::error title=HTTP Error!::Error downloading {github_url}: {response.reason}"
        )
        return

    with open(local_path, "w", encoding="utf-8") as file:
        file.write(response.text)


def add_reviewers_to_pr(
    token: str, owner: str, repo: str, pr_number: int, user_names: List[str]
) -> List[str]:
    """Adds the set of GitHub usernames as reviewers to the PR.

    Args:
        token (str): The GitHub token to use for authentication.
        owner (str): The GitHub owner (organization) name.
        repo (str): The GitHub repository name (e.g. 'edk2').
        pr_number (int): The pull request number.
        user_names (List[str]): List of GitHub usernames to add as reviewers.

    Returns:
        List[str]: A list of GitHub usernames that were successfully added as
                   reviewers to the PR. This list will exclude any reviewers
                   from the list provided if they are not relevant to the PR.
    """
    try:
        g = _authenticate(token)
        repo_gh = g.get_repo(f"{owner}/{repo}")
        pr = repo_gh.get_pull(pr_number)
    except GithubException as ge:
        print(
            f"::error title=Error Getting PR {pr_number} Info!::"
            f"{ge.data['message']}"
        )
        return None

    pr_author = pr.user.login.strip()

    while pr_author in user_names:
        user_names.remove(pr_author)

    repo_collaborators = [c.login.strip() for c in repo_gh.get_collaborators()]
    non_collaborators = [u for u in user_names if u not in repo_collaborators]

    if non_collaborators:
        print(
            f"::error title=User is not a Collaborator!::{', '.join(non_collaborators)}"
            )

        leave_pr_comment(
            token,
            owner,
            repo,
            pr_number,
            f"&#9888; **WARNING: Cannot add reviewers**: A user specified as a "
            f"reviewer for this PR is not a collaborator "
            f"of the edk2 repository. Please add them as a collaborator to the "
            f"repository and re-request the review.\n\n"
            f"Users requested:\n{', '.join(user_names)}",
        )

    pr.create_review_request(reviewers=user_names)

    return user_names
