#ifndef REGISTRY_H_INCLUDED
#define REGISTRY_H_INCLUDED

class RegHelp
{
public:
    RegHelp(HKEY hRoot);
    ~RegHelp();

    int OpenKey(char *szSection);
    int CloseKey();
    int AddString(char *szSection, char *szBuff);
    int AddDword(char *szSection, DWORD dwVal);
    int GetString(char *szSection, char *szBuff, unsigned long ulSize);
    int GetDword(char *szSection, DWORD *dwVal);

private:
    HKEY m_hRoot;
    HKEY m_hKey;
};
#endif
