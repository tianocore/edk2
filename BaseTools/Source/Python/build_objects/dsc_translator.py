# @file dsc_translator
# Translates a DSC object into a file
# Copyright (c) Microsoft Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
import os
import logging
from edk2toollib.uefi.edk2.build_objects.dsc import dsc
from edk2toollib.uefi.edk2.build_objects.dsc import sku_id
from edk2toollib.uefi.edk2.build_objects.dsc import dsc_set
from edk2toollib.uefi.edk2.build_objects.dsc import library_class
from edk2toollib.uefi.edk2.build_objects.dsc import definition
from edk2toollib.uefi.edk2.build_objects.dsc import component
from edk2toollib.uefi.edk2.build_objects.dsc import pcd
from edk2toollib.uefi.edk2.build_objects.dsc import pcd_typed
from edk2toollib.uefi.edk2.build_objects.dsc import pcd_variable
from edk2toollib.uefi.edk2.build_objects.dsc import build_option


class DscTranslator():

    @classmethod
    def dsc_to_file(cls, dsc_obj, filepath):
        file_path = os.path.abspath(filepath)
        f = open(file_path, "w")
        lines = cls._GetDscLinesFromDscObj(dsc_obj)
        for l in lines:
            f.write(l + "\n")
        f.close()

    @classmethod
    def _GetDscLinesFromDscObj(cls, obj, depth=0) -> list:
        ''' gets the DSC strings for an data model objects '''
        lines = []
        depth_pad = ''.ljust(depth)
        org_depth = depth
        depth += 2

        if type(obj) is list or type(obj) is set or type(obj) is dsc_set:
            for item in obj:
                lines += cls._GetDscLinesFromDscObj(item, org_depth)
        elif type(obj) is dsc:
            lines.append(f"{depth_pad}[Defines]")
            lines += cls._GetDscLinesFromDscObj(obj.defines, depth)

            # Second do the Skus
            lines.append(f"{depth_pad}[SkuIds]")
            for x in obj.skus:
                lines += cls._GetDscLinesFromDscObj(x, depth)

            # Third, library classes
            for header, x in obj.library_classes.items():
                lines.append(f"{depth_pad}[LibraryClasses{header}]")
                lines += cls._GetDscLinesFromDscObj(x, depth)

            # Next do the components
            for header, x in obj.components.items():
                lines.append(f"{depth_pad}[Components{header}]")
                lines += cls._GetDscLinesFromDscObj(x, depth)

            # Then PCD's
            for header, x in obj.pcds.items():
                lines.append(f"{depth_pad}[{header}]")
                lines += cls._GetDscLinesFromDscObj(x, depth)

            # Then Build Options
            print(obj.build_options.items())
            for header, x in obj.build_options.items():
                lines.append(f"{depth_pad}[BuildOptions{header}]")
                lines += cls._GetDscLinesFromDscObj(x, depth)

        elif type(obj) is sku_id:
            lines.append(f"{depth_pad}{obj.id}|{obj.name}|{obj.parent}")
        elif type(obj) is library_class:
            lines.append(f"{depth_pad}{obj.libraryclass}|{obj.inf}")
        elif type(obj) is definition:
            def_str = f"{obj.name} =\t{obj.value}"
            if obj.local:
                def_str = "DEFINE " + def_str
            lines.append(depth_pad + def_str)

        elif type(obj) is component:
            lines += cls._FormatComponent(obj, depth)

        elif type(obj) is pcd:
            lines.append(f"{depth_pad}{obj.namespace}.{obj.name}|{obj.value}")

        elif type(obj) is pcd_typed:
            pcd_str = f"{depth_pad}{obj.namespace}.{obj.name}|{obj.value}|{obj.datum_type}"
            if obj.max_size > 0:
                pcd_str += f"|{obj.max_size}"
            lines.append(pcd_str)

        elif type(obj) is pcd_variable:
            pcd_name = f"{depth_pad}{obj.namespace}.{obj.name}|{obj.var_name}"
            if obj.default is None:
                lines.append(f"{pcd_name}|{obj.var_guid}|{obj.var_offset}")
            elif len(obj.attributes) == 0:
                lines.append(
                    f"{pcd_name}|{obj.var_guid}|{obj.var_offset}|{obj.default}")
            else:
                attr = ", ".join(obj.attributes)
                lines.append(
                    f"{pcd_name}|{obj.var_guid}|{obj.var_offset}|{obj.default}|{attr}")

        elif type(obj) is build_option:
            rep = depth_pad if obj.family is None else f"{depth_pad}{obj.family}:"
            rep += "_".join((obj.target, obj.tagname, obj.arch, obj.tool_code, obj.attribute))
            rep += f"= {obj.data}"
            lines.append(rep)
        else:
            logging.warning(f"UNKNOWN OBJECT {obj}")
        return lines

    @classmethod
    def _FormatComponent(cls, comp, depth=0):
        has_subsection = len(comp.pcds) > 0 or len(comp.defines) > 0 or len(
            comp.build_options) > 0 or len(comp.library_classes) > 0
        depth_pad = ''.ljust(depth)
        if not has_subsection:
            return [f"{depth_pad}{comp.inf}", ]
        lines = []
        org_depth_pad = depth_pad
        depth_pad += "   "  # add two more onto our pad
        depth += 4
        lines.append(f"{org_depth_pad}{comp.inf} {{")
        if len(comp.pcds) > 0:
            for section, pcds in comp.pcds.items():
                lines.append(f"{depth_pad}<{section}>")
                lines += cls._GetDscLinesFromDscObj(pcds, depth)
            pass
        if len(comp.library_classes) > 0:
            lines.append(f"{depth_pad}<LibraryClasses>")
            lines += cls._GetDscLinesFromDscObj(comp.library_classes, depth)
        if len(comp.defines) > 0:
            lines.append(f"{depth_pad}<Defines>")
            lines += cls._GetDscLinesFromDscObj(comp.defines, depth)
        if len(comp.build_options) > 0:
            lines.append(f"{depth_pad}<BuildOptions>")
            lines += cls._GetDscLinesFromDscObj(comp.build_options, depth)
        lines.append(f"{org_depth_pad}}}")
        return lines
