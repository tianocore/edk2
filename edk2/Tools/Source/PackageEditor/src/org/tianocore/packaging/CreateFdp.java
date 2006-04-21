/** @file
  Java class CreateFdp is used to create a distributable package containing 
  FDPManifest.xml file in its root directory.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import java.io.*;
import java.util.jar.*;

/**
 This class contains static method create to generate *.fdp format package.
 
 @since PackageEditor 1.0
**/
public class CreateFdp {

    /**
     recursively add contents under dir into output package.
     
     @param dir The directory with files that will be put into package
     @param jos Stream used to create output package
     @param wkDir The position of source directory
     @throws Exception Any exception occurred during this process
    **/
    public static void create(File dir, JarOutputStream jos, String wkDir) throws Exception {
        
        String[] list = dir.list();
        
        try {
            byte[] buffer = new byte[1024];
            int bytesRead;

            //
            // Loop through the file names provided.
            //
            for (int i = 0; i < list.length; i++) {
                
                File f = new File(dir, list[i]);
                if (f.getName().equals("..")) {
                    continue;
                }
                if (f.isDirectory()) {
                    //
                    // Call this method recursively for directory
                    //
                    CreateFdp.create(f, jos, wkDir);
                    continue;
                }

                try {
                    //
                    // Open the file
                    //
                    FileInputStream fis = new FileInputStream(f);

                    try {
                        //
                        // Create a Jar entry and add it, keep relative path only.
                        //
                        JarEntry entry = new JarEntry(f.getPath().substring(wkDir.length() + 1));
                        jos.putNextEntry(entry);

                        //
                        // Read the file and write it to the Jar.
                        //
                        while ((bytesRead = fis.read(buffer)) != -1) {
                            jos.write(buffer, 0, bytesRead);
                        }

                        System.out.println(entry.getName() + " added.");
                    } catch (Exception ex) {
                        System.out.println(ex);
                    } finally {
                        fis.close();
                    }
                } catch (IOException ex) {
                    System.out.println(ex);
                }
            }
        } finally {
            System.out.println(dir.getPath() + " processed.");
        }
    }
}
