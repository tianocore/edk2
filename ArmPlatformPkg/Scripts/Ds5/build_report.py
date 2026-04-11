#
#  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import re

class BuildReport:
    PCDs = {}

    def parse_platform_summary(self, file):
        pass

    def parse_pcd_report(self, report_file):
        pcd_reg = re.compile(" (\*P|\*F|\*M|  ) (\w+)(\ +)\: (.*) \((\w+)\) = (.*)\n")

        for line in report_file.xreadlines():
            stripped_line = line.strip()
            if re.match("\<=+\>", stripped_line):
                return
            elif re.match("g.*Guid", stripped_line):
                guid = stripped_line
                self.PCDs[guid] = {}
            else:
                m = pcd_reg.match(line)
                if m:
                    self.PCDs[guid][m.group(2)] = (m.group(6).strip(),m.group(5))

    def parse_firmware_device(self, file):
        pass

    def parse_module_summary(self, file):
        #print "Module Summary"
        pass

    CONST_SECTION_HEADERS = [('Platform Summary', parse_platform_summary),
                             ('Platform Configuration Database Report',parse_pcd_report),
                             ('Firmware Device (FD)',parse_firmware_device),
                             ('Module Summary',parse_module_summary)]

    def __init__(self, filename = 'report.log'):
        report_file = open(filename, 'r')
        for line in report_file.xreadlines():
            for section_header in BuildReport.CONST_SECTION_HEADERS:
                if line.strip() == section_header[0]:
                    section_header[1](self, report_file)
        #print self.PCDs
