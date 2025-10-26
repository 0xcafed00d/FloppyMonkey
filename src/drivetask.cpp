#include "drivetask.h"
#include "drivefunc.h"
#include "winfunc.h"

#include <windows.h>
#include <expected>
#include <fstream>
#include <sstream>

namespace drivefunc {

	// ----------------------------------------------------------------
	// FormatTask implementation
	// ----------------------------------------------------------------
	FormatTask::~FormatTask() {
		if (hDisk != INVALID_HANDLE_VALUE) {
			CloseHandle(hDisk);
		}
	}

	std::expected<void, std::string> FormatTask::init(const initParams& params) {
		if (!params.driveInfo.isFloppy) {
			return std::unexpected<std::string>("Drive is not a floppy drive.");
		}

		hDisk = CreateFileA(params.driveInfo.driveName.c_str(), GENERIC_READ | GENERIC_WRITE,
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDisk == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Failed to open drive: " +
			                                    std::to_string(GetLastError()));
		}

		return {};
	}

	std::expected<bool, std::string> FormatTask::execute() {
		if (hDisk == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Drive handle is invalid.");
		}

		// Simulate formatting operation
		Sleep(2000);  // Simulate time taken to format

		return true;  // Indicate task is complete
	}

	std::string FormatTask::getProgress() const {
		return "Formatting in progress...";
	}

	// ----------------------------------------------------------------
	// WriteImageTask implementation
	// ----------------------------------------------------------------
	WriteImageTask::~WriteImageTask() {
		if (hDisk != INVALID_HANDLE_VALUE) {
			CloseHandle(hDisk);
		}
		if (hImage != INVALID_HANDLE_VALUE) {
			CloseHandle(hImage);
		}
	}
	std::expected<void, std::string> WriteImageTask::init(const initParams& params) {
		if (!params.imageName.has_value()) {
			return std::unexpected<std::string>("Image file name not provided.");
		}
		if (!params.driveInfo.isFloppy) {
			return std::unexpected<std::string>("Drive is not a floppy drive.");
		}

		driveInfo = params.driveInfo;

		auto res = drivefunc::openDisk(params.driveInfo.physicalDrive, true);
		if (!res) {
			return std::unexpected<std::string>("Failed to open physical drive: " + res.error());
		}
		hDisk = *res;

		if (auto lockRes = drivefunc::lockVolume(hDisk); !lockRes) {
			return std::unexpected<std::string>("Failed to lock volume: " + lockRes.error());
		}

		hImage = CreateFileA(params.imageName->c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
		                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hImage == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Failed to open image file: " +
			                                    std::to_string(GetLastError()));
		}

		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(hImage, &fileSize)) {
			return std::unexpected<std::string>("Failed to get image file size: " +
			                                    std::to_string(GetLastError()));
		}
		imageSize = static_cast<uint64_t>(fileSize.QuadPart);
		totalWritten = 0;
		trackCounter = 0;

		return {};
	}

	std::expected<bool, std::string> WriteImageTask::execute() {
		if (hDisk == INVALID_HANDLE_VALUE || hImage == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Invalid handle(s).");
		}

		const size_t trackSize = driveInfo.geometry.trackSize();
		std::vector<char> buffer(trackSize);
		DWORD bytesRead = 0;

		while (totalWritten < imageSize) {
			if (!ReadFile(hImage, buffer.data(), buffer.size(), &bytesRead, NULL)) {
				return std::unexpected<std::string>("Failed to read from image file: " +
				                                    getLastErrorAsString());
			}
			if (bytesRead == 0) {
				break;  // EOF
			}

			auto wres = writeTrackRaw(hDisk, driveInfo.geometry, trackCounter, buffer);
			if (!wres) {
				return std::unexpected<std::string>("Failed to write track to disk: " +
				                                    getLastErrorAsString());
			}
			totalWritten += wres.value();
			trackCounter++;

			if (bytesRead != wres.value()) {
				return std::unexpected<std::string>("Incomplete write to disk.");
			}

			return false;  // Indicate task is not yet complete
		}

		return true;  // Indicate task is complete
	}

	std::string WriteImageTask::getProgress() const {
		std::ostringstream oss;
		oss << "Writing image: Track " << trackCounter << ", " << (totalWritten * 100 / imageSize)
		    << "%";
		return oss.str();
	}

	// ----------------------------------------------------------------
	// ReadImageTask implementation
	// ----------------------------------------------------------------
	ReadImageTask::~ReadImageTask() {
		if (hDisk != INVALID_HANDLE_VALUE) {
			CloseHandle(hDisk);
		}
		if (hImage != INVALID_HANDLE_VALUE) {
			CloseHandle(hImage);
		}
	}

	std::expected<void, std::string> ReadImageTask::init(const initParams& params) {
		if (!params.imageName.has_value()) {
			return std::unexpected<std::string>("Image file name not provided.");
		}
		if (!params.driveInfo.isFloppy) {
			return std::unexpected<std::string>("Drive is not a floppy drive.");
		}

		driveInfo = params.driveInfo;

		hDisk = CreateFileA(params.driveInfo.driveName.c_str(), GENERIC_READ,
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDisk == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Failed to open drive: " +
			                                    std::to_string(GetLastError()));
		}

		hImage = CreateFileA(params.imageName->c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		                     FILE_ATTRIBUTE_NORMAL, NULL);
		if (hImage == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Failed to create image file: " +
			                                    std::to_string(GetLastError()));
		}

		imageSize = driveInfo.geometry.totalSize;
		totalRead = 0;
		trackCounter = 0;

		return {};
	}

	std::expected<bool, std::string> ReadImageTask::execute() {
		if (hDisk == INVALID_HANDLE_VALUE || hImage == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Invalid handle(s).");
		}

		DWORD bytesRead = 0;
		DWORD bytesWritten = 0;
		std::vector<char> trackBuffer(driveInfo.geometry.trackSize());

		while (totalRead < imageSize) {
			auto rres = readTrackRaw(hDisk, driveInfo.geometry, trackCounter, trackBuffer);
			if (!rres) {
				return std::unexpected<std::string>("Failed to read track from disk: " +
				                                    rres.error());
			}

			bytesRead = rres.value();
			if (bytesRead == 0) {
				break;  // EOF
			}
			totalRead += bytesRead;
			trackCounter++;

			if (!WriteFile(hImage, trackBuffer.data(), bytesRead, &bytesWritten, NULL)) {
				return std::unexpected<std::string>("Failed to write to image file: " +
				                                    std::to_string(GetLastError()));
			}
			if (bytesWritten != bytesRead) {
				return std::unexpected<std::string>("Incomplete write to image file.");
			}

			return false;  // Indicate task is not yet complete
		}

		return true;  // Indicate task is complete
	}

	std::string ReadImageTask::getProgress() const {
		std::ostringstream oss;
		oss << "Reading image: Track " << trackCounter << ", " << (totalRead * 100 / imageSize)
		    << "%";
		return oss.str();
	}

}  // namespace drivefunc