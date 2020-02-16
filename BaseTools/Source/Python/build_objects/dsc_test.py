# @file dsc_test.py
# Tests for the data model for the EDK II DSC
#
# Copyright (c) Microsoft Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import unittest
from edk2toollib.uefi.edk2.build_objects.dsc import dsc
from edk2toollib.uefi.edk2.build_objects.dsc import library_class
from edk2toollib.uefi.edk2.build_objects.dsc import component
from edk2toollib.uefi.edk2.build_objects.dsc import definition
from edk2toollib.uefi.edk2.build_objects.dsc import dsc_buildoption_section_type
from edk2toollib.uefi.edk2.build_objects.dsc import dsc_pcd_section_type
from edk2toollib.uefi.edk2.build_objects.dsc import dsc_section_type


class TestDscObject(unittest.TestCase):

    def test_null_creation(self):
        d = dsc()
        self.assertNotEqual(d, None)

    def test_dsc_multple_defines(self):
        # When we add an object, it should overwrite the previous one
        d = TestDscObject.create_dsc_object()
        d.defines.add(definition("PLATFORM_NAME", "TEST2"))
        for define in d.defines:
            if define.name == "PLATFORM_NAME":  # check to make sure it matches
                self.assertEqual(define.value, "TEST2")

    def test_dsc_multple_library_classes(self):
        d = dsc()
        # When we add an object, it should overwrite the previous one
        common_section = dsc_section_type()
        d.library_classes[common_section].add(library_class("TEST", "BOB.inf"))
        self.assertEqual(len(d.library_classes[common_section]), 1)
        # we should override the previous one
        d.library_classes[common_section].add(library_class("TEST", "BOB2.inf"))
        self.assertEqual(len(d.library_classes[common_section]), 1)
        for lib in d.library_classes[common_section]:
            self.assertEqual(lib.inf, "BOB2.inf")  # make sure we overrode it
        self.assertEqual(len(d.library_classes[common_section]), 1)

        # make sure we can add a library to a different section and that
        IA32_section = dsc_section_type(arch="IA32")
        self.assertEqual(len(d.library_classes[IA32_section]), 0)
        d.library_classes[IA32_section].add(library_class("NULL", "BOB1.inf"))
        self.assertEqual(len(d.library_classes[IA32_section]), 1)
        d.library_classes[IA32_section].add(library_class("NULL", "BOB2.inf"))
        self.assertEqual(len(d.library_classes[IA32_section]), 2)

    def test_get_library_classes(self):
        ''' This serves more as an example of how to walk the DSC to get a library class for a componenet '''
        pass

    def test_put_in_bad_things(self):
        d = dsc()
        # make sure we can't add stuff to d.defines
        with self.assertRaises(ValueError):
            d.defines.add(library_class("NULL", "TEST.inf"))
        # make sure we can't add stuff to skus
        with self.assertRaises(ValueError):
            d.skus.add(library_class("TEST", "TEST.inf"))
        # make sure we can't add stuff to skus
        with self.assertRaises(ValueError):
            d.default_stores.add(component("TEST", "TEST.inf"))

        common_section = dsc_section_type()
        build_opt_section = dsc_buildoption_section_type()
        pcd_section = dsc_pcd_section_type("FEATUREFLAG")

        # now to check the build options
        d.build_options[build_opt_section] = set()
        with self.assertRaises(ValueError):
            d.build_options[pcd_section] = set()
        with self.assertRaises(ValueError):
            d.build_options[common_section] = set()
        with self.assertRaises(ValueError):
            d.build_options[build_opt_section].add(library_class("TEST", "TEST.inf"))
        with self.assertRaises(ValueError):  # TODO: once the adding logic is implemented, this will be need to redone
            d.build_options[build_opt_section] = set()

        # now to check the pcds
        d.pcds[pcd_section] = set()
        with self.assertRaises(ValueError):
            d.pcds[build_opt_section] = set()
        with self.assertRaises(ValueError):
            d.pcds[pcd_section].add(library_class("TEST", "TEST.inf"))
        with self.assertRaises(ValueError):  # TODO: once the adding logic is implemented, this will be need to redone
            d.pcds[pcd_section] = set()

        # now to check the library classes
        d.library_classes[common_section] = set()
        with self.assertRaises(ValueError):
            d.library_classes[build_opt_section] = set()
        with self.assertRaises(ValueError):
            d.library_classes[common_section].add(component("TEST.inf"))
        with self.assertRaises(ValueError):  # TODO: once the adding logic is implemented, this will be need to redone
            d.library_classes[common_section] = set()

        # now to check the components
        d.components[common_section] = set()
        with self.assertRaises(ValueError):
            d.components[build_opt_section] = set()
        with self.assertRaises(ValueError):
            d.components[common_section].add(library_class("TEST", "TEST.inf"))
        with self.assertRaises(ValueError):  # TODO: once the adding logic is implemented, this will be need to redone
            d.components[common_section] = set()

    @staticmethod
    def create_dsc_object():
        # Normally we would just read the dsc object
        d = dsc()
        # first add the defines
        d.defines.add(definition("PLATFORM_NAME", "TEST"))
        d.defines.add(definition("PLATFORM_GUID", "EB216561-961F-47EE-9EF9-CA426EF547C2"))
        d.defines.add(definition("OUTPUT_DIRECTORY", "Build/TEST"))
        d.defines.add(definition("SUPPORTED_ARCHITECTURES", "IA32 X64 AARCH64"))

        # Next add some library classes
        default_section = dsc_section_type()
        d.library_classes[default_section].add(library_class("NULL", "BOB.inf"))

        # Next add a component
        return d
