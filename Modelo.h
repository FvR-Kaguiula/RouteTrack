#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// ============================================================================
// ENUMERACIONES DEL DOMINIO
// ============================================================================

enum class Zona { NORTE, SUR, CENTRO, ESTE };
enum class EstadoPedido { PENDIENTE, EN_CAMINO, ENTREGADO };
enum class EstadoRuta { CREADA, COMPLETADA };

// Conversión de enumeraciones a texto (para interfaz, CSV y reportes)
inline std::string toString(Zona zona) {
    switch (zona) {
    case Zona::NORTE: return "NORTE";
    case Zona::SUR: return "SUR";
    case Zona::CENTRO: return "CENTRO";
    case Zona::ESTE: return "ESTE";
    }
    return "CENTRO";
}

inline std::string toString(EstadoPedido estado) {
    switch (estado) {
    case EstadoPedido::PENDIENTE: return "PENDIENTE";
    case EstadoPedido::EN_CAMINO: return "EN_CAMINO";
    case EstadoPedido::ENTREGADO: return "ENTREGADO";
    }
    return "PENDIENTE";
}

inline std::string toString(EstadoRuta estado) {
    return estado == EstadoRuta::COMPLETADA ? "COMPLETADA" : "CREADA";
}

// Conversión de texto a enumeración (útil al leer archivos CSV y del teclado)
inline bool parseZona(const std::string& texto, Zona& zona) {
    std::string s = texto;
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (s == "NORTE") { zona = Zona::NORTE; return true; }
    if (s == "SUR") { zona = Zona::SUR; return true; }
    if (s == "CENTRO") { zona = Zona::CENTRO; return true; }
    if (s == "ESTE") { zona = Zona::ESTE; return true; }
    return false;
}

inline bool parseEstadoPedido(const std::string& texto, EstadoPedido& estado) {
    std::string s = texto;
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (s == "PENDIENTE") { estado = EstadoPedido::PENDIENTE; return true; }
    if (s == "EN_CAMINO") { estado = EstadoPedido::EN_CAMINO; return true; }
    if (s == "ENTREGADO") { estado = EstadoPedido::ENTREGADO; return true; }
    return false;
}

inline bool parseEstadoRuta(const std::string& texto, EstadoRuta& estado) {
    std::string s = texto;
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (s == "CREADA") { estado = EstadoRuta::CREADA; return true; }
    if (s == "COMPLETADA") { estado = EstadoRuta::COMPLETADA; return true; }
    return false;
}

// ============================================================================
// ESTRUCTURAS DEL DOMINIO (MODELO DE DATOS)
// ============================================================================

// Representa una orden de entrega solicitada por un cliente
struct Pedido {
    std::string codigo;          // Ej: PED-0001 (autogenerado secuencialmente)
    std::string cliente;         // Nombre o razón social
    std::string direccion;       // Dirección de entrega (soporta comas entre comillas)
    Zona zona{ Zona::CENTRO };     // Zona logística asignada
    EstadoPedido estado{ EstadoPedido::PENDIENTE }; // Flujo: PENDIENTE -> EN_CAMINO -> ENTREGADO
    std::string codigoConductor; // Conductor que realiza el despacho
    std::string codigoRuta;      // Código de ruta en la que fue despachado
    std::string fechaRegistro;   // Fecha y hora de creación
    std::string fechaDespacho;   // Fecha y hora en que pasó a EN_CAMINO
    std::string fechaEntrega;    // Fecha y hora en que pasó a ENTREGADO
};

// Representa a un repartidor registrado en el sistema
struct Conductor {
    std::string codigo;          // Ej: CON-0001
    std::string nombre;          // Nombre completo
    Zona zonaAsignada{ Zona::CENTRO }; // Zona en la que opera exclusivamente
    bool activo{ true };           // Estado operativo
};

// Representa un lote de despacho por zona
struct RutaEntrega {
    std::string codigo;          // Ej: RUT-0001
    Zona zona{ Zona::CENTRO };     // Zona de cobertura de la ruta
    std::string codigoConductor; // Conductor responsable (debe coincidir con la zona)
    std::string fechaCreacion;   // Fecha de despacho
    EstadoRuta estado{ EstadoRuta::CREADA }; // CREADA -> COMPLETADA
    std::vector<std::string> codigosPedidos; // Lista de pedidos incluidos
};

// Registro de trazabilidad y auditoría de cambios de estado de cada pedido
struct EventoPedido {
    std::string codigoPedido;    // Pedido afectado
    std::string fecha;           // Timestamp del cambio
    std::string estadoAnterior;  // Estado previo
    std::string estadoNuevo;     // Nuevo estado
    std::string descripcion;     // Motivo o detalle de la operación
};

