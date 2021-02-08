#!/usr/bin/env python
## @ FspDscBsf2Yaml.py
# This script convert DSC or BSF format file into YAML format
#
# Copyright(c) 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import os
import re
import sys
from datetime import date
from collections import OrderedDict
from functools import reduce

from GenCfgOpt import CGenCfgOpt

__copyright_tmp__ = """## @file
#
#  YAML CFGDATA %s File.
#
#  Copyright(c) %4d, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
"""

__copyright_dsc__ = """## @file
#
#  Copyright (c) %04d, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[PcdsDynamicVpd.Upd]
  #
  # Global definitions in BSF
  # !BSF BLOCK:{NAME:"FSP UPD Configuration", VER:"0.1"}
  #

"""


def Bytes2Val(Bytes):
    return reduce(lambda x, y: (x << 8) | y, Bytes[::-1])


class CFspBsf2Dsc:

    def __init__(self, bsf_file):
        self.cfg_list = CFspBsf2Dsc.parse_bsf(bsf_file)

    def get_dsc_lines(self):
        return CFspBsf2Dsc.generate_dsc(self.cfg_list)

    def save_dsc(self, dsc_file):
        return CFspBsf2Dsc.generate_dsc(self.cfg_list, dsc_file)

    @staticmethod
    def parse_bsf(bsf_file):

        fd = open(bsf_file, 'r')
        bsf_txt = fd.read()
        fd.close()

        find_list = []
        regex = re.compile(r'\s+Find\s+"(.*?)"(.*?)^\s+\$(.*?)\s+', re.S | re.MULTILINE)
        for match in regex.finditer(bsf_txt):
            find = match.group(1)
            name = match.group(3)
            if not name.endswith('_Revision'):
                raise Exception("Unexpected CFG item following 'Find' !")
            find_list.append((name, find))

        idx = 0
        count = 0
        prefix = ''
        chk_dict = {}
        cfg_list = []
        cfg_temp = {'find': '', 'cname': '', 'length': 0, 'value': '0', 'type': 'Reserved',
                    'embed': '', 'page': '', 'option': '', 'instance': 0}
        regex = re.compile(r'^\s+(\$(.*?)|Skip)\s+(\d+)\s+bytes(\s+\$_DEFAULT_\s+=\s+(.+?))?$',
                           re.S | re.MULTILINE)

        for match in regex.finditer(bsf_txt):
            dlen = int(match.group(3))
            if match.group(1) == 'Skip':
                key = 'gPlatformFspPkgTokenSpaceGuid_BsfSkip%d' % idx
                val = ', '.join(['%02X' % ord(i) for i in '\x00' * dlen])
                idx += 1
                option = '$SKIP'
            else:
                key = match.group(2)
                val = match.group(5)
                option = ''

            cfg_item = dict(cfg_temp)
            finds = [i for i in find_list if i[0] == key]
            if len(finds) > 0:
                if count >= 1:
                    # Append a dummy one
                    cfg_item['cname'] = 'Dummy'
                    cfg_list.append(dict(cfg_item))
                    cfg_list[-1]['embed'] = '%s:TAG_%03X:END' % (prefix, ord(prefix[-1]))
                prefix = finds[0][1]
                cfg_item['embed'] = '%s:TAG_%03X:START' % (prefix, ord(prefix[-1]))
                cfg_item['find'] = prefix
                cfg_item['cname'] = 'Signature'
                cfg_item['length'] = len(finds[0][1])
                cfg_item['value'] = '0x%X' % Bytes2Val(finds[0][1].encode('UTF-8'))
                cfg_list.append(dict(cfg_item))
                cfg_item = dict(cfg_temp)
                find_list.pop(0)
                count = 0

            cfg_item['cname'] = key
            cfg_item['length'] = dlen
            cfg_item['value'] = val
            cfg_item['option'] = option

            if key not in chk_dict.keys():
                chk_dict[key] = 0
            else:
                chk_dict[key] += 1
            cfg_item['instance'] = chk_dict[key]

            cfg_list.append(cfg_item)
            count += 1

        if prefix:
            cfg_item = dict(cfg_temp)
            cfg_item['cname'] = 'Dummy'
            cfg_item['embed'] = '%s:%03X:END' % (prefix, ord(prefix[-1]))
            cfg_list.append(cfg_item)

        option_dict = {}
        selreg = re.compile(r'\s+Selection\s*(.+?)\s*,\s*"(.*?)"$', re.S | re.MULTILINE)
        regex = re.compile(r'^List\s&(.+?)$(.+?)^EndList$', re.S | re.MULTILINE)
        for match in regex.finditer(bsf_txt):
            key = match.group(1)
            option_dict[key] = []
            for select in selreg.finditer(match.group(2)):
                option_dict[key].append((int(select.group(1), 0), select.group(2)))

        chk_dict = {}
        pagereg = re.compile(r'^Page\s"(.*?)"$(.+?)^EndPage$', re.S | re.MULTILINE)
        for match in pagereg.finditer(bsf_txt):
            page = match.group(1)
            for line in match.group(2).splitlines():
                match = re.match(r'\s+(Combo|EditNum)\s\$(.+?),\s"(.*?)",\s(.+?),$', line)
                if match:
                    cname = match.group(2)
                    if cname not in chk_dict.keys():
                        chk_dict[cname] = 0
                    else:
                        chk_dict[cname] += 1
                    instance = chk_dict[cname]
                    cfg_idxs = [i for i, j in enumerate(cfg_list) if j['cname'] == cname and j['instance'] == instance]
                    if len(cfg_idxs) != 1:
                        raise Exception("Multiple CFG item '%s' found !" % cname)
                    cfg_item = cfg_list[cfg_idxs[0]]
                    cfg_item['page'] = page
                    cfg_item['type'] = match.group(1)
                    cfg_item['prompt'] = match.group(3)
                    cfg_item['range'] = None
                    if cfg_item['type'] == 'Combo':
                        cfg_item['option'] = option_dict[match.group(4)[1:]]
                    elif cfg_item['type'] == 'EditNum':
                        cfg_item['option'] = match.group(4)
                match = re.match(r'\s+ Help\s"(.*?)"$', line)
                if match:
                    cfg_item['help'] = match.group(1)

                match = re.match(r'\s+"Valid\srange:\s(.*)"$', line)
                if match:
                    parts = match.group(1).split()
                    cfg_item['option'] = (
                        (int(parts[0], 0), int(parts[2], 0), cfg_item['option']))

        return cfg_list

    @staticmethod
    def generate_dsc(option_list, dsc_file=None):
        dsc_lines = []
        header = '%s' % (__copyright_dsc__ % date.today().year)
        dsc_lines.extend(header.splitlines())

        pages = []
        for cfg_item in option_list:
            if cfg_item['page'] and (cfg_item['page'] not in pages):
                pages.append(cfg_item['page'])

        page_id = 0
        for page in pages:
            dsc_lines.append('  # !BSF PAGES:{PG%02X::"%s"}' % (page_id, page))
            page_id += 1
        dsc_lines.append('')

        last_page = ''
        for option in option_list:
            dsc_lines.append('')
            default = option['value']
            pos = option['cname'].find('_')
            name = option['cname'][pos + 1:]

            if option['find']:
                dsc_lines.append('  # !BSF FIND:{%s}' % option['find'])
                dsc_lines.append('')

            if option['instance'] > 0:
                name = name + '_%s' % option['instance']

            if option['embed']:
                dsc_lines.append('  # !HDR EMBED:{%s}' % option['embed'])

            if option['type'] == 'Reserved':
                dsc_lines.append('  # !BSF NAME:{Reserved} TYPE:{Reserved}')
                if option['option'] == '$SKIP':
                    dsc_lines.append('  # !BSF OPTION:{$SKIP}')
            else:
                prompt = option['prompt']

                if last_page != option['page']:
                    last_page = option['page']
                    dsc_lines.append('  # !BSF PAGE:{PG%02X}' % (pages.index(option['page'])))

                if option['type'] == 'Combo':
                    dsc_lines.append('  # !BSF NAME:{%s} TYPE:{%s}' %
                                     (prompt, option['type']))
                    ops = []
                    for val, text in option['option']:
                        ops.append('0x%x:%s' % (val, text))
                    dsc_lines.append('  # !BSF OPTION:{%s}' % (', '.join(ops)))
                elif option['type'] == 'EditNum':
                    cfg_len = option['length']
                    if ',' in default and cfg_len > 8:
                        dsc_lines.append('  # !BSF NAME:{%s} TYPE:{Table}' % (prompt))
                        if cfg_len > 16:
                            cfg_len = 16
                        ops = []
                        for i in range(cfg_len):
                            ops.append('%X:1:HEX' % i)
                        dsc_lines.append('  # !BSF OPTION:{%s}' % (', '.join(ops)))
                    else:
                        dsc_lines.append(
                            '  # !BSF NAME:{%s} TYPE:{%s, %s,(0x%X, 0x%X)}' %
                            (prompt, option['type'], option['option'][2],
                             option['option'][0], option['option'][1]))
                dsc_lines.append('  # !BSF HELP:{%s}' % option['help'])

            if ',' in default:
                default = '{%s}' % default
            dsc_lines.append('  gCfgData.%-30s | * | 0x%04X | %s' %
                             (name, option['length'], default))

        if dsc_file:
            fd = open(dsc_file, 'w')
            fd.write('\n'.join(dsc_lines))
            fd.close()

        return dsc_lines


class CFspDsc2Yaml():

    def __init__(self):
        self._Hdr_key_list = ['EMBED', 'STRUCT']
        self._Bsf_key_list = ['NAME', 'HELP', 'TYPE', 'PAGE', 'PAGES', 'OPTION',
                              'CONDITION', 'ORDER', 'MARKER', 'SUBT', 'FIELD', 'FIND']
        self.gen_cfg_data = None
        self.cfg_reg_exp = re.compile(r"^([_a-zA-Z0-9$\(\)]+)\s*\|\s*(0x[0-9A-F]+|\*)\s*\|"
                                      + r"\s*(\d+|0x[0-9a-fA-F]+)\s*\|\s*(.+)")
        self.bsf_reg_exp = re.compile(r"(%s):{(.+?)}(?:$|\s+)" % '|'.join(self._Bsf_key_list))
        self.hdr_reg_exp = re.compile(r"(%s):{(.+?)}" % '|'.join(self._Hdr_key_list))
        self.prefix = ''
        self.unused_idx = 0
        self.offset = 0
        self.base_offset = 0

    def load_config_data_from_dsc(self, file_name):
        """
        Load and parse a DSC CFGDATA file.
        """
        gen_cfg_data = CGenCfgOpt('FSP')
        if file_name.endswith('.dsc'):
            # if gen_cfg_data.ParseDscFileYaml(file_name, '') != 0:
            if gen_cfg_data.ParseDscFile(file_name, '') != 0:
                raise Exception('DSC file parsing error !')
            if gen_cfg_data.CreateVarDict() != 0:
                raise Exception('DSC variable creation error !')
        else:
            raise Exception('Unsupported file "%s" !' % file_name)
        gen_cfg_data.UpdateDefaultValue()
        self.gen_cfg_data = gen_cfg_data

    def print_dsc_line(self):
        """
        Debug function to print all DSC lines.
        """
        for line in self.gen_cfg_data._DscLines:
            print(line)

    def format_value(self, field, text, indent=''):
        """
        Format a CFGDATA item into YAML format.
        """
        if(not text.startswith('!expand')) and (': ' in text):
            tgt = ':' if field == 'option' else '- '
            text = text.replace(': ', tgt)
        lines = text.splitlines()
        if len(lines) == 1 and field != 'help':
            return text
        else:
            return '>\n   ' + '\n   '.join([indent + i.lstrip() for i in lines])

    def reformat_pages(self, val):
        # Convert XXX:YYY into XXX::YYY format for page definition
        parts = val.split(',')
        if len(parts) <= 1:
            return val

        new_val = []
        for each in parts:
            nodes = each.split(':')
            if len(nodes) == 2:
                each = '%s::%s' % (nodes[0], nodes[1])
            new_val.append(each)
        ret = ','.join(new_val)
        return ret

    def reformat_struct_value(self, utype, val):
        # Convert DSC UINT16/32/64 array into new format by
        # adding prefix 0:0[WDQ] to provide hint to the array format
        if utype in ['UINT16', 'UINT32', 'UINT64']:
            if val and val[0] == '{' and val[-1] == '}':
                if utype == 'UINT16':
                    unit = 'W'
                elif utype == 'UINT32':
                    unit = 'D'
                else:
                    unit = 'Q'
                val = '{ 0:0%s, %s }' % (unit, val[1:-1])
        return val

    def process_config(self, cfg):
        if 'page' in cfg:
            cfg['page'] = self.reformat_pages(cfg['page'])

        if 'struct' in cfg:
            cfg['value'] = self.reformat_struct_value(cfg['struct'], cfg['value'])

    def parse_dsc_line(self, dsc_line, config_dict, init_dict, include):
        """
        Parse a line in DSC and update the config dictionary accordingly.
        """
        init_dict.clear()
        match = re.match(r'g(CfgData|\w+FspPkgTokenSpaceGuid)\.(.+)', dsc_line)
        if match:
            match = self.cfg_reg_exp.match(match.group(2))
            if not match:
                return False
            config_dict['cname'] = self.prefix + match.group(1)
            value = match.group(4).strip()
            length = match.group(3).strip()
            config_dict['length'] = length
            config_dict['value'] = value
            if match.group(2) == '*':
                self.offset += int(length, 0)
            else:
                org_offset = int(match.group(2), 0)
                if org_offset == 0:
                    self.base_offset = self.offset
                offset = org_offset + self.base_offset
                if self.offset != offset:
                    if offset > self.offset:
                        init_dict['padding'] = offset - self.offset
                self.offset = offset + int(length, 0)
            return True

        match = re.match(r"^\s*#\s+!([<>])\s+include\s+(.+)", dsc_line)
        if match and len(config_dict) == 0:
            # !include should not be inside a config field
            # if so, do not convert include into YAML
            init_dict = dict(config_dict)
            config_dict.clear()
            config_dict['cname'] = '$ACTION'
            if match.group(1) == '<':
                config_dict['include'] = match.group(2)
            else:
                config_dict['include'] = ''
            return True

        match = re.match(r"^\s*#\s+(!BSF|!HDR)\s+(.+)", dsc_line)
        if not match:
            return False

        remaining = match.group(2)
        if match.group(1) == '!BSF':
            result = self.bsf_reg_exp.findall(remaining)
            if not result:
                return False

            for each in result:
                key = each[0].lower()
                val = each[1]
                if key == 'field':
                    name = each[1]
                    if ':' not in name:
                        raise Exception('Incorrect bit field format !')
                    parts = name.split(':')
                    config_dict['length'] = parts[1]
                    config_dict['cname'] = '@' + parts[0]
                    return True
                elif key in ['pages', 'page', 'find']:
                    init_dict = dict(config_dict)
                    config_dict.clear()
                    config_dict['cname'] = '$ACTION'
                    if key == 'find':
                        config_dict['find'] = val
                    else:
                        config_dict['page'] = val
                    return True
                elif key == 'subt':
                    config_dict.clear()
                    parts = each[1].split(':')
                    tmp_name = parts[0][:-5]
                    if tmp_name == 'CFGHDR':
                        cfg_tag = '_$FFF_'
                        sval = '!expand { %s_TMPL : [ ' % tmp_name + '%s, %s, ' % (parts[1], cfg_tag) \
                               + ', '.join(parts[2:]) + ' ] }'
                    else:
                        sval = '!expand { %s_TMPL : [ ' % tmp_name + ', '.join(parts[1:]) + ' ] }'
                    config_dict.clear()
                    config_dict['cname'] = tmp_name
                    config_dict['expand'] = sval
                    return True
                else:
                    if key in ['name', 'help', 'option'] and val.startswith('+'):
                        val = config_dict[key] + '\n' + val[1:]
                    if val.strip() == '':
                        val = "''"
                    config_dict[key] = val

        else:
            match = self.hdr_reg_exp.match(remaining)
            if not match:
                return False
            key = match.group(1)
            remaining = match.group(2)
            if key == 'EMBED':
                parts = remaining.split(':')
                names = parts[0].split(',')
                if parts[-1] == 'END':
                    prefix = '>'
                else:
                    prefix = '<'
                skip = False
                if parts[1].startswith('TAG_'):
                    tag_txt = '%s:%s' % (names[0], parts[1])
                else:
                    tag_txt = names[0]
                    if parts[2] in ['START', 'END']:
                        if names[0] == 'PCIE_RP_PIN_CTRL[]':
                            skip = True
                        else:
                            tag_txt = '%s:%s' % (names[0], parts[1])
                if not skip:
                    config_dict.clear()
                    config_dict['cname'] = prefix + tag_txt
                    return True

            if key == 'STRUCT':
                text = remaining.strip()
                config_dict[key.lower()] = text

        return False

    def process_template_lines(self, lines):
        """
        Process a line in DSC template section.
        """
        template_name = ''
        bsf_temp_dict = OrderedDict()
        temp_file_dict = OrderedDict()
        include_file = ['.']

        for line in lines:
            match = re.match(r"^\s*#\s+!([<>])\s+include\s+(.+)", line)
            if match:
                if match.group(1) == '<':
                    include_file.append(match.group(2))
                else:
                    include_file.pop()

            match = re.match(r"^\s*#\s+(!BSF)\s+DEFT:{(.+?):(START|END)}", line)
            if match:
                if match.group(3) == 'START' and not template_name:
                    template_name = match.group(2).strip()
                    temp_file_dict[template_name] = list(include_file)
                    bsf_temp_dict[template_name] = []
                if match.group(3) == 'END' and (template_name == match.group(2).strip()) \
                   and template_name:
                    template_name = ''
            else:
                if template_name:
                    bsf_temp_dict[template_name].append(line)
        return bsf_temp_dict, temp_file_dict

    def process_option_lines(self, lines):
        """
        Process a line in DSC config section.
        """
        cfgs = []
        struct_end = False
        config_dict = dict()
        init_dict = dict()
        include = ['']
        for line in lines:
            ret = self.parse_dsc_line(line, config_dict, init_dict, include)
            if ret:
                if 'padding' in init_dict:
                    num = init_dict['padding']
                    init_dict.clear()
                    padding_dict = {}
                    cfgs.append(padding_dict)
                    padding_dict['cname'] = 'UnusedUpdSpace%d' % self.unused_idx
                    padding_dict['length'] = '0x%x' % num
                    padding_dict['value'] = '{ 0 }'
                    self.unused_idx += 1

                if cfgs and cfgs[-1]['cname'][0] != '@' and config_dict['cname'][0] == '@':
                    # it is a bit field, mark the previous one as virtual
                    cname = cfgs[-1]['cname']
                    new_cfg = dict(cfgs[-1])
                    new_cfg['cname'] = '@$STRUCT'
                    cfgs[-1].clear()
                    cfgs[-1]['cname'] = cname
                    cfgs.append(new_cfg)

                if cfgs and cfgs[-1]['cname'] == 'CFGHDR' and config_dict['cname'][0] == '<':
                    # swap CfgHeader and the CFG_DATA order
                    if ':' in config_dict['cname']:
                        # replace the real TAG for CFG_DATA
                        cfgs[-1]['expand'] = cfgs[-1]['expand'].replace(
                            '_$FFF_', '0x%s' %
                            config_dict['cname'].split(':')[1][4:])
                    cfgs.insert(-1, config_dict)
                else:
                    self.process_config(config_dict)
                    if struct_end:
                        struct_end = False
                        cfgs.insert(-1, config_dict)
                    else:
                        cfgs.append(config_dict)
                        if config_dict['cname'][0] == '>':
                            struct_end = True

                config_dict = dict(init_dict)
        return cfgs

    def variable_fixup(self, each):
        """
        Fix up some variable definitions for SBL.
        """
        key = each
        val = self.gen_cfg_data._MacroDict[each]
        return key, val

    def template_fixup(self, tmp_name, tmp_list):
        """
        Fix up some special config templates for SBL
        """
        return

    def config_fixup(self, cfg_list):
        """
        Fix up some special config items for SBL.
        """

        # Insert FSPT_UPD/FSPM_UPD/FSPS_UPD tag so as to create C strcture
        idxs = []
        for idx, cfg in enumerate(cfg_list):
            if cfg['cname'].startswith('<FSP_UPD_HEADER'):
                idxs.append(idx)

        if len(idxs) != 3:
            return

        # Handle insert backwards so that the index does not change in the loop
        fsp_comp = 'SMT'
        idx_comp = 0
        for idx in idxs[::-1]:
            # Add current FSP?_UPD start tag
            cfgfig_dict = {}
            cfgfig_dict['cname'] = '<FSP%s_UPD' % fsp_comp[idx_comp]
            cfg_list.insert(idx, cfgfig_dict)
            if idx_comp < 2:
                # Add previous FSP?_UPD end tag
                cfgfig_dict = {}
                cfgfig_dict['cname'] = '>FSP%s_UPD' % fsp_comp[idx_comp + 1]
                cfg_list.insert(idx, cfgfig_dict)
            idx_comp += 1

        # Add final FSPS_UPD end tag
        cfgfig_dict = {}
        cfgfig_dict['cname'] = '>FSP%s_UPD' % fsp_comp[0]
        cfg_list.append(cfgfig_dict)

        return

    def get_section_range(self, section_name):
        """
        Extract line number range from config file for a given section name.
        """
        start = -1
        end = -1
        for idx, line in enumerate(self.gen_cfg_data._DscLines):
            if start < 0 and line.startswith('[%s]' % section_name):
                start = idx
            elif start >= 0 and line.startswith('['):
                end = idx
                break
        if start == -1:
            start = 0
        if end == -1:
            end = len(self.gen_cfg_data._DscLines)
        return start, end

    def normalize_file_name(self, file, is_temp=False):
        """
        Normalize file name convention so that it is consistent.
        """
        if file.endswith('.dsc'):
            file = file[:-4] + '.yaml'
        dir_name = os.path.dirname(file)
        base_name = os.path.basename(file)
        if is_temp:
            if 'Template_' not in file:
                base_name = base_name.replace('Template', 'Template_')
        else:
            if 'CfgData_' not in file:
                base_name = base_name.replace('CfgData', 'CfgData_')
        if dir_name:
            path = dir_name + '/' + base_name
        else:
            path = base_name
        return path

    def output_variable(self):
        """
        Output variable block into a line list.
        """
        lines = []
        for each in self.gen_cfg_data._MacroDict:
            key, value = self.variable_fixup(each)
            lines.append('%-30s : %s' % (key, value))
        return lines

    def output_template(self):
        """
        Output template block into a line list.
        """
        self.offset = 0
        self.base_offset = 0
        start, end = self.get_section_range('PcdsDynamicVpd.Tmp')
        bsf_temp_dict, temp_file_dict = self.process_template_lines(self.gen_cfg_data._DscLines[start:end])
        template_dict = dict()
        lines = []
        file_lines = {}
        last_file = '.'
        file_lines[last_file] = []

        for tmp_name in temp_file_dict:
            temp_file_dict[tmp_name][-1] = self.normalize_file_name(temp_file_dict[tmp_name][-1], True)
            if len(temp_file_dict[tmp_name]) > 1:
                temp_file_dict[tmp_name][-2] = self.normalize_file_name(temp_file_dict[tmp_name][-2], True)

        for tmp_name in bsf_temp_dict:
            file = temp_file_dict[tmp_name][-1]
            if last_file != file and len(temp_file_dict[tmp_name]) > 1:
                inc_file = temp_file_dict[tmp_name][-2]
                file_lines[inc_file].extend(['', '- !include %s' % temp_file_dict[tmp_name][-1], ''])
            last_file = file
            if file not in file_lines:
                file_lines[file] = []
            lines = file_lines[file]
            text = bsf_temp_dict[tmp_name]
            tmp_list = self.process_option_lines(text)
            self.template_fixup(tmp_name, tmp_list)
            template_dict[tmp_name] = tmp_list
            lines.append('%s: >' % tmp_name)
            lines.extend(self.output_dict(tmp_list, False)['.'])
            lines.append('\n')
        return file_lines

    def output_config(self):
        """
        Output config block into a line list.
        """
        self.offset = 0
        self.base_offset = 0
        start, end = self.get_section_range('PcdsDynamicVpd.Upd')
        cfgs = self.process_option_lines(self.gen_cfg_data._DscLines[start:end])
        self.config_fixup(cfgs)
        file_lines = self.output_dict(cfgs, True)
        return file_lines

    def output_dict(self, cfgs, is_configs):
        """
        Output one config item into a line list.
        """
        file_lines = {}
        level = 0
        file = '.'
        for each in cfgs:
            if 'length' in each and int(each['length'], 0) == 0:
                continue

            if 'include' in each:
                if each['include']:
                    each['include'] = self.normalize_file_name(each['include'])
                    file_lines[file].extend(['', '- !include %s' % each['include'], ''])
                    file = each['include']
                else:
                    file = '.'
                continue

            if file not in file_lines:
                file_lines[file] = []

            lines = file_lines[file]
            name = each['cname']

            prefix = name[0]
            if prefix == '<':
                level += 1

            padding = '  ' * level
            if prefix not in '<>@':
                padding += '  '
            else:
                name = name[1:]
                if prefix == '@':
                    padding += '    '

            if ':' in name:
                parts = name.split(':')
                name = parts[0]

            padding = padding[2:] if is_configs else padding

            if prefix != '>':
                if 'expand' in each:
                    lines.append('%s- %s' % (padding, each['expand']))
                else:
                    lines.append('%s- %-12s :' % (padding, name))

            for field in each:
                if field in ['cname', 'expand', 'include']:
                    continue
                value_str = self.format_value(field, each[field], padding + ' ' * 16)
                full_line = '  %s  %-12s : %s' % (padding, field, value_str)
                lines.extend(full_line.splitlines())

            if prefix == '>':
                level -= 1
                if level == 0:
                    lines.append('')

        return file_lines


def bsf_to_dsc(bsf_file, dsc_file):
    fsp_dsc = CFspBsf2Dsc(bsf_file)
    dsc_lines = fsp_dsc.get_dsc_lines()
    fd = open(dsc_file, 'w')
    fd.write('\n'.join(dsc_lines))
    fd.close()
    return


def dsc_to_yaml(dsc_file, yaml_file):
    dsc2yaml = CFspDsc2Yaml()
    dsc2yaml.load_config_data_from_dsc(dsc_file)

    cfgs = {}
    for cfg in ['Template', 'Option']:
        if cfg == 'Template':
            file_lines = dsc2yaml.output_template()
        else:
            file_lines = dsc2yaml.output_config()
        for file in file_lines:
            lines = file_lines[file]
            if file == '.':
                cfgs[cfg] = lines
            else:
                if('/' in file or '\\' in file):
                    continue
                file = os.path.basename(file)
                fo = open(os.path.join(file), 'w')
                fo.write(__copyright_tmp__ % (cfg, date.today().year) + '\n\n')
                for line in lines:
                    fo.write(line + '\n')
                fo.close()

    variables = dsc2yaml.output_variable()
    fo = open(yaml_file, 'w')
    fo.write(__copyright_tmp__ % ('Default', date.today().year))
    if len(variables) > 0:
        fo.write('\n\nvariable:\n')
        for line in variables:
            fo.write('  ' + line + '\n')

    fo.write('\n\ntemplate:\n')
    for line in cfgs['Template']:
        if line != '':
            fo.write('  ' + line + '\n')

    fo.write('\n\nconfigs:\n')
    for line in cfgs['Option']:
        if line != '':
            fo.write('  ' + line + '\n')

    fo.close()


def get_fsp_name_from_path(bsf_file):
    name = ''
    parts = bsf_file.split(os.sep)
    for part in parts:
        if part.endswith('FspBinPkg'):
            name = part[:-9]
            break
    if not name:
        raise Exception('Could not get FSP name from file path!')
    return name


def usage():
    print('\n'.join([
          "FspDscBsf2Yaml Version 0.10",
          "Usage:",
          "    FspDscBsf2Yaml  BsfFile|DscFile  YamlFile"
          ]))


def main():
    #
    # Parse the options and args
    #
    argc = len(sys.argv)
    if argc < 3:
        usage()
        return 1

    bsf_file = sys.argv[1]
    yaml_file = sys.argv[2]
    if os.path.isdir(yaml_file):
        yaml_file = os.path.join(yaml_file, get_fsp_name_from_path(bsf_file) + '.yaml')

    if bsf_file.endswith('.dsc'):
        dsc_file = bsf_file
        bsf_file = ''
    else:
        dsc_file = os.path.splitext(yaml_file)[0] + '.dsc'
        bsf_to_dsc(bsf_file, dsc_file)

    dsc_to_yaml(dsc_file, yaml_file)

    print("'%s' was created successfully!" % yaml_file)

    return 0


if __name__ == '__main__':
    sys.exit(main())
