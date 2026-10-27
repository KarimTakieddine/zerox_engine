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

		void setViewport(float width, float height);
		void createRenderTargetView();
		void targetBackBuffer();
		void resizeBuffers(UINT width, UINT height);

	private:
		void releaseBufferResources();

		ComPtr<ID3D11Device> m_device			{ nullptr };
		ComPtr<ID3D11DeviceContext> m_context	{ nullptr };
		ComPtr<IDXGISwapChain> m_swapChain		{ nullptr };
	};
}
