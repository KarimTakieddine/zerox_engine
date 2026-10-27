#include <stdexcept>

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
			.Format	= DXGI_FORMAT_R8G8B8A8_UNORM
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
			// TODO(Karim): General function for mapping GetLastError() to string
			throw std::runtime_error("Failed to create GraphicsDevice");
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
}