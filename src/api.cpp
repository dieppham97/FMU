#include "fmu/api.hpp"
#include "fmu/data_models.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#endif

#include <ctime>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

// ============================================================================
// UTILITY FUNCTIONS - FOR FILE OPERATIONS
// ============================================================================

// Get storage directory path (from environment variable or default ./data)
std::string getStorageDir() {
	const char* env = ::getenv("FMU_STORAGE_DIR");
	if (env && *env) return std::string(env);
	return std::string("./data");
}

// Convert DataType to string for filename
std::string dataTypeToString(fmu::DataType dataType) {
	switch (dataType) {
		case fmu::DataType::GPS_DATA:
			return "GPS_data";
		case fmu::DataType::DRIVER_INFORMATION:
			return "Driver_information";
		case fmu::DataType::DRIVER_VIOLATION_BEHAVIOR:
			return "Driver_violation_behavior";
		default:
			return "Unknown";
	}
}

// Get current date string in format YYYY_MM_DD
std::string getCurrentDateString() {
	std::time_t now = std::time(nullptr);
	struct tm* timeinfo;
#ifdef _WIN32
	// Use thread-safe localtime on Windows (localtime_s is MSVC-specific)
	timeinfo = std::localtime(&now);
	if (!timeinfo) return "1970_01_01"; // Fallback
#else
	struct tm timeinfoBuf;
	timeinfo = localtime_r(&now, &timeinfoBuf);
	if (!timeinfo) return "1970_01_01"; // Fallback
#endif
	std::ostringstream oss;
	oss << std::setfill('0') 
		<< (1900 + timeinfo->tm_year) << '_'
		<< std::setw(2) << (timeinfo->tm_mon + 1) << '_'
		<< std::setw(2) << timeinfo->tm_mday;
	return oss.str();
}

// Get file path separator
std::string getPathSeparator() {
#ifdef _WIN32
	return "\\";
#else
	return "/";
#endif
}

// Get file path for a data type and date
std::string getFilePathForDate(fmu::DataType dataType, const std::string& dateStr) {
	std::string typeStr = dataTypeToString(dataType);
	return getStorageDir() + getPathSeparator() + typeStr + "_" + dateStr + ".txt";
}

// Get file path for current date
std::string getCurrentFilePath(fmu::DataType dataType) {
	return getFilePathForDate(dataType, getCurrentDateString());
}

// Parse date from filename (format: {type}_YYYY_MM_DD.txt)
// Returns empty string if parsing fails
std::string parseDateFromFilename(const std::string& filename, const std::string& expectedPrefix) {
	// Expected format: {prefix}_YYYY_MM_DD.txt
	std::string prefix = expectedPrefix + "_";
	if (filename.size() < prefix.size() + 11) return ""; // YYYY_MM_DD.txt = 11 chars
	
	if (filename.substr(0, prefix.size()) != prefix) return "";
	if (filename.substr(filename.size() - 4) != ".txt") return "";
	
	std::string datePart = filename.substr(prefix.size(), 10); // YYYY_MM_DD = 10 chars
	// Basic validation: check format YYYY_MM_DD
	if (datePart.size() == 10 && datePart[4] == '_' && datePart[7] == '_') {
		return datePart;
	}
	return "";
}

// Convert date string (YYYY_MM_DD) to timestamp (milliseconds since epoch)
int64_t dateStringToTimestamp(const std::string& dateStr) {
	if (dateStr.size() != 10) return 0;
	
	try {
		int year = std::stoi(dateStr.substr(0, 4));
		int month = std::stoi(dateStr.substr(5, 2));
		int day = std::stoi(dateStr.substr(8, 2));
		
		struct tm timeinfo = {};
		timeinfo.tm_year = year - 1900;
		timeinfo.tm_mon = month - 1;
		timeinfo.tm_mday = day;
		timeinfo.tm_hour = 0;
		timeinfo.tm_min = 0;
		timeinfo.tm_sec = 0;
		
		std::time_t t = std::mktime(&timeinfo);
		if (t == -1) return 0;
		return static_cast<int64_t>(t) * 1000; // Convert to milliseconds
	} catch (...) {
		return 0;
	}
}

// Get list of files for a data type, sorted by date (newest first)
std::vector<std::string> getFilesForDataType(fmu::DataType dataType) {
	std::vector<std::string> files;
	std::string prefix = dataTypeToString(dataType);
	std::string dir = getStorageDir();
	
#ifdef _WIN32
	WIN32_FIND_DATAA findData;
	std::string sep = getPathSeparator();
	std::string pattern = dir + sep + prefix + "_*.txt";
	HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				std::string filename(findData.cFileName);
				std::string dateStr = parseDateFromFilename(filename, prefix);
				if (!dateStr.empty()) {
					files.push_back(dir + sep + filename);
				}
			}
		} while (FindNextFileA(hFind, &findData));
		FindClose(hFind);
	}
#else
	DIR* dp = opendir(dir.c_str());
	if (dp != nullptr) {
		struct dirent* entry;
		while ((entry = readdir(dp)) != nullptr) {
			std::string filename = entry->d_name;
			std::string dateStr = parseDateFromFilename(filename, prefix);
			if (!dateStr.empty()) {
				std::string sep = getPathSeparator();
				files.push_back(dir + sep + filename);
			}
		}
		closedir(dp);
	}
#endif
	
	// Sort files by date (newest first)
	std::sort(files.begin(), files.end(), [&prefix](const std::string& a, const std::string& b) {
		std::string dateA = parseDateFromFilename(a.substr(a.find_last_of("/\\") + 1), prefix);
		std::string dateB = parseDateFromFilename(b.substr(b.find_last_of("/\\") + 1), prefix);
		return dateA > dateB; // Newest first
	});
	
	return files;
}

// Get newest file path for a data type
std::string getNewestFilePath(fmu::DataType dataType) {
	auto files = getFilesForDataType(dataType);
	if (files.empty()) return "";
	return files[0]; // Already sorted newest first
}

// Create directory if it doesn't exist
bool ensureDirExists(std::string* err) {
	std::string dir = getStorageDir();
	struct stat st{};
	if (::stat(dir.c_str(), &st) == 0) {
		if (S_ISDIR(st.st_mode)) return true;
		if (err) *err = "Storage path exists but is not a directory: " + dir;
		return false;
	}
#ifdef _WIN32
	if (::mkdir(dir.c_str()) == 0) return true;
#else
	if (::mkdir(dir.c_str(), 0755) == 0) return true;
#endif
	if (errno == EEXIST) return true;
	if (err) *err = std::string("mkdir failed: ") + std::strerror(errno);
	return false;
}

// Write all data to file
bool writeAll(int fd, const void* buf, size_t len) {
	const char* p = static_cast<const char*>(buf);
	size_t left = len;
	while (left > 0) {
		ssize_t n = ::write(fd, p, left);
		if (n < 0) {
			if (errno == EINTR) continue;
			return false;
		}
		p += static_cast<size_t>(n);
		left -= static_cast<size_t>(n);
	}
	return true;
}

// Read entire file content
bool readAll(int fd, std::string& out) {
	char buf[8192];
	for (;;) {
		ssize_t n = ::read(fd, buf, sizeof(buf));
		if (n == 0) break;
		if (n < 0) {
			if (errno == EINTR) continue;
			return false;
		}
		out.append(buf, static_cast<size_t>(n));
	}
	return true;
}

// ============================================================================
// DATA CONVERSION FUNCTIONS
// ============================================================================

// Convert one record to one CSV line
std::string recordToCSV(const fmu::CompositeData& r) {
	std::ostringstream oss;
	oss.setf(std::ios::fixed);
	oss.precision(9);
	oss << r.location.timestampMs << ','
		<< r.location.latitude << ','
		<< r.location.longitude << ','
		<< r.location.accurate << ','
		<< (r.location.valid ? 1 : 0) << ','
		<< r.location.fixType << ','
		<< r.device.powerStage << ','
		<< r.vehicle.vehicleSpeed << ','
		<< r.vehicle.acceleration << ','
		<< r.vehicle.fuelLevelPct << ','
		<< r.vehicle.cargoWeight << '\n';
	return oss.str();
}

// Convert one CSV line to one record
bool csvToRecord(const std::string& line, fmu::CompositeData& out) {
	// Split string by comma
	std::vector<std::string> fields;
	std::string field;
	for (char c : line) {
		if (c == ',') {
			fields.push_back(field);
			field.clear();
		} else {
			field += c;
		}
	}
	fields.push_back(field); // Add last field
	
	// Check if we have exactly 11 fields
	if (fields.size() != 11) return false;
	
	// Convert each field
	try {
		out.location.timestampMs = std::stoll(fields[0]);
		out.location.latitude = std::stod(fields[1]);
		out.location.longitude = std::stod(fields[2]);
		out.location.accurate = std::stod(fields[3]);
		out.location.valid = (std::stoll(fields[4]) != 0);
		out.location.fixType = std::stoi(fields[5]);
		out.device.powerStage = std::stoi(fields[6]);
		out.vehicle.vehicleSpeed = std::stod(fields[7]);
		out.vehicle.acceleration = std::stod(fields[8]);
		out.vehicle.fuelLevelPct = std::stod(fields[9]);
		out.vehicle.cargoWeight = std::stod(fields[10]);
		return true;
	} catch (...) {
		return false;
	}
}

// ============================================================================
// 3 MAIN APIs FOR USERS
// ============================================================================

namespace fmu {

// API 1: WRITE NEW DATA (append data to date-based file)
bool RestoreData(DataType dataType, const std::vector<CompositeData>& records, std::string* errorMessage) {
	// Step 1: Create storage directory if it doesn't exist
	std::string err;
	if (!ensureDirExists(&err)) {
		if (errorMessage) *errorMessage = err;
		return false;
	}
	
	// Step 2: Get file path for current date
	const std::string filePath = getCurrentFilePath(dataType);
	
	// Step 3: Open file for appending (O_APPEND) or create if doesn't exist
	int fd = ::open(filePath.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
	if (fd < 0) {
		if (errorMessage) *errorMessage = std::string("Cannot open file for appending: ") + std::strerror(errno);
		return false;
	}
	
	// Step 4: Write each record to file (append mode)
	bool ok = true;
	for (const auto& r : records) {
		std::string line = recordToCSV(r);
		if (!writeAll(fd, line.data(), line.size())) { 
			ok = false; 
			break; 
		}
	}
	
	// Step 5: Ensure data is written to disk
	if (ok) {
#ifdef _WIN32
		if (::_commit(fd) != 0) ok = false;
#else
		if (::fsync(fd) != 0) ok = false;
#endif
	}
	
	// Step 6: Close file
	int savedErrno = errno;
	::close(fd);
	
	// Step 7: Report error if any
	if (!ok) {
		if (errorMessage) *errorMessage = std::string("Write failed: ") + std::strerror(savedErrno);
		return false;
	}
	
	return true;
}

// API 2: DELETE OLD FILES (delete files older than specified days)
bool DeleteOldData(DataType dataType, int daysOlder, std::string* errorMessage) {
	if (daysOlder < 0) {
		if (errorMessage) *errorMessage = "daysOlder must be non-negative";
		return false;
	}
	
	// Step 1: Get current date string (YYYY_MM_DD)
	std::string currentDateStr = getCurrentDateString();
	
	// Step 2: Parse current date to calculate cutoff date
	// Extract year, month, day from current date
	if (currentDateStr.size() != 10) {
		if (errorMessage) *errorMessage = "Invalid current date format";
		return false;
	}
	
	try {
		int currentYear = std::stoi(currentDateStr.substr(0, 4));
		int currentMonth = std::stoi(currentDateStr.substr(5, 2));
		int currentDay = std::stoi(currentDateStr.substr(8, 2));
		
		// Calculate cutoff date by subtracting daysOlder
		struct tm cutoffTime = {};
		cutoffTime.tm_year = currentYear - 1900;
		cutoffTime.tm_mon = currentMonth - 1;
		cutoffTime.tm_mday = currentDay - daysOlder;
		cutoffTime.tm_hour = 0;
		cutoffTime.tm_min = 0;
		cutoffTime.tm_sec = 0;
		
		// Normalize the date (handles month/year overflow)
		std::mktime(&cutoffTime);
		
		// Convert cutoff date back to string for comparison
		std::ostringstream cutoffOss;
		cutoffOss << std::setfill('0')
			<< (1900 + cutoffTime.tm_year) << '_'
			<< std::setw(2) << (cutoffTime.tm_mon + 1) << '_'
			<< std::setw(2) << cutoffTime.tm_mday;
		std::string cutoffDateStr = cutoffOss.str();
		
		// Step 3: Get all files for this data type
		auto files = getFilesForDataType(dataType);
		
		// Step 4: Delete files older than cutoff date
		std::string prefix = dataTypeToString(dataType);
		int deletedCount = 0;
		
		for (const auto& filePath : files) {
			// Extract filename from full path
			size_t lastSep = filePath.find_last_of("/\\");
			std::string filename = (lastSep == std::string::npos) ? filePath : filePath.substr(lastSep + 1);
			
			// Parse date from filename
			std::string fileDateStr = parseDateFromFilename(filename, prefix);
			if (fileDateStr.empty()) continue;
			
			// Compare date strings directly (YYYY_MM_DD format sorts lexicographically)
			// Delete file if file date < cutoff date
			if (fileDateStr < cutoffDateStr) {
				if (::unlink(filePath.c_str()) != 0) {
					// Log error but continue deleting other files
					if (errorMessage && errorMessage->empty()) {
						*errorMessage = std::string("Failed to delete some files: ") + std::strerror(errno);
					}
				} else {
					deletedCount++;
				}
			}
		}
		
		return true; // Return true even if some deletions failed (we tried our best)
	} catch (...) {
		if (errorMessage) *errorMessage = "Error parsing dates";
		return false;
	}
}

// API 3: READ DATA (get last data from newest file, like top function)
std::vector<CompositeData> RetrieveData(DataType dataType, int64_t fromTsMs, int64_t toTsMs, std::string* errorMessage) {
	std::vector<CompositeData> result;
	
	// Step 1: Get newest file path for this data type
	std::string srcPath = getNewestFilePath(dataType);
	if (srcPath.empty()) {
		// No files exist for this data type, return empty list
		return result;
	}
	
	// Step 2: Open file for reading
	int inFd = ::open(srcPath.c_str(), O_RDONLY);
	if (inFd < 0) {
		if (errno == ENOENT) return result; // File doesn't exist, return empty list
		if (errorMessage) *errorMessage = std::string("Cannot open file: ") + std::strerror(errno);
		return result;
	}
	
	// Step 3: Read entire file content
	std::string content;
	bool ok = readAll(inFd, content);
	int savedErrno = errno;
	::close(inFd);
	if (!ok) {
		if (errorMessage) *errorMessage = std::string("Read failed: ") + std::strerror(savedErrno);
		return result;
	}
	
	// Step 4: Parse all records from file
	std::vector<CompositeData> allRecords;
	fmu::CompositeData rec{};
	size_t start = 0;
	for (size_t i = 0; i <= content.size(); ++i) {
		if (i == content.size() || content[i] == '\n') {
			std::string line(content.data() + start, i - start);
			start = i + 1;
			if (line.empty()) continue;
			
			// Convert CSV line to record
			if (csvToRecord(line, rec)) {
				allRecords.push_back(rec);
			}
		}
	}
	
	// Step 5: Return the last data block (like top function)
	// Get the last block of data from the file
	// We return the last record(s) from the file (the most recent data)
	// For now, we return the last record as it represents the most recent data point
	// If there are multiple records with same timestamp at the end, we could return all of them
	if (!allRecords.empty()) {
		// Return the last record (most recent data in file)
		result.push_back(allRecords.back());
	}
	
	return result;
}

} // namespace fmu
