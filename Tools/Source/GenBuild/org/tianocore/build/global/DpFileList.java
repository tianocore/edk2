/** @file
This file is used to nest elements corresponding to DpFile

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.global;

import java.util.ArrayList;
import java.util.List;

import org.apache.tools.ant.DirectoryScanner;
import org.apache.tools.ant.types.DataType;
import org.apache.tools.ant.types.FileSet;

/**
 DpFileList is a container of Dpfile at the point of ANT task/datatype
 **/
public class DpFileList extends DataType {
    ///
    /// Keep all the file names from all nested DpFile
    ///
    List<String> nameList = new ArrayList<String>();

    /**
     Empty constructor just in case
     **/
    public DpFileList() {
    }

    /**
     Empty execute method of ANT task. ANT will call it even we don't need it.
     **/
    public void execute() {
    }

    /**
     Standard add method of ANT task, for nested DpFile type of elements. It just
     simply fetch the files list from DpFile and put them in its own nameList.

     @param     f   a DpFile object which will be instantiated by ANT
     **/
    public void addConfiguredFile(DpFile f) {
        this.nameList.addAll(f.getList());
    }

    public void addConfiguredFileSet(FileSet fileSet) {
        DirectoryScanner ds = fileSet.getDirectoryScanner(getProject());
        String dir = fileSet.getDir(getProject()).getAbsolutePath();
        String[] files = ds.getIncludedFiles();

        for (int i = 0; i < files.length; ++i) {
            nameList.add(dir + "/" + files[i]);
        }
    }
}

