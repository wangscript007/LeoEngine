#include "LSLEvaluator.h"
#include "LSLBuilder.h"

namespace platform {
	using namespace scheme;
	using namespace v1;


	void
		LoadSequenceSeparators(EvaluationPasses& passes)
	{
		RegisterSequenceContextTransformer(passes, TokenValue(";"), true),
			RegisterSequenceContextTransformer(passes, TokenValue(","));
	}

	LSLEvaluator::LSLEvaluator(std::function<void(REPLContext&)> loader)
	:
#ifdef NDEBUG
		context()
#else
		context(true)
#endif
	{
		auto& root(context.Root);

		//LoadSequenceSeparators(context.ListTermPreprocess),

		root.EvaluateLiteral = lsl::context::FetchNumberLiteral();

		lsl::math::RegisterTypeLiteralAction(context);

		loader(context);
	}

	ImplDeDtor(LSLEvaluator)
}