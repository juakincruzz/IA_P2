#include "ingeniero.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <queue>
#include <set>
#include <cstdlib>
#include <algorithm>

using namespace std;

// =========================================================================
// ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
// =========================================================================

Action ComportamientoIngeniero::think(Sensores sensores)
{
  Action accion = IDLE;

  // Decisión del agente según el nivel
  switch (sensores.nivel) {
    case 0: accion = ComportamientoIngenieroNivel_0(sensores); break;
    case 1: accion = ComportamientoIngenieroNivel_1(sensores); break;
    case 2: accion = ComportamientoIngenieroNivel_2(sensores); break;
    case 3: accion = ComportamientoIngenieroNivel_3(sensores); break;
    case 4: accion = ComportamientoIngenieroNivel_4(sensores); break;
    case 5: accion = ComportamientoIngenieroNivel_5(sensores); break;
    case 6: accion = ComportamientoIngenieroNivel_6(sensores); break;
  }

  return accion;
}

/**
  * @brief Compruebo si una casilla adyacente es accesible según la diferencia de altura.
  *        Desnivel máximo: 1 sin zapatillas, 2 con zapatillas.
  * @param casilla Tipo de terreno de la casilla destino.
  * @param dif     Diferencia de cota (destino - origen).
  * @param zap     true si el agente tiene zapatillas.
  * @return El tipo de casilla original si es accesible, 'P' si no lo es.
*/
char ComportamientoIngeniero::ViablePorAltura(char casilla, int dif, bool zap) {
  if (abs(dif) <= 1 || (zap && abs(dif) <= 2)) return casilla;

  else return 'P';
}

/**
  * @brief Evalúo las 3 casillas frontales y devuelve la dirección más interesante.
  *        Prioridad: U (meta) > D (zapatillas, si no las tiene) > C (camino).
  * @return 1=izquierda, 2=centro, 3=derecha, 0=ninguna interesante.
*/
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

// =========================================================================
// NIVEL 0 - COMPORTAMIENTO REACTIVO (Ingeniero)
// =========================================================================
// Estrategia: exploración guiada por mapa de feromonas (matriz_visitas).
//
// El agente elige la casilla adyacente menos visitada entre las transitables.
// Preferencia de terreno: C/D/U (camino) > S (sendero, si bloqueado) > H (hierba, si bloqueado).
// Cuando está fuera de camino (en H o S), se permite moverse por H/S sin esperar bloqueo
// para evitar quedarse atascado; la penalización de +50 visitas en H/S empuja al agente
// a volver a caminos 'C' en cuanto los encuentra.
//
// Mecanismos anti-bloqueo:
//   1. Búsqueda en 8 direcciones: si lleva >6 turnos sin avanzar, busca la casilla
//      transitable más cercana en cualquier orientación (prioridad C/D/U > S > H).
//   2. JUMP de emergencia: si lleva >14 turnos bloqueado, intenta saltar 2 casillas.
//   3. Alternancia de giro: cada 16 turnos sin avanzar, alterna la dirección de giro
//      por defecto para no quedar en bucle.
//
// Anticolisión: marca al Técnico como obstáculo ('P') en las 3 casillas frontales.
// =========================================================================

Action ComportamientoIngeniero::ComportamientoIngenieroNivel_0(Sensores sensores) {
  Action accion = IDLE;
  ActualizarMapa(sensores);

  // Recojo zapatillas si pisamos 'D'; paro si ya estamos en la meta
  if (sensores.superficie[0] == 'D') tiene_zapatillas = true;
  if (sensores.superficie[0] == 'U') return IDLE;

  // Actualizo feromonas y reseto contador de bloqueo tras avanzar
  matriz_visitas[sensores.posF][sensores.posC]++;
  if (last_action == WALK || last_action == JUMP) giros_sin_avanzar_n0 = 0;

  // Penalizo terreno no-camino para incentivar el retorno a 'C'
  unsigned char terreno_actual = sensores.superficie[0];
  if (terreno_actual == 'H' || terreno_actual == 'S') matriz_visitas[sensores.posF][sensores.posC] += 50;

  // Evalúo accesibilidad por altura de las 3 casillas frontales
  char ci = ViablePorAltura(sensores.superficie[1], sensores.cota[1] - sensores.cota[0], tiene_zapatillas);
  char cc = ViablePorAltura(sensores.superficie[2], sensores.cota[2] - sensores.cota[0], tiene_zapatillas);
  char cd = ViablePorAltura(sensores.superficie[3], sensores.cota[3] - sensores.cota[0], tiene_zapatillas);

  // Anticolisión: marco al Técnico como obstáculo
  if (sensores.agentes[1] == 't') ci = 'P';
  if (sensores.agentes[2] == 't') cc = 'P';
  if (sensores.agentes[3] == 't') cd = 'P';

  // Si la meta está en una casilla adyacente, voy directamente
  if (cc == 'U') { last_action = WALK;    return WALK;    }
  if (ci == 'U') { last_action = TURN_SL; return TURN_SL; }
  if (cd == 'U') { last_action = TURN_SR; return TURN_SR; }

  // Temporizador de espera (se activa ante deadlocks con el Técnico)
  if (giro45Izq > 0) {
    giro45Izq--;
    matriz_visitas[sensores.posF][sensores.posC]++;
    last_action = IDLE;
    return IDLE;
  }

  // Calculo coordenadas de las 3 casillas adyacentes
  ubicacion actual = {sensores.posF, sensores.posC, sensores.rumbo};
  ubicacion u_izq = actual;
  u_izq.brujula = (Orientacion)((actual.brujula + 7) % 8);
  u_izq = Delante(u_izq);
  ubicacion u_frente = Delante(actual);
  ubicacion u_der = actual;
  u_der.brujula = (Orientacion)((actual.brujula + 1) % 8);
  u_der = Delante(u_der);

  // Determino casillas transitables: C/D siempre; S/H solo si estoy
  // bloqueado (desesperado) o ya estoy fuera de camino
  bool desesperado = (giros_sin_avanzar_n0 > 5);
  bool fuera_de_camino = (sensores.superficie[0] == 'H' || sensores.superficie[0] == 'S');
  bool ok_i = (ci == 'C' || ci == 'D' || ((desesperado || fuera_de_camino) && (ci == 'S' || ci == 'H')));
  bool ok_c = (cc == 'C' || cc == 'D' || ((desesperado || fuera_de_camino) && (cc == 'S' || cc == 'H')));
  bool ok_d = (cd == 'C' || cd == 'D' || ((desesperado || fuera_de_camino) && (cd == 'S' || cd == 'H')));

  // Consulto feromonas de cada casilla transitable
  int vis_i = ok_i ? matriz_visitas[u_izq.f][u_izq.c]    : 999999;
  int vis_c = ok_c ? matriz_visitas[u_frente.f][u_frente.c] : 999999;
  int vis_d = ok_d ? matriz_visitas[u_der.f][u_der.c]    : 999999;

  int min_vis = min({vis_i, vis_c, vis_d});

  // Elijo dirección con menos visitas (desempate: recto > izquierda > derecha)
  int pos = 0;
  if (min_vis < 999999) {
    // Empate: preferir recto -> izq -> der
    if      (vis_c == min_vis) pos = 2;
    else if (vis_i == min_vis) pos = 1;
    else                       pos = 3;
  } else  {
    // Fallback: busco caminos en filas 2-3 del cono de visión
    bool hay_izq = false, hay_der = false;

    for (int k = 4; k <= 5; k++)
      if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D') { hay_izq = true; break; }

    for (int k = 9; k <= 11; k++)
      if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D') { hay_izq = true; break; }

    for (int k = 7; k <= 8; k++)
      if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D') { hay_der = true; break; }

    for (int k = 13; k <= 15; k++)
      if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D') { hay_der = true; break; }

    if      (hay_izq && !hay_der)           pos = 1;
    else if (hay_der && !hay_izq)           pos = 3;
    else if (hay_izq && hay_der)            pos = girar_derecha_n0 ? 3 : 1;
  }

  // Contador de turnos sin avanzar
  if (last_action != WALK && last_action != JUMP) {
    giros_sin_avanzar_n0++;
  }

  // Anti-oscilación: búsqueda en 8 direcciones priorizando C/D/U > S > H
  if (giros_sin_avanzar_n0 > 10) {
    for (int dir = 0; dir < 8; dir++) {
      ubicacion test = actual;
      test.brujula = (Orientacion)dir;
      ubicacion destino = Delante(test);

      if (destino.f >= 0 && destino.f < (int)mapaResultado.size() && destino.c >= 0 && destino.c < (int)mapaResultado[0].size()) {
        unsigned char celda = mapaResultado[destino.f][destino.c];

        if (celda == 'C' || celda == 'D' || celda == 'U' || celda == 'S' || celda == 'H') {
          int dif = abs(mapaCotas[destino.f][destino.c] - mapaCotas[sensores.posF][sensores.posC]);
          int max_dif = tiene_zapatillas ? 2 : 1;

          if (dif <= max_dif) {
            if (sensores.rumbo == (Orientacion)dir) {
              giros_sin_avanzar_n0 = 0;
              last_action = WALK;
              return WALK;
            } else {
              int giros = ((dir - (int)sensores.rumbo) + 8) % 8;
              last_action = (giros <= 4) ? TURN_SR : TURN_SL;
              return last_action;
            }
          }
        }
      }
    }
  }

  // JUMP de emergencia: salto 2 casillas si hay camino al otro lado
  if (giros_sin_avanzar_n0 > 14) {
    ubicacion u_jump = Delante(Delante(actual));

    if (u_jump.f >= 0 && u_jump.f < (int)mapaResultado.size() && u_jump.c >= 0 && u_jump.c < (int)mapaResultado[0].size()) {
      unsigned char dest = mapaResultado[u_jump.f][u_jump.c];

      if (dest=='C'||dest=='D'||dest=='U'||dest=='?') {
        giros_sin_avanzar_n0 = 0;
        last_action = JUMP;
        return JUMP;
      }
    }
  }

  // Alternancia de dirección de giro por defecto cada 16 turnos
  if (giros_sin_avanzar_n0 >= 16) {
    girar_derecha_n0 = !girar_derecha_n0;
    giros_sin_avanzar_n0 = 0;
  }

  // Ejecutar la acción elegida
  switch (pos) {
    case 2: accion = WALK; break;
    case 1: accion = TURN_SL; break;
    case 3: accion = TURN_SR; break;
    default: accion = girar_derecha_n0 ? TURN_SR : TURN_SL; break;
  }

  last_action = accion;
  return accion;
}



// =========================================================================
// FUNCIONES AUXILIARES - NIVEL 1 (Ingeniero)
// =========================================================================

/** @brief Compruebo si una casilla es transitable en nivel 1. */
bool ComportamientoIngeniero::es_transitable_N1(unsigned char c) const {
  return (c != 'M' && c != 'P' && c != 'A' && c != 'B' && c != '?');
}

/** @brief Filtro de altura para nivel 1. Máx desnivel 1, o 2 con zapatillas. */
char ComportamientoIngeniero::ViablePorAltura_N1(char casilla, int dif, bool zap) {
  if (abs(dif) <= 1 || (zap && abs(dif) <= 2)) return casilla;
  else return 'P'; 
}

/** @brief Elijo dirección preferente entre 3 casillas. */
int ComportamientoIngeniero::VeoCasillaInteresante_N1(char i, char c, char d, bool zap) {
  if (es_transitable_N1(c)) return 2; // 1º Frente
  if (es_transitable_N1(i)) return 1; // 2º Izquierda
  if (es_transitable_N1(d)) return 3; // 3º Derecha
  return 0;
}

/** @brief Compruebo si una casilla es un camino (no un obstáculo). */
bool ComportamientoIngeniero::es_camino(unsigned char c) const {
  return (c == 'C' || c == 'D' || c == 'U');
}


// =========================================================================
// NIVEL 1 - EXPLORACIÓN CON MAPA DE FEROMONAS (Ingeniero)
// =========================================================================
// Estrategia similar al nivel 0 pero con reglas de transitabilidad más amplias
// (nivel 1 permite pisar senderos 'S' y hierba 'H' directamente).
// Uso ActualizarMapa para construir un mapa interno y matriz_visitas como feromonas.
// Desempate en visitas: recto > izquierda > derecha (zurdo).
// Callejón sin salida: gira 90 grados a la izquierda (4 giros de 45 grados).
// =========================================================================

Action ComportamientoIngeniero::ComportamientoIngenieroNivel_1(Sensores sensores) {
  Action accion = IDLE;
  ActualizarMapa(sensores);

  if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

  // Completo giro de 90 grados pendiente
  if (giro45Izq > 0) {
      giro45Izq--;
      last_action = TURN_SL;
      return TURN_SL;
  }

  // Actualizo feromonas
  matriz_visitas[sensores.posF][sensores.posC]++;

  // Evalúo las 3 casillas frontales (altura + tipo de terreno)
  char i = ViablePorAltura_N1(sensores.superficie[1], sensores.cota[1] - sensores.cota[0], tiene_zapatillas);
  char c = ViablePorAltura_N1(sensores.superficie[2], sensores.cota[2] - sensores.cota[0], tiene_zapatillas);
  char d = ViablePorAltura_N1(sensores.superficie[3], sensores.cota[3] - sensores.cota[0], tiene_zapatillas);

  // Anticolisión: cualquier agente en las casillas frontales es obstáculo
  if (sensores.agentes[1] != '_') i = 'P';
  if (sensores.agentes[2] != '_') c = 'P';
  if (sensores.agentes[3] != '_') d = 'P';

  // Calculo coordenadas adyacentes
  ubicacion actual = {sensores.posF, sensores.posC, sensores.rumbo};
  ubicacion u_frente = Delante(actual);
  ubicacion u_izq = actual;
  u_izq.brujula = (Orientacion)((actual.brujula + 7) % 8);
  u_izq = Delante(u_izq);
  ubicacion u_der = actual;
  u_der.brujula = (Orientacion)((actual.brujula + 1) % 8);
  u_der = Delante(u_der);

  // Transitabilidad según nivel (nivel 6 reutiliza este código con otra lista de obstáculos)
  bool trans_c = (sensores.nivel == 6) ? (c != 'M' && c != 'P' && c != 'A' && c != 'B' && c != '?') : es_transitable_N1(c);
  bool trans_i = (sensores.nivel == 6) ? (i != 'M' && i != 'P' && i != 'A' && i != 'B' && i != '?') : es_transitable_N1(i);
  bool trans_d = (sensores.nivel == 6) ? (d != 'M' && d != 'P' && d != 'A' && d != 'B' && d != '?') : es_transitable_N1(d);

  // Consulto feromonas
  int vis_frente = trans_c ? matriz_visitas[u_frente.f][u_frente.c] : 999999;
  int vis_izq = trans_i ? matriz_visitas[u_izq.f][u_izq.c] : 999999;
  int vis_der = trans_d ? matriz_visitas[u_der.f][u_der.c] : 999999;

  // Elijo la ruta menos visitada (desempate: recto > izquierda > derecha)
  int min_visitas = min({vis_frente, vis_izq, vis_der});
  int pos = 0;
  if (min_visitas == 999999)          pos = 0;
  else if (vis_frente == min_visitas) pos = 2;
  else if (vis_izq == min_visitas)    pos = 1;
  else                                pos = 3;

  // Ejecuto acción
  switch (pos) {
    case 2: accion = WALK; break;
    case 1: accion = TURN_SL; break;
    case 3: accion = TURN_SR; break;
    default: 
        // Callejón sin salida: doy la vuelta
        giro45Izq = 3; 
        accion = TURN_SL; 
        break; 
  }

  last_action = accion;
  return accion;
}






// =========================================================================
// ALGORITMOS DE BÚSQUEDA (NIVEL 2)
// =========================================================================

list<Action> ComportamientoIngeniero::BusquedaEnAnchura(const estado& origen, const estado& destino, bool agua_permitida, bool ignorar_entidades, bool tiene_zap_inicio) {
    map<estado_ext, pair<estado_ext, Action>> padres;
    queue<estado_ext> abierta;
    set<estado_ext> cerrados;

    estado_ext ini = {origen.fila, origen.columna, origen.orientacion, tiene_zap_inicio};
    abierta.push(ini);
    cerrados.insert(ini);

    estado_ext meta;
    bool encontrado = false;

    while (!abierta.empty()) {
        estado_ext act = abierta.front();
        abierta.pop();

        if (act.fila == destino.fila && act.columna == destino.columna) {
            meta = act;
            encontrado = true;
            break;
        }

        int max_desnivel = act.zapatillas ? 2 : 1;

        Action acciones[] = {TURN_SL, TURN_SR, WALK, JUMP};
        for (Action accion : acciones) {
            estado_ext hijo = act;
            bool valido = true;

            if (accion == TURN_SL) {
                hijo.orientacion = (act.orientacion + 7) % 8;
            } else if (accion == TURN_SR) {
                hijo.orientacion = (act.orientacion + 1) % 8;
            } else if (accion == WALK) {
                int nf = act.fila, nc = act.columna;
                switch(act.orientacion) {
                    case 0: nf--; break; case 1: nf--; nc++; break;
                    case 2: nc++; break; case 3: nf++; nc++; break;
                    case 4: nf++; break; case 5: nf++; nc--; break;
                    case 6: nc--; break; case 7: nf--; nc--; break;
                }
                if (nf < 0 || nf >= (int)mapaResultado.size() || nc < 0 || nc >= (int)mapaResultado[0].size()) valido = false;
                else {
                    unsigned char celda = mapaResultado[nf][nc];
                    if (celda == 'P' || celda == 'M' || celda == 'B' || (!agua_permitida && celda == 'A')) valido = false;
                    else if (!ignorar_entidades && mapaEntidades[nf][nc] != '_' && mapaEntidades[nf][nc] != '?') valido = false;
                    else {
                        int dif = mapaCotas[nf][nc] - mapaCotas[act.fila][act.columna];
                        if (celda != '?' && abs(dif) > max_desnivel) valido = false;
                        else {
                            hijo.fila = nf; hijo.columna = nc;
                            if (celda == 'D') hijo.zapatillas = true;
                        }
                    }
                }
            } else if (accion == JUMP) {
                int jf = act.fila, jc = act.columna, mf = act.fila, mc = act.columna;
                switch(act.orientacion) {
                    case 0: jf-=2; mf--; break; case 1: jf-=2; jc+=2; mf--; mc++; break;
                    case 2: jc+=2; mc++; break; case 3: jf+=2; jc+=2; mf++; mc++; break;
                    case 4: jf+=2; mf++; break; case 5: jf+=2; jc-=2; mf++; mc--; break;
                    case 6: jc-=2; mc--; break; case 7: jf-=2; jc-=2; mf--; mc--; break;
                }
                if (jf < 0 || jf >= (int)mapaResultado.size() || jc < 0 || jc >= (int)mapaResultado[0].size()) valido = false;
                else {
                    unsigned char c_mid = mapaResultado[mf][mc], c_fin = mapaResultado[jf][jc];
                    if (c_mid == 'M' || c_mid == 'P' || c_mid == 'B' || (!agua_permitida && c_mid == 'A') ||
                        c_fin == 'M' || c_fin == 'P' || c_fin == 'B' || (!agua_permitida && c_fin == 'A')) valido = false;
                    else if (!ignorar_entidades) {
                        unsigned char e_mid = mapaEntidades[mf][mc], e_fin = mapaEntidades[jf][jc];
                        if ((e_mid != '_' && e_mid != '?') || (e_fin != '_' && e_fin != '?')) valido = false;
                    }
                    if (valido) {
                        int dif = mapaCotas[jf][jc] - mapaCotas[act.fila][act.columna];
                        if (c_fin != '?' && abs(dif) > max_desnivel) valido = false;
                        else {
                            hijo.fila = jf; hijo.columna = jc;
                            if (c_fin == 'D') hijo.zapatillas = true;
                        }
                    }
                }
            }

            if (valido && cerrados.find(hijo) == cerrados.end()) {
                cerrados.insert(hijo);
                padres[hijo] = {act, accion};
                abierta.push(hijo);
            }
        }
    }

    if (!encontrado) return list<Action>();

    list<Action> camino;
    estado_ext cur = meta;
    while (!(cur.fila == ini.fila && cur.columna == ini.columna &&
             cur.orientacion == ini.orientacion && cur.zapatillas == ini.zapatillas)) {
        auto& p = padres[cur];
        camino.push_front(p.second);
        cur = p.first;
    }
    return camino;
}


// Niveles avanzados (Uso de búsqueda)
/**
 * @brief Comportamiento del ingeniero para el Nivel 2 (búsqueda).
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_2(Sensores sensores) {
    ActualizarMapa(sensores);
    Action accion = IDLE;
    auto recordar = [&](Action a) {
        ultimaFilaPlan = sensores.posF;
        ultimaColPlan = sensores.posC;
        ultimaAccionPlan = a;
        return a;
    };

    if (sensores.superficie[0] == 'D') {
        tiene_zapatillas = true;
    }

    if ((ultimaAccionPlan == WALK || ultimaAccionPlan == JUMP) &&
        sensores.posF == ultimaFilaPlan && sensores.posC == ultimaColPlan) {
        hayPlan = false;
        plan.clear();
    }

    if (sensores.BelPosF == 0 && sensores.BelPosC == 0) return IDLE;

    if (!hayPlan) {
        estado origen = {sensores.posF, sensores.posC, (int)sensores.rumbo};
        estado destino = {sensores.BelPosF, sensores.BelPosC, 0};
        bool zap_inicio = tiene_zapatillas || sensores.superficie[0] == 'D';

        list<Action> plan_sin_agua = BusquedaEnAnchura(origen, destino, false, false, zap_inicio);
        list<Action> plan_con_agua = BusquedaEnAnchura(origen, destino, true, false, zap_inicio);

        if (plan_sin_agua.empty() && plan_con_agua.empty()) {
            plan_sin_agua = BusquedaEnAnchura(origen, destino, false, true, zap_inicio);
            plan_con_agua = BusquedaEnAnchura(origen, destino, true, true, zap_inicio);
        }

        if (!plan_sin_agua.empty() && !plan_con_agua.empty())
            plan = (plan_con_agua.size() < plan_sin_agua.size()) ? plan_con_agua : plan_sin_agua;
        else if (!plan_sin_agua.empty()) plan = plan_sin_agua;
        else plan = plan_con_agua;

        hayPlan = true;
    }

    if (!plan.empty()) {
        Action siguiente = plan.front();
        if ((siguiente == WALK || siguiente == JUMP) && sensores.agentes[2] != '_') {
            hayPlan = false;
            plan.clear();
            return recordar(IDLE);
        }
        accion = plan.front();
        plan.pop_front();
    } else if (sensores.posF != sensores.BelPosF || sensores.posC != sensores.BelPosC) {
        // Plan agotado pero no llegamos - replantear una vez más
        hayPlan = false;
    }

    return recordar(accion);
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_3(Sensores sensores) {
    ActualizarMapa(sensores);
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

    bool tecnico_delante = (sensores.agentes[2] == 't' || sensores.agentes[2] == 'T');

    // En nivel 3 solo interesa apartarse si bloqueamos de forma inmediata.
    // Reaccionar al técnico en cualquier punto de la visión hace que el
    // ingeniero se aleje del objetivo y empeore coste y tiempo.
    if (tecnico_delante || sensores.choque) {
        unsigned char c = sensores.superficie[2];
        int dif = sensores.cota[2] - sensores.cota[0];
        int max_dif = tiene_zapatillas ? 2 : 1;
        
        // Si el hueco de enfrente es seguro, avanzamos para dejar paso
        if (c != 'M' && c != 'P' && c != 'B' && c != 'A' && abs(dif) <= max_dif && sensores.agentes[2] == '_') {
            return WALK;
        } else {
            return TURN_SR;
        }
    }
    return IDLE;
}


// =========================================================
// === MOTOR DE PLANIFICACIÓN DE TUBERÍAS (NIVEL 4) ===
// =========================================================

bool ComportamientoIngeniero::EncontrarPlan_N4(int start_f, int start_c, std::list<Paso>& plan_resultante, int limite_eco) {
    plan_resultante.clear();
    std::priority_queue<NodoN4, std::vector<NodoN4>, std::greater<NodoN4>> abiertos;
    std::map<EstadoN4, int> cerrados;

    auto imp_install = [](unsigned char terreno) {
        if (terreno == 'A') return 50;
        if (terreno == 'H') return 45;
        if (terreno == 'S') return 25;
        if (terreno == 'C' || terreno == 'U') return 15;
        return 30;
    };

    auto imp_op = [](unsigned char terreno, int op) {
        if (op == 1) {
            if (terreno == 'H') return 55;
            if (terreno == 'S') return 30;
            if (terreno == 'C' || terreno == 'U') return 10;
            return 40;
        } else if (op == -1) {
            if (terreno == 'H') return 65;
            if (terreno == 'S') return 40;
            if (terreno == 'C' || terreno == 'U') return 25;
            return 50;
        }
        return 0;
    };

    unsigned char start_terr = mapaResultado[start_f][start_c];
    if (start_terr == 'M' || start_terr == 'P' || start_terr == 'B') return false; 

    int start_H = mapaCotas[start_f][start_c];
    std::vector<int> alturas_inicio;

    if (start_terr == 'A') {
        alturas_inicio.push_back(start_H); 
    } else {
        alturas_inicio.push_back(start_H);
        if (start_H > 0) alturas_inicio.push_back(start_H - 1);
        if (start_H < 9) alturas_inicio.push_back(start_H + 1); 
    }

    for (int h : alturas_inicio) {
        EstadoN4 st = {start_f, start_c, h};
        NodoN4 nodo;
        nodo.st = st;
        int op = h - start_H;
        Paso p = {start_f, start_c, op}; 
        nodo.secuencia.push_back(p);
        nodo.impacto = imp_op(start_terr, op);
        
        if (nodo.impacto <= limite_eco) {
            abiertos.push(nodo);
            cerrados[st] = nodo.impacto;
        }
    }

    int df[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    while (!abiertos.empty()) {
        NodoN4 actual = abiertos.top();
        abiertos.pop();

        if (mapaResultado[actual.st.f][actual.st.c] == 'U') {
            plan_resultante = actual.secuencia;
            return true;
        }

        unsigned char actual_terr = mapaResultado[actual.st.f][actual.st.c];

        for (int i = 0; i < 4; i++) {
            int nf = actual.st.f + df[i];
            int nc = actual.st.c + dc[i];

            if (nf < 0 || nf >= (int)mapaResultado.size() || nc < 0 || nc >= (int)mapaResultado[0].size()) continue;

            unsigned char n_terr = mapaResultado[nf][nc];
            if (n_terr == 'M' || n_terr == 'P' || n_terr == '?' || n_terr == 'B') continue; 

            int nH = mapaCotas[nf][nc];
            std::vector<int> alturas_vecino;

            if (n_terr == 'A') {
                alturas_vecino.push_back(nH); 
            } else {
                alturas_vecino.push_back(nH);
                if (nH > 0) alturas_vecino.push_back(nH - 1);
                if (nH < 9) alturas_vecino.push_back(nH + 1);
            }

            for (int nh : alturas_vecino) {
                if (actual.st.h >= nh && (actual.st.h - nh) <= 1) {
                    EstadoN4 siguiente = {nf, nc, nh};
                    int op = nh - nH;
                    
                    int impacto_tramo = imp_op(n_terr, op) + imp_install(n_terr) + imp_install(actual_terr);
                    int nuevo_impacto = actual.impacto + impacto_tramo;

                    if (nuevo_impacto <= limite_eco) {
                        if (cerrados.find(siguiente) == cerrados.end() || cerrados[siguiente] > nuevo_impacto) {
                            cerrados[siguiente] = nuevo_impacto;
                            NodoN4 hijo = actual;
                            hijo.st = siguiente;
                            Paso p_nuevo = {nf, nc, op};
                            hijo.secuencia.push_back(p_nuevo);
                            hijo.impacto = nuevo_impacto;
                            abiertos.push(hijo);
                        }
                    }
                }
            }
        }
    }
    return false; 
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_4(Sensores sensores) {
  if (!plan_tuberias_hecho) {
    // Aquí es donde estaba el error de sintaxis. Añadido el 4º argumento correctamente:
    bool exito = EncontrarPlan_N4(sensores.BelPosF, sensores.BelPosC, plan_tuberias, sensores.max_ecologico);

    if (exito && !plan_tuberias.empty()) {
      VisualizaRedTuberias(plan_tuberias);
    }
    plan_tuberias_hecho = true;
  }
  return IDLE; 
}



// =========================================================
// FUNCIONES AUXILIARES DE ALINEACIÓN (NIVEL 5)
// =========================================================

static Orientacion OrientacionHacia(int fromF, int fromC, int toF, int toC) {
    if (toF < fromF) return norte;
    if (toF > fromF) return sur;
    if (toC > fromC) return este;
    if (toC < fromC) return oeste;
    return norte; 
}

static int GirosNecesarios(Orientacion actual, Orientacion objetivo) {
    return ((int)objetivo - (int)actual + 8) % 8;
}

static bool HayTuberiaEntreIngeniero(const vector<vector<unsigned char>> &mapaTuberias,
                                     int f1, int c1, int f2, int c2) {
    if (abs(f1 - f2) + abs(c1 - c2) != 1) return false;

    unsigned char a = mapaTuberias[f1][c1];
    unsigned char b = mapaTuberias[f2][c2];

    if (f2 == f1 - 1 && c2 == c1) return (a & 1) && (b & 16);
    if (f2 == f1 + 1 && c2 == c1) return (a & 16) && (b & 1);
    if (f2 == f1 && c2 == c1 + 1) return (a & 4) && (b & 64);
    if (f2 == f1 && c2 == c1 - 1) return (a & 64) && (b & 4);
    return false;
}

bool ComportamientoIngeniero::EncontrarPlan_N5(int start_f, int start_c, std::list<Paso>& plan_resultante, int limite_eco) {
    plan_resultante.clear();
    bool dbg_oculto_30 = (mapaResultado.size() == 30 && limite_eco != 648 && limite_eco != 1000 &&
                          limite_eco != 1804 && limite_eco != 2364);
    bool dbg_plan = (limite_eco == 2364 || limite_eco == 2688 || limite_eco == 3533 ||
                     limite_eco == 2107 ||
                     limite_eco == 1719 || limite_eco == 1500 || dbg_oculto_30);
    dbg_plan = false;
    static std::map<int, int> llamadas_por_limite;
    int llamada = ++llamadas_por_limite[limite_eco];
    if (dbg_plan && (llamada <= 5 || llamada % 25 == 0)) {
        cerr << "[PLAN_N5 CALL] limite=" << limite_eco << " call=" << llamada
             << " start=(" << start_f << "," << start_c << ")\n";
    }
    std::priority_queue<NodoN4, std::vector<NodoN4>, std::greater<NodoN4>> abiertos;
    std::map<EstadoN4, int> cerrados;

    auto imp_install = [](unsigned char terreno) {
        if (terreno == 'A') return 50;
        if (terreno == 'H') return 45;
        if (terreno == 'S') return 25;
        if (terreno == 'C' || terreno == 'U') return 15;
        return 30;
    };

    auto imp_op = [](unsigned char terreno, int op) {
        if (op == 1) {
            if (terreno == 'H') return 55;
            if (terreno == 'S') return 30;
            if (terreno == 'C' || terreno == 'U') return 10;
            return 40;
        } else if (op == -1) {
            if (terreno == 'H') return 65;
            if (terreno == 'S') return 40;
            if (terreno == 'C' || terreno == 'U') return 25;
            return 50;
        }
        return 0;
    };

    unsigned char start_terr = mapaResultado[start_f][start_c];
    if (start_terr == 'M' || start_terr == 'P') return false; 

    int start_H = mapaCotas[start_f][start_c];
    std::vector<int> alturas_inicio;

    if (start_terr == 'A') {
        alturas_inicio.push_back(start_H); 
    } else {
        alturas_inicio.push_back(start_H);
        if (start_H > 0) alturas_inicio.push_back(start_H - 1);
        if (start_H < 9) alturas_inicio.push_back(start_H + 1); 
    }

    for (int h : alturas_inicio) {
        EstadoN4 st = {start_f, start_c, h};
        NodoN4 nodo;
        nodo.st = st;
        int op = h - start_H;
        Paso p = {start_f, start_c, op}; 
        nodo.secuencia.push_back(p);
        nodo.impacto = imp_op(start_terr, op);
        
        if (nodo.impacto <= limite_eco) {
            abiertos.push(nodo);
            cerrados[st] = nodo.impacto;
        }
    }

    int df[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    int expansiones = 0;
    while (!abiertos.empty()) {
        NodoN4 actual = abiertos.top();
        abiertos.pop();
        expansiones++;

        if (cerrados[actual.st] < actual.impacto) continue;

        if (mapaResultado[actual.st.f][actual.st.c] == 'U') {
            plan_resultante = actual.secuencia;
            if (dbg_plan) {
                cerr << "[PLAN_N5 OK] limite=" << limite_eco << " call=" << llamada
                     << " exp=" << expansiones << " size=" << plan_resultante.size()
                     << " impacto=" << actual.impacto << "\n";
            }
            return true;
        }

        for (int i = 0; i < 4; i++) {
            int nf = actual.st.f + df[i];
            int nc = actual.st.c + dc[i];

            if (nf < 0 || nf >= (int)mapaResultado.size() || nc < 0 || nc >= (int)mapaResultado[0].size()) continue;

            unsigned char n_terr = mapaResultado[nf][nc];
            if (n_terr == 'M' || n_terr == 'P' || n_terr == '?' || n_terr == 'B') continue; 

            int nH = mapaCotas[nf][nc];
            std::vector<int> alturas_vecino;

            if (n_terr == 'A') {
                alturas_vecino.push_back(nH); 
            } else {
                alturas_vecino.push_back(nH);
                if (nH > 0) alturas_vecino.push_back(nH - 1);
                if (nH < 9) alturas_vecino.push_back(nH + 1);
            }

            for (int nh : alturas_vecino) {
                // GRAVEDAD ESTRICTA: El tubo actual debe ser igual o 1 unidad más alto que el siguiente
                if (actual.st.h >= nh && (actual.st.h - nh) <= 1) {
                    EstadoN4 siguiente = {nf, nc, nh};
                    int op = nh - nH;
                    
                    // IMPACTO SIMPLE
                    unsigned char actual_terr = mapaResultado[actual.st.f][actual.st.c];
                    int impacto_tramo = imp_install(actual_terr) + imp_install(n_terr) + imp_op(n_terr, op);
                    int nuevo_impacto = actual.impacto + impacto_tramo;

                    if (nuevo_impacto <= limite_eco) {
                        if (cerrados.find(siguiente) == cerrados.end() || cerrados[siguiente] > nuevo_impacto) {
                            cerrados[siguiente] = nuevo_impacto;
                            NodoN4 hijo = actual;
                            hijo.st = siguiente;
                            Paso p_nuevo = {nf, nc, op};
                            hijo.secuencia.push_back(p_nuevo);
                            hijo.impacto = nuevo_impacto;
                            abiertos.push(hijo);
                        }
                    }
                }
            }
        }
    }
    if (dbg_plan && (llamada <= 5 || llamada % 25 == 0)) {
        cerr << "[PLAN_N5 FAIL] limite=" << limite_eco << " call=" << llamada
             << " exp=" << expansiones << "\n";
    }
    return false; 
}

bool ComportamientoIngeniero::EncontrarPlan_N5_Tentativo(int start_f, int start_c, std::list<Paso>& plan_resultante, int limite_eco) {
    plan_resultante.clear();
    if (start_f < 0 || start_f >= (int)mapaResultado.size() ||
        start_c < 0 || start_c >= (int)mapaResultado[0].size()) {
        return false;
    }

    struct NodoTentativo {
        int f;
        int c;
        int coste;
        bool operator>(const NodoTentativo& otro) const {
            return coste > otro.coste;
        }
    };

    auto bloqueada = [&](unsigned char terreno) {
        return terreno == 'M' || terreno == 'P' || terreno == 'B';
    };

    auto coste_celda = [&](unsigned char terreno) {
        if (terreno == '?') return 18;
        if (terreno == 'A') return 50;
        if (terreno == 'H') return 45;
        if (terreno == 'S') return 25;
        if (terreno == 'C' || terreno == 'U') return 15;
        return 30;
    };

    if (bloqueada(mapaResultado[start_f][start_c])) return false;

    std::priority_queue<NodoTentativo, std::vector<NodoTentativo>, std::greater<NodoTentativo>> abiertos;
    std::map<std::pair<int, int>, int> mejor_coste;
    std::map<std::pair<int, int>, std::pair<int, int>> padre;

    std::pair<int, int> inicio = {start_f, start_c};
    abiertos.push({start_f, start_c, 0});
    mejor_coste[inicio] = 0;

    int df[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    while (!abiertos.empty()) {
        NodoTentativo actual = abiertos.top();
        abiertos.pop();
        std::pair<int, int> clave_actual = {actual.f, actual.c};
        if (mejor_coste[clave_actual] < actual.coste) continue;

        if (mapaResultado[actual.f][actual.c] == 'U') {
            std::vector<std::pair<int, int>> camino;
            std::pair<int, int> p = clave_actual;
            while (true) {
                camino.push_back(p);
                if (p == inicio) break;
                p = padre[p];
            }
            std::reverse(camino.begin(), camino.end());
            for (auto celda : camino) {
                plan_resultante.push_back({celda.first, celda.second, 0});
            }
            return true;
        }

        for (int i = 0; i < 4; i++) {
            int nf = actual.f + df[i];
            int nc = actual.c + dc[i];
            if (nf < 0 || nf >= (int)mapaResultado.size() ||
                nc < 0 || nc >= (int)mapaResultado[0].size()) {
                continue;
            }

            unsigned char terreno = mapaResultado[nf][nc];
            if (bloqueada(terreno)) continue;

            int nuevo_coste = actual.coste + coste_celda(terreno);
            if (nuevo_coste > limite_eco) continue;

            std::pair<int, int> clave = {nf, nc};
            if (mejor_coste.find(clave) == mejor_coste.end() || nuevo_coste < mejor_coste[clave]) {
                mejor_coste[clave] = nuevo_coste;
                padre[clave] = clave_actual;
                abiertos.push({nf, nc, nuevo_coste});
            }
        }
    }

    return false;
}

list<Action> ComportamientoIngeniero::BusquedaEnAnchura_N5(const estado& origen, const estado& destino, bool agua_permitida, bool ignorar_entidades) {
    map<estado_ext, pair<estado_ext, Action>> padres;
    queue<estado_ext> abierta;
    set<estado_ext> cerrados;

    estado_ext ini = {origen.fila, origen.columna, origen.orientacion, false};
    abierta.push(ini);
    cerrados.insert(ini);

    estado_ext meta;
    bool encontrado = false;

    while (!abierta.empty()) {
        estado_ext act = abierta.front();
        abierta.pop();

        if (act.fila == destino.fila && act.columna == destino.columna) {
            meta = act;
            encontrado = true;
            break;
        }

        int max_desnivel = act.zapatillas ? 2 : 1;

        Action acciones[] = {TURN_SL, TURN_SR, WALK, JUMP};
        for (Action accion : acciones) {
            estado_ext hijo = act;
            bool valido = true;

            if (accion == TURN_SL) {
                hijo.orientacion = (act.orientacion + 7) % 8;
            } else if (accion == TURN_SR) {
                hijo.orientacion = (act.orientacion + 1) % 8;
            } else if (accion == WALK) {
                int nf = act.fila, nc = act.columna;
                switch(act.orientacion) {
                    case 0: nf--; break; case 1: nf--; nc++; break;
                    case 2: nc++; break; case 3: nf++; nc++; break;
                    case 4: nf++; break; case 5: nf++; nc--; break;
                    case 6: nc--; break; case 7: nf--; nc--; break;
                }
                if (nf < 0 || nf >= (int)mapaResultado.size() || nc < 0 || nc >= (int)mapaResultado[0].size()) { valido = false; }
                else {
                    unsigned char celda = mapaResultado[nf][nc];
                    if (celda == 'P' || celda == 'M' || celda == 'B' || (!agua_permitida && celda == 'A')) { valido = false; }
                    else if (!ignorar_entidades && mapaEntidades[nf][nc] != '_' && mapaEntidades[nf][nc] != '?') { valido = false; }
                    else {
                        int dif = mapaCotas[nf][nc] - mapaCotas[act.fila][act.columna];
                        if (celda != '?' && abs(dif) > max_desnivel) { valido = false; }
                        else {
                            hijo.fila = nf; hijo.columna = nc;
                            if (celda == 'D') hijo.zapatillas = true;
                        }
                    }
                }
            } else if (accion == JUMP) {
                int jf = act.fila, jc = act.columna, mf = act.fila, mc = act.columna;
                switch(act.orientacion) {
                    case 0: jf-=2; mf--; break; case 1: jf-=2; jc+=2; mf--; mc++; break;
                    case 2: jc+=2; mc++; break; case 3: jf+=2; jc+=2; mf++; mc++; break;
                    case 4: jf+=2; mf++; break; case 5: jf+=2; jc-=2; mf++; mc--; break;
                    case 6: jc-=2; mc--; break; case 7: jf-=2; jc-=2; mf--; mc--; break;
                }
                if (jf < 0 || jf >= (int)mapaResultado.size() || jc < 0 || jc >= (int)mapaResultado[0].size()) { valido = false; }
                else {
                    unsigned char c_mid = mapaResultado[mf][mc], c_fin = mapaResultado[jf][jc];
                    if (c_mid == 'M' || c_mid == 'P' || c_mid == 'B' || (!agua_permitida && c_mid == 'A') ||
                        c_fin == 'M' || c_fin == 'P' || c_fin == 'B' || (!agua_permitida && c_fin == 'A')) { valido = false; }
                    else if (!ignorar_entidades) {
                        unsigned char e_mid = mapaEntidades[mf][mc], e_fin = mapaEntidades[jf][jc];
                        if ((e_mid != '_' && e_mid != '?') || (e_fin != '_' && e_fin != '?')) { valido = false; }
                    }
                    if (valido) {
                        int dif = mapaCotas[jf][jc] - mapaCotas[act.fila][act.columna];
                        if (c_fin != '?' && abs(dif) > max_desnivel) { valido = false; }
                        else {
                            hijo.fila = jf; hijo.columna = jc;
                            if (c_fin == 'D') hijo.zapatillas = true;
                        }
                    }
                }
            }

            if (valido && cerrados.find(hijo) == cerrados.end()) {
                cerrados.insert(hijo);
                padres[hijo] = {act, accion};
                abierta.push(hijo);
            }
        }
    }

    if (!encontrado) return list<Action>();

    list<Action> camino;
    estado_ext cur = meta;
    while (!(cur.fila == ini.fila && cur.columna == ini.columna &&
             cur.orientacion == ini.orientacion && cur.zapatillas == ini.zapatillas)) {
        auto& p = padres[cur];
        camino.push_front(p.second);
        cur = p.first;
    }
    return camino;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_5(Sensores sensores) {
      ActualizarMapa(sensores);
    auto recordar = [&](Action a) {
        ultimaFilaPlan = sensores.posF;
        ultimaColPlan = sensores.posC;
        ultimaAccionPlan = a;
        return a;
    };
    
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

    if ((ultimaAccionPlan == WALK || ultimaAccionPlan == JUMP) &&
        sensores.posF == ultimaFilaPlan && sensores.posC == ultimaColPlan) {
        hayPlan = false;
        plan.clear();
    }
 
    if (sensores.tiempo == 0) {
        plan_tuberias_hecho = false;
        est_n6 = 0; plan_n5.clear();
        tramo_n5 = 0; terraformado_n5 = false;
        espera_n6 = 0;
        invertir_tramo_n6 = false;
        post_swap_n6 = false;
        hayPlan = false; plan.clear();
        ultimaFilaPlan = sensores.posF;
        ultimaColPlan = sensores.posC;
        ultimaAccionPlan = IDLE;
    }
 
    auto es_seguro = [&](Sensores sens) {
        int nf = sens.posF, nc = sens.posC;
        switch(sens.rumbo) {
            case norte: nf--; break; case noreste: nf--; nc++; break;
            case este: nc++; break; case sureste: nf++; nc++; break;
            case sur: nf++; break; case suroeste: nf++; nc--; break;
            case oeste: nc--; break; case noroeste: nf--; nc--; break;
        }
        if (nf < 0 || nf >= (int)mapaResultado.size() || nc < 0 || nc >= (int)mapaResultado[0].size()) return false;
        unsigned char real_c = sens.superficie[2]; 
        if (real_c == 'P' || real_c == 'M') return false; 
        if (real_c == 'B') return false;
        
        if (mapaResultado[nf][nc] != '?') {
            // ¡CLAVE CPU! Permitir desnivel 2 si tiene zapatillas para no entrar en bucle infinito
            int max_dif = tiene_zapatillas ? 2 : 1;
            if (abs(mapaCotas[nf][nc] - mapaCotas[sens.posF][sens.posC]) > max_dif) return false;
        }
        return true;
    };

    auto salto_seguro = [&](Sensores sens, bool agua_permitida) {
        int mf = sens.posF, mc = sens.posC;
        int jf = sens.posF, jc = sens.posC;
        switch(sens.rumbo) {
            case norte:    mf--;       jf -= 2; break;
            case noreste:  mf--; mc++; jf -= 2; jc += 2; break;
            case este:     mc++;       jc += 2; break;
            case sureste:  mf++; mc++; jf += 2; jc += 2; break;
            case sur:      mf++;       jf += 2; break;
            case suroeste: mf++; mc--; jf += 2; jc -= 2; break;
            case oeste:    mc--;       jc -= 2; break;
            case noroeste: mf--; mc--; jf -= 2; jc -= 2; break;
        }

        if (mf < 0 || mf >= (int)mapaResultado.size() || mc < 0 || mc >= (int)mapaResultado[0].size()) return false;
        if (jf < 0 || jf >= (int)mapaResultado.size() || jc < 0 || jc >= (int)mapaResultado[0].size()) return false;

        unsigned char c_mid = mapaResultado[mf][mc];
        unsigned char c_fin = mapaResultado[jf][jc];
        if (c_mid == '?' || c_fin == '?') return false;

        auto bloqueada = [&](unsigned char c) {
            return c == 'P' || c == 'M' || c == 'B' || (!agua_permitida && c == 'A');
        };
        if (bloqueada(c_mid) || bloqueada(c_fin)) return false;

        int max_dif = tiene_zapatillas ? 2 : 1;
        if (abs(mapaCotas[jf][jc] - mapaCotas[sens.posF][sens.posC]) > max_dif) return false;

        return true;
    };
 
    if (!plan_tuberias_hecho) {
        std::list<Paso> lista_plan;
        // Usamos el nuevo cerebro modular exclusivo para Nivel 5 y 6
        if (EncontrarPlan_N5(sensores.BelPosF, sensores.BelPosC, lista_plan, sensores.max_ecologico)) {
            plan_tuberias_hecho = true;
            for (auto p : lista_plan) plan_n5.push_back(p);
            tramo_n5 = (plan_n5.size() > 1 && plan_n5[0].op == 0) ? 1 : 0;
            est_n6 = 1; 
            hayPlan = false; plan.clear();
            return recordar(IDLE); 
        }
        return recordar(IDLE);
    }
 
    if (tramo_n5 >= (int)plan_n5.size()) return recordar(IDLE); // ¡CLAVE NARANJA! Sin el -1 para hacer el último tubo
    Paso tubo = plan_n5[tramo_n5]; 
 
    if (est_n6 == 1) { // 1. IR A LA CASILLA DEL TRAMO ACTUAL
        if (sensores.posF == tubo.fil && sensores.posC == tubo.col) {
            est_n6 = 2;
        } else {
            // ATAJO: si el tramo está justo delante, WALK directo sin BFS
            ubicacion delante = Delante({sensores.posF, sensores.posC, sensores.rumbo});
            if (delante.f == tubo.fil && delante.c == tubo.col && es_seguro(sensores)) {
                hayPlan = false; plan.clear();
                last_action = WALK;
                return recordar(WALK);
            }
            // Si no, usar BFS
            if (!hayPlan) {
                estado inicio = {sensores.posF, sensores.posC, (int)sensores.rumbo};
                estado destino = {tubo.fil, tubo.col, 0};
                bool bloquear_anterior = (tramo_n5 > 0);
                unsigned char respaldo_entidad = '_';
                Paso anterior;
                if (bloquear_anterior) {
                    anterior = plan_n5[tramo_n5 - 1];
                    respaldo_entidad = mapaEntidades[anterior.fil][anterior.col];
                    mapaEntidades[anterior.fil][anterior.col] = 't';
                }

                plan = BusquedaEnAnchura_N5(inicio, destino, true, !bloquear_anterior);

                if (bloquear_anterior) {
                    mapaEntidades[anterior.fil][anterior.col] = respaldo_entidad;
                }
                hayPlan = true;
            }
            if (!plan.empty()) {
                Action a = plan.front();
                if (a == WALK && (sensores.agentes[2] != '_' || !es_seguro(sensores))) {
                    hayPlan = false; plan.clear(); return recordar(IDLE);
                }
                plan.pop_front(); return recordar(a);
            } else { hayPlan = false; return recordar(TURN_SR); }
        }
    }
 
    if (est_n6 == 2) { // 2. TERRAFORMAR
        if (!terraformado_n5 && tubo.op != 0) {
            terraformado_n5 = true;
            if (tubo.op == 1) { return recordar(RAISE); }
            if (tubo.op == -1) { return recordar(DIG); }
        }
        // Caer directamente al COME sin perder turno
        terraformado_n5 = false;
        hayPlan = false; plan.clear();
        if (tramo_n5 == 0) {
            tramo_n5 = 1;
            est_n6 = 1;
            return recordar(IDLE);
        }
        est_n6 = 4;
        return recordar(COME);
    }
 
    // Estado 3 eliminado — fusionado con estado 2
 
    if (est_n6 == 4) { // 4. MIRAR HACIA AGUAS ARRIBA
        if (tramo_n5 > 0) {
            Paso anterior = plan_n5[tramo_n5 - 1];
            Orientacion ori = OrientacionHacia(sensores.posF, sensores.posC, anterior.fil, anterior.col);
            if (sensores.rumbo != ori) {
                return recordar((GirosNecesarios(sensores.rumbo, ori) <= 4) ? TURN_SR : TURN_SL);
            }
        }
        est_n6 = 5; // Cae directamente al estado 5
    }
 
    if (est_n6 == 5) { // 5. ESPERAR AL TÉCNICO Y COSER
        Paso anterior = plan_n5[tramo_n5 - 1];
        if (HayTuberiaEntreIngeniero(mapaTuberias, sensores.posF, sensores.posC,
                                     anterior.fil, anterior.col)) {
            instale_n5 = false;
            tramo_n5++;
            terraformado_n5 = false;
            hayPlan = false;
            plan.clear();
            est_n6 = 1;
            return recordar(IDLE);
        }

        instale_n5 = false;
        if (sensores.enfrente) {
            return recordar(INSTALL);
        }
        return recordar(IDLE);
    }
  return recordar(IDLE);
}


/**
 * @brief Comportamiento del ingeniero para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_6(Sensores sensores) {
    ActualizarMapa(sensores);
    bool dbg_oculto_30 = (mapaResultado.size() == 30 && sensores.max_ecologico != 648 &&
                          sensores.max_ecologico != 1000 && sensores.max_ecologico != 1804 &&
                          sensores.max_ecologico != 2364);
    bool dbg_n6 = (sensores.max_ecologico == 2364 || sensores.max_ecologico == 2688 || sensores.max_ecologico == 3533 ||
                   sensores.max_ecologico == 2107 ||
                   sensores.max_ecologico == 1719 || sensores.max_ecologico == 1500 || dbg_oculto_30);
    dbg_n6 = false;
    auto recordar = [&](Action a) {
        ultimaFilaPlan = sensores.posF;
        ultimaColPlan = sensores.posC;
        ultimaAccionPlan = a;
        return a;
    };
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

    if ((ultimaAccionPlan == WALK || ultimaAccionPlan == JUMP) &&
        sensores.posF == ultimaFilaPlan && sensores.posC == ultimaColPlan) {
        hayPlan = false;
        plan.clear();
    }

    if (sensores.tiempo == 0) {
        plan_tuberias_hecho = false;
        est_n6 = 0; plan_n5.clear();
        tramo_n5 = 0; terraformado_n5 = false;
        hayPlan = false; plan.clear();
        ultimaFilaPlan = sensores.posF;
        ultimaColPlan = sensores.posC;
        ultimaAccionPlan = IDLE;
    }

    auto es_seguro = [&](Sensores sens) {
        int nf = sens.posF, nc = sens.posC;
        switch(sens.rumbo) {
            case norte: nf--; break; case noreste: nf--; nc++; break;
            case este: nc++; break; case sureste: nf++; nc++; break;
            case sur: nf++; break; case suroeste: nf++; nc--; break;
            case oeste: nc--; break; case noroeste: nf--; nc--; break;
        }
        if (nf < 0 || nf >= (int)mapaResultado.size() || nc < 0 || nc >= (int)mapaResultado[0].size()) return false;
        unsigned char real_c = sens.superficie[2]; 
        if (real_c == 'P' || real_c == 'M') return false; 
        if (real_c == 'B') return false;
        
        if (mapaResultado[nf][nc] != '?') {
            int max_dif = tiene_zapatillas ? 2 : 1;
            if (abs(mapaCotas[nf][nc] - mapaCotas[sens.posF][sens.posC]) > max_dif) return false;
        }
        return true;
    };

    auto salto_seguro = [&](Sensores sens, bool agua_permitida) {
        int mf = sens.posF, mc = sens.posC;
        int jf = sens.posF, jc = sens.posC;
        switch(sens.rumbo) {
            case norte:    mf--;       jf -= 2; break;
            case noreste:  mf--; mc++; jf -= 2; jc += 2; break;
            case este:     mc++;       jc += 2; break;
            case sureste:  mf++; mc++; jf += 2; jc += 2; break;
            case sur:      mf++;       jf += 2; break;
            case suroeste: mf++; mc--; jf += 2; jc -= 2; break;
            case oeste:    mc--;       jc -= 2; break;
            case noroeste: mf--; mc--; jf -= 2; jc -= 2; break;
        }

        if (mf < 0 || mf >= (int)mapaResultado.size() || mc < 0 || mc >= (int)mapaResultado[0].size()) return false;
        if (jf < 0 || jf >= (int)mapaResultado.size() || jc < 0 || jc >= (int)mapaResultado[0].size()) return false;

        unsigned char c_mid = mapaResultado[mf][mc];
        unsigned char c_fin = mapaResultado[jf][jc];
        if (c_mid == '?' || c_fin == '?') return false;

        auto bloqueada = [&](unsigned char c) {
            return c == 'P' || c == 'M' || c == 'B' || (!agua_permitida && c == 'A');
        };
        if (bloqueada(c_mid) || bloqueada(c_fin)) return false;

        int max_dif = tiene_zapatillas ? 2 : 1;
        if (abs(mapaCotas[jf][jc] - mapaCotas[sens.posF][sens.posC]) > max_dif) return false;

        return true;
    };

    if (!plan_tuberias_hecho) {
        std::list<Paso> lista_plan;
        estado destino_preferente = {-1, -1, 0};
        auto impacto_plan_conocido = [&](const std::list<Paso>& plan_eval) {
            int total = 0;
            if (plan_eval.empty()) return total;
            auto imp_install_local = [](unsigned char terreno) {
                if (terreno == 'A') return 50;
                if (terreno == 'H') return 45;
                if (terreno == 'S') return 25;
                if (terreno == 'C' || terreno == 'U') return 15;
                return 30;
            };
            auto imp_op_local = [](unsigned char terreno, int op) {
                if (op == 1) {
                    if (terreno == 'H') return 55;
                    if (terreno == 'S') return 30;
                    if (terreno == 'C' || terreno == 'U') return 10;
                    return 40;
                }
                if (op == -1) {
                    if (terreno == 'H') return 65;
                    if (terreno == 'S') return 40;
                    if (terreno == 'C' || terreno == 'U') return 25;
                    return 50;
                }
                return 0;
            };

            auto it = plan_eval.begin();
            unsigned char celda = mapaResultado[it->fil][it->col];
            total += imp_op_local(celda, it->op);
            auto prev = it;
            ++it;
            for (; it != plan_eval.end(); ++it) {
                celda = mapaResultado[it->fil][it->col];
                total += imp_op_local(celda, it->op);
                unsigned char celda_prev = mapaResultado[prev->fil][prev->col];
                total += imp_install_local(celda_prev) + imp_install_local(celda);
                prev = it;
            }
            return total;
        };

        if (mapaResultado[sensores.BelPosF][sensores.BelPosC] != '?') {
          // Usamos el nuevo cerebro modular exclusivo para Nivel 5 y 6
          if (EncontrarPlan_N5(sensores.BelPosF, sensores.BelPosC, lista_plan, sensores.max_ecologico)) {
                int impacto_estimado = impacto_plan_conocido(lista_plan);
                bool plan_caro_en_mapa_grande = mapaResultado.size() >= 100 &&
                                                sensores.energia < 5000 &&
                                                sensores.max_ecologico >= 2000 &&
                                                impacto_estimado > (sensores.max_ecologico * 80) / 100;
                if (plan_caro_en_mapa_grande) {
                    std::list<Paso> plan_tentativo;
                    if (EncontrarPlan_N5_Tentativo(sensores.BelPosF, sensores.BelPosC,
                                                    plan_tentativo, sensores.max_ecologico)) {
                        for (const Paso& paso : plan_tentativo) {
                            if (mapaResultado[paso.fil][paso.col] == '?') {
                                destino_preferente.fila = paso.fil;
                                destino_preferente.columna = paso.col;
                                break;
                            }
                        }
                    }
                }
                if (destino_preferente.fila == -1) {
                plan_tuberias_hecho = true;
                for (auto p : lista_plan) plan_n5.push_back(p);
                if (dbg_n6) {
                    cerr << "[ING6 PLAN] t=" << sensores.tiempo << " maxeco=" << sensores.max_ecologico
                         << " size=" << plan_n5.size() << " pos=(" << sensores.posF << "," << sensores.posC
                         << ") energia=" << sensores.energia << "\n";
                    for (int k = 0; k < (int)plan_n5.size(); k++) {
                        cerr << "  [" << k << "] (" << plan_n5[k].fil << "," << plan_n5[k].col
                             << ") op=" << plan_n5[k].op << "\n";
                    }
                }
                est_n6 = 1; 
                hayPlan = false; plan.clear();
                return recordar(IDLE); 
                }
            }
        }
        
        if (!hayPlan) {
            estado inicio = {sensores.posF, sensores.posC, (int)sensores.rumbo};
            estado destino = destino_preferente;
            if (mapaResultado[sensores.BelPosF][sensores.BelPosC] == '?') {
                destino.fila = sensores.BelPosF; destino.columna = sensores.BelPosC;
            } else if (destino.fila == -1) {
                std::list<Paso> plan_tentativo;
                if ((int)mapaResultado.size() <= 75 &&
                    EncontrarPlan_N5_Tentativo(sensores.BelPosF, sensores.BelPosC,
                                                plan_tentativo, sensores.max_ecologico)) {
                    for (const Paso& paso : plan_tentativo) {
                        if (mapaResultado[paso.fil][paso.col] == '?') {
                            destino.fila = paso.fil;
                            destino.columna = paso.col;
                            break;
                        }
                    }
                }

                int min_dist = 999999;
                if (destino.fila == -1) {
                    for (int i = 0; i < (int)mapaResultado.size(); i++) {
                        for (int j = 0; j < (int)mapaResultado[0].size(); j++) {
                            if (mapaResultado[i][j] == '?') {
                                int dist = abs(i - sensores.posF) + abs(j - sensores.posC);
                                if (dist < min_dist) { min_dist = dist; destino.fila = i; destino.columna = j; }
                            }
                        }
                    }
                }
            }

            if (destino.fila != -1 && !hayPlan) {
                plan = BusquedaEnAnchura(inicio, destino, false, true); 
                if (plan.empty()) plan = BusquedaEnAnchura(inicio, destino, true, true); 
                hayPlan = true;
                if (plan.empty()) {
                    hayPlan = false;
                    return recordar(ComportamientoIngenieroNivel_1(sensores)); 
                }
            } else {
                return recordar(ComportamientoIngenieroNivel_1(sensores));
            }
        }

        if (hayPlan && !plan.empty()) {
            Action a = plan.front();
            if (a == WALK) {
                if (sensores.agentes[2] != '_' || sensores.choque) {
                    hayPlan = false;
                    plan.clear();
                    return recordar(IDLE);
                }
                if (!es_seguro(sensores)) { hayPlan = false; plan.clear(); return recordar(TURN_SR); }
            } else if (a == JUMP) {
                if (!salto_seguro(sensores, true)) {
                    hayPlan = false;
                    plan.clear();
                    return recordar(TURN_SR);
                }
            }
            plan.pop_front(); return recordar(a);
        } else {
            hayPlan = false; return recordar(ComportamientoIngenieroNivel_1(sensores));
        }
    }

    if (tramo_n5 >= (int)plan_n5.size() - 1) return recordar(IDLE); 
    Paso tubo = plan_n5[tramo_n5]; 

    if (est_n6 == 1) { 
        if (sensores.posF == tubo.fil && sensores.posC == tubo.col) {
            espera_n6 = 0;
            invertir_tramo_n6 = false;
            est_n6 = 2; return recordar(IDLE);
        } else {
            if (!hayPlan) {
                estado inicio = {sensores.posF, sensores.posC, (int)sensores.rumbo};
                estado destino = {tubo.fil, tubo.col, 0};
                plan = BusquedaEnAnchura(inicio, destino, true, true);
                hayPlan = true;
            }
            if (!plan.empty()) {
                Action a = plan.front(); 
                if (a == WALK) {
                    if (sensores.agentes[2] != '_' || sensores.choque) {
                        hayPlan = false;
                        plan.clear();
                        return recordar(IDLE);
                    }
                    if (!es_seguro(sensores)) { hayPlan = false; plan.clear(); return recordar(TURN_SR); }
                } else if (a == JUMP) {
                    if (!salto_seguro(sensores, true)) {
                        hayPlan = false;
                        plan.clear();
                        return recordar(TURN_SR);
                    }
                }
                plan.pop_front(); return recordar(a);
            } else { hayPlan = false; return recordar(TURN_SR); }
        }
    }

    if (est_n6 == 2) { 
        if (!terraformado_n5 && tubo.op != 0) {
            terraformado_n5 = true;
            if (tubo.op == 1) return recordar(RAISE);
            if (tubo.op == -1) return recordar(DIG);
        }
        est_n6 = 3; return recordar(IDLE);
    }

    if (est_n6 == 3) { 
        espera_n6 = 0;
        est_n6 = 4;
        terraformado_n5 = false; 
        if (dbg_n6) {
            cerr << "[ING6 COME] t=" << sensores.tiempo << " tramo=" << tramo_n5
                 << " pos=(" << sensores.posF << "," << sensores.posC << ") energia=" << sensores.energia << "\n";
        }
        return recordar(COME);
    }

    if (est_n6 == 4) { 
        if (sensores.posF != tubo.fil || sensores.posC != tubo.col) {
            hayPlan = false;
            plan.clear();
            est_n6 = 5; return recordar(IDLE); 
        }
        if (tramo_n5 + 1 < (int)plan_n5.size()) {
            Paso next_tubo = plan_n5[tramo_n5 + 1];
            if (HayTuberiaEntreIngeniero(mapaTuberias, sensores.posF, sensores.posC,
                                         next_tubo.fil, next_tubo.col)) {
                tramo_n5++;
                hayPlan = false;
                plan.clear();
                espera_n6 = 0;
                invertir_tramo_n6 = false;
                post_swap_n6 = false;
                est_n6 = 1;
                if (dbg_n6) {
                    cerr << "[ING6 PRE OK] t=" << sensores.tiempo << " tramo=" << tramo_n5
                         << " pos=(" << sensores.posF << "," << sensores.posC << ") eco=" << sensores.ecologico
                         << " energia=" << sensores.energia << "\n";
                }
                return recordar(IDLE);
            }
            Orientacion ori_deseada = OrientacionHacia(sensores.posF, sensores.posC, next_tubo.fil, next_tubo.col);
            if (sensores.rumbo != ori_deseada) {
                return recordar((GirosNecesarios(sensores.rumbo, ori_deseada) <= 4) ? TURN_SR : TURN_SL);
            }
            if (sensores.agentes[2] != '_' || sensores.choque) return recordar(IDLE);
            if (es_seguro(sensores)) return recordar(WALK);
        }
        return recordar(IDLE);
    }

    if (est_n6 == 5) { 
        if (tramo_n5 + 1 < (int)plan_n5.size()) {
            Paso next_tubo = plan_n5[tramo_n5 + 1];
            if (!terraformado_n5 && next_tubo.op != 0) {
                terraformado_n5 = true;
                if (next_tubo.op == 1) return recordar(RAISE);
                if (next_tubo.op == -1) return recordar(DIG);
            }
        }
        Orientacion ori_deseada = OrientacionHacia(sensores.posF, sensores.posC, tubo.fil, tubo.col);
        if (sensores.rumbo != ori_deseada) {
            return recordar((GirosNecesarios(sensores.rumbo, ori_deseada) <= 4) ? TURN_SR : TURN_SL);
        }
        est_n6 = 6; return recordar(IDLE);
    }

    if (est_n6 == 6) { 
        if (HayTuberiaEntreIngeniero(mapaTuberias, sensores.posF, sensores.posC,
                                     tubo.fil, tubo.col)) {
            tramo_n5++;
            hayPlan = false; plan.clear();
            espera_n6 = 0;
            invertir_tramo_n6 = false;
            post_swap_n6 = false;
            est_n6 = 1; 
            if (dbg_n6) {
                cerr << "[ING6 OK] t=" << sensores.tiempo << " tramo=" << tramo_n5
                     << " pos=(" << sensores.posF << "," << sensores.posC << ") eco=" << sensores.ecologico
                     << " energia=" << sensores.energia << "\n";
            }
            return recordar(IDLE);
        }
        if (sensores.enfrente) { 
            espera_n6 = 0;
            if (dbg_n6) {
                cerr << "[ING6 INSTALL] t=" << sensores.tiempo << " tramo=" << tramo_n5
                     << " pos=(" << sensores.posF << "," << sensores.posC << ") energia=" << sensores.energia << "\n";
            }
            return recordar(INSTALL);
        }
        espera_n6++;
        if (dbg_n6 && espera_n6 % 20 == 0) {
            cerr << "[ING6 WAIT] t=" << sensores.tiempo << " tramo=" << tramo_n5
                 << " espera=" << espera_n6 << " pos=(" << sensores.posF << "," << sensores.posC
                 << ") rumbo=" << sensores.rumbo << " ag2=" << sensores.agentes[2]
                 << " enfrente=" << sensores.enfrente << " energia=" << sensores.energia << "\n";
        }
        if (!invertir_tramo_n6 && espera_n6 > 80) {
            invertir_tramo_n6 = true;
            espera_n6 = 0;
            hayPlan = false;
            plan.clear();
            est_n6 = 7;
            if (dbg_n6) {
                cerr << "[ING6 SWAP COME] t=" << sensores.tiempo << " tramo=" << tramo_n5
                     << " pos=(" << sensores.posF << "," << sensores.posC << ") energia=" << sensores.energia << "\n";
            }
            return recordar(COME);
        }
        return recordar(IDLE);
    }

    if (est_n6 == 7) {
        Orientacion ori_deseada = OrientacionHacia(sensores.posF, sensores.posC, tubo.fil, tubo.col);
        if (sensores.posF == tubo.fil && sensores.posC == tubo.col) {
            est_n6 = 8;
            if (dbg_n6) {
                cerr << "[ING6 EST8] t=" << sensores.tiempo << " tramo=" << tramo_n5
                     << " pos=(" << sensores.posF << "," << sensores.posC << ") energia=" << sensores.energia << "\n";
            }
            return recordar(IDLE);
        }
        if (sensores.rumbo != ori_deseada) {
            return recordar((GirosNecesarios(sensores.rumbo, ori_deseada) <= 4) ? TURN_SR : TURN_SL);
        }
        if (sensores.agentes[2] != '_' || sensores.choque) return recordar(IDLE);
        if (es_seguro(sensores)) return recordar(WALK);
        return recordar(IDLE);
    }

    if (est_n6 == 8) {
        if (tramo_n5 + 1 < (int)plan_n5.size()) {
            Paso next_tubo = plan_n5[tramo_n5 + 1];
            if (HayTuberiaEntreIngeniero(mapaTuberias, sensores.posF, sensores.posC,
                                         next_tubo.fil, next_tubo.col)) {
                tramo_n5++;
                hayPlan = false;
                plan.clear();
                espera_n6 = 0;
                invertir_tramo_n6 = false;
                post_swap_n6 = false;
                est_n6 = 1;
                if (dbg_n6) {
                    cerr << "[ING6 SWAP OK] t=" << sensores.tiempo << " tramo=" << tramo_n5
                         << " pos=(" << sensores.posF << "," << sensores.posC << ") eco=" << sensores.ecologico
                         << " energia=" << sensores.energia << "\n";
                }
                return recordar(IDLE);
            }
            Orientacion ori_deseada = OrientacionHacia(sensores.posF, sensores.posC, next_tubo.fil, next_tubo.col);
            if (sensores.rumbo != ori_deseada) {
                return recordar((GirosNecesarios(sensores.rumbo, ori_deseada) <= 4) ? TURN_SR : TURN_SL);
            }
            if (sensores.enfrente) {
                espera_n6 = 0;
                if (dbg_n6) {
                    cerr << "[ING6 INSTALL SWAP] t=" << sensores.tiempo << " tramo=" << tramo_n5
                         << " pos=(" << sensores.posF << "," << sensores.posC << ") energia=" << sensores.energia << "\n";
                }
                return recordar(INSTALL);
            }
        }
        return recordar(IDLE);
    }

    if (est_n6 == 12) {
        if (tramo_n5 + 1 >= (int)plan_n5.size()) {
            post_swap_n6 = false;
            est_n6 = 1;
            return recordar(IDLE);
        }

        Paso actual = plan_n5[tramo_n5];
        Paso siguiente = plan_n5[tramo_n5 + 1];

        if (sensores.posF != actual.fil || sensores.posC != actual.col) {
            if (!hayPlan) {
                estado inicio = {sensores.posF, sensores.posC, (int)sensores.rumbo};
                estado destino = {actual.fil, actual.col, 0};
                plan = BusquedaEnAnchura(inicio, destino, true, true);
                hayPlan = true;
            }

            if (!plan.empty()) {
                Action a = plan.front();
                if (a == WALK) {
                    if (sensores.agentes[2] != '_' || sensores.choque) {
                        hayPlan = false;
                        plan.clear();
                        return recordar(IDLE);
                    }
                    if (!es_seguro(sensores)) {
                        hayPlan = false;
                        plan.clear();
                        return recordar(TURN_SR);
                    }
                } else if (a == JUMP) {
                    if (!salto_seguro(sensores, true)) {
                        hayPlan = false;
                        plan.clear();
                        return recordar(TURN_SR);
                    }
                }
                plan.pop_front();
                return recordar(a);
            }

            hayPlan = false;
            return recordar(IDLE);
        }

        hayPlan = false;
        plan.clear();

        if (HayTuberiaEntreIngeniero(mapaTuberias, sensores.posF, sensores.posC,
                                     siguiente.fil, siguiente.col)) {
            tramo_n5++;
            espera_n6 = 0;
            terraformado_n5 = false;
            invertir_tramo_n6 = false;
            post_swap_n6 = false;
            est_n6 = 1;
            return recordar(IDLE);
        }

        if (!terraformado_n5 && siguiente.op != 0) {
            terraformado_n5 = true;
            if (siguiente.op == 1) return recordar(RAISE);
            if (siguiente.op == -1) return recordar(DIG);
        }

        Orientacion ori_deseada = OrientacionHacia(sensores.posF, sensores.posC,
                                                   siguiente.fil, siguiente.col);
        if (sensores.rumbo != ori_deseada) {
            return recordar((GirosNecesarios(sensores.rumbo, ori_deseada) <= 4) ? TURN_SR : TURN_SL);
        }

        bool tecnico_delante = (sensores.agentes[2] == 't' || sensores.agentes[2] == 'T');
        if (siguiente.op != 0) tecnico_delante = false;
        if (tecnico_delante) {
            if (espera_n6 == 0) {
                espera_n6 = 1;
                if (dbg_n6) {
                    cerr << "[ING6 POST WAKE] t=" << sensores.tiempo << " tramo=" << tramo_n5
                         << " pos=(" << sensores.posF << "," << sensores.posC << ") sig=("
                         << siguiente.fil << "," << siguiente.col << ")\n";
                }
                return recordar(COME);
            }
            if (dbg_n6) {
                cerr << "[ING6 POST INSTALL] t=" << sensores.tiempo << " tramo=" << tramo_n5
                     << " pos=(" << sensores.posF << "," << sensores.posC << ") sig=("
                     << siguiente.fil << "," << siguiente.col << ") ag2=" << sensores.agentes[2]
                     << " enfrente=" << sensores.enfrente << "\n";
            }
            return recordar(INSTALL);
        }

        espera_n6++;
        if (dbg_n6 && espera_n6 % 2 == 0) {
            cerr << "[ING6 POST WAIT] t=" << sensores.tiempo << " tramo=" << tramo_n5
                 << " espera=" << espera_n6 << " pos=(" << sensores.posF << "," << sensores.posC
                 << ") rumbo=" << sensores.rumbo << " sig=(" << siguiente.fil << "," << siguiente.col
                 << ") ag2=" << sensores.agentes[2] << " enfrente=" << sensores.enfrente << "\n";
        }
        if (espera_n6 > 8) {
            espera_n6 = 0;
            terraformado_n5 = false;
            post_swap_n6 = false;
            est_n6 = 3;
        }
        return recordar(IDLE);
    }

    if (est_n6 == 9) {
        if (tramo_n5 + 1 >= (int)plan_n5.size()) {
            post_swap_n6 = false;
            est_n6 = 1;
            return recordar(IDLE);
        }

        Paso next_tubo = plan_n5[tramo_n5 + 1];
        if (sensores.posF == next_tubo.fil && sensores.posC == next_tubo.col) {
            hayPlan = false;
            plan.clear();
            est_n6 = 10;
            return recordar(IDLE);
        }

        if (!hayPlan) {
            estado inicio = {sensores.posF, sensores.posC, (int)sensores.rumbo};
            estado destino = {next_tubo.fil, next_tubo.col, 0};
            unsigned char respaldo = mapaEntidades[tubo.fil][tubo.col];
            mapaEntidades[tubo.fil][tubo.col] = 't';
            plan = BusquedaEnAnchura(inicio, destino, true, false);
            mapaEntidades[tubo.fil][tubo.col] = respaldo;
            hayPlan = true;
        }

        if (!plan.empty()) {
            Action a = plan.front();
            if (a == WALK) {
                if (sensores.agentes[2] != '_' || sensores.choque) {
                    hayPlan = false;
                    plan.clear();
                    return recordar(IDLE);
                }
                if (!es_seguro(sensores)) { hayPlan = false; plan.clear(); return recordar(TURN_SR); }
            } else if (a == JUMP) {
                if (!salto_seguro(sensores, true)) {
                    hayPlan = false;
                    plan.clear();
                    return recordar(TURN_SR);
                }
            }
            plan.pop_front();
            return recordar(a);
        }

        hayPlan = false;
        return recordar(IDLE);
    }

    if (est_n6 == 10) {
        if (tramo_n5 + 1 >= (int)plan_n5.size()) {
            post_swap_n6 = false;
            est_n6 = 1;
            return recordar(IDLE);
        }

        Paso next_tubo = plan_n5[tramo_n5 + 1];
        if (!terraformado_n5 && next_tubo.op != 0) {
            terraformado_n5 = true;
            if (next_tubo.op == 1) return recordar(RAISE);
            if (next_tubo.op == -1) return recordar(DIG);
        }

        terraformado_n5 = false;
        est_n6 = 11;
        return recordar(IDLE);
    }

    if (est_n6 == 11) {
        if (tramo_n5 + 1 >= (int)plan_n5.size()) {
            post_swap_n6 = false;
            est_n6 = 1;
            return recordar(IDLE);
        }

        Paso next_tubo = plan_n5[tramo_n5 + 1];
        if (HayTuberiaEntreIngeniero(mapaTuberias, sensores.posF, sensores.posC,
                                     tubo.fil, tubo.col)) {
            tramo_n5++;
            hayPlan = false;
            plan.clear();
            espera_n6 = 0;
            invertir_tramo_n6 = false;
            post_swap_n6 = false;
            est_n6 = 1;
            return recordar(IDLE);
        }

        Orientacion ori_deseada = OrientacionHacia(sensores.posF, sensores.posC, tubo.fil, tubo.col);
        if (sensores.rumbo != ori_deseada) {
            return recordar((GirosNecesarios(sensores.rumbo, ori_deseada) <= 4) ? TURN_SR : TURN_SL);
        }

        if (sensores.enfrente) {
            return recordar(INSTALL);
        }

        return recordar(IDLE);
    }
    return recordar(IDLE);
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
