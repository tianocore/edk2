/** @file
  OutputManager class.
  
  OutputManager class set output directories for every module by BUILD_MODE.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build.global;

import org.apache.tools.ant.Project;
import java.io.File;

/**
  OutputManager class is used to setup output directories (BIN_DIR, DEST_DIR_OUTPUT, 
  DEST_DIR_DEBUG) according to BUILD_MODE. 
  
  @since GenBuild 1.0
**/
public class OutputManager {
    
    ///
    /// Single Module build
    ///
    public static final String MODULE_BUILD = "MODULE";

    ///
    /// Package build
    ///
    public static final String PACKAGE_BUILD = "PACKAGE";

    ///
    /// Platform build
    ///
    public static final String PLATFORM_BUILD = "PLATFORM";
    
    public static String buildMode = MODULE_BUILD;
    
    ///
    /// For Package build, PLATFORM represent PACKAGE
    ///
    public static String PLATFORM;
    
    ///
    /// For Platform build, PLATFORM_DIR represent PACKAGE_DIR
    ///
    public static String PLATFORM_DIR;
    
    ///
    /// means intermediate files will put under Module's dir
    ///
    public static final String MODULE = "MODULE";
    
    ///
    /// mean intermediate files will put under a unify dir
    ///
    public static final String UNIFIED = "UNIFIED";
    
    ///
    /// Flag to ensure the function <code>update</code> will be called only one in the whole build.
    ///
    private static boolean flag = true;
    
    /**
      If BUILD_MODE is PLATFORM or PACKAGE, record PLATFORM and PLARFORM_DIR.
      Reminder that for PACKAGE build, here set value PACKAGE to PLATFORM and
      PACKAGE_DIR to PLARFORM_DIR, and also update the ant properties. 
      
      <p>Note that this function will be called only once in the whole build.</p> 
      
      @param project current ANT build Project
    **/
    public synchronized static void update(Project project) {
        if (flag){
            flag = false;
            String str = project.getProperty("BUILD_MODE");
            if (str != null){
                if (str.equals(PLATFORM_BUILD)) {
                    buildMode = PLATFORM_BUILD;
                    PLATFORM = project.getProperty("PLATFORM");
                    PLATFORM_DIR = project.getProperty("PLATFORM_DIR");
                }
                else if (str.equals(PACKAGE_BUILD)) {
                    buildMode = PACKAGE_BUILD;
                    PLATFORM = project.getProperty("PACKAGE");
                    PLATFORM_DIR = project.getProperty("PACKAGE_DIR");
                    project.setProperty("PLATFORM", PLATFORM);
                    project.setProperty("PLATFORM_DIR", PLATFORM_DIR);
                }
            }
        }
    }
    
    /**
      Setup BIN_DIR, DEST_DIR_OUTPUT and DEST_DIR_OUTPUT, following are the rules:
      
      <pre>
        Those three variables are defined as following
        DEST_DIR_OUTPUT (intermediate files)
        DEST_DIR_DEBUG (intermediate debug files)
        BIN_DIR (final files)
        
        Output Dir (MODULE or UNIFIED):
        For <b>Module</b> build: 
        All intermediate files are at ${MODULE_DIR}/Build/${TARGET}/${ARCH}/DEBUG|OUTPUT
        All final files are at ${MODULE_DIR}/Build/${TARGET}/${ARCH}
        
        For <b>Platform</b> build:
        If specified with MODULE
        Intermediate files->${MODULE_DIR}/Build/${PLATFORM}/${TARGET}/${ARCH}/DEBUG|OUTPUT
        Final files -> ${PLARFORM_DIR}/Build/${TARGET}/${ARCH}
        
        Else if specified with UNIFIED
        Intermediate files->${PLARFORM_DIR}/Build/${TARGET}/${ARCH}/${PACKAGE}/${SOURCE_RELATIVE_PATH}/DEBUG|OUTPUT
        Final files -> ${PLARFORM_DIR}/Build/${TARGET}/${ARCH}
        
        For <b>Package</b> build:
        If specified with MODULE
        Intermediate files->${MODULE_DIR}/Build/${PACKAGE}/${TARGET}/${ARCH}/DEBUG|OUTPUT
        Final files -> ${PACKAGE_DIR}/Build/${TARGET}/${ARCH}
        
        Else if specified with UNIFIED
        Intermediate files->${PACKAGE_DIR}/Build/${TARGET}/${ARCH}/${PACKAGE}/${SOURCE_RELATIVE_PATH}/DEBUG|OUTPUT
        Final files -> ${PACKAGE_DIR}/Build/${TARGET}/${ARCH}
      </pre>
      
      @param project current ANT build Project
      @param userdir user-defined directory
      @param type the module build type (MODULE or UNIFIED)
    **/
    public synchronized static void update(Project project, String userdir, String type) {
        //
        // userdir TBD
        //
       if(  type == null || ! type.equals(MODULE)){
           type = UNIFIED;
       }
       if (buildMode.equals(MODULE_BUILD)){
           project.setProperty("DEST_DIR_OUTPUT", project.replaceProperties("${MODULE_DIR}"
                        + File.separatorChar + "Build" + File.separatorChar + "${TARGET}"
                        + File.separatorChar + "${ARCH}" + File.separatorChar + "OUTPUT"));
           project.setProperty("DEST_DIR_DEBUG", project.replaceProperties("${MODULE_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "DEBUG"));
           project.setProperty("BIN_DIR", project.replaceProperties("${MODULE_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}"));
       }
       else if (buildMode.equals(PLATFORM_BUILD)) {
           if (type.equals(MODULE)) {
               project.setProperty("DEST_DIR_OUTPUT", project.replaceProperties("${MODULE_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${PLATFORM}" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "OUTPUT"));
               project.setProperty("DEST_DIR_DEBUG", project.replaceProperties("${MODULE_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${PLATFORM}" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "DEBUG"));
               project.setProperty("BIN_DIR", project.replaceProperties("${PLATFORM_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}"));
           }
           else if (type.equals(UNIFIED)){
               project.setProperty("DEST_DIR_OUTPUT", project.replaceProperties("${PLATFORM_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "${PACKAGE}" + File.separatorChar + "${MODULE_RELATIVE_PATH}" + File.separatorChar + "OUTPUT"));
               project.setProperty("DEST_DIR_DEBUG", project.replaceProperties("${PLATFORM_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "${PACKAGE}" + File.separatorChar + "${MODULE_RELATIVE_PATH}" + File.separatorChar + "DEBUG"));
               project.setProperty("BIN_DIR", project.replaceProperties("${PLATFORM_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}"));
           }
       }
       else if (buildMode.equals(PACKAGE_BUILD)) {
           if (type.equals(MODULE)) {
               project.setProperty("DEST_DIR_OUTPUT", project.replaceProperties("${MODULE_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${PLATFORM}" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "OUTPUT"));
               project.setProperty("DEST_DIR_DEBUG", project.replaceProperties("${MODULE_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${PLATFORM}" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "DEBUG"));
               project.setProperty("BIN_DIR", project.replaceProperties("${PLATFORM_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}"));
           }
           else if (type.equals(UNIFIED)){
               project.setProperty("DEST_DIR_OUTPUT", project.replaceProperties("${PLATFORM_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "${PACKAGE}" + File.separatorChar + "${MODULE_RELATIVE_PATH}" + File.separatorChar + "OUTPUT"));
               project.setProperty("DEST_DIR_DEBUG", project.replaceProperties("${PLATFORM_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}" + File.separatorChar + "${PACKAGE}" + File.separatorChar + "${MODULE_RELATIVE_PATH}" + File.separatorChar + "DEBUG"));
               project.setProperty("BIN_DIR", project.replaceProperties("${PLATFORM_DIR}" + File.separatorChar + "Build" + File.separatorChar + "${TARGET}" + File.separatorChar + "${ARCH}"));
           }
       }
    }
}
