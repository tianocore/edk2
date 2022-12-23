#Converts a string to an EFI_GUID.
from BaseTypes import *
from CommonCtypes import *
from VfrError import *

def StringToGuid(AsciiGuidBuffer:str, GuidBuffer:EFI_GUID):
    Data4 = [0]*8
    #logger = gVfrErrorHandle.getLogger('GenSec')

    if AsciiGuidBuffer == None or GuidBuffer == None:
        return EFI_INVALID_PARAMETER

    #Check Guid Format strictly xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    Index = 0
    while Index < 36:
        if Index == 8 or Index == 13 or Index == 18 or Index == 23:
            if AsciiGuidBuffer[Index] != '-':
                break
        else:
            if (AsciiGuidBuffer[Index] >= '0' and AsciiGuidBuffer[Index] <= '9') or\
                (AsciiGuidBuffer[Index] >= 'a' and AsciiGuidBuffer[Index] <= 'f') or\
                    (AsciiGuidBuffer[Index] >= 'A' and AsciiGuidBuffer[Index] <= 'F'):
                    Index += 1
                    continue
            else:
                break
        Index += 1
        continue

    if Index < 36:
        #logger.error("Invalid option value")
        return EFI_ABORTED

    #Scan the guid string into the buffer
    Index = 11
    try:
        Data1 = int(AsciiGuidBuffer[0:8],16)
        Data2 = int(AsciiGuidBuffer[9:13],16)
        Data3 = int(AsciiGuidBuffer[14:18],16)
        Data4[0] = int(AsciiGuidBuffer[19:21],16)
        Data4[1] = int(AsciiGuidBuffer[21:23],16)
        Data4[2] = int(AsciiGuidBuffer[24:26],16)
        Data4[3] = int(AsciiGuidBuffer[26:28],16)
        Data4[4] = int(AsciiGuidBuffer[28:30],16)
        Data4[5] = int(AsciiGuidBuffer[30:32],16)
        Data4[6] = int(AsciiGuidBuffer[32:34],16)
        Data4[7] = int(AsciiGuidBuffer[34:36],16)
    except:
        #logger.error("Invalid Data value!")
        Index = 0


    #Verify the correct number of items were scanned.
    if Index != 11:
        #logger.error("Invalid option value")
        return EFI_ABORTED

    #Copy the data into our GUID.
    GuidBuffer.Data1     = Data1
    GuidBuffer.Data2     = Data2
    GuidBuffer.Data3     = Data3
    GuidBuffer.Data4[0]  = Data4[0]
    GuidBuffer.Data4[1]  = Data4[1]
    GuidBuffer.Data4[2]  = Data4[2]
    GuidBuffer.Data4[3]  = Data4[3]
    GuidBuffer.Data4[4]  = Data4[4]
    GuidBuffer.Data4[5]  = Data4[5]
    GuidBuffer.Data4[6]  = Data4[6]
    GuidBuffer.Data4[7]  = Data4[7]
    Status = EFI_SUCCESS

    return Status, GuidBuffer