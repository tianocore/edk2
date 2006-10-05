/** @file
 
 The file is used to distribute Far
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.far;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class DistributeRule {
    static PackageQuery pkgQ = new PackageQuery();

    static WorkspaceTools wsTools = new WorkspaceTools();

    static List<PackageIdentification> farPkgList = new ArrayList<PackageIdentification>();

    public static List<PackageIdentification> installFarCheck(Far far) {

        Far myFar = far;
        List<PackageIdentification> pkgDepList = new ArrayList<PackageIdentification>();
        List<PackageIdentification> dbPkgList = new ArrayList<PackageIdentification>();
        List<PackageIdentification> depResultList = new ArrayList<PackageIdentification>();
        //
        // Get Far packages list;
        // 
        try {
            farPkgList = myFar.manifest.getPackageList();
            Iterator pkgItems = farPkgList.iterator();
            while (pkgItems.hasNext()) {
                PackageIdentification id = (PackageIdentification) pkgItems.next();
                pkgDepList = myFar.getPackageDependencies(id);
                depResultList = AggregationOperation.union(depResultList, pkgDepList);
            }
            dbPkgList = vectorToList(wsTools.getAllPackages());

        } catch (Exception e) {
            System.out.println(e.getMessage());
        }

        //
        // Check does the dependence meet the requirement.
        //
        List<PackageIdentification> resultList = AggregationOperation.minus(depResultList,
                                                                            AggregationOperation.union(farPkgList,
                                                                                                       dbPkgList));

        return resultList;

    }

    //    public void installPackgCheck (PackageIdentification pkgId, String pkgPath){
    //        List<PackageIdentification> dbPkgList = new ArrayList<PackageIdentification>();
    //        dbPkgList = vectorToList(wsTools.getAllPackages());
    //        //
    //        // Install far's package one by one.
    //        //
    //        if ((AggregationOperation.getExistItems(pkgId, dbPkgList))){
    //            
    //        }
    //    }

    public void UpdatCheck(String orgFar, String destFar) {

    }

    public static List<PackageIdentification> vectorToList(Vector vec) {
        List<PackageIdentification> set = new ArrayList<PackageIdentification>();
        Iterator vecItem = vec.iterator();
        while (vecItem.hasNext()) {
            set.add((PackageIdentification) vecItem.next());
        }
        return set;
    }
}
