# FltUsedLib

This library provides a global (fltused) that needs to be defined anywhere floating point operations are used.
The C compiler produces the _fltused symbol by default, this is just to satisfy the linker.

## Using

To use FltUsedLib, just include it in the INF of the module that uses floating point.

```inf
[LibraryClasses]
  BaseLib
  BaseMemoryLib
  FltUsedLib
```
