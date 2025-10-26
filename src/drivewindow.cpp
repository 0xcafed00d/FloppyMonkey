#include "drivewindow.h"
#include <sstream>
#include "imgui_app.h"
#include "winfunc.h"

DriveWindow::DriveWindow() {
	auto result = drivefunc::enumerateDrives();
	if (result) {
		drives = *result;
		if (!drives.empty()) {
			selectedDriveIndex = 0;
		}
	}
}

DriveWindow::~DriveWindow() {
}

bool DriveWindow::update() {
	for (auto& driveWindow : driveWindows) {
		if (driveWindow.isOpen) {
			if (driveWindow.currentTask) {
				auto taskResult = driveWindow.currentTask->execute();
				if (!taskResult) {
					// error occurred
					driveWindow.taskProgress = "Error: " + taskResult.error();
					driveWindow.currentTask.reset();
				} else if (taskResult.value()) {
					// task complete
					driveWindow.taskProgress = "Task completed successfully.";
					driveWindow.currentTask.reset();
				} else {
					// task still running
					driveWindow.taskProgress = driveWindow.currentTask->getProgress();
				}
			}
		}
	}

	return true;
}

bool DriveWindow::render() {
	if (ImGui::Begin("Drives")) {
		// refresh drive list button
		if (ImGui::Button("Refresh Drives")) {
			auto result = drivefunc::enumerateDrives();
			if (result) {
				drives = *result;
				if (!drives.empty()) {
					selectedDriveIndex = 0;
				}
			} else {
				selectedDriveIndex = static_cast<size_t>(-1);
			}
		}
		if (drives.empty()) {
			ImGui::Text("No drives found.");
		} else {
			// list drives
			if (ImGui::BeginListBox("##drives",
			                        ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
				for (size_t i = 0; i < drives.size(); ++i) {
					const bool isSelected = (i == selectedDriveIndex);
					if (ImGui::Selectable(drives[i].name.c_str(), isSelected)) {
						selectedDriveIndex = i;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
		}
		if (selectedDriveIndex < drives.size()) {
			if (ImGui::Button("Open Drive")) {
				// Check if a window for this drive is already open
				bool alreadyOpen = false;
				for (const auto& dw : driveWindows) {
					if (dw.driveInfo.name == drives[selectedDriveIndex].name && dw.isOpen) {
						alreadyOpen = true;
						break;
					}
				}
				if (!alreadyOpen) {
					DriveWindowInfo newWindow;
					newWindow.driveInfo = drives[selectedDriveIndex];
					newWindow.isOpen = true;
					driveWindows.push_back(std::move(newWindow));
				}
			}
		} else {
			ImGui::Text("No drive selected.");
		}
	}
	ImGui::End();

	// Render individual drive windows
	for (auto& driveWindow : driveWindows) {
		if (driveWindow.isOpen) {
			renderDriveWindow(driveWindow);
		}
	}

	return true;
}

void DriveWindow::renderDriveWindow(DriveWindowInfo& driveWindow) {
	bool open = true;

	if (ImGui::Begin(driveWindow.driveInfo.name.c_str(), &open)) {
		ImGui::Text("Task Status: %s", driveWindow.taskProgress.c_str());

		ImGui::BeginTable("table_drive_info", 2);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if (driveWindow.hDevice == INVALID_HANDLE_VALUE) {
			auto result = drivefunc::openDisk(driveWindow.driveInfo.physicalDrive, false);
			if (!result) {
				ImGui::Text("Error opening disk: %s", result.error().c_str());
				ImGui::EndTable();
				ImGui::End();
				return;
			}
			driveWindow.hDevice = result.value();
		}

		// show selected drive info
		if (selectedDriveIndex < drives.size()) {
			drivefunc::DriveInfo& drive = driveWindow.driveInfo;
			ImGui::Separator();
			ImGui::Text("Drive: %s", drive.name.c_str());
			if (drive.isFloppy) {
				ImGui::Text("Type: Floppy Drive");
			} else {
				ImGui::Text("Type: %s", drive.type.c_str());
			}
			ImGui::Text("Volume Label: %s", drive.volumeLabel.c_str());
			ImGui::Text("File System: %s", drive.fileSystem.c_str());
			ImGui::Text("Physical Drive: %s", drive.physicalDrive.c_str());
			if (drive.hasGeometry) {
				const drivefunc::DiskGeometry& geo = drive.geometry;
				ImGui::Text("Bytes per Sector: %u", geo.bytesPerSector);
				ImGui::Text("Sectors per Track: %u", geo.sectorsPerTrack);
				ImGui::Text("Tracks per Cylinder: %u", geo.tracksPerCylinder);
				ImGui::Text("Cylinders: %llu", geo.cylinders);
				ImGui::Text("Total Size: %.2f MB",
				            static_cast<double>(geo.totalSize) / (1024.0 * 1024.0));
			} else {
				ImGui::Text("Disk geometry not available.");
			}

			ImGui::Separator();
			if (ImGui::Button("Write Image to Drive")) {
				// Open file dialog to select image
				auto fileOpt = select_file_to_open(true);
				if (fileOpt) {
					drivefunc::initParams params = {.imageName = *fileOpt,
					                                .driveInfo = driveWindow.driveInfo};

					drivefunc::closeDisk(driveWindow.hDevice);
					driveWindow.hDevice = INVALID_HANDLE_VALUE;
					// Create and assign the task
					driveWindow.currentTask = std::make_shared<drivefunc::WriteImageTask>();
					auto result = driveWindow.currentTask->init(params);
					if (result) {
						driveWindow.taskProgress = "Starting image Write...";
					} else {
						driveWindow.taskProgress =
						    "Error initializing Write task: " + result.error();
						driveWindow.currentTask.reset();
					}
				}
			}

			if (ImGui::Button("Save Image from Drive")) {
				// Open file dialog to select image
				auto fileOpt = select_file_to_open(false);
				if (fileOpt) {
					drivefunc::initParams params = {.imageName = *fileOpt,
					                                .driveInfo = driveWindow.driveInfo};
					// Create and assign the task
					driveWindow.currentTask = std::make_shared<drivefunc::ReadImageTask>();
					auto result = driveWindow.currentTask->init(params);
					if (result) {
						driveWindow.taskProgress = "Starting image Read...";
					} else {
						driveWindow.taskProgress =
						    "Error initializing Read task: " + result.error();
						driveWindow.currentTask.reset();
					}
				}
			}

			ImGui::TableNextColumn();
			if (ImGui::Button("Show Floppy Boot Sector")) {
				std::stringstream ss;
				drivefunc::printFloppyBootSector(drive.driveName, ss);
				drive.bootSectorInfo = ss.str().c_str();
			}
			ImGui::Text("%s", drive.bootSectorInfo.c_str());

			ImGui::EndTable();
		}
	}

	bool doRead = false;

	if (ImGui::Button("Prev Sector <<<")) {
		if (driveWindow.currentSector > 0) {
			--driveWindow.currentSector;
			doRead = true;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Read Sector")) {
		doRead = true;
	}

	ImGui::SameLine();
	// set width for input box
	ImGui::SetNextItemWidth(50);
	ImGui::InputScalar("##Sector Number", ImGuiDataType_U32, &driveWindow.currentSector);

	ImGui::SameLine();
	if (ImGui::Button("Next Sector >>>")) {
		++driveWindow.currentSector;
		doRead = true;
	}

	if (doRead) {
		doRead = false;
		driveWindow.sectorData.resize(driveWindow.driveInfo.geometry.sectorSize());
		auto result = drivefunc::readSectorRaw(driveWindow.hDevice, driveWindow.driveInfo.geometry,
		                                       static_cast<DWORD>(driveWindow.currentSector),
		                                       driveWindow.sectorData);
		if (!result) {
			ImGui::Text("Error reading sector %zu : %s", driveWindow.currentSector,
			            result.error().c_str());
		}
	}

	if (!driveWindow.sectorData.empty()) {
		ImGui::Separator();
		displaySectorData(driveWindow.sectorData);
	}

	ImGui::End();

	if (!open) {
		if (driveWindow.hDevice != INVALID_HANDLE_VALUE) {
			drivefunc::closeDisk(driveWindow.hDevice);
			driveWindow.hDevice = INVALID_HANDLE_VALUE;
			driveWindow.isOpen = false;
		}
	}
}

void DriveWindow::displaySectorData(const std::vector<char>& data, int bytesPerRow) {
	const int totalBytes = static_cast<int>(data.size());
	for (int i = 0; i < totalBytes; i += bytesPerRow) {
		// Print offset
		ImGui::Text("%08X: ", i);
		ImGui::SameLine();

		// Print hex values
		for (int j = 0; j < bytesPerRow; ++j) {
			if (i + j < totalBytes) {
				ImGui::Text("%02X ", static_cast<unsigned char>(data[i + j]));
			} else {
				ImGui::Text("   ");  // padding for incomplete rows
			}
			ImGui::SameLine();
		}

		// Print ASCII representation
		ImGui::Text("|");
		ImGui::SameLine();
		for (int j = 0; j < bytesPerRow; ++j) {
			if (i + j < totalBytes) {
				char c = data[i + j];
				if (isprint(static_cast<unsigned char>(c))) {
					ImGui::Text("%c", c);
				} else {
					ImGui::Text(".");
				}
			} else {
				ImGui::Text(" ");  // padding for incomplete rows
			}
			ImGui::SameLine();
		}
		ImGui::Text("|");
	}
}