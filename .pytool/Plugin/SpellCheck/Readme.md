# Spell Check Plugin

This CiBuildPlugin scans all the files in a given package and checks for
spelling errors.

This plugin requires NodeJs and cspell.  If the plugin doesn't find its required
tools then it will mark the test as skipped.

* NodeJS: https://nodejs.org/en/
* cspell: https://www.npmjs.com/package/cspell
  * Src and doc available: https://github.com/streetsidesoftware/cspell

## Configuration

The plugin has a few configuration options to support the UEFI codebase.

``` yaml
  "SpellCheck": {
      "AuditOnly": False,          # If True, log all errors and then mark as skipped
      "IgnoreFiles": [],           # use gitignore syntax to ignore errors in matching files
      "ExtendWords": [],           # words to extend to the dictionary for this package
      "IgnoreStandardPaths": [],   # Standard Plugin defined paths that should be ignore
      "AdditionalIncludePaths": [] # Additional paths to spell check (wildcards supported)
  }
```

### AuditOnly

Boolean - Default is False.
If True run the test in an Audit only mode which will log all errors but instead
of failing the build it will set the test as skipped.  This allows visibility
into the failures without breaking the build.

### IgnoreFiles

This supports .gitignore file and folder matching strings including wildcards

* All files will be parsed regardless but then any spelling errors found within
  ignored files will not be reported as an error.
* Errors in ignored files will still be output to the test results as
  informational comments.

### ExtendWords

This list allows words to be added to the dictionary for the spell checker when
this package is tested.  These follow the rules of the cspell config words field.

### IgnoreStandardPaths

This plugin by default will check the below standard paths.  If the package
would like to ignore any of them list that here.

```python
[
# C source
"*.c",
"*.h",

# Assembly files
"*.nasm",
"*.asm",
"*.masm",
"*.s",

# ACPI source language
"*.asl",

# Edk2 build files
"*.dsc", "*.dec", "*.fdf", "*.inf",

# Documentation files
"*.md", "*.txt"
]
```

### AdditionalIncludePaths

If the package would to add additional path patterns to be included in
spellchecking they can be defined here.

## Other configuration

In the cspell.base.json there are numerous other settings configured.  There is
no support to override these on a per package basis but future features could
make this available.  One interesting configuration option is `minWordLength`.
Currently it is set to _5_ which means all 2,3, and 4 letter words will be
ignored.  This helps minimize the number of technical acronyms, register names,
and other UEFI specific values that must be ignored.

## False positives

The cspell dictionary is not perfect and there are cases where technical words
or acronyms are not found in the dictionary.  There are three ways to resolve
false positives and the choice for which method should be based on how broadly
the word should be accepted.

### CSpell Base Config file

If the change should apply to all UEFI code and documentation then it should be
added to the base config file `words` section.  The base config file is adjacent
to this file and titled `cspell.base.json`.  This is a list of accepted words
for all spell checking operations on all packages.

### Package Config

In the package `*.ci.yaml` file there is a `SpellCheck` config section.  This
section allows files to be ignored as well as words that should be considered
valid for all files within this package.  Add the desired words to the
"ExtendedWords" member.

### In-line File

CSpell supports numerous methods to annotate your files to ignore words,
sections, etc.  This can be found in CSpell documentation.  Suggestion here is
to use a c-style comment at the top of the file to add words that should be
ignored just for this file.  Obviously this has the highest maintenance cost so
it should only be used for file unique words.

``` c
// spell-checker:ignore unenroll, word2, word3
```

or

```ini
# spell-checker:ignore unenroll, word2, word3
```
