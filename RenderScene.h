#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Skybox;

/**
 * @brief Contiene todos los datos de la escena que se van a renderizar en un frame.
 *
 * Guarda los objetos, luces y el skybox separados para que el renderer
 * los procese en el orden correcto.
 */
class RenderScene {
public:

    /** @brief Limpia todas las listas de la escena para el siguiente frame. */
    void clear();

public:

    // Objetos de la escena separados por tipo de renderizado
    std::vector<RenderObject> opaqueObjects;      // Objetos sin transparencia
    std::vector<RenderObject> transparentObjects; // Objetos con transparencia

    // Luces de la escena
    std::vector<LightData> directionalLights; // Luces direccionales tipo sol

    // Skybox que se dibuja como fondo de la escena
    Skybox* skybox = nullptr;
};