#include "fmu/api.hpp"

#include <cstdio>
#include <string>
#include <vector>

int main() {
	printf("=== FMU Storage API Demo ===\n\n");
	
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

	// Test API 1: RestoreData
	printf("2. Writing data (RestoreData)...\n");
	std::string err;
	if (!fmu::RestoreData(records, &err)) {
		printf("   ❌ Error: %s\n", err.c_str());
		return 1;
	}
	printf("   ✓ Successfully written to file\n\n");

	// Test API 2: DeleteOldData
	printf("3. Deleting old data (DeleteOldData)...\n");
	if (!fmu::DeleteOldData(1729999999000LL, &err)) {
		printf("   ❌ Error: %s\n", err.c_str());
		return 1;
	}
	printf("   ✓ Successfully deleted old data\n\n");

	// Test API 3: RetrieveData
	printf("4. Reading data (RetrieveData)...\n");
	auto out = fmu::RetrieveData(1729999999000LL, 1730000001000LL, &err);
	if (!err.empty()) {
		printf("   ⚠️  Warning: %s\n", err.c_str());
	}
	printf("   ✓ Retrieved %zu record(s)\n", out.size());
	
	if (!out.empty()) {
		printf("   📍 Location: %.6f, %.6f\n", out[0].location.latitude, out[0].location.longitude);
		printf("   🚗 Speed: %.1f km/h\n", out[0].vehicle.vehicleSpeed);
		printf("   ⛽ Fuel: %.1f%%\n", out[0].vehicle.fuelLevelPct);
	}
	
	printf("\n=== Complete! ===\n");
	return 0;
}
