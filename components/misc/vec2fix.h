#include "direction.h"
#include "fixedpoint.h"

class Vec2Fix
{
public:
    Vec2Fix() = default;
    Vec2Fix(FixedPoint x, FixedPoint y) : x(x), y(y) {}

    FixedPoint magnitude() const;
    void normalise();

    /// NOTE: this function returns a direction in isometric space,
    /// where positive x extends south east from the origin, and positive y south west
    Misc::Direction getIsometricDirection() const;

    /// NOTE:
    /// Differs from cmath atan2 in a few ways:
    ///   - y axis is positive-down, like graphics, not like maths.
    ///   - result is degrees, not radians
    ///   - will always return positive angles (ie, will return 355, not -5).
    static FixedPoint atan2Degrees(Vec2Fix v);

    Vec2Fix operator-(const Vec2Fix& other) const;

    FixedPoint x;
    FixedPoint y;
};
