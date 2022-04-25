**How to report a Security Issue**

The bug tracking system used for Tianocore projects is Tianocore Bugzilla. An account must be created to enter a new issue or update exiting issues. New security issues must be entered using the Tianocore Security Issue product. Issues in the Tianocore Security Issue product are only visible to the Reporter of the issue and the members of the infosec group. In your report please include the paths of the modules you believe are involved and a detailed description of the issue.

Additional information about Tianocore Bugzilla can be found in Reporting Issues https://github.com/tianocore/tianocore.github.io/wiki/Reporting-Issues

How Security Issues are Evaluated
When a Tianocore Security Issue is entered, the issue is evaluated by the infosec group to determine if the issue is a security issue or not. If it is not deemed to be a security issue, then the issue is converted to a standard issue and follows the normal issue resolution process. If the issue is confirmed to be a security issue, then the priority, severity, and impact of the issue is assessed by the infosec group. Discussions, resolution, and patches are completed within Bugzilla. A date for public disclose is determined, and on that date the issue is made public and added to the list of Security Advisories.

If you are interested in being involved in the evaluation of Tianocore Security Issues, then please send an email request to join the Tianocore Bugzilla infosec group to the Tianocore Community Manager or one of the Tianocore Stewards.

NOTE: Never send any details related to a security issue in email.

Also, tianocore infosec team members should only share details of unmitigated issues in the infosec-tagged Bugzilla entries. Any sharing of unmitigated issues on un-encrypted email or open source prior to embargo expiry may lead to removal from the infosec group.

Now that tianocore is a CNA https://cve.mitre.org/cve/cna.html, namely https://www.cvedetails.com/product/64326/Tianocore-Edk2.html?vendor_id=19679, CVE issuance will be a “Must” for tianocore content and “May” for downstream derivatives of tianocore (open or closed). We request that the reporter perform the initial CVSS calculation. Recommend using https://www.first.org/cvss/calculator/3.1#CVSS:3.1/AV:L/AC:L/PR:L/UI:N/S:U/C:L/I:H/A:L. If reporter doesn’t wish to grade, then infosec will propose a grade and share w/ reporter prior to applying the grading.
