#include <iostream>
#include <vector>
#include "funciones.h"

using namespace std;

int main() {
    vector<Pedido> listaPedidos;
    int siguienteCodigo = 1;
    int siguienteRuta = 1;
    int opcion;

    // Al arrancar el programa, carga automáticamente los pedidos guardados previamente
    cargarDatos(listaPedidos, siguienteCodigo, siguienteRuta);

    do {
        mostrarMenu();
        opcion = leerNumero(1, 10);

        switch (opcion) {
        case 1: registrarPedido(listaPedidos, siguienteCodigo, siguienteRuta); break;
        case 2: listarPedidos(listaPedidos); break;
        case 3: buscarYMostrarPedido(listaPedidos); break;
        case 4: actualizarPedido(listaPedidos, siguienteCodigo, siguienteRuta); break;
        case 5: eliminarPedido(listaPedidos, siguienteCodigo, siguienteRuta); break;
        case 6: crearRuta(listaPedidos, siguienteCodigo, siguienteRuta); break;
        case 7: confirmarEntrega(listaPedidos, siguienteCodigo, siguienteRuta); break;
        case 8: guardarReporte(listaPedidos); break;
        case 9: verReporte(); break;
        case 10: cout << "Programa finalizado. Todos los datos estan respaldados." << endl; break;
        }
    } while (opcion != 10);

    return 0;
}