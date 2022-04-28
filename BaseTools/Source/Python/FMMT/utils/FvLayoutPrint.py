## @file
# This file is used to define the printer for Bios layout.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from utils.FmmtLogger import FmmtLogger as logger

def GetFormatter(layout_format: str):
    if layout_format == 'json':
        return JsonFormatter()
    elif layout_format == 'yaml':
        return YamlFormatter()
    elif layout_format == 'html':
        return HtmlFormatter()
    else:
        return TxtFormatter()

class Formatter(object):
    def dump(self, layoutdict, layoutlist, outputfile: str=None) -> None:
        raise NotImplemented

class JsonFormatter(Formatter):
    def dump(self,layoutdict: dict, layoutlist: list, outputfile: str=None) -> None:
        try:
            import json
        except:
            TxtFormatter().dump(layoutdict, layoutlist, outputfile)
            return
        print(outputfile)
        if outputfile:
            with open(outputfile,"w") as fw:
                json.dump(layoutdict, fw, indent=2)
        else:
            print(json.dumps(layoutdict,indent=2))

class TxtFormatter(Formatter):
    def LogPrint(self,layoutlist: list) -> None:
        for item in layoutlist:
            print(item)
        print('\n')

    def dump(self,layoutdict: dict, layoutlist: list, outputfile: str=None) -> None:
        logger.info('Binary Layout Info is saved in {} file.'.format(outputfile))
        with open(outputfile, "w") as f:
            for item in layoutlist:
                f.writelines(item + '\n')

class YamlFormatter(Formatter):
    def dump(self,layoutdict, layoutlist, outputfile = None):
        TxtFormatter().dump(layoutdict, layoutlist, outputfile)

class HtmlFormatter(Formatter):
    def dump(self,layoutdict, layoutlist, outputfile = None):
        TxtFormatter().dump(layoutdict, layoutlist, outputfile)