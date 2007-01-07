#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "resource.h"
#include "text_window.h"

using boost::replace_all_copy;
using boost::trim_copy;

namespace 
{
    string text_window_title;
    string text_window_error;
    BOOL CALLBACK text_window_proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        switch(Message) {
            case WM_INITDIALOG:
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON)));
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON)));
                SetWindowText(hwnd, text_window_title.c_str());
                SetDlgItemText(hwnd, IDC_TEXT, text_window_error.c_str());
                return TRUE;
            case WM_COMMAND:
                if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                    EndDialog(hwnd, LOWORD(wParam));
                }
                break;
            default:
                return FALSE;
        }
        return TRUE;
    }
}

namespace Win32
{
    void text_window(const string& title, const string& error)
    {
        text_window_title = title + " - FreeCNC";
        text_window_error = replace_all_copy(trim_copy(error), "\n", "\r\n");
        DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TEXT_WINDOW), 0, text_window_proc);
        text_window_title.clear();
        text_window_error.clear();
    }
}