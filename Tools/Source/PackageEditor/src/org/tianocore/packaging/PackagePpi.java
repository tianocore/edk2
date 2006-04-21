/** @file
  Java class PackagePpi is GUI for create Ppi definition elements of spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

/**
 GUI derived from PackageProtocols class, override save() method
  
 @since PackageEditor 1.0
**/
public class PackagePpi extends PackageProtocols {

    private SpdFileContents sfc = null;

    public PackagePpi(SpdFileContents sfc) {
        super(sfc);
        // TODO Auto-generated constructor stub
        this.sfc = sfc;
    }

    /**
      add ppi definitions from GUI to SpdFileContents object passed in.
     **/
    protected void save() {
        try {
            sfc.genSpdPpiDeclarations(getJTextField().getText(), getJTextFieldC_Name().getText(),
                                      getJTextFieldGuid().getText(), null);
        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }
}
