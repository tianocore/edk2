# Security Policy

Tianocore Edk2 is an open source firmware project that is leveraged by and combined into other projects to build the firmware for a given product.
We build and maintain edk2 knowing that there are many downstream repositories and projects that derive or inherit significant code from this project.
But, that said, in the firmware ecosystem there is a lot of variation and differentiation, and the license in this project allows
flexibility for use without contribution back to Edk2. Therefore, any issues found here may or may not exist in products derived from Edk2.

## Supported Versions

Due to the usage model we generally only supply fixes to the master branch. If requested we may generate a release branch from a stable
tag and apply patches but given our downstream consumption model this is generally not necessary.

## Reporting a Vulnerability

Please do not report security vulnerabilities through public GitHub issues or bugzilla.

Instead please use Github Private vulnerability reporting, which is enabled for the edk2 repository.
This process is well documented by github in their documentation
[here](https://docs.github.com/en/code-security/security-advisories/guidance-on-reporting-and-writing/privately-reporting-a-security-vulnerability#privately-reporting-a-security-vulnerability).

This process will allow us to privately discuss the issue, collaborate on a solution, and then disclose the vulnerability.

## Preferred Languages

We prefer all communications to be in English.

## Policy

Tianocore Edk2 follows the principle of Coordinated Vulnerability Disclosure.
More information is available here:

* [ISO/IEC 29147:2018 on Vulnerability Disclosure](https://www.iso.org/standard/72311.html)
* [The CERT Guide to Coordinated Vulnerability Disclosure](https://resources.sei.cmu.edu/asset_files/SpecialReport/2017_003_001_503340.pdf)
