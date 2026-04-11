# Depdendency Check Plugin

A CiBuildPlugin that finds all modules (inf files) in a package and reviews the
packages used to confirm they are acceptable.  This is to help enforce layering
and identify improper dependencies between packages.

## Configuration

The plugin must be configured with the acceptabe package dependencies for the
package.

``` yaml
"DependencyCheck": {
    "AcceptableDependencies": [],
    "AcceptableDependencies-<MODULE_TYPE>": [],
    "IgnoreInf": []
}
```

### AcceptableDependencies

Package dec files that are allowed in all INFs.  Example: MdePkg/MdePkg.dec

### AcceptableDependencies-<MODULE_TYPE>

OPTIONAL Package dependencies for INFs that have module type <MODULE_TYPE>.
Example: AcceptableDependencies-HOST_APPLICATION.

### IgnoreInf

OPTIONAL list of INFs to ignore for this dependency check.
