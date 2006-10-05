/** @file
 
 The file is used to caculate file MD5 value.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.far;

import java.io.File;
import java.io.FileInputStream;
import java.security.MessageDigest;

public class FarMd5 {

    static public String md5(File file) throws Exception {
        byte[] buffer = new byte[(int) file.length()];
        FileInputStream fInput = new FileInputStream(file);
        fInput.read(buffer);
        fInput.close();
        return md5(buffer);

    }

    static public String md5(byte[] buffer) throws Exception {
        MessageDigest md = MessageDigest.getInstance("MD5");
        byte[] md5 = md.digest(buffer);
        return new String(String.format("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", md5[0], md5[1], md5[2], md5[3], md5[4],
                                        md5[5], md5[6], md5[7], md5[8], md5[9], md5[10], md5[11], md5[12], md5[13],
                                        md5[14], md5[15]));

    }
}
