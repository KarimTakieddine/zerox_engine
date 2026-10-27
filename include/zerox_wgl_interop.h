#pragma once

#include <d3d11.h>

#include <glad/wgl.h>

#include <render_buffer.h>

namespace ZeroX
{
	class WGLInterop
	{
	public:
		explicit WGLInterop(const renderer::RenderBuffer& buffer, ID3D11Device* device, ID3D11Texture2D* clientSurface);
		~WGLInterop();

		const HANDLE& getWGLDXDevice() const;
		HANDLE& getWGLDXDevice();

		const HANDLE& getWGLSharedTexture() const;
		HANDLE& getWGLSharedTexture();

	private:
		HANDLE m_wglDXDeviceHandle	{ NULL };
		HANDLE m_wglSharedTexture	{ NULL };
	};
}
