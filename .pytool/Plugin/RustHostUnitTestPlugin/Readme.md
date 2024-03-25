# Rust Host Unit Test Plugin

This CI plugin runs all unit tests and requires that they pass. If code coverage is turned on with
`"CalculateCoverage": true`, then coverage percentages will be calculated on a per rust crate basis.

## Plugin Customizations

- `CalculateCoverage`: true / false - Whether or not to calculate coverage results
- `Coverage`: int (0, 1) - The percentage of coverage required to pass the CI check, if `CalculateCoverage` is enabled
- `CoverageOverrides` int (0, 1) - Crate specific override of percentage needed to pass

As a default, Calculating Coverage is enabled and at least 75% (.75) code coverage is required to pass.

### Example CI settings

``` yaml
"RustHostUnitTestPlugin": {
    "CalculateCoverage": true,
    "Coverage": 1,
    "CoverageOverrides": {
        "DxeRust": 0.4,
        "UefiEventLib": 0.67,
    }
}
```
