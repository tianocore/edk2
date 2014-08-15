## @file
# This file is used to create report for Eot tool
#
# Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import Common.LongFilePathOs as os
import EotGlobalData
from Common.LongFilePathSupport import OpenLongFilePath as open

## Report() class
#
#  This class defined Report
#
#  @param object: Inherited from object class
#
class Report(object):
    ## The constructor
    #
    #  @param  self: The object pointer
    #  @param  ReportName: name of the report
    #  @param  FvObj: FV object after parsing FV images
    #
    def __init__(self, ReportName = 'Report.html', FvObj = None, DispatchName=None):
        self.ReportName = ReportName
        self.Op = open(ReportName, 'w+')
        self.DispatchList = None
        if DispatchName:
            self.DispatchList = open(DispatchName, 'w+')
        self.FvObj = FvObj
        self.FfsIndex = 0
        self.PpiIndex = 0
        self.ProtocolIndex = 0
        if EotGlobalData.gMACRO['EFI_SOURCE'] == '':
            EotGlobalData.gMACRO['EFI_SOURCE'] = EotGlobalData.gMACRO['EDK_SOURCE']

    ## WriteLn() method
    #
    #  Write a line in the report
    #
    #  @param  self: The object pointer
    #  @param Line:  The lint to be written into
    #
    def WriteLn(self, Line):
        self.Op.write('%s\n' % Line)

    ## GenerateReport() method
    #
    #  A caller to generate report
    #
    #  @param  self: The object pointer
    #
    def GenerateReport(self):
        self.GenerateHeader()
        self.GenerateFv()
        self.GenerateTail()
        self.Op.close()
        self.GenerateUnDispatchedList()

    ## GenerateUnDispatchedList() method
    #
    #  Create a list for not dispatched items
    #
    #  @param  self: The object pointer
    #
    def GenerateUnDispatchedList(self):
        FvObj = self.FvObj
        EotGlobalData.gOP_UN_DISPATCHED.write('%s\n' % FvObj.Name)
        for Item in FvObj.UnDispatchedFfsDict:
            EotGlobalData.gOP_UN_DISPATCHED.write('%s\n' % FvObj.UnDispatchedFfsDict[Item])

    ## GenerateFv() method
    #
    #  Generate FV information
    #
    #  @param  self: The object pointer
    #
    def GenerateFv(self):
        FvObj = self.FvObj
        Content = """  <tr>
    <td width="20%%"><strong>Name</strong></td>
    <td width="60%%"><strong>Guid</strong></td>
    <td width="20%%"><strong>Size</strong></td>
  </tr>"""
        self.WriteLn(Content)

        for Info in FvObj.BasicInfo:
            FvName = Info[0]
            FvGuid = Info[1]
            FvSize = Info[2]

            Content = """  <tr>
    <td>%s</td>
    <td>%s</td>
    <td>%s</td>
  </tr>"""  % (FvName, FvGuid, FvSize)
            self.WriteLn(Content)

        Content = """    <td colspan="3"><table width="100%%"  border="1">
      <tr>"""
        self.WriteLn(Content)

        EotGlobalData.gOP_DISPATCH_ORDER.write('Dispatched:\n')
        for FfsId in FvObj.OrderedFfsDict:
            self.GenerateFfs(FvObj.OrderedFfsDict[FfsId])
        Content = """     </table></td>
  </tr>"""
        self.WriteLn(Content)

        # For UnDispatched
        Content = """    <td colspan="3"><table width="100%%"  border="1">
      <tr>
        <tr><strong>UnDispatched</strong></tr>"""
        self.WriteLn(Content)

        EotGlobalData.gOP_DISPATCH_ORDER.write('\nUnDispatched:\n')
        for FfsId in FvObj.UnDispatchedFfsDict:
            self.GenerateFfs(FvObj.UnDispatchedFfsDict[FfsId])
        Content = """     </table></td>
  </tr>"""
        self.WriteLn(Content)

    ## GenerateDepex() method
    #
    #  Generate Depex information
    #
    #  @param  self: The object pointer
    #  @param DepexString: A DEPEX string needed to be parsed
    #
    def GenerateDepex(self, DepexString):
        NonGuidList = ['AND', 'OR', 'NOT', 'BEFORE', 'AFTER', 'TRUE', 'FALSE']
        ItemList = DepexString.split(' ')
        DepexString = ''
        for Item in ItemList:
            if Item not in NonGuidList:
                SqlCommand = """select DISTINCT GuidName from Report where GuidValue like '%s' and ItemMode = 'Produced' group by GuidName""" % (Item)
                RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
                if RecordSet != []:
                    Item = RecordSet[0][0]
            DepexString = DepexString + Item + ' '
        Content = """                <tr>
                  <td width="5%%"></td>
                  <td width="95%%">%s</td>
                </tr>""" % (DepexString)
        self.WriteLn(Content)

    ## GeneratePpi() method
    #
    #  Generate PPI information
    #
    #  @param self: The object pointer
    #  @param Name: CName of a GUID
    #  @param Guid: Value of a GUID
    #  @param Type: Type of a GUID
    #
    def GeneratePpi(self, Name, Guid, Type):
        self.GeneratePpiProtocol('Ppi', Name, Guid, Type, self.PpiIndex)

    ## GenerateProtocol() method
    #
    #  Generate PROTOCOL information
    #
    #  @param self: The object pointer
    #  @param Name: CName of a GUID
    #  @param Guid: Value of a GUID
    #  @param Type: Type of a GUID
    #
    def GenerateProtocol(self, Name, Guid, Type):
        self.GeneratePpiProtocol('Protocol', Name, Guid, Type, self.ProtocolIndex)

    ## GeneratePpiProtocol() method
    #
    #  Generate PPI/PROTOCOL information
    #
    #  @param self: The object pointer
    #  @param Model: Model of a GUID, PPI or PROTOCOL
    #  @param Name: Name of a GUID
    #  @param Guid: Value of a GUID
    #  @param Type: Type of a GUID
    #  @param CName: CName(Index) of a GUID
    #
    def GeneratePpiProtocol(self, Model, Name, Guid, Type, CName):
        Content = """                <tr>
                  <td width="5%%"></td>
                  <td width="10%%">%s</td>
                  <td width="85%%" colspan="3">%s</td>
                  <!-- %s -->
                </tr>""" % (Model, Name, Guid)
        self.WriteLn(Content)
        if Type == 'Produced':
            SqlCommand = """select DISTINCT SourceFileFullPath, BelongsToFunction from Report where GuidName like '%s' and ItemMode = 'Callback'""" % Name
            RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            for Record in RecordSet:
                SqlCommand = """select FullPath from File
                                where ID = (
                                select DISTINCT BelongsToFile from Inf
                                where Value1 like '%s')""" % Record[0]
                ModuleSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
                Inf = ModuleSet[0][0].replace(EotGlobalData.gMACRO['WORKSPACE'], '.')
                Function = Record[1]
                Address = ''
                for Item in EotGlobalData.gMap:
                    if Function in EotGlobalData.gMap[Item]:
                        Address = EotGlobalData.gMap[Item][Function]
                        break
                    if '_' + Function in EotGlobalData.gMap[Item]:
                        Address = EotGlobalData.gMap[Item]['_' + Function]
                        break
                Content = """                <tr>
                      <td width="5%%"></td>
                      <td width="10%%">%s</td>
                      <td width="40%%">%s</td>
                      <td width="35%%">%s</td>
                      <td width="10%%">%s</td>
                    </tr>""" % ('Callback', Inf, Function, Address)
                self.WriteLn(Content)

    ## GenerateFfs() method
    #
    #  Generate FFS information
    #
    #  @param self: The object pointer
    #  @param FfsObj: FFS object after FV image is parsed
    #
    def GenerateFfs(self, FfsObj):
        self.FfsIndex = self.FfsIndex + 1
        if FfsObj != None and FfsObj.Type in [0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xA]:
            FfsGuid = FfsObj.Guid
            FfsOffset = FfsObj._OFF_
            FfsName = 'Unknown-Module'
            FfsPath = FfsGuid
            FfsType = FfsObj._TypeName[FfsObj.Type]

            # Hard code for Binary INF
            if FfsGuid.upper() == '7BB28B99-61BB-11D5-9A5D-0090273FC14D':
                FfsName = 'Logo'

            if FfsGuid.upper() == '7E374E25-8E01-4FEE-87F2-390C23C606CD':
                FfsName = 'AcpiTables'

            if FfsGuid.upper() == '961578FE-B6B7-44C3-AF35-6BC705CD2B1F':
                FfsName = 'Fat'

            # Find FFS Path and Name
            SqlCommand = """select Value2 from Inf
                            where BelongsToFile = (select BelongsToFile from Inf where Value1 = 'FILE_GUID' and lower(Value2) = lower('%s') and Model = %s)
                            and Model = %s and Value1='BASE_NAME'""" % (FfsGuid, 5001, 5001)
            RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            if RecordSet != []:
                FfsName = RecordSet[0][0]

            SqlCommand = """select FullPath from File
                            where ID = (select BelongsToFile from Inf where Value1 = 'FILE_GUID' and lower(Value2) = lower('%s') and Model = %s)
                            and Model = %s""" % (FfsGuid, 5001, 1011)
            RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            if RecordSet != []:
                FfsPath = RecordSet[0][0]

            Content = """  <tr>
      <tr class='styleFfs' id='FfsHeader%s'>
        <td width="55%%"><span onclick="Display('FfsHeader%s', 'Ffs%s')" onMouseOver="funOnMouseOver()" onMouseOut="funOnMouseOut()">%s</span></td>
        <td width="15%%">%s</td>
        <!--<td width="20%%">%s</td>-->
        <!--<td width="20%%">%s</td>-->
        <td width="10%%">%s</td>
      </tr>
      <tr id='Ffs%s' style='display:none;'>
        <td colspan="4"><table width="100%%"  border="1">""" % (self.FfsIndex, self.FfsIndex, self.FfsIndex, FfsPath, FfsName, FfsGuid, FfsOffset, FfsType, self.FfsIndex)
            
            if self.DispatchList:
                if FfsObj.Type in [0x04, 0x06]:
                    self.DispatchList.write("%s %s %s %s\n" % (FfsGuid, "P", FfsName, FfsPath))
                if FfsObj.Type in [0x05, 0x07, 0x08, 0x0A]:
                    self.DispatchList.write("%s %s %s %s\n" % (FfsGuid, "D", FfsName, FfsPath))
               
            self.WriteLn(Content)

            EotGlobalData.gOP_DISPATCH_ORDER.write('%s\n' %FfsName)

            if FfsObj.Depex != '':
                Content = """          <tr>
            <td><span id='DepexHeader%s' class="styleDepex" onclick="Display('DepexHeader%s', 'Depex%s')" onMouseOver="funOnMouseOver()" onMouseOut="funOnMouseOut()">&nbsp&nbspDEPEX expression</span></td>
          </tr>
          <tr id='Depex%s' style='display:none;'>
            <td><table width="100%%"  border="1">""" % (self.FfsIndex, self.FfsIndex, self.FfsIndex, self.FfsIndex)
                self.WriteLn(Content)
                self.GenerateDepex(FfsObj.Depex)
                Content = """            </table></td>
          </tr>"""
                self.WriteLn(Content)
            # End of DEPEX

            # Find Consumed Ppi/Protocol
            SqlCommand = """select ModuleName, ItemType, GuidName, GuidValue, GuidMacro from Report
                            where SourceFileFullPath in
                            (select Value1 from Inf where BelongsToFile =
                            (select BelongsToFile from Inf
                            where Value1 = 'FILE_GUID' and Value2 like '%s' and Model = %s)
                            and Model = %s)
                            and ItemMode = 'Consumed' group by GuidName order by ItemType""" \
                            % (FfsGuid, 5001, 3007)

            RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            if RecordSet != []:
                Count = len(RecordSet)
                Content = """          <tr>
            <td><span id='ConsumedHeader%s' class="styleConsumed" onclick="Display('ConsumedHeader%s', 'Consumed%s')" onMouseOver="funOnMouseOver()" onMouseOut="funOnMouseOut()">&nbsp&nbspConsumed Ppis/Protocols List (%s)</span></td>
          </tr>
          <tr id='Consumed%s' style='display:none;'>
            <td><table width="100%%"  border="1">""" % (self.FfsIndex, self.FfsIndex, self.FfsIndex, Count, self.FfsIndex)
                self.WriteLn(Content)
                self.ProtocolIndex = 0
                for Record in RecordSet:
                    self.ProtocolIndex = self.ProtocolIndex + 1
                    Name = Record[2]
                    CName = Record[4]
                    Guid = Record[3]
                    Type = Record[1]
                    self.GeneratePpiProtocol(Type, Name, Guid, 'Consumed', CName)

                Content = """            </table></td>
          </tr>"""
                self.WriteLn(Content)
            #End of Consumed Ppi/Portocol

            # Find Produced Ppi/Protocol
            SqlCommand = """select ModuleName, ItemType, GuidName, GuidValue, GuidMacro from Report
                            where SourceFileFullPath in
                            (select Value1 from Inf where BelongsToFile =
                            (select BelongsToFile from Inf
                            where Value1 = 'FILE_GUID' and Value2 like '%s' and Model = %s)
                            and Model = %s)
                            and ItemMode = 'Produced' group by GuidName order by ItemType""" \
                            % (FfsGuid, 5001, 3007)

            RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            if RecordSet != []:
                Count = len(RecordSet)
                Content = """          <tr>
            <td><span id='ProducedHeader%s' class="styleProduced" onclick="Display('ProducedHeader%s', 'Produced%s')" onMouseOver="funOnMouseOver()" onMouseOut="funOnMouseOut()">&nbsp&nbspProduced Ppis/Protocols List (%s)</span></td>
          </tr>
          <tr id='Produced%s' style='display:none;'>
            <td><table width="100%%"  border="1">""" % (self.FfsIndex, self.FfsIndex, self.FfsIndex, Count, self.FfsIndex)
                self.WriteLn(Content)
                self.PpiIndex = 0
                for Record in RecordSet:
                    self.PpiIndex = self.PpiIndex + 1
                    Name = Record[2]
                    CName = Record[4]
                    Guid = Record[3]
                    Type = Record[1]
                    self.GeneratePpiProtocol(Type, Name, Guid, 'Produced', CName)

                Content = """            </table></td>
          </tr>"""
                self.WriteLn(Content)
            RecordSet = None
            # End of Produced Ppi/Protocol

            Content = """        </table></td>
        </tr>"""
            self.WriteLn(Content)

    ## GenerateTail() method
    #
    #  Generate end tags of HTML report
    #
    #  @param self: The object pointer
    #
    def GenerateTail(self):
        Tail = """</table>
</body>
</html>"""
        self.WriteLn(Tail)

    ## GenerateHeader() method
    #
    #  Generate start tags of HTML report
    #
    #  @param self: The object pointer
    #
    def GenerateHeader(self):
        Header = """<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>Execution Order Tool Report</title>
<meta http-equiv="Content-Type" content="text/html">
<style type="text/css">
<!--
.styleFfs {
    color: #006600;
    font-weight: bold;
}
.styleDepex {
    color: #FF0066;
    font-weight: bold;
}
.styleProduced {
    color: #0000FF;
    font-weight: bold;
}
.styleConsumed {
    color: #FF00FF;
    font-weight: bold;
}
-->
</style>
<Script type="text/javascript">
function Display(ParentID, SubID)
{
    SubItem = document.getElementById(SubID);
    ParentItem = document.getElementById(ParentID);
    if (SubItem.style.display == 'none')
    {
        SubItem.style.display = ''
        ParentItem.style.fontWeight = 'normal'
    }
    else
    {
        SubItem.style.display = 'none'
        ParentItem.style.fontWeight = 'bold'
    }

}

function funOnMouseOver()
{
    document.body.style.cursor = "hand";
}

function funOnMouseOut()
{
    document.body.style.cursor = "";
}

</Script>
</head>

<body>
<table width="100%%"  border="1">"""
        self.WriteLn(Header)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    # Initialize log system
    FilePath = 'FVRECOVERYFLOPPY.fv'
    if FilePath.lower().endswith(".fv"):
        fd = open(FilePath, 'rb')
        buf = array('B')
        try:
            buf.fromfile(fd, os.path.getsize(FilePath))
        except EOFError:
            pass

        fv = FirmwareVolume("FVRECOVERY", buf, 0)

    report = Report('Report.html', fv)
    report.GenerateReport()
