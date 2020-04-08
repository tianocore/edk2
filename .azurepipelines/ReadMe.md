# Azure DevOps Pipelines

These yml files are used to provide CI builds using the Azure DevOps Pipeline Service.
Most of the CI leverages edk2-pytools to support cross platform building and execution.

## Core CI

Focused on building and testing all packages in Edk2 without an actual target platform.

See `.pytools/ReadMe.py` for more details

## Platform CI

Focused on building a single target platform and confirming functionality on that platform.

## Conventions

* Files extension should be *.yml.  *.yaml is also supported but in Edk2 we use those for our package configuration.
* Platform CI files should be in the `<PlatformPkg>/.azurepipelines` folder.
* Core CI files are in the root folder.
* Shared templates are in the `templates` folder.
* Top level CI files should be named `<host os>-<tool_chain_tag>.yml`

## Links

* Basic Azure Landing Site - https://docs.microsoft.com/en-us/azure/devops/pipelines/?view=azure-devops
* Pipeline jobs - https://docs.microsoft.com/en-us/azure/devops/pipelines/process/phases?view=azure-devops&tabs=yaml
* Pipeline yml scheme - https://docs.microsoft.com/en-us/azure/devops/pipelines/yaml-schema?view=azure-devops&tabs=schema%2Cparameter-schema
* Pipeline expression - https://docs.microsoft.com/en-us/azure/devops/pipelines/process/expressions?view=azure-devops
* PyTools - https://github.com/tianocore/edk2-pytool-extensions and https://github.com/tianocore/edk2-pytool-library

## Lessons Learned

### Templates and parameters

They are great but evil.  If they are used as part of determining the steps of a build they must resolve before the build starts.  They can not use variables set in a yml or determined as part of a matrix.  If they are used in a step then they can be bound late.

### File matching patterns

On Linux this can hang if there are too many files in the search list.

### Templates and file splitting

Suggestion is to do one big yaml file that does what you want for one of your targets.  Then do the second one and find the deltas.  From that you can start to figure out the right split of files, steps, jobs.

### Conditional steps

If you want the step to show up in the log but not run, use a step conditional. This is great when a platform doesn't currently support a feature but you want the builders to know that the features exists and maybe someday it will.

If you want the step to not show up use a template step conditional wrapper.  Beware this will be evaluated early (at build start).  This can hide things not needed on a given OS for example.
