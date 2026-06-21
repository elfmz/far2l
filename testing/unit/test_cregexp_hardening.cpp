#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <string>
#include "colorer/src/Colorer-library/src/colorer/cregexp/cregexp.h"

// ============================================================================
// Benchmark-style tests: measure timing of slow CRegExp operations.
// These print timing info to stderr so perf regressions are visible in CI logs.
// ============================================================================

TEST(CRegExpBenchmarks, BacktrackHeavyTiming)
{
	// Backtrack-heavy pattern stresses insert_stack / lowParse without
	// deep compile-time nesting that cregexp rejects.
	UnicodeString pattern("/(a?){30}/");
	CRegExp re(&pattern);
	ASSERT_TRUE(re.isOk());

	UnicodeString input("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");  // 30 a's
	SMatches mtch{};

	auto t0 = std::chrono::high_resolution_clock::now();
	bool result = re.parse(&input, &mtch);
	auto t1 = std::chrono::high_resolution_clock::now();

	auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
	std::cerr << "[BENCH] Backtrack-heavy (a?){30} on 30 a's: " << us << " us, result=" << result << "\n";

	EXPECT_TRUE(result);
	EXPECT_LT(us, 500000);
}

TEST(CRegExpBenchmarks, RepeatedParseThroughput)
{
	UnicodeString pattern("/\\w+\\s*=/");
	CRegExp re(&pattern);
	ASSERT_TRUE(re.isOk());

	const int kIterations = 10000;
	UnicodeString input("foo = bar");

	auto t0 = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < kIterations; ++i) {
		SMatches mtch{};
		(void)re.parse(&input, &mtch);
	}
	auto t1 = std::chrono::high_resolution_clock::now();

	auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
	double per_us = static_cast<double>(total_us) / kIterations;
	std::cerr << "[BENCH] Repeated parse throughput (" << kIterations << " iters): "
	          << total_us << " us total, " << per_us << " us/parse\n";

	EXPECT_LT(per_us, 1000.0);  // 1 ms per parse is very generous
}

TEST(CRegExpBenchmarks, ComplexColorerPatternTiming)
{
	// Realistic colorer-style pattern: keyword + punctuation + value
	UnicodeString pattern("/\\b(int|void|char)\\s+\\w+\\s*=/");
	CRegExp re(&pattern);
	ASSERT_TRUE(re.isOk());

	const int kIterations = 5000;
	UnicodeString input("int variable_name = 42;");

	auto t0 = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < kIterations; ++i) {
		SMatches mtch{};
		(void)re.parse(&input, &mtch);
	}
	auto t1 = std::chrono::high_resolution_clock::now();

	auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
	double per_us = static_cast<double>(total_us) / kIterations;
	std::cerr << "[BENCH] Complex colorer pattern (" << kIterations << " iters): "
	          << total_us << " us total, " << per_us << " us/parse\n";

	EXPECT_LT(per_us, 2000.0);
}

TEST(CRegExpBenchmarks, AlternationHeavyTiming)
{
	// Many alternations stress lowParse / parseRE
	std::string ps = "/(";
	for (int i = 0; i < 20; ++i) {
		if (i > 0) ps += "|";
		ps += "word" + std::to_string(i);
	}
	ps += ")/";

	UnicodeString pattern(ps.c_str());
	CRegExp re(&pattern);
	ASSERT_TRUE(re.isOk());

	UnicodeString input("word19");
	SMatches mtch{};

	auto t0 = std::chrono::high_resolution_clock::now();
	bool result = re.parse(&input, &mtch);
	auto t1 = std::chrono::high_resolution_clock::now();

	auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
	std::cerr << "[BENCH] Alternation-heavy (20 branches): " << us << " us, result=" << result << "\n";

	EXPECT_TRUE(result);
	EXPECT_LT(us, 500000);
}

TEST(CRegExpBenchmarks, WordBoundaryHeavyTiming)
{
	UnicodeString pattern("/\\b\\w+\\b/");
	CRegExp re(&pattern);
	ASSERT_TRUE(re.isOk());

	UnicodeString input("the quick brown fox jumps over the lazy dog");
	const int kIterations = 5000;

	auto t0 = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < kIterations; ++i) {
		SMatches mtch{};
		(void)re.parse(&input, &mtch);
	}
	auto t1 = std::chrono::high_resolution_clock::now();

	auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
	double per_us = static_cast<double>(total_us) / kIterations;
	std::cerr << "[BENCH] Word-boundary heavy (" << kIterations << " iters): "
	          << total_us << " us total, " << per_us << " us/parse\n";

	EXPECT_LT(per_us, 1000.0);
}

TEST(CRegExpBenchmarks, NestedGroupsTiming)
{
	// Deeply nested groups stress insert_stack and lowParse recursion.
	std::string ps = "/";
	for (int i = 0; i < 15; ++i) ps += "(a";
	for (int i = 0; i < 15; ++i) ps += ")";
	ps += "/";

	UnicodeString pattern(ps.c_str());
	CRegExp re(&pattern);
	ASSERT_TRUE(re.isOk());

	UnicodeString input("aaaaaaaaaaaaaaa");
	SMatches mtch{};

	auto t0 = std::chrono::high_resolution_clock::now();
	bool result = re.parse(&input, &mtch);
	auto t1 = std::chrono::high_resolution_clock::now();

	auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
	std::cerr << "[BENCH] Nested groups (15 levels): " << us << " us, result=" << result << "\n";

	EXPECT_TRUE(result);
	EXPECT_LT(us, 500000);
}

TEST(CRegExpBenchmarks, QuantifierRangeTiming)
{
	UnicodeString pattern("/\\w{5,20}/");
	CRegExp re(&pattern);
	ASSERT_TRUE(re.isOk());

	UnicodeString input("abcdefghijklmnop");
	const int kIterations = 5000;

	auto t0 = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < kIterations; ++i) {
		SMatches mtch{};
		(void)re.parse(&input, &mtch);
	}
	auto t1 = std::chrono::high_resolution_clock::now();

	auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
	double per_us = static_cast<double>(total_us) / kIterations;
	std::cerr << "[BENCH] Quantifier range {5,20} (" << kIterations << " iters): "
	          << total_us << " us total, " << per_us << " us/parse\n";

	EXPECT_LT(per_us, 1000.0);
}
