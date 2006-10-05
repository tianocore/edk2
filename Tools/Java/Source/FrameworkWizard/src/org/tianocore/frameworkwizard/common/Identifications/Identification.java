/** @file
 
 The file is used to save basic information
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.Identifications;

public class Identification {

    ///
    /// Define class members
    ///
    private String name;

    private String guid;

    private String version;

    private String path;

    public Identification(String name, String guid, String version) {
        this.name = name;
        this.guid = guid;
        this.version = version;
    }

    public Identification() {

    }

    public Identification(String name, String guid, String version, String path) {
        this.name = name;
        this.guid = guid;
        this.version = version;
        this.path = path;
    }

    public boolean equals(Object obj) {
        if (obj instanceof Identification) {
            Identification id = (Identification) obj;
            if (path.equals(id.path)) {
                return true;
            }
            return false;
        } else {
            return super.equals(obj);
        }
    }

    public boolean equalsWithGuid(Object obj) {
        if (obj instanceof Identification) {
            Identification id = (Identification) obj;
            if (guid.equalsIgnoreCase(id.guid)) {
                if (version == null || id.version == null) {
                    return true;
                } else if (version.trim().equalsIgnoreCase("") || id.version.trim().equalsIgnoreCase("")) {
                    return true;
                } else if (version.equalsIgnoreCase(id.version)) {
                    return true;
                }
            }
            return false;
        } else {
            return super.equals(obj);
        }
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public String getGuid() {
        return guid;
    }

    public String getName() {
        return name;
    }

    public String getVersion() {
        return version;
    }

    public String getPath() {
        return path;
    }

    public int hashCode() {
        return guid.toLowerCase().hashCode();
    }
}
