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
#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <set>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

#include "ProcessorBind.h"

class putUINT64
{
public:
    putUINT64(UINT64 ullVal) : m_ull(ullVal) {}
    putUINT64(const putUINT64& r) : m_ull(r.m_ull) {}

    template <class _E, class _Tr>
    friend basic_ostream<_E, _Tr>& operator << (basic_ostream<_E, _Tr>&, putUINT64);

private:
    UINT64 m_ull;
};

template <class _E, class _Tr>
basic_ostream<_E, _Tr>& operator << (basic_ostream<_E, _Tr>& os, putUINT64 ull)
{
    static const char cDigits[] = "0123456789abcdef";

    UINT64 base = 10;
    if (os.flags() & ios_base::hex)
        base = 16;
    else if (os.flags() & ios_base::oct)
        base = 8;

    ostringstream ostr;
    UINT64 ullVal = ull.m_ull;
    while (ullVal != 0)
    {
        ostr << cDigits[ullVal % base];
        ullVal /= base;
    }

    string s1(ostr.str());
    string s2(s1.rbegin(), s1.rend());
    return os << s2;
}

class getUINT64
{
public:
    getUINT64(UINT64& ullVal) : m_ull(ullVal) {}
    getUINT64(const getUINT64& r) : m_ull(r.m_ull) {}

    template <class _E, class _Tr>
    friend basic_istream<_E, _Tr>& operator >> (basic_istream<_E, _Tr>&, getUINT64);

private:
    UINT64& m_ull;

private:
    getUINT64& operator = (const getUINT64&);
};

template <class _E, class _Tr>
basic_istream<_E, _Tr>& operator >> (basic_istream<_E, _Tr>& is, getUINT64 ull)
{
    string strBuf;
    is >> strBuf;

    UINT64 base = 10;
    if (is.flags() & ios_base::hex)
        base = 16;
    else if (is.flags() & ios_base::oct)
        base = 8;

    UINT64 ullVal = 0;
    for (string::iterator i = strBuf.begin(); i != strBuf.end(); i++)
    {
        if (*i <= '9' && *i >= '0')
            *i -= '0';
        else if (*i <= 'F' && *i >= 'A')
            *i -= 'A' - '\x0a';
        else if (*i <= 'f' && *i >= 'a')
            *i -= 'a' - '\x0a';
        else throw runtime_error("Invalid number format");

        ullVal = ullVal * base + *i;
    }
    ull.m_ull = ullVal;
    return is;
}

class EMemoryLeak : public logic_error
{
public:
    EMemoryLeak() : logic_error("Memory leak detected") {}
};

class EInvalidGuidString : public invalid_argument
{
public:
    EInvalidGuidString() : invalid_argument("Unexpected format of GUID string") {}
};

class ELogFileError : public logic_error
{
public:
    ELogFileError(const string& strMsg) : logic_error(strMsg) {}
};

class EDuplicatedFfsFile : public ELogFileError
{
public:
    EDuplicatedFfsFile() : ELogFileError("Duplicated FFS found in LOG file") {}
};

class EUnexpectedLogFileToken : public ELogFileError
{
public:
    EUnexpectedLogFileToken() : ELogFileError("Unexpected LOG file token") {}
};

class EFileNotFound : public invalid_argument
{
public:
    EFileNotFound(const string& strFName) : invalid_argument("File not found - " + strFName) {}
};

class EUnexpectedMapFile : public logic_error
{
public:
    EUnexpectedMapFile(const string& strKeyWord) : logic_error("Unexpected map file format - " + strKeyWord) {}
};

class EUsage : public invalid_argument
{
public:
    EUsage() : invalid_argument("Usage: GenFvMap <FV.LOG> <FV.INF> <FV.MAP>") {}
};

template <class T>
class CMemoryLeakChecker : public set<T*>
{
protected:
    CMemoryLeakChecker()
    {
    }

public:
    virtual ~CMemoryLeakChecker();
    static CMemoryLeakChecker<T>& GetInstance();

private:
    CMemoryLeakChecker(const CMemoryLeakChecker<T>&);
};

template <class T>
CMemoryLeakChecker<T>::~CMemoryLeakChecker()
{
    if (!CMemoryLeakChecker<T>::empty())
        throw EMemoryLeak();
}

template <class T>
CMemoryLeakChecker<T>& CMemoryLeakChecker<T>::GetInstance()
{
    static CMemoryLeakChecker<T> s_instance;
    return s_instance;
}

class CObjRoot
{
protected:
    CObjRoot()
    {
#ifdef _CHK_MEM_LEAK
        CMemoryLeakChecker<CObjRoot>::GetInstance().insert(this);
#endif
    }

public:
    virtual ~CObjRoot()
    {
#ifdef _CHK_MEM_LEAK
        CMemoryLeakChecker<CObjRoot>::GetInstance().erase(this);
#endif
    }

private:
    CObjRoot(const CObjRoot&);
};

class CIdentity : public CObjRoot
{
public:
    CIdentity(const string&);
    operator string (void) const;

    bool operator < (const CIdentity& id) const
    {
        return memcmp(this, &id, sizeof(*this)) < 0;
    }

    CIdentity() : ulD1(0), wD2(0), wD3(0), wD4(0), ullD5(0)
    {
    }

    CIdentity(const CIdentity& r) : ulD1(r.ulD1), wD2(r.wD2), wD3(r.wD3), wD4(r.wD4), ullD5(r.ullD5)
    {
    }

    template <class _E, class _Tr>
    basic_istream<_E, _Tr>& ReadId(basic_istream<_E, _Tr>&);
    template <class _E, class _Tr>
    basic_ostream<_E, _Tr>& WriteId(basic_ostream<_E, _Tr>&);

    template <class _E, class _Tr>
    friend basic_istream<_E, _Tr>& operator >> (basic_istream<_E, _Tr>&, CIdentity&);
    template <class _E, class _Tr>
    friend basic_ostream<_E, _Tr>& operator << (basic_ostream<_E, _Tr>&, CIdentity);

private:
    UINT32 ulD1;
    UINT16 wD2, wD3, wD4;
    UINT64 ullD5;
};

CIdentity::CIdentity(const string& strGuid)
{
    try
    {
        string str(strGuid);
        str.erase(0, str.find_first_not_of(" {"));
        str.resize(str.find_last_not_of(" }") + 1);
        str[str.find('-')] = ' ';
        str[str.find('-')] = ' ';
        str[str.find('-')] = ' ';
        str[str.find('-')] = ' ';

        istringstream is(str);
        is >> hex >> ulD1 >> wD2 >> wD3 >> wD4 >> getUINT64(ullD5);
    }
    catch (const exception&)
    {
        throw EInvalidGuidString();
    }
}

CIdentity::operator string(void) const
{
    ostringstream os;
    os << hex << setfill('0')
        << setw(8) << ulD1 << '-'
        << setw(4) << wD2 << '-'
        << setw(4) << wD3 << '-'
        << setw(4) << wD4 << '-'
        << setw(12) << putUINT64(ullD5);
    return os.str();
}

template <class _E, class _Tr>
basic_istream<_E, _Tr>& CIdentity::ReadId(basic_istream<_E, _Tr>& is)
{
    string str;
    if (!!(is >> str))
        *this = CIdentity(str);
    return is;
}

template <class _E, class _Tr>
basic_ostream<_E, _Tr>& CIdentity::WriteId(basic_ostream<_E, _Tr>& os)
{
    return os << (string)(*this);
}

template <class _E, class _Tr>
basic_istream<_E, _Tr>& operator >> (basic_istream<_E, _Tr>& is, CIdentity& id)
{
    return id.ReadId(is);
}

template <class _E, class _Tr>
basic_ostream<_E, _Tr>& operator << (basic_ostream<_E, _Tr>& os, CIdentity id)
{
    return id.WriteId(os);
}

template <class T>
class IVectorContainerByReference : virtual public CObjRoot, public vector<T*>
{
};

template <class T>
class IMapContainer : virtual public CObjRoot, public map<CIdentity, T>
{
};

struct ISymbol : virtual public CObjRoot
{
    string strAddress;
    string strName;
    string strFrom;
    UINT64 ullRva;
    bool bStatic;
    bool bFunction;
    virtual void Relocate(UINT64)=0;
};

class IModule : public IVectorContainerByReference<ISymbol>
{
public:
    string strName;
    CIdentity id;
    virtual UINT64 BaseAddress(void) const=0;
    virtual UINT64 BaseAddress(UINT64)=0;
    virtual const ISymbol *EntryPoint(void) const=0;
};

class IFirmwareVolume : public IVectorContainerByReference<IModule>
{
};

class IMapFileSet : public IMapContainer<istream*>
{
};

class IFfsSet : public IMapContainer<UINT64>
{
};

class CFfsSetFromLogFile : public IFfsSet
{
public:
    CFfsSetFromLogFile(const string&);
};

CFfsSetFromLogFile::CFfsSetFromLogFile(const string& strFName)
{
    ifstream ifs(strFName.c_str());
    if (!ifs)
        throw EFileNotFound(strFName);

    CIdentity ffsId;
    while (!!ffsId.ReadId(ifs))
    {
        UINT64 ullBase;
        if (!(ifs >> hex >> getUINT64(ullBase)))
            throw EUnexpectedLogFileToken();
        if (!insert(value_type(ffsId, ullBase)).second)
            throw EDuplicatedFfsFile();
    }
}

class CMapFileSetFromInfFile : public IMapFileSet
{
public:
    CMapFileSetFromInfFile(const string&);
    ~CMapFileSetFromInfFile();
};

CMapFileSetFromInfFile::CMapFileSetFromInfFile(const string& strFName)
{
    static const char cszEfiFileName[] = "EFI_FILE_NAME";

    ifstream ifs(strFName.c_str());
    if (!ifs)
        throw EFileNotFound(strFName);

    string strFile;
    getline(ifs, strFile, ifstream::traits_type::to_char_type(ifstream::traits_type::eof()));
    strFile.erase(0, strFile.find("[files]"));

    istringstream is(strFile);
    string strTmp;
    while (!!getline(is, strTmp))
    {
        string::size_type pos = strTmp.find(cszEfiFileName);
        if (pos == string::npos)
            continue;

        strTmp.erase(0, strTmp.find_first_not_of(" =", pos + sizeof(cszEfiFileName) - 1));
        pos = strTmp.find_last_of("\\/");
        string strId(
            strTmp.begin() + pos + 1,
            strTmp.begin() + strTmp.find('-', strTmp.find('-', strTmp.find('-', strTmp.find('-', strTmp.find('-') + 1) + 1) + 1) + 1)
            );
        strTmp.erase(pos + 1, strId.length() + 1);
        strTmp.replace(strTmp.rfind('.'), string::npos, ".map");

        istream *ifmaps = new ifstream(strTmp.c_str());
        if (ifmaps && !!*ifmaps &&
            !insert(value_type(CIdentity(strId), ifmaps)).second)
                throw EDuplicatedFfsFile();
    }
}

CMapFileSetFromInfFile::~CMapFileSetFromInfFile()
{
    for (iterator i = begin(); i != end(); i++)
        delete i->second;
}

class CSymbolFromString : public ISymbol
{
public:
    CSymbolFromString(const string&, bool = false);
    void Relocate(UINT64);
};

CSymbolFromString::CSymbolFromString(const string& strSymbol, bool b)
{
    bStatic = b;

    istringstream is(strSymbol);
    is >> strAddress >> strName >> hex >> getUINT64(ullRva) >> strFrom;
    if (strFrom == "f")
    {
        bFunction = true;
        is >> strFrom;
    }
    else bFunction = false;
    if (!is)
        throw EUnexpectedMapFile("Symbol line format");
}

void CSymbolFromString::Relocate(UINT64 ullDelta)
{
    if (ullRva > 0)
        ullRva += ullDelta;
}

class CModuleFromMap : public IModule
{
public:
    CModuleFromMap(istream&);
    ~CModuleFromMap();

    UINT64 BaseAddress() const;
    UINT64 BaseAddress(UINT64);
    const ISymbol *EntryPoint() const;

private:
    UINT64 m_ullLoadAddress;
    iterator m_iEntryPoint;

    static pair<string, string::size_type> FindToken(istream&, const string&);
};

pair<string, string::size_type> CModuleFromMap::FindToken(istream& is, const string& strToken)
{
    for (string strTmp; !!getline(is, strTmp);)
    {
        string::size_type pos = strTmp.find(strToken);
        if (pos != string::npos)
            return pair<string, string::size_type>(strTmp, pos);
    }
    throw EUnexpectedMapFile(strToken);
}

CModuleFromMap::CModuleFromMap(istream& imaps)
{
    static const char cszLoadAddr[] = "Preferred load address is";
    static const char cszGlobal[] = "Address";
    static const char cszEntryPoint[] = "entry point at";
    static const char cszStatic[] = "Static symbols";

    pair<string, string::size_type> pairTmp;
    istringstream iss;

    getline(imaps, strName);
    strName.erase(0, strName.find_first_not_of(' '));

    pairTmp = FindToken(imaps, cszLoadAddr);
    iss.str(pairTmp.first.substr(pairTmp.second + sizeof(cszLoadAddr) - 1));
    iss >> getUINT64(m_ullLoadAddress);

    pairTmp = FindToken(imaps, cszGlobal);
    while (!!getline(imaps, pairTmp.first) &&
            pairTmp.first.find(cszEntryPoint) == string::npos)
        if (pairTmp.first.find_first_not_of(' ') != string::npos)
            push_back(new CSymbolFromString(pairTmp.first));

    iss.str(pairTmp.first.substr(pairTmp.first.find(cszEntryPoint) + sizeof(cszEntryPoint) - 1));
    iss.clear();
    string strEntryPoint;
    iss >> strEntryPoint;

    pairTmp = FindToken(imaps, cszStatic);
    if (pairTmp.second)
        while (!!getline(imaps, pairTmp.first))
            if (pairTmp.first.find_first_not_of(' ') != string::npos)
                push_back(new CSymbolFromString(pairTmp.first, true));

    for (m_iEntryPoint = begin();
         m_iEntryPoint != end() && (*m_iEntryPoint)->strAddress != strEntryPoint;
         m_iEntryPoint++);
    if (m_iEntryPoint == end())
        throw EUnexpectedMapFile("Entry point not found");
}

CModuleFromMap::~CModuleFromMap()
{
    for (iterator i = begin(); i != end(); i++)
        delete *i;
}

UINT64 CModuleFromMap::BaseAddress(void) const
{
    return m_ullLoadAddress;
}

UINT64 CModuleFromMap::BaseAddress(UINT64 ullNewBase)
{
    ullNewBase -= m_ullLoadAddress;
    for (iterator i = begin(); i != end(); i++)
        (*i)->Relocate(ullNewBase);
    m_ullLoadAddress += ullNewBase;
    return m_ullLoadAddress - ullNewBase;
}

const ISymbol *CModuleFromMap::EntryPoint(void) const
{
    return *m_iEntryPoint;
}

class CFvMap : public IFirmwareVolume
{
public:
    CFvMap(IFfsSet*, IMapFileSet*);
    ~CFvMap();

private:
    CFvMap(const CFvMap&);
};

CFvMap::CFvMap(IFfsSet *pFfsSet, IMapFileSet *pMapSet)
{
    for (IFfsSet::iterator i = pFfsSet->begin(); i != pFfsSet->end(); i++)
    {
        IMapFileSet::iterator j = pMapSet->find(i->first);
        if (j != pMapSet->end())
        {
            IModule *pModule = new CModuleFromMap(*j->second);
            pModule->id = i->first;
            pModule->BaseAddress(i->second);
            push_back(pModule);
        }
    }
}

CFvMap::~CFvMap()
{
    for (iterator i = begin(); i != end(); i++)
        delete *i;
}

class CFvMapGenerator : public CObjRoot
{
public:
    CFvMapGenerator(const IFirmwareVolume *pFv) : m_pFv(pFv) {}
    CFvMapGenerator(const CFvMapGenerator& r) : m_pFv(r.m_pFv) {}

    template <class _E, class _Tr>
    friend basic_ostream<_E, _Tr>& operator << (basic_ostream<_E, _Tr>&, CFvMapGenerator);

private:
    static bool Less(const IModule*, const IModule*);

private:
    const IFirmwareVolume *m_pFv;
};

template <class _E, class _Tr>
basic_ostream<_E, _Tr>& operator << (basic_ostream<_E, _Tr>& os, CFvMapGenerator fvMapFmt)
{
    vector<IModule*> rgMods(fvMapFmt.m_pFv->begin(), fvMapFmt.m_pFv->end());
    sort(rgMods.begin(), rgMods.end(), CFvMapGenerator::Less);
    for (vector<IModule*>::iterator i = rgMods.begin(); i != rgMods.end(); i++)
    {
        os << (*i)->strName << hex << " (BaseAddress=" << putUINT64((*i)->BaseAddress());
        os << ", EntryPoint=" << hex << putUINT64((*i)->EntryPoint()->ullRva);
        os << ", GUID=";
        (*i)->id.WriteId(os);
        os << ")" << endl << endl;

        for (IModule::iterator j = (*i)->begin(); j != (*i)->end(); j++)
        {
            os << hex << "  " << setw(16) << setfill('0') << putUINT64((*j)->ullRva);
            os << ((*j)->bFunction ? " F" : "  ")
                << ((*j)->bStatic ? "S " : "  ")
                << (*j)->strName << endl;
        }

        os << endl << endl;
    }
    return os;
}

bool CFvMapGenerator::Less(const IModule *pModL, const IModule *pModR)
{
    return pModL->BaseAddress() < pModR->BaseAddress();
}

class CApplication : public CObjRoot
{
public:
    CApplication(int, char**);
    int Run(void);

private:
    char **m_ppszArg;
private:
    CApplication(const CApplication&);
};

CApplication::CApplication(int cArg, char *ppszArg[])
: m_ppszArg(ppszArg)
{
    if (cArg != 4)
        throw EUsage();
}

int CApplication::Run(void)
{
    CFfsSetFromLogFile ffsSet(m_ppszArg[1]);
    CMapFileSetFromInfFile mapSet(m_ppszArg[2]);
    ofstream ofs(m_ppszArg[3]);
    CFvMap fvMap(&ffsSet, &mapSet);
    ofs << CFvMapGenerator(&fvMap);
    return 0;
}

int main(int argc, char *argv[])
{
    try
    {
        CApplication app(argc, argv);
        return app.Run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return -1;
    }
}

#ifdef _DDK3790x1830_WORKAROUND
extern "C" void __fastcall __security_check_cookie(int)
{
}
#endif
