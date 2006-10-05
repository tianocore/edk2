/** @file
 
 The file is used to define Package Dependencies Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.Identifications.Externs;

import java.util.Vector;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.EnumerationData;

public class ExternsVector {

    private Vector<ExternsIdentification> vExterns = new Vector<ExternsIdentification>();

    public int findExterns(ExternsIdentification sfi) {
        for (int index = 0; index < vExterns.size(); index++) {
            if (vExterns.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public ExternsIdentification getExterns(int index) {
        if (index > -1) {
            return vExterns.elementAt(index);
        } else {
            return null;
        }
    }

    public void addExterns(ExternsIdentification arg0) {
        vExterns.addElement(arg0);
    }

    public void setExterns(ExternsIdentification arg0, int arg1) {
        vExterns.setElementAt(arg0, arg1);
    }

    public void removeExterns(ExternsIdentification arg0) {
        int index = findExterns(arg0);
        if (index > -1) {
            vExterns.removeElementAt(index);
        }
    }

    public void removeExterns(int index) {
        if (index > -1 && index < this.size()) {
            vExterns.removeElementAt(index);
        }
    }

    public Vector<ExternsIdentification> getvExterns() {
        return vExterns;
    }

    public void setvExterns(Vector<ExternsIdentification> Externs) {
        vExterns = Externs;
    }

    public int size() {
        return this.vExterns.size();
    }

    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();

        //
        // For Specification
        //
        if (this.getExterns(index).getType().equals(EnumerationData.EXTERNS_SPECIFICATION)) {
            v.addElement(getExterns(index).getType());
            v.addElement(getExterns(index).getType() + " Name");
            v.addElement(getExterns(index).getName0());
            return v;
        }

        //
        // For Image
        //
        if (this.getExterns(index).getType().equals(EnumerationData.EXTERNS_IMAGE)) {
            v.addElement(getExterns(index).getType());
            String name = "";
            String value = "";

            name = EnumerationData.EXTERNS_MODULE_ENTRY_POINT;
            value = this.getExterns(index).getName0();
            name = name + DataType.HTML_LINE_SEPARATOR + EnumerationData.EXTERNS_MODULE_UNLOAD_IMAGE;
            value = value + DataType.HTML_LINE_SEPARATOR + this.getExterns(index).getName1()
                    + DataType.HTML_LINE_SEPARATOR;

            v.addElement("<html>" + name + "<html>");
            v.addElement("<html>" + value + "<html>");
            return v;
        }

        //
        // For Library
        //
        if (this.getExterns(index).getType().equals(EnumerationData.EXTERNS_LIBRARY)) {
            v.addElement(getExterns(index).getType());
            String name = "";
            String value = "";

            name = EnumerationData.EXTERNS_CONSTRUCTOR;
            value = this.getExterns(index).getName0();
            name = name + DataType.HTML_LINE_SEPARATOR + EnumerationData.EXTERNS_DESTRUCTOR;
            value = value + DataType.HTML_LINE_SEPARATOR + this.getExterns(index).getName1()
                    + DataType.HTML_LINE_SEPARATOR;

            v.addElement("<html>" + name + "<html>");
            v.addElement("<html>" + value + "<html>");
            return v;
        }

        //
        // For Driver
        //
        if (this.getExterns(index).getType().equals(EnumerationData.EXTERNS_DRIVER)) {
            v.addElement(getExterns(index).getType());
            String name = "";
            String value = "";

            name = EnumerationData.EXTERNS_DRIVER_BINDING;
            value = this.getExterns(index).getName0();
            name = name + DataType.HTML_LINE_SEPARATOR + EnumerationData.EXTERNS_COMPONENT_NAME;
            value = value + DataType.HTML_LINE_SEPARATOR + this.getExterns(index).getName1();
            name = name + DataType.HTML_LINE_SEPARATOR + EnumerationData.EXTERNS_DRIVER_CONFIG;
            value = value + DataType.HTML_LINE_SEPARATOR + this.getExterns(index).getName2();
            name = name + DataType.HTML_LINE_SEPARATOR + EnumerationData.EXTERNS_DRIVER_DIAG;
            value = value + DataType.HTML_LINE_SEPARATOR + this.getExterns(index).getName3()
                    + DataType.HTML_LINE_SEPARATOR;

            v.addElement("<html>" + name + "<html>");
            v.addElement("<html>" + value + "<html>");
            return v;
        }

        //
        // For Call Back
        //
        if (this.getExterns(index).getType().equals(EnumerationData.EXTERNS_CALL_BACK)) {
            v.addElement(getExterns(index).getType());
            String name = "";
            String value = "";

            name = EnumerationData.EXTERNS_VIRTUAL_ADDRESS_MAP_CALL_BACK;
            value = this.getExterns(index).getName0();
            name = name + DataType.HTML_LINE_SEPARATOR + EnumerationData.EXTERNS_EXIT_BOOT_SERVICES_CALL_BACK;
            value = value + DataType.HTML_LINE_SEPARATOR + this.getExterns(index).getName1()
                    + DataType.HTML_LINE_SEPARATOR;

            v.addElement("<html>" + name + "<html>");
            v.addElement("<html>" + value + "<html>");
            return v;
        }

        //
        // Return a empty v
        //
        return v;
    }
}
