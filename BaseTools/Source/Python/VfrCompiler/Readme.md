# Python VfrCompiler Tool
## Overview
This python VfrCompiler tool is the python implementation of the edk2 VfrCompiler tool which C implementation locates at https://github.com/tianocore/edk2/tree/master/BaseTools/Source/C/VfrCompile.

This python implementation not only covers the same usage as the C version VfrCompiler, but also extends several new features.

### Introduction
The core function of the original C VfrCompiler tool is to convert VFR files into IFR binaries. However, the VFR format syntax is uncommon and has poor readability for users. Additionally, the C tool doesn't gather or store default variable information, except for binary generation. When modifications are required, developers have to deal with the abstract IFR binary, resulting in inefficiency. To address these challenges, this python tool generates YAML format files from VFR files. This approach allows for preservation of the VFR file's opcode layout and hierarchy, simplifying readability. Moreover, the tool generates an additional JSON file to provide a more accessible presentation of variable-related information. In a few words, the new tool offers the same usage as the original C VfrCompiler while expanding functionality with the YAML/JSON output, enhancing readability and adaptability.

### Main update in this commitment
- Update the vfr parser generator from ANTLR to ANTLR v4, which is more readable and is eaiser to add on new grammars and functions.
- Cover the same usage as the edk2 C VfrCompiler. The tool is able to compiles VFR into .c/lst/hpk output files.
- Extract variable information from each VFR file to JSON. The output JSON file stores the data structures and default values of variables.
- Convert each VFR source format to an equivalent YAML. The generated YAML file covers the same format contents in the Vfr file.

### Known issues
- Issue 1 - Test Coverage and Method
  - The current python vfrcompiler tool does not have a full code coverage test.
    - Should add unit tests for higher code coverage.
  - The test should be enabled into Azure pipelines for retriggering.
  - The current test script uses some output binaries for test input, will remove these binaries, and save the necessary part content of it directly into the test script itself.
- Issue 2 - Code style
  - The current python vfrcompiler tool has some coding style still like C style which not follows the python standardize rule.
    - Should enhance GUID usage with 'uuid' pip module;
    - Should figure out common way for Status Code processing;
  - The current python vfrcompiler tool need docstring description for code explanation.
- Issue 3 - New generated YAML format
  - The current YAML format is a reference version, and we warmly welcome everyone to provide feedback.
- Future extension
  - The tool will extend new functions, which is able to compile yaml files. This feature will be added in future update.

### Use with Build System
To use the VfrCompiler Python Tool with Build System,  please do the following steps in the build command.
1. Locate the **VfrCompiler** folder to path **'\edk2\BaseTools\Source\Python'.**
1. Open  **'build_rule.template'**  file  in path **'\edk2\BaseTools\Conf\'.**
  - Find the C VFR command line `$(VFR)" $(VFR_FLAGS) --string-db $(OUTPUT_DIR)(+)$(MODULE_NAME)StrDefs.hpk --output-directory ${d_path} $(OUTPUT_DIR)(+)${s_base}.i` in **build_rule.template** file. There are two C VFR commands in it.
  - Add new command line `"$(PYVFR)" ${src} --string-db $(OUTPUT_DIR)(+)$(MODULE_NAME)StrDefs.hpk -w $(WORKSPACE) -m $(MODULE_NAME) -o $(OUTPUT_DIR) --vfr` after each VFR command lines.
2. Open  **'tools_def.template'**  file  in path **'\edk2\BaseTools\Conf\'.**
  - Find the C VFR_PATH command line `*_*_*_VFR_PATH                      = VfrCompile` in **tools_def.template** file.
  - Add new command line `*_*_*_PYVFR_PATH                    = PyVfrCompile` after the VFR_PATH command line.
3. For windows build, create a **PyVfrCompile.bat** file in path **'C:\edk2\BaseTools\BinWrappers\WindowsLike'.**
  - Add the following lines in the created **PyVfrCompile.bat** file.
    ```
    @setlocal
    @set ToolName=IfrCompiler
    @set PYTHONPATH=%PYTHONPATH%;%BASE_TOOLS_PATH%\Source\Python;%BASE_TOOLS_PATH%\Source\Python\VfrCompiler
    @%PYTHON_COMMAND% -m %ToolName% %*
    ```
4. For Unix build, create a **PyVfrCompile** file in path **'C:\edk2\BaseTools\BinWrappers\PosixLike'.**
  - Add the following lines in the created **PyVfrCompile** file.
    ```
    #!/usr/bin/env bash
    #python `dirname $0`/RunToolFromSource.py `basename $0` $*

    # If a ${PYTHON_COMMAND} command is available, use it in preference to python
    if command -v ${PYTHON_COMMAND} >/dev/null 2>&1; then
        python_exe=${PYTHON_COMMAND}
    fi
    full_cmd=${BASH_SOURCE:-$0} # see http://mywiki.wooledge.org/BashFAQ/028 for a discussion of why $0 is not a good choice here
    dir=$(dirname "$full_cmd")
    cmd=${full_cmd##*/}

    export PYTHONPATH="$dir/../../Source/Python:$dir/../../Source/Python/VfrCompiler:$dir/../../Source/Python${PYTHONPATH:+:"$PYTHONPATH"}"
    exec "${python_exe:-python}" -m IfrCompiler "$@"
    ```
5. Add Env: run `pip install antlr4-python3-runtime==4.7.1` based on the original build environment.
6. Run Build Command: `build -p OvmfPkg\OvmfPkgIa32X64.dsc -a IA32 -a X64 -j build.log`
`
