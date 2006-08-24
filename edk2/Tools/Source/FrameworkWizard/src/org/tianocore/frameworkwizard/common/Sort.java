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
import org.tianocore.frameworkwizard.module.Identifications.Guids.GuidsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Guids.GuidsVector;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassIdentification;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassVector;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdCodedIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdCodedVector;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdVector;
import org.tianocore.frameworkwizard.module.Identifications.Ppis.PpisIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Ppis.PpisVector;
import org.tianocore.frameworkwizard.module.Identifications.Protocols.ProtocolsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Protocols.ProtocolsVector;
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
     Sort all ppi entries
     
     @param v
     @param mode
     
     **/
    public static void sortPpis(PpisVector v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.getPpis(indexJ).getName().compareTo(v.getPpis(indexI).getName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.getPpis(indexI).getName().compareTo(v.getPpis(indexJ).getName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        PpisIdentification temp = v.getPpis(indexI);
                        v.setPpis(v.getPpis(indexJ), indexI);
                        v.setPpis(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all protocol entries
     
     @param v
     @param mode
     
     **/
    public static void sortProtocols(ProtocolsVector v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.getProtocols(indexJ).getName().compareTo(v.getProtocols(indexI).getName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.getProtocols(indexI).getName().compareTo(v.getProtocols(indexJ).getName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        ProtocolsIdentification temp = v.getProtocols(indexI);
                        v.setProtocols(v.getProtocols(indexJ), indexI);
                        v.setProtocols(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all guid entries
     
     @param v
     @param mode
     
     **/
    public static void sortGuids(GuidsVector v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.getGuids(indexJ).getName().compareTo(v.getGuids(indexI).getName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.getGuids(indexI).getName().compareTo(v.getGuids(indexJ).getName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        GuidsIdentification temp = v.getGuids(indexI);
                        v.setGuids(v.getGuids(indexJ), indexI);
                        v.setGuids(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all pcd coded entries
     
     @param v
     @param mode
     
     **/
    public static void sortPcdCodeds(PcdCodedVector v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.getPcdCoded(indexJ).getName().compareTo(v.getPcdCoded(indexI).getName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.getPcdCoded(indexI).getName().compareTo(v.getPcdCoded(indexJ).getName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        PcdCodedIdentification temp = v.getPcdCoded(indexI);
                        v.setPcdCoded(v.getPcdCoded(indexJ), indexI);
                        v.setPcdCoded(temp, indexJ);
                    }
                }
            }
        }
    }

    /**
     Sort all pcd coded entries
     
     @param v
     @param mode
     
     **/
    public static void sortLibraryClass(LibraryClassVector v, int mode) {
        if (v != null) {
            //
            // sort by name
            //
            for (int indexI = 0; indexI < v.size(); indexI++) {
                for (int indexJ = indexI + 1; indexJ < v.size(); indexJ++) {
                    if ((v.getLibraryClass(indexJ).getLibraryClassName().compareTo(
                                                                                   v.getLibraryClass(indexI)
                                                                                    .getLibraryClassName()) < 0 && mode == DataType.SORT_TYPE_ASCENDING)
                        || (v.getLibraryClass(indexI).getLibraryClassName().compareTo(
                                                                                      v.getLibraryClass(indexJ)
                                                                                       .getLibraryClassName()) < 0 && mode == DataType.SORT_TYPE_DESCENDING)) {
                        LibraryClassIdentification temp = v.getLibraryClass(indexI);
                        v.setLibraryClass(v.getLibraryClass(indexJ), indexI);
                        v.setLibraryClass(temp, indexJ);
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
