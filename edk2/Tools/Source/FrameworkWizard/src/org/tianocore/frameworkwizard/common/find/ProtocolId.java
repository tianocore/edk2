/** @file

 The file is used to define PROTOCOL Identification used by find function

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.common.find;

public class ProtocolId {

    private String name = null;

    private String type = null;

    private String producedModules = null;

    private String consumedModules = null;
    
    private String declaredBy = null;

    public ProtocolId(String arg0, String arg1, String arg2, String arg3, String arg4) {
        this.name = (arg0 == null ? "" : arg0);
        this.type = (arg1 == null ? "" : arg1);
        this.producedModules = (arg2 == null ? "" : arg2);
        this.consumedModules = (arg3 == null ? "" : arg3);
        this.declaredBy = (arg4 == null ? "" : arg4);
    }

    public String getConsumedModules() {
        return consumedModules;
    }

    public void setConsumedModules(String consumedModules) {
        this.consumedModules = consumedModules;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getProducedModules() {
        return producedModules;
    }

    public void setProducedModules(String producedModules) {
        this.producedModules = producedModules;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getDeclaredBy() {
        return declaredBy;
    }

    public void setDeclaredBy(String declaredBy) {
        this.declaredBy = declaredBy;
    }
}
