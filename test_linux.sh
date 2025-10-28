#!/bin/bash

# FMU Storage API Linux Test Script
echo "=== FMU Storage API Linux Test ==="
echo

# Check if we're on Linux
if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    echo "❌ This script is designed for Linux"
    echo "Current OS: $OSTYPE"
    exit 1
fi

echo "✅ Running on Linux: $OSTYPE"
echo

# Check dependencies
echo "Checking dependencies..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Installing..."
    sudo apt-get update
    sudo apt-get install -y cmake build-essential
fi

# Check for g++
if ! command -v g++ &> /dev/null; then
    echo "❌ g++ not found. Installing..."
    sudo apt-get install -y g++
fi

echo "✅ Dependencies OK"
echo

# Build project
echo "Building project..."
mkdir -p build
cd build

cmake ..
if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    exit 1
fi

cmake --build . -j
if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

echo "✅ Build successful"
echo

# Run tests
echo "Running tests..."
echo

# Test 1: Basic functionality
echo "Test 1: Basic API functionality"
./fmu_example
if [ $? -eq 0 ]; then
    echo "✅ Test 1 passed"
else
    echo "❌ Test 1 failed"
    exit 1
fi
echo

# Test 2: Check file creation
echo "Test 2: File creation"
if [ -f "data/store.ndjson" ]; then
    echo "✅ Data file created successfully"
    echo "File contents:"
    cat data/store.ndjson
    echo
else
    echo "❌ Data file not created"
    exit 1
fi

# Test 3: Custom storage directory
echo "Test 3: Custom storage directory"
export FMU_STORAGE_DIR="/tmp/fmu_test"
mkdir -p /tmp/fmu_test
./fmu_example
if [ -f "/tmp/fmu_test/store.ndjson" ]; then
    echo "✅ Custom storage directory works"
    rm -rf /tmp/fmu_test
else
    echo "❌ Custom storage directory failed"
    exit 1
fi
echo

echo "🎉 All tests passed! Code is ready for Linux deployment."