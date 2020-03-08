#include "D3D12RayTracing.h"
#include "Context.h"
#include "RayTracingPipelineState.h"
#include "../../Core/Hash/CityHash.h"

void RayTracingShaderTable::UploadToGPU(Windows::D3D12::Device* Device)
{

	if (!bIsDirty)
		return;

	Buffer = leo::share_raw(Device->CreateVertexBuffer(
		platform::Render::Buffer::Static,
		0,
		static_cast<uint32>(Data.size()),
		platform::Render::EF_Unknown,
		Data.data()
	));


	bIsDirty = false;
}

COMPtr<ID3D12StateObject> CreateRayTracingStateObject(ID3D12Device5* RayTracingDevice, const leo::span<const DXILLibrary*>& ShaderLibraries, const leo::span<LPCWSTR>& Exports, uint32 MaxPayloadSizeInBytes, const leo::span<const D3D12_HIT_GROUP_DESC>& HitGroups, const ID3D12RootSignature* GlobalRootSignature, const leo::span<ID3D12RootSignature*>& LocalRootSignatures, const leo::span<uint32>& LocalRootSignatureAssociations, const leo::span<D3D12_EXISTING_COLLECTION_DESC>& ExistingCollections, D3D12_STATE_OBJECT_TYPE StateObjectType)
{
	// There are several pipeline sub-objects that are always required:
	// 1) D3D12_RAYTRACING_SHADER_CONFIG
	// 2) D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION
	// 3) D3D12_RAYTRACING_PIPELINE_CONFIG
	// 4) Global root signature
	static constexpr uint32 NumRequiredSubobjects = 4;

	leo::vector< D3D12_STATE_SUBOBJECT> Subobjects;
	Subobjects.reserve(NumRequiredSubobjects
		+ ShaderLibraries.size()
		+ HitGroups.size()
		+ LocalRootSignatures.size()
		+ Exports.size()
		+ ExistingCollections.size()
	);

	leo::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> ExportAssociations;
	ExportAssociations.reserve(Exports.size());
	auto NumExports = Exports.size();

	//Shader libraries
	for (auto& Library : ShaderLibraries)
	{
		Subobjects.emplace_back(Library->GetSubobject());
	}

	// Shader config

	D3D12_RAYTRACING_SHADER_CONFIG ShaderConfig = {};
	ShaderConfig.MaxAttributeSizeInBytes = 8; // sizeof 2 floats (barycentrics)
	ShaderConfig.MaxPayloadSizeInBytes = MaxPayloadSizeInBytes;
	const uint32 ShaderConfigIndex = static_cast<uint32>(Subobjects.size());
	Subobjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &ShaderConfig });

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation = {};
	ShaderConfigAssociation.NumExports = static_cast<UINT>(Exports.size());
	ShaderConfigAssociation.pExports = Exports.data();
	ShaderConfigAssociation.pSubobjectToAssociate = &Subobjects[ShaderConfigIndex];
	Subobjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &ShaderConfigAssociation });

	for (auto& HitGroupDesc : HitGroups)
	{
		Subobjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &HitGroupDesc });
	}

	// Pipeline config

	D3D12_RAYTRACING_PIPELINE_CONFIG PipelineConfig = {};
	PipelineConfig.MaxTraceRecursionDepth = 1; // Only allow ray tracing from RayGen shader
	const uint32 PipelineConfigIndex = static_cast<uint32>(Subobjects.size());
	Subobjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &PipelineConfig });

	// Global root signature

	Subobjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &GlobalRootSignature });

	// Local root signatures

	const uint32 LocalRootSignatureBaseIndex = static_cast<uint32>(Subobjects.size());
	for (int32 SignatureIndex = 0; SignatureIndex < LocalRootSignatures.size(); ++SignatureIndex)
	{
		Subobjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, &LocalRootSignatures[SignatureIndex] });
	}

	// Local root signature associations

	for (int32 ExportIndex = 0; ExportIndex < Exports.size(); ++ExportIndex)
	{
		// If custom LocalRootSignatureAssociations data is not provided, then assume same default local RS association.
		const int32 LocalRootSignatureIndex = LocalRootSignatureAssociations.size() != 0
			? LocalRootSignatureAssociations[ExportIndex]
			: 0;

		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& Association = ExportAssociations[ExportIndex];
		Association = D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION{};
		Association.NumExports = 1;
		Association.pExports = &Exports[ExportIndex];

		Association.pSubobjectToAssociate = &Subobjects[LocalRootSignatureBaseIndex + LocalRootSignatureIndex];

		Subobjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &ExportAssociations[ExportIndex] });
	}

	// Existing collection objects

	for (int32 CollectionIndex = 0; CollectionIndex < ExistingCollections.size(); ++CollectionIndex)
	{
		Subobjects.push_back({ D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION, &ExistingCollections[CollectionIndex] });
	}

	// Create ray tracing pipeline state object

	D3D12_STATE_OBJECT_DESC Desc = {};
	Desc.NumSubobjects = static_cast<UINT>(Subobjects.size());
	Desc.pSubobjects = &Subobjects[0];
	Desc.Type = StateObjectType;

	COMPtr<ID3D12StateObject> Result;
	CheckHResult(RayTracingDevice->CreateStateObject(&Desc, COMPtr_RefParam(Result, IID_ID3D12StateObject)));

	return Result;
}



RayTracingDescriptorHeapCache::~RayTracingDescriptorHeapCache()
{
	std::unique_lock Lock{ CriticalSection };

	for (auto& It : Entries)
	{
		It.Heap->Release();
	}
	Entries.clear();
}

void RayTracingDescriptorHeapCache::ReleaseHeap(Entry& Entry)
{
	std::unique_lock Lock{ CriticalSection };

	Entries.emplace_back(Entry);

	--AllocatedEntries;
}

RayTracingDescriptorHeapCache::Entry RayTracingDescriptorHeapCache::AllocateHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32 NumDescriptors)
{
	std::unique_lock Lock{ CriticalSection };

	++AllocatedEntries;

	Entry Result = {};

	auto& Fence = Device->GetFence();
	auto CompletedFenceValue = Fence.GetLastCompletedFenceFast();

	for (unsigned EntryIndex = 0; EntryIndex < Entries.size(); ++EntryIndex)
	{
		auto& It = Entries[EntryIndex];

		if (It.Type == Type && It.NumDescriptors >= NumDescriptors && It.FenceValue <= CompletedFenceValue)
		{
			Result = It;
			Entries[EntryIndex] = Entries.back();
			Entries.pop_back();

			return Result;
		}
	}

	// Compatible heap was not found in cache, so create a new one.
	ReleaseStaleEntries(100, CompletedFenceValue); // Release heaps that were not used for 100 frames before allocating new.

	D3D12_DESCRIPTOR_HEAP_DESC Desc = {};

	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptors;
	Desc.NodeMask = 0;

	ID3D12DescriptorHeap* D3D12Heap = nullptr;
	Device->GetRayTracingDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&D3D12Heap));
	Windows::D3D::Debug(D3D12Heap, Desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? "RT View Heap" : "RT Sampler Heap");

	Result.NumDescriptors = NumDescriptors;
	Result.Type = Type;
	Result.Heap = D3D12Heap;

	return Result;
}

void RayTracingDescriptorHeapCache::ReleaseStaleEntries(uint32 MaxAge, uint64 CompletedFenceValue)
{
	unsigned EntryIndex = 0;
	while (EntryIndex < Entries.size())
	{
		auto& It = Entries[EntryIndex];
		if (It.FenceValue + MaxAge <= CompletedFenceValue)
		{
			It.Heap->Release();
			Entries[EntryIndex] = Entries.back();
			Entries.pop_back();
		}
		else
		{
			++EntryIndex;
		}
	}
}

RayTracingDescriptorHeap::~RayTracingDescriptorHeap()
{
	if (D3D12Heap)
	{
		Device->GetRayTracingDescriptorHeapCache()->ReleaseHeap(HeapCacheEntry);
	}
}

void RayTracingDescriptorHeap::Init(uint32 InMaxNumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	HeapCacheEntry = Device->GetRayTracingDescriptorHeapCache()->AllocateHeap(Type, InMaxNumDescriptors);

	MaxNumDescriptors = HeapCacheEntry.NumDescriptors;
	D3D12Heap = HeapCacheEntry.Heap;

	CPUBase = D3D12Heap->GetCPUDescriptorHandleForHeapStart();
	GPUBase = D3D12Heap->GetGPUDescriptorHandleForHeapStart();

	DescriptorSize = Device->GetRayTracingDevice()->GetDescriptorHandleIncrementSize(Type);
}

bool RayTracingDescriptorHeap::CanAllocate(uint32 InNumDescriptors) const
{
	return NumAllocatedDescriptors + InNumDescriptors <= MaxNumDescriptors;
}

uint32 RayTracingDescriptorHeap::Allocate(uint32 InNumDescriptors)
{
	lconstraint(CanAllocate(InNumDescriptors));

	uint32 Result = NumAllocatedDescriptors;
	NumAllocatedDescriptors += InNumDescriptors;
	return Result;
}

void RayTracingDescriptorHeap::UpdateSyncPoint()
{
	auto& Fence = Device->GetFence();

	HeapCacheEntry.FenceValue = std::max(HeapCacheEntry.FenceValue, Fence.GetCurrentFence());
}

void RayTracingDescriptorCache::SetDescriptorHeaps(D3D12RayContext& Context)
{
	ID3D12DescriptorHeap* Heaps[2] =
	{
		ViewHeap.D3D12Heap,
		SamplerHeap.D3D12Heap
	};

	Context.RayTracingCommandList()->SetDescriptorHeaps(2, Heaps);
}

uint32 RayTracingDescriptorCache::GetDescriptorTableBaseIndex(const D3D12_CPU_DESCRIPTOR_HANDLE* Descriptors, uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	auto& Heap = (Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ? ViewHeap : SamplerHeap;
	auto& Map = (Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ? ViewDescriptorTableCache : SamplerDescriptorTableCache;

	const uint64 Key = CityHash64((const char*)Descriptors, sizeof(Descriptors[0]) * NumDescriptors);

	uint32 DescriptorTableBaseIndex = ~0u;
	auto itr = Map.find(Key);

	if (itr != Map.end())
	{
		DescriptorTableBaseIndex = itr->second;
	}
	else
	{
		DescriptorTableBaseIndex = Heap.Allocate(NumDescriptors);

		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = Heap.GetDescriptorCPU(DescriptorTableBaseIndex);
		LAssert(Heap.CPUBase.ptr, "Ray tracing descriptor heap of type assigned to descriptor cache is invalid.");
		Device->GetRayTracingDevice()->CopyDescriptors(1, &DestDescriptor, &NumDescriptors, NumDescriptors, Descriptors, nullptr, Type);

		Map.emplace(Key, DescriptorTableBaseIndex);
	}

	return DescriptorTableBaseIndex;
}

struct FD3D12RayTracingGlobalResourceBinder
{
	FD3D12RayTracingGlobalResourceBinder(D3D12RayContext& InCommandContext)
		: CommandContext(InCommandContext)
	{
	}

	void SetRootCBV(uint32 BaseSlotIndex, uint32 DescriptorIndex, D3D12_GPU_VIRTUAL_ADDRESS Address)
	{
		CommandContext.RayTracingCommandList()->SetComputeRootConstantBufferView(BaseSlotIndex + DescriptorIndex, Address);
	}

	void SetRootSRV(uint32 BaseSlotIndex, uint32 DescriptorIndex, D3D12_GPU_VIRTUAL_ADDRESS Address)
	{
		CommandContext.RayTracingCommandList()->SetComputeRootShaderResourceView(BaseSlotIndex + DescriptorIndex, Address);
	}

	void SetRootDescriptorTable(uint32 SlotIndex, D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTable)
	{
		CommandContext.RayTracingCommandList()->SetComputeRootDescriptorTable(SlotIndex, DescriptorTable);
	}

	D3D12_GPU_VIRTUAL_ADDRESS CreateTransientConstantBuffer(const void* Data, uint32 DataSize)
	{
		LAssert(0, "Loose parameters and transient constant buffers are not implemented for global ray tracing shaders (raygen, miss, callable)");
		return (D3D12_GPU_VIRTUAL_ADDRESS)0;
	}

	D3D12RayContext& CommandContext;
};

template <typename ResourceBinderType>
static void SetRayTracingShaderResources(
	D3D12RayContext& CommandContext,
	const platform_ex::Windows::D3D12::RayTracingShader* Shader,
	const RayTracingShaderBindings& ResourceBindings,
	RayTracingDescriptorCache& DescriptorCache,
	ResourceBinderType& Binder);

void DispatchRays(D3D12RayContext* CommandContext, const RayTracingShaderBindings& GlobalBindings, const D3D12RayTracingPipelineState* Pipeline,
	uint32 RayGenShaderIndex, RayTracingShaderTable* OptShaderTable, const D3D12_DISPATCH_RAYS_DESC& DispatchDesc)
{
	CommandContext->RayTracingCommandList()->SetComputeRootSignature(Pipeline->GetRootSignature());

	auto RayGenShader = Pipeline->RayGenShaders.Shaders[RayGenShaderIndex].get();

	if (OptShaderTable && OptShaderTable->DescriptorCache)
	{
		//TODO
	}
	else
	{
		RayTracingDescriptorCache TransientDescriptorCache(&CommandContext->GetDevice());

		TransientDescriptorCache.Init(MAX_SRVS + MAX_UAVS, MAX_SAMPLERS);
		TransientDescriptorCache.SetDescriptorHeaps(*CommandContext);

		FD3D12RayTracingGlobalResourceBinder ResourceBinder(*CommandContext);
		SetRayTracingShaderResources(*CommandContext, RayGenShader, GlobalBindings, TransientDescriptorCache, ResourceBinder);
	}

	auto StateObject = Pipeline->StateObject.Get();

	auto RayTracingCommandList = CommandContext->RayTracingCommandList();

	RayTracingCommandList->SetPipelineState1(StateObject);
	RayTracingCommandList->DispatchRays(&DispatchDesc);
}
