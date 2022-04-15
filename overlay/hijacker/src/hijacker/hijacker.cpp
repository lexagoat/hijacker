#include "hijacker.hpp"
#include <TlHelp32.h>
#include <dwmapi.h>

Hijacker::Hijacker::Hijacker()
{
	this->ProcessName = "";
	this->ProcessClassName = "";
	this->ProcessWindowName = "";

}

unsigned int Hijacker::Hijacker::GetProcessID(std::string_view process_name)
{
	HANDLE HandleToProcessID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);

	do
	{
		if (!strcmp(pEntry.szExeFile, process_name.data()))
		{
			printf("%s ID -> %d \n", process_name.data(), pEntry.th32ProcessID);
			CloseHandle(HandleToProcessID);
			return pEntry.th32ProcessID;
		}

	} while (Process32Next(HandleToProcessID, &pEntry));
	return 0;
}

std::vector<unsigned int> Hijacker::Hijacker::GetProcessIDList(std::string_view process_name)
{
	std::vector<unsigned int> ProcessList;
	HANDLE HandleToProcessID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	do
	{
		if (!strcmp(pEntry.szExeFile, process_name.data()))
		{
			ProcessList.push_back(pEntry.th32ProcessID);
			CloseHandle(HandleToProcessID);
		}

	} while (Process32Next(HandleToProcessID, &pEntry));

	return ProcessList;
}

HWND Hijacker::Hijacker::FindTopWindow(DWORD pid)
{
	std::pair<HWND, DWORD> params = { 0, pid };

	BOOL bResult = EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
		{
			auto pParams = (std::pair<HWND, DWORD>*)(lParam);

			DWORD processId;
			if (GetWindowThreadProcessId(hwnd, &processId) && processId == pParams->second)
			{
				SetLastError(-1);
				pParams->first = hwnd;
				return FALSE;
			}

			return TRUE;
		}, (LPARAM)&params);

	if (!bResult && GetLastError() == -1 && params.first)
	{
		return params.first;
	}

	return 0;
}

HWND Hijacker::Hijacker::Hijack(std::string_view target, std::string_view class_name, std::string_view window_name)
{
	this->ProcessName = target;
	this->ProcessClassName = class_name;
	this->ProcessWindowName = window_name;

	std::vector<unsigned int> RunningProcesses = GetProcessIDList(this->ProcessName);
	std::vector<HWND> RunningWindows = {};

	if (RunningProcesses.empty())
		return 0x0;

	for (auto& Process : RunningProcesses)
	{
		HANDLE OldProcessHandle = OpenProcess(PROCESS_TERMINATE, FALSE, Process);
		TerminateProcess(OldProcessHandle, NULL);
		CloseHandle(OldProcessHandle);
	}

	RunningProcesses.clear();

	size_t suffix = target.find_last_of(".");
	std::string_view rawname = target.substr(0, suffix);
	std::string final = std::string("start ") + rawname.data();
	OverlayString = final;
	printf("System Call -> {%s} \n", final.c_str());

	if (system(final.data()) == 1)
		return 0x0;

	RunningProcesses = GetProcessIDList(target);

	if (RunningProcesses.empty() || RunningProcesses.size() > 1)
		return 0x0;

	WP.WindowStyle = WS_VISIBLE;

	WP.PIDOwner = RunningProcesses.at(0);

	printf("Waiting For Windows To Initiate... \n");

	Sleep(1000);

	RunningWindows = this->GrabOverlayHWND();

	HWND FinalHWND = RunningWindows.at(0);
	RECT ChildWindowRECT;

	GetWindowRect(FindWindowA(0, "Untitled - Paint"), &ChildWindowRECT);

	ChildWindowRECT.top += 30;

	this->ChildWindowSizeX = ChildWindowRECT.right - ChildWindowRECT.left;
	this->ChildWindowSizeY = ChildWindowRECT.bottom - ChildWindowRECT.top;


	SetMenu(FinalHWND, NULL);




	SetWindowPos(FinalHWND, HWND_TOPMOST, ChildWindowRECT.left, ChildWindowRECT.top, ChildWindowSizeX, ChildWindowSizeY, 0);

	SetLayeredWindowAttributes(FinalHWND, RGB(0, 0, 0), 0, LWA_COLORKEY | LWA_ALPHA);

	MARGINS Margins = { -1 };
	DwmExtendFrameIntoClientArea(FinalHWND, &Margins);

	return FinalHWND;
}

std::vector<HWND> Hijacker::Hijacker::GrabOverlayHWND()
{
	std::vector<HWND> WindowList = {};
	HWND current_hwnd = NULL;
	DWORD PID = this->GetProcessID(this->ProcessName);

	do
	{
		current_hwnd = FindWindowEx(NULL, current_hwnd, NULL, NULL);
		GetWindowThreadProcessId(current_hwnd, &PID);

		char className[256]; char windowName[256];
		GetClassNameA(current_hwnd, className, 256);
		GetWindowTextA(current_hwnd, windowName, 256);

		std::string c_name = className;
		std::string w_name = windowName;


		if (this->ProcessClassName == c_name && this->ProcessWindowName == "")
		{
			printf("Found HWND -> 0x%llX With Classname -> %s \n", current_hwnd, c_name.c_str());
			WindowList.push_back(current_hwnd);
		}

		else if (this->ProcessClassName == c_name && w_name == this->ProcessWindowName)
		{
			printf("Found HWND -> 0x%llX With Window Name -> %s Classname -> %s \n", current_hwnd, w_name.c_str(), c_name.c_str());
			WindowList.push_back(current_hwnd);
		}

	} while (current_hwnd != NULL);

	return WindowList;
}