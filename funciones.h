#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

class Pedido {
private:
    int codigo;
    string cliente;
    string zona;
    string prioridad;
    string estado;
    string codigoRuta; // Nuevo: guarda "Sin asignar" o "RUT-001", "RUT-002"...

public:
    // Constructor para pedidos nuevos
    Pedido(int c, string cl, string z, string p);

    // Constructor sobrecargado para reconstruir pedidos al cargar desde el archivo
    Pedido(int c, string cl, string z, string p, string est, string ruta);

    int getCodigo() const;
    string getCodigoFormateado() const;
    string getCliente() const;
    string getZona() const;
    string getPrioridad() const;
    string getEstado() const;
    string getCodigoRuta() const;

    void setCliente(string cl);
    void setEstado(string e);
    void setCodigoRuta(string r);
};

// --- FUNCIONES ---
int leerNumero(int min, int max);
string leerTexto(string mensaje);
string elegirOpcion(string titulo, const string lista[], int cantidad);
int buscarPedido(const vector<Pedido>& pedidos);
int buscarPendiente(const vector<Pedido>& pedidos);
string formatearCodigoRuta(int numRuta);

// --- PROCEDIMIENTOS VOID ---
void mostrarMenu();
void imprimirCabeceraTabla(ostream& salida);
void imprimirFilaPedido(const Pedido& p, ostream& salida);

// Persistencia automática de base de datos en texto plano
void guardarDatos(const vector<Pedido>& pedidos, int siguienteCodigo, int siguienteRuta);
void cargarDatos(vector<Pedido>& pedidos, int& siguienteCodigo, int& siguienteRuta);

// Operaciones principales
void registrarPedido(vector<Pedido>& pedidos, int& siguienteCodigo, int siguienteRuta);
void listarPedidos(const vector<Pedido>& pedidos);
void buscarYMostrarPedido(const vector<Pedido>& pedidos);
void actualizarPedido(vector<Pedido>& pedidos, int siguienteCodigo, int siguienteRuta);
void eliminarPedido(vector<Pedido>& pedidos, int siguienteCodigo, int siguienteRuta);
void crearRuta(vector<Pedido>& pedidos, int siguienteCodigo, int& siguienteRuta);
void confirmarEntrega(vector<Pedido>& pedidos, int siguienteCodigo, int siguienteRuta);
void guardarReporte(const vector<Pedido>& pedidos);
void verReporte();

#endif