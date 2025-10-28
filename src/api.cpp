#include "fmu/api.hpp"
#include "fmu/data_models.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#endif

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
// UTILITY FUNCTIONS - FOR FILE OPERATIONS
// ============================================================================

// Get storage directory path (from environment variable or default ./data)
std::string getStorageDir() {
	const char* env = ::getenv("FMU_STORAGE_DIR");
	if (env && *env) return std::string(env);
	return std::string("./data");
}

// Get main file path
std::string getActivePath() {
	return getStorageDir() + "/store.ndjson";
}

// Get temporary file path
std::string getTempPath() {
	return getStorageDir() + "/store.ndjson.tmp";
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

// API 1: WRITE NEW DATA (replace all existing data)
bool RestoreData(const std::vector<CompositeData>& records, std::string* errorMessage) {
	// Step 1: Create storage directory if it doesn't exist
	std::string err;
	if (!ensureDirExists(&err)) {
		if (errorMessage) *errorMessage = err;
		return false;
	}
	
	// Step 2: Open temporary file for writing
	const std::string tmpPath = getTempPath();
	const std::string dstPath = getActivePath();
	int fd = ::open(tmpPath.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0) {
		if (errorMessage) *errorMessage = std::string("Cannot create temp file: ") + std::strerror(errno);
		return false;
	}
	
	// Step 3: Write each record to file
	bool ok = true;
	for (const auto& r : records) {
		std::string line = recordToCSV(r);
		if (!writeAll(fd, line.data(), line.size())) { 
			ok = false; 
			break; 
		}
	}
	
	// Step 4: Ensure data is written to disk
	if (ok) {
#ifdef _WIN32
		if (::_commit(fd) != 0) ok = false;
#else
		if (::fsync(fd) != 0) ok = false;
#endif
	}
	
	// Step 5: Close file
	int savedErrno = errno;
	::close(fd);
	
	// Step 6: If error, remove temp file and report error
	if (!ok) {
		if (errorMessage) *errorMessage = std::string("Write failed: ") + std::strerror(savedErrno);
		::unlink(tmpPath.c_str());
		return false;
	}
	
	// Step 7: Replace old file with new file
	if (::rename(tmpPath.c_str(), dstPath.c_str()) != 0) {
		// If destination file exists, remove it first
		::unlink(dstPath.c_str());
		if (::rename(tmpPath.c_str(), dstPath.c_str()) != 0) {
			if (errorMessage) *errorMessage = std::string("Rename failed: ") + std::strerror(errno);
			::unlink(tmpPath.c_str());
			return false;
		}
	}
	
	return true;
}

// API 2: DELETE OLD DATA (remove records with timestamp < olderThanTimestampMs)
bool DeleteOldData(int64_t olderThanTimestampMs, std::string* errorMessage) {
	// Step 1: Open file for reading
	const std::string srcPath = getActivePath();
	const std::string tmpPath = getTempPath();
	int inFd = ::open(srcPath.c_str(), O_RDONLY);
	if (inFd < 0) {
		if (errno == ENOENT) return true; // File doesn't exist, nothing to delete
		if (errorMessage) *errorMessage = std::string("Cannot open file: ") + std::strerror(errno);
		return false;
	}
	
	// Step 2: Read entire file content
	std::string content;
	bool ok = readAll(inFd, content);
	int savedErrno = errno;
	::close(inFd);
	if (!ok) {
		if (errorMessage) *errorMessage = std::string("Read failed: ") + std::strerror(savedErrno);
		return false;
	}
	
	// Step 3: Open temporary file for writing
	int outFd = ::open(tmpPath.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (outFd < 0) {
		if (errorMessage) *errorMessage = std::string("Cannot create temp file: ") + std::strerror(errno);
		return false;
	}
	
	// Step 4: Process each line, keep only new lines
	fmu::CompositeData rec{};
	size_t start = 0;
	for (size_t i = 0; i <= content.size(); ++i) {
		if (i == content.size() || content[i] == '\n') {
			std::string line(content.data() + start, i - start);
			start = i + 1;
			if (line.empty()) continue;
			
			// Convert CSV line to record
			if (!csvToRecord(line, rec)) continue;
			
			// Keep only records with timestamp >= olderThanTimestampMs
			if (rec.location.timestampMs >= olderThanTimestampMs) {
				std::string outLine = line + "\n";
				if (!writeAll(outFd, outLine.data(), outLine.size())) { 
					ok = false; 
					break; 
				}
			}
		}
	}
	
	// Step 5: Ensure data is written to disk
	if (ok) {
#ifdef _WIN32
		if (::_commit(outFd) != 0) ok = false;
#else
		if (::fsync(outFd) != 0) ok = false;
#endif
	}
	savedErrno = errno;
	::close(outFd);
	
	// Step 6: If error, remove temp file
	if (!ok) {
		if (errorMessage) *errorMessage = std::string("Write failed: ") + std::strerror(savedErrno);
		::unlink(tmpPath.c_str());
		return false;
	}
	
	// Step 7: Replace old file with new file
	if (::rename(tmpPath.c_str(), srcPath.c_str()) != 0) {
		// If destination file exists, remove it first
		::unlink(srcPath.c_str());
		if (::rename(tmpPath.c_str(), srcPath.c_str()) != 0) {
			if (errorMessage) *errorMessage = std::string("Rename failed: ") + std::strerror(errno);
			::unlink(tmpPath.c_str());
			return false;
		}
	}
	
	return true;
}

// API 3: READ DATA (get records in time range [fromTsMs, toTsMs])
std::vector<CompositeData> RetrieveData(int64_t fromTsMs, int64_t toTsMs, std::string* errorMessage) {
	std::vector<CompositeData> result;
	
	// Step 1: Open file for reading
	const std::string srcPath = getActivePath();
	int inFd = ::open(srcPath.c_str(), O_RDONLY);
	if (inFd < 0) {
		if (errno == ENOENT) return result; // File doesn't exist, return empty list
		if (errorMessage) *errorMessage = std::string("Cannot open file: ") + std::strerror(errno);
		return result;
	}
	
	// Step 2: Read entire file content
	std::string content;
	bool ok = readAll(inFd, content);
	int savedErrno = errno;
	::close(inFd);
	if (!ok) {
		if (errorMessage) *errorMessage = std::string("Read failed: ") + std::strerror(savedErrno);
		return result;
	}
	
	// Step 3: Process each line, filter by time range
	fmu::CompositeData rec{};
	size_t start = 0;
	for (size_t i = 0; i <= content.size(); ++i) {
		if (i == content.size() || content[i] == '\n') {
			std::string line(content.data() + start, i - start);
			start = i + 1;
			if (line.empty()) continue;
			
			// Convert CSV line to record
			if (!csvToRecord(line, rec)) continue;
			
			// Only get records within requested time range
			if (rec.location.timestampMs >= fromTsMs && rec.location.timestampMs <= toTsMs) {
				result.push_back(rec);
			}
		}
	}
	
	return result;
}

} // namespace fmu
