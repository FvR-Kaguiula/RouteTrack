#pragma once

#include "Modelo.h"

#include <filesystem>
#include <string>
#include <vector>

// ============================================================================
// CONTROLADOR DEL SISTEMA LOGÍSTICO (ROUTE TRACK)
// Centraliza la lógica de negocio, las colecciones en memoria (std::vector),
// la persistencia en archivos CSV, la bitácora TXT y la generación de reportes.
// ============================================================================
class Controlador {
public:
    explicit Controlador(std::filesystem::path raiz = std::filesystem::current_path());

    // Inicialización y persistencia
    void inicializarDirectorios() const;
    bool cargarDatos();
    bool guardarDatos(std::string& error) const;

    // --- GESTIÓN DE PEDIDOS (CRUD) ---
    std::string registrarPedido(const std::string& cliente, const std::string& direccion,
        Zona zona, std::string& error);
    const Pedido* buscarPedido(const std::string& codigo) const;
    bool actualizarPedido(const std::string& codigo, const std::string& cliente,
        const std::string& direccion, Zona zona, std::string& error);
    bool eliminarPedido(const std::string& codigo, std::string& error);
    const std::vector<Pedido>& getPedidos() const { return pedidos_; }

    // --- GESTIÓN DE CONDUCTORES ---
    std::string registrarConductor(const std::string& nombre, Zona zona, std::string& error);
    const Conductor* buscarConductor(const std::string& codigo) const;
    std::vector<const Conductor*> listarConductoresActivosPorZona(Zona zona) const;
    const std::vector<Conductor>& getConductores() const { return conductores_; }

    // --- GESTIÓN DE RUTAS Y DESPACHO ---
    std::string crearRuta(Zona zona, const std::string& codigoConductor, std::string& error);
    bool marcarEntregado(const std::string& codigoPedido, std::string& error);
    const RutaEntrega* buscarRuta(const std::string& codigo) const;
    std::vector<const Pedido*> obtenerPedidosDeRuta(const std::string& codigoRuta) const;
    const std::vector<RutaEntrega>& getRutas() const { return rutas_; }

    // --- CONSULTAS Y BÚSQUEDAS FILTRADAS ---
    std::vector<const Pedido*> filtrarPedidosPorZona(Zona zona) const;
    std::vector<const Pedido*> filtrarPedidosPorEstado(EstadoPedido estado) const;
    std::vector<const Pedido*> filtrarPedidosPorConductor(const std::string& codigoConductor) const;

    // --- AUDITORÍA Y REPORTES ---
    bool registrarBitacora(const std::string& accion, const std::string& codigo,
        const std::string& descripcion) const;
    bool generarReportesCsv(std::string& error) const;
    bool exportarReportesHtml(std::string& error) const;

    // Directorio base del proyecto
    const std::filesystem::path& getRaiz() const { return raiz_; }

    // Utilidades públicas para manejo de cadenas y CSV
    static std::string escaparCsv(const std::string& valor);
    static std::vector<std::string> separarCsv(const std::string& linea);

private:
    std::filesystem::path raiz_;

    // Colecciones dinámicas en memoria principal (std::vector)
    std::vector<Pedido> pedidos_;
    std::vector<Conductor> conductores_;
    std::vector<RutaEntrega> rutas_;
    std::vector<EventoPedido> eventos_;

    // Métodos auxiliares internos
    Pedido* buscarPedidoInterno(const std::string& codigo);
    Conductor* buscarConductorInterno(const std::string& codigo);
    RutaEntrega* buscarRutaInterna(const std::string& codigo);
    void verificarEstadoRuta(RutaEntrega& ruta);

    std::string generarSiguienteCodigo(const std::string& prefijo,
        const std::vector<std::string>& codigosExistentes) const;
    static std::string fechaHoraActual();
    static std::string recortar(const std::string& texto);
    static std::string aMayusculas(std::string texto);
    static std::string escaparHtml(const std::string& texto);

    // Helpers de persistencia
    bool guardarPedidosCsv(std::string& error) const;
    bool guardarConductoresCsv(std::string& error) const;
    bool guardarRutasCsv(std::string& error) const;
    bool guardarEventosCsv(std::string& error) const;

    bool exportarArchivoHtml(const std::filesystem::path& origenCsv,
        const std::filesystem::path& destinoHtml,
        std::string& error) const;
};

