#ifndef GUI_H_INCLUDED
#define GUI_H_INCLUDED

#define IMASM_REGISTRY_KEY        "Software\\ShinyTechnologies\\IMASM"

const int WL_INFOPTR = 0;
const int WL_NEXT = WL_INFOPTR + sizeof (LONG);
const int WL_SIZE = WL_NEXT + 4;

typedef struct _TAG_WNDINFO
{
    HWND hRichEd;
    char szFileName[MAX_PATH];
    bool bIsDirty;
    DWORD kBytes;
    bool bInsert;
} WndInfo;

typedef struct _TAG_SYNTAXINFO
{
    COLORREF crCol;
    std::string sName;
} SyntaxInfo;

INT APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, INT);
ATOM MyRegisterClass(HINSTANCE);
HWND InitInstance(HINSTANCE, int);

void DoExitProcessing(HWND hWnd);
void Highlight(HWND hWnd, COLORREF col, DWORD dwStart, DWORD dwEnd);
int LoadFile(HWND hRichEd, char *fName);
int SaveFile(HWND hRichEd, char *fName);
COLORREF GetWordColor(char *szWord);
DWORD CALLBACK LoadFileCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);
DWORD CALLBACK SaveFileCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);
void HighlightDoc(HWND hWnd);
void ConfirmSave(HWND hWnd);
void LoadConfigInfo();
void CreateToolbar(HWND hWnd);
int DoCommandLine(char *szBuff);
bool ParseFiles(const char *szSrc, const char *szDst, char *szError);
void UpdateStatusBar(HWND hWnd);
int LoadFileIntoNewWindow(char *szFile);
int GetAsmFileName(HWND hWnd, char *szFileTitle, char *szFileName);

#endif
