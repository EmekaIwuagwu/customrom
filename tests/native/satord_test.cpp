#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "../../vendor/google/services/satord/satord_utils.h"

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

TEST_F(SatordUtilsTest, PackageNameValidation) {
    EXPECT_TRUE(validate_package_name("com.example.app"));
    EXPECT_TRUE(validate_package_name("com.example_app.v1"));
    EXPECT_TRUE(validate_package_name("app123"));
    
    EXPECT_FALSE(validate_package_name(""));
    EXPECT_FALSE(validate_package_name("com.example/app")); // No path separators
    EXPECT_FALSE(validate_package_name("com.example;rm")); // No shell metachars
    EXPECT_FALSE(validate_package_name(".."));
}

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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
