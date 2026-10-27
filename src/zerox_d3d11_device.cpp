#include <stdexcept>

#include <d3dcompiler.h>

#include "zerox_d3d11_device.h"

namespace ZeroX
{
	D3D11Device::D3D11Device(HWND windowHandle, UINT width, UINT height)
	{
		if (width * height == 0)
		{
			throw std::runtime_error("Cannot create D3D11 device for window with invalid dimensions");
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = { };

		swapChainDesc.BufferCount	= 2;
		swapChainDesc.SwapEffect	= DXGI_SWAP_EFFECT_FLIP_DISCARD;

		swapChainDesc.BufferDesc = {
			.Width	= width,
			.Height	= height,
			.Format	= DXGI_FORMAT_B8G8R8A8_UNORM
		};

		swapChainDesc.BufferDesc.RefreshRate = {
			.Numerator		= 60,
			.Denominator	= 1
		};

		swapChainDesc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow	= windowHandle;

		/*
		* The image quality level. The higher the quality, the lower the performance.
		* The valid range is between zero and one less than the level returned by
		* ID3D10Device::CheckMultisampleQualityLevels for Direct3D 10 or
		* ID3D11Device::CheckMultisampleQualityLevels for Direct3D 11
		* 
		* The default sampler mode, with no anti-aliasing, has a count of 1 and a quality level of 0.
		*/

		swapChainDesc.SampleDesc = {
			.Count		= 1,
			.Quality	= 0
		};

		/*
		* Because the target output can't be chosen explicitly when the swap chain is created,
		* we recommend not to create a full-screen swap chain. This can reduce presentation
		* performance if the swap chain size and the output window size do not match.
		* Here are two ways to ensure that the sizes match:
		*
		*	- Create a windowed swap chain and then set it full-screen using
		*	IDXGISwapChain::SetFullscreenState.
		*	- Save a pointer to the swap chain immediately after creation, and
		*	use it to get the output window size during a WM_SIZE event. Then
		*	resize the swap chain buffers (with IDXGISwapChain::ResizeBuffers)
		*	during the transition from windowed to full-screen.
		* If the swap chain is in full-screen mode, before you release it you must use SetFullscreenState to switch it to windowed mode.
		*/

		swapChainDesc.Windowed = TRUE;

		/*
		* If you provide a D3D_FEATURE_LEVEL array that contains D3D_FEATURE_LEVEL_11_1 on a computer
		* that doesn't have the Direct3D 11.1 runtime installed, this function immediately fails
		* with E_INVALIDARG.
		*/

		D3D_FEATURE_LEVEL targetArray[1] = { D3D_FEATURE_LEVEL_11_1 };

		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_1_0_GENERIC;

		HRESULT status = D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE, NULL,
			0,
			targetArray, 1,
			D3D11_SDK_VERSION,
			&swapChainDesc, &m_swapChain,
			&m_device,
			&featureLevel,
			&m_context);

		if (status == E_INVALIDARG || featureLevel != D3D_FEATURE_LEVEL_11_1)
		{
			/*
			* Fall back to the following array of feature levels:
			*	{
			*		D3D_FEATURE_LEVEL_11_0,
			*		D3D_FEATURE_LEVEL_10_1,
			*		D3D_FEATURE_LEVEL_10_0,
			*		D3D_FEATURE_LEVEL_9_3,
			*		D3D_FEATURE_LEVEL_9_2,
			*		D3D_FEATURE_LEVEL_9_1,
			*	};
			*/

			status = D3D11CreateDeviceAndSwapChain(
				NULL,
				D3D_DRIVER_TYPE_HARDWARE, NULL,
				0,
				NULL, 0,
				D3D11_SDK_VERSION,
				&swapChainDesc, &m_swapChain,
				&m_device,
				&featureLevel,
				&m_context);
		}

		if (status != S_OK || ( featureLevel != D3D_FEATURE_LEVEL_11_0 && featureLevel != D3D_FEATURE_LEVEL_11_1 ))
		{
			// TODO(Karim): General function for mapping GetLastError() to string
			throw std::runtime_error("Failed to create GraphicsDevice");
		}

		D3D11_TEXTURE2D_DESC clientSurfaceDesc = {
				.Width              = width,
				.Height             = height,
				.MipLevels          = 1,
				.ArraySize          = 1,
				.Format             = DXGI_FORMAT_B8G8R8A8_UNORM,
				.SampleDesc			= { .Count = 1, .Quality = 0 },
				.Usage              = D3D11_USAGE_DEFAULT,
				.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
				.CPUAccessFlags     = 0,
				.MiscFlags          = 0
		};

		status = m_device->CreateTexture2D(&clientSurfaceDesc, nullptr, &m_clientSurface);

		if (FAILED(status))
		{
			throw std::runtime_error("Failed to create DX11 surface texture");
		}

		initBlitPipeline();

		createBuffersAndViews(width, height);
	}

	ComPtr<ID3D11Device> D3D11Device::getDevice() const
	{
		return m_device;
	}

	ComPtr<ID3D11DeviceContext> D3D11Device::getContext() const
	{
		return m_context;
	}

	ComPtr<IDXGISwapChain> D3D11Device::getSwapChain() const
	{
		return m_swapChain;
	}

	ComPtr<ID3D11Texture2D> D3D11Device::getClientSurface() const
	{
		return m_clientSurface;
	}

	void D3D11Device::copySurfaceToBackBuffer(UINT width, UINT height)
	{
		ID3D11RenderTargetView* rtvs = { m_backBufferRTV.Get() };
		m_context->OMSetRenderTargets(1, &rtvs, nullptr);

		D3D11_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
		m_context->RSSetViewports(1, &vp);

		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->IASetInputLayout(nullptr);
		m_context->VSSetShader(m_blitVertexShader.Get(), nullptr, 0);
		m_context->PSSetShader(m_blitPixelShader.Get(), nullptr, 0);

		ID3D11ShaderResourceView* srvs = { m_stagingSRV.Get() };
		m_context->PSSetShaderResources(0, 1, &srvs);

		ID3D11SamplerState* samplers = { m_pointSampler.Get() };
		m_context->PSSetSamplers(0, 1, &samplers);

		m_context->Draw(3, 0);

		ID3D11ShaderResourceView* nullSRVs = { nullptr };
		m_context->PSSetShaderResources(0, 1, &nullSRVs);
	}

	void D3D11Device::initBlitPipeline()
	{
        const char* vertexShaderSource = R"(
            struct VS_OUTPUT {
                float4 position : SV_POSITION;
                float2 texcoord : TEXCOORD;
            };
            VS_OUTPUT main(uint vertexID : SV_VertexID) {
                VS_OUTPUT output;
                output.texcoord = float2((vertexID << 1) & 2, vertexID & 2);
                output.position = float4(output.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
                return output;
            }
        )";

        const char* pixelShaderSource = R"(
            Texture2D shaderTexture : register(t0);
            SamplerState sampleType : register(s0);
            struct VS_OUTPUT {
                float4 position : SV_POSITION;
                float2 texcoord : TEXCOORD;
            };
            float4 main(VS_OUTPUT input) : SV_TARGET {
                float2 flippedTexCoord = float2(input.texcoord.x, 1.0f - input.texcoord.y);
                return shaderTexture.Sample(sampleType, flippedTexCoord);
            }
        )";

        ComPtr<ID3DBlob> vsBlob;
        D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, nullptr);
        m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_blitVertexShader);

        ComPtr<ID3DBlob> psBlob;
        D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, nullptr);
        m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_blitPixelShader);

        // Enforce pixel-perfect point sampler configurations
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter		= D3D11_FILTER_MIN_MAG_MIP_POINT;
        sampDesc.AddressU	= D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV	= D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW	= D3D11_TEXTURE_ADDRESS_CLAMP;
        m_device->CreateSamplerState(&sampDesc, &m_pointSampler);
	}

	void D3D11Device::createBuffersAndViews(UINT width, UINT height)
	{
		D3D11_TEXTURE2D_DESC stagingDesc = {};
        stagingDesc.Width              = width;
        stagingDesc.Height             = height;
        stagingDesc.MipLevels          = 1;
        stagingDesc.ArraySize          = 1;
        stagingDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
        stagingDesc.SampleDesc.Count   = 1;
        stagingDesc.SampleDesc.Quality = 0;
        stagingDesc.Usage              = D3D11_USAGE_DEFAULT;
        stagingDesc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT status = m_device->CreateTexture2D(&stagingDesc, nullptr, &m_clientSurface);
        if (FAILED(status)) throw std::runtime_error("Failed to allocate interop staging texture resource");

		status = m_device->CreateShaderResourceView(m_clientSurface.Get(), nullptr, &m_stagingSRV);
        if (FAILED(status)) throw std::runtime_error("Failed to create staging SRV descriptor binding");

        ComPtr<ID3D11Texture2D> backBuffer;
		status = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (FAILED(status)) throw std::runtime_error("Failed to query Swapchain output backbuffer buffer resource");

		status = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_backBufferRTV);
        if (FAILED(status)) throw std::runtime_error("Failed to bind output backbuffer RTV wrapper");
	}
}