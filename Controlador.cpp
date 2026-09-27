#include "Controlador.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================
Controlador::Controlador(std::filesystem::path raiz) : raiz_(std::move(raiz)) {
    inicializarDirectorios();
    cargarDatos();
}

void Controlador::inicializarDirectorios() const {
    std::filesystem::create_directories(raiz_ / "data");
    std::filesystem::create_directories(raiz_ / "logs");
    std::filesystem::create_directories(raiz_ / "reportes" / "csv");
    std::filesystem::create_directories(raiz_ / "reportes" / "html");
}

// ============================================================================
// UTILIDADES ESTÁTICAS (Cadena, Fecha y Formateo)
// ============================================================================
std::string Controlador::recortar(const std::string& texto) {
    const auto inicio = texto.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) return "";
    const auto fin = texto.find_last_not_of(" \t\r\n");
    return texto.substr(inicio, fin - inicio + 1);
}

std::string Controlador::aMayusculas(std::string texto) {
    std::transform(texto.begin(), texto.end(), texto.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return texto;
}

std::string Controlador::fechaHoraActual() {
    const auto ahora = std::chrono::system_clock::now();
    const std::time_t tiempo = std::chrono::system_clock::to_time_t(ahora);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &tiempo);
#else
    localtime_r(&tiempo, &local);
#endif
    std::ostringstream ss;
    ss << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Genera identificadores correlativos con formato PREFIJO-0001
std::string Controlador::generarSiguienteCodigo(const std::string& prefijo,
    const std::vector<std::string>& codigosExistentes) const {
    int maximo = 0;
    const std::string marcador = prefijo + "-";
    for (const auto& cod : codigosExistentes) {
        if (cod.rfind(marcador, 0) == 0) { // Si inicia con "PREFIJO-"
            try {
                int num = std::stoi(cod.substr(marcador.size()));
                if (num > maximo) maximo = num;
            }
            catch (...) {
                // Se ignoran códigos con formato no numérico
            }
        }
    }
    std::ostringstream ss;
    ss << prefijo << "-" << std::setw(4) << std::setfill('0') << (maximo + 1);
    return ss.str();
}

// ============================================================================
// PARSEO Y ESCAPADO DE CSV (Estándar RFC 4180)
// Permite guardar campos que contienen comas, comillas dobles y saltos de línea.
// ============================================================================
std::string Controlador::escaparCsv(const std::string& valor) {
    if (valor.find_first_of(",\"\r\n") == std::string::npos) return valor;
    std::string resultado = "\"";
    for (char c : valor) {
        if (c == '"') resultado += "\"\""; // Se duplican las comillas internas
        else resultado += c;
    }
    resultado += '"';
    return resultado;
}

std::vector<std::string> Controlador::separarCsv(const std::string& linea) {
    std::vector<std::string> campos;
    std::string actual;
    bool entreComillas = false;

    for (std::size_t i = 0; i < linea.size(); ++i) {
        char c = linea[i];
        if (c == '"') {
            if (entreComillas && i + 1 < linea.size() && linea[i + 1] == '"') {
                actual += '"';
                ++i; // Saltar la comilla escapada
            }
            else {
                entreComillas = !entreComillas;
            }
        }
        else if (c == ',' && !entreComillas) {
            campos.push_back(actual);
            actual.clear();
        }
        else {
            actual += c;
        }
    }
    campos.push_back(actual);
    return campos;
}

std::string Controlador::escaparHtml(const std::string& texto) {
    std::string res;
    for (char c : texto) {
        switch (c) {
        case '&': res += "&amp;"; break;
        case '<': res += "&lt;"; break;
        case '>': res += "&gt;"; break;
        case '"': res += "&quot;"; break;
        case '\'': res += "&#39;"; break;
        default: res += c;
        }
    }
    return res;
}

// ============================================================================
// AUDITORÍA (BITÁCORA TXT)
// ============================================================================
bool Controlador::registrarBitacora(const std::string& accion, const std::string& codigo,
    const std::string& descripcion) const {
    const auto archivo = raiz_ / "logs" / "bitacora.txt";
    std::ofstream salida(archivo, std::ios::binary | std::ios::app);
    if (!salida) return false;
    salida << fechaHoraActual() << " | " << accion << " | " << codigo << " | " << descripcion << "\n";
    return true;
}

// ============================================================================
// BÚSQUEDAS INTERNAS
// ============================================================================
Pedido* Controlador::buscarPedidoInterno(const std::string& codigo) {
    const auto norm = aMayusculas(recortar(codigo));
    auto it = std::find_if(pedidos_.begin(), pedidos_.end(), [&](const Pedido& p) {
        return aMayusculas(p.codigo) == norm;
        });
    return it != pedidos_.end() ? &(*it) : nullptr;
}

Conductor* Controlador::buscarConductorInterno(const std::string& codigo) {
    const auto norm = aMayusculas(recortar(codigo));
    auto it = std::find_if(conductores_.begin(), conductores_.end(), [&](const Conductor& c) {
        return aMayusculas(c.codigo) == norm;
        });
    return it != conductores_.end() ? &(*it) : nullptr;
}

RutaEntrega* Controlador::buscarRutaInterna(const std::string& codigo) {
    const auto norm = aMayusculas(recortar(codigo));
    auto it = std::find_if(rutas_.begin(), rutas_.end(), [&](const RutaEntrega& r) {
        return aMayusculas(r.codigo) == norm;
        });
    return it != rutas_.end() ? &(*it) : nullptr;
}

// ============================================================================
// GESTIÓN DE PEDIDOS (CRUD)
// ============================================================================
std::string Controlador::registrarPedido(const std::string& cliente, const std::string& direccion,
    Zona zona, std::string& error) {
    const auto cLimpio = recortar(cliente);
    const auto dLimpia = recortar(direccion);
    if (cLimpio.empty() || dLimpia.empty()) {
        error = "El cliente y la dirección son obligatorios.";
        return "";
    }

    // Para evitar reutilizar códigos si hubo eliminaciones, recopilamos códigos históricos
    std::vector<std::string> codigos;
    for (const auto& p : pedidos_) codigos.push_back(p.codigo);
    for (const auto& e : eventos_) codigos.push_back(e.codigoPedido);

    Pedido pedido;
    pedido.codigo = generarSiguienteCodigo("PED", codigos);
    pedido.cliente = cLimpio;
    pedido.direccion = dLimpia;
    pedido.zona = zona;
    pedido.estado = EstadoPedido::PENDIENTE;
    pedido.fechaRegistro = fechaHoraActual();

    pedidos_.push_back(pedido);
    eventos_.push_back({ pedido.codigo, pedido.fechaRegistro, "", "PENDIENTE", "Pedido registrado" });

    if (!guardarPedidosCsv(error) || !guardarEventosCsv(error)) {
        pedidos_.pop_back();
        eventos_.pop_back();
        return "";
    }

    registrarBitacora("REGISTRAR_PEDIDO", pedido.codigo, "Pedido creado en zona " + toString(zona));
    return pedido.codigo;
}

const Pedido* Controlador::buscarPedido(const std::string& codigo) const {
    const auto norm = aMayusculas(recortar(codigo));
    auto it = std::find_if(pedidos_.cbegin(), pedidos_.cend(), [&](const Pedido& p) {
        return aMayusculas(p.codigo) == norm;
        });
    return it != pedidos_.cend() ? &(*it) : nullptr;
}

bool Controlador::actualizarPedido(const std::string& codigo, const std::string& cliente,
    const std::string& direccion, Zona zona, std::string& error) {
    Pedido* p = buscarPedidoInterno(codigo);
    if (!p) {
        error = "Pedido no encontrado.";
        return false;
    }
    // Regla de negocio: Solo los pedidos pendientes son editables
    if (p->estado != EstadoPedido::PENDIENTE) {
        error = "Solo se pueden modificar pedidos pendientes.";
        return false;
    }

    const auto cLimpio = recortar(cliente);
    const auto dLimpia = recortar(direccion);
    if (cLimpio.empty() || dLimpia.empty()) {
        error = "El cliente y la dirección son obligatorios.";
        return false;
    }

    p->cliente = cLimpio;
    p->direccion = dLimpia;
    p->zona = zona;

    if (!guardarPedidosCsv(error)) return false;

    registrarBitacora("ACTUALIZAR_PEDIDO", p->codigo, "Datos descriptivos actualizados");
    return true;
}

bool Controlador::eliminarPedido(const std::string& codigo, std::string& error) {
    const auto norm = aMayusculas(recortar(codigo));
    auto it = std::find_if(pedidos_.begin(), pedidos_.end(), [&](const Pedido& p) {
        return aMayusculas(p.codigo) == norm;
        });
    if (it == pedidos_.end()) {
        error = "Pedido no encontrado.";
        return false;
    }
    // Regla de negocio: Solo se pueden eliminar pedidos en estado PENDIENTE
    if (it->estado != EstadoPedido::PENDIENTE) {
        error = "Solo se pueden eliminar pedidos pendientes.";
        return false;
    }

    const std::string cod = it->codigo;
    eventos_.push_back({ cod, fechaHoraActual(), "PENDIENTE", "", "Pedido eliminado" });
    pedidos_.erase(it);

    if (!guardarPedidosCsv(error) || !guardarEventosCsv(error)) {
        error = "Error al persistir la eliminación: " + error;
        return false;
    }

    registrarBitacora("ELIMINAR_PEDIDO", cod, "Pedido pendiente eliminado");
    return true;
}

// ============================================================================
// GESTIÓN DE CONDUCTORES
// ============================================================================
std::string Controlador::registrarConductor(const std::string& nombre, Zona zona, std::string& error) {
    const auto nLimpio = recortar(nombre);
    if (nLimpio.empty()) {
        error = "El nombre del conductor es obligatorio.";
        return "";
    }

    std::vector<std::string> codigos;
    for (const auto& c : conductores_) codigos.push_back(c.codigo);

    Conductor conductor;
    conductor.codigo = generarSiguienteCodigo("CON", codigos);
    conductor.nombre = nLimpio;
    conductor.zonaAsignada = zona;
    conductor.activo = true;

    conductores_.push_back(conductor);
    if (!guardarConductoresCsv(error)) {
        conductores_.pop_back();
        return "";
    }

    registrarBitacora("REGISTRAR_CONDUCTOR", conductor.codigo, "Conductor asignado a zona " + toString(zona));
    return conductor.codigo;
}

const Conductor* Controlador::buscarConductor(const std::string& codigo) const {
    const auto norm = aMayusculas(recortar(codigo));
    auto it = std::find_if(conductores_.cbegin(), conductores_.cend(), [&](const Conductor& c) {
        return aMayusculas(c.codigo) == norm;
        });
    return it != conductores_.cend() ? &(*it) : nullptr;
}

std::vector<const Conductor*> Controlador::listarConductoresActivosPorZona(Zona zona) const {
    std::vector<const Conductor*> res;
    for (const auto& c : conductores_) {
        if (c.activo && c.zonaAsignada == zona) res.push_back(&c);
    }
    return res;
}

// ============================================================================
// GESTIÓN DE RUTAS Y DESPACHOS
// ============================================================================
std::string Controlador::crearRuta(Zona zona, const std::string& codigoConductor, std::string& error) {
    Conductor* conductor = buscarConductorInterno(codigoConductor);
    if (!conductor || !conductor->activo) {
        error = "El conductor no existe o está inactivo.";
        return "";
    }
    if (conductor->zonaAsignada != zona) {
        error = "El conductor no pertenece a la zona seleccionada.";
        return "";
    }

    // Seleccionar todos los pedidos pendientes de la zona
    std::vector<Pedido*> pendientes;
    for (auto& p : pedidos_) {
        if (p.zona == zona && p.estado == EstadoPedido::PENDIENTE) {
            pendientes.push_back(&p);
        }
    }
    if (pendientes.empty()) {
        error = "No hay pedidos pendientes en la zona seleccionada.";
        return "";
    }

    std::vector<std::string> codigosRutas;
    for (const auto& r : rutas_) codigosRutas.push_back(r.codigo);

    RutaEntrega ruta;
    ruta.codigo = generarSiguienteCodigo("RUT", codigosRutas);
    ruta.zona = zona;
    ruta.codigoConductor = conductor->codigo;
    ruta.fechaCreacion = fechaHoraActual();
    ruta.estado = EstadoRuta::CREADA;

    for (auto* p : pendientes) {
        p->estado = EstadoPedido::EN_CAMINO;
        p->codigoConductor = conductor->codigo;
        p->codigoRuta = ruta.codigo;
        p->fechaDespacho = ruta.fechaCreacion;
        ruta.codigosPedidos.push_back(p->codigo);

        eventos_.push_back({ p->codigo, ruta.fechaCreacion, "PENDIENTE", "EN_CAMINO",
                            "Asignado a " + ruta.codigo + " con " + conductor->codigo });
    }

    rutas_.push_back(ruta);

    // Persistir todos los cambios en lote
    if (!guardarPedidosCsv(error) || !guardarRutasCsv(error) || !guardarEventosCsv(error)) {
        return "";
    }

    registrarBitacora("CREAR_RUTA", ruta.codigo,
        "Zona " + toString(zona) + ", conductor " + conductor->codigo +
        ", pedidos: " + std::to_string(ruta.codigosPedidos.size()));
    return ruta.codigo;
}

bool Controlador::marcarEntregado(const std::string& codigoPedido, std::string& error) {
    Pedido* p = buscarPedidoInterno(codigoPedido);
    if (!p) {
        error = "Pedido no encontrado.";
        return false;
    }
    if (p->estado != EstadoPedido::EN_CAMINO) {
        error = "Solo un pedido EN_CAMINO puede marcarse como ENTREGADO.";
        return false;
    }

    p->estado = EstadoPedido::ENTREGADO;
    p->fechaEntrega = fechaHoraActual();
    eventos_.push_back({ p->codigo, p->fechaEntrega, "EN_CAMINO", "ENTREGADO", "Entrega confirmada" });

    // Si la ruta a la que pertenece ya tiene todos sus pedidos entregados, se completa
    RutaEntrega* r = buscarRutaInterna(p->codigoRuta);
    if (r) verificarEstadoRuta(*r);

    if (!guardarPedidosCsv(error) || !guardarRutasCsv(error) || !guardarEventosCsv(error)) {
        return false;
    }

    registrarBitacora("ENTREGAR_PEDIDO", p->codigo, "Entrega confirmada en ruta " + p->codigoRuta);
    return true;
}

void Controlador::verificarEstadoRuta(RutaEntrega& ruta) {
    bool todosEntregados = true;
    for (const auto& codPedido : ruta.codigosPedidos) {
        const auto* p = buscarPedido(codPedido);
        if (!p || p->estado != EstadoPedido::ENTREGADO) {
            todosEntregados = false;
            break;
        }
    }
    if (todosEntregados) {
        ruta.estado = EstadoRuta::COMPLETADA;
    }
}

const RutaEntrega* Controlador::buscarRuta(const std::string& codigo) const {
    const auto norm = aMayusculas(recortar(codigo));
    auto it = std::find_if(rutas_.cbegin(), rutas_.cend(), [&](const RutaEntrega& r) {
        return aMayusculas(r.codigo) == norm;
        });
    return it != rutas_.cend() ? &(*it) : nullptr;
}

std::vector<const Pedido*> Controlador::obtenerPedidosDeRuta(const std::string& codigoRuta) const {
    std::vector<const Pedido*> res;
    const auto* r = buscarRuta(codigoRuta);
    if (!r) return res;
    for (const auto& codPedido : r->codigosPedidos) {
        const auto* p = buscarPedido(codPedido);
        if (p) res.push_back(p);
    }
    return res;
}

// ============================================================================
// CONSULTAS FILTRADAS
// ============================================================================
std::vector<const Pedido*> Controlador::filtrarPedidosPorZona(Zona zona) const {
    std::vector<const Pedido*> res;
    for (const auto& p : pedidos_) {
        if (p.zona == zona) res.push_back(&p);
    }
    return res;
}

std::vector<const Pedido*> Controlador::filtrarPedidosPorEstado(EstadoPedido estado) const {
    std::vector<const Pedido*> res;
    for (const auto& p : pedidos_) {
        if (p.estado == estado) res.push_back(&p);
    }
    return res;
}

std::vector<const Pedido*> Controlador::filtrarPedidosPorConductor(const std::string& codigoConductor) const {
    std::vector<const Pedido*> res;
    const auto norm = aMayusculas(recortar(codigoConductor));
    for (const auto& p : pedidos_) {
        if (aMayusculas(p.codigoConductor) == norm) res.push_back(&p);
    }
    return res;
}

// ============================================================================
// PERSISTENCIA CSV (Carga y Guardado)
// ============================================================================
namespace {
    std::vector<std::vector<std::string>> leerFilasCsv(const std::filesystem::path& rutaArchivo) {
        std::vector<std::vector<std::string>> filas;
        std::ifstream archivo(rutaArchivo, std::ios::binary);
        if (!archivo) return filas;

        std::string linea;
        bool primera = true;
        while (std::getline(archivo, linea)) {
            if (!linea.empty() && linea.back() == '\r') linea.pop_back();
            if (primera) {
                primera = false;
                // Remover BOM UTF-8 si existe (0xEF, 0xBB, 0xBF)
                if (linea.size() >= 3 &&
                    static_cast<unsigned char>(linea[0]) == 0xEF &&
                    static_cast<unsigned char>(linea[1]) == 0xBB &&
                    static_cast<unsigned char>(linea[2]) == 0xBF) {
                    linea.erase(0, 3);
                }
                continue; // Saltar encabezados
            }
            if (!linea.empty()) {
                filas.push_back(Controlador::separarCsv(linea));
            }
        }
        return filas;
    }
}

bool Controlador::cargarDatos() {
    const auto rutaData = raiz_ / "data";

    // 1. Pedidos
    pedidos_.clear();
    for (const auto& fila : leerFilasCsv(rutaData / "pedidos.csv")) {
        if (fila.size() < 10) continue;
        Zona z;
        EstadoPedido ep;
        if (!parseZona(fila[3], z) || !parseEstadoPedido(fila[4], ep)) continue;
        Pedido p;
        p.codigo = fila[0];
        p.cliente = fila[1];
        p.direccion = fila[2];
        p.zona = z;
        p.estado = ep;
        p.codigoConductor = fila[5];
        p.codigoRuta = fila[6];
        p.fechaRegistro = fila[7];
        p.fechaDespacho = fila[8];
        p.fechaEntrega = fila[9];
        pedidos_.push_back(std::move(p));
    }

    // 2. Conductores
    conductores_.clear();
    for (const auto& fila : leerFilasCsv(rutaData / "conductores.csv")) {
        if (fila.size() < 4) continue;
        Zona z;
        if (!parseZona(fila[2], z)) continue;
        bool act = (fila[3] == "1" || fila[3] == "true" || aMayusculas(fila[3]) == "ACTIVO");
        conductores_.push_back({ fila[0], fila[1], z, act });
    }

    // 3. Rutas y Rutas Detalle
    rutas_.clear();
    for (const auto& fila : leerFilasCsv(rutaData / "rutas.csv")) {
        if (fila.size() < 5) continue;
        Zona z;
        EstadoRuta er;
        if (!parseZona(fila[1], z) || !parseEstadoRuta(fila[4], er)) continue;
        rutas_.push_back({ fila[0], z, fila[2], fila[3], er, {} });
    }
    std::unordered_map<std::string, std::size_t> indiceRuta;
    for (std::size_t i = 0; i < rutas_.size(); ++i) indiceRuta[rutas_[i].codigo] = i;
    for (const auto& fila : leerFilasCsv(rutaData / "rutas_detalle.csv")) {
        if (fila.size() < 2) continue;
        auto it = indiceRuta.find(fila[0]);
        if (it != indiceRuta.end()) {
            rutas_[it->second].codigosPedidos.push_back(fila[1]);
        }
    }

    // 4. Eventos
    eventos_.clear();
    for (const auto& fila : leerFilasCsv(rutaData / "eventos.csv")) {
        if (fila.size() >= 5) {
            eventos_.push_back({ fila[0], fila[1], fila[2], fila[3], fila[4] });
        }
    }

    return true;
}

bool Controlador::guardarPedidosCsv(std::string& error) const {
    std::ofstream out(raiz_ / "data" / "pedidos.csv", std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "No se pudo escribir en data/pedidos.csv";
        return false;
    }
    out << "codigo,cliente,direccion,zona,estado,codigo_conductor,codigo_ruta,fecha_registro,fecha_despacho,fecha_entrega\n";
    for (const auto& p : pedidos_) {
        out << escaparCsv(p.codigo) << "," << escaparCsv(p.cliente) << "," << escaparCsv(p.direccion) << ","
            << toString(p.zona) << "," << toString(p.estado) << "," << escaparCsv(p.codigoConductor) << ","
            << escaparCsv(p.codigoRuta) << "," << escaparCsv(p.fechaRegistro) << ","
            << escaparCsv(p.fechaDespacho) << "," << escaparCsv(p.fechaEntrega) << "\n";
    }
    return true;
}

bool Controlador::guardarConductoresCsv(std::string& error) const {
    std::ofstream out(raiz_ / "data" / "conductores.csv", std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "No se pudo escribir en data/conductores.csv";
        return false;
    }
    out << "codigo,nombre,zona,activo\n";
    for (const auto& c : conductores_) {
        out << escaparCsv(c.codigo) << "," << escaparCsv(c.nombre) << "," << toString(c.zonaAsignada)
            << "," << (c.activo ? "1" : "0") << "\n";
    }
    return true;
}

bool Controlador::guardarRutasCsv(std::string& error) const {
    std::ofstream cab(raiz_ / "data" / "rutas.csv", std::ios::binary | std::ios::trunc);
    if (!cab) {
        error = "No se pudo escribir en data/rutas.csv";
        return false;
    }
    cab << "codigo,zona,codigo_conductor,fecha_creacion,estado\n";
    for (const auto& r : rutas_) {
        cab << escaparCsv(r.codigo) << "," << toString(r.zona) << "," << escaparCsv(r.codigoConductor)
            << "," << escaparCsv(r.fechaCreacion) << "," << toString(r.estado) << "\n";
    }
    cab.close();

    std::ofstream det(raiz_ / "data" / "rutas_detalle.csv", std::ios::binary | std::ios::trunc);
    if (!det) {
        error = "No se pudo escribir en data/rutas_detalle.csv";
        return false;
    }
    det << "codigo_ruta,codigo_pedido\n";
    for (const auto& r : rutas_) {
        for (const auto& codPedido : r.codigosPedidos) {
            det << escaparCsv(r.codigo) << "," << escaparCsv(codPedido) << "\n";
        }
    }
    return true;
}

bool Controlador::guardarEventosCsv(std::string& error) const {
    std::ofstream out(raiz_ / "data" / "eventos.csv", std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "No se pudo escribir en data/eventos.csv";
        return false;
    }
    out << "codigo_pedido,fecha,estado_anterior,estado_nuevo,descripcion\n";
    for (const auto& e : eventos_) {
        out << escaparCsv(e.codigoPedido) << "," << escaparCsv(e.fecha) << ","
            << escaparCsv(e.estadoAnterior) << "," << escaparCsv(e.estadoNuevo) << ","
            << escaparCsv(e.descripcion) << "\n";
    }
    return true;
}

bool Controlador::guardarDatos(std::string& error) const {
    return guardarPedidosCsv(error) && guardarConductoresCsv(error) &&
        guardarRutasCsv(error) && guardarEventosCsv(error);
}

// ============================================================================
// GENERACIÓN DE REPORTES (CSV Y HTML)
// ============================================================================
namespace {
    bool escribirReporteLista(const std::filesystem::path& archivo,
        const std::vector<const Pedido*>& lista,
        std::string& error) {
        std::ofstream out(archivo, std::ios::binary | std::ios::trunc);
        if (!out) {
            error = "No se pudo generar el reporte en: " + archivo.string();
            return false;
        }
        out << "codigo,cliente,direccion,zona,estado,conductor,ruta,fecha_registro,fecha_despacho,fecha_entrega\n";
        for (const auto* p : lista) {
            out << Controlador::escaparCsv(p->codigo) << "," << Controlador::escaparCsv(p->cliente) << ","
                << Controlador::escaparCsv(p->direccion) << "," << toString(p->zona) << ","
                << toString(p->estado) << "," << Controlador::escaparCsv(p->codigoConductor) << ","
                << Controlador::escaparCsv(p->codigoRuta) << "," << Controlador::escaparCsv(p->fechaRegistro) << ","
                << Controlador::escaparCsv(p->fechaDespacho) << "," << Controlador::escaparCsv(p->fechaEntrega) << "\n";
        }
        return true;
    }
}

bool Controlador::generarReportesCsv(std::string& error) const {
    const auto dirCsv = raiz_ / "reportes" / "csv";
    std::filesystem::create_directories(dirCsv);

    std::vector<const Pedido*> base;
    for (const auto& p : pedidos_) base.push_back(&p);

    // 1. Reporte General
    if (!escribirReporteLista(dirCsv / "pedidos_general.csv", base, error)) return false;

    // 2. Reporte agrupado/ordenado por Zona
    auto porZona = base;
    std::stable_sort(porZona.begin(), porZona.end(), [](const Pedido* a, const Pedido* b) {
        return toString(a->zona) < toString(b->zona);
        });
    if (!escribirReporteLista(dirCsv / "pedidos_por_zona.csv", porZona, error)) return false;

    // 3. Reporte agrupado/ordenado por Estado
    auto porEstado = base;
    std::stable_sort(porEstado.begin(), porEstado.end(), [](const Pedido* a, const Pedido* b) {
        return toString(a->estado) < toString(b->estado);
        });
    if (!escribirReporteLista(dirCsv / "pedidos_por_estado.csv", porEstado, error)) return false;

    // 4. Reporte agrupado/ordenado por Conductor
    auto porConductor = base;
    std::stable_sort(porConductor.begin(), porConductor.end(), [](const Pedido* a, const Pedido* b) {
        return a->codigoConductor < b->codigoConductor;
        });
    if (!escribirReporteLista(dirCsv / "pedidos_por_conductor.csv", porConductor, error)) return false;

    // 5. Detalle de Rutas y sus pedidos
    std::ofstream outRutas(dirCsv / "rutas_detalle.csv", std::ios::binary | std::ios::trunc);
    if (!outRutas) {
        error = "No se pudo generar rutas_detalle.csv";
        return false;
    }
    outRutas << "ruta,zona,estado_ruta,conductor,pedido,cliente,estado_pedido,fecha_creacion\n";
    for (const auto& r : rutas_) {
        for (const auto& codPedido : r.codigosPedidos) {
            const auto* p = buscarPedido(codPedido);
            outRutas << escaparCsv(r.codigo) << "," << toString(r.zona) << ","
                << toString(r.estado) << "," << escaparCsv(r.codigoConductor) << ","
                << escaparCsv(codPedido) << ","
                << escaparCsv(p ? p->cliente : "") << ","
                << (p ? toString(p->estado) : "NO_DISPONIBLE") << ","
                << escaparCsv(r.fechaCreacion) << "\n";
        }
    }

    // 6. Reportes individuales de trazabilidad por pedido
    for (const auto& p : pedidos_) {
        std::ofstream outPed(dirCsv / ("pedido_" + p.codigo + ".csv"), std::ios::binary | std::ios::trunc);
        if (!outPed) continue;
        outPed << "codigo,cliente,direccion,zona,estado_actual,fecha_evento,estado_anterior,estado_nuevo,descripcion\n";
        bool tieneEv = false;
        for (const auto& ev : eventos_) {
            if (ev.codigoPedido == p.codigo) {
                tieneEv = true;
                outPed << escaparCsv(p.codigo) << "," << escaparCsv(p.cliente) << ","
                    << escaparCsv(p.direccion) << "," << toString(p.zona) << ","
                    << toString(p.estado) << "," << escaparCsv(ev.fecha) << ","
                    << escaparCsv(ev.estadoAnterior) << "," << escaparCsv(ev.estadoNuevo) << ","
                    << escaparCsv(ev.descripcion) << "\n";
            }
        }
        if (!tieneEv) {
            outPed << escaparCsv(p.codigo) << "," << escaparCsv(p.cliente) << ","
                << escaparCsv(p.direccion) << "," << toString(p.zona) << ","
                << toString(p.estado) << ",,,,\n";
        }
    }

    return true;
}

bool Controlador::exportarArchivoHtml(const std::filesystem::path& origenCsv,
    const std::filesystem::path& destinoHtml,
    std::string& error) const {
    std::ifstream in(origenCsv, std::ios::binary);
    if (!in) {
        error = "No se pudo leer: " + origenCsv.string();
        return false;
    }
    std::ofstream out(destinoHtml, std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "No se pudo escribir: " + destinoHtml.string();
        return false;
    }

    std::string titulo = origenCsv.stem().string();
    std::replace(titulo.begin(), titulo.end(), '_', ' ');

    out << "<!doctype html>\n<html lang=\"es\">\n<head>\n"
        << "<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        << "<title>RouteTrack - " << escaparHtml(titulo) << "</title>\n"
        << "<link rel=\"stylesheet\" href=\"styles.css\">\n</head>\n<body>\n<main>\n"
        << "<a class=\"volver\" href=\"index.html\">← Volver al índice</a>\n"
        << "<h1>" << escaparHtml(titulo) << "</h1>\n"
        << "<p class=\"meta\">Reporte estático generado por RouteTrack.</p>\n<div class=\"tabla\"><table>\n";

    std::string linea;
    bool encabezado = true;
    while (std::getline(in, linea)) {
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
        auto campos = separarCsv(linea);
        out << (encabezado ? "<thead><tr>" : "<tbody><tr>");
        for (const auto& c : campos) {
            out << (encabezado ? "<th>" : "<td>") << escaparHtml(c) << (encabezado ? "</th>" : "</td>");
        }
        out << (encabezado ? "</tr></thead>\n" : "</tr></tbody>\n");
        encabezado = false;
    }
    out << "</table></div>\n</main>\n</body>\n</html>\n";
    return true;
}

bool Controlador::exportarReportesHtml(std::string& error) const {
    const auto dirHtml = raiz_ / "reportes" / "html";
    std::filesystem::create_directories(dirHtml);

    // 1. Métricas de Estado
    int pend = 0, camino = 0, entregado = 0;
    for (const auto& p : pedidos_) {
        if (p.estado == EstadoPedido::PENDIENTE) pend++;
        else if (p.estado == EstadoPedido::EN_CAMINO) camino++;
        else if (p.estado == EstadoPedido::ENTREGADO) entregado++;
    }

    // 2. Métricas de Carga por Zona
    int zNorte = 0, zSur = 0, zCentro = 0, zEste = 0;
    for (const auto& p : pedidos_) {
        switch (p.zona) {
        case Zona::NORTE:  zNorte++;  break;
        case Zona::SUR:    zSur++;    break;
        case Zona::CENTRO: zCentro++; break;
        case Zona::ESTE:   zEste++;   break;
        }
    }
    int maxZona = std::max({ zNorte, zSur, zCentro, zEste, 4 });

    // 3. Conteo desagregado por conductor (En Camino vs Entregados)
    std::unordered_map<std::string, int> condCamino;
    std::unordered_map<std::string, int> condEntregados;
    for (const auto& c : conductores_) {
        condCamino[c.codigo] = 0;
        condEntregados[c.codigo] = 0;
    }
    for (const auto& p : pedidos_) {
        if (!p.codigoConductor.empty()) {
            if (p.estado == EstadoPedido::EN_CAMINO) {
                condCamino[p.codigoConductor]++;
            }
            else if (p.estado == EstadoPedido::ENTREGADO) {
                condEntregados[p.codigoConductor]++;
            }
        }
    }

    std::string nombresConductoresJs = "[";
    std::string caminoConductoresJs = "[";
    std::string entregadosConductoresJs = "[";
    for (std::size_t i = 0; i < conductores_.size(); ++i) {
        nombresConductoresJs += "'" + escaparHtml(conductores_[i].nombre) + "'";
        caminoConductoresJs += std::to_string(condCamino[conductores_[i].codigo]);
        entregadosConductoresJs += std::to_string(condEntregados[conductores_[i].codigo]);
        if (i + 1 < conductores_.size()) {
            nombresConductoresJs += ",";
            caminoConductoresJs += ",";
            entregadosConductoresJs += ",";
        }
    }
    nombresConductoresJs += "]";
    caminoConductoresJs += "]";
    entregadosConductoresJs += "]";

    std::ofstream html(dirHtml / "index.html", std::ios::binary | std::ios::trunc);
    if (!html) {
        error = "No se pudo generar el archivo HTML.";
        return false;
    }

    html << "<!doctype html>\n<html lang=\"es\">\n<head>\n"
        << "<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        << "<title>RouteTrack - Dashboard Logístico</title>\n"
        << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n"
        << "<style>\n"
        << "  *{box-sizing:border-box} body{margin:0;background:#090d16;color:#f8fafc;font-family:'Segoe UI',system-ui,sans-serif;padding:24px}\n"
        << "  .contenedor{max-width:1300px;margin:auto}\n"
        << "  header{display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid #1e293b;padding-bottom:16px;margin-bottom:24px}\n"
        << "  h1{margin:0;font-size:26px;color:#38bdf8} .fecha{color:#94a3b8;font-size:14px}\n"
        << "  .kpis{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;margin-bottom:24px}\n"
        << "  .card{background:#111827;padding:18px;border-radius:12px;border:1px solid #1f2937}\n"
        << "  .card h3{margin:0;font-size:12px;color:#9ca3af;text-transform:uppercase;letter-spacing:.05em}\n"
        << "  .card p{margin:8px 0 0;font-size:28px;font-weight:700;color:#f3f4f6}\n"
        << "  .graficos{display:grid;grid-template-columns:1fr 1fr 1.2fr;gap:20px;margin-bottom:24px}\n"
        << "  @media(max-width:1050px){.graficos{grid-template-columns:1fr}}\n"
        << "  .grafico-card{background:#111827;padding:20px;border-radius:12px;border:1px solid #1f2937;display:flex;flex-direction:column;justify-content:center}\n"
        << "  .grafico-card h3{margin:0 0 16px;font-size:14px;color:#9ca3af;text-transform:uppercase;text-align:center}\n"
        << "  .seccion{background:#111827;border-radius:12px;padding:20px;border:1px solid #1f2937}\n"
        << "  .buscador{width:100%;padding:10px 14px;border-radius:8px;border:1px solid #374151;background:#030712;color:#fff;margin-bottom:16px;font-size:14px}\n"
        << "  table{width:100%;border-collapse:collapse;font-size:14px}\n"
        << "  th{background:#1f2937;color:#e5e7eb;text-align:left;padding:12px}\n"
        << "  td{padding:12px;border-bottom:1px solid #1f2937;color:#cbd5e1}\n"
        << "  tr:hover{background:#1e293b}\n"
        << "  .badge{display:inline-block;padding:4px 10px;border-radius:20px;font-size:12px;font-weight:600}\n"
        << "  .badge-pend{background:#78350f;color:#fde68a}\n"
        << "  .badge-camino{background:#1e3a8a;color:#93c5fd}\n"
        << "  .badge-entregado{background:#14532d;color:#86efac}\n"
        << "</style>\n</head>\n<body>\n"
        << "<div class=\"contenedor\">\n"
        << "  <header>\n"
        << "    <div>\n"
        << "      <h1>🚚 RouteTrack Analytics Dashboard</h1>\n"
        << "      <span class=\"fecha\">Sincronizado desde el núcleo en C++: " << fechaHoraActual() << "</span>\n"
        << "    </div>\n"
        << "  </header>\n"

        // KPI CARDS
        << "  <div class=\"kpis\">\n"
        << "    <div class=\"card\"><h3>Total Pedidos</h3><p>" << pedidos_.size() << "</p></div>\n"
        << "    <div class=\"card\"><h3>En Ruta</h3><p style=\"color:#60a5fa\">" << camino << "</p></div>\n"
        << "    <div class=\"card\"><h3>Entregados</h3><p style=\"color:#4ade80\">" << entregado << "</p></div>\n"
        << "    <div class=\"card\"><h3>Rutas Generadas</h3><p>" << rutas_.size() << "</p></div>\n"
        << "    <div class=\"card\"><h3>Flota Operativa</h3><p>" << conductores_.size() << "</p></div>\n"
        << "  </div>\n"

        // SECCIÓN DE GRÁFICOS
        << "  <div class=\"graficos\">\n"
        << "    <div class=\"grafico-card\"><h3>Estado de la Carga</h3><canvas id=\"chartEstado\"></canvas></div>\n"
        << "    <div class=\"grafico-card\"><h3>Distribución Radial por Zona</h3><canvas id=\"chartRadar\"></canvas></div>\n"
        << "    <div class=\"grafico-card\"><h3>Rendimiento por Conductor</h3><canvas id=\"chartConductor\"></canvas></div>\n"
        << "  </div>\n"

        // TABLA
        << "  <div class=\"seccion\">\n"
        << "    <h2 style=\"margin-top:0;font-size:18px;color:#f9fafb\">Control Maestro de Despachos</h2>\n"
        << "    <input type=\"text\" id=\"filtro\" class=\"buscador\" placeholder=\"Filtrar por código, cliente, zona, estado o conductor...\">\n"
        << "    <table><thead><tr>"
        << "<th>Código</th><th>Cliente</th><th>Dirección</th><th>Zona</th><th>Estado</th><th>Conductor</th><th>Ruta</th>"
        << "</tr></thead><tbody id=\"tablaCuerpo\">\n";

    for (const auto& p : pedidos_) {
        std::string badgeClass = (p.estado == EstadoPedido::PENDIENTE) ? "badge-pend" :
            (p.estado == EstadoPedido::EN_CAMINO ? "badge-camino" : "badge-entregado");
        html << "<tr>"
            << "<td><strong>" << escaparHtml(p.codigo) << "</strong></td>"
            << "<td>" << escaparHtml(p.cliente) << "</td>"
            << "<td>" << escaparHtml(p.direccion) << "</td>"
            << "<td>" << toString(p.zona) << "</td>"
            << "<td><span class=\"badge " << badgeClass << "\">" << toString(p.estado) << "</span></td>"
            << "<td>" << (p.codigoConductor.empty() ? "-" : escaparHtml(p.codigoConductor)) << "</td>"
            << "<td>" << (p.codigoRuta.empty() ? "-" : escaparHtml(p.codigoRuta)) << "</td>"
            << "</tr>\n";
    }

    // SCRIPTS
    html << "    </tbody></table>\n  </div>\n</div>\n"
        << "<script>\n"
        // 1. Doughnut: Estados
        << "  new Chart(document.getElementById('chartEstado'), {\n"
        << "    type: 'doughnut',\n"
        << "    data: {\n"
        << "      labels: ['Pendientes', 'En Camino', 'Entregados'],\n"
        << "      datasets: [{\n"
        << "        data: [" << pend << ", " << camino << ", " << entregado << "],\n"
        << "        backgroundColor: ['#eab308', '#3b82f6', '#22c55e'],\n"
        << "        borderWidth: 0\n"
        << "      }]\n"
        << "    },\n"
        << "    options: { plugins: { legend: { labels: { color: '#cbd5e1' } } } }\n"
        << "  });\n"

        // 2. Radar Chart: Polígono corregido con cierre de área y nodos destacados
        << "  new Chart(document.getElementById('chartRadar'), {\n"
        << "    type: 'radar',\n"
        << "    data: {\n"
        << "      labels: ['Norte', 'Sur', 'Centro', 'Este'],\n"
        << "      datasets: [{\n"
        << "        label: 'Pedidos por Zona',\n"
        << "        data: [" << zNorte << ", " << zSur << ", " << zCentro << ", " << zEste << "],\n"
        << "        backgroundColor: 'rgba(56, 189, 248, 0.35)',\n"
        << "        borderColor: '#38bdf8',\n"
        << "        borderWidth: 2,\n"
        << "        pointBackgroundColor: '#38bdf8',\n"
        << "        pointBorderColor: '#fff',\n"
        << "        pointRadius: 5,\n"
        << "        pointHoverRadius: 7\n"
        << "      }]\n"
        << "    },\n"
        << "    options: {\n"
        << "      scales: {\n"
        << "        r: {\n"
        << "          min: 0,\n"
        << "          suggestedMax: " << (maxZona + 1) << ",\n"
        << "          angleLines: { color: '#374151' },\n"
        << "          grid: { color: '#1f2937' },\n"
        << "          pointLabels: { color: '#93c5fd', font: { size: 12, weight: 'bold' } },\n"
        << "          ticks: { color: '#9ca3af', backdropColor: 'transparent', stepSize: 1 }\n"
        << "        }\n"
        << "      },\n"
        << "      plugins: { legend: { display: false } }\n"
        << "    }\n"
        << "  });\n"

        // 3. Gráfico Horizontal de Barras Apiladas (Entregados vs En Camino)
        << "  new Chart(document.getElementById('chartConductor'), {\n"
        << "    type: 'bar',\n"
        << "    data: {\n"
        << "      labels: " << nombresConductoresJs << ",\n"
        << "      datasets: [\n"
        << "        {\n"
        << "          label: 'Entregados',\n"
        << "          data: " << entregadosConductoresJs << ",\n"
        << "          backgroundColor: '#22c55e',\n"
        << "          borderRadius: 4\n"
        << "        },\n"
        << "        {\n"
        << "          label: 'En Camino',\n"
        << "          data: " << caminoConductoresJs << ",\n"
        << "          backgroundColor: '#3b82f6',\n"
        << "          borderRadius: 4\n"
        << "        }\n"
        << "      ]\n"
        << "    },\n"
        << "    options: {\n"
        << "      indexAxis: 'y',\n"
        << "      scales: {\n"
        << "        x: {\n"
        << "          stacked: true,\n"
        << "          ticks: { color: '#9ca3af', stepSize: 1 },\n"
        << "          grid: { color: '#1f2937' }\n"
        << "        },\n"
        << "        y: {\n"
        << "          stacked: true,\n"
        << "          ticks: { color: '#e5e7eb' },\n"
        << "          grid: { display: false }\n"
        << "        }\n"
        << "      },\n"
        << "      plugins: {\n"
        << "        legend: {\n"
        << "          display: true,\n"
        << "          labels: { color: '#cbd5e1', boxWidth: 12 }\n"
        << "        }\n"
        << "      }\n"
        << "    }\n"
        << "  });\n"

        // 4. Filtro reactivo
        << "  document.getElementById('filtro').addEventListener('keyup', function() {\n"
        << "    let valor = this.value.toLowerCase();\n"
        << "    let filas = document.querySelectorAll('#tablaCuerpo tr');\n"
        << "    filas.forEach(f => {\n"
        << "      f.style.display = f.textContent.toLowerCase().includes(valor) ? '' : 'none';\n"
        << "    });\n"
        << "  });\n"
        << "</script>\n"
        << "</body>\n</html>\n";

    return true;
}

