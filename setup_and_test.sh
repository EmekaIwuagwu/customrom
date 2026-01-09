#!/bin/bash
set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}[*] Starting Saturn Project Setup & Test...${NC}"

# ==========================================
# 1. Native / C++ Test Setup
# ==========================================
echo -e "${GREEN}[*] [Native] Installing dependencies...${NC}"
if [ -f /etc/debian_version ]; then
    sudo apt-get update -y
    # Install JDK and C++ tools
    sudo apt-get install -y build-essential cmake libgtest-dev openjdk-17-jdk wget unzip
else
    echo -e "${RED}[!] Unsupported OS. Please install deps manually.${NC}"
fi

# Build GTest if needed
if [ -d "/usr/src/gtest" ] && [ ! -f "/usr/lib/libgtest.a" ] && [ ! -f "/usr/lib/x86_64-linux-gnu/libgtest.a" ]; then
    echo -e "${GREEN}[*] [Native] Compiling GTest from source...${NC}"
    cd /usr/src/gtest
    sudo cmake CMakeLists.txt
    sudo make
    sudo cp *.a /usr/lib
    cd -
fi

# Build Native Tests
echo -e "${GREEN}[*] [Native] Building Satord Tests...${NC}"
TEST_DIR="./tests/native"
BUILD_DIR="${TEST_DIR}/build"

if [ -d "$BUILD_DIR" ]; then rm -rf "$BUILD_DIR"; fi
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR" || exit
cmake ..
make

# Run Native Tests
echo -e "${GREEN}[*] [Native] Running Tests...${NC}"
if ./satord_test; then
    echo -e "${GREEN}[SUCCESS] Native tests passed!${NC}"
else
    echo -e "${RED}[FAILURE] Native tests failed.${NC}"
    exit 1
fi

cd ../../../ # Back to root

# ==========================================
# 2. Java / Service Logic Test Setup
# ==========================================
echo -e "${GREEN}[*] [Java] Setting up test environment...${NC}"

# Download Test Dependencies (JUnit, Mockito)
mkdir -p libs
cd libs
BASE_URL="https://repo1.maven.org/maven2"

download_jar() {
    if [ ! -f "$1" ]; then
        echo "Downloading $1..."
        wget -q "$2" -O "$1"
    fi
}

download_jar "junit-4.13.2.jar" "$BASE_URL/junit/junit/4.13.2/junit-4.13.2.jar"
download_jar "hamcrest-core-1.3.jar" "$BASE_URL/org/hamcrest/hamcrest-core/1.3/hamcrest-core-1.3.jar"
download_jar "mockito-core-5.11.0.jar" "$BASE_URL/org/mockito/mockito-core/5.11.0/mockito-core-5.11.0.jar"
download_jar "byte-buddy-1.14.12.jar" "$BASE_URL/net/bytebuddy/byte-buddy/1.14.12/byte-buddy-1.14.12.jar"
download_jar "byte-buddy-agent-1.14.12.jar" "$BASE_URL/net/bytebuddy/byte-buddy-agent/1.14.12/byte-buddy-agent-1.14.12.jar"
download_jar "objenesis-3.3.jar" "$BASE_URL/org/objenesis/objenesis/3.3/objenesis-3.3.jar"

cd ..

# Compile Java
echo -e "${GREEN}[*] [Java] Compiling Service and Tests...${NC}"
rm -rf build_java
mkdir -p build_java/classes

# Find all java files including the new stubs
JAVA_SOURCES=$(find frameworks/base tests/java tests/stubs -name "*.java")

# Compile with libraries in classpath
javac -d build_java/classes -cp "libs/*" $JAVA_SOURCES

# Run Java Tests
echo -e "${GREEN}[*] [Java] Running JUnit...${NC}"
if java -cp "build_java/classes:libs/*" org.junit.runner.JUnitCore com.android.server.lifecycle.DeviceLifecycleServiceTest; then
    echo -e "${GREEN}[SUCCESS] Java tests passed!${NC}"
else
    echo -e "${RED}[FAILURE] Java tests failed.${NC}"
    exit 1
fi
