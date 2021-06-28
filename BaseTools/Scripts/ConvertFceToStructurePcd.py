#!/usr/bin/python
## @file
# Firmware Configuration Editor (FCE) from https://firmware.intel.com/develop
# can parse BIOS image and generate Firmware Configuration file.
# This script bases on Firmware Configuration file, and generate the structure
# PCD setting in DEC/DSC/INF files.
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
ConvertFceToStructurePcd
'''

import re
import os
import datetime
import argparse

#
# Globals for help information
#
__prog__        = 'ConvertFceToStructurePcd'
__version__     = '%s Version %s' % (__prog__, '0.1 ')
__copyright__   = 'Copyright (c) 2018, Intel Corporation. All rights reserved.'
__description__ = 'Generate Structure PCD in DEC/DSC/INF based on Firmware Configuration.\n'


dscstatement='''[Defines]
  VPD_TOOL_GUID                  = 8C3D856A-9BE6-468E-850A-24F7A8D38E08

[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

[DefaultStores]
  0|STANDARD             # UEFI Standard default  0|STANDARD is reserved.
  1|MANUFACTURING        # UEFI Manufacturing default 1|MANUFACTURING is reserved.

[PcdsDynamicExVpd.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdNvStoreDefaultValueBuffer|*
'''

decstatement = '''[Guids]
  gStructPcdTokenSpaceGuid = {0x3f1406f4, 0x2b, 0x487a, {0x8b, 0x69, 0x74, 0x29, 0x1b, 0x36, 0x16, 0xf4}}

[PcdsFixedAtBuild,PcdsPatchableInModule,PcdsDynamic,PcdsDynamicEx]
'''

infstatement = '''[Pcd]
'''

SECTION='PcdsDynamicHii'
PCD_NAME='gStructPcdTokenSpaceGuid.Pcd'
Max_Pcd_Len = 100

WARNING=[]
ERRORMSG=[]

class parser_lst(object):

  def __init__(self,filelist):
    self._ignore=['BOOLEAN', 'UINT8', 'UINT16', 'UINT32', 'UINT64']
    self.file=filelist
    self.text=self.megre_lst()[0]
    self.content=self.megre_lst()[1]

  def megre_lst(self):
    alltext=''
    content={}
    for file in self.file:
      with open(file,'r') as f:
        read =f.read()
      alltext += read
      content[file]=read
    return alltext,content

  def struct_lst(self):#{struct:lst file}
    structs_file={}
    name_format = re.compile(r'(?<!typedef)\s+struct (\w+) {.*?;', re.S)
    for i in list(self.content.keys()):
      structs= name_format.findall(self.content[i])
      if structs:
        for j in structs:
          if j not in self._ignore:
            structs_file[j]=i
      else:
        print("%s"%structs)
    return structs_file

  def struct(self):#struct:{offset:name}
    unit_num = re.compile('(\d+)')
    offset1_re = re.compile('(\d+)\[')
    pcdname_num_re = re.compile('\w+\[(\S+)\]')
    pcdname_re = re.compile('\](.*)\<')
    pcdname2_re = re.compile('(\w+)\[')
    uint_re = re.compile('\<(\S+)\>')
    name_format = re.compile(r'(?<!typedef)\s+struct (\w+) {.*?;', re.S)
    name=name_format.findall(self.text)
    info={}
    unparse=[]
    if name:
      tmp_n = [n for n in name if n not in self._ignore]
      name = list(set(tmp_n))
      name.sort(key = tmp_n.index)
      name.reverse()
      #name=list(set(name).difference(set(self._ignore)))
      for struct in name:
        s_re = re.compile(r'struct %s :(.*?)};'% struct, re.S)
        content = s_re.search(self.text)
        if content:
          tmp_dict = {}
          text = content.group().split('+')
          for line in text[1:]:
            offset = offset1_re.findall(line)
            t_name = pcdname_re.findall(line)
            uint = uint_re.findall(line)
            if offset and uint:
              offset = offset[0]
              uint = uint[0]
              if t_name:
                t_name = t_name[0].strip()
                if (' ' in t_name) or ("=" in t_name) or (";" in t_name) or("\\" in name) or (t_name ==''):
                  WARNING.append("Warning:Invalid Pcd name '%s' for Offset %s in struct %s" % (t_name,offset, struct))
                else:
                  if '[' in t_name:
                    if uint in ['UINT8', 'UINT16', 'UINT32', 'UINT64']:
                      offset = int(offset, 10)
                      tmp_name = pcdname2_re.findall(t_name)[0] + '[0]'
                      tmp_dict[offset] = tmp_name
                      pcdname_num = int(pcdname_num_re.findall(t_name)[0],10)
                      uint = int(unit_num.findall(uint)[0],10)
                      bit = uint // 8
                      for i in range(1, pcdname_num):
                        offset += bit
                        tmp_name = pcdname2_re.findall(t_name)[0] + '[%s]' % i
                        tmp_dict[offset] = tmp_name
                    else:
                      tmp_name = pcdname2_re.findall(t_name)[0]
                      pcdname_num = pcdname_num_re.findall(t_name)[0]
                      line = [offset,tmp_name,pcdname_num,uint]
                      line.append(struct)
                      unparse.append(line)
                  else:
                    if uint not in ['UINT8', 'UINT16', 'UINT32', 'UINT64']:
                      line = [offset, t_name, 0, uint]
                      line.append(struct)
                      unparse.append(line)
                    else:
                      offset = int(offset,10)
                      tmp_dict[offset] = t_name
        info[struct] = tmp_dict
      if len(unparse) != 0:
        for u in unparse:
          if u[3] in list(info.keys()):
            unpar = self.nameISstruct(u,info[u[3]])
            info[u[4]]= dict(list(info[u[4]].items())+list(unpar[u[4]].items()))
    else:
      print("ERROR: No struct name found in %s" % self.file)
      ERRORMSG.append("ERROR: No struct name found in %s" % self.file)
    return info


  def nameISstruct(self,line,key_dict):
    dict={}
    dict2={}
    s_re = re.compile(r'struct %s :(.*?)};' % line[3], re.S)
    size_re = re.compile(r'mTotalSize \[(\S+)\]')
    content = s_re.search(self.text)
    if content:
      s_size = size_re.findall(content.group())[0]
    else:
      s_size = '0'
      print("ERROR: Struct %s not define mTotalSize in lst file" %line[3])
      ERRORMSG.append("ERROR: Struct %s not define mTotalSize in lst file" %line[3])
    size = int(line[0], 10)
    if line[2] != 0:
      for j in range(0, int(line[2], 10)):
        for k in list(key_dict.keys()):
          offset = size  + k
          name ='%s.%s' %((line[1]+'[%s]'%j),key_dict[k])
          dict[offset] = name
        size = int(s_size,16)+size
    elif line[2] == 0:
      for k in list(key_dict.keys()):
        offset = size + k
        name = '%s.%s' % (line[1], key_dict[k])
        dict[offset] = name
    dict2[line[4]] = dict
    return dict2

  def efivarstore_parser(self):
    efivarstore_format = re.compile(r'efivarstore.*?;', re.S)
    struct_re = re.compile(r'efivarstore(.*?),',re.S)
    name_re = re.compile(r'name=(\w+)')
    efivarstore_dict={}
    efitxt = efivarstore_format.findall(self.text)
    for i in efitxt:
      struct = struct_re.findall(i.replace(' ',''))
      if struct[0] in self._ignore:
          continue
      name = name_re.findall(i.replace(' ',''))
      if struct and name:
        efivarstore_dict[name[0]]=struct[0]
      else:
        print("ERROR: Can't find Struct or name in lst file, please check have this format:efivarstore XXXX, name=xxxx")
        ERRORMSG.append("ERROR: Can't find Struct or name in lst file, please check have this format:efivarstore XXXX, name=xxxx")
    return efivarstore_dict

class Config(object):

  def __init__(self,Config):
    self.config=Config

  #Parser .config file,return list[offset,name,guid,value,help]
  def config_parser(self):
    ids_re =re.compile('_ID:(\d+)',re.S)
    id_re= re.compile('\s+')
    info = []
    info_dict={}
    with open(self.config, 'r') as text:
      read = text.read()
    if 'DEFAULT_ID:' in read:
      all_txt = read.split('FCEKEY DEFAULT')
      for i in all_txt[1:]:
        part = [] #save all infomation for DEFAULT_ID
        str_id=''
        ids = ids_re.findall(i.replace(' ',''))
        for m in ids:
          str_id +=m+'_'
        str_id=str_id[:-1]
        part.append(ids)
        section = i.split('\nQ') #split with '\nQ ' to get every block
        part +=self.section_parser(section)
        info_dict[str_id] = self.section_parser(section)
        info.append(part)
    else:
      part = []
      id=('0','0')
      str_id='0_0'
      part.append(id)
      section = read.split('\nQ')
      part +=self.section_parser(section)
      info_dict[str_id] = self.section_parser(section)
      info.append(part)
    return info_dict

  def eval_id(self,id):
    id = id.split("_")
    default_id=id[0:len(id)//2]
    platform_id=id[len(id)//2:]
    text=''
    for i in range(len(default_id)):
      text +="%s.common.%s.%s,"%(SECTION,self.id_name(platform_id[i],'PLATFORM'),self.id_name(default_id[i],'DEFAULT'))
    return '\n[%s]\n'%text[:-1]

  def id_name(self,ID, flag):
    platform_dict = {'0': 'DEFAULT'}
    default_dict = {'0': 'STANDARD', '1': 'MANUFACTURING'}
    if flag == "PLATFORM":
      try:
        value = platform_dict[ID]
      except KeyError:
        value = 'SKUID%s' % ID
    elif flag == 'DEFAULT':
      try:
        value = default_dict[ID]
      except KeyError:
        value = 'DEFAULTID%s' % ID
    else:
      value = None
    return value

  def section_parser(self,section):
    offset_re = re.compile(r'offset=(\w+)')
    name_re = re.compile(r'name=(\S+)')
    guid_re = re.compile(r'guid=(\S+)')
  #  help_re = re.compile(r'help = (.*)')
    attribute_re=re.compile(r'attribute=(\w+)')
    value_re = re.compile(r'(//.*)')
    part = []
    part_without_comment = []
    for x in section[1:]:
        line=x.split('\n')[0]
        comment_list = value_re.findall(line) # the string \\... in "Q...." line
        comment_list[0] = comment_list[0].replace('//', '')
        comment_ori = comment_list[0].strip()
        comment = ""
        for each in comment_ori:
            if each != " " and "\x21" > each or each > "\x7E":
                if bytes(each, 'utf-16') == b'\xff\xfe\xae\x00':
                    each = '(R)'
                else:
                    each = ""
            comment += each
        line=value_re.sub('',line) #delete \\... in "Q...." line
        list1=line.split(' ')
        value=self.value_parser(list1)
        offset = offset_re.findall(x.replace(' ',''))
        name = name_re.findall(x.replace(' ',''))
        guid = guid_re.findall(x.replace(' ',''))
        attribute =attribute_re.findall(x.replace(' ',''))
        if offset and name and guid and value and attribute:
          if attribute[0] in ['0x3','0x7']:
            offset = int(offset[0], 16)
            #help = help_re.findall(x)
            text_without_comment = offset, name[0], guid[0], value, attribute[0]
            if text_without_comment in part_without_comment:
                # check if exists same Pcd with different comments, add different comments in one line with "|".
                dupl_index = part_without_comment.index(text_without_comment)
                part[dupl_index] = list(part[dupl_index])
                if comment not in part[dupl_index][-1]:
                    part[dupl_index][-1] += " | " + comment
                part[dupl_index] = tuple(part[dupl_index])
            else:
                text = offset, name[0], guid[0], value, attribute[0], comment
                part_without_comment.append(text_without_comment)
                part.append(text)
    return(part)

  def value_parser(self, list1):
    list1 = [t for t in list1 if t != '']  # remove '' form list
    first_num = int(list1[0], 16)
    if list1[first_num + 1] == 'STRING':  # parser STRING
      if list1[-1] == '""':
        value = "{0x0, 0x0}"
      else:
        value = 'L%s' % list1[-1]
    elif list1[first_num + 1] == 'ORDERED_LIST':  # parser ORDERED_LIST
      value_total = int(list1[first_num + 2])
      list2 = list1[-value_total:]
      tmp = []
      line = ''
      for i in list2:
        if len(i) % 2 == 0 and len(i) != 2:
          for m in range(0, len(i) // 2):
            tmp.append('0x%02x' % (int('0x%s' % i, 16) >> m * 8 & 0xff))
        else:
          tmp.append('0x%s' % i)
      for i in tmp:
        line += '%s,' % i
      value = '{%s}' % line[:-1]
    else:
      value = "0x%01x" % int(list1[-1], 16)
    return value


#parser Guid file, get guid name form guid value
class GUID(object):

  def __init__(self,path):
    self.path = path
    self.guidfile = self.gfile()
    self.guiddict = self.guid_dict()

  def gfile(self):
    for root, dir, file in os.walk(self.path, topdown=True, followlinks=False):
      if 'FV' in dir:
        gfile = os.path.join(root,'Fv','Guid.xref')
        if os.path.isfile(gfile):
          return gfile
        else:
          print("ERROR: Guid.xref file not found")
          ERRORMSG.append("ERROR: Guid.xref file not found")
          exit()

  def guid_dict(self):
    guiddict={}
    with open(self.guidfile,'r') as file:
      lines = file.readlines()
    guidinfo=lines
    for line in guidinfo:
      list=line.strip().split(' ')
      if list:
        if len(list)>1:
          guiddict[list[0].upper()]=list[1]
        elif list[0] != ''and len(list)==1:
          print("Error: line %s can't be parser in %s"%(line.strip(),self.guidfile))
          ERRORMSG.append("Error: line %s can't be parser in %s"%(line.strip(),self.guidfile))
      else:
        print("ERROR: No data in %s" %self.guidfile)
        ERRORMSG.append("ERROR: No data in %s" %self.guidfile)
    return guiddict

  def guid_parser(self,guid):
    if guid.upper() in self.guiddict:
      return self.guiddict[guid.upper()]
    else:
      print("ERROR: GUID %s not found in file %s"%(guid, self.guidfile))
      ERRORMSG.append("ERROR: GUID %s not found in file %s"%(guid, self.guidfile))
      return guid

class PATH(object):

  def __init__(self,path):
    self.path=path
    self.rootdir=self.get_root_dir()
    self.usefuldir=set()
    self.lstinf = {}
    for path in self.rootdir:
      for o_root, o_dir, o_file in os.walk(os.path.join(path, "OUTPUT"), topdown=True, followlinks=False):
        for INF in o_file:
          if os.path.splitext(INF)[1] == '.inf':
            for l_root, l_dir, l_file in os.walk(os.path.join(path, "DEBUG"), topdown=True,
                               followlinks=False):
              for LST in l_file:
                if os.path.splitext(LST)[1] == '.lst':
                  self.lstinf[os.path.join(l_root, LST)] = os.path.join(o_root, INF)
                  self.usefuldir.add(path)

  def get_root_dir(self):
    rootdir=[]
    for root,dir,file in os.walk(self.path,topdown=True,followlinks=False):
      if "OUTPUT" in root:
        updir=root.split("OUTPUT",1)[0]
        rootdir.append(updir)
    rootdir=list(set(rootdir))
    return rootdir

  def lst_inf(self):
    return self.lstinf

  def package(self):
    package={}
    package_re=re.compile(r'Packages\.\w+]\n(.*)',re.S)
    for i in list(self.lstinf.values()):
      with open(i,'r') as inf:
        read=inf.read()
      section=read.split('[')
      for j in section:
        p=package_re.findall(j)
        if p:
          package[i]=p[0].rstrip()
    return package

  def header(self,struct):
    header={}
    head_re = re.compile('typedef.*} %s;[\n]+(.*)(?:typedef|formset)'%struct,re.M|re.S)
    head_re2 = re.compile(r'#line[\s\d]+"(\S+h)"')
    for i in list(self.lstinf.keys()):
      with open(i,'r') as lst:
        read = lst.read()
      h = head_re.findall(read)
      if h:
        head=head_re2.findall(h[0])
        if head:
          format = head[0].replace('\\\\','/').replace('\\','/')
          name =format.split('/')[-1]
          head = self.headerfileset.get(name)
          if head:
            head = head.replace('\\','/')
            header[struct] = head
    return header
  @property
  def headerfileset(self):
    headerset = dict()
    for root,dirs,files in os.walk(self.path):
      for file in files:
        if os.path.basename(file) == 'deps.txt':
          with open(os.path.join(root,file),"r") as fr:
            for line in fr.readlines():
              headerset[os.path.basename(line).strip()] = line.strip()
    return headerset

  def makefile(self,filename):
    re_format = re.compile(r'DEBUG_DIR.*(?:\S+Pkg)\\(.*\\%s)'%filename)
    for i in self.usefuldir:
      with open(os.path.join(i,'Makefile'),'r') as make:
        read = make.read()
      dir = re_format.findall(read)
      if dir:
        return dir[0]
    return None

class mainprocess(object):

  def __init__(self,InputPath,Config,OutputPath):
    self.init = 0xFCD00000
    self.inputpath = os.path.abspath(InputPath)
    self.outputpath = os.path.abspath(OutputPath)
    self.LST = PATH(self.inputpath)
    self.lst_dict = self.LST.lst_inf()
    self.Config = Config
    self.attribute_dict = {'0x3': 'NV, BS', '0x7': 'NV, BS, RT'}
    self.guid = GUID(self.inputpath)
    self.header={}

  def main(self):
    conf=Config(self.Config)
    config_dict=conf.config_parser() #get {'0_0':[offset,name,guid,value,attribute]...,'1_0':....}
    lst=parser_lst(list(self.lst_dict.keys()))
    efi_dict=lst.efivarstore_parser() #get {name:struct} form lst file
    keys=sorted(config_dict.keys())
    all_struct=lst.struct()
    stru_lst=lst.struct_lst()
    title_list=[]
    info_list=[]
    header_list=[]
    inf_list =[]
    for i in stru_lst:
      tmp = self.LST.header(i)
      self.header.update(tmp)
    for id_key in keys:
      tmp_id=[id_key] #['0_0',[(struct,[name...]),(struct,[name...])]]
      tmp_info={} #{name:struct}
      for section in config_dict[id_key]:
        c_offset,c_name,c_guid,c_value,c_attribute,c_comment = section
        if c_name in efi_dict:
          struct = efi_dict[c_name]
          title='%s%s|L"%s"|%s|0x00||%s\n'%(PCD_NAME,c_name,c_name,self.guid.guid_parser(c_guid),self.attribute_dict[c_attribute])
          if struct in all_struct:
            lstfile = stru_lst[struct]
            struct_dict=all_struct[struct]
            try:
              title2 = '%s%s|{0}|%s|0xFCD00000{\n <HeaderFiles>\n  %s\n <Packages>\n%s\n}\n' % (PCD_NAME, c_name, struct, self.header[struct], self.LST.package()[self.lst_dict[lstfile]])
            except KeyError:
              WARNING.append("Warning: No <HeaderFiles> for struct %s"%struct)
              title2 = '%s%s|{0}|%s|0xFCD00000{\n <HeaderFiles>\n  %s\n <Packages>\n%s\n}\n' % (PCD_NAME, c_name, struct, '', self.LST.package()[self.lst_dict[lstfile]])
            header_list.append(title2)
          elif struct not in lst._ignore:
            struct_dict ={}
            print("ERROR: Struct %s can't found in lst file" %struct)
            ERRORMSG.append("ERROR: Struct %s can't found in lst file" %struct)
          if c_offset in struct_dict:
            offset_name=struct_dict[c_offset]
            info = "%s%s.%s|%s\n"%(PCD_NAME,c_name,offset_name,c_value)
            blank_length = Max_Pcd_Len - len(info)
            if blank_length <= 0:
                info_comment = "%s%s.%s|%s%s# %s\n"%(PCD_NAME,c_name,offset_name,c_value,"     ",c_comment)
            else:
                info_comment = "%s%s.%s|%s%s# %s\n"%(PCD_NAME,c_name,offset_name,c_value,blank_length*" ",c_comment)
            inf = "%s%s\n"%(PCD_NAME,c_name)
            inf_list.append(inf)
            tmp_info[info_comment]=title
          else:
            print("ERROR: Can't find offset %s with struct name %s"%(c_offset,struct))
            ERRORMSG.append("ERROR: Can't find offset %s with name %s"%(c_offset,struct))
        else:
          print("ERROR: Can't find name %s in lst file"%(c_name))
          ERRORMSG.append("ERROR: Can't find name %s in lst file"%(c_name))
      tmp_id.append(list(self.reverse_dict(tmp_info).items()))
      id,tmp_title_list,tmp_info_list = self.read_list(tmp_id)
      title_list +=tmp_title_list
      info_list.append(tmp_info_list)
    inf_list = self.del_repeat(inf_list)
    header_list = self.plus(self.del_repeat(header_list))
    title_all=list(set(title_list))
    info_list = self.remove_bracket(self.del_repeat(info_list))
    for i in range(len(info_list)-1,-1,-1):
      if len(info_list[i]) == 0:
        info_list.remove(info_list[i])
    for i in (inf_list, title_all, header_list):
      i.sort()
    return keys,title_all,info_list,header_list,inf_list

  def correct_sort(self, PcdString):
    # sort the Pcd list with two rules:
    # First sort through Pcd name;
    # Second if the Pcd exists several elements, sort them through index value.
    if ("]|") in PcdString:
        Pcdname = PcdString.split("[")[0]
        Pcdindex = int(PcdString.split("[")[1].split("]")[0])
    else:
        Pcdname = PcdString.split("|")[0]
        Pcdindex = 0
    return Pcdname, Pcdindex

  def remove_bracket(self,List):
    for i in List:
      for j in i:
        tmp = j.split("|")
        if (('L"' in j) and ("[" in j)) or (tmp[1].strip() == '{0x0, 0x0}'):
          tmp[0] = tmp[0][:tmp[0].index('[')]
          List[List.index(i)][i.index(j)] = "|".join(tmp)
        else:
          List[List.index(i)][i.index(j)] = j
    for i in List:
      if type(i) == type([0,0]):
        i.sort(key = lambda x:(self.correct_sort(x)[0], self.correct_sort(x)[1]))
    return List

  def write_all(self):
    title_flag=1
    info_flag=1
    if not os.path.isdir(self.outputpath):
      os.makedirs(self.outputpath)
    decwrite = write2file(os.path.join(self.outputpath,'StructurePcd.dec'))
    dscwrite = write2file(os.path.join(self.outputpath,'StructurePcd.dsc'))
    infwrite = write2file(os.path.join(self.outputpath, 'StructurePcd.inf'))
    conf = Config(self.Config)
    ids,title,info,header,inf=self.main()
    decwrite.add2file(decstatement)
    decwrite.add2file(header)
    infwrite.add2file(infstatement)
    infwrite.add2file(inf)
    dscwrite.add2file(dscstatement)
    for id in ids:
      dscwrite.add2file(conf.eval_id(id))
      if title_flag:
        dscwrite.add2file(title)
        title_flag=0
      if len(info) == 1:
        dscwrite.add2file(info)
      elif len(info) == 2:
        if info_flag:
          dscwrite.add2file(info[0])
          info_flag =0
        else:
          dscwrite.add2file(info[1])

  def del_repeat(self,List):
    if len(List) == 1 or len(List) == 0:
      return List
    else:
      if type(List[0]) != type('xxx'):
        alist=[]
        for i in range(len(List)):
          if i == 0:
            alist.append(List[0])
          else:
            plist = []
            for j in range(i):
              plist += List[j]
            alist.append(self.__del(list(set(plist)), List[i]))
        return alist
      else:
        return list(set(List))


  def __del(self,list1,list2):
    return list(set(list2).difference(set(list1)))

  def reverse_dict(self,dict):
    data={}
    for i in list(dict.items()):
      if i[1] not in list(data.keys()):
        data[i[1]]=[i[0]]
      else:
        data[i[1]].append(i[0])
    return data

  def read_list(self,list):
    title_list=[]
    info_list=[]
    for i in list[1]:
      title_list.append(i[0])
      for j in i[1]:
        info_list.append(j)
    return list[0],title_list,info_list

  def plus(self,list):
    nums=[]
    for i in list:
      if type(i) != type([0]):
        self.init += 1
        num = "0x%01x" % self.init
        j=i.replace('0xFCD00000',num.upper())
        nums.append(j)
    return nums

class write2file(object):

  def __init__(self,Output):
    self.output=Output
    self.text=''
    if os.path.exists(self.output):
      os.remove(self.output)

  def add2file(self,content):
    self.text = ''
    with open(self.output,'a+') as file:
      file.write(self.__gen(content))

  def __gen(self,content):
    if type(content) == type(''):
      return content
    elif type(content) == type([0,0])or type(content) == type((0,0)):
      return self.__readlist(content)
    elif type(content) == type({0:0}):
      return self.__readdict(content)

  def __readlist(self,list):
    for i in list:
      if type(i) == type([0,0])or type(i) == type((0,0)):
        self.__readlist(i)
      elif type(i) == type('') :
        self.text +=i
    return self.text

  def __readdict(self,dict):
    content=list(dict.items())
    return self.__readlist(content)

def stamp():
  return datetime.datetime.now()

def dtime(start,end,id=None):
  if id:
    pass
    print("%s time:%s" % (id,str(end - start)))
  else:
    print("Total time:%s" %str(end-start)[:-7])


def main():
  start = stamp()
  parser = argparse.ArgumentParser(prog = __prog__,
                                   description = __description__ + __copyright__,
                                   conflict_handler = 'resolve')
  parser.add_argument('-v', '--version', action = 'version',version = __version__, help="show program's version number and exit")
  parser.add_argument('-p', '--path', metavar='PATH', dest='path', help="platform build output directory")
  parser.add_argument('-c', '--config',metavar='FILENAME', dest='config', help="firmware configuration file")
  parser.add_argument('-o', '--outputdir', metavar='PATH', dest='output', help="output directoy")
  options = parser.parse_args()
  if options.config:
    if options.path:
      if options.output:
        run = mainprocess(options.path, options.config, options.output)
        print("Running...")
        run.write_all()
        if WARNING:
          warning = list(set(WARNING))
          for j in warning:
            print(j)
        if ERRORMSG:
          ERROR = list(set(ERRORMSG))
          with open("ERROR.log", 'w+') as error:
            for i in ERROR:
              error.write(i + '\n')
          print("Some error find, error log in ERROR.log")
        print('Finished, Output files in directory %s'%os.path.abspath(options.output))
      else:
        print('Error command, no output path, use -h for help')
    else:
      print('Error command, no build path input, use -h for help')
  else:
    print('Error command, no output file, use -h for help')
  end = stamp()
  dtime(start, end)

if __name__ == '__main__':
  main()
