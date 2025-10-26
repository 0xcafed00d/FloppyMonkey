#include "winfunc.h"
#include "imgui_app.h"

#include <SDL.h>
#include <SDL_syswm.h>

std::optional<std::string> select_file_to_open(bool loadImage) {
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[MAX_PATH];  // buffer for file name

	SDL_Window* window;
	SDL_Renderer* renderer;
	ImGuiApp::get_SDL_window_and_renderer(&window, &renderer);

	SDL_SysWMinfo wmInfo;
	SDL_GetVersion(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = wmInfo.info.win.window;

	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "Image Fine\0*.img\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	if (loadImage) {
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	} else {
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	}

	// Display the Open dialog box.

	if (GetOpenFileName(&ofn) == TRUE)
		return szFile;
	else
		return {};
}

std::string getLastErrorAsString() {
	// Get the error code
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return "No error";  // No error message has been recorded

	LPSTR messageBuffer = nullptr;

	// Format the error message
	size_t size = FormatMessageA(
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0,
	    NULL);

	std::string message(messageBuffer, size);

	// Free the buffer allocated by FormatMessage
	LocalFree(messageBuffer);

	return message;
}