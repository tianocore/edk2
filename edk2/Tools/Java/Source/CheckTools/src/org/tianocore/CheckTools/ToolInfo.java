/** @file
 Tool Definition Class for translating the tools_def.txt entries
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

/**
 * This section should complete array values
 * this.TagName
 * this.Family
 * this.Valid
 * this.Arch
 * this.Targets[]
 * this.CmdCode[]
 * this.Path[]
 * this.Arguments[]
 * 
 */

package org.tianocore.CheckTools;

import java.util.*;

public class ToolInfo extends ArrayList {

    ///
    /// Define Class Serial Version UID
    ///
    private static final long serialVersionUID = 2513613555731096258L;

    private String sTagName;

    private String sFamily;

    private boolean bValid;

    private String sArch;

    private ArrayList<String> aTargetName = null;

    private ArrayList<String> aCmdCode = null;

    private ArrayList<String> aCmdName = null;

    private ArrayList<String> aPath = null;

    private ArrayList<String> aBadPath = null;

    private ArrayList<String> aArguments = null;

    public ToolInfo() {
        super();
        init();
    }

    private void init() {
        sTagName = "";
        sFamily = "";
        bValid = false;
        sArch = "";
        aTargetName = new ArrayList<String>();
        aCmdCode = new ArrayList<String>();
        aCmdName = new ArrayList<String>();
        aPath = new ArrayList<String>();
        aArguments = new ArrayList<String>();
        aBadPath = new ArrayList<String>();
    }

    public String getTagName() {
        return this.sTagName.trim();
    }

    public String getFamily() {
        return this.sFamily.trim();
    }

    public boolean isValid() {
        return this.bValid;
    }

    public String getArch() {
        return this.sArch.trim();
    }

    public ArrayList<String> getTargetName() {
        return this.aTargetName;
    }

    public String getTargetName(int id) {
        return this.aTargetName.get(id).trim();
    }

    public ArrayList<String> getCmdCode() {
        return this.aCmdCode;
    }

    public String getCmdCode(int id) {
        return this.aCmdCode.get(id).trim();
    }

    public ArrayList<String> getCmdName() {
        return this.aCmdName;
    }

    public String getCmdName(int id) {
        return this.aCmdName.get(id).trim();
    }

    public ArrayList<String> getPath() {
        return this.aPath;
    }

    public String getPath(int id) {
        return this.aPath.get(id).trim();
    }

    public ArrayList<String> getArguments() {
        return this.aArguments;
    }

    public String getArguments(int id) {
        return this.aArguments.get(id).trim();
    }

    public ArrayList<String> getBadPath() {
        return this.aBadPath;
    }

    public String getBadPath(int id) {
        return this.aBadPath.get(id).trim();
    }

    public void setTagName(String val) {
        this.sTagName = val.trim();
    }

    public void setFamily(String val) {
        this.sFamily = val.trim();
    }

    public void setValid() {
        this.bValid = true;
    }

    public void setInvalid() {
        this.bValid = false;
    }

    public void setArch(String val) {
        this.sArch = val.trim();
    }

    public void addTargetName(String val) {
        this.aTargetName.add(val.trim());
    }

    public void addCmdCode(String val) {
        this.aCmdCode.add(val.trim());
    }

    public void addCmdName(String val) {
        this.aCmdName.add(val.trim());
    }

    public void addPath(String val) {
        this.aPath.add(val.trim());
    }

    public void addArguments(String val) {
        this.aArguments.add(val.trim());
    }

    public void addBadPath(String val) {
        this.aBadPath.add(val.trim());
    }

}
