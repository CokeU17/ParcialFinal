#pragma once
#include "Prerequisites.h"
#include "Buffer.h"
#include "DepthStencilState.h"
#include "Rendering/RenderScene.h"
#include "Rendering/RenderTypes.h"

class Device;
class DeviceContext;
class Camera;
class EditorViewportPass;
class Material;

/**
 * @brief Clase que se encarga de renderizar la escena usando Forward Rendering.
 */
class ForwardRenderer {
public:

    /**
     * @brief Inicializa todo lo necesario para que el renderer funcione.
     * @param device El dispositivo de DirectX 11.
     * @return HRESULT que indica si salio bien o hubo un error.
     */
    HRESULT init(Device& device);

    void resize(Device& device, unsigned int width, unsigned int height);

    /**
     * @brief Actualiza la informacion de camara y escena cada frame.
     * @param camera La camara que esta viendo la escena.
     * @param scene La escena con todos los objetos.
     * @param deviceContext El contexto del dispositivo.
     */
    void updatePerFrame(const Camera& camera, const RenderScene& scene, DeviceContext& deviceContext);

    /**
     * @brief Renderiza todo el frame, pasando por los diferentes pasos de render.
     * @param deviceContext Contexto del dispositivo para mandar comandos a la GPU.
     * @param camera La camara activa.
     * @param scene La escena a renderizar.
     * @param viewportPass El viewport del editor donde se ve el resultado.
     */
    void render(DeviceContext& deviceContext,
        const Camera& camera,
        RenderScene& scene,
        EditorViewportPass& viewportPass);

    void destroy();

private:

    /**
     * @brief Clasifica los objetos de la escena en opacos y transparentes.
     * @param scene La escena con los objetos.
     * @param camera La camara para saber que objetos son visibles.
     */
    void buildQueues(RenderScene& scene, const Camera& camera);

    /** @brief Dibuja los objetos que no tienen transparencia. */
    void renderOpaquePass(DeviceContext& deviceContext);

    /** @brief Dibuja los objetos que tienen transparencia. */
    void renderTransparentPass(DeviceContext& deviceContext);

    /** @brief Dibuja el skybox al final del frame. */
    void renderSkyboxPass(DeviceContext& deviceContext, RenderScene& scene);

    /**
     * @brief Dibuja un objeto con su material y el tipo de pass indicado.
     * @param deviceContext Contexto del dispositivo.
     * @param object El objeto a dibujar.
     * @param passType El tipo de pass (opaco, transparente, etc).
     */
    void renderObject(DeviceContext& deviceContext, const RenderObject& object, RenderPassType passType);

    /**
     * @brief Crea los blend states que se usan para la transparencia.
     * @param device El dispositivo de DirectX 11.
     * @return HRESULT que indica si se crearon bien.
     */
    HRESULT createBlendStates(Device& device);

    ID3D11BlendState* resolveBlendState(const Material* material) const { return nullptr; }

private:

    // Buffers para mandar datos a la GPU por frame, objeto y material
    Buffer m_perFrameBuffer;
    Buffer m_perObjectBuffer;
    Buffer m_perMaterialBuffer;

    // Estado de depth para los objetos transparentes
    DepthStencilState m_transparentDepthStencil;

    // Los diferentes blend states segun el tipo de objeto
    ID3D11BlendState* m_alphaBlendState = nullptr;
    ID3D11BlendState* m_opaqueBlendState = nullptr;
    ID3D11BlendState* m_additiveBlendState = nullptr;
    ID3D11BlendState* m_premultipliedBlendState = nullptr;

    float m_blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Structs con los datos que se mandan a los shaders
    CBPerFrame m_cbPerFrame{};
    CBPerObject m_cbPerObject{};
    CBPerMaterial m_cbPerMaterial{};

    // Listas de objetos separados por tipo de renderizado
    std::vector<const RenderObject*> m_opaqueQueue;
    std::vector<const RenderObject*> m_transparentQueue;
};