#pragma once
#include "Prerequisites.h"

class Mesh;
class MaterialInstance;

/**
 * @brief Define si un material es opaco, enmascarado o transparente.
 */
enum class MaterialDomain {
    Opaque = 0, // Sin transparencia
    Masked,          // Transparencia por mascara (alpha cutoff)
    Transparent      // Transparencia real
};

/**
 * @brief Define como se mezcla el objeto con lo que esta detras de el.
 */
enum class BlendMode {
    Opaque = 0, // Sin mezcla
    Alpha,                // Mezcla por alpha normal
    Additive,             // Suma los colores (efectos de luz)
    PremultipliedAlpha    // Alpha premultiplicado
};

/**
 * @brief Indica en que paso del renderizado se va a dibujar un objeto.
 */
enum class RenderPassType {
    Shadow = 0, // Pass de sombras
    Opaque,          // Pass de objetos opacos
    Skybox,          // Pass del fondo de la escena
    Transparent,     // Pass de objetos transparentes
    Editor           // Pass del viewport del editor
};

/**
 * @brief Tipo de luz en la escena.
 */
enum class LightType {
    Directional = 0, // Luz tipo sol, sin posicion
    Point,           // Luz que ilumina en todas direcciones
    Spot             // Luz en forma de cono
};

/**
 * @brief Datos de una luz en la escena.
 */
struct LightData {
    LightType type = LightType::Directional;          // Tipo de luz
    EU::Vector3 color = EU::Vector3(1.0f, 1.0f, 1.0f);  // Color de la luz
    float intensity = 1.0f;                            // Que tan fuerte es la luz
    EU::Vector3 direction = EU::Vector3(0.0f, -1.0f, 0.0f); // Direccion (luces direccionales y spot)
    float range = 0.0f;                            // Alcance (luces point y spot)
    EU::Vector3 position = EU::Vector3(0.0f, 0.0f, 0.0f); // Posicion en la escena
    float spotAngle = 0.0f;                            // Angulo del cono (solo spot)
};

/**
 * @brief Parametros del material que se mandan al shader.
 */
struct MaterialParams {
    XMFLOAT4 baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // Color base
    float metallic = 1.0f; // Que tan metalico es el objeto
    float roughness = 1.0f; // Que tan rugosa es la superficie
    float ao = 1.0f; // Intensidad de la oclusion ambiental
    float normalScale = 1.0f; // Intensidad del mapa de normales
    float emissiveStrength = 1.0f; // Que tan fuerte es el brillo propio
    float alphaCutoff = 0.5f; // Umbral de corte para materiales enmascarados
};

/**
 * @brief Datos que se mandan a la GPU una vez por frame.
 *
 * Contiene las matrices de camara y la informacion de la luz principal.
 */
struct CBPerFrame {
    XMFLOAT4X4 View{};                                      // Matriz de vista de la camara
    XMFLOAT4X4 Projection{};                                // Matriz de proyeccion
    EU::Vector3 CameraPos{};                                // Posicion de la camara en el mundo
    float pad0 = 0.0f;
    EU::Vector3 LightDir = EU::Vector3(0.0f, -1.0f, 0.0f); // Direccion de la luz principal
    float pad1 = 0.0f;
    EU::Vector3 LightColor = EU::Vector3(1.0f, 1.0f, 1.0f); // Color de la luz principal
    float pad2 = 0.0f;
};

/**
 * @brief Datos que se mandan a la GPU una vez por objeto.
 */
struct CBPerObject {
    XMFLOAT4X4 World{}; // Matriz que posiciona al objeto en el mundo
};

/**
 * @brief Datos del material que se mandan a la GPU por cada objeto.
 */
struct CBPerMaterial {
    XMFLOAT4 BaseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // Color base
    float Metallic = 1.0f; // Metalico
    float Roughness = 1.0f; // Rugosidad
    float AO = 1.0f; // Oclusion ambiental
    float NormalScale = 1.0f; // Escala del mapa de normales
    float EmissiveStrength = 1.0f; // Fuerza del brillo propio
    float AlphaCutoff = 0.0f; // Corte de transparencia
    // Padding para alinear el buffer a 16 bytes (requerido por DirectX)
    float pad0 = 0.0f;
    float pad1 = 0.0f;
    float pad2 = 0.0f;
    float pad3 = 0.0f;
    float pad4 = 0.0f;
    float pad5 = 0.0f;
};

/**
 * @brief Representa un objeto listo para ser dibujado en la escena.
 */
struct RenderObject {
    Mesh* mesh = nullptr;                              // Geometria del objeto
    MaterialInstance* materialInstance = nullptr;      // Material principal
    std::vector<MaterialInstance*> materialInstances;  // Materiales por submesh
    XMMATRIX world = XMMatrixIdentity();               // Posicion y orientacion en el mundo
    bool castShadow = true;  // Si el objeto proyecta sombras
    bool transparent = false; // Si el objeto es transparente
    float distanceToCamera = 0.0f; // Distancia a la camara para ordenar el renderizado
};