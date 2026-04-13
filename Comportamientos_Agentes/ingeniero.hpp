#ifndef COMPORTAMIENTOINGENIERO_H
#define COMPORTAMIENTOINGENIERO_H

#include <chrono>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <time.h>
#include <list>

#include "comportamientos/comportamiento.hpp"

// Estructura para representar la situación exacta del agente
struct estado {
    int fila;
    int columna;
    int orientacion; // 0: Norte, 1: Este, 2: Sur, 3: Oeste

    // Sobrecargamos el operador < para que C++ nos deje usar std::set (memoria de visitados)
    bool operator<(const estado& otro) const {
        if (fila != otro.fila) return fila < otro.fila;
        if (columna != otro.columna) return columna < otro.columna;
        return orientacion < otro.orientacion;
    }
    
    // Sobrecargamos el operador == para comprobar fácilmente si hemos llegado a la meta
    bool operator==(const estado& otro) const {
        return fila == otro.fila && columna == otro.columna;
        // Nota: La orientación de llegada no importa para la meta, solo la posición
    }
};

// Estructura para el árbol de búsqueda
struct nodo {
    estado st;
    std::list<Action> secuencia; // La lista de acciones para llegar a este estado
};

class ComportamientoIngeniero : public Comportamiento {
public:
    // =========================================================================
    // CONSTRUCTORES
    // =========================================================================

    /**
    * @brief Constructor para niveles 0, 1 y 6 (sin mapa completo)
    * @param size Tamaño del mapa (si es 0, se inicializa más tarde)
    */
    ComportamientoIngeniero(unsigned int size = 0) : Comportamiento(size) {
        // Inicializar Variables de Estado
        last_action = IDLE;
        tiene_zapatillas = false;

        giro45Izq = 0;

        giros_sin_avanzar_n0 = 0;
        girar_derecha_n0 = false;

        // Dentro del constructor:
        for(int i = 0; i < 200; i++){
            for(int j = 0; j < 200; j++){
                matriz_visitas[i][j] = 0;
            }
        }

        est_n6 = 0;
    }

    /**
    * @brief Constructor para niveles 2, 3, 4 y 5 (con mapa completo conocido)
    * @param mapaR Mapa de terreno conocido
    * @param mapaC Mapa de cotas conocido
    */
    ComportamientoIngeniero(std::vector<std::vector<unsigned char>> mapaR, 
                            std::vector<std::vector<unsigned char>> mapaC): 
                            Comportamiento(mapaR, mapaC) {
        // Inicializar Variables de Estado
        hayPlan = false;
    }
    
    ComportamientoIngeniero(const ComportamientoIngeniero &comport)
        : Comportamiento(comport) {}
    ~ComportamientoIngeniero() {}

    /**
    * @brief Bucle principal de decisión del agente.
    * Estudia los sensores y decide la siguiente acción.
    * 
    * EJEMPLO DE USO:
    * Action accion = think(sensores);
    * return accion; // El motor ejecutará esta acción
    */
    Action think(Sensores sensores);

    ComportamientoIngeniero *clone() {
        return new ComportamientoIngeniero(*this);
    }

    // =========================================================================
    // ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
    // =========================================================================

    // Funciones específicas para cada nivel (para ser implementadas por el alumno)
    
    /**
    * @brief Implementación del Nivel 0.
    * @param sensores Datos actuales de los sensores del agente.
    * @return Acción a realizar.
    */
    Action ComportamientoIngenieroNivel_0(Sensores sensores);

    /**
    * @brief Implementación del Nivel 1.
    * @param sensores Datos actuales de los sensores del agente.
    * @return Acción a realizar.
    */
    Action ComportamientoIngenieroNivel_1(Sensores sensores);

    /**
    * @brief Implementación del Nivel 2.
    * @param sensores Datos actuales de los sensores del agente.
    * @return Acción a realizar.
    */ 
    Action ComportamientoIngenieroNivel_2(Sensores sensores);

    /**
    * @brief Implementación del Nivel 3.
    * @param sensores Datos actuales de los sensores del agente.
    * @return Acción a realizar.
    */
    Action ComportamientoIngenieroNivel_3(Sensores sensores);

    /**
    * @brief Implementación del Nivel 4.
    * @param sensores Datos actuales de los sensores del agente.
    * @return Acción a realizar.
    */
    Action ComportamientoIngenieroNivel_4(Sensores sensores);

    /**
    * @brief Implementación del Nivel 5.
    * @param sensores Datos actuales de los sensores del agente.
    * @return Acción a realizar.
    */
    Action ComportamientoIngenieroNivel_5(Sensores sensores);

    /**
    * @brief Implementación del Nivel 6.
    * @param sensores Datos actuales de los sensores del agente.
    * @return Acción a realizar.
    */
    Action ComportamientoIngenieroNivel_6(Sensores sensores);


    // Estado para la planificación de tuberías
    struct estado_tuberia {
        int fila;
        int columna;
        int mod; // Modificación del terreno: -1 (DIG), 0 (Nada), 1 (RAISE)

        bool operator<(const estado_tuberia& otro) const {
            if (fila != otro.fila) return fila < otro.fila;

            if (columna != otro.columna) return columna < otro.columna;

            return mod < otro.mod;
        }
    };

    struct nodo_tuberia {
        estado_tuberia st;

        std::list<Paso> secuencia; // Usamos la estructura 'Paso' del motor
    };

protected:
    // =========================================================================
    // FUNCIONES PROPORCIONADAS
    // =========================================================================

    /**
    * @brief Actualiza la información del mapa interno basándose en los sensores.
    * IMPORTANTE: Esta función ya está implementada. Actualiza mapaResultado y mapaCotas
    * con la información de los 16 sensores (casilla actual + 15 casillas alrededor).
    */
    void ActualizarMapa(Sensores sensores);

    /**
    * @brief Comprueba si una casilla es transitable.
    * @param f Fila de la casilla.
    * @param c Columna de la casilla.
    * @param tieneZapatillas Indica si el agente posee zapatillas.
    * @return true si la casilla es transitable (no es muro ni precipicio).
    */
    bool EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas);

    /**
    * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
    * REGLAS: Desnivel máximo 1 sin zapatillas, 2 con zapatillas.
    * @param actual Estado actual del agente (fila, columna, orientacion).
    * @return true si el desnivel con la casilla de delante es admisible.
    */
    bool EsAccesiblePorAltura(const ubicacion &actual, bool zap);

    /**
    * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
    * @param actual Estado actual del agente (fila, columna, orientacion).
    * @return Estado con la fila y columna de la casilla de enfrente.
    */
    ubicacion Delante(const ubicacion &actual) const;

    bool es_camino(unsigned char c) const;

    /**
    * @brief Imprime por consola la secuencia de acciones de un plan para un agente.
    * @param plan  Lista de acciones del plan.
    */
    void PintaPlan(const list<Action> &plan);


    /**
    * @brief Imprime las coordenadas y operaciones de un plan de tubería.
    * @param plan  Lista de pasos (fila, columna, operación).
    */
    void PintaPlan(const list<Paso> &plan);


    /**
    * @brief Convierte un plan de acciones en una lista de casillas para
    *        su visualización en el mapa gráfico.
    * @param st    Estado de partida.
    * @param plan  Lista de acciones del plan.
    */
    void VisualizaPlan(const ubicacion &st, const list<Action> &plan);

    /**
    * @brief Convierte un plan de tubería en la lista de casillas usada
    *        por el sistema de visualización.
    * @param st    Estado de partida (no utilizado directamente).
    * @param plan  Lista de pasos del plan de tubería.
    */
    void VisualizaRedTuberias(const list<Paso> &plan);



private:
    // =========================================================================
    // VARIABLES DE ESTADO (PUEDEN SER EXTENDIDAS POR EL ALUMNO)
    // =========================================================================

    // =========================================================
    // === VARIABLES Y ESTRUCTURAS NIVEL 0  ====================
    // =========================================================
    // Variables para el Nivel 0
    Action last_action ;
    bool tiene_zapatillas ;
    int giros_sin_avanzar_n0;
    bool girar_derecha_n0;

    // Métodos auxiliares para el Nivel 0
    char ViablePorAltura(char casilla, int dif, bool zap);
    int VeoCasillaInteresante(char i, char c, char d, bool zap);



    // =========================================================
    // === VARIABLES Y ESTRUCTURAS NIVEL 1  ====================
    // =========================================================
    // Variables para el Nivel 1 
    int giro45Izq; // Indica el número de giros a la izquierda que quedan por dar
    int matriz_visitas[200][200];

    // Métodos auxiliares para el Nivel 1
    bool es_transitable_N1(unsigned char c) const;
    char ViablePorAltura_N1(char casilla, int dif, bool zap);
    int VeoCasillaInteresante_N1(char i, char c, char d, bool zap);

    // =========================================================
    // === VARIABLES Y ESTRUCTURAS NIVEL 2 (DELIBERATIVO) ===
    // =========================================================
    bool hayPlan;                 // Para saber si ya hemos calculado la ruta
    std::list<Action> plan;       // Aquí guardaremos las instrucciones a seguir
    
    // Declaramos nuestra función del algoritmo de búsqueda
    list<Action> BusquedaEnAnchura(const estado &origen, const estado &destino, bool agua_permitida = false, bool ignorar_entidades = false, bool tiene_zap_inicio = false);

    // NIVEL 3
    struct Estado {
        int f;
        int c;
        Orientacion brujula;
        bool zapatillas; // ¡NUEVO! Vital para cruzar bosques
        
        bool operator<(const Estado& otro) const {
            if (f != otro.f) return f < otro.f;
            if (c != otro.c) return c < otro.c;
            if (brujula != otro.brujula) return brujula < otro.brujula;
            return zapatillas < otro.zapatillas;
        }
        bool operator==(const Estado& otro) const {
            return f == otro.f && c == otro.c && brujula == otro.brujula && zapatillas == otro.zapatillas;
        }
    };

    struct Nodo {
        Estado st;
        std::list<Action> secuencia;
        int coste_g; // Coste real acumulado (Batería gastada)
        int coste_h; // Heurística (Distancia estimada)
        int f() const { return coste_g + coste_h; } // Coste total estimado

        // ¡NUEVO! Para que la Cola de Prioridad ordene de menor a mayor coste
        bool operator>(const Nodo& otro) const {
            return f() > otro.f();
        }
    };

    struct estado_ext {
        int fila;
        int columna;
        int orientacion;
        bool zapatillas;

        bool operator<(const estado_ext& otro) const {
            if (fila != otro.fila) return fila < otro.fila;
            if (columna != otro.columna) return columna < otro.columna;
            if (orientacion != otro.orientacion) return orientacion < otro.orientacion;
            return zapatillas < otro.zapatillas;
        }
        bool operator==(const estado_ext& otro) const {
            return fila == otro.fila && columna == otro.columna;
        }
    };

    struct nodo_ext {
        estado_ext st;
        std::list<Action> secuencia;
    };

    // =========================================================
    // === VARIABLES Y ESTRUCTURAS NIVEL 4 (TUBERÍAS) ==========
    // =========================================================
    bool plan_tuberias_hecho = false;
    std::list<Paso> plan_tuberias;

    struct EstadoN4 {
        int f;
        int c;
        int h; // Altura que tendrá la tubería en esta casilla

        bool operator<(const EstadoN4& otro) const {
            if (f != otro.f) return f < otro.f;
            if (c != otro.c) return c < otro.c;
            return h < otro.h;
        }
        bool operator==(const EstadoN4& otro) const {
            return f == otro.f && c == otro.c && h == otro.h;
        }
    };

    struct NodoN4 {
        EstadoN4 st;
        std::list<Paso> secuencia;
        int impacto = 0 ; // Impacto ecológico acumulado

        bool operator>(const NodoN4& otro) const {
            int coste_a = (int)secuencia.size() * 10000 + impacto;
            int coste_b = (int)otro.secuencia.size() * 10000 + otro.impacto;
            return coste_a > coste_b;
        }
    };

    bool EncontrarPlan_N4(int start_f, int start_c, std::list<Paso>& plan_resultante);

    // =========================================================
    // === VARIABLES NIVEL 5 (MÁQUINA DE ESTADOS) =============
    // =========================================================
    int tramo_n5 = 0;
    bool acabo_de_instalar_n5 = false;
    bool terraformado_n5 = false;
    std::vector<Paso> plan_n5; // Vector para acceder a los tramos fácilmente

    // =========================================================
    // === MÁQUINA DE ESTADOS NIVEL 5 (JEFE DE OBRA) ==========
    // =========================================================
    enum EstadoObraIng { ING_CALCULAR_PLAN, ING_IR_CASILLA, ING_TERRAFORMAR, ING_LLAMAR, ING_ALINEARSE };
    EstadoObraIng estado_obra_ing = ING_CALCULAR_PLAN;

    // =========================================================
    // === NIVEL 6 =============================================
    // =========================================================
    int est_n6 ;
};



#endif