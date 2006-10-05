/** @file

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

import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class AggregationOperation {

    public static synchronized List<PackageIdentification> union(List<PackageIdentification> list1,
                                                                 List<PackageIdentification> list2) {
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();
        result.addAll(list1);
        Iterator<PackageIdentification> iter = list2.iterator();
        while (iter.hasNext()) {
            PackageIdentification item = iter.next();
            if (!belongs(item, result)) {
                result.add(item);
            }
        }
        return result;
    }

    public static synchronized List<PackageIdentification> intersection(List<PackageIdentification> list1,
                                                                        List<PackageIdentification> list2) {
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();
        Iterator<PackageIdentification> iter = list1.iterator();
        while (iter.hasNext()) {
            PackageIdentification item = iter.next();
            if (belongs(item, list2)) {
                result.add(item);
            }
        }
        return result;
    }

    public static synchronized List<PackageIdentification> minus(List<PackageIdentification> list1,
                                                                 List<PackageIdentification> list2) {
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();
        Iterator<PackageIdentification> iter = list1.iterator();
        while (iter.hasNext()) {
            PackageIdentification item = iter.next();
            if (!belongs(item, list2)) {
                result.add(item);
            }
        }
        return result;
    }

    public static synchronized boolean belongs(PackageIdentification o, List<PackageIdentification> list) {
        Iterator<PackageIdentification> iter = list.iterator();
        while (iter.hasNext()) {
            if (iter.next().equalsWithGuid(o)) {
                return true;
            }
        }
        return false;
    }

    public static synchronized List<PackageIdentification> getExistedItems(PackageIdentification o,
                                                                           List<PackageIdentification> list) {
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();
        Iterator<PackageIdentification> iter = list.iterator();
        while (iter.hasNext()) {
            PackageIdentification item = iter.next();
            if (item.equalsWithGuid(o)) {
                result.add(item);
            }
        }
        return result;
    }
}
