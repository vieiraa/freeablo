#include <gtest/gtest.h>
#include <misc/fixedpoint.h>
#include <misc/vec2fix.h>

#define ASSERT_ALMOST_EQ(expected, actual, delta)                                                                                                              \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto assert_expected = (expected);                                                                                                                     \
        auto assert_actual = (actual);                                                                                                                         \
        auto assert_delta = (delta);                                                                                                                           \
        ASSERT_GT(assert_expected, assert_actual - assert_delta);                                                                                              \
        ASSERT_LT(assert_expected, assert_actual + assert_delta);                                                                                              \
    } while (false)

TEST(FixedPoint, TestConstruction)
{
    FixedPoint val;

    val = FixedPoint("1.23");
    ASSERT_EQ(val.intPart(), 1);
    ASSERT_EQ(val.fractionPart(), FixedPoint("0.23"));
    ASSERT_EQ(val.str(), "1.23");

    val = FixedPoint("1.023");
    ASSERT_EQ(val.intPart(), 1);
    ASSERT_EQ(val.fractionPart(), FixedPoint("0.023"));
    ASSERT_EQ(val.str(), "1.023");

    val = FixedPoint("981.00006");
    ASSERT_EQ(val.intPart(), 981);
    ASSERT_EQ(val.fractionPart(), FixedPoint("0.00006"));
    ASSERT_EQ(val.str(), "981.00006");

    val = FixedPoint("-1.023");
    ASSERT_EQ(val.intPart(), -1);
    ASSERT_EQ(val.fractionPart(), FixedPoint("-0.023"));
    ASSERT_EQ(val.str(), "-1.023");
}

TEST(FixedPoint, AddSubtract)
{
    FixedPoint val;

    val = FixedPoint("0.789") + FixedPoint("12.345");
    ASSERT_EQ(val.str(), "13.134");

    val = FixedPoint("0.5") + FixedPoint("1.6");
    ASSERT_EQ(val.str(), "2.1");

    val = FixedPoint("9.763345") - FixedPoint("72.00084542");
    ASSERT_EQ(val.str(), "-62.23750042");
}

TEST(FixedPoint, MultiplyDivide)
{
    FixedPoint val;

    val = FixedPoint("7.25") * FixedPoint("2.0");
    ASSERT_EQ(val.str(), "14.5");

    val = FixedPoint("1.0") / FixedPoint("100");
    ASSERT_EQ(val.str(), "0.01");

    val = FixedPoint("7") / FixedPoint("2");
    ASSERT_EQ(val.str(), "3.5");

    val = FixedPoint("1.0") / FixedPoint("0.5");
    ASSERT_EQ(val.str(), "2");
}

TEST(FixedPoint, sqrt)
{
    for (auto x : {"16", "200", "7.6345", "9.122938", "0.000123123"})
    {
        double doubleVal = 0;
        {
            std::stringstream ss(x);
            ss >> doubleVal;
        }

        auto fixedSquare = FixedPoint(x).sqrt();
        double floatSquare = std::sqrt(doubleVal);

        ASSERT_ALMOST_EQ(fixedSquare.toDouble(), floatSquare, 0.000001);
    }
}

TEST(FixedPoint, PlainIntOperators)
{
    FixedPoint val;

    val = FixedPoint("1.245") + int32_t(10);
    ASSERT_EQ(val, FixedPoint("11.245"));

    val = FixedPoint("1.0") / uint8_t(2);
    ASSERT_EQ(val, FixedPoint("0.5"));
}

TEST(FixedPoint, Rounding)
{
    ASSERT_EQ(1, FixedPoint("1.49").round());
    ASSERT_EQ(2, FixedPoint("1.50").round());
    ASSERT_EQ(-1, FixedPoint("-1.49").round());
    ASSERT_EQ(-2, FixedPoint("-1.50").round());
}

TEST(FixedPoint, VectorToDirection)
{
    ASSERT_EQ(Vec2Fix(1, 0).getIsometricDirection(), Misc::Direction::south_east);
    ASSERT_EQ(Vec2Fix(-1, 0).getIsometricDirection(), Misc::Direction::north_west);
    ASSERT_EQ(Vec2Fix(0, 1).getIsometricDirection(), Misc::Direction::south_west);
    ASSERT_EQ(Vec2Fix(0, -1).getIsometricDirection(), Misc::Direction::north_east);

    ASSERT_EQ(Vec2Fix(1, 1).getIsometricDirection(), Misc::Direction::south);
    ASSERT_EQ(Vec2Fix(1, -1).getIsometricDirection(), Misc::Direction::east);
    ASSERT_EQ(Vec2Fix(-1, -1).getIsometricDirection(), Misc::Direction::north);
    ASSERT_EQ(Vec2Fix(-1, 1).getIsometricDirection(), Misc::Direction::west);
}

TEST(FixedPoint, Atan2)
{
    for (float degrees = 0; degrees <= 360; degrees+= 0.5)
    {
        constexpr float degToRad = 0.01745329;
        constexpr float radToDeg = 57.29578;

        float radians = degrees * degToRad;

        float vectorX = cos(radians);
        float vectorY = sin(radians);

        float calculatedFloatRadians = atan2(vectorY, vectorX);
        float calculatedFloatDegrees = calculatedFloatRadians * radToDeg;
        if (calculatedFloatDegrees < 0)
            calculatedFloatDegrees = 360 + calculatedFloatDegrees;

        FixedPoint fixedPointX(std::to_string(vectorX));
        FixedPoint fixedPointY(std::to_string(-vectorY));

        FixedPoint calculatedFixedPointDegrees = Vec2Fix::atan2Degrees(Vec2Fix(fixedPointX, fixedPointY));

        float difference = abs(calculatedFixedPointDegrees.toDouble() - calculatedFloatDegrees);

        bool aBig = calculatedFixedPointDegrees > 270;
        bool aSmall = calculatedFixedPointDegrees < 90;
        bool bBig = calculatedFloatDegrees > 270;
        bool bSmall = calculatedFloatDegrees < 90;

        // account for errors like 359.8 vs 0.1
        if ((aBig && bSmall) || (aSmall && bBig))
            difference = 360 - difference;

        // std::cout << degrees << "," << calculatedFixedPointDegrees.str() << "," << calculatedFixedPointDegrees.mDebugVal << "," << calculatedFloatDegrees << " " << vectorX << " " << vectorY <<std::endl;
        ASSERT_LT(difference, 1);
        ASSERT_NEAR(calculatedFloatDegrees, degrees, 1);
    }
}
