/** @file
 AutoGen class.

 This class is to generate Autogen.h and Autogen.c according to module surface area
 or library surface area.

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.build.autogen;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.apache.tools.ant.BuildException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.build.exception.AutoGenException;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.pcd.action.PCDAutoGenAction;
import org.tianocore.common.definitions.ToolDefinitions;
import org.tianocore.common.definitions.EdkDefinitions;
import org.tianocore.common.exception.EdkException;
import org.tianocore.common.logger.EdkLog;

/**
  This class is to generate Autogen.h and Autogen.c according to module surface
  area or library surface area.
**/
public class AutoGen {
    ///
    /// The output path of Autogen.h and Autogen.c
    ///
    private String outputPath;

    ///
    /// The name of FV directory
    ///
    private String fvDir;

    ///
    /// The base name of module or library.
    ///
    private ModuleIdentification moduleId;

    ///
    /// The build architecture
    ///
    private String arch;

    ///
    /// PcdAutogen instance which is used to manage how to generate the PCD
    /// information.
    ///
    private PCDAutoGenAction myPcdAutogen;

    ///
    /// the one of type : NOT_PCD_DRIVER, PEI_PCD_DRIVER, DXE_PCD_DRIVER
    /// 
    private CommonDefinition.PCD_DRIVER_TYPE pcdDriverType;

    ///
    /// The Guid CName list which recoreded in module or library surface area 
    /// and it's dependence on library instance surface area.
    ///
    private Set<String> mGuidCNameList = new HashSet<String>();

    ///
    /// The dependence package list which recoreded in module or library surface
    /// area and it's dependence on library instance surface area.
    ///
    private List<PackageIdentification> mDepPkgList = new LinkedList<PackageIdentification>();

    ///
    ///  For non library module, add its library instance's construct and destructor to
    ///  list. String[0] recode LibConstructor name, String[1] recode Lib instance 
    ///  module type.
    ///
    private List<String[]> libConstructList = new ArrayList<String[]>();
    private List<String[]> libDestructList = new ArrayList<String[]>();

    ///
    /// List to store SetVirtalAddressMapCallBack, ExitBootServiceCallBack
    ///
    private List<String> setVirtalAddList = new ArrayList<String>();
    private List<String> exitBootServiceList = new ArrayList<String>();

    private StringBuffer functionDeclarations = new StringBuffer(10240);
    private StringBuffer globalDeclarations = new StringBuffer(10240);

    //
    // flag of PcdComponentNameDisable, PcdDriverDiagnosticDisable 
    //
    private boolean componentNamePcd = false;
    private boolean driverDiagnostPcd = false;

    //
    // Instance of SurfaceAreaQuery
    //
    private SurfaceAreaQuery saq = null;

    private ModuleIdentification parentId = null;

    /**
      Construct function
     
      This function mainly initialize some member variable.
      
      @param fvDir
                 Absolute path of FV directory.
      @param outputPath
                 Output path of AutoGen file.
      @param moduleId
                 Module identification.
      @param arch
                 Target architecture.
    **/
    public AutoGen(String fvDir, String outputPath, ModuleIdentification moduleId, String arch, SurfaceAreaQuery saq, ModuleIdentification parentId) {
        this.outputPath = outputPath;
        this.moduleId = moduleId;
        this.arch = arch;
        this.fvDir = fvDir;
        this.saq = saq;
        this.parentId = parentId;
    }

    /**
      saveFile function
     
      This function save the content in stringBuffer to file.
     
      @param fileName
                 The name of file.
      @param fileBuffer
                 The content of AutoGen file in buffer.
      @return boolean           
                 "true"  successful
                 "false" failed
    **/
    private boolean saveFile(String fileName, StringBuffer fileBuffer) {

        File autoGenH = new File(fileName);

        //
        // if the file exists, compare their content
        //
        if (autoGenH.exists()) {
            char[] oldFileBuffer = new char[(int) autoGenH.length()];
            try {
                FileReader fIn = new FileReader(autoGenH);
                fIn.read(oldFileBuffer, 0, (int) autoGenH.length());
                fIn.close();
            } catch (IOException e) {
                EdkLog.log(EdkLog.EDK_INFO, this.moduleId.getName() 
                           + "'s " 
                           + fileName 
                           + " is exist, but can't be open!!");
                return false;
            }

            //
            // if we got the same file, don't re-generate it to prevent
            // sources depending on it from re-building
            //
            if (fileBuffer.toString().compareTo(new String(oldFileBuffer)) == 0) {
                return true;
            }
        }

        try {
            FileWriter fOut = new FileWriter(autoGenH);
            fOut.write(fileBuffer.toString());
            fOut.flush();
            fOut.close();
        } catch (IOException e) {
            EdkLog.log(EdkLog.EDK_INFO, this.moduleId.getName() 
                       + "'s " 
                       + fileName 
                       + " can't be create!!");
            return false;
        }
        return true;
    }

    /**
      genAutogen function
     
      This function call libGenAutoGen or moduleGenAutogen function, which
      dependence on generate library autogen or module autogen.
     
      @throws BuildException
                  Failed to creat AutoGen.c & AutoGen.h.
    **/
    public void genAutogen() throws EdkException {
        //
        // If outputPath do not exist, create it.
        //
        File path = new File(outputPath);
        path.mkdirs();

        //
        // Check current is library or not, then call the corresponding
        // function.
        //
        if (this.moduleId.isLibrary()) {
            libGenAutogen();
        } else {
            moduleGenAutogen();
        }
    }

    /**
      moduleGenAutogen function
     
      This function generates AutoGen.c & AutoGen.h for module.
     
      @throws BuildException
                  Faile to create module AutoGen.c & AutoGen.h.
    **/
    void moduleGenAutogen() throws EdkException {
        setPcdComponentName();
        setPcdDriverDiagnostic();
        collectLibInstanceInfo();
        moduleGenAutogenC();
        moduleGenAutogenH();
    }

    /**
      libGenAutogen function
     
      This function generates AutoGen.c & AutoGen.h for library.
     
      @throws BuildException
                  Faile to create library AutoGen.c & AutoGen.h
    **/
    void libGenAutogen() throws EdkException {
        libGenAutogenC();
        libGenAutogenH();
    }

    /**
      moduleGenAutogenH
     
      This function generates AutoGen.h for module.
     
      @throws BuildException
                  Failed to generate AutoGen.h.
    **/
    void moduleGenAutogenH() throws EdkException {

        Set<String> libClassIncludeH;
        String moduleType;
        // List<String> headerFileList;
        List<String> headerFileList;
        Iterator item;
        StringBuffer fileBuffer = new StringBuffer(8192);

        //
        // Write Autogen.h header notation
        //
        fileBuffer.append(CommonDefinition.AUTOGENHNOTATION);

        //
        // Add #ifndef ${BaseName}_AUTOGENH
        // #def ${BseeName}_AUTOGENH
        //
        fileBuffer.append(CommonDefinition.IFNDEF 
                          + CommonDefinition.AUTOGENH
                          + this.moduleId.getGuid().replaceAll("-", "_") 
                          + ToolDefinitions.LINE_SEPARATOR);
        fileBuffer.append(CommonDefinition.DEFINE 
                          + CommonDefinition.AUTOGENH
                          + this.moduleId.getGuid().replaceAll("-", "_") 
                          + ToolDefinitions.LINE_SEPARATOR 
                          + ToolDefinitions.LINE_SEPARATOR);

        //
        // Write the specification version and release version at the begine
        // of autogen.h file.
        // Note: the specification version and release version should
        // be got from module surface area instead of hard code by it's
        // moduleType.
        //
        moduleType = saq.getModuleType();

        //
        // Add "extern int __make_me_compile_correctly;" at begin of
        // AutoGen.h.
        //
        fileBuffer.append(CommonDefinition.AUTOGENHBEGIN);

        //
        // Put EFI_SPECIFICATION_VERSION, and EDK_RELEASE_VERSION.
        //
        String[] specList = saq.getExternSpecificaiton();
        for (int i = 0; i < specList.length; i++) {
            fileBuffer.append(CommonDefinition.DEFINE + specList[i]
                              + "\r\n");
        }
        //
        // Write consumed package's mdouleInfo related .h file to autogen.h
        //
        // PackageIdentification[] consumedPkgIdList = SurfaceAreaQuery
        // .getDependencePkg(this.arch);
        PackageIdentification[] consumedPkgIdList = saq.getDependencePkg(this.arch);
        if (consumedPkgIdList != null) {
            headerFileList = depPkgToAutogenH(consumedPkgIdList, moduleType);
            item = headerFileList.iterator();
            while (item.hasNext()) {
                fileBuffer.append(item.next().toString());
            }
        }

        //
        // Write library class's related *.h file to autogen.h.
        //
        List<String> libClasses = new ArrayList<String>(100);
        String[] libClassList = saq.getLibraryClasses(CommonDefinition.ALWAYSCONSUMED, this.arch, null);
        for (int i = 0; i < libClassList.length; ++i) {
            libClasses.add(libClassList[i]);
        }

        libClassList = saq.getLibraryClasses(CommonDefinition.ALWAYSPRODUCED, this.arch, null);
        for (int i = 0; i < libClassList.length; ++i) {
            libClasses.add(libClassList[i]);
        }
        //
        // Add AutoGen used library class
        // 
        int moduleTypeId = CommonDefinition.getModuleType(moduleType);
        if (!libClasses.contains("DebugLib") && moduleTypeId != CommonDefinition.ModuleTypeUnknown
            && moduleTypeId != CommonDefinition.ModuleTypeBase) {
            libClasses.add("DebugLib");
        }
        switch (moduleTypeId) {
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeDxeSmmDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            if (!libClasses.contains("UefiBootServicesTableLib")) {
                libClasses.add("UefiBootServicesTableLib");
            }
            break;
        }
        LibraryClassToAutogenH(fileBuffer, libClasses.toArray(new String[libClasses.size()]));
        fileBuffer.append("\r\n");

        //
        //  If is TianoR8FlashMap, copy {Fv_DIR}/FlashMap.h to
        // {DEST_DIR_DRBUG}/FlashMap.h
        //
        if (saq.isHaveTianoR8FlashMap()) {
            fileBuffer.append(CommonDefinition.INCLUDE);
            fileBuffer.append("  <");
            fileBuffer.append(CommonDefinition.TIANOR8PLASHMAPH + ">\r\n");
            copyFlashMapHToDebugDir();
        }

        // Write PCD autogen information to AutoGen.h.
        //
        if (this.myPcdAutogen != null) {
            fileBuffer.append("\r\n");
            fileBuffer.append(this.myPcdAutogen.getHAutoGenString());
        }

        fileBuffer.append(globalDeclarations);
        fileBuffer.append(functionDeclarations);
        //
        // Append the #endif at AutoGen.h
        //
        fileBuffer.append("#endif\r\n");

        //
        // Save string buffer content in AutoGen.h.
        //
        if (!saveFile(outputPath + File.separatorChar + "AutoGen.h", fileBuffer)) {
            throw new AutoGenException("Failed to generate AutoGen.h !!!");
        }
    }

    /**
      moduleGenAutogenC
     
      This function generates AutoGen.c for module.
     
      @throws BuildException
                  Failed to generate AutoGen.c.
    **/
    void moduleGenAutogenC() throws EdkException {

        StringBuffer fileBuffer = new StringBuffer(8192);
        //
        // Write Autogen.c header notation
        //
        fileBuffer.append(CommonDefinition.AUTOGENCNOTATION);

        //
        // Get the native MSA file infomation. Since before call autogen,
        // the MSA native <Externs> information were overrided. So before
        // process <Externs> it should be set the DOC as the Native MSA info.
        //
        Map<String, XmlObject> doc = GlobalData.getNativeMsa(this.moduleId);
        saq.push(doc);
        //
        // Write <Extern>
        // DriverBinding/ComponentName/DriverConfiguration/DriverDialog
        // to AutoGen.c
        //
        if (!moduleId.getModuleType().equalsIgnoreCase("UEFI_APPLICATION")) {
            ExternsDriverBindingToAutoGenC(fileBuffer);
        }

        //
        // Write DriverExitBootServicesEvent/DriverSetVirtualAddressMapEvent
        // to Autogen.c
        //
        ExternCallBackToAutoGenC(fileBuffer);

        //
        // Write EntryPoint to autgoGen.c
        //
        String[] entryPointList = saq.getModuleEntryPointArray();
        String[] unloadImageList = saq.getModuleUnloadImageArray();
        EntryPointToAutoGen(CommonDefinition.remDupString(entryPointList), 
                            CommonDefinition.remDupString(unloadImageList),
                            fileBuffer);

        pcdDriverType = saq.getPcdDriverType();

        //
        // Restore the DOC which include the FPD module info.
        //
        saq.pop();

        //
        // Write Guid to autogen.c
        //
        String guid = CommonDefinition.formatGuidName(saq.getModuleGuid());
        if (this.moduleId.getModuleType().equalsIgnoreCase(EdkDefinitions.MODULE_TYPE_BASE)) {
            globalDeclarations.append("extern GUID gEfiCallerIdGuid;\r\n");
            fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED GUID gEfiCallerIdGuid = {");
        } else if (!this.moduleId.getModuleType().equalsIgnoreCase(EdkDefinitions.MODULE_TYPE_USER_DEFINED)) {
            globalDeclarations.append("extern EFI_GUID gEfiCallerIdGuid;\r\n");
            fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiCallerIdGuid = {");
        }

        if (guid == null) {
            throw new AutoGenException("Guid value must set!\n");
        }

        //
        // Formate Guid as ANSI c form.Example:
        // {0xd2b2b828, 0x826, 0x48a7,{0xb3, 0xdf, 0x98, 0x3c, 0x0, 0x60, 0x24,
        // 0xf0}}
        //

        fileBuffer.append(guid);
        fileBuffer.append("};\r\n");

        //
        // Generate library instance consumed protocol, guid, ppi, pcd list.
        // Save those to this.protocolList, this.ppiList, this.pcdList,
        // this.guidList. Write Consumed library constructor and desconstuct to
        // autogen.c
        //
        LibInstanceToAutogenC(fileBuffer);

        //
        // Get module dependent Package identification.
        //
        PackageIdentification[] packages = saq.getDependencePkg(this.arch);
        for (int i = 0; i < packages.length; i++) {
            if (!this.mDepPkgList.contains(packages[i])) {
                this.mDepPkgList.add(packages[i]);
            }

        }

        //
        // Write consumed ppi, guid, protocol, etc to autogen.c
        //
        CNameToAutogenC(fileBuffer);

        //
        // Call pcd autogen.
        //
        this.myPcdAutogen = new PCDAutoGenAction(moduleId, 
                                                 arch, 
                                                 false, 
                                                 null,
                                                 pcdDriverType, 
                                                 parentId);

        this.myPcdAutogen.execute();
        if (this.myPcdAutogen != null) {
            fileBuffer.append("\r\n");
            fileBuffer.append(this.myPcdAutogen.getCAutoGenString());
        }

        if (!saveFile(outputPath + File.separatorChar + "AutoGen.c", fileBuffer)) {
            throw new AutoGenException("Failed to generate AutoGen.c !!!");
        }

    }

    /**
      libGenAutogenH
     
      This function generates AutoGen.h for library.
     
      @throws BuildException
                  Failed to generate AutoGen.c.
    **/
    void libGenAutogenH() throws EdkException {

        Set<String> libClassIncludeH;
        String moduleType;
        List<String> headerFileList;
        Iterator item;
        StringBuffer fileBuffer = new StringBuffer(10240);

        //
        // Write Autogen.h header notation
        //
        fileBuffer.append(CommonDefinition.AUTOGENHNOTATION);

        //
        // Add #ifndef ${BaseName}_AUTOGENH
        // #def ${BseeName}_AUTOGENH
        //
        fileBuffer.append(CommonDefinition.IFNDEF 
                          + CommonDefinition.AUTOGENH
                          + this.moduleId.getGuid().replaceAll("-", "_") 
                          + ToolDefinitions.LINE_SEPARATOR);
        fileBuffer.append(CommonDefinition.DEFINE 
                          + CommonDefinition.AUTOGENH
                          + this.moduleId.getGuid().replaceAll("-", "_") 
                          + ToolDefinitions.LINE_SEPARATOR 
                          + ToolDefinitions.LINE_SEPARATOR);

        //
        // Write EFI_SPECIFICATION_VERSION and EDK_RELEASE_VERSION
        // to autogen.h file.
        // Note: the specification version and release version should
        // be get from module surface area instead of hard code.
        //
        fileBuffer.append(CommonDefinition.AUTOGENHBEGIN);
        String[] specList = saq.getExternSpecificaiton();
        for (int i = 0; i < specList.length; i++) {
            fileBuffer.append(CommonDefinition.DEFINE + specList[i] + "\r\n");
        }
        // fileBuffer.append(CommonDefinition.autoGenHLine1);
        // fileBuffer.append(CommonDefinition.autoGenHLine2);

        //
        // Write consumed package's mdouleInfo related *.h file to autogen.h.
        //
        moduleType = saq.getModuleType();
        PackageIdentification[] cosumedPkglist = saq.getDependencePkg(this.arch);
        headerFileList = depPkgToAutogenH(cosumedPkglist, moduleType);
        item = headerFileList.iterator();
        while (item.hasNext()) {
            fileBuffer.append(item.next().toString());
        }
        //
        // Write library class's related *.h file to autogen.h
        //
        List<String> libClasses = new ArrayList<String>(100);
        String[] libClassList = saq.getLibraryClasses(CommonDefinition.ALWAYSCONSUMED, this.arch, null);
        for (int i = 0; i < libClassList.length; ++i) {
            libClasses.add(libClassList[i]);
        }

        libClassList = saq.getLibraryClasses(CommonDefinition.ALWAYSPRODUCED, this.arch, null);
        for (int i = 0; i < libClassList.length; ++i) {
            libClasses.add(libClassList[i]);
        }
        //
        // Add AutoGen used library class
        // 
        int moduleTypeId = CommonDefinition.getModuleType(moduleType);
        if (!libClasses.contains("DebugLib") && moduleTypeId != CommonDefinition.ModuleTypeUnknown
            && moduleTypeId != CommonDefinition.ModuleTypeBase) {
            libClasses.add("DebugLib");
        }
        switch (moduleTypeId) {
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeDxeSmmDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            if (!libClasses.contains("UefiBootServicesTableLib")) {
                libClasses.add("UefiBootServicesTableLib");
            }
            break;
        }
        LibraryClassToAutogenH(fileBuffer, libClasses.toArray(new String[libClasses.size()]));
        fileBuffer.append(ToolDefinitions.LINE_SEPARATOR);

        //
        //  If is TianoR8FlashMap, copy {Fv_DIR}/FlashMap.h to
        // {DEST_DIR_DRBUG}/FlashMap.h
        //
        if (saq.isHaveTianoR8FlashMap()) {
            fileBuffer.append(CommonDefinition.INCLUDE);
            fileBuffer.append("  <");
            fileBuffer.append(CommonDefinition.TIANOR8PLASHMAPH + ">\r\n");
            copyFlashMapHToDebugDir();
        }

        //
        // Write PCD information to library AutoGen.h.
        //
        if (this.myPcdAutogen != null) {
            fileBuffer.append("\r\n");
            fileBuffer.append(this.myPcdAutogen.getHAutoGenString());
        }
        //
        // generate function prototype for constructor and destructor
        // 
        LibConstructorToAutogenH(moduleType);
        LibDestructorToAutogenH(moduleType);
        ExternCallBackToAutoGenH(moduleType);
        fileBuffer.append(functionDeclarations);
        //
        // Append the #endif at AutoGen.h
        //
        fileBuffer.append("#endif\r\n");
        //
        // Save content of string buffer to AutoGen.h file.
        //
        if (!saveFile(outputPath + File.separatorChar + "AutoGen.h", fileBuffer)) {
            throw new AutoGenException("Failed to generate AutoGen.h !!!");
        }
    }

    /**
      libGenAutogenC
     
      This function generates AutoGen.h for library.
     
      @throws BuildException
                  Failed to generate AutoGen.c.
    **/
    void libGenAutogenC() throws EdkException {
        StringBuffer fileBuffer = new StringBuffer(10240);

        //
        // Write Autogen.c header notation
        //
        fileBuffer.append(CommonDefinition.AUTOGENCNOTATION);

        fileBuffer.append(ToolDefinitions.LINE_SEPARATOR);
        fileBuffer.append(ToolDefinitions.LINE_SEPARATOR);

        //
        // Call pcd autogen.
        //
        this.myPcdAutogen = new PCDAutoGenAction(moduleId,
                                                 arch,
                                                 true,
                                                 saq.getModulePcdEntryNameArray(this.arch),
                                                 pcdDriverType, 
                                                 parentId);
        this.myPcdAutogen.execute();
        if (this.myPcdAutogen != null) {
            fileBuffer.append(ToolDefinitions.LINE_SEPARATOR);
            fileBuffer.append(this.myPcdAutogen.getCAutoGenString());
        }
    }

    /**
      LibraryClassToAutogenH
     
      This function returns *.h files declared by library classes which are
      consumed or produced by current build module or library.
     
      @param libClassList
                 List of library class which consumed or produce by current
                 build module or library.
      @return includeStrList List of *.h file.
    **/
    void LibraryClassToAutogenH(StringBuffer fileBuffer, String[] libClassList)
    throws EdkException {
        String includeName[];
        String str = "";

        //
        // Get include file from GlobalData's SPDTable according to
        // library class name.
        //
        for (int i = 0; i < libClassList.length; i++) {
            includeName = GlobalData.getLibraryClassHeaderFiles(
                                                               saq.getDependencePkg(this.arch),
                                                               libClassList[i]);
            if (includeName == null) {
                throw new AutoGenException("Can not find library class ["
                                           + libClassList[i] + "] declaration in any SPD package. ");
            }
            for (int j = 0; j < includeName.length; j++) {
                String includeNameStr = includeName[j];
                if (includeNameStr != null) {
                    fileBuffer.append(CommonDefinition.INCLUDE);
                    fileBuffer.append(" <");
                    fileBuffer.append(includeNameStr);
                    fileBuffer.append(">\r\n");
                    includeNameStr = null;
                }
            }
        }
    }

    /**
      IncludesToAutogenH
     
      This function add include file in AutoGen.h file.
     
      @param packageNameList
                 List of module depended package.
      @param moduleType
                 Module type.
      @return
    **/
    List<String> depPkgToAutogenH(PackageIdentification[] packageNameList,
                                  String moduleType) throws AutoGenException {

        List<String> includeStrList = new LinkedList<String>();
        String pkgHeader;
        String includeStr = "";

        //
        // Get include file from moduleInfo file
        //
        for (int i = 0; i < packageNameList.length; i++) {
            pkgHeader = GlobalData.getPackageHeaderFiles(packageNameList[i],
                                                         moduleType);
            if (pkgHeader == null) {
                throw new AutoGenException("Can not find package ["
                                           + packageNameList[i]
                                           + "] declaration in any SPD package. ");
            } else if (!pkgHeader.equalsIgnoreCase("")) {
                includeStr = CommonDefinition.INCLUDE + " <" + pkgHeader + ">\r\n";
                includeStrList.add(includeStr);
            }
        }

        return includeStrList;
    }

    /**
      EntryPointToAutoGen
     
      This function convert <ModuleEntryPoint> & <ModuleUnloadImage>
      information in mas to AutoGen.c
     
      @param entryPointList
                 List of entry point.
      @param fileBuffer
                 String buffer fo AutoGen.c.
      @throws Exception
    **/
    void EntryPointToAutoGen(String[] entryPointList, String[] unloadImageList, StringBuffer fileBuffer)
    throws EdkException {

        String typeStr = saq.getModuleType();
        String debugStr = "DEBUG ((EFI_D_INFO | EFI_D_LOAD, \"Module Entry Point (%s) 0x%%p\\n\", (VOID *)(UINTN)%s));\r\n";
        int unloadImageCount = 0;
        int entryPointCount  = 0;

        //
        // The parameters and return value of entryPoint is difference
        // for difference module type.
        //
        switch (CommonDefinition.getModuleType(typeStr)) {
        
        case CommonDefinition.ModuleTypePeiCore:
            if (entryPointList == null ||entryPointList.length != 1 ) {
                throw new AutoGenException("Module type = 'PEI_CORE', can have only one module entry point!");
            } else {
                functionDeclarations.append("EFI_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(entryPointList[0]);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,\r\n");
                functionDeclarations.append("  IN VOID                        *OldCoreData\r\n");
                functionDeclarations.append("  );\r\n\r\n");

                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,\r\n");
                fileBuffer.append("  IN VOID                        *OldCoreData\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (PeiStartupDescriptor, OldCoreData);\r\n");
                fileBuffer.append("}\r\n\r\n");
            }
            break;

        case CommonDefinition.ModuleTypeDxeCore:
            fileBuffer.append("const UINT32 _gUefiDriverRevision = 0;\r\n");
            if (entryPointList == null || entryPointList.length != 1) {
                throw new AutoGenException("Module type = 'DXE_CORE', can have only one module entry point!");
            } else {
                functionDeclarations.append("VOID\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(entryPointList[0]);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN VOID  *HobStart\r\n");
                functionDeclarations.append("  );\r\n\r\n");

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN VOID  *HobStart\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (HobStart);\r\n");
                fileBuffer.append("}\r\n\r\n");
            }
            break;

        case CommonDefinition.ModuleTypePeim:
            entryPointCount = 0;
            fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = 0;\r\n");
            if (entryPointList == null || entryPointList.length == 0) {
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_FFS_FILE_HEADER  *FfsHeader,\r\n");
                fileBuffer.append("  IN EFI_PEI_SERVICES     **PeiServices\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
                fileBuffer.append("}\r\n\r\n");
                break;
            }
            for (int i = 0; i < entryPointList.length; i++) {
                functionDeclarations.append("EFI_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(entryPointList[i]);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_FFS_FILE_HEADER  *FfsHeader,\r\n");
                functionDeclarations.append("  IN EFI_PEI_SERVICES     **PeiServices\r\n");
                functionDeclarations.append("  );\r\n");
                entryPointCount++;
            }

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleEntryPointList (\r\n");
            fileBuffer.append("  IN EFI_FFS_FILE_HEADER  *FfsHeader,\r\n");
            fileBuffer.append("  IN EFI_PEI_SERVICES     **PeiServices\r\n");
            fileBuffer.append("  )\r\n\r\n");
            fileBuffer.append("{\r\n");
            if (entryPointCount == 1) {
                fileBuffer.append(String.format("  " + debugStr, entryPointList[0], entryPointList[0]));
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (FfsHeader, PeiServices);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n");
                fileBuffer.append("  EFI_STATUS  CombinedStatus;\r\n\r\n");
                fileBuffer.append("  CombinedStatus = EFI_LOAD_ERROR;\r\n\r\n");
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append(String.format("  " + debugStr, entryPointList[i], entryPointList[i]));
                        fileBuffer.append("  Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append(" (FfsHeader, PeiServices);\r\n");
                        fileBuffer.append("  if (!EFI_ERROR (Status) || EFI_ERROR (CombinedStatus)) {\r\n");
                        fileBuffer.append("    CombinedStatus = Status;\r\n");
                        fileBuffer.append("  }\r\n\r\n");
                    } else {
                        break;
                    }
                }
                fileBuffer.append("  return CombinedStatus;\r\n");
            }
            fileBuffer.append("}\r\n\r\n");
            break;

        case CommonDefinition.ModuleTypeDxeSmmDriver:
            entryPointCount = 0;
            //
            // If entryPoint is null, create an empty ProcessModuleEntryPointList
            // function.
            //
            if (entryPointList == null || entryPointList.length == 0) {
                fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED  const UINT8  _gDriverEntryPointCount = ");
                fileBuffer.append(Integer.toString(entryPointCount));
                fileBuffer.append(";\r\n");
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
                fileBuffer.append("}\r\n\r\n");

            } else {
                for (int i = 0; i < entryPointList.length; i++) {
                    functionDeclarations.append("EFI_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(entryPointList[i]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                    functionDeclarations.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                    functionDeclarations.append("  );\r\n");
                    entryPointCount++;
                }
                fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED  const UINT8  _gDriverEntryPointCount = ");
                fileBuffer.append(Integer.toString(entryPointCount));
                fileBuffer.append(";\r\n");
                fileBuffer.append("static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;\r\n");
                fileBuffer.append("static EFI_STATUS  mDriverEntryPointStatus = EFI_LOAD_ERROR;\r\n\r\n");

                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");

                for (int i = 0; i < entryPointList.length; i++) {
                    fileBuffer.append("  if (SetJump (&mJumpContext) == 0) {\r\n");
                    fileBuffer.append(String.format("    " + debugStr, entryPointList[i], entryPointList[i]));
                    fileBuffer.append("    ExitDriver (");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (ImageHandle, SystemTable));\r\n");
                    fileBuffer.append("    ASSERT (FALSE);\r\n");
                    fileBuffer.append("  }\r\n");
                }
                fileBuffer.append("  return mDriverEntryPointStatus;\r\n");
                fileBuffer.append("}\r\n\r\n");

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ExitDriver (\r\n");
                fileBuffer.append("  IN EFI_STATUS  Status\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {\r\n");
                fileBuffer.append("    mDriverEntryPointStatus = Status;\r\n");
                fileBuffer.append("  }\r\n");
                fileBuffer.append("  LongJump (&mJumpContext, (UINTN)-1);\r\n");
                fileBuffer.append("  ASSERT (FALSE);\r\n");
                fileBuffer.append("}\r\n\r\n");
            }


            //
            // Add "ModuleUnloadImage" for DxeSmmDriver module type;
            //

            unloadImageCount = 0;
            if (unloadImageList != null) {
                for (int i = 0; i < unloadImageList.length; i++) {
                    functionDeclarations.append("EFI_STATUS\r\n");
                    functionDeclarations.append(unloadImageList[i]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  IN EFI_HANDLE        ImageHandle\r\n");
                    functionDeclarations.append("  );\r\n");
                    unloadImageCount++;
                }
            }

            fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverUnloadImageCount = ");
            fileBuffer.append(Integer.toString(unloadImageCount));
            fileBuffer.append(";\r\n\r\n");

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleUnloadList (\r\n");
            fileBuffer.append("  IN EFI_HANDLE  ImageHandle\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");

            if (unloadImageCount == 0) {
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
            } else if (unloadImageCount == 1) {
                fileBuffer.append("  return ");
                fileBuffer.append(unloadImageList[0]);
                fileBuffer.append("(ImageHandle);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n\r\n");
                fileBuffer.append("  Status = EFI_SUCCESS;\r\n\r\n");
                for (int i = 0; i < unloadImageList.length; i++) {
                    if (i == 0) {
                        fileBuffer.append("     Status = ");
                        fileBuffer.append(unloadImageList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                    } else {
                        fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                        fileBuffer.append("    ");
                        fileBuffer.append(unloadImageList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  } else {\r\n");
                        fileBuffer.append("    Status = ");
                        fileBuffer.append(unloadImageList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  }\r\n");
                    }
                }
                fileBuffer.append("  return Status;\r\n");
            }
            fileBuffer.append("}\r\n\r\n");
            break;

        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            entryPointCount = 0;
            fileBuffer.append("const UINT32 _gUefiDriverRevision = 0;\r\n");
            //
            // If entry point is null, create a empty ProcessModuleEntryPointList function.
            //
            if (entryPointList == null || entryPointList.length == 0) {
                fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverEntryPointCount = 0;\r\n");
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
                fileBuffer.append("}\r\n");

            } else {
                for (int i = 0; i < entryPointList.length; i++) {
                    functionDeclarations.append("EFI_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(entryPointList[i]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                    functionDeclarations.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                    functionDeclarations.append("  );\r\n");
                    entryPointCount++;
                }

                fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverEntryPointCount = ");
                fileBuffer.append(Integer.toString(entryPointCount));
                fileBuffer.append(";\r\n");
                if (entryPointCount > 1) {
                    fileBuffer.append("static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;\r\n");
                    fileBuffer.append("static EFI_STATUS  mDriverEntryPointStatus = EFI_LOAD_ERROR;\r\n");
                }
                fileBuffer.append("\r\n");

                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");

                if (entryPointCount == 1) {
                    fileBuffer.append(String.format("  " + debugStr, entryPointList[0], entryPointList[0]));
                    fileBuffer.append("  return ");
                    fileBuffer.append(entryPointList[0]);
                    fileBuffer.append(" (ImageHandle, SystemTable);\r\n");
                } else {
                    for (int i = 0; i < entryPointList.length; i++) {
                        if (!entryPointList[i].equals("")) {
                            fileBuffer.append("  if (SetJump (&mJumpContext) == 0) {\r\n");
                            fileBuffer.append(String.format("    " + debugStr, entryPointList[i], entryPointList[i]));
                            if (CommonDefinition.getModuleType(typeStr) == CommonDefinition.ModuleTypeUefiApplication) {
                                fileBuffer.append("    Exit (");
                            } else {
                                fileBuffer.append("    ExitDriver (");
                            }
                            fileBuffer.append(entryPointList[i]);
                            fileBuffer.append(" (ImageHandle, SystemTable));\r\n");
                            fileBuffer.append("    ASSERT (FALSE);\r\n");
                            fileBuffer.append("  }\r\n");
                        } else {
                            break;
                        }
                    }
                    fileBuffer.append("  return mDriverEntryPointStatus;\r\n");
                }
                fileBuffer.append("}\r\n\r\n");

                if (CommonDefinition.getModuleType(typeStr) != CommonDefinition.ModuleTypeUefiApplication) {
                    fileBuffer.append("VOID\r\n");
                    fileBuffer.append("EFIAPI\r\n");
                    fileBuffer.append("ExitDriver (\r\n");
                    fileBuffer.append("  IN EFI_STATUS  Status\r\n");
                    fileBuffer.append("  )\r\n\r\n");
                    fileBuffer.append("{\r\n");
                    if (entryPointCount <= 1) {
                        fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                        fileBuffer.append("    ProcessLibraryDestructorList (gImageHandle, gST);\r\n");
                        fileBuffer.append("  }\r\n");
                        fileBuffer.append("  gBS->Exit (gImageHandle, Status, 0, NULL);\r\n");
                    } else {
                        fileBuffer.append("  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {\r\n");
                        fileBuffer.append("    mDriverEntryPointStatus = Status;\r\n");
                        fileBuffer.append("  }\r\n");
                        fileBuffer.append("  LongJump (&mJumpContext, (UINTN)-1);\r\n");
                        fileBuffer.append("  ASSERT (FALSE);\r\n");
                    }
                    fileBuffer.append("}\r\n\r\n");
                }
            }

            if (CommonDefinition.getModuleType(typeStr) == CommonDefinition.ModuleTypeUefiApplication) {
                break;
            }
            //
            // Add ModuleUnloadImage for DxeDriver and UefiDriver module type.
            //
            //entryPointList = SurfaceAreaQuery.getModuleUnloadImageArray();
            //
            // Remover duplicate unload entry point.
            //
            //entryPointList = CommonDefinition.remDupString(entryPointList);
            //entryPointCount = 0;
            unloadImageCount = 0;
            if (unloadImageList != null) {
                for (int i = 0; i < unloadImageList.length; i++) {
                    functionDeclarations.append("EFI_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(unloadImageList[i]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  IN EFI_HANDLE        ImageHandle\r\n");
                    functionDeclarations.append("  );\r\n");
                    unloadImageCount++;
                }
            }

            fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverUnloadImageCount = ");
            fileBuffer.append(Integer.toString(unloadImageCount));
            fileBuffer.append(";\r\n\r\n");

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleUnloadList (\r\n");
            fileBuffer.append("  IN EFI_HANDLE  ImageHandle\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");

            if (unloadImageCount == 0) {
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
            } else if (unloadImageCount == 1) {
                fileBuffer.append("  return ");
                fileBuffer.append(unloadImageList[0]);
                fileBuffer.append("(ImageHandle);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n\r\n");
                fileBuffer.append("  Status = EFI_SUCCESS;\r\n\r\n");
                for (int i = 0; i < unloadImageList.length; i++) {
                    if (i == 0) {
                        fileBuffer.append("  Status = ");
                        fileBuffer.append(unloadImageList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                    } else {
                        fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                        fileBuffer.append("    ");
                        fileBuffer.append(unloadImageList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  } else {\r\n");
                        fileBuffer.append("    Status = ");
                        fileBuffer.append(unloadImageList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  }\r\n");
                    }
                }
                fileBuffer.append("  return Status;\r\n");
            }
            fileBuffer.append("}\r\n\r\n");
            break;
        }
    }

    /**
      CNameToAutogenc
     
      This function gets GUIDs from SPD file accrodeing to <Protocols> <Ppis> 
      <Guids> information and write those GUIDs to AutoGen.c.
     
      @param fileBuffer
                 String Buffer for Autogen.c file.
     
    **/
    void CNameToAutogenC(StringBuffer fileBuffer) throws AutoGenException {
        String[] cNameGuid = null;
        String guidKeyWord = null;

        String[] cnameList = saq.getCNameArray(this.arch);
        for (int i = 0; i < cnameList.length; i++) {
            this.mGuidCNameList.add(cnameList[i]);
        }


        Iterator guidIterator = this.mGuidCNameList.iterator();
        while (guidIterator.hasNext()) {
            guidKeyWord = guidIterator.next().toString();
            cNameGuid = GlobalData.getGuid(this.mDepPkgList, guidKeyWord);
            if (cNameGuid == null) {
                cNameGuid = GlobalData.getProtocolGuid(this.mDepPkgList, guidKeyWord);
                if (cNameGuid == null) {
                    cNameGuid = GlobalData.getPpiGuid(this.mDepPkgList, guidKeyWord);
                    if (cNameGuid == null) {
                        //
                        // If can't find GUID declaration in every package, stop the build
                        //
                        EdkLog.log(EdkLog.EDK_INFO,"WARN: Can not find Guid [" + guidKeyWord + "] declaration in any SPD file.");
                        continue;
                        //throw new AutoGenException("Can not find Guid [" + guidKeyWord
                        //                           + "] declaration in any SPD package. ");
                    }
                }
            }

            fileBuffer.append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID ");
            fileBuffer.append(cNameGuid[0]);
            fileBuffer.append(" =     { ");
            fileBuffer.append(CommonDefinition.formatGuidName(cNameGuid[1]));
            fileBuffer.append("} ;");
        }
    }

    /**
      LibInstanceToAutogenC
     
      This function adds dependent library instance to autogen.c,which
      includeing library's constructor, destructor, and library dependent ppi,
      protocol, guid, pcd information.
     
      @param fileBuffer
                 String buffer for AutoGen.c
      @throws BuildException
    **/
    void LibInstanceToAutogenC(StringBuffer fileBuffer) throws EdkException {
        String moduleType = this.moduleId.getModuleType();
        //
        // Add library constructor to AutoGen.c
        //
        LibConstructorToAutogenC(libConstructList, moduleType, fileBuffer);
        //
        // Add library destructor to AutoGen.c
        //
        LibDestructorToAutogenC(libDestructList, moduleType, fileBuffer);
    }

    /**
      LibConstructorToAutogenH
     
      This function writes library constructor declarations AutoGen.h. The library
      constructor's parameter and return value depend on module type.
     
      @param libInstanceList
                 List of library construct name.
      @param moduleType
                 Module type.
      @param fileBuffer
                 String buffer for AutoGen.c
      @throws Exception
    **/
    void LibConstructorToAutogenH(String moduleType) throws EdkException {
        boolean isFirst = true;

        //
        // If not yet parse this library instance's constructor
        // element,parse it.
        //
        String libConstructName = saq.getLibConstructorName();
        if (libConstructName == null) {
            return;
        }

        //
        // The library constructor's parameter and return value depend on
        // module type.
        //
        if (moduleType.equalsIgnoreCase(EdkDefinitions.MODULE_TYPE_BASE)) {
            functionDeclarations.append("RETURN_STATUS\r\n");
            functionDeclarations.append("EFIAPI\r\n");
            functionDeclarations.append(libConstructName);
            functionDeclarations.append(" (\r\n");
            functionDeclarations.append("  VOID\r\n");
            functionDeclarations.append("  );\r\n");
        } else {
            switch (CommonDefinition.getModuleType(moduleType)) {
            case CommonDefinition.ModuleTypeBase:
                functionDeclarations.append("RETURN_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(libConstructName);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  VOID\r\n");
                functionDeclarations.append("  );\r\n");
                break;

            case CommonDefinition.ModuleTypePeiCore:
            case CommonDefinition.ModuleTypePeim:
                functionDeclarations.append("EFI_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(libConstructName);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
                functionDeclarations.append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
                functionDeclarations.append("  );\r\n");
                break;

            case CommonDefinition.ModuleTypeDxeCore:
            case CommonDefinition.ModuleTypeDxeDriver:
            case CommonDefinition.ModuleTypeDxeRuntimeDriver:
            case CommonDefinition.ModuleTypeDxeSmmDriver:
            case CommonDefinition.ModuleTypeDxeSalDriver:
            case CommonDefinition.ModuleTypeUefiDriver:
            case CommonDefinition.ModuleTypeUefiApplication:
                functionDeclarations.append("EFI_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(libConstructName);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                functionDeclarations.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                functionDeclarations.append("  );\r\n");
                break;

            }
        }
    }

    /**
      LibDestructorToAutogenH
     
      This function writes library destructor declarations AutoGen.h. The library
      destructor's parameter and return value depend on module type.
     
      @param libInstanceList
                 List of library destructor name.
      @param moduleType
                 Module type.
      @param fileBuffer
                 String buffer for AutoGen.c
      @throws Exception
    **/
    void LibDestructorToAutogenH(String moduleType) throws EdkException {
        boolean isFirst = true;
        String libDestructName = saq.getLibDestructorName();
        if (libDestructName == null) {
            return;
        }

        if (moduleType.equalsIgnoreCase(EdkDefinitions.MODULE_TYPE_BASE)) {
            functionDeclarations.append("RETURN_STATUS\r\n");
            functionDeclarations.append("EFIAPI\r\n");
            functionDeclarations.append(libDestructName);
            functionDeclarations.append(" (\r\n");
            functionDeclarations.append("  VOID\r\n");
            functionDeclarations.append("  );\r\n");
        } else {
            switch (CommonDefinition.getModuleType(moduleType)) {
            case CommonDefinition.ModuleTypeBase:
                functionDeclarations.append("RETURN_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(libDestructName);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  VOID\r\n");
                functionDeclarations.append("  );\r\n");
                break;
            case CommonDefinition.ModuleTypePeiCore:
            case CommonDefinition.ModuleTypePeim:
                functionDeclarations.append("EFI_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(libDestructName);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
                functionDeclarations.append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
                functionDeclarations.append("  );\r\n");
                break;
            case CommonDefinition.ModuleTypeDxeCore:
            case CommonDefinition.ModuleTypeDxeDriver:
            case CommonDefinition.ModuleTypeDxeRuntimeDriver:
            case CommonDefinition.ModuleTypeDxeSmmDriver:
            case CommonDefinition.ModuleTypeDxeSalDriver:
            case CommonDefinition.ModuleTypeUefiDriver:
            case CommonDefinition.ModuleTypeUefiApplication:
                functionDeclarations.append("EFI_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(libDestructName);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                functionDeclarations.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                functionDeclarations.append("  );\r\n");
                break;
            }
        }
    }

    /**
      LibConstructorToAutogenc
     
      This function writes library constructor list to AutoGen.c. The library
      constructor's parameter and return value depend on module type.
     
      @param libInstanceList
                 List of library construct name.
      @param moduleType
                 Module type.
      @param fileBuffer
                 String buffer for AutoGen.c
      @throws Exception
    **/
    void LibConstructorToAutogenC(List<String[]> libInstanceList,
                                  String moduleType, StringBuffer fileBuffer) throws EdkException {
        boolean isFirst = true;

        //
        // The library constructor's parameter and return value depend on
        // module type.
        //
        for (int i = 0; i < libInstanceList.size(); i++) {
            if (libInstanceList.get(i)[1].equalsIgnoreCase(EdkDefinitions.MODULE_TYPE_BASE)) {
                functionDeclarations.append("RETURN_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(libInstanceList.get(i)[0]);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  VOID\r\n");
                functionDeclarations.append("  );\r\n");
            } else {
                switch (CommonDefinition.getModuleType(moduleType)) {
                case CommonDefinition.ModuleTypeBase:
                    functionDeclarations.append("RETURN_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(libInstanceList.get(i)[0]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  VOID\r\n");
                    functionDeclarations.append("  );\r\n");
                    break;

                case CommonDefinition.ModuleTypePeiCore:
                case CommonDefinition.ModuleTypePeim:
                    functionDeclarations.append("EFI_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(libInstanceList.get(i)[0]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
                    functionDeclarations.append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
                    functionDeclarations.append("  );\r\n");
                    break;

                case CommonDefinition.ModuleTypeDxeCore:
                case CommonDefinition.ModuleTypeDxeDriver:
                case CommonDefinition.ModuleTypeDxeRuntimeDriver:
                case CommonDefinition.ModuleTypeDxeSmmDriver:
                case CommonDefinition.ModuleTypeDxeSalDriver:
                case CommonDefinition.ModuleTypeUefiDriver:
                case CommonDefinition.ModuleTypeUefiApplication:
                    functionDeclarations.append("EFI_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(libInstanceList.get(i)[0]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                    functionDeclarations.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                    functionDeclarations.append("  );\r\n");
                    break;

                }
            }
        }

        //
        // Add ProcessLibraryConstructorList in AutoGen.c
        //
        fileBuffer.append("VOID\r\n");
        fileBuffer.append("EFIAPI\r\n");
        fileBuffer.append("ProcessLibraryConstructorList (\r\n");
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeBase:
            fileBuffer.append("  VOID\r\n");
            break;

        case CommonDefinition.ModuleTypePeiCore:
        case CommonDefinition.ModuleTypePeim:
            fileBuffer.append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
            fileBuffer.append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
            break;

        case CommonDefinition.ModuleTypeDxeCore:
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSmmDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
            fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
            break;
        }

        fileBuffer.append("  )\r\n");
        fileBuffer.append("{\r\n");
        //
        // If no constructor function, return EFI_SUCCESS.
        //
        for (int i = 0; i < libInstanceList.size(); i++) {
            if (isFirst) {
                fileBuffer.append("  EFI_STATUS  Status;\r\n");
                fileBuffer.append("  Status = EFI_SUCCESS;\r\n");
                fileBuffer.append("\r\n");
                isFirst = false;
            }
            if (libInstanceList.get(i)[1].equalsIgnoreCase(EdkDefinitions.MODULE_TYPE_BASE)) {
                fileBuffer.append("  Status = ");
                fileBuffer.append(libInstanceList.get(i)[0]);
                fileBuffer.append("();\r\n");
            } else {
                switch (CommonDefinition.getModuleType(moduleType)) {
                case CommonDefinition.ModuleTypeBase:
                    fileBuffer.append("  Status = ");
                    fileBuffer.append(libInstanceList.get(i)[0]);
                    fileBuffer.append("();\r\n");
                    break;
                case CommonDefinition.ModuleTypePeiCore:
                case CommonDefinition.ModuleTypePeim:
                    fileBuffer.append("  Status = ");
                    fileBuffer.append(libInstanceList.get(i)[0]);
                    fileBuffer.append(" (FfsHeader, PeiServices);\r\n");
                    break;
                case CommonDefinition.ModuleTypeDxeCore:
                case CommonDefinition.ModuleTypeDxeDriver:
                case CommonDefinition.ModuleTypeDxeRuntimeDriver:
                case CommonDefinition.ModuleTypeDxeSmmDriver:
                case CommonDefinition.ModuleTypeDxeSalDriver:
                case CommonDefinition.ModuleTypeUefiDriver:
                case CommonDefinition.ModuleTypeUefiApplication:
                    fileBuffer.append("  Status = ");
                    fileBuffer.append(libInstanceList.get(i)[0]);
                    fileBuffer.append(" (ImageHandle, SystemTable);\r\n");
                    break;
                default:
                    EdkLog.log(EdkLog.EDK_INFO,"Autogen doesn't know how to deal with module type - " + moduleType + "!");
                }

            }
            fileBuffer.append("  ASSERT_EFI_ERROR (Status);\r\n");
        }
        fileBuffer.append("}\r\n");
    }

    /**
      LibDestructorToAutogenc
     
      This function writes library destructor list to AutoGen.c. The library
      destructor's parameter and return value depend on module type.
     
      @param libInstanceList
                 List of library destructor name.
      @param moduleType
                 Module type.
      @param fileBuffer
                 String buffer for AutoGen.c
      @throws Exception
    **/
    void LibDestructorToAutogenC(List<String[]> libInstanceList,
                                 String moduleType, StringBuffer fileBuffer) throws EdkException {
        boolean isFirst = true;
        for (int i = 0; i < libInstanceList.size(); i++) {
            if (libInstanceList.get(i)[1].equalsIgnoreCase(EdkDefinitions.MODULE_TYPE_BASE)) {
                functionDeclarations.append("RETURN_STATUS\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(libInstanceList.get(i)[0]);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  VOID\r\n");
                functionDeclarations.append("  );\r\n");
            } else {
                switch (CommonDefinition.getModuleType(moduleType)) {
                case CommonDefinition.ModuleTypeBase:
                    functionDeclarations.append("RETURN_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(libInstanceList.get(i)[0]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  VOID\r\n");
                    functionDeclarations.append("  );\r\n");
                    break;
                case CommonDefinition.ModuleTypePeiCore:
                case CommonDefinition.ModuleTypePeim:
                    functionDeclarations.append("EFI_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(libInstanceList.get(i)[0]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
                    functionDeclarations.append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
                    functionDeclarations.append("  );\r\n");
                    break;
                case CommonDefinition.ModuleTypeDxeCore:
                case CommonDefinition.ModuleTypeDxeDriver:
                case CommonDefinition.ModuleTypeDxeRuntimeDriver:
                case CommonDefinition.ModuleTypeDxeSmmDriver:
                case CommonDefinition.ModuleTypeDxeSalDriver:
                case CommonDefinition.ModuleTypeUefiDriver:
                case CommonDefinition.ModuleTypeUefiApplication:
                    functionDeclarations.append("EFI_STATUS\r\n");
                    functionDeclarations.append("EFIAPI\r\n");
                    functionDeclarations.append(libInstanceList.get(i)[0]);
                    functionDeclarations.append(" (\r\n");
                    functionDeclarations.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                    functionDeclarations.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                    functionDeclarations.append("  );\r\n");
                    break;
                }
            }
        }

        //
        // Write ProcessLibraryDestructor list to autogen.c
        //
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeBase:
        case CommonDefinition.ModuleTypePeiCore:
        case CommonDefinition.ModuleTypePeim:
            break;
        case CommonDefinition.ModuleTypeDxeCore:
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSmmDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            fileBuffer.append("VOID\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessLibraryDestructorList (\r\n");
            fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
            fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");
            //
            // If no library destructor function, return EFI_SUCCESS.
            //

            for (int i = 0; i < libInstanceList.size(); i++) {
                if (isFirst) {
                    fileBuffer.append("  EFI_STATUS  Status;\r\n");
                    fileBuffer.append("  Status = EFI_SUCCESS;\r\n");
                    fileBuffer.append("\r\n");
                    isFirst = false;
                }
                if (libInstanceList.get(i)[1].equalsIgnoreCase(EdkDefinitions.MODULE_TYPE_BASE)) {
                    fileBuffer.append("  Status = ");
                    fileBuffer.append(libInstanceList.get(i)[0]);
                    fileBuffer.append("();\r\n");
                    fileBuffer.append("  VOID\r\n");
                } else {
                    fileBuffer.append("  Status = ");
                    fileBuffer.append(libInstanceList.get(i)[0]);
                    fileBuffer.append("(ImageHandle, SystemTable);\r\n");
                    fileBuffer.append("  ASSERT_EFI_ERROR (Status);\r\n");
                }
            }
            fileBuffer.append("}\r\n");
            break;
        }
    }

    /**
      ExternsDriverBindingToAutoGenC
     
      This function is to write DRIVER_BINDING, COMPONENT_NAME,
      DRIVER_CONFIGURATION, DRIVER_DIAGNOSTIC in AutoGen.c.
     
      @param fileBuffer
                 String buffer for AutoGen.c
    **/
    void ExternsDriverBindingToAutoGenC(StringBuffer fileBuffer)
    throws EdkException {
        //
        // Get the arry of extern. The driverBindingGroup is a 2 dimension array.
        // The second dimension is include following element: DriverBinding, 
        // ComponentName, DriverConfiguration, DriverDiag;
        // 
        String[][] driverBindingGroup = this.saq.getExternProtocolGroup();


        //
        // inital BitMask;
        // 
        int BitMask = 0;

        //
        // Write driver binding protocol extern to autogen.c
        //
        for (int i = 0; i < driverBindingGroup.length; i++) {
            if (driverBindingGroup[i][0] != null) {
                globalDeclarations.append("extern EFI_DRIVER_BINDING_PROTOCOL ");
                globalDeclarations.append(driverBindingGroup[i][0]);
                globalDeclarations.append(";\r\n");
            }
        }

        //
        // Write component name protocol extern to autogen.c
        //
        if (!componentNamePcd) {
            for (int i = 0; i < driverBindingGroup.length; i++) {
                if (driverBindingGroup[i][1]!= null) {
                    if (driverBindingGroup[i][0] != null) {
                        BitMask |= 0x01;
                        globalDeclarations.append("extern EFI_COMPONENT_NAME_PROTOCOL ");
                        globalDeclarations.append(driverBindingGroup[i][1]);
                        globalDeclarations.append(";\r\n");
                    } else {
                        throw new AutoGenException("DriverBinding can't be empty!!");
                    }
                }
            }
        }

        //
        // Write driver configration protocol extern to autogen.c
        //
        for (int i = 0; i < driverBindingGroup.length; i++) {
            if (driverBindingGroup[i][2] != null) {
                if (driverBindingGroup[i][0] != null) {
                    BitMask |= 0x02;
                    globalDeclarations.append("extern EFI_DRIVER_CONFIGURATION_PROTOCOL ");
                    globalDeclarations.append(driverBindingGroup[i][2]);
                    globalDeclarations.append(";\r\n");
                } else {
                    throw new AutoGenException("DriverBinding can't be empty!!");
                }
            }
        }

        //
        // Write driver dignastic protocol extern to autogen.c
        //
        if (!driverDiagnostPcd) {
            for (int i = 0; i < driverBindingGroup.length; i++) {
                if (driverBindingGroup[i][3] != null) {
                    if (driverBindingGroup[i][0] != null) {
                        BitMask |= 0x04;
                        globalDeclarations.append("extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL ");
                        globalDeclarations.append(driverBindingGroup[i][3]);
                        globalDeclarations.append(";\r\n");
                    } else {
                        throw new AutoGenException("DriverBinding can't be empty!!");
                    }
                }
            }
        }


        //
        // Write driver module protocol bitmask.
        //
        fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverModelProtocolBitmask = ");
        fileBuffer.append(Integer.toString(BitMask));
        fileBuffer.append(";\r\n");

        //
        // Write driver module protocol list entry
        //
        fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED const UINTN  _gDriverModelProtocolListEntries = ");

        fileBuffer.append(Integer.toString(driverBindingGroup.length));
        fileBuffer.append(";\r\n");

        //
        // Write drive module protocol list to autogen.c
        //
        if (driverBindingGroup.length > 0) {
            fileBuffer.append("GLOBAL_REMOVE_IF_UNREFERENCED const EFI_DRIVER_MODEL_PROTOCOL_LIST  _gDriverModelProtocolList[] = {");
        }


        for (int i = 0; i < driverBindingGroup.length; i++) {
            if (i != 0) {
                fileBuffer.append(",");
            }
            //
            //  DriverBinding
            // 
            fileBuffer.append("\r\n {\r\n");
            fileBuffer.append("  &");
            fileBuffer.append(driverBindingGroup[i][0]);
            fileBuffer.append(", \r\n");

            //
            //  ComponentName
            // 
            if (driverBindingGroup[i][1] != null && componentNamePcd != true) {
                fileBuffer.append("  &");
                fileBuffer.append(driverBindingGroup[i][1]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }

            //
            // DriverConfiguration
            // 
            if (driverBindingGroup[i][2] != null) {
                fileBuffer.append("  &");
                fileBuffer.append(driverBindingGroup[i][2]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }

            //
            // DriverDiagnostic
            // 
            if (driverBindingGroup[i][3] != null && driverDiagnostPcd != true) {
                fileBuffer.append("  &");
                fileBuffer.append(driverBindingGroup[i][3]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }
            fileBuffer.append("  }");
        }

        if (driverBindingGroup.length > 0) {
            fileBuffer.append("\r\n};\r\n");
        }
    }

    /**
      ExternCallBackToAutoGenC
     
      This function adds <SetVirtualAddressMapCallBack> and
      <ExitBootServicesCallBack> infomation to AutoGen.c
     
      @param fileBuffer
                 String buffer for AutoGen.c
      @throws BuildException
    **/
    void ExternCallBackToAutoGenH(String moduleType)
    throws EdkException {
        //
        // Collect module's <SetVirtualAddressMapCallBack> and
        // <ExitBootServiceCallBack> and add to setVirtualAddList
        //  exitBootServiceList.
        //
        String[] setVirtuals = saq.getSetVirtualAddressMapCallBackArray();
        String[] exitBoots = saq.getExitBootServicesCallBackArray();
        //
        //  Add c code in autogen.c which relate to <SetVirtualAddressMapCallBack>
        //  and <ExitBootServicesCallBack>
        //
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            //
            // Write SetVirtualAddressMap function definition.
            //
            for (int i = 0; setVirtuals != null && i < setVirtuals.length; i++) {
                if (setVirtuals[i].equalsIgnoreCase("")) {
                    continue;
                }
                functionDeclarations.append("VOID\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(setVirtuals[i]);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_EVENT  Event,\r\n");
                functionDeclarations.append("  IN VOID       *Context\r\n");
                functionDeclarations.append("  );\r\n\r\n");
            }

            //
            // Write DriverExitBootServices function definition.
            //
            for (int i = 0; exitBoots != null && i < exitBoots.length; i++) {
                if (exitBoots[i].equalsIgnoreCase("")) {
                    continue;
                }

                functionDeclarations.append("VOID\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(exitBoots[i]);
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_EVENT  Event,\r\n");
                functionDeclarations.append("  IN VOID       *Context\r\n");
                functionDeclarations.append("  );\r\n\r\n");
            }
            break;
        default:
            break;
        }
    }

    /**
      ExternCallBackToAutoGenC
     
      This function adds <SetVirtualAddressMapCallBack> and
      <ExitBootServicesCallBack> infomation to AutoGen.c
     
      @param fileBuffer
                 String buffer for AutoGen.c
      @throws BuildException
    **/
    void ExternCallBackToAutoGenC(StringBuffer fileBuffer)
    throws EdkException {
        //
        // Collect module's <SetVirtualAddressMapCallBack> and
        // <ExitBootServiceCallBack> and add to setVirtualAddList
        //  exitBootServiceList.
        //
        String[] setVirtuals = saq.getSetVirtualAddressMapCallBackArray();
        String[] exitBoots = saq.getExitBootServicesCallBackArray();
        if (setVirtuals != null) {
            for (int j = 0; j < setVirtuals.length; j++) {
                this.setVirtalAddList.add(setVirtuals[j]);
            }
        }
        if (exitBoots != null) {
            for (int k = 0; k < exitBoots.length; k++) {
                this.exitBootServiceList.add(exitBoots[k]);
            }
        }
        //
        //  Add c code in autogen.c which relate to <SetVirtualAddressMapCallBack>
        //  and <ExitBootServicesCallBack>
        //
        String moduleType = this.moduleId.getModuleType();
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            //
            //  If moduleType is one of above, call setVirtualAddressToAutogenC,
            //  and setExitBootServiceToAutogenC.
            // 
            setVirtualAddressToAutogenC(fileBuffer);
            setExitBootServiceToAutogenC(fileBuffer);
            break;
        default:
            break;
        }
    }

    /**
      copyFlashMapHToDebugDir
      
      This function is to copy the falshmap.h to debug directory and change 
      its name to TianoR8FlashMap.h
      
      @param 
      @return
    **/
    private void copyFlashMapHToDebugDir() throws  AutoGenException{

        File inFile = new File(fvDir + File.separatorChar + CommonDefinition.FLASHMAPH);
        int size = (int)inFile.length();
        byte[] buffer = new byte[size];
        File outFile = new File (this.outputPath + File.separatorChar + CommonDefinition.TIANOR8PLASHMAPH);
        //
        //  If TianoR8FlashMap.h existed and the flashMap.h don't change,
        //  do nothing.
        //
        if ((!outFile.exists()) ||(inFile.lastModified() - outFile.lastModified()) >= 0) {
            if (inFile.exists()) {
                try {
                    FileInputStream fis = new FileInputStream (inFile);
                    fis.read(buffer);
                    FileOutputStream fos = new FileOutputStream(outFile);
                    fos.write(buffer);
                    fis.close();
                    fos.close();
                } catch (IOException e) {
                    throw new AutoGenException("The file, flashMap.h can't be open!");
                }

            } else {
                throw new AutoGenException("The file, flashMap.h doesn't exist!");
            }
        }
    }

    /**
      This function first order the library instances, then collect
      library instance 's PPI, Protocol, GUID,
      SetVirtalAddressMapCallBack, ExitBootServiceCallBack, and
      Destructor, Constructor.
      
      @param
      @return    
    **/
    private void collectLibInstanceInfo() throws EdkException{
        int index;

        String moduleType = moduleId.getModuleType();
        String libConstructName = null;
        String libDestructName = null;
        String libModuleType   = null;
        String[] setVirtuals = null;
        String[] exitBoots = null;

        ModuleIdentification[] libraryIdList = saq.getLibraryInstance(this.arch);
        if (libraryIdList.length <= 0) {
            return;
        }
        //
        // Reorder library instance sequence.
        //
        AutogenLibOrder libOrder = new AutogenLibOrder(libraryIdList, this.arch);
        List<ModuleIdentification> orderList = libOrder.orderLibInstance();
        //
        // Process library instance one by one.
        //
        for (int i = 0; i < orderList.size(); i++) {
            //
            // Get library instance basename.
            //
            ModuleIdentification libInstanceId = orderList.get(i);

            //
            // Get override map
            //

            Map<String, XmlObject> libDoc = GlobalData.getDoc(libInstanceId, this.arch);
            saq.push(libDoc);
            //
            // check if the library instance support current module
            // 
            String[] libraryClassList = saq.getLibraryClasses(CommonDefinition.ALWAYSPRODUCED,
                                                              this.arch,
                                                              moduleType
                                                             );
            if (libraryClassList.length <= 0) {
                throw new EdkException("Library instance " + libInstanceId.getName() 
                                       + " doesn't support module type " + moduleType);
            }
            //
            // Get CName list from <PPis>, <Protocols>, <Guids>, etc. of this library
            // instance.
            //
            String[] guidCNameList = saq.getCNameArray(this.arch);            
            PackageIdentification[] pkgList = saq.getDependencePkg(this.arch);

            //
            // Add those ppi, protocol, guid in global ppi,
            // protocol, guid
            // list.
            //
            for (index = 0; index < guidCNameList.length; index++) {
                this.mGuidCNameList.add(guidCNameList[index]);
            }

            for (index = 0; index < pkgList.length; index++) {
                if (!this.mDepPkgList.contains(pkgList[index])) {
                    this.mDepPkgList.add(pkgList[index]);
                }
            }

            //
            // If not yet parse this library instance's constructor
            // element,parse it.
            //
            libConstructName = saq.getLibConstructorName();
            libDestructName = saq.getLibDestructorName();
            libModuleType = saq.getModuleType();

            //
            // Collect SetVirtualAddressMapCallBack and
            // ExitBootServiceCallBack.
            //
            setVirtuals = saq.getSetVirtualAddressMapCallBackArray();
            exitBoots = saq.getExitBootServicesCallBackArray();
            if (setVirtuals != null) {
                for (int j = 0; j < setVirtuals.length; j++) {
                    this.setVirtalAddList.add(setVirtuals[j]);
                }
            }
            if (exitBoots != null) {
                for (int k = 0; k < exitBoots.length; k++) {
                    this.exitBootServiceList.add(exitBoots[k]);
                }
            }
            saq.pop();
            //
            // Add dependent library instance constructor function.
            //
            if (libConstructName != null) {
                this.libConstructList.add(new String[] {libConstructName, libModuleType});
            }
            //
            // Add dependent library instance destructor fuction.
            //
            if (libDestructName != null) {
                this.libDestructList.add(new String[] {libDestructName, libModuleType});
            }
        }
    }

    private void setVirtualAddressToAutogenC(StringBuffer fileBuffer){
        //
        // Entry point lib for these module types needs to know the count
        // of entryPoint.
        //
        fileBuffer.append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED  const UINTN _gDriverSetVirtualAddressMapEventCount = ");

        //
        // If the list is not valid or has no entries set count to zero else
        // set count to the number of valid entries
        //
        int Count = 0;
        int i = 0;
        if (this.setVirtalAddList != null) {
            for (i = 0; i < this.setVirtalAddList.size(); i++) {
                if (this.setVirtalAddList.get(i).equalsIgnoreCase("")) {
                    break;
                }
            }
            Count = i;
        }

        fileBuffer.append(Integer.toString(Count));
        fileBuffer.append(";\r\n\r\n");
        if (this.setVirtalAddList == null || this.setVirtalAddList.size() == 0) {
            //
            // No data so make a NULL list
            //
            fileBuffer.append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverSetVirtualAddressMapEvent[] = {\r\n");
            fileBuffer.append("  NULL\r\n");
            fileBuffer.append("};\r\n\r\n");
        } else {
            //
            // Write SetVirtualAddressMap function definition.
            //
            for (i = 0; i < this.setVirtalAddList.size(); i++) {
                if (this.setVirtalAddList.get(i).equalsIgnoreCase("")) {
                    break;
                }
                functionDeclarations.append("VOID\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(this.setVirtalAddList.get(i));
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_EVENT  Event,\r\n");
                functionDeclarations.append("  IN VOID       *Context\r\n");
                functionDeclarations.append("  );\r\n\r\n");
            }

            //
            // Write SetVirtualAddressMap entry point array.
            //
            fileBuffer.append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverSetVirtualAddressMapEvent[] = {");
            for (i = 0; i < this.setVirtalAddList.size(); i++) {
                if (this.setVirtalAddList.get(i).equalsIgnoreCase("")) {
                    break;
                }

                if (i == 0) {
                    fileBuffer.append("\r\n  ");
                } else {
                    fileBuffer.append(",\r\n  ");
                }

                fileBuffer.append(this.setVirtalAddList.get(i));
            }
            //
            // add the NULL at the end of _gDriverSetVirtualAddressMapEvent list.
            //
            fileBuffer.append(",\r\n  NULL");
            fileBuffer.append("\r\n};\r\n\r\n");
        }
    }


    private void setExitBootServiceToAutogenC(StringBuffer fileBuffer){
        //
        // Entry point lib for these module types needs to know the count.
        //
        fileBuffer.append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED  const UINTN _gDriverExitBootServicesEventCount = ");

        //
        // If the list is not valid or has no entries set count to zero else
        // set count to the number of valid entries.
        //
        int Count = 0;
        int i = 0; 
        if (this.exitBootServiceList != null) {
            for (i = 0; i < this.exitBootServiceList.size(); i++) {
                if (this.exitBootServiceList.get(i).equalsIgnoreCase("")) {
                    break;
                }
            }
            Count = i;
        }
        fileBuffer.append(Integer.toString(Count));
        fileBuffer.append(";\r\n\r\n");

        if (this.exitBootServiceList == null || this.exitBootServiceList.size() == 0) {
            //      
            // No data so make a NULL list.
            //
            fileBuffer.append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverExitBootServicesEvent[] = {\r\n");
            fileBuffer.append("  NULL\r\n");
            fileBuffer.append("};\r\n\r\n");
        } else {
            //
            // Write DriverExitBootServices function definition.
            //
            for (i = 0; i < this.exitBootServiceList.size(); i++) {
                if (this.exitBootServiceList.get(i).equalsIgnoreCase("")) {
                    break;
                }

                functionDeclarations.append("VOID\r\n");
                functionDeclarations.append("EFIAPI\r\n");
                functionDeclarations.append(this.exitBootServiceList.get(i));
                functionDeclarations.append(" (\r\n");
                functionDeclarations.append("  IN EFI_EVENT  Event,\r\n");
                functionDeclarations.append("  IN VOID       *Context\r\n");
                functionDeclarations.append("  );\r\n\r\n");
            }

            //
            // Write DriverExitBootServices entry point array.
            //
            fileBuffer.append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverExitBootServicesEvent[] = {");
            for (i = 0; i < this.exitBootServiceList.size(); i++) {
                if (this.exitBootServiceList.get(i).equalsIgnoreCase("")) {
                    break;
                }

                if (i == 0) {
                    fileBuffer.append("\r\n  ");
                } else {
                    fileBuffer.append(",\r\n  ");
                }
                fileBuffer.append(this.exitBootServiceList.get(i));
            }

            fileBuffer.append(",\r\n  NULL");
            fileBuffer.append("\r\n};\r\n\r\n");
        }   
    }
    /**
      setPcdComponentName
      
         Get the Pcd Value of ComponentName to 
         decide whether need to disable the componentName.         
         
    **/
    public void setPcdComponentName (){
        String pcdValue = null;
        pcdValue = saq.getPcdValueBycName("PcdComponentNameDisable");
        if (pcdValue != null && pcdValue.equalsIgnoreCase("true")) {
            this.componentNamePcd = true;
        }
    }

    /**
      setPcdDriverDiagnostic 
      
        Get the Pcd Value of DriverDiagnostic to 
        decide whether need to disable DriverDiagnostic.
         
    **/
    public void setPcdDriverDiagnostic (){
        String pcdValue = null;
        pcdValue  = saq.getPcdValueBycName("PcdDriverDiagnosticsDisable");
        if (pcdValue != null && pcdValue.equalsIgnoreCase("true")) {
            this.driverDiagnostPcd = true;
        }
    }  

}
