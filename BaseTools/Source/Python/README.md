# Edk2 Basetools

This folder has traditionally held the source of Python based tools used by EDK2.
The official repo this source has moved to https://github.com/tianocore/edk2-basetools.
This folder will remain in the tree until the next stable release (expected 202102).
There is a new folder under Basetools `BinPipWrappers` that uses the pip module rather than this tree for Basetools.
By adding the scope `pipbuild-win` or `pipbuild-unix` (depending on your host system), the SDE will use the
`BinPipWrappers` instead of the regular `BinWrappers`.

## Why Move It?

The discussion is on the mailing list. The RFC is here: https://edk2.groups.io/g/rfc/topic/74009714#270
The benefits allow for the Basetools project to be used separately from EDK2 itself as well as offering it in a
globally accessible manner.
This makes it much easier to build a module using Basetools.
Separating the Basetools into their own repo allows for easier CI and contribution process.
Additional pros, cons, and process can be found on the mailing list.

## How Do I Install It?

By default, EDK2 is tied to and tested with a specific version of the Basetools through `pip-requirements.txt`.
You can simply run:

```bash
pip install -r pip-requirements.txt
```

This will install the required module, thought we strongly suggest setting up a virtual environment.
Additionally, you can also install a local clone of the Basetools as well as a specific git commit.
