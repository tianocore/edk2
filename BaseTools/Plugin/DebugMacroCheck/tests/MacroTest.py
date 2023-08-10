# @file MacroTest.py
#
# Contains the data classes that are used to compose debug macro tests.
#
# All data classes inherit from a single abstract base class that expects
# the subclass to define the category of test it represents.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from dataclasses import dataclass, field
from os import linesep
from typing import Tuple

import abc


@dataclass(frozen=True)
class MacroTest(abc.ABC):
    """Abstract base class for an individual macro test case."""

    macro: str
    result: Tuple[int, int, int]
    description: str = field(default='')

    @property
    @abc.abstractmethod
    def category(self) -> str:
        """Returns the test class category identifier.

        Example: 'equal_specifier_equal_argument_macro_test'

        This string is used to bind test objects against this class.

        Returns:
            str: Test category identifier string.
        """
        pass

    @property
    def category_description(self) -> str:
        """Returns the test class category description.

        Example: 'Test case with equal count of print specifiers to arguments.'

        This string is a human readable description of the test category.

        Returns:
            str: String describing the test category.
        """
        return self.__doc__

    def __str__(self):
        """Returns a macro test case description string."""

        s = [
            f"{linesep}",
            "=" * 80,
            f"Macro Test Type:  {self.category_description}",
            f"{linesep}Macro:            {self.macro}",
            f"{linesep}Expected Result:  {self.result}"
        ]

        if self.description:
            s.insert(3, f"Test Description: {self.description}")

        return f'{linesep}'.join(s)


@dataclass(frozen=True)
class NoSpecifierNoArgumentMacroTest(MacroTest):
    """Test case with no print specifier and no arguments."""

    @property
    def category(self) -> str:
        return "no_specifier_no_argument_macro_test"


@dataclass(frozen=True)
class EqualSpecifierEqualArgumentMacroTest(MacroTest):
    """Test case with equal count of print specifiers to arguments."""

    @property
    def category(self) -> str:
        return "equal_specifier_equal_argument_macro_test"


@dataclass(frozen=True)
class MoreSpecifiersThanArgumentsMacroTest(MacroTest):
    """Test case with more print specifiers than arguments."""

    @property
    def category(self) -> str:
        return "more_specifiers_than_arguments_macro_test"


@dataclass(frozen=True)
class LessSpecifiersThanArgumentsMacroTest(MacroTest):
    """Test case with less print specifiers than arguments."""

    @property
    def category(self) -> str:
        return "less_specifiers_than_arguments_macro_test"


@dataclass(frozen=True)
class IgnoredSpecifiersMacroTest(MacroTest):
    """Test case to test ignored print specifiers."""

    @property
    def category(self) -> str:
        return "ignored_specifiers_macro_test"


@dataclass(frozen=True)
class SpecialParsingMacroTest(MacroTest):
    """Test case with special (complicated) parsing scenarios."""

    @property
    def category(self) -> str:
        return "special_parsing_macro_test"


@dataclass(frozen=True)
class CodeSnippetMacroTest(MacroTest):
    """Test case within a larger code snippet."""

    @property
    def category(self) -> str:
        return "code_snippet_macro_test"
