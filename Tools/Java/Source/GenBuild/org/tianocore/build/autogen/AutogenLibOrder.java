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
    private Map<String, ModuleIdentification> libClassMap = new HashMap<String, ModuleIdentification>();

    ///
    /// The map of library instance and its implemet libraryClass.
    ///
    private Map<ModuleIdentification, String[]> libInstanceMap = new HashMap<ModuleIdentification, String[]>();

    ///
    /// List of library instance. It is String[3] list, String[0] is libraryName,
    /// String[1] is libraryConstructor name, String[2] is libDestructor name.
    ///
    private List<LibraryInstanceNode> libInstanceList = new ArrayList<LibraryInstanceNode>();
    
    /**
      Constructor function
    
      This function mainly initialize some member variable.
     
      @param  libraryList   List of the library instance.
      @throws Exception
    **/
    AutogenLibOrder(ModuleIdentification[] libraryList, String arch) throws EdkException {
        LibraryInstanceNode libInstanceNode;
        String[]       libClassDeclList = null;
        String[]       libClassConsmList = null;
        
        for (int i = 0; i < libraryList.length; i++) {
            //
            // Add libraryInstance in to libInstanceList.
            // 
            Map<String, XmlObject> libDoc = GlobalData.getDoc(libraryList[i], arch);
            SurfaceAreaQuery saq = new SurfaceAreaQuery(libDoc);
            libInstanceNode = new LibraryInstanceNode (libraryList[i],saq.getLibConstructorName(), saq.getLibDestructorName());
            libInstanceList.add(libInstanceNode);
            
            //
            // Add library instance and consumed library class list to
            // libInstanceMap.
            //
            libClassConsmList = saq.getLibraryClasses(CommonDefinition.ALWAYSCONSUMED, arch);
            if (libClassConsmList != null) {
                String[] classStr = new String[libClassConsmList.length];
                for (int k = 0; k < libClassConsmList.length; k++) {
                    classStr[k] = libClassConsmList[k];
                }
                if (this.libInstanceMap.containsKey(libraryList[i])) {
                    throw new AutoGenException(
                            libraryList[i].getName()
                                    + "-- this library instance already exists, please check the library instance list!");
                } else {
                    this.libInstanceMap.put(libraryList[i], classStr);
                }
            }

            //
            // Add library class and library instance map.
            //
            libClassDeclList = saq.getLibraryClasses(CommonDefinition.ALWAYSPRODUCED, arch);
            if (libClassDeclList != null) {
                for (int j = 0; j < libClassDeclList.length; j++) {
                    if (this.libClassMap.containsKey(libClassDeclList[j])) {
                        EdkLog.log(EdkLog.EDK_ERROR,libClassDeclList[j]
                                + " class is already implement by "
                                + this.libClassMap.get(libClassDeclList[j]));
                        throw new AutoGenException("Library Class: " + libClassDeclList
                                + " already has a library instance!");
                    } else {
                        this.libClassMap.put(libClassDeclList[j], libraryList[i]);
                    }
                }
            }
        }
    }

    /**
      orderLibInstance
      
      This function reorder the library instance according the library class 
      dependency.
      
      @return     List which content the ordered library instance.
    **/
    List<ModuleIdentification> orderLibInstance() {
        LinkedList<ModuleIdentification> orderList = new LinkedList<ModuleIdentification>();
        for (int i = 0; i < libInstanceList.size(); ++i) {
            ModuleIdentification current = libInstanceList.get(i).libId;
            int insertPoint = orderList.size();
            //
            // check current library instance against orderred ones in orderList
            // 
            for (int j = 0; j < orderList.size(); ++j) {
                ModuleIdentification old = orderList.get(j);
                if (consumes(current, old)) {
                    //
                    // if current library instance consumes the one in orderList
                    // it must be put after
                    // 
                    insertPoint = j + 1;
                } else if (consumes(old, current)) {
                    //
                    // if current library instance is consumed by the one in orderList
                    // it must be put before. And no further check is needed.
                    // 
                    insertPoint = j;
                    break;
                }
            }
            orderList.add(insertPoint, current);
        }

        return orderList;
    }

    //
    // Test if one library consumes another library
    // 
    private boolean consumes(ModuleIdentification lib1, ModuleIdentification lib2) {
        LinkedList<ModuleIdentification> stack = new LinkedList<ModuleIdentification>();

        stack.add(lib1);
        int j = 0;
        while (j < stack.size()) {
            //
            // get the last library instance in stack, which hasn't been checked
            // 
            ModuleIdentification lib = stack.get(j++);
            //
            // get the library classes consumed by it
            // 
            String[] consumedClasses = libInstanceMap.get(lib);
            for (int i = 0; i < consumedClasses.length; ++i) {
                //
                // for each library class, find its corresponding library instance
                // 
                ModuleIdentification consumedLib = libClassMap.get(consumedClasses[i]);
                //
                // if the corresponding instance is the "lib2", we can say that
                // "lib1"  consumes "lib2"
                // 
                if (consumedLib == lib2) {
                    EdkLog.log(EdkLog.EDK_DEBUG, lib1 + "\n   consumes\n" + lib2 + "\n");
                    return true;
                }
                //
                // otherwise, we put it back into the stack to check it later
                // to see if it consumes "lib2" or not. If the library instance
                // consumed by "lib1" consumes "lib2", we can also say that "lib1"
                // consumes "lib2"
                // 
                if (consumedLib != null && !stack.contains(consumedLib)) {
                    stack.offer(consumedLib);
                } else if (consumedLib == lib1) {
                    //
                    // found circular consume, do nothing now but just print
                    // out message for debugging
                    // 
                    String msg = "!!! Library consumes circularly: ";
                    for (int k = 0; k < j; k++) {
                        msg += stack.get(k).getName() + "->";
                    }
                    msg += lib1.getName();
                    EdkLog.log(EdkLog.EDK_DEBUG, msg);
                }
            }
        }
        return false;
    }

    /**
      isInLibInstance
    
      This function check does the library instance already in the list.
    
      @param list             List of the library instance.
      @param instanceName     Name of library instance.
      @return                 "true" the library instance in list |
                              "false" the library instance is not in list.
    **/
    private boolean isInLibInstance(List<ModuleIdentification> list, ModuleIdentification instanceId) {
        for (int i = 0; i < list.size(); i++) {
            
            if (instanceId.equals(list.get(i))) {
                return true;
            }
        }
        return false;
    }

    /**
      isInStackList 
      
      This function check if the node already in the stack.
       
      @param list        Stack.
      @param nodeName    Name of node.
      @return            "true" if node have in stack |
                         "false" if node don't in stack.
    **/ 
    private boolean isInStackList(List<Node> list, ModuleIdentification instanceId) {
        for (int i = 0; i < list.size(); i++) {
            if (instanceId.equals(list.get(i).nodeId)) {
                return true;
            }
        }
        return false;
    }
    
    /**
      isHaveConsDestructor
      
      This function check if the library have constructor or destructor 
      function.
      
      @param  libName    Name of library
      @return            "true" if library have constructor or desconstructor |
                         "false" if library don't have constructor 
                         and desconstructor.
    **/
    private boolean isHaveConsDestructor (ModuleIdentification libNode){
        for (int i = 0; i < libInstanceList.size(); i++){
            if (libInstanceList.get(i).libId.equals(libNode)){
                if (libInstanceList.get(i).constructorName != null || libInstanceList.get(i).deconstructorName != null){
                    return true;
                }
            }
        }
        return false;
    }
}

/**
  Node 
 
  This class is used as stack node.
 
 **/
class Node {
    ModuleIdentification nodeId;

    boolean isVisit;

    Node(ModuleIdentification nodeId, boolean isVisit) {
        this.nodeId = nodeId;
        this.isVisit = false;
    }
}  
/**
  LibraryInstance Node   
  
  This class is used to store LibrayInstance and it's deconstructor and constructor
**/
    
class LibraryInstanceNode {
    ModuleIdentification libId;
    String deconstructorName;
    String constructorName;
    
    LibraryInstanceNode (ModuleIdentification libId, String deconstructor, String constructor){
        this.libId = libId;
        this.deconstructorName = deconstructor;
        this.constructorName   = constructor;
    }
}
