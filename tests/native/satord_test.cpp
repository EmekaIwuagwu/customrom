#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

// Forward declare verification logic from satord.cpp
// In a real build, we'd separate the logic into a static library.
// For this single-file PoC, we paste the logic or include headers.
// We will replicate the logic function for testing purposes here to ensure correctness.

bool validate_path_prefix(const std::string& path, const std::vector<std::string>& allowed) {
    // Mock realpath for testing without filesystem? 
    // Or just test actual FS behavior. 
    // We'll test actual FS logic but creating temp files.
    
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved) == NULL) return false;
    std::string safe(resolved);
    for (const auto& prefix : allowed) {
        if (safe.find(prefix) == 0) return true;
    }
    return false;
}

class SatordUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create known temp layout
        system("mkdir -p /tmp/test_area/data/data/com.pkg");
        system("mkdir -p /tmp/test_area/sdcard");
        system("touch /tmp/test_area/data/data/com.pkg/file");
        system("ln -s /etc/passwd /tmp/test_area/data/data/com.pkg/symlink_attack");
    }

    void TearDown() override {
        system("rm -rf /tmp/test_area");
    }
};

TEST_F(SatordUtilsTest, PathValidation_Success) {
    std::string path = "/tmp/test_area/data/data/com.pkg";
    std::vector<std::string> allowed = {"/tmp/test_area/data/data/"};
    EXPECT_TRUE(validate_path_prefix(path, allowed));
}

TEST_F(SatordUtilsTest, PathValidation_TraversalSymlink_Fail) {
    // If I ask for the symlink, realpath() resolves it to /etc/passwd
    std::string path = "/tmp/test_area/data/data/com.pkg/symlink_attack";
    std::vector<std::string> allowed = {"/tmp/test_area/data/data/"};
    
    // Should fail because realpath resolves to /etc/passwd which doesn't start with allowed prefix
    // Note: depends on host /etc/passwd existing for realpath to work
    EXPECT_FALSE(validate_path_prefix(path, allowed)); 
}

TEST_F(SatordUtilsTest, PathValidation_OutsidePrefix_Fail) {
    std::string path = "/tmp/test_area/sdcard";
    std::vector<std::string> allowed = {"/tmp/test_area/data/data/"};
    EXPECT_FALSE(validate_path_prefix(path, allowed));
}

TEST_F(SatordUtilsTest, PathValidation_PartialMatch_Fail) {
    // Verify directory boundaries
    std::string path = "/tmp/test_area/data/data_fake";
    system("mkdir -p /tmp/test_area/data/data_fake");
    std::vector<std::string> allowed = {"/tmp/test_area/data/data/"};
    // safe path: .../data/data_fake
    // prefix:    .../data/data/
    // mismatch
    EXPECT_FALSE(validate_path_prefix(path, allowed));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
