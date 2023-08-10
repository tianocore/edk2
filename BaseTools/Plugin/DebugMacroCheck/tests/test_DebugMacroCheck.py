# @file test_DebugMacroCheck.py
#
# Contains unit tests for the DebugMacroCheck build plugin.
#
# An example of running these tests from the root of the workspace:
#   python -m unittest discover -s ./BaseTools/Plugin/DebugMacroCheck/tests -v
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import inspect
import pathlib
import sys
import unittest

# Import the build plugin
test_file = pathlib.Path(__file__)
sys.path.append(str(test_file.parent.parent))

# flake8 (E402): Ignore flake8 module level import not at top of file
import DebugMacroCheck                          # noqa: E402

from os import linesep                          # noqa: E402
from tests import DebugMacroDataSet             # noqa: E402
from tests import MacroTest                     # noqa: E402
from typing import Callable, Tuple              # noqa: E402


#
# This metaclass is provided to dynamically produce test case container
# classes. The main purpose of this approach is to:
#   1. Allow categories of test cases to be defined (test container classes)
#   2. Allow test cases to automatically (dynamically) be assigned to their
#      corresponding test container class when new test data is defined.
#
#      The idea being that infrastructure and test data are separated. Adding
#      / removing / modifying test data does not require an infrastructure
#      change (unless new categories are defined).
#   3. To work with the unittest discovery algorithm and VS Code Test Explorer.
#
# Notes:
#  - (1) can roughly be achieved with unittest test suites. In another
#    implementation approach, this solution was tested with relatively minor
#    modifications to use test suites. However, this became a bit overly
#    complicated with the dynamic test case method generation and did not
#    work as well with VS Code Test Explorer.
#  - For (2) and (3), particularly for VS Code Test Explorer to work, the
#    dynamic population of the container class namespace needed to happen prior
#    to class object creation. That is why the metaclass assigns test methods
#    to the new classes based upon the test category specified in the
#    corresponding data class.
#  - This could have been simplified a bit by either using one test case
#    container class and/or testing data in a single, monolithic test function
#    that iterates over the data set. However, the dynamic hierarchy greatly
#    helps organize test results and reporting. The infrastructure though
#    inheriting some complexity to support it, should not need to change (much)
#    as the data set expands.
#  - Test case categories (container classes) are derived from the overall
#    type of macro conditions under test.
#
#  - This implementation assumes unittest will discover test cases
#    (classes derived from unittest.TestCase) with the name pattern "Test_*"
#    and test functions with the name pattern "test_x". Individual tests are
#    dynamically numbered monotonically within a category.
#  - The final test case description is also able to return fairly clean
#    context information.
#
class Meta_TestDebugMacroCheck(type):
    """
    Metaclass for debug macro test case class factory.
    """
    @classmethod
    def __prepare__(mcls, name, bases, **kwargs):
        """Returns the test case namespace for this class."""
        candidate_macros, cls_ns, cnt = [], {}, 0

        if "category" in kwargs.keys():
            candidate_macros = [m for m in DebugMacroDataSet.DEBUG_MACROS if
                                m.category == kwargs["category"]]
        else:
            candidate_macros = DebugMacroDataSet.DEBUG_MACROS

        for cnt, macro_test in enumerate(candidate_macros):
            f_name = f'test_{macro_test.category}_{cnt}'
            t_desc = f'{macro_test!s}'
            cls_ns[f_name] = mcls.build_macro_test(macro_test, t_desc)
        return cls_ns

    def __new__(mcls, name, bases, ns, **kwargs):
        """Defined to prevent variable args from bubbling to the base class."""
        return super().__new__(mcls, name, bases, ns)

    def __init__(mcls, name, bases, ns, **kwargs):
        """Defined to prevent variable args from bubbling to the base class."""
        return super().__init__(name, bases, ns)

    @classmethod
    def build_macro_test(cls, macro_test: MacroTest.MacroTest,
                         test_desc: str) -> Callable[[None], None]:
        """Returns a test function for this macro test data."

        Args:
            macro_test (MacroTest.MacroTest): The macro test class.

            test_desc (str): A test description string.

        Returns:
            Callable[[None], None]: A test case function.
        """
        def test_func(self):
            act_result = cls.check_regex(macro_test.macro)
            self.assertCountEqual(
                act_result,
                macro_test.result,
                test_desc + f'{linesep}'.join(
                    ["", f"Actual Result:    {act_result}", "=" * 80, ""]))

        return test_func

    @classmethod
    def check_regex(cls, source_str: str) -> Tuple[int, int, int]:
        """Returns the plugin result for the given macro string.

        Args:
            source_str (str): A string containing debug macros.

        Returns:
            Tuple[int, int, int]: A tuple of the number of formatting errors,
            number of print specifiers, and number of arguments for the macros
            given.
        """
        return DebugMacroCheck.check_debug_macros(
            DebugMacroCheck.get_debug_macros(source_str),
            cls._get_function_name())

    @classmethod
    def _get_function_name(cls) -> str:
        """Returns the function name from one level of call depth.

        Returns:
            str: The caller function name.
        """
        return "function: " + inspect.currentframe().f_back.f_code.co_name


# Test container classes for dynamically generated macro test cases.
# A class can be removed below to skip / remove it from testing.
# Test case functions will be added to the appropriate class as they are
# created.
class Test_NoSpecifierNoArgument(
        unittest.TestCase,
        metaclass=Meta_TestDebugMacroCheck,
        category="no_specifier_no_argument_macro_test"):
    pass


class Test_EqualSpecifierEqualArgument(
        unittest.TestCase,
        metaclass=Meta_TestDebugMacroCheck,
        category="equal_specifier_equal_argument_macro_test"):
    pass


class Test_MoreSpecifiersThanArguments(
        unittest.TestCase,
        metaclass=Meta_TestDebugMacroCheck,
        category="more_specifiers_than_arguments_macro_test"):
    pass


class Test_LessSpecifiersThanArguments(
        unittest.TestCase,
        metaclass=Meta_TestDebugMacroCheck,
        category="less_specifiers_than_arguments_macro_test"):
    pass


class Test_IgnoredSpecifiers(
        unittest.TestCase,
        metaclass=Meta_TestDebugMacroCheck,
        category="ignored_specifiers_macro_test"):
    pass


class Test_SpecialParsingMacroTest(
        unittest.TestCase,
        metaclass=Meta_TestDebugMacroCheck,
        category="special_parsing_macro_test"):
    pass


class Test_CodeSnippetMacroTest(
        unittest.TestCase,
        metaclass=Meta_TestDebugMacroCheck,
        category="code_snippet_macro_test"):
    pass


if __name__ == '__main__':
    unittest.main()
