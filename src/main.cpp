#include "imgui_app.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <assert.h>

#include "drivewindow.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
	if (!ImGuiApp::init("Floppy Monkey", 1024, 768)) {
		return -1;
	}

// if builing on linux, you can use the following to load a font
#ifdef __linux__
	// load linux font
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16.0f);
#endif

	DriveWindow driveWindow;

	while (ImGuiApp::loop()) {
		// menu bar

		if (!driveWindow.update()) {
			ImGuiApp::request_quit();
		} else {
			if (!driveWindow.render()) {
				ImGuiApp::request_quit();
			}
		}
	}

	ImGuiApp::shutdown();
	return 0;
}
