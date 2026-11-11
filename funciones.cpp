#include "funciones.h"

const string ZONAS[5] = { "Norte", "Sur", "Este", "Oeste", "Centro" };
const string PRIORIDADES[3] = { "Alta", "Media", "Baja" };
const string ESTADOS[3] = { "Pendiente", "En Ruta", "Entregado" };
const string OPCIONES_MENU[10] = {
    "Registrar pedido", "Listar pedidos", "Buscar pedido",
    "Actualizar cliente", "Eliminar pedido", "Crear ruta por zona",
    "Confirmar entrega", "Guardar reporte (.txt)", "Ver reporte (.txt)", "Salir"
};

// ==========================================
// IMPLEMENTACIÓN DE LA CLASE PEDIDO
// ==========================================
Pedido::Pedido(int c, string cl, string z, string p) {
    codigo = c;
    cliente = cl;
    zona = z;
    prioridad = p;
    estado = "Pendiente";
    codigoRuta = "Sin asignar";
}

Pedido::Pedido(int c, string cl, string z, string p, string est, string ruta) {
    codigo = c;
    cliente = cl;
    zona = z;
    prioridad = p;
    estado = est;
    codigoRuta = ruta;
}

int Pedido::getCodigo() const { return codigo; }
string Pedido::getCliente() const { return cliente; }
string Pedido::getZona() const { return zona; }
string Pedido::getPrioridad() const { return prioridad; }
string Pedido::getEstado() const { return estado; }
string Pedido::getCodigoRuta() const { return codigoRuta; }

void Pedido::setCliente(string cl) { cliente = cl; }
void Pedido::setEstado(string e) { estado = e; }
void Pedido::setCodigoRuta(string r) { codigoRuta = r; }

string Pedido::getCodigoFormateado() const {
    if (codigo < 10) return "PED-00" + to_string(codigo);
    else if (codigo < 100) return "PED-0" + to_string(codigo);
    else return "PED-" + to_string(codigo);
}

string formatearCodigoRuta(int numRuta) {
    if (numRuta < 10) return "RUT-00" + to_string(numRuta);
    else if (numRuta < 100) return "RUT-0" + to_string(numRuta);
    else return "RUT-" + to_string(numRuta);
}

// ==========================================
// PROCEDIMIENTOS DE TABLAS ESTILIZADAS (setw)
// ==========================================
void imprimirCabeceraTabla(ostream& salida) {
    salida << "\n"
        << left << setw(10) << "CODIGO"
        << left << setw(20) << "CLIENTE"
        << left << setw(10) << "ZONA"
        << left << setw(12) << "PRIORIDAD"
        << left << setw(12) << "ESTADO"
        << left << setw(12) << "RUTA" << endl;
    salida << string(76, '-') << endl;
}

void imprimirFilaPedido(const Pedido& p, ostream& salida) {
    salida << left << setw(10) << p.getCodigoFormateado()
        << left << setw(20) << p.getCliente()
        << left << setw(10) << p.getZona()
        << left << setw(12) << p.getPrioridad()
        << left << setw(12) << p.getEstado()
        << left << setw(12) << p.getCodigoRuta() << endl;
}

// ==========================================
// PERSISTENCIA AUTOMÁTICA (Autoguardado y Autocarga)
// ==========================================
void guardarDatos(const vector<Pedido>& pedidos, int siguienteCodigo, int siguienteRuta) {
    ofstream archivo("datos_pedidos.txt");
    if (!archivo.is_open()) return;

    // Primera línea guarda los contadores para no repetir códigos al reiniciar
    archivo << siguienteCodigo << endl;
    archivo << siguienteRuta << endl;
    archivo << pedidos.size() << endl;

    // Guardamos cada atributo en una línea para leer nombres con espacios sin errores
    for (size_t i = 0; i < pedidos.size(); ++i) {
        archivo << pedidos[i].getCodigo() << endl;
        archivo << pedidos[i].getCliente() << endl;
        archivo << pedidos[i].getZona() << endl;
        archivo << pedidos[i].getPrioridad() << endl;
        archivo << pedidos[i].getEstado() << endl;
        archivo << pedidos[i].getCodigoRuta() << endl;
    }
    archivo.close();
}

void cargarDatos(vector<Pedido>& pedidos, int& siguienteCodigo, int& siguienteRuta) {
    ifstream archivo("datos_pedidos.txt");
    if (!archivo.is_open()) return; // Si es la primera vez que se abre, no hace nada

    int cantidad = 0;
    if (!(archivo >> siguienteCodigo)) return;
    if (!(archivo >> siguienteRuta)) return;
    if (!(archivo >> cantidad)) return;
    archivo.ignore(1000, '\n');

    pedidos.clear();
    for (int i = 0; i < cantidad; ++i) {
        int cod;
        string cli, zon, pri, est, rut;
        archivo >> cod;
        archivo.ignore(1000, '\n');
        getline(archivo, cli);
        getline(archivo, zon);
        getline(archivo, pri);
        getline(archivo, est);
        getline(archivo, rut);
        pedidos.push_back(Pedido(cod, cli, zon, pri, est, rut));
    }
    archivo.close();
    cout << "[Sistema] Se cargaron " << pedidos.size() << " pedidos previos desde 'datos_pedidos.txt'." << endl;
}

// ==========================================
// VALIDACIONES Y BÚSQUEDAS
// ==========================================
int leerNumero(int min, int max) {
    int n;
    while (!(cin >> n) || n < min || n > max) {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Dato no valido. Ingrese un numero de " << min << " a " << max << ": ";
    }
    cin.ignore(1000, '\n');
    return n;
}

string leerTexto(string mensaje) {
    string texto;
    do {
        cout << mensaje;
        getline(cin, texto);
    } while (texto.empty());
    return texto;
}

string elegirOpcion(string titulo, const string lista[], int cantidad) {
    cout << titulo << endl;
    for (int i = 0; i < cantidad; i++) {
        cout << i + 1 << ". " << lista[i] << endl;
    }
    cout << "Opcion: ";
    return lista[leerNumero(1, cantidad) - 1];
}

int buscarPedido(const vector<Pedido>& pedidos) {
    if (pedidos.empty()) {
        cout << "No hay pedidos registrados en el sistema." << endl;
        return -1;
    }
    cout << "Ingrese el numero del pedido (ej. 1 para PED-001): ";
    int codigo = leerNumero(1, 9999);
    for (size_t i = 0; i < pedidos.size(); ++i) {
        if (pedidos[i].getCodigo() == codigo) {
            return i;
        }
    }
    cout << "Pedido no encontrado." << endl;
    return -1;
}

int buscarPendiente(const vector<Pedido>& pedidos) {
    int pos = buscarPedido(pedidos);
    if (pos != -1 && pedidos[pos].getEstado() != "Pendiente") {
        cout << "Accion denegada: Solo se pueden modificar pedidos en estado 'Pendiente'." << endl;
        return -1;
    }
    return pos;
}

// ==========================================
// PROCEDIMIENTOS DEL MENÚ Y CRUD
// ==========================================
void mostrarMenu() {
    cout << "\n===== SISTEMA DE MICROENTREGAS (LAST-MILE) =====" << endl;
    for (int i = 0; i < 10; i++) {
        cout << i + 1 << ". " << OPCIONES_MENU[i] << endl;
    }
    cout << "Opcion: ";
}

void registrarPedido(vector<Pedido>& pedidos, int& siguienteCodigo, int siguienteRuta) {
    string cliente = leerTexto("Nombre del cliente: ");
    string zona = elegirOpcion("Seleccione la Zona:", ZONAS, 5);
    string prioridad = elegirOpcion("Seleccione la Prioridad:", PRIORIDADES, 3);

    Pedido nuevo(siguienteCodigo, cliente, zona, prioridad);
    pedidos.push_back(nuevo);
    cout << "-> Pedido registrado con codigo: " << nuevo.getCodigoFormateado() << endl;
    siguienteCodigo++;

    guardarDatos(pedidos, siguienteCodigo, siguienteRuta);
}

void listarPedidos(const vector<Pedido>& pedidos) {
    if (pedidos.empty()) {
        cout << "No hay pedidos registrados." << endl;
        return;
    }
    imprimirCabeceraTabla(cout);
    for (size_t i = 0; i < pedidos.size(); ++i) {
        imprimirFilaPedido(pedidos[i], cout);
    }
}

void buscarYMostrarPedido(const vector<Pedido>& pedidos) {
    int pos = buscarPedido(pedidos);
    if (pos != -1) {
        imprimirCabeceraTabla(cout);
        imprimirFilaPedido(pedidos[pos], cout);
    }
}

void actualizarPedido(vector<Pedido>& pedidos, int siguienteCodigo, int siguienteRuta) {
    int pos = buscarPendiente(pedidos);
    if (pos != -1) {
        pedidos[pos].setCliente(leerTexto("Nuevo nombre del cliente: "));
        cout << "-> Pedido " << pedidos[pos].getCodigoFormateado() << " actualizado." << endl;
        guardarDatos(pedidos, siguienteCodigo, siguienteRuta);
    }
}

void eliminarPedido(vector<Pedido>& pedidos, int siguienteCodigo, int siguienteRuta) {
    int pos = buscarPendiente(pedidos);
    if (pos != -1) {
        string cod = pedidos[pos].getCodigoFormateado();
        pedidos.erase(pedidos.begin() + pos);
        cout << "-> Pedido " << cod << " eliminado del sistema." << endl;
        guardarDatos(pedidos, siguienteCodigo, siguienteRuta);
    }
}

void crearRuta(vector<Pedido>& pedidos, int siguienteCodigo, int& siguienteRuta) {
    if (pedidos.empty()) {
        cout << "No hay pedidos registrados." << endl;
        return;
    }
    string zona = elegirOpcion("Seleccione la Zona para consolidar ruta:", ZONAS, 5);
    string codRutaActual = formatearCodigoRuta(siguienteRuta);
    int total = 0;

    for (int p = 0; p < 3; p++) {
        for (size_t i = 0; i < pedidos.size(); ++i) {
            if (pedidos[i].getZona() == zona &&
                pedidos[i].getEstado() == "Pendiente" &&
                pedidos[i].getPrioridad() == PRIORIDADES[p]) {

                if (total == 0) {
                    cout << "\n--- Despachando Ruta " << codRutaActual << " para Zona " << zona << " ---";
                    imprimirCabeceraTabla(cout);
                }
                pedidos[i].setEstado("En Ruta");
                pedidos[i].setCodigoRuta(codRutaActual);
                imprimirFilaPedido(pedidos[i], cout);
                total++;
            }
        }
    }

    if (total == 0) {
        cout << "No hay pedidos pendientes en la zona " << zona << "." << endl;
    }
    else {
        cout << string(76, '-') << endl;
        cout << "-> Ruta " << codRutaActual << " creada con " << total << " pedidos asignados." << endl;
        siguienteRuta++;
        guardarDatos(pedidos, siguienteCodigo, siguienteRuta);
    }
}

void confirmarEntrega(vector<Pedido>& pedidos, int siguienteCodigo, int siguienteRuta) {
    int pos = buscarPedido(pedidos);
    if (pos != -1) {
        if (pedidos[pos].getEstado() != "En Ruta") {
            cout << "Accion denegada: Solo se pueden confirmar pedidos que estan 'En Ruta'." << endl;
        }
        else {
            pedidos[pos].setEstado("Entregado");
            cout << "-> Entrega del pedido " << pedidos[pos].getCodigoFormateado() << " confirmada." << endl;
            guardarDatos(pedidos, siguienteCodigo, siguienteRuta);
        }
    }
}

void guardarReporte(const vector<Pedido>& pedidos) {
    ofstream archivo("reporte.txt");
    if (!archivo.is_open()) {
        cout << "Error: No se pudo crear el archivo reporte.txt." << endl;
        return;
    }

    int resumen[5][3] = { 0 };

    archivo << "======================= REPORTE DE MICROENTREGAS =======================";
    imprimirCabeceraTabla(archivo);

    for (size_t i = 0; i < pedidos.size(); ++i) {
        imprimirFilaPedido(pedidos[i], archivo);
        for (int z = 0; z < 5; z++) {
            for (int e = 0; e < 3; e++) {
                if (pedidos[i].getZona() == ZONAS[z] && pedidos[i].getEstado() == ESTADOS[e]) {
                    resumen[z][e]++;
                }
            }
        }
    }

    archivo << "\n================ RESUMEN OPERATIVO (ZONA X ESTADO) ================" << endl;
    archivo << left << setw(15) << "ZONA"
        << left << setw(15) << "PENDIENTE"
        << left << setw(15) << "EN RUTA"
        << left << setw(15) << "ENTREGADO" << endl;
    archivo << string(60, '-') << endl;

    for (int z = 0; z < 5; z++) {
        archivo << left << setw(15) << ZONAS[z]
            << left << setw(15) << resumen[z][0]
            << left << setw(15) << resumen[z][1]
            << left << setw(15) << resumen[z][2] << endl;
    }
    archivo << string(60, '-') << endl;
    archivo << "Total general de pedidos procesados: " << pedidos.size() << endl;

    archivo.close();
    cout << "-> Reporte gerencial exportado exitosamente en 'reporte.txt'." << endl;
}

void verReporte() {
    ifstream archivo("reporte.txt");
    if (!archivo.is_open()) {
        cout << "El archivo 'reporte.txt' aun no existe. Primero elija la opcion 8." << endl;
        return;
    }
    cout << "\n";
    string linea;
    while (getline(archivo, linea)) {
        cout << linea << endl;
    }
    archivo.close();
}