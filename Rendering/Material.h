#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class ShaderProgram;
class RasterizerState;
class DepthStencilState;
class SamplerState;

/**
 * @brief Clase que representa un material con sus estados de renderizado.
 *
 * Guarda el shader y los estados que definen como se ve un objeto al renderizarse.
 */
class Material {
public:

    /** @brief Asigna el shader que va a usar el material. */
    void setShader(ShaderProgram* shader) { m_shader = shader; }

    /** @brief Asigna el estado del rasterizador. */
    void setRasterizerState(RasterizerState* state) { m_rasterizerState = state; }

    /** @brief Asigna el estado de depth stencil. */
    void setDepthStencilState(DepthStencilState* state) { m_depthStencilState = state; }

    /** @brief Asigna el sampler state para las texturas. */
    void setSamplerState(SamplerState* state) { m_samplerState = state; }

    /** @brief Define el dominio del material (opaco, transparente, etc). */
    void setDomain(MaterialDomain domain) { m_domain = domain; }

    /** @brief Define el modo de blend del material. */
    void setBlendMode(BlendMode blendMode) { m_blendMode = blendMode; }

    /** @brief Regresa el shader del material. */
    ShaderProgram* getShader() const { return m_shader; }

    /** @brief Regresa el estado del rasterizador. */
    RasterizerState* getRasterizerState() const { return m_rasterizerState; }

    /** @brief Regresa el estado de depth stencil. */
    DepthStencilState* getDepthStencilState() const { return m_depthStencilState; }

    /** @brief Regresa el sampler state. */
    SamplerState* getSamplerState() const { return m_samplerState; }

    /** @brief Regresa el dominio del material. */
    MaterialDomain getDomain() const { return m_domain; }

    /** @brief Regresa el modo de blend del material. */
    BlendMode getBlendMode() const { return m_blendMode; }

private:

    // Shader que define como se procesa visualmente el objeto
    ShaderProgram* m_shader = nullptr;

    // Estados que controlan como se dibuja el objeto
    RasterizerState* m_rasterizerState = nullptr;
    DepthStencilState* m_depthStencilState = nullptr;
    SamplerState* m_samplerState = nullptr;

    // Tipo de material y modo de mezcla con otros objetos
    MaterialDomain m_domain = MaterialDomain::Opaque;
    BlendMode m_blendMode = BlendMode::Opaque;
};