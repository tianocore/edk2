# @file
# A CI plugin used to run cargo tarpaulin for all host based tests.
# Ensures that all host based tests pass and meet code coverage requirements.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from typing import List
from edk2toolext.environment.repo_resolver import repo_details
from pathlib import Path
import re
import logging
import platform

class RustHostUnitTestPlugin(ICiBuildPlugin):
    def GetTestName(self, packagename: str, environment: object) -> tuple[str, str]:
        return (f'Host Unit Tests in {packagename}', f'{packagename}.RustHostUnitTestPlugin')

    def RunsOnTargetList(self) -> List[str]:
        return ["NO-TARGET"]

    def RunBuildPlugin(self, packagename, Edk2pathObj, pkgconfig, environment, PLM, PLMHelper, tc, output_stream):
        ws = Edk2pathObj.WorkspacePath
        rust_ws = PLMHelper.RustWorkspace(ws)  # .pytool/Plugin/RustPackageHelper

        with_coverage = pkgconfig.get("CalculateCoverage", True)

        if platform.system() == "Windows" and platform.machine() == 'ARM64':
            logging.debug("Coverage skipped by plugin, not supported on Windows ARM")
            with_coverage = False

        # Build list of packages that are in the EDK2 package we are running CI on
        pp = Path(Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(packagename))
        crate_name_list = [pkg.name for pkg in filter(lambda pkg: Path(pkg.path).is_relative_to(pp), rust_ws.members)]
        crate_path_list = [pkg.path for pkg in filter(lambda pkg: Path(pkg.path).is_relative_to(pp), rust_ws.members)]
        if not crate_name_list:
            logging.debug("No Rust crates found in package, skipping...")
            tc.SetSkipped()
            return -1
        logging.debug(f"Rust Crates to test: {' '.join(crate_name_list)}")

        # Build a list of paths to ignore when computing results. This includes:
        # 1. Any tests folder in a rust package
        # 2. Everything in a submodule
        # 3. Everything in an EDK2 package not being tested.
        ignore_list = [Path("**", "tests", "*")]
        ignore_list.extend([Path(s, "**", "*") for s in repo_details(ws)["Submodules"]])
        ignored_local_crates = list(set(crate.path for crate in rust_ws.members) - set(crate_path_list))
        ignore_list.extend([Path(crate, "**", "*").relative_to(ws) for crate in ignored_local_crates])
        ignore_list = [str(i) for i in ignore_list]
        logging.debug(f"Paths to ignore when computing coverage: {' '.join(ignore_list)}")

        # Run tests and evaluate results
        try:
            results = rust_ws.test(crate_name_list, ignore_list = ignore_list, report_type = "xml", coverage=with_coverage)
        except RuntimeError as e:
            logging.warning(str(e))
            tc.LogStdError(str(e))
            tc.SetFailed(str(e), "CHECK_FAILED")
            return 1

        # Evaluate unit test results
        failed = 0
        for test in results["pass"]:
            tc.LogStdOut(f'{test} ... PASS')

        for test in results["fail"]:
            e = f'{test} ... FAIL'
            logging.warning(e)
            tc.LogStdError(e)
            failed += 1

        # If we failed a unit test, we have no coverage data to evaluate
        if failed > 0:
            tc.SetFailed(f'Host unit tests failed. Failures {failed}', "CHECK_FAILED")
            return failed

        # Return if we are not running with coverage
        if not with_coverage:
            tc.SetSuccess()
            return 0

        # Calculate coverage
        coverage = {}
        for file, cov in results["coverage"].items():
            try:
                package = next(
                    crate.name
                    for crate in rust_ws.members
                    if Path(ws, file).is_relative_to(crate.path)
                )
            except StopIteration:
                continue
            covered, total = cov.split("/")
            if package in coverage:
                coverage[package]["cov"] += int(covered)
                coverage[package]["total"] += int(total)
            else:
                coverage[package] = {"cov": int(covered), "total": int(total)}

        # Evaluate coverage results
        default_cov = pkgconfig.get("coverage", 0.75)
        for pkg, cov in coverage.items():
            required_cov = pkgconfig.get("CoverageOverrides", {pkg: default_cov}).get(pkg, default_cov)

            calc_cov = round(cov["cov"] / cov["total"], 2)
            if calc_cov >= required_cov:
                tc.LogStdOut(f'coverage::{pkg}: {calc_cov} greater than {required_cov} ... PASS')
            else:
                e = f'coverage::{pkg}: {calc_cov}% less than {required_cov}% ... FAIL'
                logging.warning(e)
                tc.LogStdError(e)
                failed += 1

        # Move coverage.xml to Build Directory
        xml = Path(rust_ws.path) / "target" / "cobertura.xml"
        if xml.exists():
            out = Path(rust_ws.path) / "Build"

            if (out / "coverage.xml").exists():
                (out / "coverage.xml").unlink()
            xml = xml.rename(out / "coverage.xml")

            with open(xml, 'r') as f:
                contents = f.read()
                contents = re.sub(r'<source>(.*?)</source>', r'<source>.</source>', contents)

            with open (xml, "w") as f:
                f.write(contents)

        # Return
        if failed > 0:
            tc.SetFailed(f'Coverage requirements not met. Failures {failed}', "CHECK_FAILED")
        else:
            tc.SetSuccess()

        return failed
