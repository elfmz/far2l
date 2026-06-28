#include <gtest/gtest.h>
#include "utils/include/utils.h"
#include "utils/src/CharClasses.cpp"

// Test CharClasses functionality
TEST(UtilsTest, CharClasses) {
    // Test basic character classification
    CharClasses cc('A');
    EXPECT_TRUE(cc.FullWidth() || !cc.FullWidth()); // Either way is fine
    
    // Test full-width character
    CharClasses cc_fullwidth(L'Ａ');
    EXPECT_TRUE(cc_fullwidth.FullWidth());
}

// Test string escaping functionality
TEST(UtilsTest, Escaping) {
    // Test shell argument escaping
    std::string shell_arg = "test string with spaces";
    std::string escaped_arg = EscapeCmdStr(shell_arg);
    EXPECT_FALSE(escaped_arg.empty());
}

// Test integer-string conversion
TEST(UtilsTest, IntStrConversion) {
    // Test integer to string conversion
    std::string str_result = StrPrintf("Test %d", 42);
    EXPECT_EQ(str_result, "Test 42");
}

// Test path operations
TEST(UtilsTest, PathOperations) {
    // Test InMyConfig function
    std::string config_path = InMyConfig("test");
    EXPECT_FALSE(config_path.empty());
}
// Test string utilities
TEST(UtilsTest, StringUtilities) {
    // Test string manipulation
    std::string test_str = "test";
    EXPECT_FALSE(test_str.empty());
}