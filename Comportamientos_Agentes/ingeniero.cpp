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

char ComportamientoIngeniero::ViablePorAltura(char casilla, int dif, bool zap) {
  if (abs(dif) <= 1 || (zap && abs(dif) <= 2)) return casilla;
  else return 'P'; // Si no es viable, la tratamos como un precipicio
}

int ComportamientoIngeniero::VeoCasillaInteresante(char i, char c, char d, bool zap) {
  if (c == 'U') return 2;
  else if (i == 'U') return 1;
  else if (d == 'U') return 3;
  else if (!zap) {
    if (c == 'D') return 2;
    else if (i == 'D') return 1;
    else if (d == 'D') return 3;
  }
  if (c == 'C') return 2;
  else if (i == 'C') return 1;
  else if (d == 'C') return 3;
  else return 0;
}

// Filtro Nivel 1 Ingeniero
char ViablePorAlturaI_Nivel1(char casilla, int dif) {
    if (casilla == 'P' || casilla == 'M' || casilla == 'B' || casilla == 'A' || casilla == 'H') return 'P';
    if (abs(dif) <= 1) return casilla;
    return 'P';
}

// Curiosidad Nivel 1 Ingeniero
int VeoCasillaInteresanteI_Nivel1(char i, char c, char d) {
    if (c != 'P') return 2; // 1. Recto
    if (i != 'P') return 1; // 2. Izquierda (ZURDO)
    if (d != 'P') return 3; // 3. Derecha
    return 0;
}

// Niveles iniciales (Comportamientos reactivos simples)
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_0(Sensores sensores) {
  Action accion = IDLE;

  ActualizarMapa(sensores);

  // Actualización de variables de estado
  if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

  // Definición del comportamiento
  if (sensores.superficie[0] == 'U') { 
    return IDLE; // Llegué a la meta
  }

  // Comprobar viabilidad de las casillas frontales
  char i = ViablePorAltura(sensores.superficie[1], sensores.cota[1] - sensores.cota[0], tiene_zapatillas);
  char c = ViablePorAltura(sensores.superficie[2], sensores.cota[2] - sensores.cota[0], tiene_zapatillas);
  char d = ViablePorAltura(sensores.superficie[3], sensores.cota[3] - sensores.cota[0], tiene_zapatillas);

  // Lógica anticolisión: El Ingeniero es educado. 
  // Si ve al Técnico en alguna de esas casillas, la trata como si fuera un precipicio/muro ('P')
  if (sensores.agentes[1] == 't') i = 'P';
  if (sensores.agentes[2] == 't') c = 'P';
  if (sensores.agentes[3] == 't') d = 'P';

  int pos = VeoCasillaInteresante(i, c, d, tiene_zapatillas);

  switch (pos) {
    case 2: accion = WALK; break;
    case 1: accion = TURN_SL; break;
    case 3: accion = TURN_SR; break;
    default: accion = TURN_SL; break; // Al no ver camino (porque el técnico lo tapa), girará
  }

  last_action = accion;
  return accion;
}




// --- FUNCIONES AUXILIARES NIVEL 1 (INGENIERO) ---
bool ComportamientoIngeniero::es_transitable_N1(unsigned char c) const {
  // Ahora el sendero 'S' y los puestos base 'X' también son válidos para explorar
  return (c == 'C' || c == 'S' || c == 'D' || c == 'U' || c == 'X');
}

char ComportamientoIngeniero::ViablePorAltura_N1(char casilla, int dif, bool zap) {
  if (abs(dif) <= 1 || (zap && abs(dif) <= 2)) return casilla;
  else return 'P'; 
}

int ComportamientoIngeniero::VeoCasillaInteresante_N1(char i, char c, char d, bool zap) {
  if (es_transitable_N1(c)) return 2; // 1º Frente
  if (es_transitable_N1(i)) return 1; // 2º Izquierda
  if (es_transitable_N1(d)) return 3; // 3º Derecha
  return 0;
}

bool ComportamientoIngeniero::es_camino(unsigned char c) const {
  return (c == 'C' || c == 'D' || c == 'U');
}

/**
 * @brief Comportamiento reactivo del ingeniero para el Nivel 1.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
*/
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_1(Sensores sensores) {
  // INICIO DEL MÉTODO ComportamientoIngenieroNivel_1
  Action accion = IDLE;
  ActualizarMapa(sensores);

  if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

  // Si estamos en medio de un giro de 90º, lo terminamos
  if (giro45Izq > 0) {
      giro45Izq--;
      last_action = TURN_SL; // El ingeniero evade por la Izquierda
      return TURN_SL;
  }

  // 1. ANOTAMOS QUE ACABAMOS DE PISAR ESTA CASILLA
  matriz_visitas[sensores.posF][sensores.posC]++;

  // 2. VISIÓN Y ANTICOLISIÓN (Igual que antes)
  // (Nota: para el técnico, quita tiene_zapatillas de ViablePorAltura_N1)
  char i = ViablePorAltura_N1(sensores.superficie[1], sensores.cota[1] - sensores.cota[0], tiene_zapatillas);
  char c = ViablePorAltura_N1(sensores.superficie[2], sensores.cota[2] - sensores.cota[0], tiene_zapatillas);
  char d = ViablePorAltura_N1(sensores.superficie[3], sensores.cota[3] - sensores.cota[0], tiene_zapatillas);

  if (sensores.agentes[1] != '_') i = 'P';
  if (sensores.agentes[2] != '_') c = 'P';
  if (sensores.agentes[3] != '_') d = 'P';

  // 3. CALCULAR COORDENADAS DE LAS CASILLAS ADYACENTES
  ubicacion actual = {sensores.posF, sensores.posC, sensores.rumbo};
  ubicacion u_frente = Delante(actual);
  
  ubicacion u_izq = actual;
  u_izq.brujula = (Orientacion)((actual.brujula + 7) % 8);
  u_izq = Delante(u_izq);
  
  ubicacion u_der = actual;
  u_der.brujula = (Orientacion)((actual.brujula + 1) % 8);
  u_der = Delante(u_der);

  // 4. LEER EL MAPA DE FEROMONAS (999999 si no es transitable)
  // (Nota: para el tecnico, añade 'tiene_zapatillas' dentro de es_transitable_N1)
  int vis_frente = es_transitable_N1(c) ? matriz_visitas[u_frente.f][u_frente.c] : 999999;
  int vis_izq = es_transitable_N1(i) ? matriz_visitas[u_izq.f][u_izq.c] : 999999;
  int vis_der = es_transitable_N1(d) ? matriz_visitas[u_der.f][u_der.c] : 999999;

  // 5. ENCONTRAR LA RUTA MENOS PISADA
  int min_visitas = vis_frente;
  if (vis_izq < min_visitas) min_visitas = vis_izq;
  if (vis_der < min_visitas) min_visitas = vis_der;

  int pos = 0;
  if (min_visitas == 999999) {
      pos = 0; // Totalmente bloqueados
  } else if (vis_frente == min_visitas) {
      pos = 2; // ¡SÚPER CLAVE! En caso de empate, vamos recto para no gastar batería girando.
  } else if (vis_izq == min_visitas) {
      pos = 1;
  } else {
      pos = 3;
  }

  // EL SWITCH FINAL 
  switch (pos) {
    case 2: accion = WALK; break;
    case 1: accion = TURN_SL; break;
    case 3: accion = TURN_SR; break;
    default: 
        // Callejón sin salida: damos la vuelta
        giro45Izq = 3; 
        accion = TURN_SL; 
        break; 
  }

  last_action = accion;
  return accion;
}






bool ComportamientoIngeniero::EncontrarPlan_N2(const Estado& inicio, int dest_f, int dest_c, std::list<Action>& plan_resultante) {
    plan_resultante.clear();
    std::queue<Nodo> abiertos;
    std::set<Estado> cerrados;

    Nodo n_inicial;
    n_inicial.st = inicio;
    abiertos.push(n_inicial);
    cerrados.insert(inicio);

    while (!abiertos.empty()) {
        Nodo actual = abiertos.front();
        abiertos.pop();

        // LA META AHORA SON LAS COORDENADAS
        if (actual.st.f == dest_f && actual.st.c == dest_c) {
            plan_resultante = actual.secuencia;
            return true;
        }

        Action acciones[] = {WALK, JUMP, TURN_SL, TURN_SR};
        for (Action accion : acciones) {
            if (EsValida_N2(actual.st, accion)) {
                Estado siguiente = AplicaAccion_N2(actual.st, accion);
                if (cerrados.find(siguiente) == cerrados.end()) {
                    cerrados.insert(siguiente);
                    Nodo hijo;
                    hijo.st = siguiente;
                    hijo.secuencia = actual.secuencia;
                    hijo.secuencia.push_back(accion);
                    abiertos.push(hijo);
                }
            }
        }
    }
    return false;
}

// =========================================================
// === MOTOR DE BÚSQUEDA NIVEL 2 (INGENIERO) ===
// =========================================================

ComportamientoIngeniero::Estado ComportamientoIngeniero::AplicaAccion_N2(const Estado& st, Action act) {
    Estado nuevo = st;
    if (act == TURN_SL) {
        nuevo.brujula = (Orientacion)((nuevo.brujula + 7) % 8);
    } else if (act == TURN_SR) {
        nuevo.brujula = (Orientacion)((nuevo.brujula + 1) % 8);
    } else if (act == WALK) {
        switch (nuevo.brujula) {
            case norte: nuevo.f--; break;
            case noreste: nuevo.f--; nuevo.c++; break;
            case este: nuevo.c++; break;
            case sureste: nuevo.f++; nuevo.c++; break;
            case sur: nuevo.f++; break;
            case suroeste: nuevo.f++; nuevo.c--; break;
            case oeste: nuevo.c--; break;
            case noroeste: nuevo.f--; nuevo.c--; break;
        }
    } else if (act == JUMP) {
        // El salto avanza 2 casillas de golpe
        switch (nuevo.brujula) {
            case norte: nuevo.f -= 2; break;
            case noreste: nuevo.f -= 2; nuevo.c += 2; break;
            case este: nuevo.c += 2; break;
            case sureste: nuevo.f += 2; nuevo.c += 2; break;
            case sur: nuevo.f += 2; break;
            case suroeste: nuevo.f += 2; nuevo.c -= 2; break;
            case oeste: nuevo.c -= 2; break;
            case noroeste: nuevo.f -= 2; nuevo.c -= 2; break;
        }
    }
    return nuevo;
}

bool ComportamientoIngeniero::EsValida_N2(const Estado& st, Action act) {
    if (act == TURN_SL || act == TURN_SR) return true;

    if (act == WALK || act == JUMP) {
        Estado destino = AplicaAccion_N2(st, act);
        
        // 1. Comprobar límites del mapa
        if (destino.f < 0 || destino.f >= mapaResultado.size() || 
            destino.c < 0 || destino.c >= mapaResultado[0].size()) {
            return false;
        }

        // 2. Comprobar tipo de terreno destino
        unsigned char tipo_terreno = mapaResultado[destino.f][destino.c];
        if (!es_transitable_N1(tipo_terreno)) return false;

        // 3. Comprobar desniveles (Caminar tolera 1, Saltar tolera 2)
        int cota_origen = mapaCotas[st.f][st.c];
        int cota_destino = mapaCotas[destino.f][destino.c];
        
        if (act == WALK && abs(cota_destino - cota_origen) > 1) return false;
        if (act == JUMP && abs(cota_destino - cota_origen) > 2) return false;

        return true;
    }
    return false;
}

// Niveles avanzados (Uso de búsqueda)
/**
 * @brief Comportamiento del ingeniero para el Nivel 2 (búsqueda).
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_2(Sensores sensores) {
  Action accion = IDLE;

  if (!hay_plan) {
      Estado estado_inicial;
      estado_inicial.f = sensores.posF;
      estado_inicial.c = sensores.posC;
      estado_inicial.brujula = sensores.rumbo;

      // Probamos con estas variables que suelen ser las estándar del guion
      hay_plan = EncontrarPlan_N2(estado_inicial, sensores.destF, sensores.destC, plan);

      if (!hay_plan) return IDLE;

  // FASE 2: EJECUCIÓN (Consumir el plan paso a paso)
  if (hay_plan && !plan.empty()) {
      accion = plan.front(); // Miramos la primera acción
      plan.pop_front();      // La borramos de la lista para no repetirla
  }

  
  return accion;
  }
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

