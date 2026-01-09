#!/bin/bash
set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}[*] Starting Saturn Project Setup & Test...${NC}"

# 1. Install Prerequisites
echo -e "${GREEN}[*] Installing dependencies...${NC}"
if [ -f /etc/debian_version ]; then
    # Update and install for Debian/Ubuntu
    sudo apt-get update -y
    sudo apt-get install -y build-essential cmake libgtest-dev
elif [ -f /etc/redhat-release ]; then
    # Update and install for CentOS/RHEL (approximate)
    sudo yum groupinstall -y "Development Tools"
    sudo yum install -y cmake gtest-devel
else
    echo -e "${RED}[!] Unsupported OS. Please install cmake and gtest manually.${NC}"
fi

# 2. Build GTest (Required on some Ubuntu versions if pre-built libs are missing)
# Sometimes libgtest-dev only installs sources in /usr/src/gtest
if [ -d "/usr/src/gtest" ] && [ ! -f "/usr/lib/libgtest.a" ] && [ ! -f "/usr/lib/x86_64-linux-gnu/libgtest.a" ]; then
    echo -e "${GREEN}[*] Compiling GTest from source...${NC}"
    cd /usr/src/gtest
    sudo cmake CMakeLists.txt
    sudo make
    sudo cp *.a /usr/lib
    cd -
fi

# 3. Build the Test Project
echo -e "${GREEN}[*] Building Satord Tests...${NC}"
TEST_DIR="./tests/native"
BUILD_DIR="${TEST_DIR}/build"

if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR" || exit
cmake ..
make

# 4. Run the Tests
echo -e "${GREEN}[*] Running Tests...${NC}"
if ./satord_test; then
    echo -e "${GREEN}[SUCCESS] All tests passed!${NC}"
else
    echo -e "${RED}[FAILURE] Some tests failed.${NC}"
    exit 1
fi
