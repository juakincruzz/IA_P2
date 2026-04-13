#ifndef COMPORTAMIENTOTECNICO_H
#define COMPORTAMIENTOTECNICO_H

#include <chrono>
#include <time.h>
#include <thread>
#include <list>

#include "comportamientos/comportamiento.hpp"

// =========================================================================
// DOCUMENTACIÓN PARA ESTUDIANTES
// =========================================================================
  /*
  * CLASE: ComportamientoTecnico
  * 
  * DESCRIPCIÓN:
  * Esta clase implementa el comportamiento del agente Técnico en el mundo Belkan.
  * El técnico colabora con el ingeniero para resolver el problema de instalación de tuberías
  */



class ComportamientoTecnico : public Comportamiento {
public:
  // =========================================================================
  // CONSTRUCTORES
  // =========================================================================
  
  /**
  * @brief Constructor para niveles 0, 1 y 6 (sin mapa completo)
  * @param size Tamaño del mapa (si es 0, se inicializa más tarde)
  */
  ComportamientoTecnico(unsigned int size = 0) : Comportamiento(size) {
    // Inicializar Variables de Estado
    last_action = IDLE;
    tiene_zapatillas = false;
    giro45Izq = 0;
    giros_sin_avanzar_n0 = 0;
    girar_derecha_n0 = true; // El técnico gira a la derecha por defecto (opuesto al ingeniero)
    turnos_viendo_ingeniero_n0 = 0;
    retroceder_n0 = 0;

    // Dentro del constructor:
    for(int i = 0; i < 200; i++){
      for(int j = 0; j < 200; j++){
        matriz_visitas[i][j] = 0;
      }
    }
  }

  /**
  * @brief Constructor para niveles 2, 3, 4 y 5 (con mapa completo conocido)
  * @param mapaR Mapa de terreno conocido
  * @param mapaC Mapa de cotas conocido
  */
  ComportamientoTecnico(std::vector<std::vector<unsigned char>> mapaR, 
                        std::vector<std::vector<unsigned char>> mapaC): 
                        Comportamiento(mapaR, mapaC) {
    hay_plan = false;
  }

  ComportamientoTecnico(const ComportamientoTecnico &comport): Comportamiento(comport) {}
  ~ComportamientoTecnico() {}

  /**
  * @brief Bucle principal de decisión del técnico.
  * Estudia los sensores y decide la siguiente acción.
  * 
  * EJEMPLO DE USO:
  * Action accion = think(sensores);
  * return accion; // El motor ejecutará esta acción
  */
  Action think(Sensores sensores);

  ComportamientoTecnico *clone() {
    return new ComportamientoTecnico(*this);
  }

  // =========================================================================
  // ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
  // =========================================================================

  /**
  * @brief Comportamiento del técnico para el Nivel 0.
  * @param sensores Datos actuales de los sensores.
  * @return Acción a realizar.
  */
  Action ComportamientoTecnicoNivel_0(Sensores sensores);
  
  /**
  * @brief Comportamiento del técnico para el Nivel 1.
  * @param sensores Datos actuales de los sensores.
  * @return Acción a realizar.
  */
  Action ComportamientoTecnicoNivel_1(Sensores sensores);
  
  /**
  * @brief Comportamiento del técnico para el Nivel 2.
  * @param sensores Datos actuales de los sensores.
  * @return Acción a realizar.
  */
  Action ComportamientoTecnicoNivel_2(Sensores sensores);
  
  /**
  * @brief Comportamiento del técnico para el Nivel 3.
  * @param sensores Datos actuales de los sensores.
  * @return Acción a realizar.
  */
  Action ComportamientoTecnicoNivel_3(Sensores sensores);
  
  /**
  * @brief Comportamiento del técnico para el Nivel 4.
  * @param sensores Datos actuales de los sensores.
  * @return Acción a realizar.
  */
  Action ComportamientoTecnicoNivel_4(Sensores sensores);
  
  /**
  * @brief Comportamiento del técnico para el Nivel 5.
  * @param sensores Datos actuales de los sensores.
  * @return Acción a realizar.
  */
  Action ComportamientoTecnicoNivel_5(Sensores sensores);
  
  /**
  * @brief Comportamiento del técnico para el Nivel 6.
  * @param sensores Datos actuales de los sensores.
  * @return Acción a realizar.
  */
  Action ComportamientoTecnicoNivel_6(Sensores sensores);

  // Estructura de estado (igual que la del ingeniero)
  struct estado {
      int fila;
      int columna;
      int orientacion;

      bool operator<(const estado& otro) const {
          if (fila != otro.fila) return fila < otro.fila;
          if (columna != otro.columna) return columna < otro.columna;
          return orientacion < otro.orientacion;
      }
      bool operator==(const estado& otro) const {
          return fila == otro.fila && columna == otro.columna;
      }
  };

  // Estructura del Nodo para el algoritmo A*
  struct nodo {
      estado st;
      std::list<Action> secuencia;
      int coste_g; // Batería gastada hasta llegar aquí
      int heuristica_h; // Estimación de batería hasta la meta
      int f; // f = g + h

      // IMPORTANTE: La cola de prioridad en C++ ordena de mayor a menor por defecto.
      // Sobrecargamos el operador > para que ponga primero los nodos con MENOR 'f' (los más baratos).
      bool operator>(const nodo& otro) const {
          return f > otro.f;
      }
  };

protected:
  // =========================================================================
  // FUNCIONES PROPORCIONADAS
  // =========================================================================

  /**
  * @brief Actualiza el mapaResultado y mapaCotas con la información de los sensores.
  * IMPORTANTE: Esta función ya está implementada. Actualiza mapaResultado y mapaCotas
  * con la información de los 16 sensores.
  */
  void ActualizarMapa(Sensores sensores);

  /**
  * @brief Determina si una casilla es transitable para el técnico.
  * NOTA: El técnico puede tener reglas de transitabilidad diferentes al ingeniero.
  * @param f Fila de la casilla.
  * @param c Columna de la casilla.
  * @param tieneZapatillas Indica si el agente posee las zapatillas.
  * @return true si la casilla es transitable.
  */
  bool EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas);

  /**
  * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
  * REGLA PARA TÉCNICO: Desnivel máximo siempre 1 (independiente de zapatillas).
  * @param actual Estado actual del agente (fila, columna, orientacion).
  * @return true si el desnivel con la casilla de delante es admisible.
  */
  bool EsAccesiblePorAltura(const ubicacion &actual);

  /**
  * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
  * @param actual Estado actual del agente (fila, columna, orientacion).
  * @return Estado con la fila y columna de la casilla de enfrente.
  */
  ubicacion Delante(const ubicacion &actual) const;

  /**
  * @brief Comprueba si una celda es de tipo transitable por defecto.
  * @param c Carácter que representa el tipo de superficie.
  * @return true si es camino ('C'), zapatillas ('D') o meta ('U').
  */
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

private:
  // =========================================================================
  // VARIABLES DE ESTADO (PUEDEN SER EXTENDIDAS POR EL ALUMNO)
  // =========================================================================

  // =========================================================
  // === VARIABLES Y ESTRUCTURAS NIVEL 0  ====================
  // =========================================================
  // Variables para el Nivel 0
  Action last_action;
  bool tiene_zapatillas;
  int giros_sin_avanzar_n0;
  bool girar_derecha_n0;
  int turnos_viendo_ingeniero_n0;
  int retroceder_n0;  // Turnos que quedan de retroceso

  // Métodos auxiliares para el Nivel 0
  char ViablePorAltura(char casilla, int dif); 
  int VeoCasillaInteresante(char i, char c, char d);



  // =========================================================
  // === VARIABLES Y ESTRUCTURAS NIVEL 1  ====================
  // =========================================================
  // Variables para el Nivel 1
  int giro45Izq; // Indica el número de giros a la izquierda que quedan por dar
  int matriz_visitas[200][200];

  // Métodos auxiliares para el Nivel 1
  bool es_transitable_N1(unsigned char c, bool zap) const; // El técnico usa zap para el bosque
  char ViablePorAltura_N1(char casilla, int dif); 
  int VeoCasillaInteresante_N1(char i, char c, char d, bool zap);

  // =========================================================
  // === VARIABLES Y ESTRUCTURAS NIVEL 2 (DELIBERATIVO) ======
  // =========================================================
  bool hay_plan;
  std::list<Action> plan;

  struct Estado {
    int f;
    int c;
    Orientacion brujula;

    bool operator<(const Estado& otro) const {
      if (f != otro.f) return f < otro.f;
      if (c != otro.c) return c < otro.c;

      return brujula < otro.brujula;
    }

    bool operator==(const Estado& otro) const {
      return f == otro.f && c == otro.c && brujula == otro.brujula;
    }
  };

  struct Nodo {
    Estado st;
    std::list<Action> secuencia;
  };

  // Funciones del cerebro
  bool EncontrarPlan_N2(const Estado& inicio, int dest_f, int dest_c, std::list<Action>& plan_resultante, bool aguaPermitida);
  Estado AplicaAccion_N2(const Estado& st, Action act);
  bool EsValida_N2(const Estado& st, Action act, bool aguaPermitida);


  // =========================================================
  // === VARIABLES Y ESTRUCTURAS NIVEL 3 (A-ESTRELLA) ========
  // =========================================================
  struct EstadoN3 {
    int f;
    int c;
    Orientacion brujula;
    bool zapatillas; // Vital para Nivel 3

    bool operator<(const EstadoN3& otro) const {
      if (f != otro.f) return f < otro.f;
      if (c != otro.c) return c < otro.c;
      if (brujula != otro.brujula) return brujula < otro.brujula;
      
      return zapatillas < otro.zapatillas;
    }

    bool operator==(const EstadoN3& otro) const {
      return f == otro.f && c == otro.c && brujula == otro.brujula && zapatillas == otro.zapatillas;
    }
  };

  struct NodoN3 {
    EstadoN3 st;
    std::list<Action> secuencia;
    int coste_g; 
    int coste_h; 
    int f() const { return coste_g + coste_h; } 

    bool operator>(const NodoN3& otro) const {
      return f() > otro.f(); 
    }
  };

  // Funciones del cerebro A* (Usan EstadoN3)
  bool EncontrarPlan_N3(const EstadoN3& inicio, int dest_f, int dest_c, std::list<Action>& plan_resultante, bool ignorar_entidades = false, bool parar_adyacente = false);
  EstadoN3 AplicaAccion_N3(const EstadoN3& st, Action act);
  bool EsValida_N3(const EstadoN3& st, Action act, bool ignorar_entidades = false);
  
  int CostoBateria_N3(const EstadoN3& st, Action act);
  int Heuristica(const EstadoN3& actual, int dest_f, int dest_c);

  // =========================================================
  // === VARIABLES NIVEL 5 (MÁQUINA DE ESTADOS) ==============
  // =========================================================
  int tramo_n5 = 0;
  bool acabo_de_instalar_n5 = false;
  bool terraformado_n5 = false;
  std::vector<Paso> plan_n5;

  // El clon del cerebro del Ingeniero para leerle la mente
  struct EstadoN4_Tecnico {
    int f, c, h;
    bool operator<(const EstadoN4_Tecnico& otro) const {
      if (f != otro.f) return f < otro.f;
      if (c != otro.c) return c < otro.c;

      return h < otro.h;
    }
  };

  struct NodoN4_Tecnico {
    EstadoN4_Tecnico st;
    std::list<Paso> secuencia;
    int impacto = 0 ; // Impacto ecológico acumulado

    bool operator>(const NodoN4_Tecnico& otro) const {
      int coste_a = (int)secuencia.size() * 10000 + impacto;
      int coste_b = (int)otro.secuencia.size() * 10000 + otro.impacto;
      return coste_a > coste_b;
    }
  };

  bool EncontrarPlan_N5_Arquitecto(int start_f, int start_c, std::list<Paso>& plan_resultante, int limite_eco);

  // =========================================================
  // === MÁQUINA DE ESTADOS NIVEL 5 (OPERARIO) ==============
  // =========================================================
  enum EstadoObraTec { TEC_ESPERAR_AVISO, TEC_IR_CASILLA, TEC_ALINEARSE };
  EstadoObraTec estado_obra_tec = TEC_ESPERAR_AVISO;

  // =========================================================
  // === NIVEL 6 (COOPERACIÓN COMPLETA) =========================
  // =========================================================
  int estado_n6 = 0;
  int destn6_f = -1, destn6_c = -1;
  int intento_orbita_n6 = 0; // Qué casilla adyacente estamos probando (0-3)
};

#endif