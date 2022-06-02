#include "TkMath.h"

namespace tk
{
	const Real Math::pos_infinity = std::numeric_limits<Real>::infinity();
	const Real Math::neg_infinity = -std::numeric_limits<Real>::infinity();
	const Real Math::pi = Real(4.0 * atan(1.0));	//3.1415926535897932384626433832795028841971693993751
	const Real Math::inv_pi = Real(1.0) / pi;
	const Real Math::two_pi = Real(2.0 * pi);
	const Real Math::half_pi = Real(0.5 * pi);
	const Real Math::fDeg2Rad = pi / Real(180.0);
	const Real Math::fRad2Deg = Real(180.0) / pi;
	const Real Math::inv_log2 = Real(1.0) / log(Real(2.0));
	const Real Math::machine_epsilon = std::numeric_limits<Real>::epsilon() * 0.5;
	const Real Math::one_minus_epsilon = 0x1.fffffep-1; //double 0x1.fffffffffffffp-1

}