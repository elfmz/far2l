#include <gtest/gtest.h>
#include "colorer/src/Colorer-library/src/colorer/editor/PerformanceEstimation.h"

using namespace colorer;

TEST(ColorerPerformanceEstimation, IdleParseLines)
{
  EXPECT_EQ(idleParseLines(0), 100);
  EXPECT_EQ(idleParseLines(1), 104);
  EXPECT_EQ(idleParseLines(10), 140);
  EXPECT_EQ(idleParseLines(50), 300);
  EXPECT_EQ(idleParseLines(100), 500);
}

TEST(ColorerPerformanceEstimation, ClampIdleTime)
{
  EXPECT_EQ(clampIdleTime(-10), kIdleTimeMin);
  EXPECT_EQ(clampIdleTime(-1), kIdleTimeMin);
  EXPECT_EQ(clampIdleTime(0), 0);
  EXPECT_EQ(clampIdleTime(50), 50);
  EXPECT_EQ(clampIdleTime(100), 100);
  EXPECT_EQ(clampIdleTime(101), kIdleTimeMax);
  EXPECT_EQ(clampIdleTime(200), kIdleTimeMax);
}

TEST(ColorerPerformanceEstimation, ShouldSkipSyntax)
{
  EXPECT_FALSE(shouldSkipSyntax(0, 0));
  EXPECT_FALSE(shouldSkipSyntax(1000, 0));
  EXPECT_TRUE(shouldSkipSyntax(1001, 0));
  EXPECT_FALSE(shouldSkipSyntax(500, 0));
  EXPECT_TRUE(shouldSkipSyntax(1500, 499));
  EXPECT_FALSE(shouldSkipSyntax(1500, 500));
}

TEST(ColorerPerformanceEstimation, IdleTimeBudget)
{
  EXPECT_EQ(idleTimeBudget(0), 0);
  EXPECT_EQ(idleTimeBudget(1), 10);
  EXPECT_EQ(idleTimeBudget(5), 50);
  EXPECT_EQ(idleTimeBudget(10), 100);
}

TEST(ColorerPerformanceEstimation, CrossedMilestone)
{
  EXPECT_FALSE(crossedMilestone(0, 0));
  EXPECT_FALSE(crossedMilestone(100, 200));
  EXPECT_TRUE(crossedMilestone(24999, 25000));
  EXPECT_TRUE(crossedMilestone(24999, 25001));
  EXPECT_TRUE(crossedMilestone(0, 25000));
  EXPECT_TRUE(crossedMilestone(25000, 50000));
  EXPECT_FALSE(crossedMilestone(50000, 49999));
  EXPECT_FALSE(crossedMilestone(50000, 50000));
  EXPECT_FALSE(crossedMilestone(75000, 74999));
}
