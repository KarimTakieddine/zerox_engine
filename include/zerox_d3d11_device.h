#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace ZeroX
{
	class D3D11Device
	{
	public:
		explicit D3D11Device(HWND windowHandle, UINT width, UINT height);

		D3D11Device(const D3D11Device&) = delete;
		D3D11Device& operator=(const D3D11Device&) = delete;

		ComPtr<ID3D11Device> getDevice() const;
		ComPtr<ID3D11DeviceContext> getContext() const;
		ComPtr<IDXGISwapChain> getSwapChain() const;
		ComPtr<ID3D11Texture2D> getClientSurface() const;

		void copySurfaceToBackBuffer(UINT width, UINT height);

	private:
		void initBlitPipeline();
		void createBuffersAndViews(UINT width, UINT height);

		ComPtr<ID3D11Device> m_device					{ nullptr };
		ComPtr<ID3D11DeviceContext> m_context			{ nullptr };
		ComPtr<IDXGISwapChain> m_swapChain				{ nullptr };
		ComPtr<ID3D11Texture2D> m_clientSurface			{ nullptr };
        ComPtr<ID3D11ShaderResourceView> m_stagingSRV	{ nullptr };
        ComPtr<ID3D11RenderTargetView> m_backBufferRTV	{ nullptr };
		ComPtr<ID3D11VertexShader> m_blitVertexShader	{ nullptr };
        ComPtr<ID3D11PixelShader> m_blitPixelShader		{ nullptr };
        ComPtr<ID3D11SamplerState> m_pointSampler		{ nullptr };
	};
}
