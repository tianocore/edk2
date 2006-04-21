/** @file
 
 The file is used to override FrameworkPkg to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.packaging;

import java.io.IOException;

/**
 The class is used to override FrameworkPkg to provides customized interfaces
 
 @since CreateMdkPkg 1.0

 **/
public class MdkPkg extends FrameworkPkg {

    /**
     Main class, reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     This is the default constructor
     
     @param strJarFile The jar file need be installed
     @throws IOException
     
     **/
    public MdkPkg(String strJarFile) throws IOException {
        this.setPkg(strJarFile);
        this.setJarFile();
    }

    /* (non-Javadoc)
     * @see org.tianocore.packaging.FrameworkPkg#pre_install()
     * 
     * Override pre_install to do nothing
     * 
     */
    protected void pre_install() {
    }
}
