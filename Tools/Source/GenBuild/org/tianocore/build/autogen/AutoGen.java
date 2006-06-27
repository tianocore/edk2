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

import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.Spd;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.GuidsDocument;
import org.tianocore.LibraryClassDocument.LibraryClass;
import org.tianocore.PPIsDocument;
import org.tianocore.ProtocolsDocument;
import org.tianocore.build.pcd.action.PCDAutoGenAction;

import org.apache.tools.ant.BuildException;
import org.apache.xmlbeans.XmlObject;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

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
    /// The base name of module or library.
    ///
    private String baseName;

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
    /// The protocl list which records in module or library surface area and
    /// it's dependence on library instance surface area.
    ///
    private List<String> mProtocolList = new ArrayList<String>();

    ///
    /// The Ppi list which recorded in module or library surface area and its
    /// dependency on library instance surface area.
    ///
    private List<String> mPpiList = new ArrayList<String>();

    ///
    /// The Guid list which recoreded in module or library surface are and it's
    /// dependence on library instance surface area.
    ///
    private List<GuidsDocument.Guids.GuidEntry> mGuidList = new ArrayList<GuidsDocument.Guids.GuidEntry>();

    /**
      Construct function
      
      This function mainly initialize some member variable.
      
      @param outputPath    Output path of AutoGen file.
      @param baseName      Module base name.
      @param arch          Target architecture.
    **/
    public AutoGen(String outputPath, String baseName, String arch) {
        this.outputPath = outputPath;
        this.baseName = baseName;
        this.arch = arch;

    }

    /**
      saveFile function
      
      This function save the content in stringBuffer to file.
      
      @param fileName      The name of file.
      @param fileBuffer    The content of AutoGen file in buffer.
      @return              "true" successful, "false" failed.
    **/
    private boolean saveFile(String fileName, StringBuffer fileBuffer) {
        try {
            File autoGenH = new File(fileName);

            //
            // if the file exists, compare their content
            //
            if (autoGenH.exists()) {
                FileReader fIn = new FileReader(autoGenH);
                char[] oldFileBuffer = new char[(int) autoGenH.length()];
                fIn.read(oldFileBuffer, 0, (int) autoGenH.length());
                fIn.close();

                //
                // if we got the same file, don't re-generate it to prevent
                // sources depending on it from re-building
                //
                if (fileBuffer.toString().compareTo(new String(oldFileBuffer)) == 0) {
                    return true;
                }
            }
            FileWriter fOut = new FileWriter(autoGenH);
            fOut.write(fileBuffer.toString());
            fOut.close();
        } catch (Exception e) {
            return false;
        }
        return true;
    }

    /**
      genAutogen function
      
      This function call libGenAutoGen or moduleGenAutogen function, which
      dependence on generate library autogen or module autogen.
      
      @throws BuildException    Failed to creat AutoGen.c & AutoGen.h.
    **/
    public void genAutogen() throws BuildException {
        try {
            //
            // If outputPath do not exist, create it.
            //
            File path = new File(outputPath);
            path.mkdirs();

            //
            // Check current is library or not, then call the corresponding
            // function.
            //
            if (SurfaceAreaQuery.getComponentType().equalsIgnoreCase(
                    CommonDefinition.LibraryStr)) {
                libGenAutogen();
            } else {
                moduleGenAutogen();
            }

        } catch (Exception e) {
            throw new BuildException(
                    "Failed to create AutoGen.c & AutoGen.h!\n"
                            + e.getMessage());
        }
    }

    /** 
      moduleGenAutogen function
    
      This function generates AutoGen.c & AutoGen.h for module.
     
      @throws BuildException  Faile to create module AutoGen.c & AutoGen.h.
    **/
    void moduleGenAutogen() throws BuildException {

        try {
            moduleGenAutogenC();
            moduleGenAutogenH();
        } catch (Exception e) {
            throw new BuildException(
                    "Faile to create module AutoGen.c & AutoGen.h!\n"
                            + e.getMessage());
        }
    }

    /**
      libGenAutogen function
      
      This function generates AutoGen.c & AutoGen.h for library.
      
      @throws BuildException
                  Faile to create library AutoGen.c & AutoGen.h
    **/
    void libGenAutogen() throws BuildException {
        try {
            libGenAutogenC();
            libGenAutogenH();
        } catch (Exception e) {
            throw new BuildException(
                    "Faile to create library AutoGen.c & AutoGen.h!\n"
                            + e.getMessage());
        }
    }

    /**
      moduleGenAutogenH
     
      This function generates AutoGen.h for module.
      
      @throws BuildException
                  Failed to generate AutoGen.h.
    **/
    void moduleGenAutogenH() throws BuildException {

        List<String> libClassIncludeH;
        String moduleType;
        List<String> headerFileList;

        StringBuffer fileBuffer = new StringBuffer(8192);
        
        //
        // Write Autogen.h header notation 
        //
        fileBuffer.append(CommonDefinition.autogenHNotation);
        
        //
        // Add #ifndef  ${BaseName}_AUTOGENH
        //     #def     ${BseeName}_AUTOGENH
        //
        fileBuffer.append("#ifndef    " + this.baseName.toUpperCase() + "_AUTOGENH\r\n");
        fileBuffer.append("#define    " + this.baseName.toUpperCase() + "_AUTOGENH\r\n\r\n");
        
        //
        // Write the specification version and release version at the begine
        // of autogen.h file.
        // Note: the specification version and release version should
        // be got from module surface area instead of hard code by it's moduleType.
        //
        moduleType = SurfaceAreaQuery.getModuleType();
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeDxeCore:
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSmmDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            fileBuffer.append(CommonDefinition.autoGenHLine1);
            break;
        default:
            fileBuffer.append(CommonDefinition.autoGenHVersionDefault);
            break;
        }
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            fileBuffer.append(CommonDefinition.autoGenHReleaseDefault);
            break;
        default:
            fileBuffer.append(CommonDefinition.autoGenHLine2);
            break;
        }

        //
        // Add "extern int __make_me_compile_correctly;" at begin of 
        // AutoGen.h.
        //
        fileBuffer.append(CommonDefinition.autoGenHbegin);

        //
        // Write consumed package's mdouleInfo related .h file to autogen.h
        //
        List<String> consumedPkgList = SurfaceAreaQuery
                .getIncludePackageName(this.arch);
        if (consumedPkgList != null) {
            headerFileList = IncludesToAutogenH(consumedPkgList, moduleType);
            for (int i = 0; i < headerFileList.size(); i++) {
                fileBuffer.append(headerFileList.get(i));
            }
        }

        //
        // Write library class's related *.h file to autogen.h.
        //
        LibraryClass[] libClassList = SurfaceAreaQuery
                .getLibraryClassArray(CommonDefinition.AlwaysConsumed);
        if (libClassList != null) {
            libClassIncludeH = LibraryClassToAutogenH(libClassList);
            for (int i = 0; i < libClassIncludeH.size(); i++) {
                fileBuffer.append(libClassIncludeH.get(i));
            }
        }

        libClassList = SurfaceAreaQuery
                .getLibraryClassArray(CommonDefinition.AlwaysProduced);
        if (libClassList != null) {
            libClassIncludeH = LibraryClassToAutogenH(libClassList);
            for (int i = 0; i < libClassIncludeH.size(); i++) {
                fileBuffer.append(libClassIncludeH.get(i));
            }
        }
        fileBuffer.append("\r\n");
        
        //
        // Write PCD autogen information to AutoGen.h.
        //
        if (this.myPcdAutogen != null) {
            fileBuffer.append(this.myPcdAutogen.OutputH());
        }

        //
        // Append the #endif at AutoGen.h
        //
        fileBuffer.append("#endif\r\n");
        
        //
        // Save string buffer content in AutoGen.h.
        //
        if (!saveFile(outputPath + File.separatorChar + "AutoGen.h", fileBuffer)) {
            throw new BuildException("Failed to generate AutoGen.h !!!");
        }
    }
    
    /**
      moduleGenAutogenC
   
      This function generates AutoGen.c for module.
    
      @throws BuildException
                Failed to generate AutoGen.c.
    **/
    void moduleGenAutogenC() throws BuildException {

        StringBuffer fileBuffer = new StringBuffer(8192);
        //
        // Write Autogen.c header notation 
        //
        fileBuffer.append(CommonDefinition.autogenCNotation);
        
        //
        // Write #include <AutoGen.h> at beginning of AutoGen.c
        //
        fileBuffer.append(CommonDefinition.includeAutogenH);
        
        //
        // Write DriverBinding/ComponentName/DriverConfiguration/DriverDialog
        // to AutoGen.c
        //
        ExternsDriverBindingToAutoGenC(fileBuffer);

        //
        // Write DriverExitBootServicesEvent/DriverSetVirtualAddressMapEvent
        // to Autogen.c
        //
        ExternCallBackToAutoGenC(fileBuffer);

        //
        // Write EntryPoint to autgoGen.c
        //
        String[] entryPointList = SurfaceAreaQuery.getModuleEntryPointArray();
        if (entryPointList != null) {
            EntryPointToAutoGen(entryPointList, fileBuffer);
        }

        //
        // Write Guid to autogen.c
        //
        String guid = SurfaceAreaQuery.getModuleGuid();
        fileBuffer
                .append("GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiCallerIdGuid = {");
        if (guid == null) {
            throw new BuildException("Guid value must set!\n");
        }

        //
        // Formate Guid as ANSI c form.Example:
        // {0xd2b2b828, 0x826, 0x48a7,{0xb3, 0xdf, 0x98, 0x3c, 0x0, 0x60, 0x24, 0xf0}}
        //
        fileBuffer.append(Spd.formatGuidName(guid));
        fileBuffer.append("};\r\n");

        //
        // Generate library instance consumed protocol, guid, ppi, pcd list.
        // Save those to this.protocolList, this.ppiList, this.pcdList,
        // this.guidList. Write Consumed library constructor and desconstuct to
        // autogen.c
        //
        LibInstanceToAutogenC(fileBuffer);

        //
        // Write consumed ppi, guid, protocol to autogen.c
        //
        ProtocolGuidToAutogenC(fileBuffer);
        PpiGuidToAutogenC(fileBuffer);
        GuidGuidToAutogenC(fileBuffer);

        //
        // Call pcd autogen. PCDAutoGenAction tool only need module name and
        // isPcdEmulatedDriver as parameter. Library inherits PCD and module's
        // PCD information has been collected in FPDParser task by
        // CollectPCDAction. 
        // Note : when PCD image tool ready,
        // isPCDEmulatedDriver parameter will be removed.
        //
        try {
            this.myPcdAutogen = new PCDAutoGenAction(baseName,
                                                     null,
                                                     null,
                                                     null,
                                                     this.arch,
                                                     null,
                                                     false,
                                                     SurfaceAreaQuery.getModulePcdEntryNameArray());
            this.myPcdAutogen.execute();
        } catch (Exception e) {
            throw new BuildException("PCD Autogen for module failed:" + e.getMessage());
        }

        if (this.myPcdAutogen != null) {
            fileBuffer.append(this.myPcdAutogen.OutputC());
        }

        if (!saveFile(outputPath + File.separatorChar + "AutoGen.c", fileBuffer)) {
            throw new BuildException("Failed to generate AutoGen.c !!!");
        }

    }
    
    /**
      libGenAutogenH
   
      This function generates AutoGen.h for library.
    
      @throws BuildException
                Failed to generate AutoGen.c.
    **/
    void libGenAutogenH() throws BuildException {

        List<String> libClassIncludeH;
        String moduleType;
        List<String> headerFileList;
        StringBuffer fileBuffer = new StringBuffer(10240);

        //
        // Write Autogen.h header notation 
        //
        fileBuffer.append(CommonDefinition.autogenHNotation);
        
        //
        // Add #ifndef  ${BaseName}_AUTOGENH
        //     #def     ${BseeName}_AUTOGENH
        //
        fileBuffer.append("#ifndef    " + this.baseName.toUpperCase() + "_AUTOGENH\r\n");
        fileBuffer.append("#define    " + this.baseName.toUpperCase() + "_AUTOGENH\r\n\r\n");
        
        //
        // Write EFI_SPECIFICATION_VERSION and EDK_RELEASE_VERSION 
        // to autogen.h file.
        // Note: the specification version and release version should
        // be get from module surface area instead of hard code.
        //
        fileBuffer.append(CommonDefinition.autoGenHbegin);
        fileBuffer.append(CommonDefinition.autoGenHLine1);
        fileBuffer.append(CommonDefinition.autoGenHLine2);

        //
        // Write consumed package's mdouleInfo related *.h file to autogen.h.
        // 
        moduleType = SurfaceAreaQuery.getModuleType();
        List<String> cosumedPkglist = SurfaceAreaQuery
                .getIncludePackageName(this.arch);
        headerFileList = IncludesToAutogenH(cosumedPkglist, moduleType);
        for (int i = 0; i < headerFileList.size(); i++) {
            fileBuffer.append(headerFileList.get(i));
        }

        //
        // Write library class's related *.h file to autogen.h
        //
        LibraryClass[] libClassList = SurfaceAreaQuery
                .getLibraryClassArray(CommonDefinition.AlwaysConsumed);
        if (libClassList != null) {
            libClassIncludeH = LibraryClassToAutogenH(libClassList);
            for (int i = 0; i < libClassIncludeH.size(); i++) {
                fileBuffer.append(libClassIncludeH.get(i));
            }
        }

        libClassList = SurfaceAreaQuery
                .getLibraryClassArray(CommonDefinition.AlwaysProduced);
        if (libClassList != null) {
            libClassIncludeH = LibraryClassToAutogenH(libClassList);
            for (int i = 0; i < libClassIncludeH.size(); i++) {
                fileBuffer.append(libClassIncludeH.get(i));
            }
        }
        fileBuffer.append("\r\n");
        
        //
        // Write PCD information to library AutoGen.h.
        //
        if (this.myPcdAutogen != null) {
            fileBuffer.append(this.myPcdAutogen.OutputH());
        }

        //
        // Append the #endif at AutoGen.h
        //
        fileBuffer.append("#endif\r\n");
        
        //
        // Save content of string buffer to AutoGen.h file.
        //
        if (!saveFile(outputPath + File.separatorChar + "AutoGen.h", fileBuffer)) {
            throw new BuildException("Failed to generate AutoGen.h !!!");
        }
    }

    /**
      libGenAutogenC
   
      This function generates AutoGen.h for library.
    
      @throws BuildException
                Failed to generate AutoGen.c.
    **/
    void libGenAutogenC() throws BuildException {
        StringBuffer fileBuffer = new StringBuffer(10240);

        //
        // Write Autogen.c header notation 
        //
        fileBuffer.append(CommonDefinition.autogenCNotation);
        
        fileBuffer.append(CommonDefinition.autoGenCLine1);
        fileBuffer.append("\r\n");

        //
        // Call pcd autogen. PCDAutoGenAction tool only need module name and
        // isPcdEmulatedDriver as parameter. Library inherit PCD and module's
        // PCD information has been collected in FPDParser task by
        // CollectPCDAction. 
        // Note : when PCD image tool ready,
        // isPCDEmulatedDriver parameter will be removed.
        //
        try {
            this.myPcdAutogen = new PCDAutoGenAction(baseName, 
                                                     null,
                                                     null,
                                                     null,
                                                     this.arch,
                                                     null,
                                                     true,
                                                     SurfaceAreaQuery.getModulePcdEntryNameArray());
            this.myPcdAutogen.execute();
        } catch (Exception e) {
            throw new BuildException("Pcd Autogen for library failed! " + e.getMessage());
        }

        if (this.myPcdAutogen != null) {
            fileBuffer.append(this.myPcdAutogen.OutputC());
        }

        if (!saveFile(outputPath + File.separatorChar + "AutoGen.c", fileBuffer)) {
            throw new BuildException("Failed to generate AutoGen.c !!!");
        }
    }

    /**
      LibraryClassToAutogenH
      
      This function returns *.h files declared by library classes which are 
      consumed or produced by current build module or library.
      
      @param   libClassList     List of library class which consumed or produce
                                by current build module or library.
      @return  includeStrList   List of *.h file.             
    **/
    List<String> LibraryClassToAutogenH(LibraryClass[] libClassList) {
        List<String> includStrList = new ArrayList<String>();
        String includerName;
        String str = "";
       
        //
        // Get include file from GlobalData's SPDTable according to 
        // library class name.
        //
        for (int i = 0; i < libClassList.length; i++) {
            includerName = GlobalData.getLibClassIncluder(getStringValue((XmlObject)libClassList[i]));
            if (includerName != null) {
                str = CommonDefinition.include + " " + "<";
                str = str + includerName + ">\r\n";
                includStrList.add(str);
                includerName = null;
            }
        }
        return includStrList;
    }

    /**
      IncludesToAutogenH
      
      This function add include file in AutoGen.h file.
      @param   packageNameList   List of module depended package.
      @param   moduleType        Module type.
      @return 
    **/
    List<String> IncludesToAutogenH(List<String> packageNameList,
            String moduleType) {

        List<String> includeStrList = new ArrayList<String>();
        String packageName = "";
        String includeStr = "";

        //
        // Get include file from moduleInfo file
        //
        for (int i = 0; i < packageNameList.size(); i++) {
            packageName = packageNameList.get(i);
            includeStr = GlobalData.getModuleInfoByPackageName(packageName,
                    moduleType);
            includeStrList.add(includeStr);
        }
        return includeStrList;
    }

    /**
      EntryPointToAutoGen 
      
      This function convert <ModuleEntryPoint> & <ModuleUnloadImage> information
      in mas to AutoGen.c
      
      @param  entryPointList    List of entry point.                            
      @param  fileBuffer        String buffer fo AutoGen.c.
      @throws Exception
    **/
    void EntryPointToAutoGen(String[] entryPointList, StringBuffer fileBuffer)
            throws BuildException {

        String typeStr = SurfaceAreaQuery.getModuleType();
        
        //
        // The parameters and return value of entryPoint is difference
        // for difference module type.
        //
        switch (CommonDefinition.getModuleType(typeStr)) {

        case CommonDefinition.ModuleTypePeiCore:
            if (entryPointList.length != 1 || entryPointList[0].equals("")) {
                throw new BuildException(
                        "Module type = 'PEI_CORE', only have one module entry point!");
            } else {
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (\r\n");
                fileBuffer
                        .append("  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,\r\n");
                fileBuffer
                        .append("  IN VOID                        *OldCoreData\r\n");
                fileBuffer.append("  );\r\n\r\n");

                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer
                        .append("  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,\r\n");
                fileBuffer
                        .append("  IN VOID                        *OldCoreData\r\n");
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
            if (entryPointList.length != 1 || entryPointList[0].equals("")) {
                throw new BuildException(
                        "Module type = 'DXE_CORE', only have one module entry point!");
            } else {

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (\n");
                fileBuffer.append("  IN VOID  *HobStart\r\n");
                fileBuffer.append("  );\r\n\r\n");

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
            int entryPointCount = 0;
            fileBuffer
                    .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = 0;\r\n");
            for (int i = 0; i < entryPointList.length; i++) {
                if (!entryPointList[i].equals("")) {
                    fileBuffer.append("EFI_STATUS\r\n");
                    fileBuffer.append("EFIAPI\r\n");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (\r\n");
                    fileBuffer
                            .append("  IN EFI_FFS_FILE_HEADER  *FfsHeader,\r\n");
                    fileBuffer
                            .append("  IN EFI_PEI_SERVICES     **PeiServices\r\n");
                    fileBuffer.append("  );\r\n");
                    entryPointCount++;
                } else {
                    break;
                }
            }

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleEntryPointList (\r\n");
            fileBuffer.append("  IN EFI_FFS_FILE_HEADER  *FfsHeader,\r\n");
            fileBuffer.append("  IN EFI_PEI_SERVICES     **PeiServices\r\n");
            fileBuffer.append("  )\r\n\r\n");
            fileBuffer.append("{\r\n");

            if (entryPointCount == 0) {
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
            } else if (entryPointCount == 1) {
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (FfsHeader, PeiServices);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n");
                fileBuffer.append("  EFI_STATUS  CombinedStatus;\r\n\r\n");
                fileBuffer.append("  CombinedStatus = EFI_LOAD_ERROR;\r\n\r\n");
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append("  Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append(" (FfsHeader, PeiServices)\r\n");
                        fileBuffer
                                .append("  if (!EFI_ERROR (Status) || EFI_ERROR (CombinedStatus)) {\r\n");
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
            for (int i = 0; i < entryPointList.length; i++) {
                if (!entryPointList[i].equals("")) {
                    fileBuffer.append("EFI_STATUS\r\n");
                    fileBuffer.append("EFIAPI\r\n");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (\r\n");
                    fileBuffer.append("  EFI_HANDLE        ImageHandle,\r\n");
                    fileBuffer.append("  EFI_SYSTEM_TABLE  *SystemTable\r\n");
                    fileBuffer.append("  );\r\n");
                    entryPointCount++;
                } else {
                    break;
                }
            }
            fileBuffer
                    .append("GLOBAL_REMOVE_IF_UNREFERENCED  const UINT8  _gDriverEntryPointCount = ");
            fileBuffer.append(Integer.toString(entryPointCount));
            fileBuffer.append(";\r\n");
            fileBuffer
                    .append("static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;\r\n");
            fileBuffer
                    .append("static EFI_STATUS  mDriverEntryPointStatus = EFI_LOAD_ERROR;\r\n\r\n");

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleEntryPointList (\r\n");
            fileBuffer.append("  EFI_HANDLE        ImageHandle,\r\n");
            fileBuffer.append("  EFI_SYSTEM_TABLE  *SystemTable\r\n");
            fileBuffer.append("  )\r\n\r\n");
            fileBuffer.append("{\r\n");

            for (int i = 0; i < entryPointList.length; i++) {
                if (!entryPointList[i].equals("")) {
                    fileBuffer
                            .append("  if (SetJump (&mJumpContext) == 0) {\r\n");
                    fileBuffer.append("    ExitDriver (");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (ImageHandle, SystemTable));\r\n");
                    fileBuffer.append("    ASSERT (FALSE);\r\n");
                    fileBuffer.append("  }\r\n");
                } else {
                    break;
                }
            }
            fileBuffer.append("  return mDriverEntryPointStatus;\r\n");
            fileBuffer.append("}\r\n\r\n");

            fileBuffer.append("VOID\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ExitDriver (\r\n");
            fileBuffer.append("  IN EFI_STATUS  Status\n");
            fileBuffer.append("  )\r\n\r\n");
            fileBuffer.append("{\r\n");
            fileBuffer
                    .append("  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {\r\n");
            fileBuffer.append("    mDriverEntryPointStatus = Status;\r\n");
            fileBuffer.append("  }\r\n");
            fileBuffer.append("  LongJump (&mJumpContext, (UINTN)-1);\r\n");
            fileBuffer.append("  ASSERT (FALSE);\r\n");
            fileBuffer.append("}\r\n\r\n");

            //
            // Add "ModuleUnloadImage" for DxeSmmDriver module type;
            //
            entryPointList = SurfaceAreaQuery.getModuleUnloadImageArray();
            entryPointCount = 0;
            if (entryPointList != null) {
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append("EFI_STATUS\r\n");
                        fileBuffer.append("EFIAPI\r\n");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append(" (\r\n");
                        fileBuffer
                                .append("  EFI_HANDLE        ImageHandle\r\n");
                        fileBuffer.append("  );\r\n");
                        entryPointCount++;
                    } else {
                        break;
                    }
                }
            }

            fileBuffer
                    .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverUnloadImageCount = ");
            fileBuffer.append(Integer.toString(entryPointCount));
            fileBuffer.append(";\r\n\r\n");

            if (entryPointList != null) {
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append("EFI_STATUS\r\n");
                        fileBuffer.append("EFIAPI\r\n");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append(" (\r\n");
                        fileBuffer
                                .append("  EFI_HANDLE        ImageHandle\r\n");
                        fileBuffer.append("  );\r\n");
                    } else {
                        break;
                    }
                }
            }

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleUnloadList (\r\n");
            fileBuffer.append("  EFI_HANDLE  ImageHandle\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");

            if (entryPointCount == 0) {
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
            } else if (entryPointCount == 1) {
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append("(ImageHandle);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n\r\n");
                fileBuffer.append("  Status = EFI_SUCCESS;\r\n\r\n");
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                        fileBuffer.append("    ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  } else {\r\n");
                        fileBuffer.append("    Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  }\r\n");
                    } else {
                        break;
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
            for (int i = 0; i < entryPointList.length; i++) {
                if (!entryPointList[i].equals("")) {
                    fileBuffer.append("EFI_STATUS\r\n");
                    fileBuffer.append("EFIAPI\r\n");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (\r\n");
                    fileBuffer.append("  EFI_HANDLE        ImageHandle,\r\n");
                    fileBuffer.append("  EFI_SYSTEM_TABLE  *SystemTable\r\n");
                    fileBuffer.append("  );\r\n");
                    entryPointCount++;
                } else {
                    break;
                }
            }

            fileBuffer
                    .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverEntryPointCount = ");
            fileBuffer.append(Integer.toString(entryPointCount));
            fileBuffer.append(";\r\n");
            if (entryPointCount > 1) {
                fileBuffer
                        .append("static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;\r\n");
                fileBuffer
                        .append("static EFI_STATUS  mDriverEntryPointStatus = EFI_LOAD_ERROR;\r\n");
            }
            fileBuffer.append("\n");

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleEntryPointList (\r\n");
            fileBuffer.append("  EFI_HANDLE        ImageHandle,\r\n");
            fileBuffer.append("  EFI_SYSTEM_TABLE  *SystemTable\r\n");
            fileBuffer.append("  )\r\n\r\n");
            fileBuffer.append("{\r\n");

            if (entryPointCount == 0) {
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
            } else if (entryPointCount == 1) {
                fileBuffer.append("  return (");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (ImageHandle, SystemTable));\r\n");
            } else {
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer
                                .append("  if (SetJump (&mJumpContext) == 0) {\r\n");
                        fileBuffer.append("    ExitDriver (");
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

            fileBuffer.append("VOID\n");
            fileBuffer.append("EFIAPI\n");
            fileBuffer.append("ExitDriver (\r\n");
            fileBuffer.append("  IN EFI_STATUS  Status\n");
            fileBuffer.append("  )\r\n\r\n");
            fileBuffer.append("{\r\n");
            if (entryPointCount <= 1) {
                fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                fileBuffer
                        .append("    ProcessLibraryDestructorList (gImageHandle, gST);\r\n");
                fileBuffer.append("  }\r\n");
                fileBuffer
                        .append("  gBS->Exit (gImageHandle, Status, 0, NULL);\r\n");
            } else {
                fileBuffer
                        .append("  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {\r\n");
                fileBuffer.append("    mDriverEntryPointStatus = Status;\r\n");
                fileBuffer.append("  }\r\n");
                fileBuffer.append("  LongJump (&mJumpContext, (UINTN)-1);\r\n");
                fileBuffer.append("  ASSERT (FALSE);\r\n");
            }
            fileBuffer.append("}\r\n\r\n");

            //
            // Add ModuleUnloadImage for DxeDriver and UefiDriver module type.
            //
            entryPointList = SurfaceAreaQuery.getModuleUnloadImageArray();
            entryPointCount = 0;
            if (entryPointList != null) {
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append("EFI_STATUS\r\n");
                        fileBuffer.append("EFIAPI\r\n");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append(" (\r\n");
                        fileBuffer
                                .append("  EFI_HANDLE        ImageHandle\r\n");
                        fileBuffer.append("  );\r\n");
                        entryPointCount++;
                    } else {
                        break;
                    }
                }
            }

            fileBuffer
                    .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverUnloadImageCount = ");
            fileBuffer.append(Integer.toString(entryPointCount));
            fileBuffer.append(";\r\n\r\n");

            if (entryPointList != null) {
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append("EFI_STATUS\r\n");
                        fileBuffer.append("EFIAPI\r\n");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append(" (\r\n");
                        fileBuffer
                                .append("  EFI_HANDLE        ImageHandle\r\n");
                        fileBuffer.append("  );\r\n");
                    } else {
                        break;
                    }
                }
            }

            fileBuffer.append("EFI_STATUS\n");
            fileBuffer.append("EFIAPI\n");
            fileBuffer.append("ProcessModuleUnloadList (\r\n");
            fileBuffer.append("  EFI_HANDLE  ImageHandle\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");

            if (entryPointCount == 0) {
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
            } else if (entryPointCount == 1) {
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append("(ImageHandle);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n\r\n");
                fileBuffer.append("  Status = EFI_SUCCESS;\r\n\r\n");
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                        fileBuffer.append("    ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  } else {\r\n");
                        fileBuffer.append("    Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  }\r\n");
                    } else {
                        break;
                    }
                }
                fileBuffer.append("  return Status;\r\n");
            }
            fileBuffer.append("}\r\n\r\n");
            break;
        }
    }

    /**
      PpiGuidToAutogenc 
    
      This function gets GUIDs from SPD file accrodeing to <PPIs> information and 
      write those GUIDs to AutoGen.c.
    
      @param   fileBuffer         String Buffer for Autogen.c file.
      @throws  BuildException     Guid must set value!
    **/
    void PpiGuidToAutogenC(StringBuffer fileBuffer) throws BuildException {
        String[] cNameGuid = null;
        boolean isEqual = false;

        PPIsDocument.PPIs.Ppi[] ppiList = SurfaceAreaQuery.getPpiArray(null);
        if (ppiList != null) {
            for (int i = 0; i < ppiList.length; i++) {
                isEqual = false;
                String ppiName = getStringValue((XmlObject)ppiList[i]);
                for (int j = 0; j < this.mPpiList.size(); j++) {
                    if (this.mPpiList.get(j).equalsIgnoreCase(ppiName)) {
                        isEqual = true;
                    }
                }
                if (!isEqual) {
                    this.mPpiList.add(ppiName);
                }
            }
        }

        PPIsDocument.PPIs.PpiNotify[] ppiNotifyList = SurfaceAreaQuery
                .getPpiNotifyArray(null);
        if (ppiNotifyList != null) {
            for (int i = 0; i < ppiNotifyList.length; i++) {
                isEqual = false;
                String ppiNotifyName = getStringValue((XmlObject)ppiNotifyList[i]);
                for (int j = 0; j < this.mPpiList.size(); j++) {
                    if (this.mPpiList.get(j).equalsIgnoreCase(ppiNotifyName)) {
                        isEqual = true;
                    }
                }
                if (!isEqual) {
                    this.mPpiList.add(ppiNotifyName);
                }
            }
        }

        for (int i = 0; i < this.mPpiList.size(); i++) {
            if (this.mPpiList.get(i) != null) {
                cNameGuid = GlobalData.getPpiInfoGuid(this.mPpiList.get(i));
                if (cNameGuid != null) {
                    fileBuffer
                            .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID ");
                    fileBuffer.append(cNameGuid[0]);
                    fileBuffer.append(" =     { ");
                    fileBuffer.append(cNameGuid[1]);
                    fileBuffer.append(" } ;");
                }
            } else {
                throw new BuildException("Guid must set value!");
            }
        }
    }

    /**
      ProtocolGuidToAutogenc 
    
      This function gets GUIDs from SPD file accrodeing to <Protocol> 
      information and write those GUIDs to AutoGen.c.
    
      @param   fileBuffer         String Buffer for Autogen.c file.
      @throws  BuildException     Protocol name must set.
    **/
    void ProtocolGuidToAutogenC(StringBuffer fileBuffer) throws BuildException {
        String[] cNameGuid = null;
        boolean isEqual = false;

        ProtocolsDocument.Protocols.Protocol[] protocolList = SurfaceAreaQuery
                .getProtocolArray(null);
        if (protocolList != null) {
            for (int i = 0; i < protocolList.length; i++) {
                isEqual = false;
                String protocolName = getStringValue((XmlObject)protocolList[i]);
                for (int j = 0; j < this.mProtocolList.size(); j++) {
                    if (this.mProtocolList.get(j).equalsIgnoreCase(protocolName)) {
                        isEqual = true;
                    }
                }
                if (!isEqual) {
                    this.mProtocolList.add(protocolName);

                }
            }
        }

        ProtocolsDocument.Protocols.ProtocolNotify[] protocolNotifyList = SurfaceAreaQuery
                .getProtocolNotifyArray(null);
        if (protocolNotifyList != null) {
            for (int i = 0; i < protocolNotifyList.length; i++) {
                isEqual = false;
                String protocolNotifyName = getStringValue((XmlObject)protocolNotifyList[i]);
                for (int j = 0; j < this.mProtocolList.size(); j++) {
                    if (this.mProtocolList.get(j).equalsIgnoreCase(protocolNotifyName)) {
                        isEqual = true;
                    }
                }
                if (!isEqual) {
                    this.mProtocolList.add(protocolNotifyName);

                }
            }
        }
        if (this.mProtocolList.size() > 0) {
            for (int i = 0; i < this.mProtocolList.size(); i++) {
                if (this.mProtocolList.get(i) != null) {
                    cNameGuid = GlobalData
                            .getProtocolInfoGuid(this.mProtocolList.get(i));
                    if (cNameGuid != null) {
                        fileBuffer
                                .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID ");
                        fileBuffer.append(cNameGuid[0]);
                        fileBuffer.append(" =     { ");
                        fileBuffer.append(cNameGuid[1]);
                        fileBuffer.append(" } ;");
                    }
                } else {
                    throw new BuildException("Protocol name must set!");
                }
            }
        }
    }

    /**
      GuidGuidToAutogenc
    
      This function gets GUIDs from SPD file accrodeing to <Guids> information
      and write those GUIDs to AutoGen.c.
    
      @param  fileBuffer       String Buffer for Autogen.c file.
    
    **/
    void GuidGuidToAutogenC(StringBuffer fileBuffer) {
        String[] cNameGuid = null;
        boolean isEqual = false;
        GuidsDocument.Guids.GuidEntry[] guidList = SurfaceAreaQuery
                .getGuidEntryArray(null);

        if (guidList != null) {
            for (int i = 0; i < guidList.length; i++) {
                for (int j = 0; j < this.mGuidList.size(); j++) {
                    isEqual = false;
                    if (this.mGuidList.get(j).getCName().equalsIgnoreCase(
                            guidList[i].getCName().toString())) {
                        isEqual = true;
                        break;
                    }
                }
                if (!isEqual) {
                    this.mGuidList.add(guidList[i]);

                }

            }
        }

        for (int i = 0; i < this.mGuidList.size(); i++) {
            if (this.mGuidList.get(i).getCName() != null) {
                cNameGuid = GlobalData.getGuidInfoGuid(this.mGuidList.get(i)
                        .getCName());
                if (cNameGuid != null) {
                    fileBuffer
                            .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID ");
                    fileBuffer.append(cNameGuid[0]);
                    fileBuffer.append(" =     { ");
                    fileBuffer.append(cNameGuid[1]);
                    fileBuffer.append("} ;");
                }
            }
        }
    }

    /**
      LibInstanceToAutogenC
      
      This function adds dependent library instance to autogen.c,which includeing 
      library's constructor, destructor, and library dependent ppi, protocol, guid,
      pcd information.
         
      @param  fileBuffer              String buffer for AutoGen.c
      @throws BuildException          
    **/
    void LibInstanceToAutogenC(StringBuffer fileBuffer) throws BuildException {
        int index;
        boolean isEqual = false;

        String moduleType = SurfaceAreaQuery.getModuleType();
        List<String> libConstructList = new ArrayList<String>();
        List<String> libDestructList = new ArrayList<String>();

        String libConstructName = null;
        String libDestructName = null;
        List<String> libraryList = SurfaceAreaQuery.getLibraryInstance(
                this.arch, CommonDefinition.AlwaysConsumed);

        try {
            if (libraryList != null) {
                //
                // Reorder library instance sequence.
                //
                AutogenLibOrder libOrder = new AutogenLibOrder(libraryList);
                List orderList = libOrder.orderLibInstance();
                
                if (orderList != null) {
                    //
                    // Process library instance one by one.
                    //
                    for (int i = 0; i < orderList.size(); i++) {
                        
                        //
                        // Get library instance basename.
                        //
                        String libInstanceName = orderList.get(i).toString();
                        
                        //
                        // Get override map
                        //
                        Map<String, XmlObject> libDoc = GlobalData
                                .getDoc(libInstanceName);
                        SurfaceAreaQuery.push(libDoc);
                        
                        //
                        // Get <PPis>, <Protocols>, <Guids> list of this library
                        // instance.
                        //
                        PPIsDocument.PPIs.Ppi[] ppiList = SurfaceAreaQuery
                                .getPpiArray(null);
                        PPIsDocument.PPIs.PpiNotify[] ppiNotifyList = SurfaceAreaQuery
                                .getPpiNotifyArray(null);
                        ProtocolsDocument.Protocols.Protocol[] protocolList = SurfaceAreaQuery
                                .getProtocolArray(null);
                        ProtocolsDocument.Protocols.ProtocolNotify[] protocolNotifyList = SurfaceAreaQuery
                                .getProtocolNotifyArray(null);
                        GuidsDocument.Guids.GuidEntry[] guidList = SurfaceAreaQuery
                                .getGuidEntryArray(null);

                        //
                        // Add those ppi, protocol, guid in global ppi, protocol, guid
                        // list.
                        //
                        if (ppiList != null) {
                            for (index = 0; index < ppiList.length; index++) {
                                isEqual = false;
                                String name = getStringValue((XmlObject)ppiList[index]);
                                for (int j = 0; j < this.mPpiList.size(); j++) {
                                    if (this.mPpiList.get(j).equalsIgnoreCase(name)) {
                                        isEqual = true;
                                    }
                                }
                                if (!isEqual) {
                                    this.mPpiList.add(name);
                                }
                            }
                        }
                        if (ppiNotifyList != null) {
                            for (index = 0; index < ppiNotifyList.length; index++) {
                                isEqual = false;
                                String name = getStringValue((XmlObject)ppiNotifyList[index]);
                                for (int j = 0; j < this.mPpiList.size(); j++) {
                                    if (this.mPpiList.get(j).equalsIgnoreCase(name)) {
                                        isEqual = true;
                                    }
                                }
                                if (!isEqual) {
                                    this.mPpiList.add(name);
                                }
                            }
                        }
                        if (protocolList != null) {
                            for (index = 0; index < protocolList.length; index++) {
                                isEqual = false;
                                String name = getStringValue((XmlObject)protocolList[index]);
                                for (int j = 0; j < this.mProtocolList.size(); j++) {
                                    if (this.mProtocolList.get(j).equalsIgnoreCase(name)) {
                                        isEqual = true;
                                    }
                                }
                                if (!isEqual) {
                                    this.mProtocolList.add(name);
                                }
                            }
                        }
                        if (protocolNotifyList != null) {
                            for (index = 0; index < protocolNotifyList.length; index++) {
                                isEqual = false;
                                String name = getStringValue((XmlObject)protocolNotifyList[index]);
                                for (int j = 0; j < this.mProtocolList.size(); j++) {
                                    if (this.mProtocolList.get(j).equalsIgnoreCase(name)) {
                                        isEqual = true;
                                    }
                                }
                                if (!isEqual) {
                                    this.mProtocolList.add(name);
                                }
                            }
                        }
                        if (guidList != null) {
                            for (index = 0; index < guidList.length; index++) {
                                isEqual = false;
                                for (int j = 0; j < this.mGuidList.size(); j++) {
                                    if (this.mGuidList.get(j).getCName()
                                            .equalsIgnoreCase(
                                                    guidList[index].getCName())) {
                                        isEqual = true;
                                    }
                                }
                                if (!isEqual) {
                                    this.mGuidList.add(guidList[index]);
                                }
                            }
                        }

                        //
                        // If not yet parse this library instance's constructor
                        // element,parse it.
                        //
                        if (!GlobalData.isHaveLibInstance(libInstanceName)) {
                            libConstructName = SurfaceAreaQuery
                                    .getLibConstructorName();
                            libDestructName = SurfaceAreaQuery
                                    .getLibDestructorName();

                            GlobalData.setLibInstanceInfo(libInstanceName,
                                    libConstructName, libDestructName);
                        } else {
                            libConstructName = GlobalData
                                    .getLibInstanceConstructor(libInstanceName);
                            libDestructName = GlobalData
                                    .getLibInstanceDestructor(libInstanceName);
                        }
                        SurfaceAreaQuery.pop();
                        //
                        // Add dependent library instance constructor function.
                        //
                        if (libConstructName != null) {
                            libConstructList.add(libConstructName);
                        }
                        //
                        // Add dependent library instance destructor fuction.
                        //
                        if (libDestructName != null) {
                            libDestructList.add(libDestructName);
                        }
                    }

                }

                //
                // Add library constructor to AutoGen.c
                //
                LibConstructorToAutogenC(libConstructList, moduleType,
                        fileBuffer/* autogenC */);
                //
                // Add library destructor to AutoGen.c
                //
                LibDestructorToAutogenC(libDestructList, moduleType,
                        fileBuffer/* autogenC */);
            }

        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

 
    /**
      LibConstructorToAutogenc
    
      This function writes library constructor list to AutoGen.c. The library 
      constructor's parameter and return value depend on module type.
    
      @param  libInstanceList        List of library construct name.
      @param  moduleType             Module type.
      @param  fileBuffer             String buffer for AutoGen.c
      @throws Exception              
    **/
    void LibConstructorToAutogenC(List<String> libInstanceList,
            String moduleType, StringBuffer fileBuffer) throws Exception {
        boolean isFirst = true;

        //
        // The library constructor's parameter and return value depend on 
        // module type.
        //
        for (int i = 0; i < libInstanceList.size(); i++) {
            switch (CommonDefinition.getModuleType(moduleType)) {
            case CommonDefinition.ModuleTypeBase:
                fileBuffer.append("RETURN_STATUS\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  VOID\r\n");
                fileBuffer.append("  );\r\n");
                break;

            case CommonDefinition.ModuleTypePeiCore:
            case CommonDefinition.ModuleTypePeim:
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer
                        .append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
                fileBuffer
                        .append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
                fileBuffer.append("  );\r\n");
                break;

            case CommonDefinition.ModuleTypeDxeCore:
            case CommonDefinition.ModuleTypeDxeDriver:
            case CommonDefinition.ModuleTypeDxeRuntimeDriver:
            case CommonDefinition.ModuleTypeDxeSmmDriver:
            case CommonDefinition.ModuleTypeDxeSalDriver:
            case CommonDefinition.ModuleTypeUefiDriver:
            case CommonDefinition.ModuleTypeUefiApplication:
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  );\r\n");
                break;
            }
        }

        //
        // Add ProcessLibraryConstructorList in AutoGen.c
        //
        fileBuffer.append("VOID\r\n");
        fileBuffer.append("ProcessLibraryConstructorList (\r\n");
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeBase:
            fileBuffer.append("  VOID\r\n");
            break;

        case CommonDefinition.ModuleTypePeiCore:
        case CommonDefinition.ModuleTypePeim:
            fileBuffer.append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
            fileBuffer
                    .append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
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

        for (int i = 0; i < libInstanceList.size(); i++) {
            if (isFirst) {
                fileBuffer.append("  EFI_STATUS  Status;\r\n");
                fileBuffer.append("\r\n");
                isFirst = false;
            }
            switch (CommonDefinition.getModuleType(moduleType)) {
            case CommonDefinition.ModuleTypeBase:
                fileBuffer.append("  Status = ");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append("();\r\n");
                fileBuffer.append("  VOID\r\n");
                break;
            case CommonDefinition.ModuleTypePeiCore:
            case CommonDefinition.ModuleTypePeim:
                fileBuffer.append("  Status = ");
                fileBuffer.append(libInstanceList.get(i));
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
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (ImageHandle, SystemTable);\r\n");
                break;
            }
            fileBuffer.append("  ASSERT_EFI_ERROR (Status);\r\n");
        }
        fileBuffer.append("}\r\n");
    }

    /**
      LibDestructorToAutogenc
    
      This function writes library destructor list to AutoGen.c. The library 
      destructor's parameter and return value depend on module type.
    
      @param  libInstanceList        List of library destructor name.
      @param  moduleType             Module type.
      @param  fileBuffer             String buffer for AutoGen.c
      @throws Exception              
    **/
    void LibDestructorToAutogenC(List<String> libInstanceList,
            String moduleType, StringBuffer fileBuffer) throws Exception {
        boolean isFirst = true;
        for (int i = 0; i < libInstanceList.size(); i++) {
            switch (CommonDefinition.getModuleType(moduleType)) {
            case CommonDefinition.ModuleTypeBase:
                fileBuffer.append("RETURN_STATUS\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  VOID\r\n");
                fileBuffer.append("  );\r\n");
                break;
            case CommonDefinition.ModuleTypePeiCore:
            case CommonDefinition.ModuleTypePeim:
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer
                        .append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
                fileBuffer
                        .append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
                fileBuffer.append("  );\r\n");
                break;
            case CommonDefinition.ModuleTypeDxeCore:
            case CommonDefinition.ModuleTypeDxeDriver:
            case CommonDefinition.ModuleTypeDxeRuntimeDriver:
            case CommonDefinition.ModuleTypeDxeSmmDriver:
            case CommonDefinition.ModuleTypeDxeSalDriver:
            case CommonDefinition.ModuleTypeUefiDriver:
            case CommonDefinition.ModuleTypeUefiApplication:
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  );\r\n");
                break;
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
            fileBuffer.append("ProcessLibraryDestructorList (\r\n");
            fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
            fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");

            for (int i = 0; i < libInstanceList.size(); i++) {
                if (isFirst) {
                    fileBuffer.append("  EFI_STATUS  Status;\r\n");
                    fileBuffer.append("\r\n");
                    isFirst = false;
                }
                fileBuffer.append("  Status = ");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append("(ImageHandle, SystemTable);\r\n");
                fileBuffer.append("  ASSERT_EFI_ERROR (Status);\r\n");
            }
            fileBuffer.append("}\r\n");
            break;
        }
    }

    /**
      ExternsDriverBindingToAutoGenC
      
      This function is to write DRIVER_BINDING, COMPONENT_NAME, 
      DRIVER_CONFIGURATION, DRIVER_DIAGNOSTIC in AutoGen.c.
      
      @param  fileBuffer             String buffer for AutoGen.c
     */
    void ExternsDriverBindingToAutoGenC(StringBuffer fileBuffer)
            throws BuildException {

        //
        // Check what <extern> contains. And the number of following elements
        // under <extern> should be same. 1. DRIVER_BINDING 2. COMPONENT_NAME
        // 3.DRIVER_CONFIGURATION 4. DRIVER_DIAGNOSTIC
        //
        
        String[] drvBindList = SurfaceAreaQuery.getDriverBindingArray();
        
        //
        // If component name protocol,component configuration protocol,
        // component diagnostic protocol is not null or empty, check 
        // if every one have the same number of the driver binding protocol.
        //
        if (drvBindList == null || drvBindList.length == 0) {
            return;
        }

        String[] compNamList = SurfaceAreaQuery.getComponentNameArray();
        String[] compConfList = SurfaceAreaQuery.getDriverConfigArray();
        String[] compDiagList = SurfaceAreaQuery.getDriverDiagArray();

        int BitMask = 0;

        //
        // Write driver binding protocol extern to autogen.c
        //
        for (int i = 0; i < drvBindList.length; i++) {
            fileBuffer.append("extern EFI_DRIVER_BINDING_PROTOCOL ");
            fileBuffer.append(drvBindList[i]);
            fileBuffer.append(";\r\n");
        }

        //
        // Write component name protocol extern to autogen.c
        //
        if (compNamList != null && compNamList.length != 0) {
            if (drvBindList.length != compNamList.length) {
                throw new BuildException(
                        "Different number of Driver Binding and Component Name protocols!");
            }

            BitMask |= 0x01;
            for (int i = 0; i < compNamList.length; i++) {
                fileBuffer.append("extern EFI_COMPONENT_NAME_PROTOCOL ");
                fileBuffer.append(compNamList[i]);
                fileBuffer.append(";\r\n");
            }
        }

        //
        // Write driver configration protocol extern to autogen.c
        //
        if (compConfList != null && compConfList.length != 0) {
            if (drvBindList.length != compConfList.length) {
                throw new BuildException(
                        "Different number of Driver Binding and Driver Configuration protocols!");
            }

            BitMask |= 0x02;
            for (int i = 0; i < compConfList.length; i++) {
                fileBuffer.append("extern EFI_DRIVER_CONFIGURATION_PROTOCOL ");
                fileBuffer.append(compConfList[i]);
                fileBuffer.append(";\r\n");
            }
        }

        //
        // Write driver dignastic protocol extern to autogen.c
        //
        if (compDiagList != null && compDiagList.length != 0) {
            if (drvBindList.length != compDiagList.length) {
                throw new BuildException(
                        "Different number of Driver Binding and Driver Configuration protocols!");
            }

            BitMask |= 0x04;
            for (int i = 0; i < compDiagList.length; i++) {
                fileBuffer.append("extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL ");
                fileBuffer.append(compDiagList[i]);
                fileBuffer.append(";\r\n");
            }
        }

        //
        // Write driver module protocol bitmask.
        //
        fileBuffer
                .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverModelProtocolBitmask = ");
        fileBuffer.append(Integer.toString(BitMask));
        fileBuffer.append(";\r\n");

        //
        // Write driver module protocol list entry
        //
        fileBuffer
                .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINTN  _gDriverModelProtocolListEntries = ");

        fileBuffer.append(Integer.toString(drvBindList.length));
        fileBuffer.append(";\r\n");

        //
        // Write drive module protocol list to autogen.c
        //
        fileBuffer
                .append("GLOBAL_REMOVE_IF_UNREFERENCED const EFI_DRIVER_MODEL_PROTOCOL_LIST  _gDriverModelProtocolList[] = {");
        for (int i = 0; i < drvBindList.length; i++) {
            if (i != 0) {
                fileBuffer.append(",");
            }
            fileBuffer.append("\r\n {\r\n");
            fileBuffer.append("  &");
            fileBuffer.append(drvBindList[i]);
            fileBuffer.append(", \r\n");

            if (compNamList != null) {
                fileBuffer.append("  &");
                fileBuffer.append(compNamList[i]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }

            if (compConfList != null) {
                fileBuffer.append("  &");
                fileBuffer.append(compConfList[i]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }

            if (compDiagList != null) {
                fileBuffer.append("  &");
                fileBuffer.append(compDiagList[i]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }
            fileBuffer.append("  }");
        }
        fileBuffer.append("\r\n};\r\n");
    }

    /**
      ExternCallBackToAutoGenC
    
      This function adds <SetVirtualAddressMapCallBack> and <ExitBootServicesCallBack>
      infomation to AutoGen.c
    
      @param  fileBuffer           String buffer for AutoGen.c
      @throws BuildException
    **/
    void ExternCallBackToAutoGenC(StringBuffer fileBuffer)
            throws BuildException {
        String[] setVirtualList = SurfaceAreaQuery
                .getSetVirtualAddressMapCallBackArray();
        String[] exitBootList = SurfaceAreaQuery
                .getExitBootServicesCallBackArray();
        String moduleType = SurfaceAreaQuery.getModuleType();
        boolean UefiOrDxeModule = false;
        int Count = 0;
        int i;

        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            //
            // Entry point lib for these module types needs to know the count
            // of entryPoint.
            //
            UefiOrDxeModule = true;
            fileBuffer
                    .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED  const UINTN _gDriverSetVirtualAddressMapEventCount = ");

            //
            // If the list is not valid or has no entries set count to zero else
            // set count to the number of valid entries
            //
            Count = 0;
            if (setVirtualList != null) {
                for (i = 0; i < setVirtualList.length; i++) {
                    if (setVirtualList[i].equalsIgnoreCase("")) {
                        break;
                    }
                }
                Count = i;
            }

            fileBuffer.append(Integer.toString(Count));
            fileBuffer.append(";\r\n\r\n");
            break;
        default:
            break;
        }

        if (setVirtualList == null) {
            if (UefiOrDxeModule) {
                //
                // No data so make a NULL list
                //
                fileBuffer
                        .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverSetVirtualAddressMapEvent[] = {\r\n");
                fileBuffer.append("  NULL\r\n");
                fileBuffer.append("};\r\n\r\n");
            }
        } else {
            //
            // Write SetVirtualAddressMap function definition.
            //
            for (i = 0; i < setVirtualList.length; i++) {
                if (setVirtualList[i].equalsIgnoreCase("")) {
                    break;
                }
                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\n");
                fileBuffer.append(setVirtualList[i]);
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  IN EFI_EVENT  Event,\r\n");
                fileBuffer.append("  IN VOID       *Context\r\n");
                fileBuffer.append("  );\r\n\r\n");
            }

            //
            // Write SetVirtualAddressMap entry point array.
            //
            fileBuffer
                    .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverSetVirtualAddressMapEvent[] = {");
            for (i = 0; i < setVirtualList.length; i++) {
                if (setVirtualList[i].equalsIgnoreCase("")) {
                    break;
                }

                if (i == 0) {
                    fileBuffer.append("\r\n  ");
                } else {
                    fileBuffer.append(",\r\n  ");
                }

                fileBuffer.append(setVirtualList[i]);
            }
            if (Count == 0) {
                fileBuffer.append("\r\n  NULL");
            }
            fileBuffer.append("\r\n};\r\n\r\n");
        }

        if (UefiOrDxeModule) {
            //
            // Entry point lib for these module types needs to know the count.
            //
            fileBuffer
                    .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED  const UINTN _gDriverExitBootServicesEventCount = ");

            //
            // If the list is not valid or has no entries set count to zero else
            // set count to the number of valid entries.
            //
            Count = 0;
            if (exitBootList != null) {
                if (setVirtualList != null) {
                    for (i = 0; i < exitBootList.length; i++) {
                        if (exitBootList[i].equalsIgnoreCase("")) {
                            break;
                        }
                    }
                    Count = i;
                }
            }
            fileBuffer.append(Integer.toString(Count));
            fileBuffer.append(";\r\n\r\n");
        }

        if (exitBootList == null) {
            if (UefiOrDxeModule) {
                //
                // No data so make a NULL list.
                //
                fileBuffer
                        .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverExitBootServicesEvent[] = {\r\n");
                fileBuffer.append("  NULL\r\n");
                fileBuffer.append("};\r\n\r\n");
            }
        } else {
            //
            // Write DriverExitBootServices function definition.
            //
            for (i = 0; i < exitBootList.length; i++) {
                if (exitBootList[i].equalsIgnoreCase("")) {
                    break;
                }

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\n");
                fileBuffer.append(exitBootList[i]);
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  IN EFI_EVENT  Event,\r\n");
                fileBuffer.append("  IN VOID       *Context\r\n");
                fileBuffer.append("  );\r\n\r\n");
            }

            //
            // Write DriverExitBootServices entry point array.
            //
            fileBuffer
                    .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverExitBootServicesEvent[] = {");
            for (i = 0; i < exitBootList.length; i++) {
                if (exitBootList[i].equalsIgnoreCase("")) {
                    break;
                }

                if (i == 0) {
                    fileBuffer.append("\r\n  ");
                } else {
                    fileBuffer.append(",\r\n  ");
                }
                fileBuffer.append(exitBootList[i]);
            }
            if (Count == 0) {
                fileBuffer.append("\r\n  NULL");
            }
            fileBuffer.append("\r\n};\r\n\r\n");
        }

    }

    private String getStringValue(XmlObject xmlDoc) {
        return xmlDoc.getDomNode().getFirstChild().getNodeValue();
    }
}
