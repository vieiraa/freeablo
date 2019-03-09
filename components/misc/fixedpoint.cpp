#include "fixedpoint.h"
#include "assert.h"
#include "stringops.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <sstream>

using uint128_t = boost::multiprecision::uint128_t;

constexpr int64_t FixedPoint::scalingFactorPowerOf10;
constexpr int64_t FixedPoint::scalingFactor;

FixedPoint FixedPoint::Pi("3.141592652");

static int64_t ipow(int64_t x, int64_t power)
{
    int64_t result = 1;
    for (int64_t i = 0; i < power; i++)
        result *= x;

    return result;
}

static int64_t i64abs(int64_t i)
{
    return i >= 0 ? i : -i; // not using std::abs because of a libc++ bug https://github.com/Project-OSRM/osrm-backend/issues/1000
}

FixedPoint::FixedPoint(const std::string& str)
{
    auto split = Misc::StringUtils::split(str, '.');
    release_assert(split.size() <= 2);
    release_assert(split.size() > 0);

    int64_t sign = split[0][0] == '-' ? -1 : 1;

    int64_t integer = 0;
    {
        std::stringstream ss(split[0]);
        ss >> integer;
        integer = i64abs(integer);
    }
    int64_t fractional = 0;
    int64_t fractionalDigits = 0;
    if (split.size() == 2)
    {
        fractionalDigits = int64_t(split[1].size()); // there could be leading zeros
        std::stringstream ss(split[1]);
        ss >> fractional;
    }

    integer = i64abs(integer);

    int64_t tmpScalePow10 = fractionalDigits;
    int64_t tmpScale = ipow(10, tmpScalePow10);
    int64_t tmpFixed = (integer * tmpScale) + (fractional);

    int64_t scaleDifference = FixedPoint::scalingFactorPowerOf10 - tmpScalePow10;

    if (scaleDifference > 0)
        mVal = tmpFixed * ipow(10, scaleDifference);
    else
        mVal = tmpFixed / ipow(10, -scaleDifference);

    mVal *= sign;

#ifndef NDEBUG
    {
        std::stringstream ss(str);
        ss >> mDebugVal;
    }
#endif
}

FixedPoint::FixedPoint(int64_t integerValue) { *this = fromRawValue(integerValue * FixedPoint::scalingFactor); }

FixedPoint FixedPoint::fromRawValue(int64_t rawValue)
{
    FixedPoint retval;
    retval.mVal = rawValue;
#ifndef NDEBUG
    retval.mDebugVal = retval.toDouble();
#endif
    return retval;
}

int64_t FixedPoint::intPart() const { return mVal / FixedPoint::scalingFactor; }

int64_t FixedPoint::round() const
{
    FixedPoint frac = fractionPart();
    int64_t i = intPart();
    if (frac >= FixedPoint("0.5"))
        i++;
    else if (frac <= FixedPoint("-0.5"))
        i--;
    return i;
}

FixedPoint FixedPoint::fractionPart() const
{
    int64_t sign = (mVal >= 0 ? 1 : -1);
    int64_t intPart = this->intPart();
    int64_t temp = i64abs(mVal - (intPart * FixedPoint::scalingFactor));
    int64_t rawVal = sign * temp;
    return fromRawValue(rawVal);
}

double FixedPoint::toDouble() const
{
    double val = mVal;
    double scale = FixedPoint::scalingFactor;

    return val / scale;
}

std::string FixedPoint::str() const
{
    std::stringstream ss;

    if (*this < 0)
        ss << "-";

    ss << this->abs().intPart();

    std::string fractionalTempStr;
    fractionalTempStr.resize(FixedPoint::scalingFactorPowerOf10);

    int64_t fractionalTemp = fractionPart().abs().mVal;
    for (int64_t i = 0; i < FixedPoint::scalingFactorPowerOf10; i++)
    {
        fractionalTempStr[size_t(FixedPoint::scalingFactorPowerOf10 - 1 - i)] = '0' + fractionalTemp % 10;
        fractionalTemp /= 10;
    }

    while (!fractionalTempStr.empty() && fractionalTempStr[fractionalTempStr.size() - 1] == '0')
        fractionalTempStr.pop_back();

    if (!fractionalTempStr.empty())
        ss << "." << fractionalTempStr;

    return ss.str();
}

FixedPoint FixedPoint::operator+(FixedPoint other) const
{
    FixedPoint retval = fromRawValue(mVal + other.mVal);
#ifndef NDEBUG
    retval.mDebugVal = mDebugVal + other.mDebugVal;
#endif
    return retval;
}

FixedPoint FixedPoint::operator-(FixedPoint other) const
{
    FixedPoint retval = fromRawValue(mVal - other.mVal);
#ifndef NDEBUG
    retval.mDebugVal = mDebugVal - other.mDebugVal;
#endif
    return retval;
}

FixedPoint FixedPoint::operator*(FixedPoint other) const
{
    int64_t sign = (mVal >= 0 ? 1 : -1) * (other.mVal >= 0 ? 1 : -1);

    uint128_t val1 = uint128_t(i64abs(mVal));
    uint128_t val2 = uint128_t(i64abs(other.mVal));
    uint128_t scale = FixedPoint::scalingFactor;

    uint128_t temp = (val1 * val2) / scale;

    int64_t val = sign * int64_t(temp);
    FixedPoint retval = fromRawValue(val);

#ifndef NDEBUG
    retval.mDebugVal = mDebugVal * other.mDebugVal;
#endif
    return retval;
}

FixedPoint FixedPoint::operator/(FixedPoint other) const
{
    int64_t sign = (mVal >= 0 ? 1 : -1) * (other.mVal >= 0 ? 1 : -1);

    uint128_t val1 = uint128_t(i64abs(mVal));
    uint128_t val2 = uint128_t(i64abs(other.mVal));
    uint128_t scale = FixedPoint::scalingFactor;

    uint128_t temp = (val1 * scale) / val2;

    int64_t val = sign * int64_t(temp);
    FixedPoint retval = fromRawValue(val);

#ifndef NDEBUG
    retval.mDebugVal = mDebugVal / other.mDebugVal;
#endif
    return retval;
}

FixedPoint FixedPoint::sqrt() const
{
    if (*this == 0)
        return 0;

    // basic newton raphson

    size_t iterationLimit = 10;

    FixedPoint x = *this;
    FixedPoint h;

    static FixedPoint epsilon("0.000001");

    size_t i = 0;
    do
    {
        h = ((x * x) - *this) / (FixedPoint(2) * x);
        x = x - h;
        i++;
    } while (h.abs() >= epsilon && i < iterationLimit);

#ifndef NDEBUG
    double floatSqrt = std::sqrt(mDebugVal);
    (void)floatSqrt;
#endif

    return x;
}

// xatan evaluates a series valid in the range [0, 0.66].
FixedPoint xatan(FixedPoint x)
{
    static FixedPoint P0("-0.8750608600031904122785");
    static FixedPoint P1("-16.15753718733365076637");
    static FixedPoint P2("-75.00855792314704667340");
    static FixedPoint P3("-122.8866684490136173410");
    static FixedPoint P4("-64.85021904942025371773");
    static FixedPoint Q0("24.85846490142306297962");
    static FixedPoint Q1("165.0270098316988542046");
    static FixedPoint Q2("432.8810604912902668951");
    static FixedPoint Q3("4.853903996359136964868e+02");
    static FixedPoint Q4("194.5506571482613964425");

    FixedPoint z = x * x;
    z = z * ((((P0*z+P1)*z+P2)*z+P3)*z + P4) / (((((z+Q0)*z+Q1)*z+Q2)*z+Q3)*z + Q4);
    z = x*z + x;
    return z;
}

// satan reduces its argument (known to be positive)

// to the range [0, 0.66] and calls xatan.

FixedPoint satan(FixedPoint x)
{
    // Morebits will just be zero for us, we don't have the precision for it.
    // Buuut... it probably doesn't matter? Left it in because it's in the source.
    FixedPoint Morebits(0);//("0.00000000000000006123233995736765886130"); // pi/2 = PIO2 + Morebits

    FixedPoint Tan3pio8("2.41421356237309504880"); // tan(3*pi/8)

    static FixedPoint zero66("0.66");
    if (x <= zero66)
        return xatan(x);

    if (x > Tan3pio8)
        return FixedPoint::Pi/2 - xatan(FixedPoint(1)/x) + Morebits;

    static FixedPoint zero5("0.5");
    return FixedPoint::Pi/4 + xatan((x-1)/(x+1)) + zero5*Morebits;
}

FixedPoint atan(FixedPoint x)
{
    if (x == 0)
        return x;

    if (x > 0)
        return satan(x);

    return FixedPoint(0)-satan(FixedPoint(0)-x);
}


constexpr size_t atanLutSize = 128;
FixedPoint atanLut[atanLutSize] =
{
    FixedPoint("0.0"),
    FixedPoint("0.007812341060101111"),
    FixedPoint("0.015623728620476831"),
    FixedPoint("0.023433209879467586"),
    FixedPoint("0.031239833430268277"),
    FixedPoint("0.03904264995516699"),
    FixedPoint("0.046840712915969654"),
    FixedPoint("0.05463307923935948"),
    FixedPoint("0.06241880999595735"),
    FixedPoint("0.07019697107187052"),
    FixedPoint("0.0779666338315423"),
    FixedPoint("0.08572687577074481"),
    FixedPoint("0.09347678115858947"),
    FixedPoint("0.10121544166746667"),
    FixedPoint("0.10894195698986579"),
    FixedPoint("0.11665543544106935"),
    FixedPoint("0.12435499454676144"),
    FixedPoint("0.13203976161463876"),
    FixedPoint("0.13970887428916365"),
    FixedPoint("0.14736148108865163"),
    FixedPoint("0.15499674192394097"),
    FixedPoint("0.16261382859794857"),
    FixedPoint("0.1702119252854744"),
    FixedPoint("0.17779022899267607"),
    FixedPoint("0.18534794999569476"),
    FixedPoint("0.19288431225797467"),
    FixedPoint("0.2003985538258785"),
    FixedPoint("0.207889927202263"),
    FixedPoint("0.21535769969773805"),
    FixedPoint("0.22280115375939452"),
    FixedPoint("0.23021958727684372"),
    FixedPoint("0.23761231386547124"),
    FixedPoint("0.24497866312686414"),
    FixedPoint("0.2523179808864272"),
    FixedPoint("0.2596296294082575"),
    FixedPoint("0.26691298758740045"),
    FixedPoint("0.2741674511196588"),
    FixedPoint("0.28139243264917846"),
    FixedPoint("0.2885873618940774"),
    FixedPoint("0.29575168575043154"),
    FixedPoint("0.3028848683749714"),
    FixedPoint("0.30998639124688343"),
    FixedPoint("0.31705575320914703"),
    FixedPoint("0.3240924704898717"),
    FixedPoint("0.3310960767041321"),
    FixedPoint("0.33806612283682547"),
    FixedPoint("0.34500217720710513"),
    FixedPoint("0.3519038254149648"),
    FixedPoint("0.35877067027057225"),
    FixedPoint("0.3656023317069669"),
    FixedPoint("0.3723984466767542"),
    FixedPoint("0.3791586690334418"),
    FixedPoint("0.38588266939807375"),
    FixedPoint("0.3925701350118286"),
    FixedPoint("0.39922076957525254"),
    FixedPoint("0.4058342930748041"),
    FixedPoint("0.4124104415973873"),
    FixedPoint("0.41894896713355284"),
    FixedPoint("0.42544963737004227"),
    FixedPoint("0.4319122354723482"),
    FixedPoint("0.43833655985795783"),
    FixedPoint("0.44472242396093936"),
    FixedPoint("0.4510696559885235"),
    FixedPoint("0.4573780986703208"),
    FixedPoint("0.4636476090008061"),
    FixedPoint("0.46987805797568694"),
    FixedPoint("0.4760693303227612"),
    FixedPoint("0.48222132422785374"),
    FixedPoint("0.48833395105640554"),
    FixedPoint("0.49440713507127537"),
    FixedPoint("0.5004408131472942"),
    FixedPoint("0.5064349344830967"),
    FixedPoint("0.5123894603107377"),
    FixedPoint("0.518304363603578"),
    FixedPoint("0.5241796287829132"),
    FixedPoint("0.5300152514237931"),
    FixedPoint("0.5358112379604637"),
    FixedPoint("0.541567605391845"),
    FixedPoint("0.5472843809874369"),
    FixedPoint("0.5529616019940283"),
    FixedPoint("0.5585993153435624"),
    FixedPoint("0.5641975773624976"),
    FixedPoint("0.5697564534829784"),
    FixedPoint("0.5752760179561178"),
    FixedPoint("0.5807563535676704"),
    FixedPoint("0.5861975513563606"),
    FixedPoint("0.5915997103351114"),
    FixedPoint("0.5969629372154015"),
    FixedPoint("0.6022873461349642"),
    FixedPoint("0.6075730583890224"),
    FixedPoint("0.6128202021652414"),
    FixedPoint("0.6180289122825618"),
    FixedPoint("0.6231993299340659"),
    FixedPoint("0.6283316024340097"),
    FixedPoint("0.6334258829691446"),
    FixedPoint("0.6384823303544376"),
    FixedPoint("0.6435011087932844"),
    FixedPoint("0.6484823876423006"),
    FixedPoint("0.6534263411807619"),
    FixedPoint("0.658333148384756"),
    FixedPoint("0.6632029927060933"),
    FixedPoint("0.6680360618560202"),
    FixedPoint("0.6728325475937632"),
    FixedPoint("0.6775926455199252"),
    FixedPoint("0.6823165548747481"),
    FixedPoint("0.687004478341245"),
    FixedPoint("0.6916566218531999"),
    FixedPoint("0.6962731944080236"),
    FixedPoint("0.7008544078844502"),
    FixedPoint("0.705400476865049"),
    FixedPoint("0.7099116184635249"),
    FixedPoint("0.714388052156769"),
    FixedPoint("0.7188299996216245"),
    FixedPoint("0.7232376845763179"),
    FixedPoint("0.7276113326265107"),
    FixedPoint("0.7319511711159166"),
    FixedPoint("0.7362574289814281"),
    FixedPoint("0.7405303366126927"),
    FixedPoint("0.7447701257160751"),
    FixedPoint("0.7489770291829414"),
    FixedPoint("0.7531512809621944"),
    FixedPoint("0.7572931159369924"),
    FixedPoint("0.7614027698055784"),
    FixedPoint("0.7654804789661445"),
    FixedPoint("0.7695264804056583"),
    FixedPoint("0.7735410115925735"),
    FixedPoint("0.7775243103733478"),
    FixedPoint("0.7814766148726883")
};

FixedPoint atanLutLookup(FixedPoint y, FixedPoint x)
{
    FixedPoint ratio = y / x;
    FixedPoint lookup = ratio * atanLutSize;

    int64_t index1 = lookup.intPart();
    FixedPoint indexRatio = lookup.fractionPart();

    if (indexRatio == 0)
        return atanLut[index1];

    int64_t index2 = index1 + 1;
    return atanLut[index1] * indexRatio + atanLut[index2] * (FixedPoint(1) - indexRatio);
}


// ***********************************************************
// Temporary hack uses floating point.
// TODO: Convert to fixed point implementation.
#define _USE_MATH_DEFINES
#include <math.h>
FixedPoint FixedPoint::atan2_degrees(FixedPoint y, FixedPoint x)
{
//    return atan(y/x);

    // TODO: Possible domain errors (0, 0)?
//    double res = atan2(y.toDouble(), x.toDouble()) * 180 / M_PI;
//    return FixedPoint::fromRawValue((uint64_t)(res * FixedPoint::scalingFactor));

//    static FixedPoint coeff_1 = Pi/4;
//    static FixedPoint coeff_2 = coeff_1;

//    static FixedPoint smallValue("0.000000001");
//    debug_assert(smallValue != 0);

//    FixedPoint abs_y = y.abs() + smallValue; // kludge to prevent 0/0 condition


//    FixedPoint angle;
//    if (x >= 0)
//    {
//       FixedPoint r = (x - abs_y) / (x + abs_y);
//       angle = coeff_1 - coeff_1 * r;
//    }
//    else
//    {
//       FixedPoint r = (x + abs_y) / (abs_y - x);
//       angle = coeff_2 - coeff_1 * r;
//    }


////    if (y < 0)
////        return(FixedPoint(0) - angle); // negate if in quad III or IV
////    else
//        return(angle);
}

FixedPoint FixedPoint::sin_degrees(FixedPoint deg)
{

    double res = sin(deg.toDouble() * M_PI / 180);
    return FixedPoint::fromRawValue((uint64_t)(res * FixedPoint::scalingFactor));
}

FixedPoint FixedPoint::cos_degrees(FixedPoint deg) { return sin_degrees(deg + 90); }
// ***********************************************************
