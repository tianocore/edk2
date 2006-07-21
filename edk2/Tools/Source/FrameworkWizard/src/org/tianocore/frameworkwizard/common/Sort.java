/** @file
 
 The file is used to provide all kinds of sorting method 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common;

import java.util.Vector;

import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdVector;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;

public class Sort {

    /**
     Sort all elements in the vector as the specific sort type
     
     @param v The vector need to be sorted
     @param mode Sort type DataType.Sort_Type_Ascendin and DataType.Sort_Type_Descending

     **/
    public static void sortVectorString(Vector<String> v, int mode) {
        if (v != null) {
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.get(indexJ).compareTo(v.get(indexI)) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.get(indexI).compareTo(v.get(indexJ)) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        String temp = v.get(indexI);
                        v.setElementAt(v.get(indexJ), indexI);
                        v.setElementAt(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all elements of vector and return sorted sequence
     
     @param v The vector need to be sorted
     @param mode Sort type DataType.Sort_Type_Ascendin and DataType.Sort_Type_Descending
     @return Vector<Integer> The sorted sequence
     
     **/
    public static Vector<Integer> getVectorSortSequence(Vector<String> v, int mode) {
        Vector<Integer> vSequence = new Vector<Integer>();
        //
        // Init sequence
        //
        if (v != null) {
            for (int index = 0; index < v.size(); index++) {
                vSequence.addElement(index);
            }
        }

        //
        // sort and get new sequence
        //
        for (int indexI = 0; indexI < v.size(); indexI++) {
            for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                if ((v.get(indexJ).compareTo(v.get(indexI)) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                    || (v.get(indexI).compareTo(v.get(indexJ)) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                    //
                    // Swap strings
                    //
                    String tempStr = v.get(indexI);
                    v.setElementAt(v.get(indexJ), indexI);
                    v.setElementAt(tempStr, indexJ);

                    //
                    // Swap sequences
                    //
                    int tempInt = vSequence.get(indexI);
                    vSequence.setElementAt(vSequence.get(indexJ), indexI);
                    vSequence.setElementAt(tempInt, indexJ);
                }
            }
        }

        return vSequence;
    }

    /**
     Sort all elements of vector as input sequence
     
     @param v The vector need to be sorted
     @param vSequence The sort sequence should be followed
     
     **/
    public static void sortVectorString(Vector<String> v, Vector<Integer> vSequence) {
        if (v != null && vSequence != null && v.size() == vSequence.size()) {
            Vector<String> tempV = new Vector<String>();
            for (int index = 0; index < v.size(); index++) {
                tempV.addElement(v.get(index));
            }
            for (int index = 0; index < v.size(); index++) {
                v.setElementAt(tempV.get(vSequence.get(index)), index);
            }
        }
    }

    /**
     Sort all modules
     
     @param v
     @param mode
     
     **/
    public static void sortModules(Vector<ModuleIdentification> v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.get(indexJ).getName().compareTo(v.get(indexI).getName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.get(indexI).getName().compareTo(v.get(indexJ).getName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        ModuleIdentification temp = v.get(indexI);
                        v.setElementAt(v.get(indexJ), indexI);
                        v.setElementAt(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all packages
     
     @param v
     @param mode
     
     **/
    public static void sortPackages(Vector<PackageIdentification> v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.get(indexJ).getName().compareTo(v.get(indexI).getName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.get(indexI).getName().compareTo(v.get(indexJ).getName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        PackageIdentification temp = v.get(indexI);
                        v.setElementAt(v.get(indexJ), indexI);
                        v.setElementAt(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all platforms
     
     @param v
     @param mode
     
     **/
    public static void sortPlatforms(Vector<PlatformIdentification> v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.get(indexJ).getName().compareTo(v.get(indexI).getName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.get(indexI).getName().compareTo(v.get(indexJ).getName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        PlatformIdentification temp = v.get(indexI);
                        v.setElementAt(v.get(indexJ), indexI);
                        v.setElementAt(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all pcd entries
     
     @param v
     @param mode
     
     **/
    public static void sortPcds(PcdVector v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.getPcd(indexJ).getName().compareTo(v.getPcd(indexI).getName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.getPcd(indexI).getName().compareTo(v.getPcd(indexJ).getName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        PcdIdentification temp = v.getPcd(indexI);
                        v.setPcd(v.getPcd(indexJ), indexI);
                        v.setPcd(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all objects of a vector based on the object's "toString"
     
     @param v
     @param mode
     
     **/
    public static void sortObjectVector(Vector<Object> v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.get(indexJ).toString().compareTo(v.get(indexI).toString()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.get(indexI).toString().compareTo(v.get(indexJ).toString()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        Object temp = v.get(indexI);
                        v.setElementAt(v.get(indexJ), indexI);
                        v.setElementAt(temp, indexJ);
                    }
                }
            }
        }
    }
}
