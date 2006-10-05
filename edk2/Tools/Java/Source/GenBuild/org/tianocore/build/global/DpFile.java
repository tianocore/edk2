/** @file
This file is used to define class which represents dependency file in ANT task

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.global;

import org.apache.tools.ant.types.DataType;
import org.apache.tools.ant.types.Path;
import org.apache.tools.ant.BuildException;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.LineNumberReader;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 DpFile is a ANT DataType which can be used to specify dependency files from
 a file list file, from file list string separated by space, comma or semi-comma,
 or from file name with absolute path
 **/
public class DpFile  extends DataType {
    ///
    /// keep the list of files path
    ///
    private List<String> nameList = new ArrayList<String>();

    /**
     Empty constructor just in case
     **/
    public DpFile() {
    }

    /**
     Empty execute method of ANT task, just in case
     **/
    public void execute() {
    }

    /**
     Standard set method of ANT task/datatype, for ListFile attribute. It simply
     fetch one file path a line from specified list file, and put them in nameList

     @param     fileListFile    file which contains a file list, one file a line,
                                with full path
     **/
    public void setListFile(String fileListFile) {
        File file = new File(fileListFile);
        if (!file.exists()) {
            return;
        }

        try {
            FileReader fileReader = new FileReader(file);
            LineNumberReader lineReader = new LineNumberReader(fileReader);

            String filePath = null;
            while ((filePath = lineReader.readLine()) != null) {
                filePath = filePath.trim();
                if (filePath.length() == 0) {
                    continue;
                }
                this.nameList.add(filePath);
            }

            lineReader.close();
            fileReader.close();
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
     Standard set method of ANT task/datatype, for List attribute.

     @param     fileList        string with file pathes separated by space, comma,
                                or semi-comma
     **/
    public void setList(String fileList) {
        //
        // space, comma or semi-comma separated files list
        //
        Pattern pattern = Pattern.compile("([^ ,;\n\r]++)[ ,;\n\r]*+");
        Matcher matcher = pattern.matcher(fileList);

        while (matcher.find()) {
            //
            // keep each file name before " ,;\n\r"
            //
            String filePath = fileList.substring(matcher.start(1), matcher.end(1)).trim();
            if (filePath.length() == 0) {
                continue;
            }
            nameList.add(Path.translateFile(filePath));
        }

    }

    /**
     Standard set method of ANT task/datatype, for Name attribute.

     @param     fileName        string of a file full path
     **/
    public void setName(String fileName) {
        this.nameList.add(fileName);
    }

    /**
     Fetch the file name list.

     @returns   A string list which contains file names specified to check dependnecy
     **/
    public List<String> getList() {
        return this.nameList;
    }
}


