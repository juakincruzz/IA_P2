#include "tecnico.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <queue>
#include <set>
#include <map>
#include <vector>



using namespace std;

// =========================================================================
// ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
// =========================================================================

Action ComportamientoTecnico::think(Sensores sensores) {
  Action accion = IDLE;


  // Decisión del agente según el nivel
  switch (sensores.nivel) {
    case 0: accion = ComportamientoTecnicoNivel_0(sensores); break;
    case 1: accion = ComportamientoTecnicoNivel_1(sensores); break;
    case 2: accion = ComportamientoTecnicoNivel_2(sensores); break;
    case 3: accion = ComportamientoTecnicoNivel_3(sensores); break;
    case 4: accion = ComportamientoTecnicoNivel_4(sensores); break;
    case 5: accion = ComportamientoTecnicoNivel_5(sensores); break;
    case 6: accion = ComportamientoTecnicoNivel_6(sensores); break;
  }

  return accion;
}

// El Técnico solo puede superar desniveles de máximo 1
char ViablePorAlturaT(char casilla, int dif) {
    if (abs(dif) <= 1) {
        return casilla;
    } else {
        return 'P';
    }
}

// Nivel 1: Filtro de viabilidad (Terreno + Altura para el Ingeniero)
char ViablePorAlturaT_Nivel1(char casilla, int dif) {
    // Permitimos la hierba ('H') como último recurso para el Ingeniero
    if (casilla == 'P' || casilla == 'M' || casilla == 'B' || casilla == 'A') return 'P';
    if (abs(dif) <= 1) return casilla;
    return 'P';
}

// Evaluamos qué hay delante (ignoramos las zapatillas para el Técnico en este nivel)
int VeoCasillaInteresanteT(char i, char c, char d) {
    if (c == 'U') return 2;
    else if (i == 'U') return 1;
    else if (d == 'U') return 3;
    
    if (c == 'C') return 2;
    else if (i == 'C') return 1;
    else if (d == 'C') return 3;
    
    else return 0;
}

// Nivel 1: Evaluamos qué es más interesante para explorar
int VeoCasillaInteresanteT_Nivel1(char i, char c, char d) {
    bool izq = (i == 'C' || i == 'S');
    bool rec = (c == 'C' || c == 'S');
    bool der = (d == 'C' || d == 'S');

    // 1. EL SECRETO: Si estamos en una bifurcación (varios caminos posibles), elegimos al azar
    if (izq && der && rec) {
        int r = rand() % 3;
        if (r == 0) return 1;
        if (r == 1) return 2;
        return 3;
    }
    if (izq && der && !rec) return (rand() % 2 == 0) ? 1 : 3; // Cruce en T
    if (izq && !der && rec) return (rand() % 2 == 0) ? 1 : 2; // Desvío a la izquierda
    if (!izq && der && rec) return (rand() % 2 == 0) ? 2 : 3; // Desvío a la derecha

    // 2. Si no hay cruces, simplemente seguimos el camino
    if (rec) return 2;
    if (izq) return 1;
    if (der) return 3;

    // 3. PLAN B: Si no hay caminos, nos metemos por la hierba ('H')
    bool izq_h = (i == 'H');
    bool rec_h = (c == 'H');
    bool der_h = (d == 'H');

    if (rec_h) return 2;
    if (izq_h && der_h) return (rand() % 2 == 0) ? 1 : 3;
    if (izq_h) return 1;
    if (der_h) return 3;

    return 0; // Callejón sin salida total
}

// Niveles del técnico
Action ComportamientoTecnico::ComportamientoTecnicoNivel_0(Sensores sensores) {
    Action accion = IDLE;

    // Fase 1: Actualizar mapa
    ActualizarMapa(sensores);

    // Fase 2: Comportamiento
    if (sensores.superficie[0] == 'U') {
        return IDLE; // Llegó a la meta
    }

    // Comprobamos viabilidad de altura (sin zapatillas)
    char i = ViablePorAlturaT(sensores.superficie[1], sensores.cota[1] - sensores.cota[0]);
    char c = ViablePorAlturaT(sensores.superficie[2], sensores.cota[2] - sensores.cota[0]);
    char d = ViablePorAlturaT(sensores.superficie[3], sensores.cota[3] - sensores.cota[0]);

    int pos = VeoCasillaInteresanteT(i, c, d);

    switch (pos) {
        case 2: accion = WALK; break;
        case 1: accion = TURN_SL; break;
        case 3: accion = TURN_SR; break;
        default: accion = TURN_SL; break; // Buscar nuevo camino
    }

    last_action = accion;
    return accion;
}

/**
 * @brief Comprueba si una celda es de tipo camino transitable.
 * @param c Carácter que representa el tipo de superficie.
 * @return true si es camino ('C'), zapatillas ('D') o meta ('U').
 */
bool ComportamientoTecnico::es_camino(unsigned char c) const {
  return (c == 'C' || c == 'D' || c == 'U');
}


/**
 * @brief Comportamiento reactivo del técnico para el Nivel 1.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_1(Sensores sensores) {
    ActualizarMapa(sensores);
    
    // Protocolo de supervivencia (nos sentamos si hay poca energía)
    if (sensores.energia <= 50) return IDLE;

    Action accion = IDLE;

    if (sensores.choque) {
        accion = (rand() % 2 == 0) ? TURN_SL : TURN_SR;
        last_action = accion;
        return accion;
    }

    char i = ViablePorAlturaT_Nivel1(sensores.superficie[1], sensores.cota[1] - sensores.cota[0]);
    char c = ViablePorAlturaT_Nivel1(sensores.superficie[2], sensores.cota[2] - sensores.cota[0]);
    char d = ViablePorAlturaT_Nivel1(sensores.superficie[3], sensores.cota[3] - sensores.cota[0]);

    int pos = VeoCasillaInteresanteT_Nivel1(i, c, d);

    if (pos == 2) accion = WALK;
    else if (pos == 1) accion = TURN_SL;
    else if (pos == 3) accion = TURN_SR;
    else {
        // ANTIBUCLE en callejones: forzamos dar la media vuelta completa
        if (last_action == TURN_SL) accion = TURN_SL;
        else if (last_action == TURN_SR) accion = TURN_SR;
        else accion = (rand() % 2 == 0) ? TURN_SL : TURN_SR;
    }

    last_action = accion;
    return accion;
}

/**
 * @brief Comportamiento del técnico para el Nivel 2.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_2(Sensores sensores) {
  return IDLE;
}

/**
 * @brief Comportamiento del técnico para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_3(Sensores sensores) {
  if (!hayPlan) {
        estado origen;
        origen.fila = sensores.posF;
        origen.columna = sensores.posC;
        origen.orientacion = sensores.rumbo;

        estado destino;
        destino.fila = sensores.BelPosF;
        destino.columna = sensores.BelPosC;

        plan = AEstrella(origen, destino);
        hayPlan = true;
    }

    Action accion = IDLE;
    if (hayPlan && !plan.empty()) {
        accion = plan.front();
        plan.pop_front();
    }
  return accion;
}

/**
 * @brief Comportamiento del técnico para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_4(Sensores sensores) {
  return IDLE;
}

/**
 * @brief Comportamiento del técnico para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_5(Sensores sensores) {
  static int dest_f = -1;
    static int dest_c = -1;

    if (sensores.tiempo == 0) {
        estado_obra_tec = TEC_ESPERAR_AVISO;
        ruta_actual_tec.clear();
        dest_f = -1;
        dest_c = -1;
    }

    // ¡NUEVO!: El Técnico siempre escucha al Jefe, pase lo que pase.
    // Si el Jefe se mueve al siguiente tubo, actualizamos el destino al vuelo.
    if (sensores.venpaca) {
        if (dest_f != sensores.GotoF || dest_c != sensores.GotoC) {
            dest_f = sensores.GotoF;
            dest_c = sensores.GotoC;
            cout << "Operario: ¡El Jefe se ha movido! Persiguiendo hacia: " << dest_f << "," << dest_c << endl;
            estado_obra_tec = TEC_IR_CASILLA;
            ruta_actual_tec.clear(); // Borramos la ruta vieja
        }
    }

    switch(estado_obra_tec) {

        case TEC_ESPERAR_AVISO:
            return IDLE; 

        case TEC_IR_CASILLA:
            // Si estamos a distancia 1 (adyacentes), paramos y nos alineamos
            if (abs(sensores.posF - dest_f) + abs(sensores.posC - dest_c) <= 1) {
                cout << "Operario: ¡Estoy al lado! Empezando maniobra de alineación." << endl;
                estado_obra_tec = TEC_ALINEARSE;
                return IDLE;
            }

            // Si no tenemos ruta, calculamos con A*
            if (ruta_actual_tec.empty()) {
                estado origen; 
                origen.fila = sensores.posF; 
                origen.columna = sensores.posC; 
                origen.orientacion = sensores.rumbo;
                
                estado destino; 
                destino.fila = dest_f; 
                destino.columna = dest_c;
                
                ruta_actual_tec = AEstrella(origen, destino);
                
                // Si falla el A* (quizás el jefe está abriendo camino con DIG/RAISE)
                // Nos quedamos quietos este instante sin atascar la consola
                if (ruta_actual_tec.empty()) return IDLE;
            }

            // Damos el siguiente paso
            if (!ruta_actual_tec.empty()) {
                Action a = ruta_actual_tec.front();
                ruta_actual_tec.pop_front();
                return a;
            }
            return IDLE;

        case TEC_ALINEARSE:
            // Buscamos al Ingeniero ('i')
            if (sensores.agentes[2] == 'i') {
                if (sensores.enfrente) {
                    cout << "Operario: ¡Contacto visual perfecto! INSTALANDO TUBERÍA." << endl;
                    estado_obra_tec = TEC_ESPERAR_AVISO;
                    return INSTALL;
                } else {
                    return IDLE; 
                }
            } else {
                return TURN_SR; // Giramos buscando al jefe
            }
    }

    return IDLE;
}

/**
 * @brief Comportamiento del técnico para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_6(Sensores sensores) {
  return IDLE;
}




// =========================================================================
// FUNCIONES PROPORCIONADAS
// =========================================================================

/**
 * @brief Actualiza el mapaResultado y mapaCotas con la información de los sensores.
 * @param sensores Datos actuales de los sensores.
 */
void ComportamientoTecnico::ActualizarMapa(Sensores sensores) {
  mapaResultado[sensores.posF][sensores.posC] = sensores.superficie[0];
  mapaCotas[sensores.posF][sensores.posC] = sensores.cota[0];

  int pos = 1;
  switch (sensores.rumbo) {
    case norte:
      for (int j = 1; j < 4; j++)
        for (int i = -j; i <= j; i++) {
          mapaResultado[sensores.posF - j][sensores.posC + i] = sensores.superficie[pos];
          mapaCotas[sensores.posF - j][sensores.posC + i] = sensores.cota[pos++];
        }
      break;
    case noreste:
      mapaResultado[sensores.posF - 1][sensores.posC] = sensores.superficie[1];
      mapaCotas[sensores.posF - 1][sensores.posC] = sensores.cota[1];
      mapaResultado[sensores.posF - 1][sensores.posC + 1] = sensores.superficie[2];
      mapaCotas[sensores.posF - 1][sensores.posC + 1] = sensores.cota[2];
      mapaResultado[sensores.posF][sensores.posC + 1] = sensores.superficie[3];
      mapaCotas[sensores.posF][sensores.posC + 1] = sensores.cota[3];
      mapaResultado[sensores.posF - 2][sensores.posC] = sensores.superficie[4];
      mapaCotas[sensores.posF - 2][sensores.posC] = sensores.cota[4];
      mapaResultado[sensores.posF - 2][sensores.posC + 1] = sensores.superficie[5];
      mapaCotas[sensores.posF - 2][sensores.posC + 1] = sensores.cota[5];
      mapaResultado[sensores.posF - 2][sensores.posC + 2] = sensores.superficie[6];
      mapaCotas[sensores.posF - 2][sensores.posC + 2] = sensores.cota[6];
      mapaResultado[sensores.posF - 1][sensores.posC + 2] = sensores.superficie[7];
      mapaCotas[sensores.posF - 1][sensores.posC + 2] = sensores.cota[7];
      mapaResultado[sensores.posF][sensores.posC + 2] = sensores.superficie[8];
      mapaCotas[sensores.posF][sensores.posC + 2] = sensores.cota[8];
      mapaResultado[sensores.posF - 3][sensores.posC] = sensores.superficie[9];
      mapaCotas[sensores.posF - 3][sensores.posC] = sensores.cota[9];
      mapaResultado[sensores.posF - 3][sensores.posC + 1] = sensores.superficie[10];
      mapaCotas[sensores.posF - 3][sensores.posC + 1] = sensores.cota[10];
      mapaResultado[sensores.posF - 3][sensores.posC + 2] = sensores.superficie[11];
      mapaCotas[sensores.posF - 3][sensores.posC + 2] = sensores.cota[11];
      mapaResultado[sensores.posF - 3][sensores.posC + 3] = sensores.superficie[12];
      mapaCotas[sensores.posF - 3][sensores.posC + 3] = sensores.cota[12];
      mapaResultado[sensores.posF - 2][sensores.posC + 3] = sensores.superficie[13];
      mapaCotas[sensores.posF - 2][sensores.posC + 3] = sensores.cota[13];
      mapaResultado[sensores.posF - 1][sensores.posC + 3] = sensores.superficie[14];
      mapaCotas[sensores.posF - 1][sensores.posC + 3] = sensores.cota[14];
      mapaResultado[sensores.posF][sensores.posC + 3] = sensores.superficie[15];
      mapaCotas[sensores.posF][sensores.posC + 3] = sensores.cota[15];
      break;
    case este:
      for (int j = 1; j < 4; j++)
        for (int i = -j; i <= j; i++) {
          mapaResultado[sensores.posF + i][sensores.posC + j] = sensores.superficie[pos];
          mapaCotas[sensores.posF + i][sensores.posC + j] = sensores.cota[pos++];
        }
      break;
    case sureste:
      mapaResultado[sensores.posF][sensores.posC + 1] = sensores.superficie[1];
      mapaCotas[sensores.posF][sensores.posC + 1] = sensores.cota[1];
      mapaResultado[sensores.posF + 1][sensores.posC + 1] = sensores.superficie[2];
      mapaCotas[sensores.posF + 1][sensores.posC + 1] = sensores.cota[2];
      mapaResultado[sensores.posF + 1][sensores.posC] = sensores.superficie[3];
      mapaCotas[sensores.posF + 1][sensores.posC] = sensores.cota[3];
      mapaResultado[sensores.posF][sensores.posC + 2] = sensores.superficie[4];
      mapaCotas[sensores.posF][sensores.posC + 2] = sensores.cota[4];
      mapaResultado[sensores.posF + 1][sensores.posC + 2] = sensores.superficie[5];
      mapaCotas[sensores.posF + 1][sensores.posC + 2] = sensores.cota[5];
      mapaResultado[sensores.posF + 2][sensores.posC + 2] = sensores.superficie[6];
      mapaCotas[sensores.posF + 2][sensores.posC + 2] = sensores.cota[6];
      mapaResultado[sensores.posF + 2][sensores.posC + 1] = sensores.superficie[7];
      mapaCotas[sensores.posF + 2][sensores.posC + 1] = sensores.cota[7];
      mapaResultado[sensores.posF + 2][sensores.posC] = sensores.superficie[8];
      mapaCotas[sensores.posF + 2][sensores.posC] = sensores.cota[8];
      mapaResultado[sensores.posF][sensores.posC + 3] = sensores.superficie[9];
      mapaCotas[sensores.posF][sensores.posC + 3] = sensores.cota[9];
      mapaResultado[sensores.posF + 1][sensores.posC + 3] = sensores.superficie[10];
      mapaCotas[sensores.posF + 1][sensores.posC + 3] = sensores.cota[10];
      mapaResultado[sensores.posF + 2][sensores.posC + 3] = sensores.superficie[11];
      mapaCotas[sensores.posF + 2][sensores.posC + 3] = sensores.cota[11];
      mapaResultado[sensores.posF + 3][sensores.posC + 3] = sensores.superficie[12];
      mapaCotas[sensores.posF + 3][sensores.posC + 3] = sensores.cota[12];
      mapaResultado[sensores.posF + 3][sensores.posC + 2] = sensores.superficie[13];
      mapaCotas[sensores.posF + 3][sensores.posC + 2] = sensores.cota[13];
      mapaResultado[sensores.posF + 3][sensores.posC + 1] = sensores.superficie[14];
      mapaCotas[sensores.posF + 3][sensores.posC + 1] = sensores.cota[14];
      mapaResultado[sensores.posF + 3][sensores.posC] = sensores.superficie[15];
      mapaCotas[sensores.posF + 3][sensores.posC] = sensores.cota[15];
      break;
    case sur:
      for (int j = 1; j < 4; j++)
        for (int i = -j; i <= j; i++) {
          mapaResultado[sensores.posF + j][sensores.posC - i] = sensores.superficie[pos];
          mapaCotas[sensores.posF + j][sensores.posC - i] = sensores.cota[pos++];
        }
      break;
    case suroeste:
      mapaResultado[sensores.posF + 1][sensores.posC] = sensores.superficie[1];
      mapaCotas[sensores.posF + 1][sensores.posC] = sensores.cota[1];
      mapaResultado[sensores.posF + 1][sensores.posC - 1] = sensores.superficie[2];
      mapaCotas[sensores.posF + 1][sensores.posC - 1] = sensores.cota[2];
      mapaResultado[sensores.posF][sensores.posC - 1] = sensores.superficie[3];
      mapaCotas[sensores.posF][sensores.posC - 1] = sensores.cota[3];
      mapaResultado[sensores.posF + 2][sensores.posC] = sensores.superficie[4];
      mapaCotas[sensores.posF + 2][sensores.posC] = sensores.cota[4];
      mapaResultado[sensores.posF + 2][sensores.posC - 1] = sensores.superficie[5];
      mapaCotas[sensores.posF + 2][sensores.posC - 1] = sensores.cota[5];
      mapaResultado[sensores.posF + 2][sensores.posC - 2] = sensores.superficie[6];
      mapaCotas[sensores.posF + 2][sensores.posC - 2] = sensores.cota[6];
      mapaResultado[sensores.posF + 1][sensores.posC - 2] = sensores.superficie[7];
      mapaCotas[sensores.posF + 1][sensores.posC - 2] = sensores.cota[7];
      mapaResultado[sensores.posF][sensores.posC - 2] = sensores.superficie[8];
      mapaCotas[sensores.posF][sensores.posC - 2] = sensores.cota[8];
      mapaResultado[sensores.posF + 3][sensores.posC] = sensores.superficie[9];
      mapaCotas[sensores.posF + 3][sensores.posC] = sensores.cota[9];
      mapaResultado[sensores.posF + 3][sensores.posC - 1] = sensores.superficie[10];
      mapaCotas[sensores.posF + 3][sensores.posC - 1] = sensores.cota[10];
      mapaResultado[sensores.posF + 3][sensores.posC - 2] = sensores.superficie[11];
      mapaCotas[sensores.posF + 3][sensores.posC - 2] = sensores.cota[11];
      mapaResultado[sensores.posF + 3][sensores.posC - 3] = sensores.superficie[12];
      mapaCotas[sensores.posF + 3][sensores.posC - 3] = sensores.cota[12];
      mapaResultado[sensores.posF + 2][sensores.posC - 3] = sensores.superficie[13];
      mapaCotas[sensores.posF + 2][sensores.posC - 3] = sensores.cota[13];
      mapaResultado[sensores.posF + 1][sensores.posC - 3] = sensores.superficie[14];
      mapaCotas[sensores.posF + 1][sensores.posC - 3] = sensores.cota[14];
      mapaResultado[sensores.posF][sensores.posC - 3] = sensores.superficie[15];
      mapaCotas[sensores.posF][sensores.posC - 3] = sensores.cota[15];
      break;
    case oeste:
      for (int j = 1; j < 4; j++)
        for (int i = -j; i <= j; i++) {
          mapaResultado[sensores.posF - i][sensores.posC - j] = sensores.superficie[pos];
          mapaCotas[sensores.posF - i][sensores.posC - j] = sensores.cota[pos++];
        }
      break;
    case noroeste:
      mapaResultado[sensores.posF][sensores.posC - 1] = sensores.superficie[1];
      mapaCotas[sensores.posF][sensores.posC - 1] = sensores.cota[1];
      mapaResultado[sensores.posF - 1][sensores.posC - 1] = sensores.superficie[2];
      mapaCotas[sensores.posF - 1][sensores.posC - 1] = sensores.cota[2];
      mapaResultado[sensores.posF - 1][sensores.posC] = sensores.superficie[3];
      mapaCotas[sensores.posF - 1][sensores.posC] = sensores.cota[3];
      mapaResultado[sensores.posF][sensores.posC - 2] = sensores.superficie[4];
      mapaCotas[sensores.posF][sensores.posC - 2] = sensores.cota[4];
      mapaResultado[sensores.posF - 1][sensores.posC - 2] = sensores.superficie[5];
      mapaCotas[sensores.posF - 1][sensores.posC - 2] = sensores.cota[5];
      mapaResultado[sensores.posF - 2][sensores.posC - 2] = sensores.superficie[6];
      mapaCotas[sensores.posF - 2][sensores.posC - 2] = sensores.cota[6];
      mapaResultado[sensores.posF - 2][sensores.posC - 1] = sensores.superficie[7];
      mapaCotas[sensores.posF - 2][sensores.posC - 1] = sensores.cota[7];
      mapaResultado[sensores.posF - 2][sensores.posC] = sensores.superficie[8];
      mapaCotas[sensores.posF - 2][sensores.posC] = sensores.cota[8];
      mapaResultado[sensores.posF][sensores.posC - 3] = sensores.superficie[9];
      mapaCotas[sensores.posF][sensores.posC - 3] = sensores.cota[9];
      mapaResultado[sensores.posF - 1][sensores.posC - 3] = sensores.superficie[10];
      mapaCotas[sensores.posF - 1][sensores.posC - 3] = sensores.cota[10];
      mapaResultado[sensores.posF - 2][sensores.posC - 3] = sensores.superficie[11];
      mapaCotas[sensores.posF - 2][sensores.posC - 3] = sensores.cota[11];
      mapaResultado[sensores.posF - 3][sensores.posC - 3] = sensores.superficie[12];
      mapaCotas[sensores.posF - 3][sensores.posC - 3] = sensores.cota[12];
      mapaResultado[sensores.posF - 3][sensores.posC - 2] = sensores.superficie[13];
      mapaCotas[sensores.posF - 3][sensores.posC - 2] = sensores.cota[13];
      mapaResultado[sensores.posF - 3][sensores.posC - 1] = sensores.superficie[14];
      mapaCotas[sensores.posF - 3][sensores.posC - 1] = sensores.cota[14];
      mapaResultado[sensores.posF - 3][sensores.posC] = sensores.superficie[15];
      mapaCotas[sensores.posF - 3][sensores.posC] = sensores.cota[15];
      break;
  }
}



/**
 * @brief Determina si una casilla es transitable para el técnico.
 * En esta práctica, si el técnico tiene zapatillas, el bosque ('B') es transitable.
 * @param f Fila de la casilla.
 * @param c Columna de la casilla.
 * @param tieneZapatillas Indica si el agente posee las zapatillas.
 * @return true si la casilla es transitable.
 */
bool ComportamientoTecnico::EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas) {
  if (f < 0 || f >= mapaResultado.size() || c < 0 || c >= mapaResultado[0].size()) return false;
  return es_camino(mapaResultado[f][c]);  // Solo 'C', 'S', 'D', 'U' son transitables en Nivel 0
}

/**
 * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
 * Para el técnico: desnivel máximo siempre 1.
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return true si el desnivel con la casilla de delante es admisible.
 */
bool ComportamientoTecnico::EsAccesiblePorAltura(const ubicacion &actual) {
  ubicacion del = Delante(actual);
  if (del.f < 0 || del.f >= mapaCotas.size() || del.c < 0 || del.c >= mapaCotas[0].size()) return false;
  int desnivel = abs(mapaCotas[del.f][del.c] - mapaCotas[actual.f][actual.c]);
  if (desnivel > 1) return false;
  return true;
}

/**
 * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
 * Calcula la casilla frontal según la orientación actual (8 direcciones).
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return Estado con la fila y columna de la casilla de enfrente.
 */
ubicacion ComportamientoTecnico::Delante(const ubicacion &actual) const {
  ubicacion delante = actual;
  switch (actual.brujula) {
    case 0: delante.f--; break;                        // norte
    case 1: delante.f--; delante.c++; break;     // noreste
    case 2: delante.c++; break;                     // este
    case 3: delante.f++; delante.c++; break;     // sureste
    case 4: delante.f++; break;                        // sur
    case 5: delante.f++; delante.c--; break;     // suroeste
    case 6: delante.c--; break;                     // oeste
    case 7: delante.f--; delante.c--; break;     // noroeste
  }
  return delante;
}


/**
 * @brief Imprime por consola la secuencia de acciones de un plan.
 *
 * @param plan  Lista de acciones del plan.
 */
void ComportamientoTecnico::PintaPlan(const list<Action> &plan)
{
  auto it = plan.begin();
  while (it != plan.end())
  {
    if (*it == WALK)
    {
      cout << "W ";
    }
    else if (*it == JUMP)
    {
      cout << "J ";
    }
    else if (*it == TURN_SR)
    {
      cout << "r ";
    }
    else if (*it == TURN_SL)
    {
      cout << "l ";
    }
    else if (*it == COME)
    {
      cout << "C ";
    }
    else if (*it == IDLE)
    {
      cout << "I ";
    }
    else
    {
      cout << "-_ ";
    }
    it++;
  }
  cout << "( longitud " << plan.size() << ")" << endl;
}



/**
 * @brief Convierte un plan de acciones en una lista de casillas para
 *        su visualización en el mapa 2D.
 *
 * @param st    Estado de partida.
 * @param plan  Lista de acciones del plan.
 */
void ComportamientoTecnico::VisualizaPlan(const ubicacion &st,
                                            const list<Action> &plan)
{
   listaPlanCasillas.clear();
  ubicacion cst = st;

  listaPlanCasillas.push_back({cst.f, cst.c, WALK});
  auto it = plan.begin();
  while (it != plan.end())
  {

    switch (*it)
    {
    case JUMP:
      switch (cst.brujula)
      {
      case 0:
        cst.f--;
        break;
      case 1:
        cst.f--;
        cst.c++;
        break;
      case 2:
        cst.c++;
        break;
      case 3:
        cst.f++;
        cst.c++;
        break;
      case 4:
        cst.f++;
        break;
      case 5:
        cst.f++;
        cst.c--;
        break;
      case 6:
        cst.c--;
        break;
      case 7:
        cst.f--;
        cst.c--;
        break;
      }
      if (cst.f >= 0 && cst.f < mapaResultado.size() &&
          cst.c >= 0 && cst.c < mapaResultado[0].size())
        listaPlanCasillas.push_back({cst.f, cst.c, JUMP});
    case WALK:
      switch (cst.brujula)
      {
      case 0:
        cst.f--;
        break;
      case 1:
        cst.f--;
        cst.c++;
        break;
      case 2:
        cst.c++;
        break;
      case 3:
        cst.f++;
        cst.c++;
        break;
      case 4:
        cst.f++;
        break;
      case 5:
        cst.f++;
        cst.c--;
        break;
      case 6:
        cst.c--;
        break;
      case 7:
        cst.f--;
        cst.c--;
        break;
      }
      if (cst.f >= 0 && cst.f < mapaResultado.size() &&
          cst.c >= 0 && cst.c < mapaResultado[0].size())
        listaPlanCasillas.push_back({cst.f, cst.c, WALK});
      break;
    case TURN_SR:
      cst.brujula = (Orientacion) (( (int) cst.brujula + 1) % 8);
      break;
    case TURN_SL:
      cst.brujula = (Orientacion) (( (int) cst.brujula + 7) % 8);
      break;
    }
    it++;
  }
}

// 1. Coste de Avanzar (WALK) para el Técnico
int costeWALKTecnico(char superficie) {
    if (superficie == 'A') return 90;
    if (superficie == 'H') return 10;
    if (superficie == 'S') return 4;
    return 3; // Resto de casillas (C, U, D, X...)
}

// 2. Coste de Girar (TURN_SL / TURN_SR) para el Técnico
int costeGIROTecnico(char superficie) {
    if (superficie == 'A') return 5;
    if (superficie == 'H') return 2;
    if (superficie == 'S') return 1;
    return 1; // Resto de casillas
}

// 3. Heurística Admisible (Distancia de Chebyshev * Coste Mínimo)
int heuristica(const ComportamientoTecnico::estado& actual, const ComportamientoTecnico::estado& meta) {
    // La distancia máxima entre filas y columnas.
    // Lo multiplicamos por 3 porque es el coste de energía más bajo posible al avanzar (1 casilla normal = 3 de energía).
    // Así garantizamos que NUNCA sobreestimamos el coste real (heurística admisible).
    return std::max(abs(actual.fila - meta.fila), abs(actual.columna - meta.columna)) * 3;
}

list<Action> ComportamientoTecnico::AEstrella(const estado& origen, const estado& destino) {
    // La cola de prioridad: Ordena automáticamente sacando primero los nodos con menor 'f'
    priority_queue<nodo, vector<nodo>, greater<nodo>> abierta;
    
    // Diccionario de los mejores costes 'g' encontrados para cada estado
    map<estado, int> g_costes;

    nodo inicial;
    inicial.st = origen;
    inicial.coste_g = 0;
    inicial.heuristica_h = heuristica(origen, destino);
    inicial.f = inicial.coste_g + inicial.heuristica_h;

    abierta.push(inicial);
    g_costes[origen] = 0;

    while (!abierta.empty()) {
        nodo actual = abierta.top();
        abierta.pop();

        // 1. ¿Llegamos a la meta?
        if (abs(actual.st.fila - destino.fila) + abs(actual.st.columna - destino.columna) <= 1) {
            return actual.secuencia; 
        }

        // 2. Si este nodo es una versión vieja y más cara de un estado que ya mejoramos, lo saltamos
        if (actual.coste_g > g_costes[actual.st]) continue;

        char superficie_actual = mapaResultado[actual.st.fila][actual.st.columna];
        int coste_giro = costeGIROTecnico(superficie_actual);

        // --- HIJO 1: GIRAR IZQUIERDA (TURN_SL) ---
        nodo hijo_sl = actual;
        hijo_sl.st.orientacion = (actual.st.orientacion + 7) % 8;
        hijo_sl.coste_g = actual.coste_g + coste_giro;
        
        // Solo lo exploramos si es la primera vez que llegamos a este estado o si es un camino más barato
        if (g_costes.find(hijo_sl.st) == g_costes.end() || hijo_sl.coste_g < g_costes[hijo_sl.st]) {
            hijo_sl.secuencia.push_back(TURN_SL);
            hijo_sl.heuristica_h = heuristica(hijo_sl.st, destino);
            hijo_sl.f = hijo_sl.coste_g + hijo_sl.heuristica_h;
            g_costes[hijo_sl.st] = hijo_sl.coste_g;
            abierta.push(hijo_sl);
        }

        // --- HIJO 2: GIRAR DERECHA (TURN_SR) ---
        nodo hijo_sr = actual;
        hijo_sr.st.orientacion = (actual.st.orientacion + 1) % 8;
        hijo_sr.coste_g = actual.coste_g + coste_giro;
        
        if (g_costes.find(hijo_sr.st) == g_costes.end() || hijo_sr.coste_g < g_costes[hijo_sr.st]) {
            hijo_sr.secuencia.push_back(TURN_SR);
            hijo_sr.heuristica_h = heuristica(hijo_sr.st, destino);
            hijo_sr.f = hijo_sr.coste_g + hijo_sr.heuristica_h;
            g_costes[hijo_sr.st] = hijo_sr.coste_g;
            abierta.push(hijo_sr);
        }

        // --- HIJO 3: AVANZAR (WALK) ---
        int nf = actual.st.fila;
        int nc = actual.st.columna;

        switch(actual.st.orientacion) {
            case 0: nf--; break;           // Norte
            case 1: nf--; nc++; break;     // Noreste
            case 2: nc++; break;           // Este
            case 3: nf++; nc++; break;     // Sureste
            case 4: nf++; break;           // Sur
            case 5: nf++; nc--; break;     // Suroeste
            case 6: nc--; break;           // Oeste
            case 7: nf--; nc--; break;     // Noroeste
        }

        // Comprobamos límites del mapa
        if (nf >= 0 && nf < mapaResultado.size() && nc >= 0 && nc < mapaResultado[0].size()) {
            char sup_destino = mapaResultado[nf][nc];
            
            // Viabilidad física: Muros, precipicios y bosques sin zapatillas
            bool es_obstaculo = (sup_destino == 'P' || sup_destino == 'M');

            if (!es_obstaculo) {
                // Viabilidad de altura
                int dif_cota = mapaCotas[nf][nc] - mapaCotas[actual.st.fila][actual.st.columna];
                int max_desnivel = tiene_zapatillas ? 2 : 1;
                
                if (abs(dif_cota) <= max_desnivel) {
                    nodo hijo_walk = actual;
                    hijo_walk.st.fila = nf;
                    hijo_walk.st.columna = nc;
                    
                    int coste_avance = costeWALKTecnico(superficie_actual);
                    hijo_walk.coste_g = actual.coste_g + coste_avance;
                    
                    if (g_costes.find(hijo_walk.st) == g_costes.end() || hijo_walk.coste_g < g_costes[hijo_walk.st]) {
                        hijo_walk.secuencia.push_back(WALK);
                        hijo_walk.heuristica_h = heuristica(hijo_walk.st, destino);
                        hijo_walk.f = hijo_walk.coste_g + hijo_walk.heuristica_h;
                        g_costes[hijo_walk.st] = hijo_walk.coste_g;
                        abierta.push(hijo_walk);
                    }
                }
            }
        }
    }
    return list<Action>(); // Si se vacía la cola y no hay ruta
}