import unittest
import os
from Parser.DscRecipe import OpenDsc

class TestDscParser(unittest.TestCase):
    def GetWorkspace(self):
        mypath = os.path.realpath(__file__)
        while os.path.basename(mypath) != "BaseTools":
            mypath = os.path.dirname(mypath)
        return os.path.dirname(mypath)

    def setUp(self):
        workspace = self.GetWorkspace()
        packages_path = [workspace]
        arch = 'IA32'
        dsc_path = os.path.join(workspace,"OvmfPkg/OvmfPkgIa32.dsc")
        build_target = 'DEBUG'
        tool_chain_tag = 'VS2015x86'
        tool_family = 'MSFT'
        cmd_macro = {'TPM2_ENABLE':"TRUE","TPM2_CONFIG_ENABLE":"TRUE","DEBUG_ON_SERIAL_PORT":"1"}
        cmd_pcd = []
        self.dsc_handle = OpenDsc(dsc_path, arch, workspace, packages_path,build_target, tool_chain_tag, tool_family, cmd_macro, cmd_pcd)

    def tearDown(self):
        unittest.TestCase.tearDown(self)

    def testDefine(self):
        for item in self.dsc_handle.ReadDefines():
            if item.name == "PLATFORM_VERSION":
                self.assertEqual(item.value,"0.1")
            if item.name == "PLATFORM_NAME":
                self.assertEqual(item.value, "Ovmf")
            if item.name == "FLASH_DEFINITION":
                self.assertEqual(item.value, r"C:\BobFeng\ToolDev\EDKIITrunk\BobEdk2\edk2\OvmfPkg\OvmfPkgIa32.fdf")
            if item.name == "BUILD_TARGETS":
                self.assertEqual(item.value, ['NOOPT', 'DEBUG', 'RELEASE'])
            if item.name == "SKUID_IDENTIFIER":
                self.assertEqual(item.value, "DEFAULT")
            if item.name == "OUTPUT_DIRECTORY":
                self.assertEqual(item.value, r"Build\OvmfIa32")
            if item.name == "PLATFORM_GUID":
                self.assertEqual(item.value, "5a9e7754-d81b-49ea-85ad-69eaa7b1539b")
            if item.name == "DSC_SPECIFICATION":
                self.assertEqual(item.value, "0x00010005")
            if item.name == "SUPPORTED_ARCHITECTURES":
                self.assertEqual(item.value, ['IA32'])

    def testPcds(self):
        Pcds = self.dsc_handle.ReadPcds()
        for pcd_sec in Pcds:
            if pcd_sec.pcd_type == 'FEATUREFLAG':
                for pcd_obj in Pcds[pcd_sec]:
                    if pcd_obj.namespace == 'gEfiMdeModulePkgTokenSpaceGuid' and pcd_obj.name == "PcdHiiOsRuntimeSupport":
                        self.assertEqual(pcd_obj.value,'0')
                        break
                else:
                    self.fail("Pcd gEfiMdeModulePkgTokenSpaceGuid.PcdHiiOsRuntimeSupport is not found")
            if pcd_sec.pcd_type == 'FIXEDATBUILD':
                for pcd_obj in Pcds[pcd_sec]:
                    if pcd_obj.namespace == 'gEfiMdeModulePkgTokenSpaceGuid' and pcd_obj.name == "PcdStatusCodeMemorySize":
                        self.assertEqual(pcd_obj.value,"1")
                        break
                else:
                    self.fail("Pcd gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeMemorySize is not found")
            if pcd_sec.pcd_type == 'DYNAMICDEFAULT':
                for pcd_obj in Pcds[pcd_sec]:
                    if pcd_obj.namespace == 'gEfiMdeModulePkgTokenSpaceGuid' and pcd_obj.name == "PcdEmuVariableNvStoreReserved":
                        self.assertEqual(pcd_obj.value,"0")
                        break
                else:
                    self.fail("Pcd gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvStoreReserved is not found")

            #gEfiSecurityPkgTokenSpaceGuid.PcdTcgPhysicalPresenceInterfaceVer|L"TCG2_VERSION"|gTcg2ConfigFormSetGuid|0x0|"1.3"|NV,BS
            if pcd_sec.pcd_type == 'DYNAMICHII':
                for pcd_obj in Pcds[pcd_sec]:
                    if pcd_obj.namespace == 'gEfiSecurityPkgTokenSpaceGuid' and pcd_obj.name == "PcdTcgPhysicalPresenceInterfaceVer":
                        self.assertEqual(pcd_obj.default,'"1.3"')
                        self.assertEqual(pcd_obj.var_name, 'L"TCG2_VERSION"')
                        self.assertEqual(pcd_obj.var_guid, "gTcg2ConfigFormSetGuid")
                        self.assertEqual(pcd_obj.var_offset,"0x0")
                        self.assertEqual(pcd_obj.attributes, ['NV', 'BS'])
                        break
                else:
                    self.fail("Pcd gEfiSecurityPkgTokenSpaceGuid.PcdTcgPhysicalPresenceInterfaceVer is not found")


    def testComponet(self):
#         !if $(TPM2_ENABLE) == TRUE
#           OvmfPkg/Tcg/Tcg2Config/Tcg2ConfigPei.inf
#           SecurityPkg/Tcg/Tcg2Pei/Tcg2Pei.inf {
#             <LibraryClasses>
#               HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterPei.inf
#               NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
#               NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
#               NULL|SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
#               NULL|SecurityPkg/Library/HashInstanceLibSha512/HashInstanceLibSha512.inf
#               NULL|SecurityPkg/Library/HashInstanceLibSm3/HashInstanceLibSm3.inf
#           }
#         !endif
        libs = {
            ("HashLib", "SecurityPkg\\Library\\HashLibBaseCryptoRouter\\HashLibBaseCryptoRouterPei.inf"),
            ("NULL","SecurityPkg\\Library\\HashInstanceLibSha1\\HashInstanceLibSha1.inf"),
            ("NULL","SecurityPkg\\Library\\HashInstanceLibSha256\\HashInstanceLibSha256.inf"),
            ("NULL","SecurityPkg\\Library\\HashInstanceLibSha384\\HashInstanceLibSha384.inf"),
            ("NULL","SecurityPkg\\Library\\HashInstanceLibSha512\\HashInstanceLibSha512.inf"),
            ("NULL","SecurityPkg\\Library\\HashInstanceLibSm3\\HashInstanceLibSm3.inf"),
            }

        components = self.dsc_handle.ReadComponents()
        for comp_set in components.values():
            for comp in comp_set:
                if comp.inf.File == "SecurityPkg\\Tcg\\Tcg2Pei\\Tcg2Pei.inf":
                    for libclass in comp.library_classes:
                        thislibclass = "NULL" if "NULL" in libclass.libraryclass else libclass.libraryclass
                        self.assertIn((thislibclass,libclass.inf.File), libs)
                    break
            else:
                self.fail(r"OvmfPkg/Tcg/Tcg2Config/Tcg2ConfigPei.inf is not found")

    def testLibClass(self):
#         [LibraryClasses.common.SEC]
#           TimerLib|OvmfPkg/Library/AcpiTimerLib/BaseRomAcpiTimerLib.inf
#           QemuFwCfgLib|OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgSecLib.inf
#         !ifdef $(DEBUG_ON_SERIAL_PORT)
#           DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
#         !else
        LibCSet = self.dsc_handle.ReadLibClasses()
        for item_sec in LibCSet:
            if item_sec.module_type != "SEC":
                continue
            for libc in LibCSet[item_sec]:
                if libc.inf.File == "MdePkg\\Library\\BaseDebugLibSerialPort\\BaseDebugLibSerialPort.inf":
                    self.assertEqual(item_sec.module_type, "SEC")
                    self.assertEqual(libc.libraryclass, "DebugLib")
                    break
            else:
                self.fail("BaseDebugLibSerialPort is not found. Check DEBUG_ON_SERIAL_PORT Macro")

    def testBuildOptions(self):
#         [BuildOptions]
#           GCC:RELEASE_*_*_CC_FLAGS             = -DMDEPKG_NDEBUG
#           INTEL:RELEASE_*_*_CC_FLAGS           = /D MDEPKG_NDEBUG
#           MSFT:RELEASE_*_*_CC_FLAGS            = /D MDEPKG_NDEBUG
#         !if $(TOOL_CHAIN_TAG) != "XCODE5" && $(TOOL_CHAIN_TAG) != "CLANGPDB"
#           GCC:*_*_*_CC_FLAGS                   = -mno-mmx -mno-sse
#         !endif
        BuildOptions = self.dsc_handle.ReadBuildOptions()
        for bosec in BuildOptions:
            if bosec.module_type == "COMMON":
                for bo in BuildOptions[bosec]:
                    if bo.data == "-mno-mmx -mno-sse":
                        self.assertEqual(bo.family, "GCC")
                        self.assertEqual(bo.target, "*")
                        self.assertEqual(bo.tagname, "*")
                        self.assertEqual(bo.arch, "*")
                        self.assertEqual(bo.tool_code, "CC")
                        self.assertEqual(bo.attribute, "FLAGS")
                        break
                else:
                    self.fail("Check TOOL_CHAIN_TAG")
            if bosec.module_type == "DXE_SMM_DRIVER":
                for bo in BuildOptions[bosec]:
                    if bo.data == "-z common-page-size=0x1000":
                        self.assertEqual(bo.family, "GCC")
                        self.assertEqual(bo.target, "*")
                        self.assertEqual(bo.tagname, "*")
                        self.assertEqual(bo.arch, "*")
                        self.assertEqual(bo.tool_code, "DLINK")
                        self.assertEqual(bo.attribute, "FLAGS")
                        break
                else:
                    self.fail("not found -z common-page-size=0x1000")

    def testSkus(self):
        for item in self.dsc_handle.ReadSkus():
            if item.id == 0:
                self.assertEqual(item.name,"DEFAULT")
                self.assertEqual(item.parent,"DEFAULT")
                break
        else:
            self.fail("No id 0 sku")
