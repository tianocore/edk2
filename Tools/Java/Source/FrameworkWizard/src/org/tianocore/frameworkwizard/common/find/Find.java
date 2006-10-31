/** @file

 The file is used to provide Find funtions in workspace

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.common.find;

import java.util.Vector;

import org.tianocore.GuidDeclarationsDocument.GuidDeclarations;
import org.tianocore.LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass;
import org.tianocore.LibraryClassDefinitionsDocument.LibraryClassDefinitions;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PcdDeclarationsDocument.PcdDeclarations.PcdEntry;
import org.tianocore.PpiDeclarationsDocument.PpiDeclarations;
import org.tianocore.ProtocolDeclarationsDocument.ProtocolDeclarations;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.common.Sort;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Guids.GuidsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Guids.GuidsVector;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassIdentification;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassVector;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdCodedIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdCodedVector;
import org.tianocore.frameworkwizard.module.Identifications.Ppis.PpisIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Ppis.PpisVector;
import org.tianocore.frameworkwizard.module.Identifications.Protocols.ProtocolsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Protocols.ProtocolsVector;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class Find {

    private final static String SEPERATOR = ".";

    /**
     Get all ppi entries from workspace
     
     @return
     
     **/
    public static PpisVector getAllPpisVector() {
        PpisVector pv = new PpisVector();
        ModuleSurfaceArea msa = null;
        ModuleIdentification mid = null;
        PpisIdentification pid = null;

        //
        // Go through each module one by one
        //
        if (GlobalData.openingModuleList.size() > 0) {
            for (int indexOfModule = 0; indexOfModule < GlobalData.openingModuleList.size(); indexOfModule++) {
                msa = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getXmlMsa();
                mid = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getId();

                if (msa.getPPIs() != null) {
                    if (msa.getPPIs().getPpiList().size() > 0) {
                        for (int index = 0; index < msa.getPPIs().getPpiList().size(); index++) {
                            String arg0 = msa.getPPIs().getPpiList().get(index).getPpiCName();
                            String arg1 = DataType.PPI_TYPE_PPI;
                            String arg2 = null;
                            if (msa.getPPIs().getPpiList().get(index).getUsage() != null) {
                                arg2 = msa.getPPIs().getPpiList().get(index).getUsage().toString();
                            }

                            String arg3 = msa.getPPIs().getPpiList().get(index).getFeatureFlag();
                            Vector<String> arg4 = Tools.convertListToVector(msa.getPPIs().getPpiList().get(index)
                                                                               .getSupArchList());
                            String arg5 = msa.getPPIs().getPpiList().get(index).getHelpText();

                            pid = new PpisIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                            pid.setBelongModule(mid);

                            //
                            // Find which package declares it
                            //
                            for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
                                PackageSurfaceArea spd = GlobalData.openingPackageList
                                                                                      .getOpeningPackageByIndex(
                                                                                                                indexOfPackage)
                                                                                      .getXmlSpd();
                                PackageIdentification packageId = GlobalData.openingPackageList
                                                                                               .getOpeningPackageByIndex(
                                                                                                                         indexOfPackage)
                                                                                               .getId();
                                if (spd.getPpiDeclarations() != null) {
                                    for (int indexOfPpis = 0; indexOfPpis < spd.getPpiDeclarations().getEntryList()
                                                                               .size(); indexOfPpis++) {
                                        if (spd.getPpiDeclarations().getEntryList().get(indexOfPpis).getCName()
                                               .equals(arg0)) {
                                            pid.setDeclaredBy(packageId);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (pid.getDeclaredBy() == null) {
                                pid.setDeclaredBy(new PackageIdentification("", "", "", ""));
                            }

                            pv.addPpis(pid);
                        }
                    }
                    if (msa.getPPIs().getPpiNotifyList().size() > 0) {
                        for (int index = 0; index < msa.getPPIs().getPpiNotifyList().size(); index++) {
                            String arg0 = msa.getPPIs().getPpiNotifyList().get(index).getPpiNotifyCName();
                            String arg1 = DataType.PPI_TYPE_PPI_NOTIFY;
                            String arg2 = null;
                            if (msa.getPPIs().getPpiNotifyList().get(index).getUsage() != null) {
                                arg2 = msa.getPPIs().getPpiNotifyList().get(index).getUsage().toString();
                            }

                            String arg3 = msa.getPPIs().getPpiNotifyList().get(index).getFeatureFlag();
                            Vector<String> arg4 = Tools.convertListToVector(msa.getPPIs().getPpiNotifyList().get(index)
                                                                               .getSupArchList());
                            String arg5 = msa.getPPIs().getPpiNotifyList().get(index).getHelpText();

                            pid = new PpisIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                            pid.setBelongModule(mid);

                            //
                            // Find which package declares it
                            //
                            for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
                                PackageSurfaceArea spd = GlobalData.openingPackageList
                                                                                      .getOpeningPackageByIndex(
                                                                                                                indexOfPackage)
                                                                                      .getXmlSpd();
                                PackageIdentification packageId = GlobalData.openingPackageList
                                                                                               .getOpeningPackageByIndex(
                                                                                                                         indexOfPackage)
                                                                                               .getId();
                                if (spd.getPpiDeclarations() != null) {
                                    for (int indexOfPpis = 0; indexOfPpis < spd.getPpiDeclarations().getEntryList()
                                                                               .size(); indexOfPpis++) {
                                        if (spd.getPpiDeclarations().getEntryList().get(indexOfPpis).getCName()
                                               .equals(arg0)) {
                                            pid.setDeclaredBy(packageId);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (pid.getDeclaredBy() == null) {
                                pid.setDeclaredBy(new PackageIdentification("", "", "", ""));
                            }

                            pv.addPpis(pid);
                        }
                    }
                }
            }
        }
        //
        // Go through all defined ppi to find which is not added yet.
        //
        for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
            PackageSurfaceArea spd = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage).getXmlSpd();
            PackageIdentification packageId = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage)
                                                                           .getId();
            if (spd.getPpiDeclarations() != null) {
                for (int indexOfPpis = 0; indexOfPpis < spd.getPpiDeclarations().getEntryList().size(); indexOfPpis++) {
                    boolean isFound = false;
                    PpiDeclarations.Entry e = spd.getPpiDeclarations().getEntryList().get(indexOfPpis);
                    for (int indexOfPv = 0; indexOfPv < pv.size(); indexOfPv++) {
                        if (e.getCName().equals(pv.getPpis(indexOfPv).getName())) {
                            isFound = true;
                            break;
                        }
                    }
                    if (!isFound) {
                        String arg0 = e.getCName();
                        String arg1 = "";
                        String arg2 = "";
                        String arg3 = "";
                        Vector<String> arg4 = Tools.convertListToVector(e.getSupArchList());
                        String arg5 = e.getHelpText();

                        pid = new PpisIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                        pid.setBelongModule(null);
                        pid.setDeclaredBy(packageId);
                        pv.addPpis(pid);
                    }
                }
            }
        }
        Sort.sortPpis(pv, DataType.SORT_TYPE_ASCENDING);
        return pv;
    }

    /**
     Re-org all ppi entries for find table
     
     @return
     
     **/
    public static Vector<PpiId> getAllPpisForFind() {
        Vector<PpiId> ppi = new Vector<PpiId>();
        PpisVector pv = Find.getAllPpisVector();
        boolean isAdded = false;
        boolean isProduced = false;

        //
        // Go through pv to add item as new format to ppi one by one
        //
        for (int indexOfPv = 0; indexOfPv < pv.size(); indexOfPv++) {
            isAdded = false;
            PpisIdentification pvId = pv.getPpis(indexOfPv);

            //
            // First check if produced or not
            //
            if (pvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_PRODUCED)
                || pvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_PRODUCED)) {
                isProduced = true;
            } else if (pvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_CONSUMED)
                       || pvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_CONSUMED)) {
                isProduced = false;
            }

            //
            // Get the sting "PackageName.ModuleName" 
            //
            String tmp = "";
            if (pvId.getBelongModule() != null) {
                tmp = pvId.getBelongModule().getPackageId().getName() + SEPERATOR + pvId.getBelongModule().getName();
            }

            //
            // Check if the item has been added in
            // If added, append package name and new module name
            // If not added, add a new one first
            //
            for (int indexOfPpi = 0; indexOfPpi < ppi.size(); indexOfPpi++) {
                PpiId ppiId = ppi.get(indexOfPpi);

                if (pvId.getName().equals(ppiId.getName())) {
                    if (isProduced) {
                        ppi.get(indexOfPpi).setProducedModules(ppiId.getProducedModules() + "<br>" + tmp);
                    } else if (!isProduced) {
                        ppi.get(indexOfPpi).setConsumedModules(ppiId.getConsumedModules() + "<br>" + tmp);
                    }
                    isAdded = true;
                    continue;
                }
            }

            //
            // Add a new one
            //
            if (!isAdded) {
                if (isProduced) {
                    ppi
                       .addElement(new PpiId(pvId.getName(), pvId.getType(), tmp, null, pvId.getDeclaredBy().getName()));
                } else if (!isProduced) {
                    ppi
                       .addElement(new PpiId(pvId.getName(), pvId.getType(), null, tmp, pvId.getDeclaredBy().getName()));
                }
            }
        }

        return ppi;
    }

    /**
     Get all protocol entries from workspace
     
     @return
     
     **/
    public static ProtocolsVector getAllProtocolsVector() {
        ProtocolsVector pv = new ProtocolsVector();
        ModuleSurfaceArea msa = null;
        ModuleIdentification mid = null;
        ProtocolsIdentification pid = null;

        //
        // Go through each module one by one
        //
        if (GlobalData.openingModuleList.size() > 0) {
            for (int indexOfModule = 0; indexOfModule < GlobalData.openingModuleList.size(); indexOfModule++) {
                msa = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getXmlMsa();
                mid = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getId();

                if (msa.getProtocols() != null) {
                    if (msa.getProtocols().getProtocolList().size() > 0) {
                        for (int index = 0; index < msa.getProtocols().getProtocolList().size(); index++) {
                            String arg0 = msa.getProtocols().getProtocolList().get(index).getProtocolCName();
                            String arg1 = DataType.PROTOCOL_TYPE_PROTOCOL;
                            String arg2 = null;
                            if (msa.getProtocols().getProtocolList().get(index).getUsage() != null) {
                                arg2 = msa.getProtocols().getProtocolList().get(index).getUsage().toString();
                            }

                            String arg3 = msa.getProtocols().getProtocolList().get(index).getFeatureFlag();
                            Vector<String> arg4 = Tools.convertListToVector(msa.getProtocols().getProtocolList()
                                                                               .get(index).getSupArchList());
                            String arg5 = msa.getProtocols().getProtocolList().get(index).getHelpText();

                            pid = new ProtocolsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                            pid.setBelongModule(mid);

                            //
                            // Find which package declares it
                            //
                            for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
                                PackageSurfaceArea spd = GlobalData.openingPackageList
                                                                                      .getOpeningPackageByIndex(
                                                                                                                indexOfPackage)
                                                                                      .getXmlSpd();
                                PackageIdentification packageId = GlobalData.openingPackageList
                                                                                               .getOpeningPackageByIndex(
                                                                                                                         indexOfPackage)
                                                                                               .getId();
                                if (spd.getProtocolDeclarations() != null) {
                                    for (int indexOfProtocols = 0; indexOfProtocols < spd.getProtocolDeclarations()
                                                                                         .getEntryList().size(); indexOfProtocols++) {
                                        if (spd.getProtocolDeclarations().getEntryList().get(indexOfProtocols)
                                               .getCName().equals(arg0)) {
                                            pid.setDeclaredBy(packageId);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (pid.getDeclaredBy() == null) {
                                pid.setDeclaredBy(new PackageIdentification("", "", "", ""));
                            }
                            pv.addProtocols(pid);
                        }
                    }
                    if (msa.getProtocols().getProtocolNotifyList().size() > 0) {
                        for (int index = 0; index < msa.getProtocols().getProtocolNotifyList().size(); index++) {
                            String arg0 = msa.getProtocols().getProtocolNotifyList().get(index)
                                             .getProtocolNotifyCName();
                            String arg1 = DataType.PPI_TYPE_PPI_NOTIFY;
                            String arg2 = null;
                            if (msa.getProtocols().getProtocolNotifyList().get(index).getUsage() != null) {
                                arg2 = msa.getProtocols().getProtocolNotifyList().get(index).getUsage().toString();
                            }

                            String arg3 = msa.getProtocols().getProtocolNotifyList().get(index).getFeatureFlag();
                            Vector<String> arg4 = Tools.convertListToVector(msa.getProtocols().getProtocolNotifyList()
                                                                               .get(index).getSupArchList());
                            String arg5 = msa.getProtocols().getProtocolNotifyList().get(index).getHelpText();

                            pid = new ProtocolsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                            pid.setBelongModule(mid);

                            //
                            // Find which package declares it
                            //
                            for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
                                PackageSurfaceArea spd = GlobalData.openingPackageList
                                                                                      .getOpeningPackageByIndex(
                                                                                                                indexOfPackage)
                                                                                      .getXmlSpd();
                                PackageIdentification packageId = GlobalData.openingPackageList
                                                                                               .getOpeningPackageByIndex(
                                                                                                                         indexOfPackage)
                                                                                               .getId();
                                if (spd.getProtocolDeclarations() != null) {
                                    for (int indexOfProtocols = 0; indexOfProtocols < spd.getProtocolDeclarations()
                                                                                         .getEntryList().size(); indexOfProtocols++) {
                                        if (spd.getProtocolDeclarations().getEntryList().get(indexOfProtocols)
                                               .getCName().equals(arg0)) {
                                            pid.setDeclaredBy(packageId);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (pid.getDeclaredBy() == null) {
                                pid.setDeclaredBy(new PackageIdentification("", "", "", ""));
                            }

                            pv.addProtocols(pid);
                        }
                    }
                }
            }
        }
        //
        // Go through all defined protocols to find which is not added yet.
        //
        for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
            PackageSurfaceArea spd = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage).getXmlSpd();
            PackageIdentification packageId = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage)
                                                                           .getId();
            if (spd.getProtocolDeclarations() != null) {
                for (int indexOfProtocols = 0; indexOfProtocols < spd.getProtocolDeclarations().getEntryList().size(); indexOfProtocols++) {
                    boolean isFound = false;
                    ProtocolDeclarations.Entry e = spd.getProtocolDeclarations().getEntryList().get(indexOfProtocols);
                    for (int indexOfPv = 0; indexOfPv < pv.size(); indexOfPv++) {
                        if (e.getCName().equals(pv.getProtocols(indexOfPv).getName())) {
                            isFound = true;
                            break;
                        }
                    }
                    if (!isFound) {
                        String arg0 = e.getCName();
                        String arg1 = "";
                        String arg2 = "";
                        String arg3 = "";
                        Vector<String> arg4 = Tools.convertListToVector(e.getSupArchList());
                        String arg5 = e.getHelpText();

                        pid = new ProtocolsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                        pid.setBelongModule(null);
                        pid.setDeclaredBy(packageId);
                        pv.addProtocols(pid);
                    }
                }
            }
        }
        Sort.sortProtocols(pv, DataType.SORT_TYPE_ASCENDING);
        return pv;
    }

    /**
     Re-org all protocol entries for find table
     
     @return
     
     **/
    public static Vector<ProtocolId> getAllProtocolsForFind() {
        Vector<ProtocolId> protocol = new Vector<ProtocolId>();
        ProtocolsVector pv = Find.getAllProtocolsVector();
        boolean isAdded = false;
        boolean isProduced = false;

        //
        // Go through pv to add item as new format to ppi one by one
        //
        for (int indexOfPv = 0; indexOfPv < pv.size(); indexOfPv++) {
            isAdded = false;
            ProtocolsIdentification pvId = pv.getProtocols(indexOfPv);

            //
            // First check if produced or not
            //
            if (pvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_PRODUCED)
                || pvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_PRODUCED)) {
                isProduced = true;
            } else if (pvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_CONSUMED)
                       || pvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_CONSUMED)) {
                isProduced = false;
            }

            //
            // Get the sting "PackageName.ModuleName" 
            //
            String tmp = "";
            if (pvId.getBelongModule() != null) {
                tmp = pvId.getBelongModule().getPackageId().getName() + SEPERATOR + pvId.getBelongModule().getName();
            }

            //
            // Check if the item has been added in
            // If added, append package name and new module name
            // If not added, add a new one first
            //
            for (int indexOfProtocol = 0; indexOfProtocol < protocol.size(); indexOfProtocol++) {
                ProtocolId protocolId = protocol.get(indexOfProtocol);

                if (pvId.getName().equals(protocolId.getName())) {
                    if (isProduced) {
                        protocol.get(indexOfProtocol)
                                .setProducedModules(protocolId.getProducedModules() + "<br>" + tmp);
                    } else if (!isProduced) {
                        protocol.get(indexOfProtocol)
                                .setConsumedModules(protocolId.getConsumedModules() + "<br>" + tmp);
                    }
                    isAdded = true;
                    continue;
                }
            }

            //
            // Add a new one
            //
            if (!isAdded) {
                if (isProduced) {
                    protocol.addElement(new ProtocolId(pvId.getName(), pvId.getType(), tmp, null, pvId.getDeclaredBy()
                                                                                                      .getName()));
                } else if (!isProduced) {
                    protocol.addElement(new ProtocolId(pvId.getName(), pvId.getType(), null, tmp, pvId.getDeclaredBy()
                                                                                                      .getName()));
                }
            }
        }

        return protocol;
    }

    /**
     Get all protocol entries from workspace
     
     @return
     
     **/
    public static GuidsVector getAllGuidsVector() {
        GuidsVector gv = new GuidsVector();
        ModuleSurfaceArea msa = null;
        ModuleIdentification mid = null;
        GuidsIdentification gid = null;

        //
        // Go through each module one by one
        //
        if (GlobalData.openingModuleList.size() > 0) {
            for (int indexOfModule = 0; indexOfModule < GlobalData.openingModuleList.size(); indexOfModule++) {
                msa = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getXmlMsa();
                mid = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getId();

                if (msa.getGuids() != null) {
                    if (msa.getGuids().getGuidCNamesList().size() > 0) {
                        for (int index = 0; index < msa.getGuids().getGuidCNamesList().size(); index++) {
                            String arg0 = msa.getGuids().getGuidCNamesList().get(index).getGuidCName();
                            String arg1 = null;
                            if (msa.getGuids().getGuidCNamesList().get(index).getUsage() != null) {
                                arg1 = msa.getGuids().getGuidCNamesList().get(index).getUsage().toString();
                            }

                            String arg2 = msa.getGuids().getGuidCNamesList().get(index).getFeatureFlag();
                            Vector<String> arg3 = Tools.convertListToVector(msa.getGuids().getGuidCNamesList()
                                                                               .get(index).getSupArchList());
                            String arg4 = msa.getGuids().getGuidCNamesList().get(index).getHelpText();

                            gid = new GuidsIdentification(arg0, arg1, arg2, arg3, arg4);
                            gid.setBelongModule(mid);

                            //
                            // Find which package declares it
                            //
                            for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
                                PackageSurfaceArea spd = GlobalData.openingPackageList
                                                                                      .getOpeningPackageByIndex(
                                                                                                                indexOfPackage)
                                                                                      .getXmlSpd();
                                PackageIdentification packageId = GlobalData.openingPackageList
                                                                                               .getOpeningPackageByIndex(
                                                                                                                         indexOfPackage)
                                                                                               .getId();
                                if (spd.getGuidDeclarations() != null) {
                                    for (int indexOfGuids = 0; indexOfGuids < spd.getGuidDeclarations().getEntryList()
                                                                                 .size(); indexOfGuids++) {
                                        if (spd.getGuidDeclarations().getEntryList().get(indexOfGuids).getCName()
                                               .equals(arg0)) {
                                            gid.setDeclaredBy(packageId);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (gid.getDeclaredBy() == null) {
                                gid.setDeclaredBy(new PackageIdentification("", "", "", ""));
                            }

                            gv.addGuids(gid);
                        }
                    }
                }
            }
        }
        //
        // Go through all defined guids to find which is not added yet.
        //
        for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
            PackageSurfaceArea spd = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage).getXmlSpd();
            PackageIdentification packageId = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage)
                                                                           .getId();
            if (spd.getGuidDeclarations() != null) {
                for (int indexOfGuids = 0; indexOfGuids < spd.getGuidDeclarations().getEntryList().size(); indexOfGuids++) {
                    boolean isFound = false;
                    GuidDeclarations.Entry e = spd.getGuidDeclarations().getEntryList().get(indexOfGuids);
                    for (int indexOfGv = 0; indexOfGv < gv.size(); indexOfGv++) {
                        if (e.getCName().equals(gv.getGuids(indexOfGv).getName())) {
                            isFound = true;
                            break;
                        }
                    }
                    if (!isFound) {
                        String arg0 = e.getCName();
                        String arg1 = "";
                        String arg2 = "";
                        Vector<String> arg3 = Tools.convertListToVector(e.getSupArchList());
                        String arg4 = e.getHelpText();

                        gid = new GuidsIdentification(arg0, arg1, arg2, arg3, arg4);
                        gid.setBelongModule(null);
                        gid.setDeclaredBy(packageId);
                        gv.addGuids(gid);
                    }
                }
            }
        }
        Sort.sortGuids(gv, DataType.SORT_TYPE_ASCENDING);
        return gv;
    }

    /**
     Re-org all guid entries for find table
     
     @return
     
     **/
    public static Vector<GuidId> getAllGuidsForFind() {
        Vector<GuidId> guid = new Vector<GuidId>();
        GuidsVector gv = Find.getAllGuidsVector();
        boolean isAdded = false;
        boolean isProduced = false;

        //
        // Go through pv to add item as new format to ppi one by one
        //
        for (int indexOfGv = 0; indexOfGv < gv.size(); indexOfGv++) {
            isAdded = false;
            GuidsIdentification gvId = gv.getGuids(indexOfGv);

            //
            // First check if produced or not
            //
            if (gvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_PRODUCED)
                || gvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_PRODUCED)) {
                isProduced = true;
            } else if (gvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_CONSUMED)
                       || gvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_CONSUMED)) {
                isProduced = false;
            }

            //
            // Get the sting "PackageName.ModuleName" 
            //
            String tmp = "";
            if (gvId.getBelongModule() != null) {
                tmp = gvId.getBelongModule().getPackageId().getName() + SEPERATOR + gvId.getBelongModule().getName();
            }

            //
            // Check if the item has been added in
            // If added, append package name and new module name
            // If not added, add a new one first
            //
            for (int indexOfGuid = 0; indexOfGuid < guid.size(); indexOfGuid++) {
                GuidId guidId = guid.get(indexOfGuid);

                if (gvId.getName().equals(guidId.getName())) {
                    if (isProduced) {
                        guid.get(indexOfGuid).setProducedModules(guidId.getProducedModules() + "<br>" + tmp);
                    } else if (!isProduced) {
                        guid.get(indexOfGuid).setConsumedModules(guidId.getConsumedModules() + "<br>" + tmp);
                    }
                    isAdded = true;
                    continue;
                }
            }

            //
            // Add a new one
            //
            if (!isAdded) {
                if (isProduced) {
                    guid.addElement(new GuidId(gvId.getName(), "GUID", tmp, null, gvId.getDeclaredBy().getName()));
                } else if (!isProduced) {
                    guid.addElement(new GuidId(gvId.getName(), "GUID", null, tmp, gvId.getDeclaredBy().getName()));
                }
            }
        }

        return guid;
    }

    /**
     Get all pcd coded entries from workspace
     
     @return
     
     **/
    public static PcdCodedVector getAllPcdCodedVector() {
        PcdCodedVector pv = new PcdCodedVector();
        ModuleSurfaceArea msa = null;
        ModuleIdentification mid = null;
        PcdCodedIdentification pid = null;

        //
        // Go through each module one by one
        //
        if (GlobalData.openingModuleList.size() > 0) {
            for (int indexOfModule = 0; indexOfModule < GlobalData.openingModuleList.size(); indexOfModule++) {
                msa = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getXmlMsa();
                mid = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getId();

                if (msa.getPcdCoded() != null) {
                    if (msa.getPcdCoded().getPcdEntryList().size() > 0) {
                        for (int index = 0; index < msa.getPcdCoded().getPcdEntryList().size(); index++) {

                            String arg0 = msa.getPcdCoded().getPcdEntryList().get(index).getCName();
                            String arg1 = msa.getPcdCoded().getPcdEntryList().get(index).getTokenSpaceGuidCName();

                            String arg2 = msa.getPcdCoded().getPcdEntryList().get(index).getFeatureFlag();
                            Vector<String> arg3 = Tools.convertListToVector(msa.getPcdCoded().getPcdEntryList()
                                                                               .get(index).getSupArchList());

                            String arg4 = msa.getPcdCoded().getPcdEntryList().get(index).getDefaultValue();
                            String arg5 = msa.getPcdCoded().getPcdEntryList().get(index).getHelpText();
                            String arg6 = null;
                            if (msa.getPcdCoded().getPcdEntryList().get(index).getPcdItemType() != null) {
                                arg6 = msa.getPcdCoded().getPcdEntryList().get(index).getPcdItemType().toString();
                            }
                            String arg7 = null;
                            if (msa.getPcdCoded().getPcdEntryList().get(index).getUsage() != null) {
                                arg7 = msa.getPcdCoded().getPcdEntryList().get(index).getUsage().toString();
                            }
                            pid = new PcdCodedIdentification(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
                            pid.setBelongModule(mid);

                            //
                            // Find which package declares it
                            //
                            for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
                                PackageSurfaceArea spd = GlobalData.openingPackageList
                                                                                      .getOpeningPackageByIndex(
                                                                                                                indexOfPackage)
                                                                                      .getXmlSpd();
                                PackageIdentification packageId = GlobalData.openingPackageList
                                                                                               .getOpeningPackageByIndex(
                                                                                                                         indexOfPackage)
                                                                                               .getId();
                                if (spd.getPcdDeclarations() != null) {
                                    for (int indexOfPcds = 0; indexOfPcds < spd.getPcdDeclarations().getPcdEntryList()
                                                                               .size(); indexOfPcds++) {
                                        PcdEntry pcdEntry = spd.getPcdDeclarations().getPcdEntryList().get(indexOfPcds);
                                        if (pcdEntry.getCName().equals(arg0)) {
                                            pid.setTokenSpaceGuidCName(pcdEntry.getTokenSpaceGuidCName());
                                            pid.setToken(pcdEntry.getToken().toString());
                                            pid.setDatumType(pcdEntry.getDatumType().toString());
                                            pid.setValue(pcdEntry.getDefaultValue());
                                            pid.setUsage(Tools.convertListToString(pcdEntry.getValidUsage()));
                                            pid.setHelp(pcdEntry.getHelpText());
                                            pid.setDeclaredBy(packageId);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (pid.getDeclaredBy() == null) {
                                pid.setDeclaredBy(new PackageIdentification("", "", "", ""));
                            }

                            pv.addPcdCoded(pid);
                        }
                    }
                }
            }
        }
        //
        // Go through all defined pcds to find which is not added yet.
        //
        for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
            PackageSurfaceArea spd = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage).getXmlSpd();
            PackageIdentification packageId = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage)
                                                                           .getId();
            if (spd.getPcdDeclarations() != null) {
                for (int indexOfPcds = 0; indexOfPcds < spd.getPcdDeclarations().getPcdEntryList().size(); indexOfPcds++) {
                    boolean isFound = false;
                    PcdEntry e = spd.getPcdDeclarations().getPcdEntryList().get(indexOfPcds);
                    for (int indexOfPv = 0; indexOfPv < pv.size(); indexOfPv++) {
                        if (e.getCName().equals(pv.getPcdCoded(indexOfPv).getName())) {
                            isFound = true;
                            break;
                        }
                    }
                    if (!isFound) {
                        pid = new PcdCodedIdentification("", "", "", null, "", "", null, null);
                        pid.setName(e.getCName());
                        pid.setTokenSpaceGuidCName(e.getTokenSpaceGuidCName());
                        pid.setToken(e.getToken().toString());
                        pid.setDatumType(e.getDatumType().toString());
                        pid.setValue(e.getDefaultValue());
                        pid.setUsage(Tools.convertListToString(e.getValidUsage()));
                        pid.setHelp(e.getHelpText());
                        pid.setDeclaredBy(packageId);

                        pid.setBelongModule(null);
                        pv.addPcdCoded(pid);
                    }
                }
            }
        }
        Sort.sortPcdCodeds(pv, DataType.SORT_TYPE_ASCENDING);
        return pv;
    }

    /**
     
     @param pv
     @return
     
     **/
    public static Vector<PcdFindResultId> getAllPcdCodedForFind(PcdCodedVector pv) {
        Vector<PcdFindResultId> pcd = new Vector<PcdFindResultId>();
        boolean isAdded = false;
        boolean isProduced = false;

        //
        // Go through pv to add item as new format to ppi one by one
        //
        for (int indexOfPv = 0; indexOfPv < pv.size(); indexOfPv++) {
            isAdded = false;
            PcdCodedIdentification pvId = pv.getPcdCoded(indexOfPv);

            //
            // First check if produced or not
            //
            if (pvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_PRODUCED)
                || pvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_PRODUCED)) {
                isProduced = true;
            } else if (pvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_CONSUMED)
                       || pvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_CONSUMED)) {
                isProduced = false;
            }

            //
            // Check if the item has been added in
            // If added, append package name and new module name
            // If not added, add a new one first
            //
            for (int indexOfGuid = 0; indexOfGuid < pcd.size(); indexOfGuid++) {
                PcdFindResultId pcdId = pcd.get(indexOfGuid);

                if (pvId.getName().equals(pcdId.getName())) {
                    if (isProduced) {
                        pcd.get(indexOfGuid).addProducedModules(pvId.getBelongModule());
                    } else if (!isProduced) {
                        pcd.get(indexOfGuid).addConsumedModules(pvId.getBelongModule());
                    }
                    isAdded = true;
                    continue;
                }
            }

            //
            // Add a new one
            //
            if (!isAdded) {
                PcdFindResultId pcdId = new PcdFindResultId(pvId.getName(), "PCD", pvId.getSupArchList(),
                                                            pvId.getHelp(), null, pvId.getDeclaredBy());
                pcdId.setTokenSpaceGuidCName(pvId.getTokenSpaceGuidCName());
                pcdId.setToken(pvId.getToken());
                pcdId.setDatumType(pvId.getDatumType());
                pcdId.setValue(pvId.getValue());
                pcdId.setUsage(pvId.getUsage());

                pcd.addElement(pcdId);
            }
        }

        return pcd;
    }

    /**
     Get all library class entries from workspace
     
     @return
     
     **/
    public static LibraryClassVector getAllLibraryClassVector() {
        LibraryClassVector lcv = new LibraryClassVector();
        ModuleSurfaceArea msa = null;
        ModuleIdentification mid = null;
        LibraryClassIdentification lcid = null;

        //
        // Go through each module one by one
        //
        if (GlobalData.openingModuleList.size() > 0) {
            for (int indexOfModule = 0; indexOfModule < GlobalData.openingModuleList.size(); indexOfModule++) {
                msa = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getXmlMsa();
                mid = GlobalData.openingModuleList.getOpeningModuleByIndex(indexOfModule).getId();

                if (msa.getLibraryClassDefinitions() != null) {
                    LibraryClassDefinitions lcd = msa.getLibraryClassDefinitions();
                    if (lcd.getLibraryClassList().size() > 0) {
                        for (int index = 0; index < lcd.getLibraryClassList().size(); index++) {
                            String name = lcd.getLibraryClassList().get(index).getKeyword();
                            String usage = null;
                            if (lcd.getLibraryClassList().get(index).getUsage() != null) {
                                usage = lcd.getLibraryClassList().get(index).getUsage().toString();
                            }
                            String version = lcd.getLibraryClassList().get(index).getRecommendedInstanceVersion();
                            String guid = lcd.getLibraryClassList().get(index).getRecommendedInstanceGuid();
                            String featureFlag = lcd.getLibraryClassList().get(index).getFeatureFlag();
                            Vector<String> arch = Tools.convertListToVector(lcd.getLibraryClassList().get(index)
                                                                               .getSupArchList());
                            Vector<String> module = Tools.convertListToVector(lcd.getLibraryClassList().get(index)
                                                                                 .getSupModuleList());
                            String help = lcd.getLibraryClassList().get(index).getHelpText();
                            lcid = new LibraryClassIdentification(name, usage, version, guid, arch, featureFlag,
                                                                  module, help);
                            lcid.setBelongModule(mid);

                            //
                            // Find which package declares it
                            //
                            for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
                                PackageSurfaceArea spd = GlobalData.openingPackageList
                                                                                      .getOpeningPackageByIndex(
                                                                                                                indexOfPackage)
                                                                                      .getXmlSpd();
                                PackageIdentification packageId = GlobalData.openingPackageList
                                                                                               .getOpeningPackageByIndex(
                                                                                                                         indexOfPackage)
                                                                                               .getId();
                                if (spd.getLibraryClassDeclarations() != null) {
                                    for (int indexOfLibraryClass = 0; indexOfLibraryClass < spd
                                                                                               .getLibraryClassDeclarations()
                                                                                               .getLibraryClassList()
                                                                                               .size(); indexOfLibraryClass++) {
                                        LibraryClass lc = spd.getLibraryClassDeclarations().getLibraryClassList()
                                                             .get(indexOfLibraryClass);
                                        if (lc.getName().equals(name)) {
                                            lcid.setSupArchList(Tools.convertListToVector(lc.getSupArchList()));
                                            lcid.setSupModuleList(Tools.convertListToVector(lc.getSupModuleList()));
                                            lcid.setHelp(lc.getHelpText());
                                            lcid.setDeclaredBy(packageId);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (lcid.getDeclaredBy() == null) {
                                lcid.setDeclaredBy(new PackageIdentification("", "", "", ""));
                            }

                            lcv.addLibraryClass(lcid);
                        }
                    }
                }
            }
        }
        //
        // Go through all defined pcds to find which is not added yet.
        //
        for (int indexOfPackage = 0; indexOfPackage < GlobalData.openingPackageList.size(); indexOfPackage++) {
            PackageSurfaceArea spd = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage).getXmlSpd();
            PackageIdentification packageId = GlobalData.openingPackageList.getOpeningPackageByIndex(indexOfPackage)
                                                                           .getId();
            if (spd.getLibraryClassDeclarations() != null) {
                for (int indexOfLibraryClass = 0; indexOfLibraryClass < spd.getLibraryClassDeclarations()
                                                                           .getLibraryClassList().size(); indexOfLibraryClass++) {
                    boolean isFound = false;
                    LibraryClass lc = spd.getLibraryClassDeclarations().getLibraryClassList().get(indexOfLibraryClass);
                    for (int indexOfLcv = 0; indexOfLcv < lcv.size(); indexOfLcv++) {
                        if (lc.getName().equals(lcv.getLibraryClass(indexOfLcv).getLibraryClassName())) {
                            isFound = true;
                            break;
                        }
                    }
                    if (!isFound) {
                        lcid = new LibraryClassIdentification("", null, "", "", null, "", null, "");
                        lcid.setLibraryClassName(lc.getName());
                        lcid.setSupArchList(Tools.convertListToVector(lc.getSupArchList()));
                        lcid.setSupModuleList(Tools.convertListToVector(lc.getSupModuleList()));
                        lcid.setHelp(lc.getHelpText());
                        lcid.setDeclaredBy(packageId);

                        lcid.setBelongModule(null);
                        lcv.addLibraryClass(lcid);
                    }
                }
            }
        }
        Sort.sortLibraryClass(lcv, DataType.SORT_TYPE_ASCENDING);
        return lcv;
    }

    /**
     Re-org all guid entries for find table
     
     @return
     
     **/
    public static Vector<FindResultId> getAllLibraryClassForFind(LibraryClassVector lcv) {
        Vector<FindResultId> libraryClass = new Vector<FindResultId>();
        boolean isAdded = false;
        boolean isProduced = false;

        //
        // Go through pv to add item as new format to ppi one by one
        //
        for (int indexOfLcv = 0; indexOfLcv < lcv.size(); indexOfLcv++) {
            isAdded = false;
            LibraryClassIdentification lcvId = lcv.getLibraryClass(indexOfLcv);

            //
            // First check if produced or not
            //
            if (lcvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_PRODUCED)
                || lcvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_PRODUCED)) {
                isProduced = true;
            } else if (lcvId.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_CONSUMED)
                       || lcvId.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_CONSUMED)) {
                isProduced = false;
            }

            //
            // Check if the item has been added in
            // If added, append package name and new module name
            // If not added, add a new one first
            //
            for (int indexOfGuid = 0; indexOfGuid < libraryClass.size(); indexOfGuid++) {
                FindResultId frId = libraryClass.get(indexOfGuid);

                if (lcvId.getLibraryClassName().equals(frId.getName())) {
                    if (isProduced) {
                        libraryClass.get(indexOfGuid).addProducedModules(lcvId.getBelongModule());
                    } else if (!isProduced) {
                        libraryClass.get(indexOfGuid).addConsumedModules(lcvId.getBelongModule());
                    }
                    isAdded = true;
                    continue;
                }
            }

            //
            // Add a new one
            //
            if (!isAdded) {
                libraryClass.addElement(new FindResultId(lcvId.getLibraryClassName(), "Library Class",
                                                         lcvId.getSupArchList(), lcvId.getHelp(),
                                                         lcvId.getSupModuleList(), lcvId.getDeclaredBy()));
            }
        }

        return libraryClass;
    }
}
