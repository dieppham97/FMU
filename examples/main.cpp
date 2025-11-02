#include "fmu/api.hpp"

#include <cstdio>
#include <string>
#include <vector>

int main() {
	printf("=== FMU Storage API Demo ===\n\n");
	
	std::string err;
	
	// Create sample data
	printf("1. Creating sample data...\n");
	std::vector<fmu::CompositeData> records;
	fmu::CompositeData r{};
	r.location.timestampMs = 1730000000000LL;  // 2024-10-28
	r.location.latitude = 10.762622;           // Ho Chi Minh City
	r.location.longitude = 106.660172;
	r.location.accurate = 5.0;
	r.location.valid = true;
	r.location.fixType = 3;
	r.device.powerStage = 2;
	r.vehicle.vehicleSpeed = 12.3;
	r.vehicle.acceleration = 0.4;
	r.vehicle.fuelLevelPct = 55.0;
	r.vehicle.cargoWeight = 1000.0;
	records.push_back(r);
	printf("   ✓ Created 1 sample record\n\n");

	// Test API 1: RestoreData - Write to all 3 data types
	printf("2. Writing data to different data types (RestoreData)...\n");
	
	// GPS Data
	if (!fmu::RestoreData(fmu::DataType::GPS_DATA, records, &err)) {
		printf("   ❌ GPS_DATA Error: %s\n", err.c_str());
		return 1;
	}
	printf("   ✓ GPS_DATA written to file\n");
	
	// Driver Information
	if (!fmu::RestoreData(fmu::DataType::DRIVER_INFORMATION, records, &err)) {
		printf("   ❌ DRIVER_INFORMATION Error: %s\n", err.c_str());
		return 1;
	}
	printf("   ✓ DRIVER_INFORMATION written to file\n");
	
	// Driver Violation Behavior
	if (!fmu::RestoreData(fmu::DataType::DRIVER_VIOLATION_BEHAVIOR, records, &err)) {
		printf("   ❌ DRIVER_VIOLATION_BEHAVIOR Error: %s\n", err.c_str());
		return 1;
	}
	printf("   ✓ DRIVER_VIOLATION_BEHAVIOR written to file\n");
	printf("\n");

	// Test API 2: DeleteOldData (delete files older than X days)
	printf("3. Deleting old files (DeleteOldData)...\n");
	if (!fmu::DeleteOldData(fmu::DataType::GPS_DATA, 30, &err)) {
		printf("   ❌ Error: %s\n", err.c_str());
		return 1;
	}
	printf("   ✓ Successfully deleted old files (older than 30 days)\n\n");

	// Test API 3: RetrieveData - Read from all 3 data types
	printf("4. Reading data from different data types (RetrieveData)...\n");
	
	// GPS Data
	auto gpsOut = fmu::RetrieveData(fmu::DataType::GPS_DATA, 0, 0, &err);
	if (!err.empty()) {
		printf("   ⚠️  GPS_DATA Warning: %s\n", err.c_str());
		err.clear();
	}
	printf("   ✓ GPS_DATA: Retrieved %zu record(s)\n", gpsOut.size());
	
	// Driver Information
	auto driverOut = fmu::RetrieveData(fmu::DataType::DRIVER_INFORMATION, 0, 0, &err);
	if (!err.empty()) {
		printf("   ⚠️  DRIVER_INFORMATION Warning: %s\n", err.c_str());
		err.clear();
	}
	printf("   ✓ DRIVER_INFORMATION: Retrieved %zu record(s)\n", driverOut.size());
	
	// Driver Violation Behavior
	auto violationOut = fmu::RetrieveData(fmu::DataType::DRIVER_VIOLATION_BEHAVIOR, 0, 0, &err);
	if (!err.empty()) {
		printf("   ⚠️  DRIVER_VIOLATION_BEHAVIOR Warning: %s\n", err.c_str());
		err.clear();
	}
	printf("   ✓ DRIVER_VIOLATION_BEHAVIOR: Retrieved %zu record(s)\n", violationOut.size());
	
	if (!gpsOut.empty()) {
		printf("\n   📍 GPS Location: %.6f, %.6f\n", gpsOut[0].location.latitude, gpsOut[0].location.longitude);
		printf("   🚗 Speed: %.1f km/h\n", gpsOut[0].vehicle.vehicleSpeed);
		printf("   ⛽ Fuel: %.1f%%\n", gpsOut[0].vehicle.fuelLevelPct);
	}
	
	printf("\n=== Complete! ===\n");
	return 0;
}
