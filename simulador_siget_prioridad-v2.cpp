#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace std;

// Enum para representar los 5 estados clásicos del ciclo de vida de un proceso
enum EstadoProceso {
    NUEVO,
    LISTO,
    EN_EJECUCION,
    BLOQUEADO,
    TERMINADO
};

// Función auxiliar para convertir el Enum a String con formato visual
string estadoToString(EstadoProceso estado) {
    switch (estado) {
        case NUEVO:        return "[NUEVO]";
        case LISTO:        return "[LISTO]";
        case EN_EJECUCION: return "[EN EJECUCION]";
        case BLOQUEADO:    return "[BLOQUEADO]";
        case TERMINADO:    return "[TERMINADO]";
        default:           return "[DESCONOCIDO]";
    }
}

// Estructura que define un Proceso del SIGET
struct Proceso {
    int id;                     // Identificador único
    string nombre;              // Descripción de la tarea vial
    int tiempo_irrupcion;       // Ráfaga total de CPU estimada
    int tiempo_restante;        // Ráfaga pendiente por procesar
    int prioridad_alerta;       // Nivel de Alerta (1: Máxima Emergencia, 2: Media, 3: Rutina)
    int tamano_datos_mb;        // Volumen de datos a procesar en MB
    
    EstadoProceso estado;       // Estado actual del proceso
    int tiempo_llegada;         // Momento de entrada a la cola de Listo
    int tiempo_finalizacion;    // Momento en que completa su ejecución
    int tiempo_espera;          // Tiempo total pasado en la cola de LISTO
    int tiempo_retorno;         // Tiempo total transcurrido (Finalización - Llegada)
    bool ya_bloqueado;          // Bandera para simular evento E/S de sensores

    Proceso(int _id, string _nombre, int _irrupcion, int _prioridad, int _datos) {
        id = _id;
        nombre = _nombre;
        tiempo_irrupcion = _irrupcion;
        tiempo_restante = _irrupcion;
        prioridad_alerta = _prioridad;
        tamano_datos_mb = _datos;
        estado = NUEVO;
        tiempo_llegada = 0;
        tiempo_finalizacion = 0;
        tiempo_espera = 0;
        tiempo_retorno = 0;
        ya_bloqueado = false;
    }
};

// Imprime el encabezado y estado actual en pantalla haciendo una pausa interactiva/observable (1.2 segundos por defecto)
void mostrarCambioEstado(const Proceso& p, int tiempoActual, const string& eventoExtra = "", int retardo_ms = 1200) {
    cout << "  [T = " << setw(2) << tiempoActual << " u] " 
         << left << setw(35) << p.nombre 
         << " | Prioridad: " << p.prioridad_alerta
         << " | Estado: " << setw(15) << estadoToString(p.estado)
         << " | Restante: " << p.tiempo_restante << "u";
    if (!eventoExtra.empty()) {
        cout << " -> " << eventoExtra;
    }
    cout << endl << flush;
    
    // Pausa temporal para permitir la observación en tiempo real durante ejecuciones y grabaciones
    if (retardo_ms > 0) {
        this_thread::sleep_for(chrono::milliseconds(retardo_ms));
    }
}

// ============================================================================
// SIMULACIÓN 1: PLANIFICACIÓN POR PRIORIDAD POR ALERTA (SIGET EMERGENCIAS)
// ============================================================================
void simularPrioridadPorAlerta(vector<Proceso> procesos, int retardo_ms = 1200) {
    cout << "\n============================================================================\n";
    cout << "   ALGORITMO 1: PLANIFICACION POR PRIORIDAD POR ALERTA (SIGET EMERGENCIAS)\n";
    cout << "   Criterio: Alertas de nivel 1 (Emergencias) toman la CPU de inmediato.\n";
    cout << "============================================================================\n\n" << flush;
    this_thread::sleep_for(chrono::milliseconds(1500));

    int tiempoActual = 0;
    int procesosCompletados = 0;
    int n = procesos.size();

    // 1. Fase Inicial: Transición de NUEVO a LISTO
    for (auto& p : procesos) {
        p.estado = LISTO;
        mostrarCambioEstado(p, tiempoActual, "Proceso registrado e ingresado a cola LISTO", retardo_ms);
    }
    cout << "----------------------------------------------------------------------------\n" << flush;

    // Bucle principal de ejecución por Prioridad por Alerta
    while (procesosCompletados < n) {
        int idxSeleccionado = -1;
        int mayorPrioridad = 999;

        for (int i = 0; i < n; ++i) {
            if (procesos[i].estado == LISTO || procesos[i].estado == BLOQUEADO) {
                if (procesos[i].estado == BLOQUEADO) {
                    procesos[i].estado = LISTO;
                    mostrarCambioEstado(procesos[i], tiempoActual, "Sensor de datos desbloqueado -> Regresa a LISTO", retardo_ms);
                }
                
                if (procesos[i].prioridad_alerta < mayorPrioridad) {
                    mayorPrioridad = procesos[i].prioridad_alerta;
                    idxSeleccionado = i;
                }
            }
        }

        if (idxSeleccionado != -1) {
            Proceso& p = procesos[idxSeleccionado];

            // Transición a EN EJECUCIÓN
            p.estado = EN_EJECUCION;
            p.tiempo_espera = tiempoActual - p.tiempo_llegada;
            mostrarCambioEstado(p, tiempoActual, "Entra a CPU (Atencion de Alerta Nivel " + to_string(p.prioridad_alerta) + ")", retardo_ms);

            // Simulación de un evento de Bloqueo por E/S para tareas de control (ej. P3 Semáforos)
            if (p.id == 3 && !p.ya_bloqueado) {
                int tiempoEjecutado = 2; // Ejecuta 2 unidades y se bloquea leyendo sensores
                tiempoActual += tiempoEjecutado;
                p.tiempo_restante -= tiempoEjecutado;
                p.ya_bloqueado = true;
                p.estado = BLOQUEADO;
                mostrarCambioEstado(p, tiempoActual, "Solicita datos de sensores RFID -> Transicion a BLOQUEADO", retardo_ms);
                continue; // Libera la CPU para atender otra prioridad
            }

            // Procesamiento completo del proceso seleccionado
            tiempoActual += p.tiempo_restante;
            p.tiempo_restante = 0;
            p.estado = TERMINADO;
            p.tiempo_finalizacion = tiempoActual;
            p.tiempo_retorno = p.tiempo_finalizacion - p.tiempo_llegada;

            mostrarCambioEstado(p, tiempoActual, "Finalizo procesamiento exitosamente.", retardo_ms);
            procesosCompletados++;
        }
    }

    // Reporte de métricas del algoritmo de Prioridades
    cout << "\n--- RESUMEN DE TIEMPOS (PRIORIDAD POR ALERTA) ---\n";
    double sumaEspera = 0, sumaRetorno = 0;
    cout << left << setw(30) << "Proceso" 
         << setw(12) << "Prioridad" 
         << setw(15) << "T. Espera" 
         << setw(15) << "T. Retorno" << endl;
    
    for (const auto& p : procesos) {
        cout << left << setw(30) << p.nombre 
             << setw(12) << p.prioridad_alerta 
             << setw(15) << (to_string(p.tiempo_espera) + " u")
             << setw(15) << (to_string(p.tiempo_retorno) + " u") << endl;
        sumaEspera += p.tiempo_espera;
        sumaRetorno += p.tiempo_retorno;
    }
    cout << "---------------------------------------------------------\n";
    cout << "Tiempo Promedio de Espera:  " << fixed << setprecision(2) << (sumaEspera / n) << " unidades\n";
    cout << "Tiempo Promedio de Retorno: " << fixed << setprecision(2) << (sumaRetorno / n) << " unidades\n" << flush;
    this_thread::sleep_for(chrono::milliseconds(2000));
}

// ============================================================================
// SIMULACIÓN 2: ROUND ROBIN (Procesamiento Equitativo de Datos de Rutina)
// ============================================================================
void simularRoundRobin(vector<Proceso> procesos, int quantum, int retardo_ms = 1200) {
    cout << "\n============================================================================\n";
    cout << "   ALGORITMO 2: ROUND ROBIN (QUANTUM = " << quantum << " u) - PROCESAMIENTO EQUITATIVO\n";
    cout << "   Criterio: Reparto equitativo de tiempo de CPU entre todos los procesos.\n";
    cout << "============================================================================\n\n" << flush;
    this_thread::sleep_for(chrono::milliseconds(1500));

    int tiempoActual = 0;
    queue<int> colaListo;
    int n = procesos.size();
    int procesosCompletados = 0;

    // Transición inicial: NUEVO -> LISTO
    for (int i = 0; i < n; ++i) {
        procesos[i].estado = LISTO;
        mostrarCambioEstado(procesos[i], tiempoActual, "Proceso creado -> Ingresa a Cola Circular", retardo_ms);
        colaListo.push(i);
    }
    cout << "----------------------------------------------------------------------------\n" << flush;

    while (!colaListo.empty()) {
        int idx = colaListo.front();
        colaListo.pop();
        Proceso& p = procesos[idx];

        // Transición a EN EJECUCIÓN
        p.estado = EN_EJECUCION;
        int tiempoEjecutar = min(quantum, p.tiempo_restante);
        
        mostrarCambioEstado(p, tiempoActual, "Asignado a CPU por Quantum de " + to_string(tiempoEjecutar) + "u", retardo_ms);

        tiempoActual += tiempoEjecutar;
        p.tiempo_restante -= tiempoEjecutar;

        // Verificar si completó su ráfaga
        if (p.tiempo_restante == 0) {
            p.estado = TERMINADO;
            p.tiempo_finalizacion = tiempoActual;
            p.tiempo_retorno = p.tiempo_finalizacion - p.tiempo_llegada;
            p.tiempo_espera = p.tiempo_retorno - p.tiempo_irrupcion;
            mostrarCambioEstado(p, tiempoActual, "Completo su tiempo de CPU -> TERMINADO", retardo_ms);
            procesosCompletados++;
        } else {
            p.estado = LISTO;
            mostrarCambioEstado(p, tiempoActual, "Fin de Quantum -> Regresa a cola LISTO", retardo_ms);
            colaListo.push(idx);
        }
    }

    // Reporte de métricas de Round Robin
    cout << "\n--- RESUMEN DE TIEMPOS (ROUND ROBIN - QUANTUM " << quantum << ") ---\n";
    double sumaEspera = 0, sumaRetorno = 0;
    cout << left << setw(30) << "Proceso" 
         << setw(12) << "Prioridad" 
         << setw(15) << "T. Espera" 
         << setw(15) << "T. Retorno" << endl;
    
    for (const auto& p : procesos) {
        cout << left << setw(30) << p.nombre 
             << setw(12) << p.prioridad_alerta 
             << setw(15) << (to_string(p.tiempo_espera) + " u")
             << setw(15) << (to_string(p.tiempo_retorno) + " u") << endl;
        sumaEspera += p.tiempo_espera;
        sumaRetorno += p.tiempo_retorno;
    }
    cout << "---------------------------------------------------------\n";
    cout << "Tiempo Promedio de Espera:  " << fixed << setprecision(2) << (sumaEspera / n) << " unidades\n";
    cout << "Tiempo Promedio de Retorno: " << fixed << setprecision(2) << (sumaRetorno / n) << " unidades\n" << flush;
    this_thread::sleep_for(chrono::milliseconds(2000));
}

// ============================================================================
// FUNCIÓN PRINCIPAL (MAIN)
// ============================================================================
int main() {
    cout << "============================================================================\n";
    cout << "       SIMULADOR DEL PLANIFICADOR DE CPU - SISTEMA SIGET (C++)\n";
    cout << "  Demostracion de Transicion de Estados y Algoritmo por Prioridad de Alerta\n";
    cout << "============================================================================\n" << flush;

    // Configuración opcional de retardo por paso (en milisegundos)
    // Se establece en 1200 ms (1.2 segundos) para permitir observación visual pausada
    int retardoPasoMS = 1200;

    // Definición de los Procesos del SIGET
    vector<Proceso> procesosBase = {
        Proceso(1, "P1: Transito Rutinario - Zona Centro", 8, 3, 500),
        Proceso(2, "P2: Emergencia Ambulancia - Ruta 5", 3, 1, 100),
        Proceso(3, "P3: Control Semaforos - Av. Flores", 5, 2, 250)
    };

    cout << "\nPROCESOS REGISTRADOS EN EL SISTEMA SIGET:\n";
    cout << left << setw(5) << "ID" 
         << setw(38) << "Nombre Tarea" 
         << setw(15) << "CPU (u)" 
         << setw(18) << "Prioridad Alerta" 
         << setw(12) << "Datos (MB)" << endl;
    cout << "----------------------------------------------------------------------------\n";
    for (const auto& p : procesosBase) {
        cout << left << setw(5) << p.id 
             << setw(38) << p.nombre 
             << setw(15) << p.tiempo_irrupcion 
             << setw(18) << (to_string(p.prioridad_alerta) + (p.prioridad_alerta == 1 ? " (ALTA)" : (p.prioridad_alerta == 2 ? " (MEDIA)" : " (BAJA)")))
             << setw(12) << p.tamano_datos_mb << endl;
    }
    cout << "\n[INFO] Modo pausado activado (Pausa de " << retardoPasoMS << " ms por cada transicion)...\n" << flush;
    this_thread::sleep_for(chrono::milliseconds(2000));

    // 1. Simulación por Prioridad por Alerta
    simularPrioridadPorAlerta(procesosBase, retardoPasoMS);

    // 2. Simulación Round Robin (Quantum = 3)
    simularRoundRobin(procesosBase, 3, retardoPasoMS);

    cout << "\n============================================================================\n";
    cout << "                  EVALUACION FINAL DE RENDIMIENTO SIGET\n";
    cout << "============================================================================\n";
    cout << "1. En PRIORIDAD POR ALERTA: La ambulancia (P2) logra latencia de espera = 0u.\n";
    cout << "   Esto garantiza una respuesta inmediata ante eventos criticos en la via.\n";
    cout << "2. En ROUND ROBIN: Los datos se procesan de forma equitativa, pero la alerta\n";
    cout << "   de emergencia (P2) sufre un retraso de espera antes de ser atendida.\n";
    cout << "3. CONCLUSION: El SIGET debe operar con un algoritmo prioritario o hibrido\n";
    cout << "   (MLFQ) para asegurar la fluidez vial sin comprometer la seguridad urbana.\n";
    cout << "============================================================================\n\n" << flush;

    return 0;
}
