#pragma once

#include <windows.h>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace drivefunc {
	struct DiskGeometry {
		uint32_t bytesPerSector{0};
		uint32_t sectorsPerTrack{0};
		uint32_t tracksPerCylinder{0};
		uint64_t cylinders{0};
		uint64_t totalSize{0};

		size_t sectorSize() const {
			return bytesPerSector;
		}
		size_t sectorOffset(size_t sectorNumber) const {
			return sectorNumber * bytesPerSector;
		}
		size_t trackSize() const {
			return sectorsPerTrack * bytesPerSector;
		}
		size_t trackOffset(size_t trackNumber) const {
			return trackNumber * trackSize();
		}
	};

	struct DriveInfo {
		std::string name;         // "C:\\"
		std::string driveName;    // "\\.\C:"
		std::string type;         // Fixed, Removable, etc.
		std::string volumeLabel;  // "Windows"
		std::string fileSystem;   // "NTFS"
		bool isFloppy{false};
		bool hasGeometry{false};
		DiskGeometry geometry;
		std::string physicalDrive;  // e.g. "\\\\.\\PhysicalDrive0"
		std::string bootSectorInfo;
	};

	// Opens a handle to the specified physical drive with optional write access.
	// Returns a std::expected containing the HANDLE on success, or an error message on failure
	std::expected<HANDLE, std::string> openDisk(const std::string& physicalDrive, bool writeAccess);

	// Closes the handle to the physical drive.
	// Returns a std::expected indicating success or containing an error message on failure
	std::expected<void, std::string> closeDisk(HANDLE hDisk);

	// Locks the volume to prevent other processes from accessing it while raw I/O is performed.
	// Returns a std::expected indicating success or containing an error message on failure
	// drive is unlocked automatically when handle is closed
	std::expected<void, std::string> lockVolume(HANDLE hDisk);

	// Reads a sector from the disk into the provided buffer.
	// returns a std::expected containing the number of bytes read on success, or an error message
	// on failure, a size of zero indicates End Of Media (EOM).
	// The Buffer will be resized to fit the sector size.
	// The DiskGeometry is required to determine the sector size.
	// sectorNumber is zero-based
	std::expected<size_t, std::string> readSectorRaw(HANDLE hDisk,
	                                                 const DiskGeometry& geometry,
	                                                 DWORD sectorNumber,
	                                                 std::vector<char>& buffer);

	// Writes a sector to the disk from the provided buffer.
	// Returns a std::expected containing the number of bytes written on success, or an error
	// message on failure. The buffer size must match the sector size defined in the DiskGeometry.
	// sectorNumber is zero-based
	std::expected<size_t, std::string> writeSectorRaw(HANDLE hDisk,
	                                                  const DiskGeometry& geometry,
	                                                  DWORD sectorNumber,
	                                                  const std::vector<char>& buffer);

	// Reads an entire track from the disk into the provided buffer.
	// Returns a std::expected containing the number of bytes read on success, or an error
	// message on failure, a size of zero indicates End Of Media (EOM).
	// The Buffer will be resized to fit the track size.
	// The DiskGeometry is required to determine the track size.
	// which is sectorsPerTrack * bytesPerSector
	// trackNumber is zero-based
	std::expected<size_t, std::string> readTrackRaw(HANDLE hDisk,
	                                                const DiskGeometry& geometry,
	                                                DWORD trackNumber,
	                                                std::vector<char>& buffer);

	// Writes an entire track to the disk from the provided buffer.
	// Returns a std::expected containing the number of bytes written on success, or an error
	// message on failure. The buffer size must match the track size defined in the DiskGeometry,
	// which is sectorsPerTrack * bytesPerSector
	// trackNumber is zero-based
	std::expected<size_t, std::string> writeTrackRaw(HANDLE hDisk,
	                                                 const DiskGeometry& geometry,
	                                                 DWORD trackNumber,
	                                                 const std::vector<char>& buffer);

	// Retrieves the disk geometry for the specified physical drive.
	// Returns a std::expected containing the DiskGeometry on success, or an error message on
	// failure
	std::expected<DiskGeometry, std::string> getDiskGeometry(const std::string& physicalDrive);

	// Enumerates all drives on the system and retrieves their information.
	// Returns a std::expected containing a vector of DriveInfo on success, or an error message
	std::expected<std::vector<DriveInfo>, std::string> enumerateDrives();

	// Prints the boot sector of a floppy disk to the provided output stream.
	// Returns true on success, false on failure
	bool printFloppyBootSector(const std::string& physicalDrive, std::ostream& out);

}  // namespace drivefunc
