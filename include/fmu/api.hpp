#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "fmu/data_models.hpp"

namespace fmu {

// ============================================================================
// DATA TYPE ENUM
// ============================================================================

enum class DataType {
	GPS_DATA,                    // GPS data
	DRIVER_INFORMATION,          // Driver information
	DRIVER_VIOLATION_BEHAVIOR    // Driver violation behavior
};

// ============================================================================
// 3 MAIN APIs FOR USERS
// ============================================================================

// API 1: WRITE NEW DATA (append data to date-based file)
// - dataType: type of data to save (GPS_DATA, DRIVER_INFORMATION, DRIVER_VIOLATION_BEHAVIOR)
// - records: list of data to write (will be appended to file for current date)
// - errorMessage: error message (can be nullptr)
// - Returns: true on success, false on error
// - Note: Creates a new file each day with format: {data_type}_YYYY_MM_DD.txt
//         Always appends new data, never deletes existing data in file
bool RestoreData(DataType dataType, const std::vector<CompositeData>& records, std::string* errorMessage = nullptr);

// API 2: DELETE OLD FILES (delete files older than specified days)
// - dataType: type of data to delete files for
// - daysOlder: delete files older than this many days from current time
// - errorMessage: error message (can be nullptr)
// - Returns: true on success, false on error
// - Note: Deletes entire files, not data within files. Used when storage is full.
bool DeleteOldData(DataType dataType, int daysOlder, std::string* errorMessage = nullptr);

// API 3: READ DATA (get last data from newest file, like top function)
// - dataType: type of data to retrieve (same as RestoreData parameter)
// - fromTsMs: start timestamp (milliseconds) - kept for compatibility but not used
// - toTsMs: end timestamp (milliseconds) - kept for compatibility but not used
// - errorMessage: error message (can be nullptr)
// - Returns: list containing the last data block from the newest file for the data type
// - Note: Retrieves the last data block (most recent record) from the newest file (file with latest date)
//         Similar to a "top" function in arrays, returns the last data entry from the newest file
std::vector<CompositeData> RetrieveData(DataType dataType, int64_t fromTsMs, int64_t toTsMs, std::string* errorMessage = nullptr);

} // namespace fmu
