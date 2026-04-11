##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
import json

try:
    from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin

    class BuildToolsReportGenerator(IUefiBuildPlugin):
        def do_report(self, thebuilder):
            try:
                from edk2toolext.environment import version_aggregator
            except ImportError:
                logging.critical("Loading BuildToolsReportGenerator failed, please update your Edk2-PyTool-Extensions")
                return 0

            OutputReport = os.path.join(thebuilder.env.GetValue("BUILD_OUTPUT_BASE"), "BUILD_TOOLS_REPORT")
            OutputReport = os.path.normpath(OutputReport)
            if not os.path.isdir(os.path.dirname(OutputReport)):
                os.makedirs(os.path.dirname(OutputReport))

            Report = BuildToolsReport()
            Report.MakeReport(version_aggregator.GetVersionAggregator().GetAggregatedVersionInformation(), OutputReport=OutputReport)

        def do_pre_build(self, thebuilder):
            self.do_report(thebuilder)
            return 0

        def do_post_build(self, thebuilder):
            self.do_report(thebuilder)
            return 0

except ImportError:
    pass


class BuildToolsReport(object):
    MY_FOLDER = os.path.dirname(os.path.realpath(__file__))
    VERSION = "1.00"

    def __init__(self):
        pass

    def MakeReport(self, BuildTools, OutputReport="BuildToolsReport"):
        logging.info("Writing BuildToolsReports to {0}".format(OutputReport))
        versions_list = []
        for key, value in BuildTools.items():
            versions_list.append(value)
        versions_list = sorted(versions_list, key=lambda k: k['type'])
        json_dict = {"modules": versions_list,
                     "PluginVersion": BuildToolsReport.VERSION}

        htmlfile = open(OutputReport + ".html", "w")
        jsonfile = open(OutputReport + ".json", "w")
        template = open(os.path.join(BuildToolsReport.MY_FOLDER, "BuildToolsReport_Template.html"), "r")

        for line in template.readlines():
            if "%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%" in line:
                line = line.replace("%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%", json.dumps(json_dict))
            htmlfile.write(line)

        jsonfile.write(json.dumps(versions_list, indent=4))

        jsonfile.close()
        template.close()
        htmlfile.close()
