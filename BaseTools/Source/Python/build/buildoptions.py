## @file
# build a platform or a module
#
#  Copyright (c) 2014, Hewlett-Packard Development Company, L.P.<BR>
#  Copyright (c) 2007 - 2021, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2018 - 2020, Hewlett Packard Enterprise Development, L.P.<BR>
#  Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

# Version and Copyright
from Common.BuildVersion import gBUILD_VERSION
from optparse import OptionParser
VersionNumber = "0.60" + ' ' + gBUILD_VERSION
__version__ = "%prog Version " + VersionNumber
__copyright__ = "Copyright (c) 2007 - 2018, Intel Corporation  All rights reserved."

gParamCheck = []
def SingleCheckCallback(option, opt_str, value, parser):
    if option not in gParamCheck:
        setattr(parser.values, option.dest, value)
        gParamCheck.append(option)
    else:
        parser.error("Option %s only allows one instance in command line!" % option)


class MyOptionParser():

    def __new__(cls, *args, **kw):
        if not hasattr(cls, '_instance'):
            orig = super(MyOptionParser, cls)
            cls._instance = orig.__new__(cls, *args, **kw)
        return cls._instance

    def __init__(self):
        if not hasattr(self, 'BuildOption'):
            self.BuildOption = None
        if not hasattr(self, 'BuildTarget'):
            self.BuildTarget = None

    def GetOption(self):
        Parser = OptionParser(description=__copyright__, version=__version__, prog="build.exe", usage="%prog [options] [all|fds|genc|genmake|clean|cleanall|cleanlib|modules|libraries|run]")
        Parser.add_option("-a", "--arch", action="append", dest="TargetArch",
            help="ARCHS is one of list: IA32, X64, ARM, AARCH64, RISCV64, LOONGARCH64 or EBC, which overrides target.txt's TARGET_ARCH definition. To specify more archs, please repeat this option.")
        Parser.add_option("-p", "--platform", action="callback", type="string", dest="PlatformFile", callback=SingleCheckCallback,
            help="Build the platform specified by the DSC file name argument, overriding target.txt's ACTIVE_PLATFORM definition.")
        Parser.add_option("-m", "--module", action="callback", type="string", dest="ModuleFile", callback=SingleCheckCallback,
            help="Build the module specified by the INF file name argument.")
        Parser.add_option("-b", "--buildtarget", type="string", dest="BuildTarget", help="Using the TARGET to build the platform, overriding target.txt's TARGET definition.",
                          action="append")
        Parser.add_option("-t", "--tagname", action="append", type="string", dest="ToolChain",
            help="Using the Tool Chain Tagname to build the platform, overriding target.txt's TOOL_CHAIN_TAG definition.")
        Parser.add_option("-x", "--sku-id", action="callback", type="string", dest="SkuId", callback=SingleCheckCallback,
            help="Using this name of SKU ID to build the platform, overriding SKUID_IDENTIFIER in DSC file.")

        Parser.add_option("-n", action="callback", type="int", dest="ThreadNumber", callback=SingleCheckCallback,
            help="Build the platform using multi-threaded compiler. The value overrides target.txt's MAX_CONCURRENT_THREAD_NUMBER. When value is set to 0, tool automatically detect number of "\
                 "processor threads, set value to 1 means disable multi-thread build, and set value to more than 1 means user specify the threads number to build.")

        Parser.add_option("-f", "--fdf", action="callback", type="string", dest="FdfFile", callback=SingleCheckCallback,
            help="The name of the FDF file to use, which overrides the setting in the DSC file.")
        Parser.add_option("-r", "--rom-image", action="append", type="string", dest="RomImage", default=[],
            help="The name of FD to be generated. The name must be from [FD] section in FDF file.")
        Parser.add_option("-i", "--fv-image", action="append", type="string", dest="FvImage", default=[],
            help="The name of FV to be generated. The name must be from [FV] section in FDF file.")
        Parser.add_option("-C", "--capsule-image", action="append", type="string", dest="CapName", default=[],
            help="The name of Capsule to be generated. The name must be from [Capsule] section in FDF file.")
        Parser.add_option("-u", "--skip-autogen", action="store_true", dest="SkipAutoGen", help="Skip AutoGen step.")
        Parser.add_option("-e", "--re-parse", action="store_true", dest="Reparse", help="Re-parse all meta-data files.")

        Parser.add_option("-c", "--case-insensitive", action="store_true", dest="CaseInsensitive", default=False, help="Don't check case of file name.")

        Parser.add_option("-w", "--warning-as-error", action="store_true", dest="WarningAsError", help="Treat warning in tools as error.")
        Parser.add_option("-j", "--log", action="store", dest="LogFile", help="Put log in specified file as well as on console.")

        Parser.add_option("-s", "--silent", action="store_true", type=None, dest="SilentMode",
            help="Make use of silent mode of (n)make.")
        Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
        Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed, "\
                                                                                   "including library instances selected, final dependency expression, "\
                                                                                   "and warning messages, etc.")
        Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")
        Parser.add_option("-D", "--define", action="append", type="string", dest="Macros", help="Macro: \"Name [= Value]\".")

        Parser.add_option("-y", "--report-file", action="store", dest="ReportFile", help="Create/overwrite the report to the specified filename.")
        Parser.add_option("-Y", "--report-type", action="append", type="choice", choices=['PCD', 'LIBRARY', 'FLASH', 'DEPEX', 'BUILD_FLAGS', 'FIXED_ADDRESS', 'HASH', 'EXECUTION_ORDER'], dest="ReportType", default=[],
            help="Flags that control the type of build report to generate.  Must be one of: [PCD, LIBRARY, FLASH, DEPEX, BUILD_FLAGS, FIXED_ADDRESS, HASH, EXECUTION_ORDER].  "\
                 "To specify more than one flag, repeat this option on the command line and the default flag set is [PCD, LIBRARY, FLASH, DEPEX, HASH, BUILD_FLAGS, FIXED_ADDRESS]")
        Parser.add_option("-F", "--flag", action="store", type="string", dest="Flag",
            help="Specify the specific option to parse EDK UNI file. Must be one of: [-c, -s]. -c is for EDK framework UNI file, and -s is for EDK UEFI UNI file. "\
                 "This option can also be specified by setting *_*_*_BUILD_FLAGS in [BuildOptions] section of platform DSC. If they are both specified, this value "\
                 "will override the setting in [BuildOptions] section of platform DSC.")
        Parser.add_option("-N", "--no-cache", action="store_true", dest="DisableCache", default=False, help="Disable build cache mechanism")
        Parser.add_option("--conf", action="store", type="string", dest="ConfDirectory", help="Specify the customized Conf directory.")
        Parser.add_option("--check-usage", action="store_true", dest="CheckUsage", default=False, help="Check usage content of entries listed in INF file.")
        Parser.add_option("--ignore-sources", action="store_true", dest="IgnoreSources", default=False, help="Focus to a binary build and ignore all source files")
        Parser.add_option("--pcd", action="append", dest="OptionPcd", help="Set PCD value by command line. Format: \"PcdName=Value\" ")
        Parser.add_option("-l", "--cmd-len", action="store", type="int", dest="CommandLength", help="Specify the maximum line length of build command. Default is 4096.")
        Parser.add_option("--hash", action="store_true", dest="UseHashCache", default=False, help="Enable hash-based caching during build process.")
        Parser.add_option("--binary-destination", action="store", type="string", dest="BinCacheDest", help="Generate a cache of binary files in the specified directory.")
        Parser.add_option("--binary-source", action="store", type="string", dest="BinCacheSource", help="Consume a cache of binary files from the specified directory.")
        Parser.add_option("--genfds-multi-thread", action="store_true", dest="GenfdsMultiThread", default=True, help="Enable GenFds multi thread to generate ffs file.")
        Parser.add_option("--no-genfds-multi-thread", action="store_true", dest="NoGenfdsMultiThread", default=False, help="Disable GenFds multi thread to generate ffs file.")
        Parser.add_option("--disable-include-path-check", action="store_true", dest="DisableIncludePathCheck", default=False, help="Disable the include path check for outside of package.")
        self.BuildOption, self.BuildTarget = Parser.parse_args()
