#include "EngineUtilities\Utilities\EditorViewportPass.h"
#include "Device.h"
#include "DeviceContext.h"

/**
 * @brief Inicializa el viewport creando todos sus recursos de GPU.
 */
HRESULT
EditorViewportPass::init(Device& device, unsigned int width, unsigned int height) {
    return createResources(device, width, height);
}

/**
 * @brief Recrea los recursos si el tamanio del viewport cambio.
 */
HRESULT
EditorViewportPass::resize(Device& device, unsigned int width, unsigned int height) {
    // Tamanio minimo para evitar errores en DirectX
    if (width < 64) width = 64;
    if (height < 64) height = 64;

    // Si el tamanio no cambio y los recursos son validos no hace nada
    if (width == m_width && height == m_height && isValid())
        return S_OK;

    return createResources(device, width, height);
}

/**
 * @brief Crea todas las texturas y vistas necesarias para el viewport.
 */
HRESULT
EditorViewportPass::createResources(Device& device, unsigned int width, unsigned int height) {
    destroy();

    // Evita crear recursos con tamanio cero
    if (width == 0) width = 1;
    if (height == 0) height = 1;

    m_width = width;
    m_height = height;

    HRESULT hr = S_OK;

    // 1) Textura de color donde se va a dibujar la escena
    hr = m_colorTexture.init(
        device,
        width, height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        1, 0
    );
    if (FAILED(hr)) return hr;

    // 2) Render Target View sobre la textura de color
    hr = m_rtv.init(
        device,
        m_colorTexture,
        D3D11_RTV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_R8G8B8A8_UNORM
    );
    if (FAILED(hr)) return hr;

    // 3) Shader Resource View para mandar la textura a ImGui
    hr = m_colorSRV.init(device, m_colorTexture, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (FAILED(hr)) return hr;

    // 4) Textura de profundidad para el depth testing
    hr = m_depthTexture.init(
        device,
        width, height,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        D3D11_BIND_DEPTH_STENCIL,
        1, 0
    );
    if (FAILED(hr)) return hr;

    // 5) Depth Stencil View sobre la textura de profundidad
    hr = m_dsv.init(
        device,
        m_depthTexture,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        D3D11_DSV_DIMENSION_TEXTURE2D
    );
    if (FAILED(hr)) return hr;

    return S_OK;
}

/**
 * @brief Limpia el viewport con el color indicado y lo activa para recibir el renderizado.
 */
void
EditorViewportPass::begin(DeviceContext& deviceContext, const float clearColor[4]) {
    m_rtv.render(deviceContext, m_dsv, 1, clearColor);
}

/**
 * @brief Limpia el buffer de profundidad para el siguiente frame.
 */
void
EditorViewportPass::clearDepth(DeviceContext& deviceContext) {
    m_dsv.render(deviceContext);
}

/**
 * @brief Configura el viewport con el tamanio actual para el pipeline de renderizado.
 */
void
EditorViewportPass::setViewport(DeviceContext& deviceContext) {
    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    deviceContext.m_deviceContext->RSSetViewports(1, &vp);
}