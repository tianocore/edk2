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

/**
 Interface NestElement is just to define common interfaces for nested element
 **/
public interface NestElement {
    /**
     nested element Interface for up-casting  
     **/
    
    public String getName();

    public void setName(String name);

    public String toString();

    public File getFile();

    public void setFile(File file);

    public void setList(String fileNameList);

    public List<String> getList();
}
