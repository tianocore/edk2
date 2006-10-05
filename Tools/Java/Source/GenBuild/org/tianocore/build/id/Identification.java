/** @file
This file is to define  Identification class.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

package org.tianocore.build.id;


/**
  This class is used to identify with its GUID and Version. 

  @since GenBuild 1.0
**/
public class Identification {

    String name;
    
    String guid;
    
    String version;
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
    **/
    Identification(String name, String guid, String version){
        this.name = name;
        this.guid = guid;
        this.version = version;
    }
    
    /**
      @param guid Guid
      @param version Version
    **/
    Identification(String guid, String version){
        this.guid = guid;
        this.version = version;
    }
    
    /* (non-Javadoc)
      @see java.lang.Object#equals(java.lang.Object)
    **/
    public boolean equals(Object obj) {
        if (obj instanceof Identification) {
            Identification id = (Identification)obj;
            if ( guid.equalsIgnoreCase(id.guid)) {
                if (version == null || id.version == null) {
                    return true;
                }
                else if (version.trim().equalsIgnoreCase("") || id.version.trim().equalsIgnoreCase("")){
                    return true;
                }
                else if (version.equalsIgnoreCase(id.version)) {
                    return true;
                }
            }
            return false;
        }
        else {
            return super.equals(obj);
        }
    }
    
    /**
      @param name Name
    **/
    public void setName(String name) {
        this.name = name;
    }

    /**
      @param guid Guid
    **/
    public void setGuid(String guid) {
        this.guid = guid;
    }

    /**
      @param version Version
    **/
    public void setVersion(String version) {
        this.version = version;
    }

    public String getGuid() {
        return guid;
    }

    /**
      @return String Name
    **/
    public String getName() {
        return name;
    }

    /**
      @return String Version
    **/
    public String getVersion() {
        return version;
    }
    
    public String toGuidString() {
        if (version == null || version.trim().equalsIgnoreCase("")) {
            return "[" + guid + "]";
        }
        else {
            return "[" + guid + "] and version [" + version + "]"; 
        }
    }
    
    /* (non-Javadoc)
      @see java.lang.Object#hashCode()
    **/
    public int hashCode(){
        return guid.toLowerCase().hashCode();
    }
}
