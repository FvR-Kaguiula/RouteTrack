#include "Controlador.h"

#include <iostream>
#include <limits>
#include <optional>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================================
// FUNCIONES AUXILIARES DE ENTRADA Y FORMATEO POR CONSOLA (VISTA)
// ============================================================================
static std::string leerTexto(const std::string& mensaje) {
    std::cout << mensaje;
    std::string s;
    std::getline(std::cin, s);
    // Eliminar espacios iniciales y finales
    auto ini = s.find_first_not_of(" \t\r\n");
    if (ini == std::string::npos) return "";
    auto fin = s.find_last_not_of(" \t\r\n");
    return s.substr(ini, fin - ini + 1);
}

static int leerEntero(const std::string& mensaje, int min, int max) {
    while (true) {
        std::string txt = leerTexto(mensaje);
        try {
            std::size_t pos = 0;
            int v = std::stoi(txt, &pos);
            if (pos == txt.size() && v >= min && v <= max) return v;
        }
        catch (...) {}
        std::cout << "Entrada invalida. Ingrese un numero entre " << min << " y " << max << ".\n";
    }
}

static std::optional<Zona> seleccionarZona() {
    std::cout << "1. NORTE | 2. SUR | 3. CENTRO | 4. ESTE | 0. Cancelar\n";
    int op = leerEntero("Seleccione zona: ", 0, 4);
    switch (op) {
    case 1: return Zona::NORTE;
    case 2: return Zona::SUR;
    case 3: return Zona::CENTRO;
    case 4: return Zona::ESTE;
    default: return std::nullopt;
    }
}

static void imprimirPedido(const Pedido& p) {
    std::cout << "[" << p.codigo << "] Cliente: " << p.cliente
        << " | Dir: " << p.direccion
        << " | Zona: " << toString(p.zona)
        << " | Estado: " << toString(p.estado);
    if (!p.codigoConductor.empty()) std::cout << " | Cond: " << p.codigoConductor;
    if (!p.codigoRuta.empty()) std::cout << " | Ruta: " << p.codigoRuta;
    std::cout << "\n";
}

static void mostrarMenu() {
    std::cout << "\n================ ROUTETRACK (LOGISTICA) ================\n"
        << " 1. Registrar pedido\n"
        << " 2. Listar pedidos\n"
        << " 3. Buscar pedido por codigo\n"
        << " 4. Actualizar pedido (Solo si esta PENDIENTE)\n"
        << " 5. Eliminar pedido (Solo si esta PENDIENTE)\n"
        << " 6. Registrar conductor\n"
        << " 7. Listar conductores\n"
        << " 8. Crear ruta por zona y asignar pedidos\n"
        << " 9. Consultar ruta y pedidos asignados\n"
        << "10. Marcar pedido como entregado\n"
        << "11. Consultar pedidos con filtros (zona / estado / conductor)\n"
        << "12. Generar reportes CSV\n"
        << "13. Exportar reportes CSV a HTML con estilos\n"
        << "14. Salir\n"
        << "========================================================\n";
}

// ============================================================================
// PUNTO DE ENTRADA PRINCIPAL (MAIN)
// ============================================================================
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try {
#ifdef ROUTETRACK_ROOT
        const std::filesystem::path rutaRaiz = ROUTETRACK_ROOT;
#else
        const std::filesystem::path rutaRaiz = std::filesystem::current_path();
#endif
        Controlador sistema(rutaRaiz);

        while (true) {
            mostrarMenu();
            int op = leerEntero("Seleccione una opcion [1-14]: ", 1, 14);

            if (op == 14) {
                std::cout << "Guardando datos y saliendo... Hasta luego.\n";
                break;
            }

            switch (op) {
            case 1: { // Registrar pedido
                std::string cliente = leerTexto("Nombre del cliente: ");
                std::string direccion = leerTexto("Direccion de entrega: ");
                auto zona = seleccionarZona();
                if (!zona) break;
                std::string error;
                std::string cod = sistema.registrarPedido(cliente, direccion, *zona, error);
                if (cod.empty()) std::cout << "Error: " << error << "\n";
                else std::cout << "Pedido registrado exitosamente con codigo " << cod << ".\n";
                break;
            }
            case 2: { // Listar pedidos
                const auto& lista = sistema.getPedidos();
                if (lista.empty()) {
                    std::cout << "No hay pedidos registrados en el sistema.\n";
                }
                else {
                    std::cout << "\n--- LISTADO GENERAL DE PEDIDOS ---\n";
                    for (const auto& p : lista) imprimirPedido(p);
                }
                break;
            }
            case 3: { // Buscar pedido
                std::string cod = leerTexto("Ingrese codigo de pedido (ej. PED-0001): ");
                const auto* p = sistema.buscarPedido(cod);
                if (!p) std::cout << "Pedido no encontrado.\n";
                else imprimirPedido(*p);
                break;
            }
            case 4: { // Actualizar pedido
                std::string cod = leerTexto("Codigo del pedido a modificar: ");
                const auto* actual = sistema.buscarPedido(cod);
                if (!actual) {
                    std::cout << "Pedido no encontrado.\n";
                    break;
                }
                imprimirPedido(*actual);
                std::string cliente = leerTexto("Nuevo cliente: ");
                std::string direccion = leerTexto("Nueva direccion: ");
                auto zona = seleccionarZona();
                if (!zona) break;
                std::string error;
                if (sistema.actualizarPedido(cod, cliente, direccion, *zona, error))
                    std::cout << "Pedido actualizado con exito.\n";
                else std::cout << "Error: " << error << "\n";
                break;
            }
            case 5: { // Eliminar pedido
                std::string cod = leerTexto("Codigo del pedido a eliminar: ");
                std::string conf = leerTexto("Confirma eliminar escribiendo 'SI': ");
                if (conf != "SI" && conf != "si") {
                    std::cout << "Operacion cancelada.\n";
                    break;
                }
                std::string error;
                if (sistema.eliminarPedido(cod, error))
                    std::cout << "Pedido " << cod << " eliminado satisfactoriamente.\n";
                else std::cout << "Error: " << error << "\n";
                break;
            }
            case 6: { // Registrar conductor
                std::string nombre = leerTexto("Nombre del conductor: ");
                auto zona = seleccionarZona();
                if (!zona) break;
                std::string error;
                std::string cod = sistema.registrarConductor(nombre, *zona, error);
                if (cod.empty()) std::cout << "Error: " << error << "\n";
                else std::cout << "Conductor registrado con codigo " << cod << ".\n";
                break;
            }
            case 7: { // Listar conductores
                const auto& lista = sistema.getConductores();
                if (lista.empty()) {
                    std::cout << "No hay conductores registrados.\n";
                }
                else {
                    std::cout << "\n--- LISTADO DE CONDUCTORES ---\n";
                    for (const auto& c : lista) {
                        std::cout << "[" << c.codigo << "] " << c.nombre
                            << " | Zona: " << toString(c.zonaAsignada)
                            << " | Estado: " << (c.activo ? "ACTIVO" : "INACTIVO") << "\n";
                    }
                }
                break;
            }
            case 8: { // Crear ruta
                auto zona = seleccionarZona();
                if (!zona) break;
                auto disponibles = sistema.listarConductoresActivosPorZona(*zona);
                if (disponibles.empty()) {
                    std::cout << "No hay conductores activos asignados a esa zona.\n";
                    break;
                }
                std::cout << "Conductores disponibles para zona " << toString(*zona) << ":\n";
                for (const auto* c : disponibles) {
                    std::cout << "  - " << c->codigo << " : " << c->nombre << "\n";
                }
                std::string codCond = leerTexto("Ingrese el codigo del conductor: ");
                std::string error;
                std::string codRuta = sistema.crearRuta(*zona, codCond, error);
                if (codRuta.empty()) std::cout << "Error: " << error << "\n";
                else std::cout << "Ruta " << codRuta << " creada. Pedidos asignados y puestos EN_CAMINO.\n";
                break;
            }
            case 9: { // Consultar ruta
                std::string cod = leerTexto("Codigo de ruta (ej. RUT-0001): ");
                const auto* r = sistema.buscarRuta(cod);
                if (!r) {
                    std::cout << "Ruta no encontrada.\n";
                    break;
                }
                std::cout << "\n[" << r->codigo << "] Zona: " << toString(r->zona)
                    << " | Conductor: " << r->codigoConductor
                    << " | Estado: " << toString(r->estado)
                    << " | Fecha: " << r->fechaCreacion << "\n";
                std::cout << "Pedidos en esta ruta:\n";
                auto peds = sistema.obtenerPedidosDeRuta(r->codigo);
                for (const auto* p : peds) imprimirPedido(*p);
                break;
            }
            case 10: { // Marcar entregado
                std::string cod = leerTexto("Codigo del pedido a entregar: ");
                std::string error;
                if (sistema.marcarEntregado(cod, error))
                    std::cout << "Entrega confirmada para el pedido " << cod << ".\n";
                else std::cout << "Error: " << error << "\n";
                break;
            }
            case 11: { // Consultas filtradas
                std::cout << "1. Filtrar por Zona\n2. Filtrar por Estado\n3. Filtrar por Conductor\n0. Cancelar\n";
                int f = leerEntero("Opcion: ", 0, 3);
                if (f == 0) break;
                std::vector<const Pedido*> resultados;
                if (f == 1) {
                    auto z = seleccionarZona();
                    if (!z) break;
                    resultados = sistema.filtrarPedidosPorZona(*z);
                }
                else if (f == 2) {
                    std::cout << "1. PENDIENTE | 2. EN_CAMINO | 3. ENTREGADO\n";
                    int estOp = leerEntero("Estado: ", 1, 3);
                    EstadoPedido est = (estOp == 1 ? EstadoPedido::PENDIENTE :
                        (estOp == 2 ? EstadoPedido::EN_CAMINO : EstadoPedido::ENTREGADO));
                    resultados = sistema.filtrarPedidosPorEstado(est);
                }
                else if (f == 3) {
                    std::string cCond = leerTexto("Codigo del conductor: ");
                    resultados = sistema.filtrarPedidosPorConductor(cCond);
                }
                if (resultados.empty()) {
                    std::cout << "No se encontraron pedidos con el criterio seleccionado.\n";
                }
                else {
                    std::cout << "Resultados encontrados (" << resultados.size() << "):\n";
                    for (const auto* p : resultados) imprimirPedido(*p);
                }
                break;
            }
            case 12: { // Generar reportes CSV
                std::string error;
                if (sistema.generarReportesCsv(error))
                    std::cout << "Reportes CSV generados con exito en 'reportes/csv/'.\n";
                else std::cout << "Error: " << error << "\n";
                break;
            }
            case 13: { // Exportar CSV a HTML
                std::string error;
                if (sistema.exportarReportesHtml(error))
                    std::cout << "Reportes HTML generados. Puede abrir 'reportes/html/index.html' en su navegador.\n";
                else std::cout << "Error: " << error << "\n";
                break;
            }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error critico en la ejecucion: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

