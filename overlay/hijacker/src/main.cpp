#include <Windows.h>
#include "hijacker/hijacker.hpp"
#include "overlay/overlay.hpp"

int main()
{
	Hijacker::Hijacker* HJ = &Hijacker::Hijacker();

	HJ->GetProcessID("notepad.exe"); // example
	HWND HijackedHWND = HJ->Hijack("notepad.exe", "Notepad");

	Overlay::Overlay* O = &Overlay::Overlay(HijackedHWND);

	if (O->InitiateD3D(HJ->ChildWindowSizeX, HJ->ChildWindowSizeY))
		O->StartRender(HJ->OverlayString, "Untitled - Paint");

	return 0;
}