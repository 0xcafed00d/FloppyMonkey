#pragma once
#include <expected>
#include <optional>
#include <string>
#include "drivefunc.h"

namespace drivefunc {

	struct initParams {
		std::optional<std::string> imageName{std::nullopt};    // e.g. "C:\\path\\to\\image.img"
		std::optional<std::string> volumeLabel{std::nullopt};  // e.g. "FLOPPY"

		std::optional<bool> quickFormat{std::nullopt};  // for format task

		DriveInfo driveInfo;  // information about the drive to be used
	};

	struct Task {
		virtual ~Task() = default;

		// return nothing on success, or an error message if initialization fails
		virtual std::expected<void, std::string> init(const initParams&) = 0;

		// return true when task is complete, false to continue later, or an error message
		virtual std::expected<bool, std::string> execute() = 0;

		// return a string representing the current progress of the task
		virtual std::string getProgress() const = 0;
	};

}  // namespace drivefunc