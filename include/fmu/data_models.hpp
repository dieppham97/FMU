#pragma once

#include <cstdint>

namespace fmu {

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Location data
struct LocationData {
	int64_t timestampMs;    // Time (milliseconds)
	double latitude;        // Latitude
	double longitude;       // Longitude  
	double accurate;        // Receiver's estimated horizontal error radius (meters), range [0.5:5]
	bool valid;             // Is valid
	int32_t fixType;        // GPS fix type: 0=NO_FIX, 1=DEAD_RECKONING, 2=2D_FIX, 3=3D_FIX, 4=GPS_DEAD_RECKONING, 5=TIME_ONLY
};

// Device status
struct DeviceStatus {
	int32_t powerStage;     // Power stage: 0=POWER_OFF, 1=POWER_ON, 2=POWER_ACTIVE, 3=POWER_STANDBY, 4=POWER_SLEEP, 5=POWER_CRITICAL
};

// Vehicle status
struct VehicleStatus {
	double vehicleSpeed;    // Speed (km/h)
	double acceleration;    // Acceleration (m/s²)
	double fuelLevelPct;    // Fuel level (%)
	double cargoWeight;     // Cargo weight (kg)
};

// Composite data (contains all above information)
struct CompositeData {
	LocationData location;  // Location information
	DeviceStatus device;    // Device information
	VehicleStatus vehicle;  // Vehicle information
};

} // namespace fmu
