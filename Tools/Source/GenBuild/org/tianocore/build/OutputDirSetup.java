package org.tianocore.build;

import java.io.File;
import java.util.Map;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.id.PlatformIdentification;

public class OutputDirSetup extends Task {
    ///
    /// Module surface area file.
    ///
    File msaFile;

    ///
    /// Module's Identification.
    ///
    private ModuleIdentification moduleId;
    
    ///
    /// Module's component type, such as SEC, LIBRARY, BS_DRIVER and so on.
    ///
    private String componentType;
    
    private boolean isSingleModuleBuild = false;
//    private ToolChainFactory toolChainFactory;
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public OutputDirSetup() {
    }
  
    public void execute() throws BuildException {
        //
        // Global Data initialization
        //
//        GlobalData.initInfo("Tools" + File.separatorChar + "Conf" + File.separatorChar + "FrameworkDatabase.db",
//                            getProject().getProperty("WORKSPACE_DIR"));
      
        //
        // Parse MSA and get the basic information
        // Including BaseName, GUID, Version, ComponentType and SupportedArchs
        //
        Map<String, XmlObject> doc = GlobalData.getNativeMsa(msaFile);
        
        SurfaceAreaQuery.setDoc(doc);
      
        //
        // String[]: {BaseName, ModuleType, ComponentType, Guid, Version}
        //
        moduleId = SurfaceAreaQuery.getMsaHeader();
        // REMOVE!!! TBD
        componentType = "APPLICATION";
      
        //
        // Judge whether it is single module build or not
        //
        if (isSingleModuleBuild) {
            //
            // Single Module build
            //
            prepareSingleModuleBuild();
        }
        else {
            //
            // Platform build
            //
            String filename = getProject().getProperty("PLATFORM_FILE");
            PlatformIdentification platformId = GlobalData.getPlatform(filename);
            getProject().setProperty("PLATFORM_DIR", platformId.getFpdFile().getParent().replaceAll("(\\\\)", "/"));
            getProject().setProperty("PLATFORM_RELATIVE_DIR", platformId.getPlatformRelativeDir().replaceAll("(\\\\)", "/"));
          
            String packageName = getProject().getProperty("PACKAGE");
            String packageGuid = getProject().getProperty("PACKAGE_GUID");
            String packageVersion = getProject().getProperty("PACKAGE_VERSION");
            PackageIdentification packageId = new PackageIdentification(packageName, packageGuid, packageVersion);
            moduleId.setPackage(packageId);
        }
        
        //
        // Tools Definition file parse
        //
        parseToolsDefinitionFile();
        
        //
        // For Every TOOLCHAIN, TARGET, ARCH
        //
//        String[] targetList = GlobalData.getTargets();
//        for (int i = 0; i < targetList.length; i ++){
//            //
//            // Prepare for target related common properties
//            // TARGET
//            //
//            getProject().setProperty("TARGET", targetList[i]);
//            String[] toolchainList = GlobalData.getToolChains();
//            for(int j = 0; j < toolchainList.length; j ++){
//                //
//                // Prepare for toolchain related common properties
//                // TOOLCHAIN
//                //
//                getProject().setProperty("TOOLCHAIN", toolchainList[j]);
//                //
//                // If single module : intersection MSA supported ARCHs and tools def!!
//                // else, get arch from pass down
//                //
//                String[] archList = GlobalData.getArchs();
//                for (int k = 0; k < archList.length; k++) {
//                    
//                    FpdModuleIdentification fpdModuleId = new FpdModuleIdentification(moduleId, archList[k]);
//                    
//                    SurfaceAreaQuery.setDoc(GlobalData.getDoc(fpdModuleId));
//                    
//                    //
//                    // Prepare for all other common properties
//                    // PACKAGE, PACKAGE_GUID, PACKAGE_VERSION, PACKAGE_DIR, PACKAGE_RELATIVE_DIR
//                    // MODULE or BASE_NAME, GUID or FILE_GUID, VERSION, COMPONENT_TYPE
//                    // MODULE_DIR, MODULE_RELATIVE_DIR
//                    // SUBSYSTEM, ENTRYPOINT, EBC_TOOL_LIB_PATH
//                    // LIBS, OBJECTS, SDB_FILES
//                    //
//                    getProject().setProperty("ARCH", archList[k]);
//                    setModuleCommonProperties();
//        
//                    //
//                    // String[0] is build mode. String[1] is user-defined output dir. 
//                    //
//                    String buildMode = SurfaceAreaQuery.getFpdIntermediateDirectories();
//                    String userDefinedOutputDir = SurfaceAreaQuery.getFpdOutputDirectory();
//                    
//                    //
//                    // OutputManage prepare for 
//                    // BIN_DIR, DEST_DIR_DEBUG, DEST_DIR_OUTPUT, BUILD_DIR, FV_DIR
//                    //
//                    OutputManager.getInstance().update(getProject(), userDefinedOutputDir, buildMode);
//                    
//                }
//            }
//        }
        
    }
    
    private void prepareSingleModuleBuild(){
        //
        // Find out the package which the module belongs to
        // TBD: Enhance it!!!!
        //
        PackageIdentification packageId = GlobalData.getPackageForModule(moduleId);
        
        moduleId.setPackage(packageId);
        
        //
        // Read ACTIVE_PLATFORM's FPD file (Call FpdParserTask's method)
        //
        String filename = getProject().getProperty("PLATFORM_FILE");

        PlatformIdentification platformId = GlobalData.getPlatform(filename);
        
        //
        // Read FPD file
        //
        FpdParserTask fpdParser = new FpdParserTask();
        fpdParser.parseFpdFile(platformId.getFpdFile());
        
        //
        // Prepare for Platform related common properties
        // PLATFORM, PLATFORM_DIR, PLATFORM_RELATIVE_DIR
        //
        getProject().setProperty("PLATFORM", platformId.getName());
        getProject().setProperty("PLATFORM_DIR", platformId.getFpdFile().getParent().replaceAll("(\\\\)", "/"));
        getProject().setProperty("PLATFORM_RELATIVE_DIR", platformId.getPlatformRelativeDir().replaceAll("(\\\\)", "/"));
    }
    


    /**

    **/
    private void setModuleCommonProperties() {
        //
        // Prepare for all other common properties
        // PACKAGE, PACKAGE_GUID, PACKAGE_VERSION, PACKAGE_DIR, PACKAGE_RELATIVE_DIR
        //
        PackageIdentification packageId = moduleId.getPackage();
        getProject().setProperty("PACKAGE", packageId.getName());
        getProject().setProperty("PACKAGE_GUID", packageId.getGuid());
        getProject().setProperty("PACKAGE_VERSION", packageId.getVersion());
        GlobalData.log.info("" + packageId);
        getProject().setProperty("PACKAGE_DIR", packageId.getPackageDir().replaceAll("(\\\\)", "/"));
        getProject().setProperty("PACKAGE_RELATIVE_DIR", packageId.getPackageRelativeDir().replaceAll("(\\\\)", "/"));
        
        //
        // MODULE or BASE_NAME, GUID or FILE_GUID, VERSION, COMPONENT_TYPE
        // MODULE_DIR, MODULE_RELATIVE_DIR
        //
        getProject().setProperty("MODULE", moduleId.getName());
        getProject().setProperty("BASE_NAME", moduleId.getName());
        getProject().setProperty("GUID", moduleId.getGuid());
        getProject().setProperty("FILE_GUID", moduleId.getGuid());
        getProject().setProperty("VERSION", moduleId.getVersion());
        getProject().setProperty("COMPONENT_TYPE", componentType);
        getProject().setProperty("MODULE_DIR", moduleId.getMsaFile().getParent().replaceAll("(\\\\)", "/"));
        getProject().setProperty("MODULE_RELATIVE_DIR", moduleId.getModuleRelativePath().replaceAll("(\\\\)", "/"));
    }
    

    /**
      Method is for ANT use to initialize MSA file.
      
      @param msaFilename MSA file name
    **/
    public void setMsaFile(String msaFilename) {
        String moduleDir = getProject().getProperty("MODULE_DIR");
        if (moduleDir == null) {
            moduleDir = getProject().getBaseDir().getPath();
        }
        msaFile = new File(moduleDir + File.separatorChar + msaFilename);
    }

    /**
      Compile flags setup. 
      
      <p> Take command <code>CC</code> and arch <code>IA32</code> for example, 
      Those flags are from <code>ToolChainFactory</code>: </p>
      <ul>
      <li> IA32_CC </li>
      <li> IA32_CC_STD_FLAGS </li>
      <li> IA32_CC_GLOBAL_FLAGS </li>
      <li> IA32_CC_GLOBAL_ADD_FLAGS </li>
      <li> IA32_CC_GLOBAL_SUB_FLAGS </li>
      </ul>
      Those flags can user-define: 
      <ul>
      <li> IA32_CC_PROJ_FLAGS </li>
      <li> IA32_CC_PROJ_ADD_FLAGS </li>
      <li> IA32_CC_PROJ_SUB_FLAGS </li>
      <li> CC_PROJ_FLAGS </li>
      <li> CC_PROJ_ADD_FLAGS </li>
      <li> CC_PROJ_SUB_FLAGS </li>
      <li> CC_FLAGS </li>
      <li> IA32_CC_FLAGS </li>
      </ul>
      
      <p> The final flags is composed of STD, GLOBAL and PROJ. If CC_FLAGS or
      IA32_CC_FLAGS is specified, STD, GLOBAL and PROJ will not affect. </p>
      
      Note that the <code>ToolChainFactory</code> executes only once 
      during whole build process. 
    **/
    private void parseToolsDefinitionFile() {
        //
        // If ToolChain has been set up before, do nothing.
        // CONF dir + tools definition file name
        //
        String confDir = GlobalData.getWorkspacePath() + File.separatorChar + "Tools" + File.separatorChar + "Conf";
        String toolsDefFilename = "tools_def.txt";
        if (getProject().getProperty("env.TOOLS_DEF") != null) {
            toolsDefFilename = getProject().getProperty("env.TOOLS_DEF");
        }
//        toolChainFactory = new ToolChainFactory(confDir, toolsDefFilename);
//        toolChainFactory.setupToolChain();
    }
}
