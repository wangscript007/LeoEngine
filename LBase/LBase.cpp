#include <type_traits>
#include <functional>

using std::invoke_result;

#include "LBase/pseudo_mutex.h"
#include "LBase/scope_gurad.hpp"
#include "LBase/cache.hpp"
#include "LBase/concurrency.h"
#include "LBase/examiner.hpp"
#include "LBase/cast.hpp"
#include "LBase/set.hpp"
#include "LBase/lmath.hpp"

namespace lm = leo::math;

static decltype(auto) foo() {

	lm::float2 smq{ 1,0 };
	lm::float3 gugugu{ 1,2,3 };

	gugugu.zx = smq.yx;

	auto mjs = lm::make_qtangent<float>(gugugu, -1);

	return 0;
}

