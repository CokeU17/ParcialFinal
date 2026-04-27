#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Material;
class DeviceContext;
class Texture;

/**
 * @brief Instancia de un material con sus texturas y parametros PBR asignados.
 *
 * Mientras Material define como se renderiza un objeto, MaterialInstance
 * guarda las texturas especificas que va a usar ese objeto.
 */
class MaterialInstance {
public:

    /** @brief Asigna el material base que va a usar esta instancia. */
    void setMaterial(Material* material) { m_material = material; }

    /** @brief Asigna la textura de color base (albedo). */
    void setAlbedo(Texture* texture) { m_albedo = texture; }

    /** @brief Asigna la textura de normales para dar detalle de relieve. */
    void setNormal(Texture* texture) { m_normal = texture; }

    /** @brief Asigna la textura que define que tan metalico es el objeto. */
    void setMetallic(Texture* texture) { m_metallic = texture; }

    /** @brief Asigna la textura que define que tan rugosa es la superficie. */
    void setRoughness(Texture* texture) { m_roughness = texture; }

    /** @brief Asigna la textura de occlusion ambiental. */
    void setAO(Texture* texture) { m_ao = texture; }

    /** @brief Asigna la textura de emision para objetos que brillan. */
    void setEmissive(Texture* texture) { m_emissive = texture; }

    /** @brief Regresa el material base. */
    Material* getMaterial() const { return m_material; }

    /** @brief Regresa la textura de albedo. */
    Texture* getAlbedo() const { return m_albedo; }

    /** @brief Regresa la textura de normales. */
    Texture* getNormal() const { return m_normal; }

    /** @brief Regresa la textura de metalico. */
    Texture* getMetallic() const { return m_metallic; }

    /** @brief Regresa la textura de rugosidad. */
    Texture* getRoughness() const { return m_roughness; }

    /** @brief Regresa la textura de occlusion ambiental. */
    Texture* getAO() const { return m_ao; }

    /** @brief Regresa la textura de emision. */
    Texture* getEmissive() const { return m_emissive; }

    /** @brief Regresa los parametros del material para modificarlos. */
    MaterialParams& getParams() { return m_params; }

    /** @brief Regresa los parametros del material en modo de solo lectura. */
    const MaterialParams& getParams() const { return m_params; }

    /** @brief Manda las texturas al shader para que se usen al renderizar. */
    void bindTextures(DeviceContext& deviceContext) const {}

private:

    // Material base que define el shader y los estados de renderizado
    Material* m_material = nullptr;

    // Texturas PBR del objeto
    Texture* m_albedo = nullptr; // Color base
    Texture* m_normal = nullptr; // Detalle de superficie
    Texture* m_metallic = nullptr; // Que tan metalico es
    Texture* m_roughness = nullptr; // Que tan rugoso es
    Texture* m_ao = nullptr; // Sombras de contacto
    Texture* m_emissive = nullptr; // Brillo propio

    // Parametros adicionales del material
    MaterialParams m_params;
};