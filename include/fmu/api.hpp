#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "fmu/data_models.hpp"

namespace fmu {

// ============================================================================
// 3 MAIN APIs FOR USERS
// ============================================================================

// API 1: WRITE NEW DATA (replace all existing data)
// - records: list of data to write
// - errorMessage: error message (can be nullptr)
// - Returns: true on success, false on error
bool RestoreData(const std::vector<CompositeData>& records, std::string* errorMessage = nullptr);

// API 2: DELETE OLD DATA (remove records with timestamp < olderThanTimestampMs)
// - olderThanTimestampMs: delete records older than this timestamp (milliseconds)
// - errorMessage: error message (can be nullptr)
// - Returns: true on success, false on error
bool DeleteOldData(int64_t olderThanTimestampMs, std::string* errorMessage = nullptr);

// API 3: READ DATA (get records in time range [fromTsMs, toTsMs])
// - fromTsMs: start timestamp (milliseconds)
// - toTsMs: end timestamp (milliseconds)
// - errorMessage: error message (can be nullptr)
// - Returns: list of records within the time range
std::vector<CompositeData> RetrieveData(int64_t fromTsMs, int64_t toTsMs, std::string* errorMessage = nullptr);

} // namespace fmu
