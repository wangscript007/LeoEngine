#include <atomic>
#include <thread>
#include <mutex>
#include <array>
#include <platform.h>
#include <Core\COM.hpp>
#include <exception.hpp>
#include "d3dx11.hpp"
#include "RenderStates.hpp"
#include "PostProcess.hpp"

namespace leo {
	class HDRProcess :public PostProcess {
	private:
		class HDRCommon:public PostProcess{
		public:
			HDRCommon(ID3D11Device* create);

			void Apply(ID3D11DeviceContext* context) override;
		protected:
			void GetSampleOffset(UINT width,UINT height);

			std::array<float4, 2> mCpuParams;

			D3D11_VIEWPORT mViewPort;

			CD3D11_TEXTURE2D_DESC mTexDesc;

			static ID3D11Buffer* mGpuParams;//sizeof = float4[2]
		private:
			
			static ID3D11VertexShader* mLumVS;

			static ID3D11SamplerState* src_sampler;
			static ID3D11SamplerState* last_lum_sampler;

		};
		class LumLogProcess : public HDRCommon {
		public:
			LumLogProcess(ID3D11Device* create,unsigned level);

			ID3D11ShaderResourceView* Output() const;

			void Draw(ID3D11DeviceContext* context, ID3D11ShaderResourceView* src, ID3D11RenderTargetView*) override;
		private:
			win::unique_com<ID3D11RenderTargetView> mLumLogRTV = nullptr;
			win::unique_com<ID3D11ShaderResourceView> mLumLogOutput = nullptr;
		};
		class LumIterativeProcess : public HDRCommon {
		public:
			LumIterativeProcess(ID3D11Device* create,unsigned level);

			void Draw(ID3D11DeviceContext* context, ID3D11ShaderResourceView* src, ID3D11RenderTargetView* dst) override;
		private:
		};
		class LumAdaptedProcess : public HDRCommon {
		public:
			LumAdaptedProcess(ID3D11Device* create);

			void Draw(ID3D11DeviceContext* context, ID3D11ShaderResourceView* src, ID3D11RenderTargetView* dst) override;
		private:
		};

		
		class IHDRBundleProcess {

		};

		//note:this class create/update/set LumAdaptedProcess pixel shader constants
		class HDRBundleProcess :public IHDRBundleProcess {

		};

		//TODO :impl this
		class HDRBundleCSProcess :public IHDRBundleProcess {
		};
	};
}
