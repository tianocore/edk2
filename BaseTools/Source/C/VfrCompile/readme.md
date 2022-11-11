# EDK2 Python VfrCompiler
## OverView
This python VfrCompiler tool is the implementation of the edk2 VfrCompiler tool which locates at https://github.com/tianocore/edk2/tree/master/BaseTools/Source/C/VfrCompile. 
### Introduction
The core function of the original C VfrCompiler tool is to convert the vfr file into IFR binary array. However, the vfr format file syntax is relatively uncommon and has poor readability for users. What's more, the C tool does not collect and store extra information except for binary generation. When any modification is needed, developers have to read and deal with the IFR binary, which is inefficient. To solve this issue, this python VfrCompiler tool proposes to generate yaml formset file on the basis of the vfr format file for better readability and variability. And the long term aim is to apply yaml as the source file to gradually replace the vfr file.

### Main function in this update:
1. Update the vfr parser generator from ANTLR to ANTLR v4.
2. Generate a yaml format file, which  displays all the contents in the Vfr file by different Opcode types in sequence.
3. Generate a json file, which 

#### Known issue:

- The VfrCompiler python tool is aim to cover the same usage as the edk2 C VfrCompiler. But currently, VfrCompiler Python tool does not support IFR binary generation feature, this feature will be added in future update.
- The VfrCompiler python tool will extend new functions, which is able to compile both the vfr and yaml files. This feature will be added in future update.

### Usage Way
To enable the python VfrCompiler tool, please add the '--vfr-yaml-enable' option in the build command.
