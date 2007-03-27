/**@file
 AutogenLibOrder class.

 This class is to reorder library instance sequence according to library
 dependence.

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.build.autogen;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Stack;
import java.util.HashSet;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.build.exception.AutoGenException;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.common.exception.EdkException;
import org.tianocore.common.logger.EdkLog;
/**
  This class This class is to reorder library instance sequence according to
  library dependence.
**/
public class AutogenLibOrder {
    ///
    /// The map of library class and its library instance.
    ///
    private Map<String, ModuleIdentification> libClassProducer = new HashMap<String, ModuleIdentification>();

    ///
    /// The map of library instance and its consumed Library Classes.
    ///
    private Map<ModuleIdentification, String[]> libInstanceConsumes = new HashMap<ModuleIdentification, String[]>();

    ///
    /// The map of library instance and its implemeted Library Classes.
    ///
    private Map<ModuleIdentification, String[]> libInstanceProduces = new HashMap<ModuleIdentification, String[]>();

    ///
    /// The map of library instance and its consumers.
    ///
    private Map<ModuleIdentification, HashSet<ModuleIdentification>> libInstanceConsumedBy = new HashMap<ModuleIdentification, HashSet<ModuleIdentification>>();

    ///
    /// List of library instance. It is String[3] list, String[0] is libraryName,
    /// String[1] is libraryConstructor name, String[2] is libDestructor name.
    ///
    private ModuleIdentification[] libInstanceList = null;

    /**
      Constructor function

      This function mainly initialize some member variable.

      @param  libraryList   List of the library instance.
      @throws Exception
    **/
    AutogenLibOrder(ModuleIdentification[] libraryList, String arch) throws EdkException {
        ModuleIdentification libInstance;
        String[]       libClassDeclList = null;
        String[]       libClassConsmList = null;

        libInstanceList = libraryList;
        for (int i = 0; i < libraryList.length; i++) {
            libInstance = libraryList[i];
            //
            // Fetch the constructor & destructor.
            //
            Map<String, XmlObject> libDoc = GlobalData.getDoc(libInstance, arch);
            SurfaceAreaQuery saq = new SurfaceAreaQuery(libDoc);
            libInstance.setConstructor(saq.getLibConstructorName());
            libInstance.setDestructor(saq.getLibDestructorName());

            //
            // Create library class consume database.
            //
            libClassConsmList = saq.getLibraryClasses(CommonDefinition.ALWAYSCONSUMED, arch, null);
            if (libClassConsmList != null) {
                if (this.libInstanceConsumes.containsKey(libInstance)) {
                    throw new AutoGenException(
                            libraryList[i].getName()
                                    + "-- this library instance already exists, please check the library instance list!");
                } else {
                    this.libInstanceConsumes.put(libInstance, libClassConsmList);
                }
            }

            //
            // Create library class implementer database
            //
            libClassDeclList = saq.getLibraryClasses(CommonDefinition.ALWAYSPRODUCED, arch, null);
            if (libClassDeclList != null) {
                this.libInstanceProduces.put(libInstance, libClassDeclList);
                for (int j = 0; j < libClassDeclList.length; j++) {
                    if (this.libClassProducer.containsKey(libClassDeclList[j])) {
                        EdkLog.log(EdkLog.EDK_ERROR,libClassDeclList[j]
                                + " class is already implemented by "
                                + this.libClassProducer.get(libClassDeclList[j]));
                        throw new AutoGenException("Library Class: " + libClassDeclList
                                + " already has a library instance!");
                    } else {
                        this.libClassProducer.put(libClassDeclList[j], libInstance);
                    }
                }
            }
        }

        //
        // Create a consumed-by database
        //
        for (Iterator it = libClassProducer.keySet().iterator(); it.hasNext();) {
            String className = (String)it.next();
            libInstance = libClassProducer.get(className);
            libInstanceConsumedBy.put(libInstance, new HashSet<ModuleIdentification>());

            for (int k = 0; k < libraryList.length; ++k) {
                ModuleIdentification consumer = libraryList[k];
                String[] consumedClassList = libInstanceConsumes.get(consumer);

                for (int l = 0; l < consumedClassList.length; ++l) {
                    if (consumedClassList[l].equals(className)) {
                        libInstanceConsumedBy.get(libInstance).add(consumer);
                    }
                }
            }
        }
    }

    /**
      orderLibInstance

      This function reorder the library instance according the library class
      dependency, using DAG anaylysis algothim

      @return     List which content the ordered library instance.
    **/
    List<ModuleIdentification> orderLibInstance() throws EdkException {
        LinkedList<ModuleIdentification> orderList = new LinkedList<ModuleIdentification>();
        LinkedList<ModuleIdentification> noConsumerList = new LinkedList<ModuleIdentification>();

        //
        // First, add the library instance without consumers to the Q
        //
        for (int i = 0; i < libInstanceList.length; ++i) {
            if (libInstanceList[i] == null) {
                continue;
            }
            
            if (libInstanceConsumedBy.get(libInstanceList[i]) == null ||  libInstanceConsumedBy.get(libInstanceList[i]).size() == 0) {
                noConsumerList.add(libInstanceList[i]);
            }
        }

        while (noConsumerList.size() > 0) {
            ModuleIdentification n = noConsumerList.poll();
            orderList.addFirst(n);

            String[] consumedClassList = libInstanceConsumes.get(n);
            for (int i = 0; i < consumedClassList.length; ++i) {
                ModuleIdentification m = libClassProducer.get(consumedClassList[i]);
                if (m == null) {
                    continue;
                }
                HashSet<ModuleIdentification> consumedBy = libInstanceConsumedBy.get(m);
                if (consumedBy == null || consumedBy.size() == 0) {
                  continue;
                }

                consumedBy.remove(n);
                if (consumedBy.size() == 0) {
                    noConsumerList.addLast(m);
                }
            }

            boolean circularlyConsumed = false;
            while (noConsumerList.size() == 0 && !circularlyConsumed) {
                circularlyConsumed = true;
                for (int i = 0; i < libInstanceList.length; ++i) {
                    ModuleIdentification libInstance = libInstanceList[i];
                    if (!libInstance.hasConstructor()) {
                        continue;
                    }

                    HashSet<ModuleIdentification> consumedBy = libInstanceConsumedBy.get(libInstance);
                    if (consumedBy == null || consumedBy.size() == 0) {
                        continue;
                    }

                    ModuleIdentification[] consumedByList = consumedBy.toArray(new ModuleIdentification[consumedBy.size()]);
                    for (int j = 0; j < consumedByList.length; ++j) {
                        ModuleIdentification consumer = consumedByList[j];
                        if (consumer.hasConstructor()) {
                            continue;
                        }

                        //
                        // if there's no constructor in the library instance's consumer,
                        // remove it from the consumer list
                        //
                        consumedBy.remove(consumer);
                        circularlyConsumed = false;
                        if (consumedBy.size() == 0) {
                            noConsumerList.addLast(libInstance);
                            break;
                        }
                    }

                    if (noConsumerList.size() > 0) {
                        break;
                    }
                }

                if (noConsumerList.size() == 0 && !circularlyConsumed) {
                  break;
                }
            }
        }

        //
        // Append the remaining library instance to the end of sorted list
        //
        boolean HasError = false;
        for (int i = 0; i < libInstanceList.length; ++i) {
            HashSet<ModuleIdentification> consumedBy = libInstanceConsumedBy.get(libInstanceList[i]);
            if (consumedBy != null && consumedBy.size() > 0 && libInstanceList[i].hasConstructor()) {
                EdkLog.log(EdkLog.EDK_ERROR, libInstanceList[i].getName()
                           + " with constructor has a circular dependency!");
                ModuleIdentification[] consumedByList = consumedBy.toArray(new ModuleIdentification[consumedBy.size()]);
                for (int j = 0; j < consumedByList.length; ++j) {
                    EdkLog.log(EdkLog.EDK_ERROR, " consumed by " + consumedByList[j].getName());
                }
                HasError = true;
            }

            if (!orderList.contains(libInstanceList[i])) {
                orderList.add(libInstanceList[i]);
            }
        }
        if (HasError) {
            throw new AutoGenException("Circular dependency in library instances is found!");
        }

        return orderList;
    }
}
