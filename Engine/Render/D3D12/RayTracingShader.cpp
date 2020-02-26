#include "RayTracingShader.h"
#include "Context.h"

using namespace platform_ex::Windows::D3D12;
using namespace platform::Render::Shader;

void QuantizeBoundShaderState(
	ShaderType Type,
	const D3D12_RESOURCE_BINDING_TIER& ResourceBindingTier,
	const RayTracingShader* const RayTracingShader,
	QuantizedBoundShaderState& OutQBSS
)
{
	lconstraint(RayTracingShader);

	const auto& Counts = RayTracingShader->ResourceCounts;

	std::memset(&OutQBSS,0, sizeof(OutQBSS));
	auto& QBSSRegisterCounts = OutQBSS.RegisterCounts[platform::Render::VisibilityAll];

	switch (Type)
	{
	case RayGen:
	case RayMiss:

		// Shared conservative root signature layout is used for all raygen and miss shaders.

		OutQBSS.RootSignatureType = RootSignatureType::RayTracingGlobal;

		QBSSRegisterCounts.NumSamplers = MAX_SAMPLERS;
		QBSSRegisterCounts.NumSRVs = MAX_SRVS;
		QBSSRegisterCounts.NumCBs = MAX_CBS;
		QBSSRegisterCounts.NumUAVs = MAX_UAVS;

		break;

	case RayHitGroup:
	case RayCallable:

		// Local root signature is used for hit group shaders, using the exact number of resources to minimize shader binding table record size.

		OutQBSS.RootSignatureType = RootSignatureType::RayTracingLocal;

		QBSSRegisterCounts.NumSamplers = Counts.NumSamplers;
		QBSSRegisterCounts.NumSRVs = Counts.NumSRVs;
		QBSSRegisterCounts.NumCBs = Counts.NumCBs;
		QBSSRegisterCounts.NumUAVs = Counts.NumUAVs;

		lconstraint(QBSSRegisterCounts.NumSamplers <= MAX_SAMPLERS);
		lconstraint(QBSSRegisterCounts.NumSRVs <= MAX_SRVS);
		lconstraint(QBSSRegisterCounts.NumCBs <= MAX_CBS);
		lconstraint(QBSSRegisterCounts.NumUAVs <= MAX_UAVS);

		break;
	default:
		lassume(false); // Unexpected shader target frequency
	}
}

RayTracingShader::RayTracingShader(const platform::Render::RayTracingShaderInitializer& initializer)
{
	auto& blob = initializer.pBlob->GetBlob();
	ShaderByteCode.first = std::make_unique<byte[]>(blob.second);
	ShaderByteCode.second = blob.second;
	std::memcpy(ShaderByteCode.first.get(), blob.first.get(), blob.second);

	ResourceCounts = initializer.pBlob->GetInfo().ResourceCounts;

	auto& Device = Context::Instance().GetDevice();

	const D3D12_RESOURCE_BINDING_TIER Tier = Device.GetResourceBindingTier();
	QuantizedBoundShaderState QBSS;
	QuantizeBoundShaderState(initializer.pBlob->GetShaderType(), Tier, this, QBSS);

	pRootSignature = Device.CreateRootSignature(QBSS);

	EntryPoint = initializer.EntryPoint;
	AnyHitEntryPoint = initializer.AnyHitEntryPoint;
	IntersectionEntryPoint = initializer.IntersectionEntryPoint;
}