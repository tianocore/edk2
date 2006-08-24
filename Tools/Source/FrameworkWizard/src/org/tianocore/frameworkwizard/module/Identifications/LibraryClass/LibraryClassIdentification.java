/** @file

 The file is used to define Library Class Identification

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.frameworkwizard.module.Identifications.LibraryClass;

import java.util.Vector;

import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class LibraryClassIdentification {
    private String libraryClassName = null;

    private String usage = null;

    private String recommendedInstanceVersion = null;

    private String recommendedInstanceGuid = null;

    private Vector<String> supArchList = null;

    private String featureFlag = null;
    
    private Vector<String> supModuleList = null;
    
    private String help = null;
    
    private ModuleIdentification belongModule = null;
    
    private PackageIdentification declaredBy = null;

    public LibraryClassIdentification() {

    }

    public LibraryClassIdentification(String strName, String strUsage, String strRecommendedInstanceVersion,
                                      String strRecommendedInstanceGuid, Vector<String> vSupArchList,
                                      String strFeatureFlag, Vector<String> vSupModuleList, String strHelp) {
        this.libraryClassName = (strName == null ? "" : strName);
        this.usage = (strUsage == null ? "" : strUsage);
        this.recommendedInstanceVersion = (strRecommendedInstanceVersion == null ? "" : strRecommendedInstanceVersion);
        this.recommendedInstanceGuid = (strRecommendedInstanceGuid == null ? "" : strRecommendedInstanceGuid);
        this.supArchList = vSupArchList;
        this.featureFlag = (strFeatureFlag == null ? "" : strFeatureFlag);
        this.supModuleList = vSupModuleList;
        this.help = (strHelp == null ? "" : strHelp);
    }

    public String getLibraryClassName() {
        return libraryClassName;
    }

    public void setLibraryClassName(String libraryClassName) {
        this.libraryClassName = libraryClassName;
    }

    public String getUsage() {
        return usage;
    }

    public void setUsage(String usage) {
        this.usage = usage;
    }

    public String getFeatureFlag() {
        return featureFlag;
    }

    public void setFeatureFlag(String featureFlag) {
        this.featureFlag = featureFlag;
    }

    public String getRecommendedInstanceGuid() {
        return recommendedInstanceGuid;
    }

    public void setRecommendedInstanceGuid(String recommendedInstanceGuid) {
        this.recommendedInstanceGuid = recommendedInstanceGuid;
    }

    public String getRecommendedInstanceVersion() {
        return recommendedInstanceVersion;
    }

    public void setRecommendedInstanceVersion(String recommendedInstanceVersion) {
        this.recommendedInstanceVersion = recommendedInstanceVersion;
    }

    public Vector<String> getSupArchList() {
        return supArchList;
    }

    public void setSupArchList(Vector<String> supArchList) {
        this.supArchList = supArchList;
    }
    
    public boolean equals(LibraryClassIdentification lib) {
        if (this.libraryClassName.equals(lib.libraryClassName) && this.usage.equals(lib.getUsage())) {
            return true;
        }
        return false;
    }

    public Vector<String> getSupModuleList() {
        return supModuleList;
    }

    public void setSupModuleList(Vector<String> supModuleList) {
        this.supModuleList = supModuleList;
    }

    public String getHelp() {
        return help;
    }

    public void setHelp(String help) {
        this.help = help;
    }

    public ModuleIdentification getBelongModule() {
        return belongModule;
    }

    public void setBelongModule(ModuleIdentification belongModule) {
        this.belongModule = belongModule;
    }

    public PackageIdentification getDeclaredBy() {
        return declaredBy;
    }

    public void setDeclaredBy(PackageIdentification declaredBy) {
        this.declaredBy = declaredBy;
    }
}
