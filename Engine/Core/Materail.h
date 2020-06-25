/*! \file Core\Material.h
\ingroup Engine
\brief 提供渲染所需的Effect,Pass使用。
*/
#ifndef LE_Core_Material_H
#define LE_Core_Material_H 1

#include <LBase/any.h>

#include "LSLEvaluator.h"
#include "Resource.h"
#include "../Asset/MaterialAsset.h"
#include "../Render/Effect/Effect.hpp"

#include <unordered_set>

namespace platform {
	class MaterialEvaluator;

	class Renderable;

	class Material :Resource {
	public:
		Material(const asset::MaterailAsset& asset, const std::string& name);

		struct RenderDelayedContext {
			Renderable* pRenderable;
		};
	public:
		const std::string& GetName() const lnothrow override {
			return identity_name;
		}

		std::shared_ptr<Render::Effect::Effect> GetEffect() const lnothrow {
			return bind_effects;
		}

	public:
		friend class Renderable;

		//after effect set/before render
		void UpdateParams(const Renderable* pRenderable) const;
	private:
		std::vector<std::pair<size_t, std::any>> bind_values;
		std::vector<std::pair<size_t, scheme::TermNode>> delay_values;
		std::shared_ptr<Render::Effect::Effect> bind_effects;
		std::string identity_name;
	public:
		static MaterialEvaluator& GetInstanceEvaluator();
	};

	class MaterialEvaluator :public LSLEvaluator {
	public:
		MaterialEvaluator();

		std::pair<scheme::TermNode, scheme::ReductionStatus> Reduce(const scheme::TermNode& input) {
			auto term(input);
			context.Prepare(term);
			auto status(scheme::v1::Reduce(term, context.Root));

			return std::make_pair(term, status);
		}

		using path = std::filesystem::path;
		void LoadFile(const path& filepath);

		void Define(string_view id, scheme::ValueObject&& vo,bool forced);

		struct InstanceTag :scheme::LSLATag
		{};

		struct RenderTag : scheme::LSLATag
		{};

		using InstanceDelayedTerm = leo::derived_entity<scheme::TermNode, InstanceTag>;
		using RenderDelayedTerm = leo::derived_entity<scheme::TermNode, RenderTag>;

		friend class Material;

		static void CheckReductionStatus(scheme::ReductionStatus status);
	private:
		static void MaterialEvalFunctions(REPLContext& context);

		void RegisterMathDotLssFile();

		std::unordered_set<std::size_t> loaded_fileshash_set;
	};
}

#endif