/** @file
 
 The file is used to install .jar file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.packaging;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

import javax.swing.JFrame;

/**
 The class is used to install .jar file
 
 @since CreateMdkPkg 1.0

 **/
public class FrameworkPkg {
    //
    // Define class members
    //
    static JFrame frame;

    private String pkg = null;

    private JarFile jf = null;

    /**
     Main clase, used to test
     
     @param args
     
     **/
    public static void main(String[] args) {
        FrameworkPkg fp = new FrameworkPkg("C:\\Documents and Settings\\hchen30\\Desktop\\com.jar");
        try {
            fp.install("C:\\MyWorkspace" + System.getProperty("file.separator"));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     This is the default constructor
     
     **/
    public FrameworkPkg() {

    }

    /**
     This is the override constructor
     
     @param package_name
     
     **/
    public FrameworkPkg(String package_name) {
        pkg = package_name;
    }

    /**
     Get package name
     
     @param package_name
     
     **/
    public void setPkg(String package_name) {
        pkg = package_name;
    }

    /**
     Set Jarfile
     
     @throws IOException
     
     **/
    public void setJarFile() throws IOException {
        jf = new JarFile(pkg);
    }

    /**
     Install the jar file to specific path
     
     @param dir The target path
     @return 0 - success
     @throws IOException
     @throws BasePkgNotInstalled
     @throws VerNotEqual
     @throws GuidNotEqual
     @throws SameAll
     
     **/
    public int install(final String dir) throws IOException, BasePkgNotInstalled, VerNotEqual, GuidNotEqual, SameAll {
        pre_install();
        extract(dir);
        post_install();
        return 0;
    }

    /**
     
     @return
     
     **/
    public int uninstall() {

        return 0;
    }

    /**
     Check before install
     
     @throws IOException
     @throws BasePkgNotInstalled
     @throws VerNotEqual
     @throws GuidNotEqual
     @throws SameAll
     
     **/
    protected void pre_install() throws IOException, BasePkgNotInstalled, VerNotEqual, GuidNotEqual, SameAll {
        jf = new JarFile(pkg);
        if (false) {
            throw new BasePkgNotInstalled();
        }
        if (false) {
            throw new VerNotEqual();
        }
        if (false) {
            throw new GuidNotEqual();
        }
        if (false) {
            throw new SameAll();
        }
    }

    /**
     End of install
     
     @throws IOException
     
     **/
    protected void post_install() throws IOException {
        jf.close();

    }

    /**
     Extract the jar file to specific dir
     
     @param dir The target path
     @throws IOException
     
     **/
    private synchronized void extract(String dir) throws IOException {

        int i = 0;
        try {
            for (Enumeration e = jf.entries(); e.hasMoreElements(); i++) {
                JarEntry je = (JarEntry) e.nextElement();
                if (je.getName().contains("META-INF"))
                    continue;
                if (je.isDirectory()) {
                    new File(dir + je.getName()).mkdirs();
                    continue;
                }

                if (je != null) {
                    //
                    // Get an input stream for the entry.
                    //
                    InputStream entryStream = jf.getInputStream(je);

                    try {
                        //
                        // Create the output file (clobbering the file if it exists).
                        //
                        FileOutputStream file = new FileOutputStream(dir + je.getName());

                        try {
                            //
                            // Allocate a buffer for reading the entry data.
                            //
                            byte[] buffer = new byte[1024];
                            int bytesRead;

                            //
                            // Read the entry data and write it to the output file.
                            //    
                            while ((bytesRead = entryStream.read(buffer)) != -1) {
                                file.write(buffer, 0, bytesRead);
                            }

                            System.out.println(je.getName() + " extracted.");
                        } finally {
                            file.close();
                        }
                    } finally {
                        entryStream.close();
                    }
                }
            }
        } finally {
            jf.close();
        }
    }
}

class BasePkgNotInstalled extends Exception {
    final static long serialVersionUID = 0;
}

class VerNotEqual extends Exception {
    final static long serialVersionUID = 0;
}

class GuidNotEqual extends Exception {
    final static long serialVersionUID = 0;
}

class SameAll extends Exception {
    final static long serialVersionUID = 0;
}
