#pragma once

#include <memory>
#include "drivefunc.h"
#include "drivetask.h"

struct DriveWindowInfo {
	drivefunc::DriveInfo driveInfo;
	HANDLE hDevice{INVALID_HANDLE_VALUE};
	std::vector<char> sectorData;
	size_t currentSector{0};
	bool isOpen{false};

	std::shared_ptr<drivefunc::Task> currentTask{nullptr};
	std::string taskProgress{"No task running"};
};

class DriveWindow {
   public:
	DriveWindow();
	~DriveWindow();

	bool update();
	bool render();

	void renderDriveWindow(DriveWindowInfo& driveWindow);

	// remove copy and move constructors and assignment operators
	DriveWindow(const DriveWindow&) = delete;
	DriveWindow& operator=(const DriveWindow&) = delete;
	DriveWindow(DriveWindow&&) = delete;
	DriveWindow& operator=(DriveWindow&&) = delete;

   private:
	std::vector<drivefunc::DriveInfo> drives;
	std::vector<DriveWindowInfo> driveWindows;

	size_t selectedDriveIndex{static_cast<size_t>(-1)};
	void displaySectorData(const std::vector<char>& data, int bytesPerRow = 16);
};
