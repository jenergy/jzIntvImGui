#ifndef FRAME_H_INCLUDED
#define FRAME_H_INCLUDED

typedef struct _TAG_POPUPSTRUCT
{
    HMENU hMenu;
    UINT uiString;
} PopupStruct;

LRESULT CALLBACK FrameProc(HWND, UINT, WPARAM, LPARAM);
void DestroyChildren(HWND hWnd);

#endif
