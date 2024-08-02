## @file
#  Used in a CI workflow to request reviewers for a pull request.
#
#  Refer to the following link for a list of pre-defined GitHub workflow
#  environment variables:
#    https://docs.github.com/actions/reference/environment-variables
#
#  Copyright (c) Microsoft Corporation.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import git
import GitHub
import os
import sys


"""Request Pull Request Reviewers Helpers"""


def request_pr_reviewers():
    """Request pull request reviewers for a GitHub PR.

    This function is intended to be used in a GitHub Actions workflow to
    request reviewers for a pull request triggered by a GitHub event. The
    function makes assumptions about GitHub workflow environment variables and
    the pull request context in which it is run.

    The function will exit with a non-zero status indicating an error if a
    critical error occurs during execution so the workflow fails.

    The following environment variables are expected to be set before calling
    this function. The recommend GitHub context values are show for reference:
          GH_TOKEN: ${{ secrets.GITHUB_TOKEN }}
          ORG_NAME: ${{ github.repository_owner }}
          PR_NUMBER: ${{ github.event.number}}
          REPO_NAME: ${{ github.event.pull_request.base.repo.name }}
          TARGET_BRANCH: ${{ github.event.pull_request.base.ref }}
          WORKSPACE_PATH: ${{ github.workspace }}
    """
    WORKSPACE_PATH = os.environ["WORKSPACE_PATH"]
    GET_MAINTAINER_LOCAL_PATH = os.path.join(
        WORKSPACE_PATH, os.environ["GET_MAINTAINER_REL_PATH"]
    )

    # Step 1: Get the GitHub created PR commit SHA (contains all changes in a single commit)
    pr_commit_sha = GitHub.get_pr_sha(
        os.environ["GH_TOKEN"],
        os.environ["ORG_NAME"],
        os.environ["REPO_NAME"],
        int(os.environ["PR_NUMBER"]),
    )
    if not pr_commit_sha:
        sys.exit(1)

    print(
        f"::notice title=PR Commit SHA::Looking at files in consolidated PR commit: {pr_commit_sha}"
    )

    # Step 2: Fetch only the PR commit to get the files changed in the PR
    git.Repo(WORKSPACE_PATH).remotes.origin.fetch(pr_commit_sha, depth=1)

    # Step 3: Get the list of reviewers for the PR
    reviewers = GitHub.get_reviewers_for_range(
        WORKSPACE_PATH, GET_MAINTAINER_LOCAL_PATH, pr_commit_sha, pr_commit_sha
    )
    if not reviewers:
        print("::notice title=No New Reviewers Found!::No reviewers found for this PR.")
        sys.exit(0)

    print(
        f"::notice title=Preliminary Reviewer List::Total reviewer candidates for "
        f"PR {os.environ['PR_NUMBER']}: {', '.join(reviewers)}"
    )

    # Step 4: Add the reviewers to the PR
    #         Note the final requested reviewer list in the workflow run for reference
    new_reviewers = GitHub.add_reviewers_to_pr(
        os.environ["GH_TOKEN"],
        os.environ["ORG_NAME"],
        os.environ["REPO_NAME"],
        int(os.environ["PR_NUMBER"]),
        reviewers,
    )
    if new_reviewers:
        print(
            f"::notice title=New Reviewers Added::New reviewers requested for PR "
            f"{os.environ['PR_NUMBER']}: {', '.join(new_reviewers)}"
        )
    else:
        print(
            "::notice title=No New Reviewers Added::No reviewers were found that "
            "should be newly requested."
        )


if __name__ == '__main__':
    request_pr_reviewers()
