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

    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;
    if (sensores.superficie[0] == 'U') return IDLE;

    // Siempre incrementar visitas
    matriz_visitas[sensores.posF][sensores.posC]++;
    if (last_action == WALK || last_action == JUMP) giros_sin_avanzar_n0 = 0;

    char ci = ViablePorAltura(sensores.superficie[1], sensores.cota[1] - sensores.cota[0], tiene_zapatillas);
    char cc = ViablePorAltura(sensores.superficie[2], sensores.cota[2] - sensores.cota[0], tiene_zapatillas);
    char cd = ViablePorAltura(sensores.superficie[3], sensores.cota[3] - sensores.cota[0], tiene_zapatillas);

    // Anticolisión
    if (sensores.agentes[1] == 't') ci = 'P';
    if (sensores.agentes[2] == 't') cc = 'P';
    if (sensores.agentes[3] == 't') cd = 'P';

    // Prioridad absoluta: U visible
    if (cc == 'U') { last_action = WALK;    return WALK;    }
    if (ci == 'U') { last_action = TURN_SL; return TURN_SL; }
    if (cd == 'U') { last_action = TURN_SR; return TURN_SR; }

    // Temporizador de espera acotada (usa giro45Izq como contador, libre en N0)
    if (giro45Izq > 0) {
        giro45Izq--;
        matriz_visitas[sensores.posF][sensores.posC]++;
        last_action = IDLE;
        return IDLE;
    }

    // Coordenadas adyacentes para leer visitas
    ubicacion actual = {sensores.posF, sensores.posC, sensores.rumbo};
    ubicacion u_izq = actual;
    u_izq.brujula = (Orientacion)((actual.brujula + 7) % 8);
    u_izq = Delante(u_izq);
    ubicacion u_frente = Delante(actual);
    ubicacion u_der = actual;
    u_der.brujula = (Orientacion)((actual.brujula + 1) % 8);
    u_der = Delante(u_der);

    bool ok_i = (ci == 'C' || ci == 'D');
    bool ok_c = (cc == 'C' || cc == 'D');
    bool ok_d = (cd == 'C' || cd == 'D');

    int vis_i = ok_i ? matriz_visitas[u_izq.f][u_izq.c]    : 999999;
    int vis_c = ok_c ? matriz_visitas[u_frente.f][u_frente.c] : 999999;
    int vis_d = ok_d ? matriz_visitas[u_der.f][u_der.c]    : 999999;

    int min_vis = min({vis_i, vis_c, vis_d});

    int pos = 0;
    if (min_vis < 999999) {
        // Empate: preferir recto → izq → der
        if      (vis_c == min_vis) pos = 2;
        else if (vis_i == min_vis) pos = 1;
        else                       pos = 3;
    } else {
        // FALLBACK: mirar filas 2 y 3 con alternancia para no crear bucle
        bool hay_izq = false, hay_der = false;
        for (int k = 4; k <= 5; k++)
            if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D')
                { hay_izq = true; break; }
        for (int k = 9; k <= 11; k++)
            if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D')
                { hay_izq = true; break; }
        for (int k = 7; k <= 8; k++)
            if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D')
                { hay_der = true; break; }
        for (int k = 13; k <= 15; k++)
            if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D')
                { hay_der = true; break; }

        if      (hay_izq && !hay_der)           pos = 1;
        else if (hay_der && !hay_izq)           pos = 3;
        else if (hay_izq && hay_der)            pos = girar_derecha_n0 ? 3 : 1; // ALTERNANCIA
    }

    // Anti-bucle — contar instantes sin avanzar de verdad
    if (last_action != WALK && last_action != JUMP) {
        giros_sin_avanzar_n0++;
    }

    // Intentar JUMP si llevamos mucho tiempo bloqueados
    if (giros_sin_avanzar_n0 > 14) {
        ubicacion u_jump = Delante(Delante(actual));
        if (u_jump.f >= 0 && u_jump.f < (int)mapaResultado.size() &&
            u_jump.c >= 0 && u_jump.c < (int)mapaResultado[0].size()) {
            unsigned char dest = mapaResultado[u_jump.f][u_jump.c];
            if (dest=='C'||dest=='D'||dest=='U'||dest=='?') {
                giros_sin_avanzar_n0 = 0;
                last_action = JUMP;
                return JUMP;
            }
        }
    }

    if (giros_sin_avanzar_n0 >= 12) { // ← Reducido de 16 a 12
        girar_derecha_n0 = !girar_derecha_n0;
        giros_sin_avanzar_n0 = 0;
    }

    // NUEVO: escape basado en visitas
    {
      // Escape por visitas: ESPERAR (no girar) para que el Técnico tenga espacio
      bool veo_tec = (sensores.agentes[1] == 't' ||
                      sensores.agentes[2] == 't' ||
                      sensores.agentes[3] == 't');
      if (veo_tec && matriz_visitas[sensores.posF][sensores.posC] > 20) {
          girar_derecha_n0 = !girar_derecha_n0;
          giros_sin_avanzar_n0 = 0;
          matriz_visitas[sensores.posF][sensores.posC] = 0; // reset total
          giro45Izq = 8;   // esperar 8 turnos (bounded, no loop infinito)
          last_action = IDLE;
          return IDLE;
      }
    }

    switch (pos) {
        case 2: accion = WALK; break;
        case 1: accion = TURN_SL; break;
        case 3: accion = TURN_SR; break;
        default: accion = girar_derecha_n0 ? TURN_SR : TURN_SL; break;
    }

    last_action = accion;
    return accion;
}




// --- FUNCIONES AUXILIARES NIVEL 1 (INGENIERO) ---
bool ComportamientoIngeniero::es_transitable_N1(unsigned char c) const {
  // LISTA NEGRA: Todo es pisable menos los obstáculos duros y lo desconocido
  return (c != 'M' && c != 'P' && c != 'A' && c != 'B' && c != '?');
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
  // ¡MAGIA MODULAR! Si estamos en Nivel 6, usamos la lista negra. Si no, tu lista blanca original.
  bool trans_c = (sensores.nivel == 6) ? (c != 'M' && c != 'P' && c != 'A' && c != 'B' && c != '?') : es_transitable_N1(c);
  bool trans_i = (sensores.nivel == 6) ? (i != 'M' && i != 'P' && i != 'A' && i != 'B' && i != '?') : es_transitable_N1(i);
  bool trans_d = (sensores.nivel == 6) ? (d != 'M' && d != 'P' && d != 'A' && d != 'B' && d != '?') : es_transitable_N1(d);

  int vis_frente = trans_c ? matriz_visitas[u_frente.f][u_frente.c] : 999999;
  int vis_izq = trans_i ? matriz_visitas[u_izq.f][u_izq.c] : 999999;
  int vis_der = trans_d ? matriz_visitas[u_der.f][u_der.c] : 999999;

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






// =========================================================================
// ALGORITMOS DE BÚSQUEDA (NIVEL 2)
// =========================================================================

list<Action> ComportamientoIngeniero::BusquedaEnAnchura(const estado& origen, const estado& destino, bool agua_permitida, bool ignorar_entidades, bool tiene_zap_inicio) {
    queue<nodo_ext> abierta;
    set<estado_ext> cerrados;

    nodo_ext inicial;
    inicial.st = {origen.fila, origen.columna, origen.orientacion, tiene_zap_inicio};
    abierta.push(inicial);
    cerrados.insert(inicial.st);

    int nodos_expandidos = 0;

    while (!abierta.empty() && nodos_expandidos < 100000) {
        nodos_expandidos++;
        nodo_ext actual = abierta.front();
        abierta.pop();

        if (actual.st.fila == destino.fila && actual.st.columna == destino.columna)
            return actual.secuencia;

        bool zap = actual.st.zapatillas;
        int max_desnivel = zap ? 2 : 1;

        // HIJO 1: GIRAR IZQUIERDA
        {
            nodo_ext hijo = actual;
            hijo.st.orientacion = (actual.st.orientacion + 7) % 8;
            if (cerrados.find(hijo.st) == cerrados.end()) {
                hijo.secuencia.push_back(TURN_SL);
                cerrados.insert(hijo.st);
                abierta.push(hijo);
            }
        }

        // HIJO 2: GIRAR DERECHA
        {
            nodo_ext hijo = actual;
            hijo.st.orientacion = (actual.st.orientacion + 1) % 8;
            if (cerrados.find(hijo.st) == cerrados.end()) {
                hijo.secuencia.push_back(TURN_SR);
                cerrados.insert(hijo.st);
                abierta.push(hijo);
            }
        }

        // HIJO 3: WALK
        {
            int nf = actual.st.fila, nc = actual.st.columna;
            switch(actual.st.orientacion) {
                case 0: nf--; break; case 1: nf--; nc++; break;
                case 2: nc++; break; case 3: nf++; nc++; break;
                case 4: nf++; break; case 5: nf++; nc--; break;
                case 6: nc--; break; case 7: nf--; nc--; break;
            }
            if (nf >= 0 && nf < (int)mapaResultado.size() && nc >= 0 && nc < (int)mapaResultado[0].size()) {
                unsigned char celda = mapaResultado[nf][nc];
                unsigned char entidad = mapaEntidades[nf][nc];
                if (ignorar_entidades || (entidad == '_' || entidad == '?')) {
                    if (celda != 'P' && celda != 'M' && celda != 'B' && (agua_permitida || celda != 'A')) {
                        int dif_cota = mapaCotas[nf][nc] - mapaCotas[actual.st.fila][actual.st.columna];
                        if (celda == '?' || abs(dif_cota) <= max_desnivel) {
                            nodo_ext hijo = actual;
                            hijo.st.fila = nf; hijo.st.columna = nc;
                            if (celda == 'D') hijo.st.zapatillas = true;
                            if (cerrados.find(hijo.st) == cerrados.end()) {
                                hijo.secuencia.push_back(WALK);
                                cerrados.insert(hijo.st);
                                abierta.push(hijo);
                            }
                        }
                    }
                }
            }
        }

        // HIJO 4: JUMP
        {
            int jf = actual.st.fila, jc = actual.st.columna;
            int mf = actual.st.fila, mc = actual.st.columna;
            switch(actual.st.orientacion) {
                case 0: jf-=2; mf--; break; case 1: jf-=2; jc+=2; mf--; mc++; break;
                case 2: jc+=2; mc++; break; case 3: jf+=2; jc+=2; mf++; mc++; break;
                case 4: jf+=2; mf++; break; case 5: jf+=2; jc-=2; mf++; mc--; break;
                case 6: jc-=2; mc--; break; case 7: jf-=2; jc-=2; mf--; mc--; break;
            }
            if (jf >= 0 && jf < (int)mapaResultado.size() && jc >= 0 && jc < (int)mapaResultado[0].size()) {
                unsigned char c_mid = mapaResultado[mf][mc], c_fin = mapaResultado[jf][jc];
                unsigned char e_mid = mapaEntidades[mf][mc], e_fin = mapaEntidades[jf][jc];
                if (c_mid != 'M' && c_mid != 'P' && c_mid != 'B' && (agua_permitida || c_mid != 'A') &&
                    c_fin != 'M' && c_fin != 'P' && c_fin != 'B' && (agua_permitida || c_fin != 'A')) {
                    if (ignorar_entidades || ((e_mid == '_' || e_mid == '?') && (e_fin == '_' || e_fin == '?'))) {
                        int dif_cota_fin = mapaCotas[jf][jc] - mapaCotas[actual.st.fila][actual.st.columna];
                        if (c_fin == '?' || abs(dif_cota_fin) <= max_desnivel) {
                            nodo_ext hijo = actual;
                            hijo.st.fila = jf; hijo.st.columna = jc;
                            if (c_fin == 'D') hijo.st.zapatillas = true;
                            if (cerrados.find(hijo.st) == cerrados.end()) {
                                hijo.secuencia.push_back(JUMP);
                                cerrados.insert(hijo.st);
                                abierta.push(hijo);
                            }
                        }
                    }
                }
            }
        }
    }
    return list<Action>();
}


// Niveles avanzados (Uso de búsqueda)
/**
 * @brief Comportamiento del ingeniero para el Nivel 2 (búsqueda).
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_2(Sensores sensores) {
    Action accion = IDLE;

    if (!hayPlan) {
        estado origen = {sensores.posF, sensores.posC, (int)sensores.rumbo};
        estado destino = {sensores.BelPosF, sensores.BelPosC, 0};

        plan = BusquedaEnAnchura(origen, destino, false, true);
        if (plan.empty())
            plan = BusquedaEnAnchura(origen, destino, true, true);
        hayPlan = true;
    }

    if (hayPlan && plan.empty() &&
        (sensores.posF != sensores.BelPosF || sensores.posC != sensores.BelPosC)) {
        hayPlan = false;
    }
    if (hayPlan && !plan.empty()) {
        accion = plan.front(); plan.pop_front();
    }
    return accion;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_3(Sensores sensores) {
    // En nivel 3, el Ingeniero solo debe apartarse del Técnico
    if (sensores.agentes[2] == 't') return TURN_SR;
    if (sensores.agentes[1] == 't') return TURN_SR;
    if (sensores.agentes[3] == 't') return TURN_SL;
    return IDLE;
}


// =========================================================
// === MOTOR DE PLANIFICACIÓN DE TUBERÍAS (NIVEL 4) ===
// =========================================================

bool ComportamientoIngeniero::EncontrarPlan_N4(int start_f, int start_c, std::list<Paso>& plan_resultante) {
    plan_resultante.clear();
    std::queue<NodoN4> abiertos;
    std::set<EstadoN4> cerrados;

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
        Paso p = {start_f, start_c, h - start_H}; 
        nodo.secuencia.push_back(p);
        
        abiertos.push(nodo);
        cerrados.insert(st);
    }

    int df[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    while (!abiertos.empty()) {
        NodoN4 actual = abiertos.front();
        abiertos.pop();

        if (mapaResultado[actual.st.f][actual.st.c] == 'U') {
            plan_resultante = actual.secuencia;
            return true;
        }

        for (int i = 0; i < 4; i++) {
            int nf = actual.st.f + df[i];
            int nc = actual.st.c + dc[i];

            if (nf < 0 || nf >= mapaResultado.size() || nc < 0 || nc >= mapaResultado[0].size()) continue;

            unsigned char n_terr = mapaResultado[nf][nc];
            if (n_terr == 'M' || n_terr == 'P' || n_terr == '?') continue; 

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
                // ¡LA LEY DE LA GRAVEDAD Y LA FÍSICA DE ROBOTS! 
                // El agua fluye (actual.st.h >= nh) Y el robot puede bajar a pie (actual.st.h - nh <= 1)
                if (actual.st.h >= nh && (actual.st.h - nh) <= 1) {
                    EstadoN4 siguiente = {nf, nc, nh};
                    
                    if (cerrados.find(siguiente) == cerrados.end()) {
                        cerrados.insert(siguiente);
                        
                        NodoN4 hijo = actual;
                        hijo.st = siguiente;
                        Paso p = {nf, nc, nh - nH};
                        hijo.secuencia.push_back(p);
                        
                        abiertos.push(hijo);
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
    bool exito = EncontrarPlan_N4(sensores.BelPosF, sensores.BelPosC, plan_tuberias);

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


/**
 * @brief Comportamiento del ingeniero para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_5(Sensores sensores) {
    if (sensores.tiempo == 0) {
        estado_obra_ing = ING_CALCULAR_PLAN;
        plan_n5.clear();
        tramo_n5 = 0;
        terraformado_n5 = false;
        hayPlan = false;
        plan.clear();
    }

    if (estado_obra_ing == ING_CALCULAR_PLAN) {
        std::list<Paso> lista_plan;
        EncontrarPlan_N4(sensores.BelPosF, sensores.BelPosC, lista_plan);
        for (auto p : lista_plan) plan_n5.push_back(p);

        if (plan_n5.empty()) return IDLE; // No hay plan, no hacemos nada

        estado_obra_ing = ING_IR_CASILLA; // Pasamos al siguiente estado para empezar a ejecutar el plan
    }

    if (estado_obra_ing == ING_IR_CASILLA) {
        if (tramo_n5 >= (int)plan_n5.size() - 1) return IDLE; 
        
        Paso mi_obj = plan_n5[tramo_n5 + 1];

        if (sensores.posF == mi_obj.fil && sensores.posC == mi_obj.col) {
            estado_obra_ing = ING_TERRAFORMAR; 
        } else {
            if (!hayPlan) {
                estado inicio = {sensores.posF, sensores.posC, (int)sensores.rumbo};
                estado destino = {mi_obj.fil, mi_obj.col, 0};
                plan = BusquedaEnAnchura(inicio, destino, true, true);
                hayPlan = true;
            }

            if (!plan.empty()) {
                Action a = plan.front();
                if (a == WALK) {
                    int nf = sensores.posF, nc = sensores.posC;
                    switch(sensores.rumbo) {
                        case norte: nf--; break; case noreste: nf--; nc++; break;
                        case este: nc++; break; case sureste: nf++; nc++; break;
                        case sur: nf++; break; case suroeste: nf++; nc--; break;
                        case oeste: nc--; break; case noroeste: nf--; nc--; break;
                    }
                    if (nf >= 0 && nf < (int)mapaEntidades.size() && nc >= 0 && nc < (int)mapaEntidades[0].size()) {
                        if (mapaEntidades[nf][nc] != '_' && mapaEntidades[nf][nc] != '?') {
                            return IDLE; // Cedemos el paso temporalmente sin borrar el plan
                        }
                    }
                }
                plan.pop_front();
                return a;
            } else {
                hayPlan = false;
                return IDLE;
            }
        }
    }

    if (estado_obra_ing == ING_TERRAFORMAR) {
        Paso mi_obj = plan_n5[tramo_n5 + 1];
        if (!terraformado_n5) {
            terraformado_n5 = true;
            if (mi_obj.op == 1) return RAISE;
            if (mi_obj.op == -1) return DIG;
        }
        estado_obra_ing = ING_LLAMAR;
    }

    if (estado_obra_ing == ING_LLAMAR) {
        cout << "[INGENIERO] ¡Terreno listo! Pitando al Técnico con COME..." << endl;
        estado_obra_ing = ING_ALINEARSE;
        return COME; 
    }

    if (estado_obra_ing == ING_ALINEARSE) {
        Paso su_obj = plan_n5[tramo_n5]; // Miramos hacia aguas arriba donde estará el Técnico
        
        // ¡LA FUNCIÓN AUXILIAR YA ESTÁ RECUPERADA!
        Orientacion ori_deseada = OrientacionHacia(sensores.posF, sensores.posC, su_obj.fil, su_obj.col);

        if (sensores.rumbo != ori_deseada) {
            int giros = GirosNecesarios(sensores.rumbo, ori_deseada);
            return (giros <= 4) ? TURN_SR : TURN_SL;
        }

        if (sensores.enfrente) {
            cout << "[INGENIERO] Contacto visual mutuo. ¡INSTALANDO TRAMO " << tramo_n5 << "!" << endl;
            tramo_n5++; 
            terraformado_n5 = false;
            hayPlan = false;
            plan.clear();
            estado_obra_ing = ING_IR_CASILLA; 
            return INSTALL;
        } 
        
        return IDLE; 
    }

    return IDLE;
}


/**
 * @brief Comportamiento del ingeniero para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_6(Sensores sensores) {
    ActualizarMapa(sensores);
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

    if (sensores.tiempo == 0 || (sensores.vida == sensores.vida && plan_n5.empty() && tramo_n5 == 0 && !plan_tuberias_hecho)) {
        plan_tuberias_hecho = false;
        est_n6 = 0; plan_n5.clear();
        tramo_n5 = 0; terraformado_n5 = false;
        hayPlan = false; plan.clear();
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
        if (real_c == 'P' || real_c == 'M' || real_c == 'A') return false; 
        if (real_c == 'B' && !tiene_zapatillas) return false;
        
        if (mapaResultado[nf][nc] != '?') {
            if (abs(mapaCotas[nf][nc] - mapaCotas[sens.posF][sens.posC]) > 1) return false;
        }
        return true;
    };

    if (!plan_tuberias_hecho) {
        std::list<Paso> lista_plan;
        if (mapaResultado[sensores.BelPosF][sensores.BelPosC] != '?') {
            if (EncontrarPlan_N4(sensores.BelPosF, sensores.BelPosC, lista_plan)) {
                plan_tuberias_hecho = true;
                for (auto p : lista_plan) plan_n5.push_back(p);
                est_n6 = 1; // Arrancamos la máquina de coser
                hayPlan = false; plan.clear();
                return IDLE; 
            }
        }
        
        // Cartógrafo para buscar la Belkanita
        if (!hayPlan) {
            estado inicio = {sensores.posF, sensores.posC, (int)sensores.rumbo};
            estado destino = {-1, -1, 0};
            
            if (mapaResultado[sensores.BelPosF][sensores.BelPosC] == '?') {
                destino.fila = sensores.BelPosF; destino.columna = sensores.BelPosC;
            } else {
                int min_dist = 999999;
                for (int i = 0; i < (int)mapaResultado.size(); i += 3) { 
                    for (int j = 0; j < (int)mapaResultado[0].size(); j += 3) {
                        if (mapaResultado[i][j] == '?') {
                            int dist = abs(i - sensores.posF) + abs(j - sensores.posC);
                            if (dist < min_dist) { min_dist = dist; destino.fila = i; destino.columna = j; }
                        }
                    }
                }
            }

            if (destino.fila != -1) {
                plan = BusquedaEnAnchura(inicio, destino, false, true); 
                if (plan.empty()) plan = BusquedaEnAnchura(inicio, destino, true, true); 
                hayPlan = true;
                
                if (plan.empty()) {
                    mapaResultado[destino.fila][destino.columna] = 'M'; 
                    hayPlan = false; return ComportamientoIngenieroNivel_1(sensores); 
                }
            } else {
                return ComportamientoIngenieroNivel_1(sensores);
            }
        }

        if (hayPlan && !plan.empty()) {
            Action a = plan.front();
            if (a == WALK) {
                // ¡CLAVE! Si hay alguien, solo espera (IDLE) para no volverse loco
                if (sensores.agentes[2] != '_' || sensores.choque) return IDLE; 
                if (!es_seguro(sensores)) { hayPlan = false; plan.clear(); return TURN_SR; }
            }
            plan.pop_front(); return a;
        } else {
            hayPlan = false; return ComportamientoIngenieroNivel_1(sensores);
        }
    }

    // ========================================================
    // FASE DE OBRA N6: "LA MÁQUINA DE COSER" Blindada
    // ========================================================
    if (tramo_n5 >= (int)plan_n5.size()) return IDLE; // Fin de la obra
    Paso su_obj = plan_n5[tramo_n5]; 

    if (est_n6 == 1) { // 1. IR A LA CASILLA DE LA TUBERÍA
        if (sensores.posF == su_obj.fil && sensores.posC == su_obj.col) {
            est_n6 = 2; return IDLE;
        } else {
            if (!hayPlan) {
                estado inicio = {sensores.posF, sensores.posC, (int)sensores.rumbo};
                estado destino = {su_obj.fil, su_obj.col, 0};
                plan = BusquedaEnAnchura(inicio, destino, true, true);
                hayPlan = true;
            }
            if (!plan.empty()) {
                Action a = plan.front(); 
                if (a == WALK) {
                    if (sensores.agentes[2] != '_' || sensores.choque) return IDLE; // Esperar al Jefe o NPC
                    if (!es_seguro(sensores)) { hayPlan = false; plan.clear(); return TURN_SR; }
                }
                plan.pop_front(); return a;
            } else { hayPlan = false; return TURN_SR; }
        }
    }

    if (est_n6 == 2) { // 2. LLAMAR AL TÉCNICO
        est_n6 = 3;
        return COME;
    }

    if (est_n6 == 3) { // 3. APARTARSE (Dar 1 paso a cualquier sitio libre)
        if (sensores.posF != su_obj.fil || sensores.posC != su_obj.col) {
            est_n6 = 4; return IDLE; // Ya se ha bajado de la casilla
        }
        if (es_seguro(sensores) && sensores.agentes[2] == '_') return WALK;
        return TURN_SR; // Gira hasta encontrar un hueco
    }

    if (est_n6 == 4) { // 4. MIRAR A LA CASILLA
        Orientacion ori_deseada = OrientacionHacia(sensores.posF, sensores.posC, su_obj.fil, su_obj.col);
        if (sensores.rumbo != ori_deseada) {
            return (GirosNecesarios(sensores.rumbo, ori_deseada) <= 4) ? TURN_SR : TURN_SL;
        }
        est_n6 = 5; return IDLE;
    }

    if (est_n6 == 5) { // 5. TERRAFORMAR
        if (!terraformado_n5) {
            terraformado_n5 = true;
            if (su_obj.op == 1) return RAISE;
            if (su_obj.op == -1) return DIG;
        }
        est_n6 = 6; return IDLE;
    }

    if (est_n6 == 6) { // 6. COSER
        if (sensores.enfrente) { // Si el Técnico ha llegado y le mira
            tramo_n5++;
            terraformado_n5 = false; hayPlan = false; plan.clear();
            est_n6 = 1; 
            return INSTALL;
        }
        return IDLE;
    }
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

