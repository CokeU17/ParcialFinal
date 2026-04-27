#pragma once
#include "Prerequisites.h"
#include "Buffer.h"

/**
 * @brief Representa una parte del mesh con su propio buffer y material.
 */
struct Submesh {
    Buffer vertexBuffer;           // Buffer con los vertices de esta parte
    Buffer indexBuffer;            // Buffer con los indices de esta parte
    unsigned int indexCount = 0; // Cantidad de indices a dibujar
    unsigned int startIndex = 0; // Desde que indice empieza a dibujar
    unsigned int materialSlot = 0; // Que material le corresponde
};

/**
 * @brief Clase que representa un modelo 3D dividido en submeshes.
 *
 * Un mesh puede tener varias partes, cada una con su propio
 * material y buffers de geometria.
 */
class Mesh {
public:

    /** @brief Regresa la lista de submeshes para modificarla. */
    std::vector<Submesh>& getSubmeshes() { return m_submeshes; }

    /** @brief Regresa la lista de submeshes en modo de solo lectura. */
    const std::vector<Submesh>& getSubmeshes() const { return m_submeshes; }

    /**
     * @brief Libera los buffers de todos los submeshes y limpia la lista.
     */
    void destroy() {
        for (Submesh& submesh : m_submeshes) {
            submesh.vertexBuffer.destroy();
            submesh.indexBuffer.destroy();
        }
        m_submeshes.clear();
    }

private:

    // Lista de partes que forman el modelo completo
    std::vector<Submesh> m_submeshes;
};