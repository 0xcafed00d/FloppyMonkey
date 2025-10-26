#include "drivefunc.h"
#include "winfunc.h"

#include <windows.h>
#include <expected>
#include <iomanip>
#include <iostream>
#include <vector>

namespace drivefunc {

	std::string utf8_encode(const std::wstring& wstr) {
		if (wstr.empty())
			return std::string();
		int size_needed =
		    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL,
		                    NULL);
		return strTo;
	}

	std::wstring utf8_decode(const std::string& str) {
		if (str.empty())
			return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

	std::expected<HANDLE, std::string> openDisk(const std::string& physicalDrive,
	                                            bool writeAccess) {
		std::wstring wPhysicalDrive = utf8_decode(physicalDrive);

		HANDLE hDisk = CreateFileW(wPhysicalDrive.c_str(),
		                           writeAccess ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
		                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		                           FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, NULL);
		if (hDisk == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Failed to open disk: " +
			                                    std::to_string(GetLastError()));
		}
		return hDisk;
	}

	std::expected<void, std::string> lockVolume(HANDLE hDisk) {
		DWORD ret{0};
		if (!DeviceIoControl(hDisk, FSCTL_LOCK_VOLUME, nullptr, 0, nullptr, 0, &ret, nullptr))
			return std::unexpected<std::string>("Failed to lock volume" + getLastErrorAsString());
		return {};
	}

	std::expected<void, std::string> closeDisk(HANDLE hDisk) {
		if (hDisk != INVALID_HANDLE_VALUE) {
			if (!CloseHandle(hDisk)) {
				return std::unexpected<std::string>("Failed to close disk handle: " +
				                                    getLastErrorAsString());
			}
		}
		return {};
	}

	std::expected<size_t, std::string> readSectorRaw(HANDLE hDisk,
	                                                 const DiskGeometry& geometry,
	                                                 DWORD sectorNumber,
	                                                 std::vector<char>& buffer) {
		LARGE_INTEGER offset{};
		offset.QuadPart = static_cast<LONGLONG>(geometry.sectorOffset(sectorNumber));

		if (SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN) == 0)
			return std::unexpected<std::string>("Failed to set file pointer: " +
			                                    getLastErrorAsString());

		DWORD bytesRead{0};
		buffer.resize(geometry.bytesPerSector);
		if (!ReadFile(hDisk, buffer.data(), geometry.sectorSize(), &bytesRead, NULL)) {
			return std::unexpected<std::string>("Failed to read sector: " + getLastErrorAsString());
		}
		return bytesRead;
	}

	std::expected<size_t, std::string> readTrackRaw(HANDLE hDisk,
	                                                const DiskGeometry& geometry,
	                                                DWORD trackNumber,
	                                                std::vector<char>& buffer) {
		LARGE_INTEGER offset{};
		offset.QuadPart = static_cast<LONGLONG>(geometry.trackOffset(trackNumber));
		if (SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN) == 0)
			return std::unexpected<std::string>("Failed to set file pointer: " +
			                                    getLastErrorAsString());

		DWORD bytesRead{0};
		buffer.resize(geometry.trackSize());
		if (!ReadFile(hDisk, buffer.data(), geometry.trackSize(), &bytesRead, NULL)) {
			return std::unexpected<std::string>("Failed to read track: " + getLastErrorAsString());
		}
		return bytesRead;
	}

	std::expected<size_t, std::string> writeTrackRaw(HANDLE hDisk,
	                                                 const DiskGeometry& geometry,
	                                                 DWORD trackNumber,
	                                                 const std::vector<char>& buffer) {
		if (buffer.size() != geometry.trackSize())
			return std::unexpected<std::string>("Invalid buffer size");

		LARGE_INTEGER offset{};
		offset.QuadPart = static_cast<LONGLONG>(geometry.trackOffset(trackNumber));

		if (SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN) == 0)
			return std::unexpected<std::string>("Failed to set file pointer: " +
			                                    getLastErrorAsString());

		DWORD bytesWritten{0};
		if (!WriteFile(hDisk, buffer.data(), geometry.trackSize(), &bytesWritten, NULL)) {
			return std::unexpected<std::string>("Failed to write track: " + getLastErrorAsString());
		}
		return bytesWritten;
	}

	std::expected<size_t, std::string> writeSectorRaw(HANDLE hDisk,
	                                                  const DiskGeometry& geometry,
	                                                  DWORD sectorNumber,
	                                                  const std::vector<char>& buffer) {
		if (buffer.size() != geometry.bytesPerSector)
			return std::unexpected<std::string>("Invalid buffer size");

		LARGE_INTEGER offset;
		offset.QuadPart = static_cast<LONGLONG>(geometry.sectorOffset(sectorNumber));

		if (SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN) == 0)
			return std::unexpected<std::string>("Failed to set file pointer: " +
			                                    getLastErrorAsString());

		DWORD bytesWritten{0};
		if (!WriteFile(hDisk, buffer.data(), geometry.sectorSize(), &bytesWritten, NULL)) {
			return std::unexpected<std::string>("Failed to write sector: " +
			                                    getLastErrorAsString());
		}
		return bytesWritten;
	}

	std::expected<DiskGeometry, std::string> getDiskGeometry(const std::string& physicalDrive) {
		std::wstring wPhysicalDrive = utf8_decode(physicalDrive);

		HANDLE hDevice =
		    CreateFileW(wPhysicalDrive.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		                NULL, OPEN_EXISTING, 0, NULL);

		if (hDevice == INVALID_HANDLE_VALUE) {
			return std::unexpected<std::string>("Failed to open disk: " + getLastErrorAsString());
		}

		DISK_GEOMETRY dg{0};
		DWORD bytesReturned{0};

		BOOL success = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg,
		                               sizeof(dg), &bytesReturned, NULL);

		if (success) {
			DiskGeometry geometry{};
			geometry.bytesPerSector = dg.BytesPerSector;
			geometry.sectorsPerTrack = dg.SectorsPerTrack;
			geometry.tracksPerCylinder = dg.TracksPerCylinder;
			geometry.cylinders = dg.Cylinders.QuadPart;
			geometry.totalSize = dg.Cylinders.QuadPart * dg.TracksPerCylinder * dg.SectorsPerTrack *
			                     dg.BytesPerSector;

			CloseHandle(hDevice);
			return geometry;
		} else {
			CloseHandle(hDevice);
			return std::unexpected<std::string>("Failed to get disk geometry: " +
			                                    getLastErrorAsString());
		}
	}

	std::expected<std::vector<DriveInfo>, std::string> enumerateDrives() {
		std::vector<DriveInfo> drives;
		DWORD driveMask = GetLogicalDrives();

		if (driveMask == 0) {
			return std::unexpected<std::string>("Failed to get logical drives: " +
			                                    getLastErrorAsString());
		}

		wchar_t driveLetter = L'A';
		while (driveMask) {
			if (driveMask & 1) {
				std::wstring root = std::wstring(1, driveLetter) + L":\\";
				UINT driveType = GetDriveTypeW(root.c_str());

				std::string typeStr;
				switch (driveType) {
					case DRIVE_UNKNOWN:
						typeStr = "Unknown";
						break;
					case DRIVE_NO_ROOT_DIR:
						typeStr = "Invalid";
						break;
					case DRIVE_REMOVABLE:
						typeStr = "Removable";
						break;
					case DRIVE_FIXED:
						typeStr = "Fixed";
						break;
					case DRIVE_REMOTE:
						typeStr = "Network";
						break;
					case DRIVE_CDROM:
						typeStr = "CD-ROM";
						break;
					case DRIVE_RAMDISK:
						typeStr = "RAM Disk";
						break;
					default:
						typeStr = "Other";
						break;
				}

				// Volume label & file system
				wchar_t volumeName[MAX_PATH + 1] = {0};
				wchar_t fsName[MAX_PATH + 1] = {0};
				DWORD serial = 0, maxCompLen = 0, fsFlags = 0;

				std::wstring volLabel, fsType;
				if (GetVolumeInformationW(root.c_str(), volumeName, MAX_PATH, &serial, &maxCompLen,
				                          &fsFlags, fsName, MAX_PATH)) {
					volLabel = volumeName;
					fsType = fsName;
				} else {
					volLabel = L"(Unavailable)";
					fsType = L"(Unknown)";
				}

				// Detect floppy: A: or B: and removable
				bool isFloppy = false;
				if ((driveLetter == L'A' || driveLetter == L'B') && driveType == DRIVE_REMOVABLE) {
					isFloppy = true;
				}

				// --- Map volume to physical drive(s) ---
				std::wstring driveName = L"\\\\.\\" + std::wstring(1, driveLetter) + L":";

				DriveInfo info{utf8_encode(root),     utf8_encode(driveName), typeStr,
				               utf8_encode(volLabel), utf8_encode(fsType),    isFloppy};

				if (isFloppy) {
					info.physicalDrive = utf8_encode(driveName);
				}

				HANDLE hVol = CreateFileW(driveName.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
				                          NULL, OPEN_EXISTING, 0, NULL);

				if (hVol != INVALID_HANDLE_VALUE) {
					VOLUME_DISK_EXTENTS extents = {0};
					DWORD bytesReturned = 0;

					if (driveType == DRIVE_REMOVABLE && isFloppy) {
						auto geom = getDiskGeometry(utf8_encode(driveName).c_str());
						if (geom) {
							info.hasGeometry = true;
							info.geometry = *geom;
						}
					} else {
						if (DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
						                    &extents, sizeof(extents), &bytesReturned, NULL)) {
							// Take the first physical disk (most common case)
							int diskNumber = extents.Extents[0].DiskNumber;
							info.physicalDrive =
							    utf8_encode(L"\\\\.\\PhysicalDrive" + std::to_wstring(diskNumber));

							auto geom = getDiskGeometry(info.physicalDrive);
							if (geom) {
								info.hasGeometry = true;
								info.geometry = *geom;
							}
						}
					}
					CloseHandle(hVol);
				}

				drives.push_back(info);
			}

			driveMask >>= 1;
			driveLetter++;
		}

		return drives;
	}

#pragma pack(push, 1)
	struct FAT12BootSector {
		BYTE BS_jmpBoot[3];
		BYTE BS_OEMName[8];
		WORD BPB_BytsPerSec;
		BYTE BPB_SecPerClus;
		WORD BPB_RsvdSecCnt;
		BYTE BPB_NumFATs;
		WORD BPB_RootEntCnt;
		WORD BPB_TotSec16;
		BYTE BPB_Media;
		WORD BPB_FATSz16;
		WORD BPB_SecPerTrk;
		WORD BPB_NumHeads;
		DWORD BPB_HiddSec;
		DWORD BPB_TotSec32;
		BYTE BS_DrvNum;
		BYTE BS_Reserved1;
		BYTE BS_BootSig;
		DWORD BS_VolID;
		BYTE BS_VolLab[11];
		BYTE BS_FilSysType[8];
		BYTE bootstrap[448];
		WORD signature;  // 0xAA55
	};
#pragma pack(pop)

	// Function to print boot sector to ostream
	bool printFloppyBootSector(const std::string& drive, std::ostream& out) {
		std::wstring devicePath = utf8_decode(drive);

		HANDLE hFloppy =
		    CreateFileW(devicePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		                OPEN_EXISTING, 0, NULL);

		if (hFloppy == INVALID_HANDLE_VALUE) {
			out << "Failed to open floppy " << drive << ". Error: " << GetLastError() << std::endl;
			return false;
		}

		FAT12BootSector bs;
		DWORD read = 0;
		SetFilePointer(hFloppy, 0, NULL, FILE_BEGIN);
		if (!ReadFile(hFloppy, &bs, sizeof(bs), &read, NULL) || read != 512) {
			out << "Failed to read boot sector." << std::endl;
			CloseHandle(hFloppy);
			return false;
		}

		CloseHandle(hFloppy);

		out << "----- Floppy Boot Sector (" << drive << ") -----\n";
		out << "Jump Instruction: " << std::hex << std::setw(2) << std::setfill('0')
		    << (int)bs.BS_jmpBoot[0] << " " << std::hex << (int)bs.BS_jmpBoot[1] << " " << std::hex
		    << (int)bs.BS_jmpBoot[2] << std::dec << "\n";
		out << "OEM Name: " << std::string(bs.BS_OEMName, bs.BS_OEMName + 8).c_str() << "\n";
		out << "Bytes per Sector: " << bs.BPB_BytsPerSec << "\n";
		out << "Sectors per Cluster: " << (int)bs.BPB_SecPerClus << "\n";
		out << "Reserved Sectors: " << bs.BPB_RsvdSecCnt << "\n";
		out << "Number of FATs: " << (int)bs.BPB_NumFATs << "\n";
		out << "Root Entries: " << bs.BPB_RootEntCnt << "\n";
		out << "Total Sectors (16-bit): " << bs.BPB_TotSec16 << "\n";
		out << "Media Descriptor: 0x" << std::hex << (int)bs.BPB_Media << std::dec << "\n";
		out << "Sectors per FAT: " << bs.BPB_FATSz16 << "\n";
		out << "Sectors per Track: " << bs.BPB_SecPerTrk << "\n";
		out << "Number of Heads: " << bs.BPB_NumHeads << "\n";
		out << "Hidden Sectors: " << bs.BPB_HiddSec << "\n";
		out << "Total Sectors (32-bit): " << bs.BPB_TotSec32 << "\n";
		out << "Drive Number: " << (int)bs.BS_DrvNum << "\n";
		out << "Boot Signature: 0x" << std::hex << (int)bs.BS_BootSig << std::dec << "\n";
		out << "Volume ID: 0x" << std::hex << bs.BS_VolID << std::dec << "\n";
		out << "Volume Label: " << std::string(bs.BS_VolLab, bs.BS_VolLab + 11) << "\n";
		out << "File System Type: " << std::string(bs.BS_FilSysType, bs.BS_FilSysType + 8).c_str()
		    << "\n";
		out << "Boot Sector Signature: 0x" << std::hex << bs.signature << std::dec << "\n";
		out << "-----------------------------------------------------------\n";

		return true;
	}

}  // namespace drivefunc