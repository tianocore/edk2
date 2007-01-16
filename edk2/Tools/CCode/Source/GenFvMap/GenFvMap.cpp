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
#include "ProcessorBind.h"
#include <iostream>
#include <stdexcept>
#include <list>
#include <map>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <algorithm>
#include <functional>
using namespace std;

typedef UINT64 ulonglong_t;

#ifdef __GNUC__
#if __STDC_VERSION__ < 199901L
#define __FUNCTION__ __FILE__
#endif
#endif

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
    friend ostream& operator << (ostream&, const CIdentity&);

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

ostream& operator << (ostream& os, const CIdentity& idRight)
{
    return os << hex << setfill('0')
        << setw(8) << (unsigned long)(idRight.m_ullId[0] >> 32) << '-'
        << setw(4) << (unsigned short)(idRight.m_ullId[0] >> 16) << '-'
        << setw(4) << (unsigned short)idRight.m_ullId[0] << '-'
        << setw(4) << (unsigned short)(idRight.m_ullId[1] >> 48) << '-'
        << setw(12) <<  (idRight.m_ullId[1] & 0xffffffffffffULL);
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

class CIdAddressPathMap : public CInputFile, public map<CIdentity, pair<ulonglong_t, string> >
{
public:
    CIdAddressPathMap(istream&);
};

CIdAddressPathMap::CIdAddressPathMap(istream& is)
: CInputFile(is)
{
    key_type k;
    mapped_type m;
    while (!!(m_is >> hex >> k >> m.first) && !!GetLine(m.second))
        if (!insert(value_type(k, m)).second)
            throw runtime_error(__FUNCTION__ ": Duplicated files");
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
    return os << symbol.m_strName;
}

class CMapFile : public CInputFile, public list<CSymbol>
{
public:
    CMapFile(const string&);

    void SetLoadAddress(ulonglong_t);

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
        if (i->m_ullRva >= m_ullLoadAddr)
            i->m_ullRva += ullLoadAddr - m_ullLoadAddr;
    m_ullLoadAddr = ullLoadAddr;
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
    CFvMapFile(const CIdAddressPathMap&);
    ~CFvMapFile(void);

    friend ostream& operator << (ostream&, const CFvMapFile&);

private:
    void Cleanup(void);
};

CFvMapFile::CFvMapFile(const CIdAddressPathMap& idAddrPath)
{
    for (CIdAddressPathMap::const_iterator i = idAddrPath.begin(); i != idAddrPath.end(); i++)
    {
        if (i->second.second == "*")
            continue;

        pair<iterator, bool> r = insert(value_type(i->first,
            new CMapFile(i->second.second.substr(0, i->second.second.rfind('.')) + ".map")));
        r.first->second->SetLoadAddress(i->second.first);
    }
}

CFvMapFile::~CFvMapFile(void)
{
    Cleanup();
}

void CFvMapFile::Cleanup(void)
{
    for (iterator i = begin(); i != end(); i++)
        delete i->second;
}

static bool map_less(const CFvMapFile::const_iterator& l, const CFvMapFile::const_iterator& r)
{
    return l->second->m_ullLoadAddr < r->second->m_ullLoadAddr;
}

ostream& operator << (ostream& os, const CFvMapFile& fvMap)
{
    vector<CFvMapFile::const_iterator> rgIter;
    rgIter.reserve(fvMap.size());
    for (CFvMapFile::const_iterator i = fvMap.begin(); i != fvMap.end(); i++)
        rgIter.push_back(i);
    sort(rgIter.begin(), rgIter.end(), map_less);

    for (vector<CFvMapFile::const_iterator>::const_iterator i = rgIter.begin(); i != rgIter.end(); i++)
    {
        CMapFile::const_iterator j = (*i)->second->begin();
        while (j != (*i)->second->end() && j->m_strAddress != (*i)->second->m_strEntryPoint) j++;
        if (j == (*i)->second->end())
            throw runtime_error(
                __FUNCTION__ ":Entry point not found for module " +
                (*i)->second->m_strModuleName);

        os << hex
            << (*i)->second->m_strModuleName
            << " (EntryPoint=" << j->m_ullRva
            << ", BaseAddress=" << (*i)->second->m_ullLoadAddr
            << ", GUID=" << (*i)->first
            << ")" << endl << endl;

        for (j = (*i)->second->begin(); j != (*i)->second->end(); j++)
            os << "  " << *j << endl;

        os << endl << endl;
    }

    return os;
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

const char CGenFvMapUsage::s_szUsage[] = "Usage: GenFvMap <LOG> <MAP>";

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
    if (cArgc != 3)
        throw CGenFvMapUsage();
}

CGenFvMapApp::~CGenFvMapApp(void)
{
}

int CGenFvMapApp::Run(void)
{
    ifstream isLog(m_ppszArgv[1]);
    CIdAddressPathMap idAddrPath(isLog);
    CFvMapFile fvMap(idAddrPath);

    ofstream osMap(m_ppszArgv[2], ios_base::out | ios_base::trunc);
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
