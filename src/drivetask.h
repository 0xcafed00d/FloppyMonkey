#include "task.h"

#include <windows.h>

namespace drivefunc {
	class FormatTask : public Task {
	   public:
		~FormatTask() override;

		std::expected<void, std::string> init(const initParams& params) override;
		std::expected<bool, std::string> execute() override;
		std::string getProgress() const override;

	   private:
		HANDLE hDisk{INVALID_HANDLE_VALUE};
	};

	class WriteImageTask : public Task {
	   public:
		~WriteImageTask() override;

		std::expected<void, std::string> init(const initParams& params) override;
		std::expected<bool, std::string> execute() override;
		std::string getProgress() const override;

	   private:
		HANDLE hDisk{INVALID_HANDLE_VALUE};
		HANDLE hImage{INVALID_HANDLE_VALUE};
		uint64_t imageSize{0};
		uint64_t totalWritten{0};
		uint32_t trackCounter{0};
		DriveInfo driveInfo;
	};

	class ReadImageTask : public Task {
	   public:
		~ReadImageTask() override;

		std::expected<void, std::string> init(const initParams& params) override;
		std::expected<bool, std::string> execute() override;
		std::string getProgress() const override;

	   private:
		HANDLE hDisk{INVALID_HANDLE_VALUE};
		HANDLE hImage{INVALID_HANDLE_VALUE};
		uint64_t imageSize{0};
		uint64_t totalRead{0};
		uint32_t trackCounter{0};
		DriveInfo driveInfo;
	};

	class CloneDiskTask : public Task {
	   public:
		~CloneDiskTask() override;

		std::expected<void, std::string> init(const initParams& params) override;
		std::expected<bool, std::string> execute() override;
		std::string getProgress() const override;

	   private:
		HANDLE hSourceDisk{INVALID_HANDLE_VALUE};
		HANDLE hTargetDisk{INVALID_HANDLE_VALUE};
		uint64_t totalCopied{0};
	};

}  // namespace drivefunc
