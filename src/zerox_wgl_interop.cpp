#include <stdexcept>

#include "zerox_wgl_interop.h"

namespace ZeroX
{
    WGLInterop::WGLInterop(const renderer::RenderBuffer& buffer, ID3D11Device* device, ID3D11Texture2D* clientSurface) { initialize(buffer, device, clientSurface); }

    WGLInterop::~WGLInterop() { release(); }

    void WGLInterop::initialize(const renderer::RenderBuffer& buffer, ID3D11Device* device, ID3D11Texture2D* clientSurface)
    {
        if (!GLAD_WGL_NV_DX_interop2)
        {
            throw std::runtime_error("Failed to initialize GLAD GLAD_WGL_NV_DX_interop2 extension");
        }

        m_wglDXDeviceHandle = wglDXOpenDeviceNV(device);
        m_wglSharedTexture  = wglDXRegisterObjectNV(
            m_wglDXDeviceHandle,
            clientSurface,
            buffer.textureHandle,
            GL_TEXTURE_2D,
            WGL_ACCESS_WRITE_DISCARD_NV
        );
    }

    void WGLInterop::release() const
    {
        if (m_wglDXDeviceHandle && m_wglSharedTexture)
        {
            wglDXUnregisterObjectNV(m_wglDXDeviceHandle, m_wglSharedTexture);
        }

        if (m_wglDXDeviceHandle)
        {
            wglDXCloseDeviceNV(m_wglDXDeviceHandle);
        }
    }

    const HANDLE& WGLInterop::getWGLDXDevice() const
    {
        return m_wglDXDeviceHandle;
    }

    HANDLE& WGLInterop::getWGLDXDevice()
    {
        return m_wglDXDeviceHandle;
    }

    const HANDLE& WGLInterop::getWGLSharedTexture() const
    {
        return m_wglSharedTexture;
    }

    HANDLE& WGLInterop::getWGLSharedTexture()
    {
        return m_wglSharedTexture;
    }
}