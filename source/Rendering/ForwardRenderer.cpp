#include "Rendering/ForwardRenderer.h"
#include <algorithm>
#include "Device.h"
#include "DeviceContext.h"
#include "Rendering/Material.h"
#include "Rendering/MaterialInstance.h"
#include "Rendering/Mesh.h"
#include "SamplerState.h"
#include "EngineUtilities/Utilities/Camera.h"
#include "EngineUtilities/Utilities/EditorViewportPass.h"
#include "EngineUtilities/Utilities/Skybox.h"

/**
 * @brief Inicializa los buffers y estados necesarios para renderizar.
 */
HRESULT
ForwardRenderer::init(Device& device) {
    // Inicializa el buffer de datos por frame
    HRESULT hr = m_perFrameBuffer.init(device, sizeof(CBPerFrame));
    if (FAILED(hr)) {
        return hr;
    }

    // Inicializa el buffer de datos por objeto
    hr = m_perObjectBuffer.init(device, sizeof(CBPerObject));
    if (FAILED(hr)) {
        return hr;
    }

    // Inicializa el buffer de datos por material
    hr = m_perMaterialBuffer.init(device, sizeof(CBPerMaterial));
    if (FAILED(hr)) {
        return hr;
    }

    // Crea el depth stencil para objetos transparentes (sin escritura de profundidad)
    hr = m_transparentDepthStencil.init(device,
        true,
        D3D11_DEPTH_WRITE_MASK_ZERO,
        D3D11_COMPARISON_LESS_EQUAL);
    if (FAILED(hr)) {
        return hr;
    }

    // Crea los blend states para los diferentes tipos de transparencia
    hr = createBlendStates(device);
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

/**
 * @brief Actualiza los datos de camara y luces que se mandan a la GPU cada frame.
 */
void
ForwardRenderer::updatePerFrame(const Camera& camera,
    const RenderScene& scene,
    DeviceContext& deviceContext) {
    // Guarda las matrices de vista y proyeccion transpuestas (requerido por HLSL)
    XMStoreFloat4x4(&m_cbPerFrame.View, XMMatrixTranspose(camera.getView()));
    XMStoreFloat4x4(&m_cbPerFrame.Projection, XMMatrixTranspose(camera.getProj()));
    m_cbPerFrame.CameraPos = camera.getPosition();

    // Valores por defecto de la luz
    m_cbPerFrame.LightDir = EU::Vector3(0.0f, -1.0f, 0.0f);
    m_cbPerFrame.LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);

    // Si hay una luz direccional en la escena, usa sus datos
    if (!scene.directionalLights.empty()) {
        const LightData& mainLight = scene.directionalLights.front();
        m_cbPerFrame.LightDir = mainLight.direction;
        m_cbPerFrame.LightColor = mainLight.color * mainLight.intensity;
    }

    // Manda los datos actualizados al buffer de la GPU
    m_perFrameBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerFrame, 0, 0);
}

/**
 * @brief Ejecuta todos los pasos de renderizado del frame.
 */
void
ForwardRenderer::render(DeviceContext& deviceContext,
    const Camera& camera,
    RenderScene& scene,
    EditorViewportPass& viewportPass) {
    // Limpia el viewport con un color gris oscuro
    const float viewportClear[4] = { 0.10f, 0.10f, 0.10f, 1.0f };
    viewportPass.begin(deviceContext, viewportClear);
    viewportPass.setViewport(deviceContext);
    viewportPass.clearDepth(deviceContext);

    // Clasifica los objetos y actualiza los datos del frame
    buildQueues(scene, camera);
    updatePerFrame(camera, scene, deviceContext);

    // Ejecuta los passes en orden: skybox, opacos y transparentes
    renderSkyboxPass(deviceContext, scene);
    renderOpaquePass(deviceContext);
    renderTransparentPass(deviceContext);
}

/**
 * @brief Separa los objetos de la escena en colas de opacos y transparentes.
 */
void
ForwardRenderer::buildQueues(RenderScene& scene, const Camera& camera) {
    (void)camera;
    m_opaqueQueue.clear();
    m_transparentQueue.clear();

    // Agrega los objetos a su cola correspondiente
    for (auto& object : scene.opaqueObjects) {
        m_opaqueQueue.push_back(&object);
    }
    for (auto& object : scene.transparentObjects) {
        m_transparentQueue.push_back(&object);
    }

    // Ordena opacos por material para reducir cambios de estado en la GPU
    std::sort(m_opaqueQueue.begin(), m_opaqueQueue.end(),
        [](const RenderObject* lhs, const RenderObject* rhs) {
            if (lhs->materialInstance != rhs->materialInstance) {
                return lhs->materialInstance < rhs->materialInstance;
            }
            return lhs->distanceToCamera < rhs->distanceToCamera;
        });

    // Ordena transparentes de atras hacia adelante para que el blend se vea bien
    std::sort(m_transparentQueue.begin(), m_transparentQueue.end(),
        [](const RenderObject* lhs, const RenderObject* rhs) {
            return lhs->distanceToCamera > rhs->distanceToCamera;
        });
}

/**
 * @brief Dibuja todos los objetos opacos de la escena.
 */
void
ForwardRenderer::renderOpaquePass(DeviceContext& deviceContext) {
    // Activa el buffer de frame y el blend state sin transparencia
    m_perFrameBuffer.render(deviceContext, 0, 1, true);
    deviceContext.OMSetBlendState(m_opaqueBlendState, m_blendFactor, 0xffffffff);

    for (const RenderObject* object : m_opaqueQueue) {
        if (!object) {
            continue;
        }
        renderObject(deviceContext, *object, RenderPassType::Opaque);
    }
}

/**
 * @brief Dibuja todos los objetos transparentes de la escena.
 */
void
ForwardRenderer::renderTransparentPass(DeviceContext& deviceContext) {
    m_perFrameBuffer.render(deviceContext, 0, 1, true);

    for (const RenderObject* object : m_transparentQueue) {
        if (!object) {
            continue;
        }
        // Resuelve el blend state segun el material del objeto
        Material* material = object->materialInstance ? object->materialInstance->getMaterial() : nullptr;
        deviceContext.OMSetBlendState(resolveBlendState(material), m_blendFactor, 0xffffffff);
        renderObject(deviceContext, *object, RenderPassType::Transparent);
    }

    // Regresa al blend state opaco al terminar
    deviceContext.OMSetBlendState(m_opaqueBlendState, m_blendFactor, 0xffffffff);
}

/**
 * @brief Dibuja el skybox si la escena tiene uno asignado.
 */
void
ForwardRenderer::renderSkyboxPass(DeviceContext& deviceContext, RenderScene& scene) {
    if (!scene.skybox) {
        return;
    }
    scene.skybox->render(deviceContext);
}

/**
 * @brief Dibuja un objeto aplicando su material y configurando el pipeline.
 */
void
ForwardRenderer::renderObject(DeviceContext& deviceContext,
    const RenderObject& object,
    RenderPassType passType) {
    if (!object.mesh || (!object.materialInstance && object.materialInstances.empty())) {
        return;
    }

    // Actualiza y manda la matriz world del objeto a la GPU
    XMStoreFloat4x4(&m_cbPerObject.World, XMMatrixTranspose(object.world));
    m_perObjectBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerObject, 0, 0);
    m_perObjectBuffer.render(deviceContext, 1, 1, true);

    deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Recorre cada submesh del objeto
    std::vector<Submesh>& submeshes = object.mesh->getSubmeshes();
    for (Submesh& submesh : submeshes) {
        // Determina que material le toca a este submesh
        MaterialInstance* materialInstance = object.materialInstance;
        if (submesh.materialSlot < object.materialInstances.size() &&
            object.materialInstances[submesh.materialSlot]) {
            materialInstance = object.materialInstances[submesh.materialSlot];
        }

        if (!materialInstance) {
            continue;
        }

        Material* material = materialInstance->getMaterial();
        if (!material) {
            continue;
        }

        // Aplica los estados del material al pipeline
        if (material->getRasterizerState()) {
            material->getRasterizerState()->render(deviceContext);
        }

        // Para transparentes usa el depth stencil sin escritura
        if (passType == RenderPassType::Transparent) {
            m_transparentDepthStencil.render(deviceContext, 0, false);
        }
        else if (material->getDepthStencilState()) {
            material->getDepthStencilState()->render(deviceContext, 0, false);
        }

        if (material->getShader()) {
            material->getShader()->render(deviceContext);
        }

        if (material->getSamplerState()) {
            material->getSamplerState()->render(deviceContext, 0, 1);
        }

        // Manda las texturas del material al shader
        materialInstance->bindTextures(deviceContext);

        // Actualiza los parametros del material en el constant buffer
        const MaterialParams& params = materialInstance->getParams();
        m_cbPerMaterial.BaseColor = params.baseColor;
        m_cbPerMaterial.Metallic = params.metallic;
        m_cbPerMaterial.Roughness = params.roughness;
        m_cbPerMaterial.AO = params.ao;
        m_cbPerMaterial.NormalScale = params.normalScale;
        m_cbPerMaterial.EmissiveStrength = params.emissiveStrength;
        m_cbPerMaterial.AlphaCutoff = 0.0f;

        // Si el material es tipo Masked activa el alpha cutoff
        if (material->getDomain() == MaterialDomain::Masked) {
            m_cbPerMaterial.AlphaCutoff = params.alphaCutoff;
        }
        m_perMaterialBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerMaterial, 0, 0);
        m_perMaterialBuffer.render(deviceContext, 2, 1, true);

        // Manda los buffers de geometria y dibuja el submesh
        submesh.vertexBuffer.render(deviceContext, 0, 1);
        submesh.indexBuffer.render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
        deviceContext.DrawIndexed(submesh.indexCount, submesh.startIndex, 0);
    }
}

/**
 * @brief Crea los blend states para los diferentes modos de transparencia.
 */
HRESULT
ForwardRenderer::createBlendStates(Device& device) {
    if (!device.m_device) {
        return E_POINTER;
    }

    D3D11_BLEND_DESC blendDesc{};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;

    D3D11_RENDER_TARGET_BLEND_DESC& renderTarget = blendDesc.RenderTarget[0];

    // Blend state opaco: sin mezcla, escribe directo al render target
    renderTarget.BlendEnable = FALSE;
    renderTarget.SrcBlend = D3D11_BLEND_ONE;
    renderTarget.DestBlend = D3D11_BLEND_ZERO;
    renderTarget.BlendOp = D3D11_BLEND_OP_ADD;
    renderTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
    renderTarget.DestBlendAlpha = D3D11_BLEND_ZERO;
    renderTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    renderTarget.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = device.m_device->CreateBlendState(&blendDesc, &m_opaqueBlendState);
    if (FAILED(hr)) {
        return hr;
    }

    // Blend state alpha: mezcla por canal alpha normal
    renderTarget.BlendEnable = TRUE;
    renderTarget.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    renderTarget.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    renderTarget.BlendOp = D3D11_BLEND_OP_ADD;
    renderTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
    renderTarget.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    renderTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;

    hr = device.m_device->CreateBlendState(&blendDesc, &m_alphaBlendState);
    if (FAILED(hr)) {
        return hr;
    }

    // Blend state aditivo: suma los colores, util para efectos de luz
    renderTarget.BlendEnable = TRUE;
    renderTarget.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    renderTarget.DestBlend = D3D11_BLEND_ONE;
    renderTarget.BlendOp = D3D11_BLEND_OP_ADD;
    renderTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
    renderTarget.DestBlendAlpha = D3D11_BLEND_ONE;
    renderTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;

    hr = device.m_device->CreateBlendState(&blendDesc, &m_additiveBlendState);
    if (FAILED(hr)) {
        return hr;
    }

    // Blend state premultiplicado: para texturas con alpha ya multiplicado
    renderTarget.BlendEnable = TRUE;
    renderTarget.SrcBlend = D3D11_BLEND_ONE;
    renderTarget.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    renderTarget.BlendOp = D3D11_BLEND_OP_ADD;
    renderTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
    renderTarget.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    renderTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;

    return device.m_device->CreateBlendState(&blendDesc, &m_premultipliedBlendState);
}