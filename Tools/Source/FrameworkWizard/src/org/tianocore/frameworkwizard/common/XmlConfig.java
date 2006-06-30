/** @file
 
 The file is used to config XML file format
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common;

import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlOptions;

public class XmlConfig {
    public static XmlCursor setupXmlCursor(XmlCursor cursor) {
        String uri = "http://www.TianoCore.org/2006/Edk2.0";
        cursor.push();
        cursor.toNextToken();
        cursor.insertNamespace("", uri);
        cursor.insertNamespace("xsi", "http://www.w3.org/2001/XMLSchema-instance");
        cursor.pop();
        return cursor;
    }
    
    public static XmlOptions setupXmlOptions() {
        XmlOptions options = new XmlOptions();
        options.setCharacterEncoding("UTF-8");
        options.setSavePrettyPrint();
        options.setSavePrettyPrintIndent(2);
        return options;
    }
    
}
