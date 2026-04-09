# CI Templates

This folder contains azure pipeline yml templates for "Core" and "Platform" Continuous Integration and PR validation.

## Common CI templates

### basetools-build-steps.yml

This template compiles the Edk2 basetools from source.  The steps in this template are
conditional and will only run if variable `pkg_count` is greater than 0.

It also has two conditional steps only used when the toolchain contains GCC. These two steps
use `apt` to update the system packages and add those necessary for Edk2 builds.

## Core CI templates

### pr-gate-build-job.yml

This templates contains the jobs and most importantly the matrix of which packages and
targets to run for Core CI.

### pr-gate-steps.yml

This template is the main Core CI template.  It controls all the steps run and is responsible for most functionality of the Core CI process.  This template sets
the `pkg_count` variable using the `stuart_pr_eval` tool when the
build type is "pull request"

### spell-check-prereq-steps.yml

This template installs the node based tools used by the spell checker plugin. The steps
in this template are conditional and will only run if variable `pkg_count` is greater than 0.

## Platform CI templates

### platform-build-run-steps.yml

This template makes heavy use of pytools to build and run a platform in the Edk2 repo

Also uses basetools-build-steps.yml to compile basetools

#### Special Notes

* For a build type of pull request it will conditionally build if the patches change files that impact the platform.
  * uses `stuart_pr_eval` to determine impact
* For manual builds or CI builds it will always build the platform
* It compiles basetools from source
* Will use `stuart_build --FlashOnly` to attempt to run the built image if the `Run` parameter is set.
* See the parameters block for expected configuration options
* Parameter `extra_install_step` allows the caller to insert extra steps.  This is useful if additional dependencies, tools, or other things need to be installed.  Here is an example of installing qemu on Windows.

    ``` yaml
    steps:
    - template: ../../.azurepipelines/templates/build-run-steps.yml
      parameters:
        extra_install_step:
        - powershell: choco install qemu; Write-Host "##vso[task.prependpath]c:\Program Files\qemu"
          displayName: Install QEMU and Set QEMU on path # friendly name displayed in the UI
          condition: and(gt(variables.pkg_count, 0), succeeded())
    ```
