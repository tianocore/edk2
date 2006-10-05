/** @file
 
 The file is used to override FileFilter to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common;

import java.io.File;

import javax.swing.filechooser.FileFilter;

/**
 The class is used to override FileFilter to provides customized interfaces 
 
 **/
public class IFileFilter extends FileFilter {

    private String strExt;

    /**
     Reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     This is the default constructor
     
     @param ext
     
     **/
    public IFileFilter(String ext) {
        this.strExt = ext;
    }

    /* (non-Javadoc)
     * @see javax.swing.filechooser.FileFilter#accept(java.io.File)
     * 
     * Override method "accept"
     * 
     */
    public boolean accept(File file) {
        if (file.isDirectory()) {
            return true;
        }
        String strFileName = file.getName();
        int intIndex = strFileName.lastIndexOf('.');
        if (intIndex > 0 && intIndex < strFileName.length() - 1) {
            String strExtension = strFileName.substring(intIndex + 1).toLowerCase();
            if (strExtension.equals(strExt))
                return true;
        }
        return false;
    }

    /* (non-Javadoc)
     * @see javax.swing.filechooser.FileFilter#getDescription()
     * 
     * Override method "getDescription" to config description via different file type 
     * 
     */
    public String getDescription() {
        if (strExt.equals(DataType.MODULE_SURFACE_AREA_EXT))
            return DataType.MODULE_SURFACE_AREA_EXT_DESCRIPTION;
        if (strExt.equals(DataType.PACKAGE_SURFACE_AREA_EXT))
            return DataType.PACKAGE_SURFACE_AREA_EXT_DESCRIPTION;
        if (strExt.equals(DataType.PLATFORM_SURFACE_AREA_EXT))
            return DataType.PLATFORM_SURFACE_AREA_EXT_DESCRIPTION;
        if (strExt.equals(DataType.TEXT_FILE_EXT))
            return DataType.TEXT_FILE_EXT_DESCRIPTION;
        if (strExt.equals(DataType.FAR_SURFACE_AREA_EXT))
            return DataType.FAR_SURFACE_AREA_EXT_DESCRIPTION;
        return "";
    }

}
