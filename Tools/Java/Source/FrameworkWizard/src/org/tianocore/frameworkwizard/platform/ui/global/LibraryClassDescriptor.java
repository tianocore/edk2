/** @file
 This file is for surface area information retrieval.

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.platform.ui.global;

import java.util.Vector;

public class LibraryClassDescriptor {
    public String className = "";
    public String supArchs = "";
    public String supModTypes = "";
    
    public LibraryClassDescriptor (String s1, String s2, String s3) {
        className = s1;
        if (s2 != null) {
            supArchs = s2;    
        }
        if (s3 != null) {
            supModTypes = s3;    
        }
        
    }
    
    public boolean equals(Object obj) {
        if (obj instanceof LibraryClassDescriptor) {
            LibraryClassDescriptor id = (LibraryClassDescriptor)obj;
            if (className.equals(id.className) && sameArchs (supArchs, id.supArchs) && sameModTypes(supModTypes, id.supModTypes)) {
                return true;
            }
            return false;
        }
        else {
            return super.equals(obj);
        }
    }
    
    public int hashCode(){
        return (className + supArchs + supModTypes).toLowerCase().hashCode();
    }
    
    public String toString() {
        return "Library Class "+ className + " [SupArchs: " + supArchs + " SupModTypes: " + supModTypes + "]";
    }
    
    public boolean isSubSetByArchs (LibraryClassDescriptor lcd) {
        if (className.equals(lcd.className)) {
            Vector<String> vArchs1 = getVectorFromString(supArchs);
            Vector<String> vArchs2 = getVectorFromString(lcd.supArchs);
            
            if (isSubSet(vArchs1, vArchs2)) {
                return true;
            }
        }
        return false;
    }
    
    public boolean isSubSetByModTypes (LibraryClassDescriptor lcd) {
        if (className.equals(lcd.className)) {
            Vector<String> vModTypes1 = getVectorFromString(supModTypes);
            Vector<String> vModTypes2 = getVectorFromString(lcd.supModTypes);
            
            if (isSubSet(vModTypes1, vModTypes2)) {
                return true;
            }
        }
        return false;
    }
    
    public boolean hasInterSectionWith (LibraryClassDescriptor lcd) {
        if (className.equals(lcd.className)) {
            Vector<String> vArchs1 = getVectorFromString(supArchs);
            Vector<String> vArchs2 = getVectorFromString(lcd.supArchs);
            if (vArchs1.size() == 0 || (vArchs1.size() == 1 && vArchs1.get(0).equalsIgnoreCase(""))) {
                return true;
            }
            if (vArchs2.size() == 0 || (vArchs2.size() == 1 && vArchs2.get(0).equalsIgnoreCase(""))) {
                return true;
            }
            vArchs1.retainAll(vArchs2);
            if (vArchs1.size() > 0) {
                return true;
            }
        }
        return false;
    }
    
    private boolean isSubSet (Vector<String> v1, Vector<String> v2) {
        if (v2.size() == 0 || (v2.size() == 1 && v2.get(0).equalsIgnoreCase(""))) {
            return true;
        }
        if (v2.containsAll(v1)) {
            return true;
        }
        return false;
    }
    
    public Vector<String> getVectorFromString (String s) {
        if (s == null || s.equals("null")) {
            s = "";
        }
        String[] sa1 = s.split(" ");
        Vector<String> v = new Vector<String>();
        for (int i = 0; i < sa1.length; ++i) {
            v.add(sa1[i]);
        }
        return v;
    }
    
    private boolean sameArchs (String archs1, String archs2) {
        Vector<String> vArchs1 = getVectorFromString(archs1);
        Vector<String> vArchs2 = getVectorFromString(archs2);
        
        if (vArchs1.containsAll(vArchs2) && vArchs2.containsAll(vArchs1)) {
            return true;
        }
        return false;
    }
    
    private boolean sameModTypes (String modTypes1, String modTypes2) {
        return sameArchs(modTypes1, modTypes2);
    }
}