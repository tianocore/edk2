from CommonCtypes import *
# Enumeration of EFI_STATUS.
MAX_BIT   =  0x8000000000000000
def ENCODE_ERROR(a):
    return MAX_BIT | a

def ENCODE_WARNING(a):
    return a

def RETURN_ERROR(a):
    return a < 0

#define ENCODE_ERROR(a)              ((RETURN_STATUS)(MAX_BIT | (a)))

#define ENCODE_WARNING(a)            ((RETURN_STATUS)(a))
#define RETURN_ERROR(a)              (((INTN)(RETURN_STATUS)(a)) < 0)

RETURN_SUCCESS   =   0
#define RETURN_LOAD_ERROR            ENCODE_ERROR (1)
#define RETURN_INVALID_PARAMETER     ENCODE_ERROR (2)
#define RETURN_UNSUPPORTED           ENCODE_ERROR (3)
#define RETURN_BAD_BUFFER_SIZE       ENCODE_ERROR (4)
#define RETURN_BUFFER_TOO_SMALL      ENCODE_ERROR (5)
#define RETURN_NOT_READY             ENCODE_ERROR (6)
#define RETURN_DEVICE_ERROR          ENCODE_ERROR (7)
#define RETURN_WRITE_PROTECTED       ENCODE_ERROR (8)
#define RETURN_OUT_OF_RESOURCES      ENCODE_ERROR (9)
#define RETURN_VOLUME_CORRUPTED      ENCODE_ERROR (10)
#define RETURN_VOLUME_FULL           ENCODE_ERROR (11)
#define RETURN_NO_MEDIA              ENCODE_ERROR (12)
#define RETURN_MEDIA_CHANGED         ENCODE_ERROR (13)
#define RETURN_NOT_FOUND             ENCODE_ERROR (14)
#define RETURN_ACCESS_DENIED         ENCODE_ERROR (15)
#define RETURN_NO_RESPONSE           ENCODE_ERROR (16)
#define RETURN_NO_MAPPING            ENCODE_ERROR (17)
#define RETURN_TIMEOUT               ENCODE_ERROR (18)
#define RETURN_NOT_STARTED           ENCODE_ERROR (19)
#define RETURN_ALREADY_STARTED       ENCODE_ERROR (20)
#define RETURN_ABORTED               ENCODE_ERROR (21)
#define RETURN_ICMP_ERROR            ENCODE_ERROR (22)
#define RETURN_TFTP_ERROR            ENCODE_ERROR (23)
#define RETURN_PROTOCOL_ERROR        ENCODE_ERROR (24)
#define RETURN_INCOMPATIBLE_VERSION  ENCODE_ERROR (25)
#define RETURN_SECURITY_VIOLATION    ENCODE_ERROR (26)
#define RETURN_CRC_ERROR             ENCODE_ERROR (27)
#define RETURN_END_OF_MEDIA          ENCODE_ERROR (28)
#define RETURN_END_OF_FILE           ENCODE_ERROR (31)

#define RETURN_WARN_UNKNOWN_GLYPH    ENCODE_WARNING (1)
#define RETURN_WARN_DELETE_FAILURE   ENCODE_WARNING (2)
#define RETURN_WARN_WRITE_FAILURE    ENCODE_WARNING (3)
#define RETURN_WARN_BUFFER_TOO_SMALL ENCODE_WARNING (4)
EFI_SUCCESS  = RETURN_SUCCESS

#Converts a string to an EFI_GUID.
def StringToGuid(AsciiGuidBuffer:str,GuidBuffer:EFI_GUID):
    Data4 = [0]*8
    logger =logging.getLogger('GenSec')

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
        logger.error("Invalid option value")
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
        logger.error("Invalid Data value!")
        Index = 0


    #Verify the correct number of items were scanned.
    if Index != 11:
        logger.error("Invalid option value")
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

    return Status,GuidBuffer