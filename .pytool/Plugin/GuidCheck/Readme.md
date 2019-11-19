# Guid Check Plugin

This CiBuildPlugin scans all the files in a code tree to find all the GUID
definitions.  After collection it will then look for duplication in the package
under test.  Uniqueness of all GUIDs are critical within the UEFI environment.
Duplication can cause numerous issues including locating the wrong data
structure, calling the wrong function, or decoding the wrong data members.

Currently Scanned:

* INF files are scanned for there Module guid
* DEC files are scanned for all of their Protocols, PPIs, and Guids as well as
  the one package GUID.

Any GUID value being equal to two names or even just defined in two files is
considered an error unless in the ignore list.

Any GUID name that is found more than once is an error unless all occurrences
are Module GUIDs.  Since the Module GUID is assigned to the Module name it is
common to have numerous versions of the same module named the same.

## Configuration

The plugin has numerous configuration options to support the UEFI codebase.

``` yaml
"GuidCheck": {
        "IgnoreGuidName": [],
        "IgnoreGuidValue": [],
        "IgnoreFoldersAndFiles": [],
        "IgnoreDuplicates": []
    }
```

### IgnoreGuidName

This list allows strings in two formats.

* _GuidName_
  * This will remove any entry with this GuidName from the list of GUIDs
    therefore ignoring any error associated with this name.
* _GuidName=GuidValue_
  * This will also ignore the GUID by name but only if the value equals the
    GuidValue.
  * GuidValue should be in registry format.
  * This is the suggested format to use as it will limit the ignore to only the
    defined case.

### IgnoreGuidValue

This list allows strings in guid registry format _GuidValue_.

* This will remove any entry with this GuidValue from the list of GUIDs
  therefore ignoring any error associated with this value.
* GuidValue must be in registry format xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

### IgnoreFoldersAndFiles

This supports .gitignore file and folder matching strings including wildcards

* Any folder or file ignored will not be parsed and therefore any GUID defined
  will be ignored.
* The plugin will always ignores the following ["/Build", "/Conf"]

### IgnoreDuplicates

This supports strings in the format of _GuidName_=_GuidName_=_GuidName_

* For the error with the GuidNames to be ignored the list must match completely
  with what is found during the code scan.
  * For example if there are two GUIDs that are by design equal within the code
    tree then it should be _GuidName_=_GuidName_
  * If instead there are three GUIDs then it must be
    _GuidName_=_GuidName_=_GuidName_
* This is the best ignore list to use because it is the most strict and will
  catch new problems when new conflicts are introduced.
* There are numerous places in the UEFI specification in which two GUID names
  are assigned the same value.  These names should be set in this ignore list so
  that they don't cause an error but any additional duplication would still be
  caught.
