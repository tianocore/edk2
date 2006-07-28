/** @file
 
 The file is used to init all JComboBox items
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common;

import java.util.Vector;

public class EnumerationData {
    //
    // Static data
    //
    public final static String EXTERNS_PCD_IS_DRIVER = "Pcd Is Driver";
    
    public final static String EXTERNS_SPECIFICATION = "Specification";
    
    public final static String EXTERNS_MODULE_ENTRY_POINT = "ModuleEntryPoint";
    public final static String EXTERNS_MODULE_UNLOAD_IMAGE = "ModuleUnloadImage";
    
    public final static String EXTERNS_CONSTRUCTOR = "Constructor";
    public final static String EXTERNS_DESTRUCTOR = "Destructor";
    
    public final static String EXTERNS_DRIVER_BINDING = "DriverBinding";
    public final static String EXTERNS_COMPONENT_NAME = "ComponentName";
    public final static String EXTERNS_DRIVER_CONFIG = "DriverConfig";
    public final static String EXTERNS_DRIVER_DIAG = "DriverDiag";
    
    public final static String EXTERNS_SET_VIRTUAL_ADDRESS_MAP_CALL_BACK = "SetVirtualAddressMapCallBack";
    public final static String EXTERNS_EXIT_BOOT_SERVICES_CALL_BACK = "ExitBootServicesCallBack";
    
    //
    // Common data
    //
    public Vector<String> vSupportedArchitectures = new Vector<String>();
    
    public Vector<String> vEnabled = new Vector<String>();
    
    public Vector<String> vBoolean = new Vector<String>();
    
    //
    // Used by Msa Header
    //
    public Vector<String> vModuleType = new Vector<String>();
    
    public Vector<String> vCompontentType = new Vector<String>();
    
    //
    // Used by Library Class Definitions
    //
    public Vector<String> vLibraryUsage = new Vector<String>();
    
    public Vector<String> vFrameworkModuleTypes = new Vector<String>();
    
    public Vector<String> vLibClassDef = new Vector<String>();
    
    public Vector<String> vLibClassDefBase = new Vector<String>();
    
    public Vector<String> vLibClassDefPei = new Vector<String>();
    
    public Vector<String> vLibClassDefPeim = new Vector<String>();
    
    public Vector<String> vLibClassDefDxeCore = new Vector<String>();
    
    public Vector<String> vLibClassDefDxeDriver = new Vector<String>();
    
    public Vector<String> vLibClassDefDxeSmmDriver = new Vector<String>();
    
    public Vector<String> vLibClassDefUefiDriver = new Vector<String>();
    
    //
    // Used by Source Files
    //
    public Vector<String> vSourceFilesToolChainFamily = new Vector<String>();
    
    public Vector<String> vSourceFilesFileType = new Vector<String>();
    
    public Vector<String> vToolCode = new Vector<String>();
    
    //
    // Used by Package Dependencies
    //
    public Vector<String> vPackageUsage = new Vector<String>();
    
    //
    // Used by Protocols
    //
    public Vector<String> vProtocolUsage = new Vector<String>();
    
    public Vector<String> vProtocolNotifyUsage = new Vector<String>();
    
    public Vector<String> vProtocolType = new Vector<String>();
    
    //
    // Used by Events
    //
    public Vector<String> vEventType = new Vector<String>();
    
    public Vector<String> vEventUsage = new Vector<String>();
    
    public Vector<String> vEventGroup = new Vector<String>();
    
    //
    // Used by Hobs
    //
    public Vector<String> vHobType = new Vector<String>();
    
    public Vector<String> vHobUsage = new Vector<String>();
    
    //
    // Used by Ppis
    //
    public Vector<String> vPpiType = new Vector<String>();
    
    public Vector<String> vPpiUsage = new Vector<String>();
    
    public Vector<String> vPpiNotifyUsage = new Vector<String>();
    
    //
    // Used by Variable
    //
    public Vector<String> vVariableUsage = new Vector<String>();
    
    //
    // Used by Boot Mode
    //
    public Vector<String> vBootModeNames = new Vector<String>();
    
    public Vector<String> vBootModeUsage = new Vector<String>();
   
    //
    // Used by System Tables
    //
    public Vector<String> vSystemTableUsage = new Vector<String>();
    
    //
    // Used by Data Hubs
    //
    public Vector<String> vDataHubUsage = new Vector<String>();
    
    //
    // Used by Hii Packages
    //
    public Vector<String> vHiiPackageUsage = new Vector<String>();
    
    //
    // Used by Guid
    //
    public Vector<String> vGuidUsage = new Vector<String>();
    
    //
    // Used by Externs
    //
    public Vector<String> vExternTypes = new Vector<String>();
    
    public Vector<String> vPcdDriverTypes = new Vector<String>();
    
    //
    // Used by Pcd
    //
    public Vector<String> vPcdItemTypes = new Vector<String>();

    public EnumerationData() {
        init();
    }
    
    private void init() {
        //
        // Init common data first
        //
        initSupportedArchitectures();
        initEnabled();
        initBoolean();
        
        //
        // Used by Msa header
        //
        initModuleType();
        
        //
        // Used by Library Class Definitions
        //
        initLibraryUsage();
        initFrameworkModuleTypes();
        initLibClassDefBase();
        initLibClassDefPei();
        initLibClassDefPeim();
        initLibClassDefDxeCore();
        initLibClassDefDxeDriver();
        initLibClassDefDxeSmmDriver();
        initLibClassDefUefiDriver();
        initLibClassDef();

        //
        // Used by Library Class Definitions
        //
        initSourceFilesToolChainFamily();
        initSourceFilesFileType();
        initToolCode();
        
        //
        // Used by Package Dependencies
        //
        initPackageUsage();
        
        //
        // Used by Protocols
        //
        initProtocolType();
        initProtocolUsage();
        initProtocolNotifyUsage();
        
        //
        // Used by Events
        //
        initEventType();
        initEventUsage();
        initEventGroup();
        
        //
        // Used by Hobs
        //
        initHobType();
        initHobUsage();
        
        //
        // Used by Ppis
        //
        initPpiType();
        initPpiUsage();
        initPpiNotifyUsage();
        
        //
        // Used by Variable
        //
        initVariableUsage();
        
        //
        // Used by Boot Mode
        //
        initBootModeNames();
        initBootModeUsage();
        
        //
        // Used by System Tables
        //
        initSystemTableUsage();
        
        //
        // Used by Data Hubs
        //
        initDataHubUsage();
        
        //
        // Used by Hii Packages
        //
        initHiiPackages();
        
        //
        // Used by Guid
        //
        initGuidUsage();
        
        //
        // Used by Externs
        //
        initExternTypes();
        initPcdDriverTypes();
        
        //
        // Used by Pcd
        //
        initPcdItemTypes();
        
    }
    
    private void initEnabled() {
        vEnabled.removeAllElements();
        vEnabled.addElement("Disabled");
        vEnabled.addElement("Enabled");
    }
    
    private void initBoolean() {
        vBoolean.removeAllElements();
        vBoolean.addElement("False");
        vBoolean.addElement("True");
    }
    
    private void initModuleType() {
        vModuleType.removeAllElements();
        vModuleType.addElement("BASE");
        vModuleType.addElement("SEC");
        vModuleType.addElement("PEI_CORE");
        vModuleType.addElement("PEIM");
        vModuleType.addElement("DXE_CORE");
        vModuleType.addElement("DXE_DRIVER");
        vModuleType.addElement("DXE_RUNTIME_DRIVER");
        vModuleType.addElement("DXE_SAL_DRIVER");
        vModuleType.addElement("DXE_SMM_DRIVER");
        vModuleType.addElement("TOOL");
        vModuleType.addElement("UEFI_DRIVER");
        vModuleType.addElement("UEFI_APPLICATION");
        vModuleType.addElement("USER_DEFINED");
        Sort.sortVectorString(vModuleType, DataType.SORT_TYPE_ASCENDING);
    }
    
//    private void initComponentType() {
//        vCompontentType.removeAllElements();
//        vCompontentType.addElement("APRIORI");
//        vCompontentType.addElement("LIBRARY");
//        vCompontentType.addElement("FV_IMAGE_FILE");
//        vCompontentType.addElement("BS_DRIVER");
//        vCompontentType.addElement("RT_DRIVER");
//        vCompontentType.addElement("SAL_RT_DRIVER");
//        vCompontentType.addElement("PE32_PEIM");
//        vCompontentType.addElement("PIC_PEIM");
//        vCompontentType.addElement("COMBINED_PEIM_DRIVER");
//        vCompontentType.addElement("PEI_CORE");
//        vCompontentType.addElement("DXE_CORE");
//        vCompontentType.addElement("APPLICATION");
//        vCompontentType.addElement("BS_DRIVER_EFI");
//        vCompontentType.addElement("SHELLAPP");
//    }
    
    private void initSupportedArchitectures() {
        vSupportedArchitectures.removeAllElements();
        vSupportedArchitectures.addElement("EBC");
        vSupportedArchitectures.addElement("IA32");
        vSupportedArchitectures.addElement("X64");
        vSupportedArchitectures.addElement("IPF");
        vSupportedArchitectures.addElement("ARM");
        vSupportedArchitectures.addElement("PPC");
    }
    
    private void initLibraryUsage() {
        vLibraryUsage.removeAllElements();
        vLibraryUsage.addElement("ALWAYS_CONSUMED");
        vLibraryUsage.addElement("SOMETIMES_CONSUMED");
        vLibraryUsage.addElement("ALWAYS_PRODUCED");
        vLibraryUsage.addElement("SOMETIMES_PRODUCED");
        vLibraryUsage.addElement("DEFAULT");
        vLibraryUsage.addElement("PRIVATE");
    }
    
    private void initFrameworkModuleTypes() {
        vFrameworkModuleTypes.removeAllElements();
        vFrameworkModuleTypes.addElement("BASE");
        vFrameworkModuleTypes.addElement("SEC");
        vFrameworkModuleTypes.addElement("PEI_CORE");
        vFrameworkModuleTypes.addElement("PEIM");
        vFrameworkModuleTypes.addElement("DXE_CORE");
        vFrameworkModuleTypes.addElement("DXE_DRIVER");
        vFrameworkModuleTypes.addElement("DXE_RUNTIME_DRIVER");
        vFrameworkModuleTypes.addElement("DXE_SAL_DRIVER");
        vFrameworkModuleTypes.addElement("DXE_SMM_DRIVER");
        vFrameworkModuleTypes.addElement("TOOL");
        vFrameworkModuleTypes.addElement("UEFI_DRIVER");
        vFrameworkModuleTypes.addElement("UEFI_APPLICATION");
        vFrameworkModuleTypes.addElement("USER_DEFINED");
        Sort.sortVectorString(vFrameworkModuleTypes, DataType.SORT_TYPE_ASCENDING);
    }

    private void initLibClassDef() {
        vLibClassDef.removeAllElements();
        for (int index = 0; index < vLibClassDefBase.size(); index++) {
            vLibClassDef.addElement(vLibClassDefBase.elementAt(index));
        }
        for (int index = 0; index < vLibClassDefPei.size(); index++) {
            vLibClassDef.addElement(vLibClassDefPei.elementAt(index));
        }
        for (int index = 0; index < vLibClassDefPeim.size(); index++) {
            vLibClassDef.addElement(vLibClassDefPeim.elementAt(index));
        }
        for (int index = 0; index < vLibClassDefDxeCore.size(); index++) {
            vLibClassDef.addElement(vLibClassDefDxeCore.elementAt(index));
        }
        for (int index = 0; index < vLibClassDefDxeDriver.size(); index++) {
            vLibClassDef.addElement(vLibClassDefDxeDriver.elementAt(index));
        }
        for (int index = 0; index < vLibClassDefDxeSmmDriver.size(); index++) {
            vLibClassDef.addElement(vLibClassDefDxeSmmDriver.elementAt(index));
        }
        for (int index = 0; index < vLibClassDefUefiDriver.size(); index++) {
            vLibClassDef.addElement(vLibClassDefUefiDriver.elementAt(index));
        }
    }
    
    private void initLibClassDefBase() {
        vLibClassDefBase.removeAllElements();
        vLibClassDefBase.addElement("BaseLib");
        vLibClassDefBase.addElement("BaseMemoryLib");
        vLibClassDefBase.addElement("CacheMaintenanceLib");
        vLibClassDefBase.addElement("DebugLib");
        vLibClassDefBase.addElement("IoLib");
        vLibClassDefBase.addElement("PcdLib");
        vLibClassDefBase.addElement("PciCf8Lib");
        vLibClassDefBase.addElement("PciExpressLib");
        vLibClassDefBase.addElement("PciLib");
        vLibClassDefBase.addElement("PeCoffGetEntryPointLib");
        vLibClassDefBase.addElement("PeCoffLib");
        vLibClassDefBase.addElement("PerformanceLib");
        vLibClassDefBase.addElement("PrintLib");
        vLibClassDefBase.addElement("SmbusLib");
        vLibClassDefBase.addElement("TimerLib");
    }
    
    private void initLibClassDefPei() {
        vLibClassDefPei.removeAllElements();
        vLibClassDefPei.addElement("PeiCoreEntryPoint");
    }
    
    private void initLibClassDefPeim() {
        vLibClassDefPeim.removeAllElements();
        vLibClassDefPeim.addElement("BaseMemoryLib");
        vLibClassDefPeim.addElement("DebugLib");
        vLibClassDefPeim.addElement("HobLib");
        vLibClassDefPeim.addElement("IoLib");
        vLibClassDefPeim.addElement("MemoryAllocationLib");
        vLibClassDefPeim.addElement("PcdLib");
        vLibClassDefPeim.addElement("PeiCoreLib");
        vLibClassDefPeim.addElement("PeiServicesTablePointerLib");
        vLibClassDefPeim.addElement("PeimEntryPoint");
        vLibClassDefPeim.addElement("ReportStatusCodeLib");
        vLibClassDefPeim.addElement("ResourcePublicationLib");
        vLibClassDefPeim.addElement("SmbusLib");
    }
    
    private void initLibClassDefDxeCore() {
        vLibClassDefDxeCore.removeAllElements();
        vLibClassDefDxeCore.addElement("DxeCoreEntryPoint");
        vLibClassDefDxeCore.addElement("HobLib");
    }
    
    private void initLibClassDefDxeDriver() {
        vLibClassDefDxeDriver.removeAllElements();
        vLibClassDefDxeDriver.addElement("DxeServicesTableLib");
        vLibClassDefDxeDriver.addElement("HiiLib");
        vLibClassDefDxeDriver.addElement("HobLib");
        vLibClassDefDxeDriver.addElement("IoLib");
        vLibClassDefDxeDriver.addElement("MemoryAllocationLib");
        vLibClassDefDxeDriver.addElement("PcdLib");
        vLibClassDefDxeDriver.addElement("ReportStatusCodeLib");
        vLibClassDefDxeDriver.addElement("SmbusLib");
        vLibClassDefDxeDriver.addElement("UefiBootServicesTableLib");
        vLibClassDefDxeDriver.addElement("UefiDecompressLib");
        vLibClassDefDxeDriver.addElement("UefiRuntimeServicesTableLib");
    }
    
    private void initLibClassDefDxeSmmDriver() {
        vLibClassDefDxeSmmDriver.removeAllElements();
        vLibClassDefDxeSmmDriver.addElement("DxeSmmDriverEntryPoint");
    }
    
    private void initLibClassDefUefiDriver() {
        vLibClassDefUefiDriver.removeAllElements();
        vLibClassDefUefiDriver.addElement("BaseMemoryLib");
        vLibClassDefUefiDriver.addElement("DebugLib");
        vLibClassDefUefiDriver.addElement("DevicePathLib");
        vLibClassDefUefiDriver.addElement("UefiDriverEntryPoint");
        vLibClassDefUefiDriver.addElement("UefiDriverModelLib");
        vLibClassDefUefiDriver.addElement("UefiLib");
    }
    
    private void initSourceFilesToolChainFamily() {
        vSourceFilesToolChainFamily.removeAllElements();
        vSourceFilesToolChainFamily.addElement("MSFT");
        vSourceFilesToolChainFamily.addElement("INTC");
        vSourceFilesToolChainFamily.addElement("GCC");
    }
    
    private void initSourceFilesFileType() {
        vSourceFilesFileType.removeAllElements();
        vSourceFilesFileType.addElement("CCODE");
        vSourceFilesFileType.addElement("CHEADER");
        vSourceFilesFileType.addElement("ASMHEADER");
        vSourceFilesFileType.addElement("ASM");
        vSourceFilesFileType.addElement("UNI");
        vSourceFilesFileType.addElement("TXT");
        vSourceFilesFileType.addElement("DXS");
        vSourceFilesFileType.addElement("BMP");
        vSourceFilesFileType.addElement("VFR");
        vSourceFilesFileType.addElement("BINARY");
        vSourceFilesFileType.addElement("FV");
        vSourceFilesFileType.addElement("FFS");
        vSourceFilesFileType.addElement("EFI");
    }
    
    private void initToolCode() {
        vToolCode.removeAllElements();
        vToolCode.addElement(DataType.EMPTY_SELECT_ITEM);
        vToolCode.addElement("CC");
        vToolCode.addElement("DLINK");
        vToolCode.addElement("SLINK");
        vToolCode.addElement("PP");
        vToolCode.addElement("ASM");
        vToolCode.addElement("ASMLINK");
        vToolCode.addElement("ASL");
    }
    
    private void initPackageUsage() {
        vPackageUsage.removeAllElements();
        vPackageUsage.addElement("ALWAYS_CONSUMED");
        vPackageUsage.addElement("ALWAYS_PRODUCED");
        vPackageUsage.addElement("DEFAULT");
    }
    
    private void initProtocolUsage() {
        vProtocolUsage.removeAllElements();
        vProtocolUsage.addElement("ALWAYS_CONSUMED");
        vProtocolUsage.addElement("SOMETIMES_CONSUMED");
        vProtocolUsage.addElement("ALWAYS_PRODUCED");
        vProtocolUsage.addElement("SOMETIMES_PRODUCED");
        vProtocolUsage.addElement("TO_START");
        vProtocolUsage.addElement("BY_START");
        vProtocolUsage.addElement("PRIVATE");
    }
    
    private void initProtocolType() {
        vProtocolType.removeAllElements();
        vProtocolType.addElement("Protocol");
        vProtocolType.addElement("Protocol Notify");
    }
    
    private void initEventType() {
        vEventType.removeAllElements();
        vEventType.addElement("CreateEvents");
        vEventType.addElement("SignalEvents");
    }
    
    private void initEventUsage() {
        vEventUsage.removeAllElements();
        vEventUsage.addElement("ALWAYS_CONSUMED");
        vEventUsage.addElement("SOMETIMES_CONSUMED");
        vEventUsage.addElement("ALWAYS_PRODUCED");
        vEventUsage.addElement("SOMETIMES_PRODUCED");
        vEventUsage.addElement("PRIVATE");
    }
    
    private void initEventGroup() {
        vEventGroup.removeAllElements();
        vEventGroup.addElement("EVENT_GROUP_GUID");
        vEventGroup.addElement("EVENT_TYPE_PERIODIC_TIMER");
        vEventGroup.addElement("EVENT_TYPE_RELATIVE_TIMER");
    }
    
    private void initHobType() {
        vHobType.removeAllElements();
        vHobType.addElement("PHIT");
        vHobType.addElement("MEMORY_ALLOCATION");
        vHobType.addElement("RESOURCE_DESCRIPTOR");
        vHobType.addElement("GUID_EXTENSION");
        vHobType.addElement("FIRMWARE_VOLUME");
        vHobType.addElement("CPU");
        vHobType.addElement("POOL");
        vHobType.addElement("CAPSULE_VOLUME");
    }
    
    private void initHobUsage() {
        vHobUsage.removeAllElements();
        vHobUsage.addElement("ALWAYS_CONSUMED");
        vHobUsage.addElement("SOMETIMES_CONSUMED");
        vHobUsage.addElement("ALWAYS_PRODUCED");
        vHobUsage.addElement("SOMETIMES_PRODUCED");
        vHobUsage.addElement("PRIVATE");
    }
    
    private void initPpiType() {
        vPpiType.removeAllElements();
        vPpiType.addElement("Ppi");
        vPpiType.addElement("Ppi Notify");
    }
    
    private void initPpiUsage() {
        vPpiUsage.removeAllElements();
        vPpiUsage.addElement("ALWAYS_CONSUMED");
        vPpiUsage.addElement("SOMETIMES_CONSUMED");
        vPpiUsage.addElement("ALWAYS_PRODUCED");
        vPpiUsage.addElement("SOMETIMES_PRODUCED");
        vPpiUsage.addElement("PRIVATE");
    }
    
    private void initPpiNotifyUsage() {
        vPpiNotifyUsage.removeAllElements();
        vPpiNotifyUsage.addElement("SOMETIMES_CONSUMED");
    }
    
    private void initProtocolNotifyUsage() {
        vProtocolNotifyUsage.addElement("SOMETIMES_CONSUMED");
    }
    
    private void initVariableUsage() {
        vVariableUsage.removeAllElements();
        vVariableUsage.addElement("ALWAYS_CONSUMED");
        vVariableUsage.addElement("SOMETIMES_CONSUMED");
        vVariableUsage.addElement("ALWAYS_PRODUCED");
        vVariableUsage.addElement("SOMETIMES_PRODUCED");
        vVariableUsage.addElement("PRIVATE");
    }
    
    private void initBootModeNames() {
        vBootModeNames.removeAllElements();
        vBootModeNames.addElement("FULL");
        vBootModeNames.addElement("MINIMAL");
        vBootModeNames.addElement("NO_CHANGE");
        vBootModeNames.addElement("DIAGNOSTICS");
        vBootModeNames.addElement("DEFAULT");
        vBootModeNames.addElement("S2_RESUME");
        vBootModeNames.addElement("S3_RESUME");
        vBootModeNames.addElement("S4_RESUME");
        vBootModeNames.addElement("S5_RESUME");
        vBootModeNames.addElement("FLASH_UPDATE");
        vBootModeNames.addElement("RECOVERY");
    }
    
    private void initBootModeUsage() {
        vBootModeUsage.removeAllElements();
        vBootModeUsage.addElement("ALWAYS_CONSUMED");
        vBootModeUsage.addElement("SOMETIMES_CONSUMED");
        vBootModeUsage.addElement("ALWAYS_PRODUCED");
        vBootModeUsage.addElement("SOMETIMES_PRODUCED");
    }
    
    private void initSystemTableUsage() {
        vSystemTableUsage.removeAllElements();
        vSystemTableUsage.addElement("ALWAYS_CONSUMED");
        vSystemTableUsage.addElement("SOMETIMES_CONSUMED");
        vSystemTableUsage.addElement("ALWAYS_PRODUCED");
        vSystemTableUsage.addElement("SOMETIMES_PRODUCED");
        vSystemTableUsage.addElement("PRIVATE");
    }
    
    private void initDataHubUsage() {
        vDataHubUsage.removeAllElements();
        vDataHubUsage.addElement("ALWAYS_CONSUMED");
        vDataHubUsage.addElement("SOMETIMES_CONSUMED");
        vDataHubUsage.addElement("ALWAYS_PRODUCED");
        vDataHubUsage.addElement("SOMETIMES_PRODUCED");
        vDataHubUsage.addElement("PRIVATE");
    }
    
    private void initHiiPackages() {
        vHiiPackageUsage.removeAllElements();
        vHiiPackageUsage.addElement("ALWAYS_PRODUCED");
        vHiiPackageUsage.addElement("SOMETIMES_PRODUCED");
        vHiiPackageUsage.addElement("PRIVATE");
    }
    
    private void initGuidUsage() {
        vGuidUsage.removeAllElements();
        vGuidUsage.addElement("ALWAYS_CONSUMED");
        vGuidUsage.addElement("SOMETIMES_CONSUMED");
        vGuidUsage.addElement("ALWAYS_PRODUCED");
        vGuidUsage.addElement("SOMETIMES_PRODUCED");
        vGuidUsage.addElement("DEFAULT");
        vGuidUsage.addElement("PRIVATE");
    }
    
    private void initExternTypes() {
        vExternTypes.removeAllElements();
        
        vExternTypes.addElement(EnumerationData.EXTERNS_PCD_IS_DRIVER);

        vExternTypes.addElement(EnumerationData.EXTERNS_SPECIFICATION);
        
        vExternTypes.addElement(EnumerationData.EXTERNS_MODULE_ENTRY_POINT);
        vExternTypes.addElement(EnumerationData.EXTERNS_MODULE_UNLOAD_IMAGE);
        
        vExternTypes.addElement(EnumerationData.EXTERNS_CONSTRUCTOR);
        vExternTypes.addElement(EnumerationData.EXTERNS_DESTRUCTOR);
        
        vExternTypes.addElement(EnumerationData.EXTERNS_DRIVER_BINDING);
        vExternTypes.addElement(EnumerationData.EXTERNS_COMPONENT_NAME);
        vExternTypes.addElement(EnumerationData.EXTERNS_DRIVER_CONFIG);
        vExternTypes.addElement(EnumerationData.EXTERNS_DRIVER_DIAG);
        
        vExternTypes.addElement(EnumerationData.EXTERNS_SET_VIRTUAL_ADDRESS_MAP_CALL_BACK);
        vExternTypes.addElement(EnumerationData.EXTERNS_EXIT_BOOT_SERVICES_CALL_BACK);
    }
    
    private void initPcdDriverTypes() {
        vPcdDriverTypes.removeAllElements();
        vPcdDriverTypes.addElement(DataType.EMPTY_SELECT_ITEM);
        vPcdDriverTypes.addElement("PEI_PCD_DRIVER");
        vPcdDriverTypes.addElement("DXE_PCD_DRIVER");
    }
    
    private void initPcdItemTypes() {
        vPcdItemTypes.removeAllElements();
        vPcdItemTypes.addElement("FEATURE_FLAG");
        vPcdItemTypes.addElement("FIXED_AT_BUILD");
        vPcdItemTypes.addElement("PATCHABLE_IN_MODULE");
        vPcdItemTypes.addElement("DYNAMIC");
        vPcdItemTypes.addElement("DYNAMIC_EX");
    }
    
    public Vector<String> getvCompontentType() {
        return vCompontentType;
    }

    public void setvCompontentType(Vector<String> componentType) {
        vCompontentType = componentType;
    }

    public Vector<String> getVModuleType() {
        return vModuleType;
    }

    public void setVModuleType(Vector<String> moduleType) {
        vModuleType = moduleType;
    }

    public Vector<String> getVLibraryUsage() {
        return vLibraryUsage;
    }

    public void setVLibClassDefUsage(Vector<String> libClassDefUsage) {
        vLibraryUsage = libClassDefUsage;
    }

    public Vector<String> getVLibClassDef() {
        return vLibClassDef;
    }

    public void setVLibClassDef(Vector<String> libClassDef) {
        vLibClassDef = libClassDef;
    }

    public Vector<String> getVCompontentType() {
        return vCompontentType;
    }

    public void setVCompontentType(Vector<String> compontentType) {
        vCompontentType = compontentType;
    }

    public Vector<String> getVLibClassDefBase() {
        return vLibClassDefBase;
    }

    public void setVLibClassDefBase(Vector<String> libClassDefBase) {
        vLibClassDefBase = libClassDefBase;
    }

    public Vector<String> getVLibClassDefDxeCore() {
        return vLibClassDefDxeCore;
    }

    public void setVLibClassDefDxeCore(Vector<String> libClassDefDxeCore) {
        vLibClassDefDxeCore = libClassDefDxeCore;
    }

    public Vector<String> getVLibClassDefDxeDriver() {
        return vLibClassDefDxeDriver;
    }

    public void setVLibClassDefDxeDriver(Vector<String> libClassDefDxeDriver) {
        vLibClassDefDxeDriver = libClassDefDxeDriver;
    }

    public Vector<String> getVLibClassDefDxeSmmDriver() {
        return vLibClassDefDxeSmmDriver;
    }

    public void setVLibClassDefDxeSmmDriver(Vector<String> libClassDefDxeSmmDriver) {
        vLibClassDefDxeSmmDriver = libClassDefDxeSmmDriver;
    }

    public Vector<String> getVLibClassDefPei() {
        return vLibClassDefPei;
    }

    public void setVLibClassDefPei(Vector<String> libClassDefPei) {
        vLibClassDefPei = libClassDefPei;
    }

    public Vector<String> getVLibClassDefPeim() {
        return vLibClassDefPeim;
    }

    public void setVLibClassDefPeim(Vector<String> libClassDefPeim) {
        vLibClassDefPeim = libClassDefPeim;
    }

    public Vector<String> getVLibClassDefUefiDriver() {
        return vLibClassDefUefiDriver;
    }

    public void setVLibClassDefUefiDriver(Vector<String> libClassDefUefiDriver) {
        vLibClassDefUefiDriver = libClassDefUefiDriver;
    }

    public Vector<String> getVSourceFilesFileType() {
        return vSourceFilesFileType;
    }

    public void setVSourceFilesFileType(Vector<String> sourceFilesFileType) {
        vSourceFilesFileType = sourceFilesFileType;
    }

    public Vector<String> getVSourceFilesToolChainFamily() {
        return vSourceFilesToolChainFamily;
    }

    public void setVSourceFilesToolChainFamily(Vector<String> sourceFilesToolChainFamily) {
        vSourceFilesToolChainFamily = sourceFilesToolChainFamily;
    }

    public void setVLibraryUsage(Vector<String> libraryUsage) {
        vLibraryUsage = libraryUsage;
    }

    public Vector<String> getVProtocolNotifyUsage() {
        return vProtocolNotifyUsage;
    }

    public void setVProtocolNotifyUsage(Vector<String> protocolNotifyUsage) {
        vProtocolNotifyUsage = protocolNotifyUsage;
    }

    public Vector<String> getVProtocolUsage() {
        return vProtocolUsage;
    }

    public void setVProtocolUsage(Vector<String> protocolUsage) {
        vProtocolUsage = protocolUsage;
    }

    public Vector<String> getVSupportedArchitectures() {
        return vSupportedArchitectures;
    }

    public void setVSupportedArchitectures(Vector<String> supportedArchitectures) {
        vSupportedArchitectures = supportedArchitectures;
    }

    public Vector<String> getVProtocolType() {
        return vProtocolType;
    }

    public void setVProtocolType(Vector<String> protocolType) {
        vProtocolType = protocolType;
    }

    public Vector<String> getVEventGroup() {
        return vEventGroup;
    }

    public void setVEventGroup(Vector<String> eventGroup) {
        vEventGroup = eventGroup;
    }

    public Vector<String> getVEventType() {
        return vEventType;
    }

    public void setVEventType(Vector<String> eventType) {
        vEventType = eventType;
    }

    public Vector<String> getVEventUsage() {
        return vEventUsage;
    }

    public void setVEventUsage(Vector<String> eventUsage) {
        vEventUsage = eventUsage;
    }

    public Vector<String> getVEnabled() {
        return vEnabled;
    }

    public void setVEnabled(Vector<String> enabled) {
        vEnabled = enabled;
    }

    public Vector<String> getVHobType() {
        return vHobType;
    }

    public void setVHobType(Vector<String> hobType) {
        vHobType = hobType;
    }

    public Vector<String> getVHobUsage() {
        return vHobUsage;
    }

    public void setVHobUsage(Vector<String> hobUsage) {
        vHobUsage = hobUsage;
    }

    public Vector<String> getVPpiNotifyUsage() {
        return vPpiNotifyUsage;
    }

    public void setVPpiNotifyUsage(Vector<String> ppiNotifyUsage) {
        vPpiNotifyUsage = ppiNotifyUsage;
    }

    public Vector<String> getVPpiType() {
        return vPpiType;
    }

    public void setVPpiType(Vector<String> ppiType) {
        vPpiType = ppiType;
    }

    public Vector<String> getVPpiUsage() {
        return vPpiUsage;
    }

    public void setVPpiUsage(Vector<String> ppiUsage) {
        vPpiUsage = ppiUsage;
    }

    public Vector<String> getVVariableUsage() {
        return vVariableUsage;
    }

    public void setVVariableUsage(Vector<String> variableUsage) {
        vVariableUsage = variableUsage;
    }

    public Vector<String> getVBootModeNames() {
        return vBootModeNames;
    }

    public void setVBootModeNames(Vector<String> bootModeNames) {
        vBootModeNames = bootModeNames;
    }

    public Vector<String> getVBootModeUsage() {
        return vBootModeUsage;
    }

    public void setVBootModeUsage(Vector<String> bootModeUsage) {
        vBootModeUsage = bootModeUsage;
    }

    public Vector<String> getVSystemTableUsage() {
        return vSystemTableUsage;
    }

    public void setVSystemTableUsage(Vector<String> systemTableUsage) {
        vSystemTableUsage = systemTableUsage;
    }

    public Vector<String> getVDataHubUsage() {
        return vDataHubUsage;
    }

    public void setVDataHubUsage(Vector<String> dataHubUsage) {
        vDataHubUsage = dataHubUsage;
    }

    public Vector<String> getVGuidUsage() {
        return vGuidUsage;
    }

    public void setVGuidUsage(Vector<String> guidUsage) {
        vGuidUsage = guidUsage;
    }

    public Vector<String> getVHiiPackageUsage() {
        return vHiiPackageUsage;
    }

    public void setVHiiPackageUsage(Vector<String> hiiPackageUsage) {
        vHiiPackageUsage = hiiPackageUsage;
    }

    public Vector<String> getVPcdItemTypes() {
        return vPcdItemTypes;
    }

    public void setVPcdItemTypes(Vector<String> pcdItemTypes) {
        vPcdItemTypes = pcdItemTypes;
    }

    public Vector<String> getVExternTypes() {
        return vExternTypes;
    }

    public void setVExternTypes(Vector<String> externTypes) {
        vExternTypes = externTypes;
    }

    public Vector<String> getVPcdDriverTypes() {
        return vPcdDriverTypes;
    }

    public void setVPcdDriverTypes(Vector<String> pcdDriverTypes) {
        vPcdDriverTypes = pcdDriverTypes;
    }

    public Vector<String> getVBoolean() {
        return vBoolean;
    }

    public void setVBoolean(Vector<String> boolean1) {
        vBoolean = boolean1;
    }

    public Vector<String> getVFrameworkModuleTypes() {
        return vFrameworkModuleTypes;
    }

    public void setVFrameworkModuleTypes(Vector<String> frameworkModuleTypes) {
        vFrameworkModuleTypes = frameworkModuleTypes;
    }

    public Vector<String> getVPackageUsage() {
        return vPackageUsage;
    }

    public void setVPackageUsage(Vector<String> packageUsage) {
        vPackageUsage = packageUsage;
    }

    public Vector<String> getVToolCode() {
        return vToolCode;
    }

    public void setVToolCode(Vector<String> toolCode) {
        vToolCode = toolCode;
    }
}
