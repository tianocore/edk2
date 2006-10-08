/** @file
This file is to define common interfaces for nested element of frameworktasks

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.framework.tasks;

import java.io.File;
import java.util.List;
import java.util.ArrayList;
import java.io.FileReader;
import java.io.BufferedReader;
import java.util.StringTokenizer;

import org.apache.tools.ant.types.DataType;
import org.apache.tools.ant.types.Path;
import org.apache.tools.ant.BuildException;

/**
 Interface NestElement is to define common interfaces for nested element
 **/
public class NestElement extends DataType {
    //
    // The name list. All the name strings got from setXXX methods will be put
    // in here.
    //
    protected List<String> nameList = new ArrayList<String>();

    /**
       Insert content in the newElement into this NestElement

       @param newElement    The new NestElement
     **/
    public void insert(NestElement newElement) {
        this.nameList.addAll(newElement.getNameList());
    }

    /**
       Handle "name" attribute. No delimiter and special treatment are assumed.

       @param name  A single string value of "name" attribute 
     **/
    public void setName(String name) {
        if (name.length() > 0) {
            this.nameList.clear();
            this.nameList.add(name);
        }
    }

    public void insName(String name) {
        if (name.length() > 0) {
            this.nameList.add(name);
        }
    }

    /**
       Handle "list" attribute. The value of "list" is assumed as string 
       separated by space, tab, comma or semmicolon.

       @param nameList  The value of "list" separated by " \t,;"
     **/
    public void setList(String nameList) {
        if (nameList.length() == 0) {
            return;
        }

        this.nameList.clear();
        StringTokenizer tokens = new StringTokenizer(nameList, " \t,;", false);
        while (tokens.hasMoreTokens()) {
            String name = tokens.nextToken().trim();
            if (name.length() > 0) {
                this.nameList.add(name);
            }
        }
    }

    /**
       Handle "ListFile" attribute. The value of "ListFile" should be the path of
       a file which contains name strings, one name per line.

       @param listFileName  The file path
     **/
    public void setListFile(String listFileName) {
        FileReader fileReader = null;
        BufferedReader in = null;
        String str;

        //
        // Check if the file exists or not
        // 
        File file = new File(listFileName);
        if (!file.exists()) {
            throw new BuildException("The file, " + file + " does not exist!");           
        } 

        try {
            fileReader = new FileReader(file);
            in = new BufferedReader(fileReader);

            //
            // Read line by line
            // 
            nameList.clear();
            while((str = in.readLine()) != null){
                str = str.trim();
                if (str.length() == 0){
                    continue;
                }

                //getProject().replaceProperties(str);
                this.nameList.add(str);
            }
        } catch (Exception e){
            throw new BuildException(e.getMessage());            
        } finally {
            try {
                //
                // close the file
                // 
                if (in != null) {
                    in.close();
                }
                if (fileReader != null) {
                    fileReader.close();
                }
            } catch (Exception e) {
                throw new BuildException(e.getMessage());            
            }
        }
    }

    /**
       Handle "file" attribute. The value of "file" should be a path.

       @param file  The path name of a file
     **/
    public void setFile(String file) {
        setPath(file);
    }

    /**
       Add a file or file list into the file list

       @param file  The path of a file
     **/
    public void insFile(String file) {
        insPath(file);
    }

    /**
       Handle "path" attribute. The value of "path" may contain compound path
       separator (/ or \) which should be cleaned up. Because the "path" string
       will always be passed to external native program which may not handle 
       non-native path separator, the clean-up action is a must. And the value
       of "path" may contains several path separated by space, tab, comma or
       semmicolon. We need to split it and put each part in nameList.

       @param path  String value of a file system path
     **/
    public void setPath(String path) {
        this.nameList.clear();
        insPath(path);
    }

    /**
       Add a path or path list into the path list

       @param path  The path string
     **/
    public void insPath(String path) {
        if (path.length() == 0) {
            return;
        }

        //
        // split the value of "path" into separated single path
        // 
        StringTokenizer tokens = new StringTokenizer(path, " \t,;", false);
        while (tokens.hasMoreTokens()) {
            String pathName = tokens.nextToken().trim();
            if (pathName.length() > 0) {
                //
                // Make clean the path string before storing it
                // 
                this.nameList.add(cleanupPath(pathName));
            }
        }
    }

    /**
       Handle "FileName" attribute. The value of "FileName" should be the path
       of a file which contains path strings, one path per line.

       @param pathFileName
     **/
    public void setPathFile(String pathFileName) {
        FileReader fileReader = null;
        BufferedReader in = null;
        String path;

        //
        // Check if the file exists or not
        // 
        File file = new File(pathFileName);
        if (!file.exists()) {
            throw new BuildException("The file, " + file + " does not exist!");           
        } 

        try {
            fileReader = new FileReader(file);
            in = new BufferedReader(fileReader);

            //
            // Read the file line by line, skipping empty ones
            // 
            nameList.clear();
            while((path = in.readLine()) != null){
                path = path.trim();
                if (path.length() == 0){
                    continue;
                }
                //getProject().replaceProperties(path);

                //
                // Make clean the path string before storing it.
                // 
                nameList.add(cleanupPath(path));
            }
        } catch (Exception e){
            throw new BuildException(e.getMessage());            
        } finally {
            try {
                //
                // close the file
                // 
                if (in != null) {
                    in.close();
                }
                if (fileReader != null) {
                    fileReader.close();
                }
            } catch (Exception e) {
                throw new BuildException(e.getMessage());            
            }
        }
    }

    /**
       Return the name list.

       @return List<String> The list contains the name(path) strings
     **/
    public List<String> getNameList() {
        return nameList;
    }

    /**
       Compose and return the the name/path string without any delimiter. The trick
       here is that it's actually used to return the value of nameList which
       has just one name/string.

       @return String
     **/
    public String toString() {
        return toString("");
    }

    /**
       Compose and return the name/path string concatenated by leading "prefix".

       @param prefix    The string will be put before each name/string in nameList
       
       @return String   The string concatenated with "prefix"
     **/
    public String toString(String prefix) {
        StringBuffer string = new StringBuffer(1024);
        int length = nameList.size();

        for (int i = 0; i < length; ++i) {
            string.append(prefix);
            string.append(nameList.get(i));
        }

        return string.toString();
    }

    /**
       Compose and return the name/path string concatenated by space and
       with only one "prefix".

       @param prefix    The prefix at the beginning of the string
       
       @return String   The string with one prefix at the beginning
     **/
    public String toStringWithSinglepPrefix(String prefix) {
        return prefix + toString(" ");
    }

    /**
       Compose a string list with file names only, separated by spcified string

       @param separator     The separator string
       
       @return String       The file list
     **/
    public String toFileList(String separator) {
        StringBuffer string = new StringBuffer(1024);
        int length = nameList.size();

        for (int i = 0; i < length; ++i) {
            File file = new File(nameList.get(i));
            string.append(file.getName());
            string.append(separator);
        }

        return string.toString();
    }

    /**
       Compose a string list with file names only, separated by space

       @return String   The list string
     **/
    public String toFileList() {
        return toFileList(" ");
    }

    /**
       Get the array of names

       @return String[]     The array contains the names
     **/
    public String[] toArray() {
        return nameList.toArray(new String[nameList.size()]);
    }

    /**
       Check if we have any name or not

       @return boolean
     **/
    public boolean isEmpty() {
        return nameList.isEmpty();
    }

    //
    // Remove any duplicated path separator or inconsistent path separator
    //
    private String cleanupPath(String path) {
        String separator = "\\" + File.separator;
        String duplicateSeparator = separator + "{2}";

        path = Path.translateFile(path);
        path = path.replaceAll(duplicateSeparator, separator);

        return path;
    }
}
