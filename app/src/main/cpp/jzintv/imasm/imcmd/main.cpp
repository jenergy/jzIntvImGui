/*

IMASM Macro Precompiler

Copyright (C) 2003  Joe Fisher, Shiny Technologies, LLC
http://www.shinytechnologies.com
Portions Copyright (C) 2003  Joseph Zbiciak.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <windows.h>
#include <iostream.h>
#include <ctype.h>

#include "..\main.h"

#define NUL '\0'

const char *const license_string =
    "IMASM Macro Precompiler\n"
    "imasm -help for help\n"
    "\n"
    "Copyright (C) 2003  Joe Fisher, Shiny Technologies, LLC\n"
    "Portions Copyright (C) 2003  Joseph Zbiciak\n"
    "\n"
    "http://www.shinytechnologies.com\n"
    "\n"
    "This program is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU General Public License\n"
    "as published by the Free Software Foundation; either version 2\n"
    "of the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program; if not, write to the Free Software\n"
    "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, "
    "USA.\n";

int stricmp_(const char *str1, const char *str2)
{
	while (tolower(*str1) == tolower(*str2))
	{
		if (*str1 == NUL)
		{
			return 0;
		}

		str1++;
		str2++;
	}

	if (tolower(*str1) > tolower(*str2))
	{
		return 1;
	}

	return -1;
}

void OutputHelp()
{
	cout << "IMASM Macro Precompiler ver " << IMASMVERSION << endl << endl;

	cout << "Usage:" << endl;
	cout << "imasm                      : Run in GUI mode" << endl;
	cout << "imasm inputfile            : Run in GUI mode and load filename into editor" << endl;
	cout << "imasm inputfile outputfile : Parse file with no GUI" << endl;
	cout << "imasm -l" << endl;
	cout << "imasm /l                   : Display GPL License" << endl;

	cout << endl << "Copyright (C) 2003  Joe Fisher, Joe Zbiciak" << endl;
	cout << "http://www.shinytechnologies.com" << endl << endl;
	cout << "Distributed under the GNU GPL" << endl;
}

void OutputLicense()
{
	cout << license_string << endl;
}

BOOL CreateChildProcess(char *szFile)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	// Create the child process.
	BOOL bRet =  CreateProcess(	NULL,
								szFile,	// command line
								NULL,	// process security attributes
								NULL,	// primary thread security attributes
								TRUE,	// handles are inherited
								0,		// creation flags
								NULL,	// use parent's environment
								NULL,	// use parent's current directory
								&si,	// STARTUPINFO pointer
								&pi);	// receives PROCESS_INFORMATION


	return bRet;
}

void DoChildProcess(char *szCmd)
{
	HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRdDup;
	HANDLE hChildStdoutRd, hChildStdoutWr;
	HANDLE hChildStdinWrDup;
	HANDLE hSaveStdout, hSaveStdin;

	BOOL fSuccess;

	// Set the bInheritHandle flag so pipe handles are inherited.
	SECURITY_ATTRIBUTES saAttr;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	try
	{
		// The steps for redirecting child process's STDOUT:
		//     1. Save current STDOUT, to be restored later.
		//     2. Create anonymous pipe to be STDOUT for child process.
		//     3. Set STDOUT of the parent process to be write handle to
		//        the pipe, so it is inherited by the child process.
		//     4. Create a noninheritable duplicate of the read handle and
		//        close the inheritable read handle.

		// Save the handle to the current STDOUT.

		hSaveStdout = GetStdHandle(STD_OUTPUT_HANDLE);

		// Create a pipe for the child process's STDOUT.
		if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
		{
			throw ("Stdout pipe creation failed\n");
		}

		// Set a write handle to the pipe to be STDOUT.

		if (!SetStdHandle(STD_OUTPUT_HANDLE, hChildStdoutWr))
		{
			throw ("Redirecting STDOUT failed");
		}

		// Create noninheritable read handle and close the inheritable read
		// handle.
		fSuccess = DuplicateHandle(	GetCurrentProcess(),
									hChildStdoutRd,
									GetCurrentProcess(),
									&hChildStdoutRdDup,
									0,
									FALSE,
									DUPLICATE_SAME_ACCESS);

		if (!fSuccess)
		{
			throw ("DuplicateHandle failed");
		}

		CloseHandle(hChildStdoutRd);

		// The steps for redirecting child process's STDIN:
		//     1.  Save current STDIN, to be restored later.
		//     2.  Create anonymous pipe to be STDIN for child process.
		//     3.  Set STDIN of the parent to be the read handle to the
		//         pipe, so it is inherited by the child process.
		//     4.  Create a noninheritable duplicate of the write handle,
		//         and close the inheritable write handle.

		// Save the handle to the current STDIN.

		hSaveStdin = GetStdHandle(STD_INPUT_HANDLE);

		// Create a pipe for the child process's STDIN.

		if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
		{
			throw ("Stdin pipe creation failed\n");
		}

		// Set a read handle to the pipe to be STDIN.

		if (!SetStdHandle(STD_INPUT_HANDLE, hChildStdinRd))
		{
			throw ("Redirecting Stdin failed");
		}

		// Duplicate the write handle to the pipe so it is not inherited.

		fSuccess = DuplicateHandle(GetCurrentProcess(),
							  hChildStdinWr,
							  GetCurrentProcess(),
							  &hChildStdinWrDup,
							  0,
							  FALSE,                  // not inherited
							  DUPLICATE_SAME_ACCESS);
		if (!fSuccess)
		{
			throw ("DuplicateHandle failed");
		}

		// Now create the child process.
		if (!CreateChildProcess(szCmd))
		{
			throw ("Create process failed");
		}

		// After process creation, restore the saved STDIN and STDOUT.
		if (!SetStdHandle(STD_INPUT_HANDLE, hSaveStdin))
		{
			throw ("Re-redirecting Stdin failed\n");
		}

		if (!SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout))
		{
			throw ("Re-redirecting Stdout failed\n");
		}

		// Close the write end of the pipe before reading from the
		// read end of the pipe.
		CloseHandle(hChildStdoutWr);
		CloseHandle(hChildStdinWr);
	}
	catch (char *szError)
	{
		cout << szError << endl;
		return;
	}

	DWORD dwRead, dwWritten;
	char chBuff[MAX_PATH];
	int i = 0;

	// Read output from the child process
	while (1)
	{
		if (!ReadFile(hChildStdoutRdDup, chBuff, MAX_PATH, &dwRead, NULL))
		{
			break;
		}

		if (dwRead > 0)
		{
			WriteFile(hSaveStdout, &chBuff, dwRead, &dwWritten, 0);
		}
	}
}

int main(int argc, char *argv[])
{
	char szCmd[MAX_PATH] = "";
	int i;

	if (argc > 3)
	{
		cout << endl << "Error, Invalid number of arguments. " << endl << endl;
		OutputHelp();
		return 0;
	}

	if (argc > 1)
	{
		if (
			(stricmp_(argv[1], "-help") == 0) ||
			(stricmp_(argv[1], "/help") == 0) ||
			(stricmp_(argv[1], "-?") == 0) ||
			(stricmp_(argv[1], "/?") == 0))
		{
			OutputHelp();
			return 0;
		}
		else if (
			(stricmp_(argv[1], "-l") == 0) ||
			(stricmp_(argv[1], "/l") == 0))
		{
			OutputLicense();
			return 0;
		}
	}

	// Spawn the process here
	strcpy(szCmd, ".\\imasm.exe ");
	for (i = 1; i < argc; i++)
	{
		strcat(szCmd, argv[i]);
		strcat(szCmd, " ");
	}

	DoChildProcess(szCmd);

	cout << "Thanks for using IMASM ver " << IMASMVERSION << endl;
	return 0;
}