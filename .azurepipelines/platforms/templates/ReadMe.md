# Platform CI Templates

This folder contains azure pipeline yml templates for building "Platform CI"

## build-run-steps.yml

Path: `.azurepipelines/platforms/template/build-run-steps.yml`

This template makes heavy use of pytools to build a platform in the Edk2 repo

### Special Notes

* For a build type of pull request it will conditionally build if the patches change files that impact the platform.
  * uses `stuart_pr_eval` to determine impact
* For manual builds or CI builds it will always build the platform
* It compiles basetools from source
* Will use `stuart_build --FlashOnly` to attempt to run the built image if the `Run` parameter is set.
* See the parameters block for expected configuration options
* Parameter `extra_install_step` allows the caller to insert extra steps.  This is useful if additional dependencies, tools, or other things need to be installed.  Here is an example of installing qemu on Windows.
    ``` yaml
    steps:
    - template: ../templates/build-run-steps.yml
      parameters:
        extra_install_step:
        - powershell: choco install qemu; Write-Host "##vso[task.prependpath]c:\Program Files\qemu"
          displayName: Install QEMU and Set QEMU on path # friendly name displayed in the UI
          condition: and(gt(variables.pkg_count, 0), succeeded())
    ```

