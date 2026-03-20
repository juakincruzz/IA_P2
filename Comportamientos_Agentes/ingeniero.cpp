#include "ingeniero.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <queue>
#include <set>
#include <cstdlib>

using namespace std;

// =========================================================================
// ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
// =========================================================================

Action ComportamientoIngeniero::think(Sensores sensores)
{
  Action accion = IDLE;

  // Decisión del agente según el nivel
  switch (sensores.nivel)
  {
  case 0:
    accion = ComportamientoIngenieroNivel_0(sensores);
    break;
  case 1:
    accion = ComportamientoIngenieroNivel_1(sensores);
    break;
  case 2:
    accion = ComportamientoIngenieroNivel_2(sensores);
    break;
  case 3:
    accion = ComportamientoIngenieroNivel_3(sensores);
    break;
  case 4:
    accion = ComportamientoIngenieroNivel_4(sensores);
    break;
  case 5:
    accion = ComportamientoIngenieroNivel_5(sensores);
    break;
  case 6:
    accion = ComportamientoIngenieroNivel_6(sensores);
    break;
  }

  return accion;
}

// Devuelve la casilla si es transitable por altura, o 'P' (precipicio) si es un muro infranqueable
char ViablePorAlturaI(char casilla, int dif, bool zap) {
    if (abs(dif) <= 1 or (zap and abs(dif) <= 2)) {
        return casilla;
    } else {
        return 'P';
    }
}

// Filtro Nivel 1 Ingeniero
char ViablePorAlturaI_Nivel1(char casilla, int dif) {
    // Permitimos la hierba ('H') como último recurso para el Ingeniero
    if (casilla == 'P' || casilla == 'M' || casilla == 'B' || casilla == 'A') return 'P';
    if (abs(dif) <= 1) return casilla;
    return 'P';
}

// Devuelve 2 (WALK), 1 (TURN_SL), 3 (TURN_SR) o 0 (nada interesante)
int VeoCasillaInteresanteI(char i, char c, char d, bool zap) {
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

// Curiosidad Nivel 1 Ingeniero
int VeoCasillaInteresanteI_Nivel1(char i, char c, char d) {
    if (c != 'P') return 2; // 1. Recto
    if (i != 'P') return 1; // 2. Izquierda (ZURDO)
    if (d != 'P') return 3; // 3. Derecha
    return 0;
}

// Niveles iniciales (Comportamientos reactivos simples)
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_0(Sensores sensores)
{
  Action accion = IDLE;

  // Fase 1: Actualización de información.
  // Pinto el mapa lo que los sensores ven.
  ActualizarMapa(sensores) ;

  // Si la casilla en la que estoy es índice 0 es una zapatilla ('D'), me la guardo.
  if (sensores.superficie[0] == 'D') tiene_zapatillas = true ;

  // Fase 2: Definición del comportamiento
    if (sensores.superficie[0] == 'U') { // Llegué a una 'U'
        return IDLE;
    }

    // Calculamos si las tres casillas de delante son viables por altura
    char i = ViablePorAlturaI(sensores.superficie[1], sensores.cota[1] - sensores.cota[0], tiene_zapatillas);
    char c = ViablePorAlturaI(sensores.superficie[2], sensores.cota[2] - sensores.cota[0], tiene_zapatillas);
    char d = ViablePorAlturaI(sensores.superficie[3], sensores.cota[3] - sensores.cota[0], tiene_zapatillas);

    // Evaluamos qué es lo más interesante de lo que SÍ podemos pisar
    int pos = VeoCasillaInteresanteI(i, c, d, tiene_zapatillas);

    switch (pos) {
        case 2:
            accion = WALK;
            break;
        case 1:
            accion = TURN_SL;
            break;
        case 3:
            accion = TURN_SR;
            break;
        default:
            accion = TURN_SL; // Si no hay nada interesante, giro para buscar
            break;
    }

    // Guardo la acción que he decidido tomar en mi memoria
    last_action = accion;

  return accion;
}

/**
 * @brief Comprueba si una celda es de tipo camino transitable.
 * @param c Carácter que representa el tipo de superficie.
 * @return true si es camino ('C'), zapatillas ('D') o meta ('U').
 */
bool ComportamientoIngeniero::es_camino(unsigned char c) const
{
  return (c == 'C' || c == 'D' || c == 'U');
}

/**
 * @brief Comportamiento reactivo del ingeniero para el Nivel 1.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
*/
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_1(Sensores sensores) {
    ActualizarMapa(sensores);
    
    // Protocolo de supervivencia (nos sentamos si hay poca energía)
    if (sensores.energia <= 50) return IDLE;

    Action accion = IDLE;

    if (sensores.choque) {
        accion = (rand() % 2 == 0) ? TURN_SL : TURN_SR;
        last_action = accion;
        return accion;
    }

    char i = ViablePorAlturaI_Nivel1(sensores.superficie[1], sensores.cota[1] - sensores.cota[0]);
    char c = ViablePorAlturaI_Nivel1(sensores.superficie[2], sensores.cota[2] - sensores.cota[0]);
    char d = ViablePorAlturaI_Nivel1(sensores.superficie[3], sensores.cota[3] - sensores.cota[0]);

    int pos = VeoCasillaInteresanteI_Nivel1(i, c, d);

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

// Niveles avanzados (Uso de búsqueda)
/**
 * @brief Comportamiento del ingeniero para el Nivel 2 (búsqueda).
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_2(Sensores sensores)
{
  Action accion = IDLE;

  // 1. Si no hay plan, es el primer instante: ¡Toca pensar!
  if (!hayPlan) {
      estado origen;
      origen.fila = sensores.posF;
      origen.columna = sensores.posC;
      origen.orientacion = sensores.rumbo;

      estado destino;
      destino.fila = sensores.BelPosF;
      destino.columna = sensores.BelPosC;

      // Llamamos al algoritmo para que nos devuelva la lista de acciones óptima
      plan = BusquedaEnAnchura(origen, destino);
      
      hayPlan = true;
    }

  // 2. Ejecutar el plan paso a paso
  if (hayPlan && !plan.empty()) {
      accion = plan.front(); // Miramos la siguiente acción
      plan.pop_front();      // La sacamos de la lista porque la vamos a ejecutar ahora mismo
  }

  return accion;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_3(Sensores sensores)
{
  return IDLE;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_4(Sensores sensores)
{
  if (!hayPlanTuberias) {
        // Planificamos desde la posición de la Belkanita
        planTuberias = PlanificaTuberias(sensores.BelPosF, sensores.BelPosC);
        
        if (!planTuberias.empty()) {
            // Le pasamos el plan al motor gráfico para que lo dibuje
            VisualizaRedTuberias(planTuberias);
        }
        hayPlanTuberias = true;
    }
    
  // El nivel 4 es solo teórico, no nos movemos.
  return IDLE;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_5(Sensores sensores)
{
  return IDLE;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_6(Sensores sensores)
{
  return IDLE;
}

// =========================================================================
// FUNCIONES PROPORCIONADAS
// =========================================================================

/**
 * @brief Actualiza el mapaResultado y mapaCotas con la información de los sensores.
 * @param sensores Datos actuales de los sensores.
 */
void ComportamientoIngeniero::ActualizarMapa(Sensores sensores)
{
  mapaResultado[sensores.posF][sensores.posC] = sensores.superficie[0];
  mapaCotas[sensores.posF][sensores.posC] = sensores.cota[0];

  int pos = 1;
  switch (sensores.rumbo)
  {
  case norte:
    for (int j = 1; j < 4; j++)
      for (int i = -j; i <= j; i++)
      {
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
      for (int i = -j; i <= j; i++)
      {
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
      for (int i = -j; i <= j; i++)
      {
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
      for (int i = -j; i <= j; i++)
      {
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
 * @brief Determina si una casilla es transitable para el ingeniero.
 * @param f Fila de la casilla.
 * @param c Columna de la casilla.
 * @param tieneZapatillas Indica si el agente posee las zapatillas.
 * @return true si la casilla es transitable (no es muro ni precipicio).
 */
bool ComportamientoIngeniero::EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas)
{
  if (f < 0 || f >= mapaResultado.size() || c < 0 || c >= mapaResultado[0].size())
    return false;
  return es_camino(mapaResultado[f][c]); // Solo 'C', 'D', 'U' son transitables en Nivel 0
}

/**
 * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
 * Para el ingeniero: desnivel máximo 1 sin zapatillas, 2 con zapatillas.
 * @param actual Estado actual del agente (fila, columna, orientacion, zap).
 * @return true si el desnivel con la casilla de delante es admisible.
 */
bool ComportamientoIngeniero::EsAccesiblePorAltura(const ubicacion &actual, bool zap)
{
  ubicacion del = Delante(actual);
  if (del.f < 0 || del.f >= mapaCotas.size() || del.c < 0 || del.c >= mapaCotas[0].size())
    return false;
  int desnivel = abs(mapaCotas[del.f][del.c] - mapaCotas[actual.f][actual.c]);
  if (zap && desnivel > 2)
    return false;
  if (!zap && desnivel > 1)
    return false;
  return true;
}

/**
 * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
 * Calcula la casilla frontal según la orientación actual (8 direcciones).
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return Estado con la fila y columna de la casilla de enfrente.
 */
ubicacion ComportamientoIngeniero::Delante(const ubicacion &actual) const
{
  ubicacion delante = actual;
  switch (actual.brujula)
  {
  case 0:
    delante.f--;
    break; // norte
  case 1:
    delante.f--;
    delante.c++;
    break; // noreste
  case 2:
    delante.c++;
    break; // este
  case 3:
    delante.f++;
    delante.c++;
    break; // sureste
  case 4:
    delante.f++;
    break; // sur
  case 5:
    delante.f++;
    delante.c--;
    break; // suroeste
  case 6:
    delante.c--;
    break; // oeste
  case 7:
    delante.f--;
    delante.c--;
    break; // noroeste
  }
  return delante;
}

/**
 * @brief Imprime por consola la secuencia de acciones de un plan.
 *
 * @param plan  Lista de acciones del plan.
 */
void ComportamientoIngeniero::PintaPlan(const list<Action> &plan)
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
 * @brief Imprime las coordenadas y operaciones de un plan de tubería.
 *
 * @param plan  Lista de pasos (fila, columna, operación),
 *              donde operacion = -1 (DIG), operación = 1 (RAISE).
 */
void ComportamientoIngeniero::PintaPlan(const list<Paso> &plan)
{
  auto it = plan.begin();
  while (it != plan.end())
  {
    cout << it->fil << ", " << it->col << " (" << it->op << ")\n";
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
void ComportamientoIngeniero::VisualizaPlan(const ubicacion &st,
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

/**
 * @brief Convierte un plan de tubería en la lista de casillas usada
 *        por el sistema de visualización.
 *
 * @param st    Estado de partida (no utilizado directamente).
 * @param plan  Lista de pasos del plan de tubería.
 */
void ComportamientoIngeniero::VisualizaRedTuberias(const list<Paso> &plan)
{
  listaCanalizacionTuberias.clear();
  auto it = plan.begin();
  while (it != plan.end())
  {
    listaCanalizacionTuberias.push_back({it->fil, it->col, it->op});
    it++;
  }
}

// =========================================================================
// ALGORITMOS DE BÚSQUEDA (NIVEL 2)
// =========================================================================

list<Action> ComportamientoIngeniero::BusquedaEnAnchura(const estado& origen, const estado& destino) {
    // La cola de estados por explorar (Abierta) y la memoria de estados ya visitados (Cerrados)
    queue<nodo> abierta;
    set<estado> cerrados;

    // Metemos el estado inicial en la cola y lo marcamos como visitado
    nodo inicial;
    inicial.st = origen;
    abierta.push(inicial);
    cerrados.insert(origen);

    while (!abierta.empty()) {
        // Sacamos el primer nodo de la cola para evaluarlo
        nodo actual = abierta.front();
        abierta.pop();

        // 1. ¿HEMOS LLEGADO A LA META?
        // Gracias a que sobrecargamos el operador == en el .hpp, esto solo comprueba fila y columna
        if (actual.st == destino) {
            return actual.secuencia; // ¡Devolvemos la lista de instrucciones ganadora!
        }

        // 2. GENERAMOS LOS HIJOS (Posibles siguientes acciones)

        // --- HIJO 1: GIRAR A LA IZQUIERDA (TURN_SL) ---
        nodo hijo_sl = actual;
        // Girar a la izquierda es restar 1 a la orientación (o sumar 7) en un reloj de 8 horas
        hijo_sl.st.orientacion = (actual.st.orientacion + 7) % 8;
        if (cerrados.find(hijo_sl.st) == cerrados.end()) {
            hijo_sl.secuencia.push_back(TURN_SL);
            cerrados.insert(hijo_sl.st);
            abierta.push(hijo_sl);
        }

        // --- HIJO 2: GIRAR A LA DERECHA (TURN_SR) ---
        nodo hijo_sr = actual;
        // Girar a la derecha es sumar 1 a la orientación
        hijo_sr.st.orientacion = (actual.st.orientacion + 1) % 8;
        if (cerrados.find(hijo_sr.st) == cerrados.end()) {
            hijo_sr.secuencia.push_back(TURN_SR);
            cerrados.insert(hijo_sr.st);
            abierta.push(hijo_sr);
        }

        // --- HIJO 3: AVANZAR (WALK) ---
        nodo hijo_walk = actual;
        int nf = actual.st.fila;
        int nc = actual.st.columna;

        // Calculamos dónde caeríamos si avanzamos según hacia dónde miramos
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

        // Comprobamos si el paso es legal (dentro del mapa)
        if (nf >= 0 && nf < mapaResultado.size() && nc >= 0 && nc < mapaResultado[0].size()) {
            unsigned char celda = mapaResultado[nf][nc];
            
            // Filtro de viabilidad: No muros, no precipicios, no agua, no bosque (asumiendo que no hay zapatillas de inicio)
            if (celda != 'P' && celda != 'M' && celda != 'B' && celda != 'A') {
                
                // Filtro de altura: Desnivel máximo de 1
                int dif_cota = mapaCotas[nf][nc] - mapaCotas[actual.st.fila][actual.st.columna];
                if (abs(dif_cota) <= 1) {
                    
                    hijo_walk.st.fila = nf;
                    hijo_walk.st.columna = nc;
                    
                    // Si nunca hemos estado en esta casilla mirando hacia allá, la añadimos
                    if (cerrados.find(hijo_walk.st) == cerrados.end()) {
                        hijo_walk.secuencia.push_back(WALK);
                        cerrados.insert(hijo_walk.st);
                        abierta.push(hijo_walk);
                    }
                }
            }
        }
    }

    // Si la cola se vacía y no encontramos la meta, devolvemos una lista vacía (no hay camino)
    return list<Action>(); 
}

list<Paso> ComportamientoIngeniero::PlanificaTuberias(int f_inicio, int c_inicio) {
    queue<nodo_tuberia> abierta;
    set<estado_tuberia> cerrados;

    // Metemos el inicio probando las 3 opciones posibles de excavación en la Belkanita
    for (int mod_inicio = -1; mod_inicio <= 1; ++mod_inicio) {
        nodo_tuberia inicial;
        inicial.st.fila = f_inicio;
        inicial.st.columna = c_inicio;
        inicial.st.mod = mod_inicio; 
        
        Paso p;
        p.fil = f_inicio;
        p.col = c_inicio;
        p.op = mod_inicio;
        inicial.secuencia.push_back(p);

        abierta.push(inicial);
        cerrados.insert(inicial.st);
    }

    // Direcciones ortogonales (Norte, Sur, Este, Oeste)
    int df[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    while (!abierta.empty()) {
        nodo_tuberia actual = abierta.front();
        abierta.pop();

        int f = actual.st.fila;
        int c = actual.st.columna;

        // ¿Llegamos a una Planta de Tratamiento?
        if (mapaResultado[f][c] == 'U') {
            return actual.secuencia; // ¡Ruta encontrada!
        }

        // Calculamos la altura real a la que está el agua en nuestra tubería actual
        int h_actual = mapaCotas[f][c] + actual.st.mod;

        // Explorar vecinos ortogonales
        for (int i = 0; i < 4; ++i) {
            int nf = f + df[i];
            int nc = c + dc[i];

            // Comprobamos límites del mapa
            if (nf >= 0 && nf < mapaResultado.size() && nc >= 0 && nc < mapaResultado[0].size()) {
                char sup = mapaResultado[nf][nc];
                
                // Las tuberías no pueden atravesar Muros ni Precipicios
                if (sup == 'M' || sup == 'P') continue;

                int h_vecino_base = mapaCotas[nf][nc];

                // Probamos a poner la tubería en el vecino con las 3 modificaciones posibles
                for (int mod_vecino = -1; mod_vecino <= 1; ++mod_vecino) {
                    int h_vecino_final = h_vecino_base + mod_vecino;

                    // REGLA DE GRAVEDAD: El agua no sube cuestas (h_actual >= h_vecino_final)
                    if (h_actual >= h_vecino_final) {
                        estado_tuberia st_hijo = {nf, nc, mod_vecino};

                        // Si es una configuración nueva, la exploramos
                        if (cerrados.find(st_hijo) == cerrados.end()) {
                            nodo_tuberia hijo = actual;
                            hijo.st = st_hijo;
                            
                            Paso p;
                            p.fil = nf;
                            p.col = nc;
                            p.op = mod_vecino;
                            hijo.secuencia.push_back(p);
                            
                            cerrados.insert(st_hijo);
                            abierta.push(hijo);
                        }
                    }
                }
            }
        }
    }
    return list<Paso>(); // No hay ruta posible
}