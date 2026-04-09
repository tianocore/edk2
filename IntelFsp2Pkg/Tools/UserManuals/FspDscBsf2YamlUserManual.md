#Name
**FspDscBsf2Yaml.py** The python script that generates YAML file for
the Boot Settings from an EDK II Platform Description (**DSC**) file
or from a Boot Settings File (**BSF**). It is created to help
transitioning FSP Updateable Product Data (**UPD**) file format to
new standardized YAML format so that it can be configured through
open source tools.

#Synopsis
```
FspDscBsf2Yaml DscFile|BsfFile  YamlFile
```

#Description
**FspDscBsf2Yaml.py** is a script that generates configuration options from an
**EDK II Platform Description (DSC)** file or **a Boot Settings File (BSF)** file.

It generates a **YAML file** that can be used by the **Config Editor** to provide
a graphical user interface for manipulating settings in the UPD regions.

The following sections explain the usage of this script.

## 1. FspDscBsf2Yaml.py DscFile YamlFile

The **DscFile** option is an input DSC file.

The **YamlFile** option is an output YAML file.

The script takes the FSP DSC file consisting BSF syntax and generates a YAML
output file describing the boot settings.

## 2. FspDscBsf2Yaml.py BsfFile YamlFile

The **BsfFile** option is an input BSF file.

The **YamlFile** option is an output YAML file.

The script generates a YAML output file from a BSF file. The BSF file
can be generated using GenCfgOpt tool.
