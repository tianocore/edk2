//****************************************************************************
//**
//**  Copyright  (C) 2006 Intel Corporation. All rights reserved. 
//**
//** The information and source code contained herein is the exclusive 
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization 
//** from the company.
//**
//****************************************************************************

#include <stdexcept>
#include <list>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <algorithm>
#include <functional>
using namespace std;

typedef unsigned __int64 ulonglong_t;

template <class T>
class CMemoryLeakChecker : public list<T*>
{
public:
    static CMemoryLeakChecker<T>& GetInstance(void);

private:
    CMemoryLeakChecker(void)
    {
    }

    ~CMemoryLeakChecker(void);
};

template <class T>
CMemoryLeakChecker<T>& CMemoryLeakChecker<T>::GetInstance(void)
{
    static CMemoryLeakChecker<T> s_memLeakChecker;
    return s_memLeakChecker;
}

template <class T>
CMemoryLeakChecker<T>::~CMemoryLeakChecker(void)
{
    if (!list<T*>::empty())
        throw logic_error(__FUNCTION__ ": Memory leak detected!");
}

class CObjRoot
{
protected:
    CObjRoot(void);
    virtual ~CObjRoot(void);
};

CObjRoot::CObjRoot(void)
{
    CMemoryLeakChecker<CObjRoot>::GetInstance().push_back(this);
}

CObjRoot::~CObjRoot(void)
{
    CMemoryLeakChecker<CObjRoot>::GetInstance().remove(this);
}

class CIdentity : public CObjRoot
{
public:
    CIdentity(void);
    CIdentity(const string&);
    CIdentity(const CIdentity&);

    bool operator < (const CIdentity&) const;
    friend istream& operator >> (istream&, CIdentity&);

    static const string::size_type s_nIdStrLen;

protected:
    ulonglong_t m_ullId[2];
};

const string::size_type CIdentity::s_nIdStrLen = 36;

CIdentity::CIdentity(void)
{
    memset(m_ullId, 0, sizeof(m_ullId));
}

CIdentity::CIdentity(const string& strId)
{
    if (strId.length() != CIdentity::s_nIdStrLen ||
        strId[8] != '-' ||
        strId[13] != '-' ||
        strId[18] != '-' ||
        strId[23] != '-')
        throw runtime_error(
            __FUNCTION__ ": Error GUID format " + strId);

    string strIdCopy(strId);
    strIdCopy.erase(23, 1);
    strIdCopy[18] = ' ';
    strIdCopy.erase(13, 1);
    strIdCopy.erase(8, 1);

    istringstream is(strIdCopy);
    is >> hex >> m_ullId[0] >> m_ullId[1];
    if (!is)
        throw runtime_error(
            __FUNCTION__ ": GUID contains invalid characters" + strId);
}

CIdentity::CIdentity(const CIdentity& idRight)
{
    memmove(m_ullId, idRight.m_ullId, sizeof(m_ullId));
}

bool CIdentity::operator < (const CIdentity& idRight) const
{
    return memcmp(m_ullId, idRight.m_ullId, sizeof(m_ullId)) < 0;
}

istream& operator >> (istream& is, CIdentity& idRight)
{
    string strId;
    is >> strId;
    if (!!is)
        idRight = CIdentity(strId);
    return is;
}

class CInputFile : public CObjRoot
{
protected:
    CInputFile(const string&);
    CInputFile(istream&);
    istream& GetLine(string&);

private:
    CInputFile(const CInputFile&);
    CInputFile& operator = (const CInputFile&);

private:
    auto_ptr<istream> m_pIs;

protected:
    istream& m_is;
};

CInputFile::CInputFile(const string& strFName)
: m_pIs(new ifstream(strFName.c_str()))
, m_is(*m_pIs)
{
    if (!m_is)
        throw runtime_error(__FUNCTION__ ": Error opening input file " + strFName);
}

CInputFile::CInputFile(istream& is)
: m_is(is)
{
    if (!m_is)
        throw runtime_error(__FUNCTION__ ": Error opening input stream");
}

istream& CInputFile::GetLine(string& strALine)
{
    if (!!m_is)
        while (!!getline(m_is, strALine))
        {
            string::size_type pos = strALine.find_last_not_of(' ');
            if (pos != string::npos)
            {
                strALine.erase(pos + 1);
                strALine.erase(0, strALine.find_first_not_of(' '));
                break;
            }
        }
    return m_is;
}

class CIdAddressMap : public CInputFile, public map<CIdentity, ulonglong_t>
{
public:
    CIdAddressMap(istream&);
};

CIdAddressMap::CIdAddressMap(istream& is)
: CInputFile(is)
{
    CIdentity id;
    ulonglong_t ullBase;

    while (!!(m_is >> hex >> id >> ullBase))
        if (!insert(value_type(id, ullBase)).second)
            throw runtime_error(__FUNCTION__ ": Duplicated files");
}

class CIdPathMap : public CInputFile, public map<CIdentity, string>
{
public:
    CIdPathMap(istream&);
};

CIdPathMap::CIdPathMap(istream& is)
: CInputFile(is)
{
    static const char cszFileSec[] = "[files]";
    static const char cszFfsFile[] = "EFI_FILE_NAME";

    string strALine;

    // Find the [files] section
    while (!!GetLine(strALine) && strALine.compare(0, sizeof(cszFileSec) - 1, cszFileSec));

    // m_is error means no FFS files listed in this INF file
    if (!m_is)
        return;

    // Parse FFS files one by one
    while (!!GetLine(strALine))
    {
        // Test if this begins a new section
        if (strALine[0] == '[')
            break;

        // Is it a line of FFS file?
        if (strALine.compare(0, sizeof(cszFfsFile) - 1, cszFfsFile))
            continue;

        string::size_type pos = strALine.find_first_not_of(' ', sizeof(cszFfsFile) - 1);
        if (pos == string::npos || strALine[pos] != '=')
            throw runtime_error(__FUNCTION__ ": Invalid FV INF format");
        pos = strALine.find_first_not_of(' ', pos + 1);
        if (pos == string::npos)
            throw runtime_error(__FUNCTION__ ": Incomplete line");

        strALine.erase(0, pos);
        pos = strALine.rfind('\\');
        if (pos == string::npos)
            pos = 0;
        else pos++;

        CIdentity id(strALine.substr(pos, CIdentity::s_nIdStrLen));
        if (!insert(value_type(id, strALine)).second)
            throw runtime_error(__FUNCTION__ ": Duplicated FFS files");
    }
}

class CSymbol : public CObjRoot
{
public:
    string m_strAddress;
    string m_strName;
    ulonglong_t m_ullRva;
    string m_strFrom;
    bool m_bStatic;
    bool m_bFunction;

    CSymbol()
    {
    }
    CSymbol(const string&, bool = false);
    friend ostream& operator << (ostream&, const CSymbol&);
};

CSymbol::CSymbol(const string& strALine, bool bStatic)
: m_bStatic(bStatic)
{
    istringstream is(strALine);

    is >> m_strAddress >> m_strName >> hex >> m_ullRva >> m_strFrom;
    if (m_strFrom == "F" || m_strFrom == "f")
    {
        m_bFunction = true;
        is >> m_strFrom;
    } else m_bFunction = false;
}

ostream& operator << (ostream& os, const CSymbol& symbol)
{
    os << hex << setw(16) << setfill('0') << symbol.m_ullRva << setw(0);
    os << ' ' << (symbol.m_bFunction ? 'F' : ' ')
        << (symbol.m_bStatic ? 'S' : ' ') << ' ';
    return os << symbol.m_strName << endl;
}

class CMapFile : public CInputFile, public list<CSymbol>
{
public:
    CMapFile(const string&);

    void SetLoadAddress(ulonglong_t);
    friend ostream& operator << (ostream&, const CMapFile&);

    string m_strModuleName;
    ulonglong_t m_ullLoadAddr;
    string m_strEntryPoint;
};

CMapFile::CMapFile(const string& strFName)
: CInputFile(strFName)
{
    static const char cszLoadAddr[] = "Preferred load address is";
    static const char cszGlobal[] = "Address";
    static const char cszEntryPoint[] = "entry point at";
    static const char cszStatic[] = "Static symbols";

    string strALine;

    GetLine(m_strModuleName);

    while (!!GetLine(strALine) && strALine.compare(0, sizeof(cszLoadAddr) - 1, cszLoadAddr));
    if (!m_is)
        throw runtime_error(__FUNCTION__ ": Load Address not listed in map file");

    istringstream is(strALine.substr(sizeof(cszLoadAddr) - 1));
    if (!(is >> hex >> m_ullLoadAddr))
        throw runtime_error(__FUNCTION__ ": Unexpected Load Address format");

    while (!!GetLine(strALine) && strALine.compare(0, sizeof(cszGlobal) - 1, cszGlobal));
    if (!m_is)
        throw runtime_error(__FUNCTION__ ": Global symbols not found in map file");

    while (!!GetLine(strALine) && strALine.compare(0, sizeof(cszEntryPoint) - 1, cszEntryPoint))
        push_back(CSymbol(strALine));
    if (!m_is)
        throw runtime_error(__FUNCTION__ ": Entry Point not listed in map file");

    is.str(strALine.substr(strALine.find_first_not_of(' ', sizeof(cszEntryPoint) - 1)));
    is.clear();
    if (!getline(is, m_strEntryPoint))
        throw runtime_error(__FUNCTION__ ": Unexpected Entry Point format");

    while (!!GetLine(strALine) && strALine.compare(0, sizeof(cszStatic) - 1, cszStatic));
    while (!!GetLine(strALine))
        push_back(CSymbol(strALine, true));
}

void CMapFile::SetLoadAddress(ulonglong_t ullLoadAddr)
{
    for (iterator i = begin(); i != end(); i++)
        if (i->m_ullRva != 0)
            i->m_ullRva += ullLoadAddr - m_ullLoadAddr;
    m_ullLoadAddr = ullLoadAddr;
}

ostream& operator << (ostream& os, const CMapFile& mapFile)
{
    CMapFile::const_iterator i = mapFile.begin();
    while (i != mapFile.end() && i->m_strAddress != mapFile.m_strEntryPoint)
        i++;
    if (i == mapFile.end())
        throw runtime_error(
            __FUNCTION__ ": Entry point not found for module " +
            mapFile.m_strModuleName);

    os << endl << hex
        << mapFile.m_strModuleName << " (EP=" << i->m_ullRva
        << ", BA=" << mapFile.m_ullLoadAddr << ')' << endl
        << endl;

    for (i = mapFile.begin(); i != mapFile.end(); i++)
        os << "  " << *i;

    return os << endl;
}

class COutputFile : public CObjRoot
{
protected:
    COutputFile(ostream&);
    ostream& m_os;

private:
    COutputFile(const COutputFile&);
    COutputFile& operator = (const COutputFile&);
};

class CFvMapFile : public CObjRoot, public map<CIdentity, CMapFile*>
{
public:
    CFvMapFile(const CIdAddressMap&, const CIdPathMap&);
    ~CFvMapFile(void);

    friend ostream& operator << (ostream&, const CFvMapFile&);

private:
    void Cleanup(void);
};

CFvMapFile::CFvMapFile(const CIdAddressMap& idAddr, const CIdPathMap& idPath)
{
    for (CIdAddressMap::const_iterator i = idAddr.begin(); i != idAddr.end(); i++)
    {
        CIdPathMap::const_iterator j = idPath.find(i->first);
        if (j == idPath.end())
            throw runtime_error(__FUNCTION__ ": Map file not found");

        try
        {
            pair<iterator, bool> k = insert(value_type(i->first,
                new CMapFile(j->second.substr(0, j->second.rfind('.')) + ".map")));
            if (!k.second)
                throw logic_error(__FUNCTION__ ": Duplicated file found in rebase log");

            k.first->second->SetLoadAddress(i->second);
        }
        catch (const runtime_error& e)
        {
            cerr << e.what() << endl;
        }
    }
}

void CFvMapFile::Cleanup(void)
{
    for (iterator i = begin(); i != end(); i++)
        delete i->second;
}

ostream& operator << (ostream& os, const CFvMapFile& fvMap)
{
    for (CFvMapFile::const_iterator i = fvMap.begin(); !!os && i != fvMap.end(); i++)
        os << *i->second;
    return os;
}

CFvMapFile::~CFvMapFile(void)
{
    Cleanup();
}

class CGenFvMapUsage : public invalid_argument
{
public:
    CGenFvMapUsage(void) : invalid_argument(s_szUsage)
    {
    }

private:
    static const char s_szUsage[];
};

const char CGenFvMapUsage::s_szUsage[] = "Usage: GenFvMap <LOG> <INF> <MAP>";

class CGenFvMapApp : public CObjRoot
{
public:
    CGenFvMapApp(int, char *[]);
    ~CGenFvMapApp(void);

    int Run(void);

private:
    int m_cArgc;
    char **m_ppszArgv;
};

CGenFvMapApp::CGenFvMapApp(int cArgc, char *ppszArgv[])
: m_cArgc(cArgc)
, m_ppszArgv(ppszArgv)
{
    if (cArgc != 4)
        throw CGenFvMapUsage();
}

CGenFvMapApp::~CGenFvMapApp(void)
{
}

int CGenFvMapApp::Run(void)
{
    ifstream isLog(m_ppszArgv[1]);
    ifstream isInf(m_ppszArgv[2]);

    CIdAddressMap idAddress(isLog);
    CIdPathMap idPath(isInf);

    CFvMapFile fvMap(idAddress, idPath);

    ofstream osMap(m_ppszArgv[3], ios_base::out | ios_base::trunc);
    osMap << fvMap;

    if (!osMap)
        throw runtime_error(__FUNCTION__ ": Error writing output file");

    return 0;
}

int main(int argc, char *argv[])
{
    try
    {
        CGenFvMapApp app(argc, argv);
        return app.Run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return -1;
    }
}
