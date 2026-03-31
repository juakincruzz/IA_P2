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

char ComportamientoTecnico::ViablePorAltura(char casilla, int dif) {
  // El técnico solo supera desniveles de 1, tenga o no zapatillas
  if (abs(dif) <= 1) return casilla;
  else return 'P';
}

int ComportamientoTecnico::VeoCasillaInteresante(char i, char c, char d) {
  if (c == 'U') return 2;
  else if (i == 'U') return 1;
  else if (d == 'U') return 3;
  // En el nivel 0, las zapatillas no le sirven para nada especial al técnico
  if (c == 'C') return 2;
  else if (i == 'C') return 1;
  else if (d == 'C') return 3;
  else return 0;
}

// Niveles del técnico
Action ComportamientoTecnico::ComportamientoTecnicoNivel_0(Sensores sensores) {
  Action accion = IDLE;

  ActualizarMapa(sensores);

  if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

  if (sensores.superficie[0] == 'U') { 
    return IDLE; 
  }

  char i = ViablePorAltura(sensores.superficie[1], sensores.cota[1] - sensores.cota[0]);
  char c = ViablePorAltura(sensores.superficie[2], sensores.cota[2] - sensores.cota[0]);
  char d = ViablePorAltura(sensores.superficie[3], sensores.cota[3] - sensores.cota[0]);

  int pos = VeoCasillaInteresante(i, c, d);

  // ELIMINAMOS la lógica anticolisión del Técnico. Él no esquiva.

  switch (pos) {
    case 2: accion = WALK; break;
    case 1: accion = TURN_SL; break;
    case 3: accion = TURN_SR; break;
    default: accion = TURN_SL; break;
  }

  last_action = accion;
  return accion;
}







// --- FUNCIONES AUXILIARES NIVEL 1 (TÉCNICO) ---
bool ComportamientoTecnico::es_transitable_N1(unsigned char c, bool zap) const {
  // LISTA NEGRA: El técnico no puede pisar muros, agua ni precipicios
  if (c == 'M' || c == 'P' || c == 'A' || c == '?') return false;
  // El bosque es intransitable si no tiene zapatillas
  if (c == 'B' && !zap) return false;
  return true;
}

char ComportamientoTecnico::ViablePorAltura_N1(char casilla, int dif) {
  // El técnico no mejora su salto con zapatillas, siempre máximo 1
  if (abs(dif) <= 1) return casilla;
  else return 'P';
}

int ComportamientoTecnico::VeoCasillaInteresante_N1(char i, char c, char d, bool zap) {
  if (es_transitable_N1(c, zap)) return 2; // 1º Frente
  if (es_transitable_N1(d, zap)) return 3; // 2º DERECHA (Su mano dominante)
  if (es_transitable_N1(i, zap)) return 1; // 3º Izquierda
  return 0;
}

/**
 * @brief Comportamiento reactivo del técnico para el Nivel 1.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_1(Sensores sensores) {
  // INICIO DEL MÉTODO ComportamientoTecnicoNivel_1
  Action accion = IDLE;
  ActualizarMapa(sensores);

  if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

  // Si estamos en medio de un giro de 90º, lo terminamos (HACIA LA DERECHA)
  if (giro45Izq > 0) {
      giro45Izq--;
      last_action = TURN_SR; 
      return TURN_SR;
  }

  // 1. ANOTAMOS QUE ACABAMOS DE PISAR ESTA CASILLA
  matriz_visitas[sensores.posF][sensores.posC]++;

  // 2. VISIÓN Y ANTICOLISIÓN (Igual que antes)
  // (Nota: para el técnico, quita tiene_zapatillas de ViablePorAltura_N1)
  char i = ViablePorAltura_N1(sensores.superficie[1], sensores.cota[1] - sensores.cota[0]);
  char c = ViablePorAltura_N1(sensores.superficie[2], sensores.cota[2] - sensores.cota[0]);
  char d = ViablePorAltura_N1(sensores.superficie[3], sensores.cota[3] - sensores.cota[0]);

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
  // ¡MAGIA MODULAR! Si estamos en Nivel 6, usamos la lista negra adaptada al Técnico.
  bool trans_c = (sensores.nivel == 6) ? (c != 'M' && c != 'P' && c != 'A' && c != '?' && (tiene_zapatillas || c != 'B')) : es_transitable_N1(c, tiene_zapatillas);
  bool trans_i = (sensores.nivel == 6) ? (i != 'M' && i != 'P' && i != 'A' && i != '?' && (tiene_zapatillas || i != 'B')) : es_transitable_N1(i, tiene_zapatillas);
  bool trans_d = (sensores.nivel == 6) ? (d != 'M' && d != 'P' && d != 'A' && d != '?' && (tiene_zapatillas || d != 'B')) : es_transitable_N1(d, tiene_zapatillas);

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





ComportamientoTecnico::Estado ComportamientoTecnico::AplicaAccion_N2(const Estado& st, Action act) {
    Estado nuevo = st;
    if (act == TURN_SL) {
        nuevo.brujula = (Orientacion)((nuevo.brujula + 7) % 8);
    } else if (act == TURN_SR) {
        nuevo.brujula = (Orientacion)((nuevo.brujula + 1) % 8);
    } else if (act == WALK || act == JUMP) {
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
    }
    return nuevo;
}

bool ComportamientoTecnico::EsValida_N2(const Estado& st, Action act) {
    if (act == TURN_SL || act == TURN_SR) return true;

    if (act == WALK || act == JUMP) {
        Estado destino = AplicaAccion_N2(st, act);
        
        if (destino.f < 0 || destino.f >= mapaResultado.size() || 
            destino.c < 0 || destino.c >= mapaResultado[0].size()) {
            return false;
        }

        unsigned char c = mapaResultado[destino.f][destino.c];
        // En principio el técnico tampoco pasa por el bosque en este nivel a menos que tenga zapatillas, 
        // pero la meta debería ser accesible por caminos limpios en su mitad del mapa.
        if (c == 'M' || c == 'P' || c == 'A' || c == 'B') return false; 

        unsigned char entidad = mapaEntidades[destino.f][destino.c];
        if (entidad != '_' && entidad != '?') return false;

        int dif = mapaCotas[destino.f][destino.c] - mapaCotas[st.f][st.c];

        if (act == WALK && abs(dif) > 1) return false;
        if (act == JUMP && dif != 2) return false;

        return true;
    }
    return false;
}

bool ComportamientoTecnico::EncontrarPlan_N2(const Estado& inicio, int dest_f, int dest_c, std::list<Action>& plan_resultante) {
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

        bool llegado = false;
        if (dest_f != -1 && dest_c != -1) {
            llegado = (actual.st.f == dest_f && actual.st.c == dest_c);
        } else {
            llegado = (mapaResultado[actual.st.f][actual.st.c] == 'U');
        }

        if (llegado) {
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

/**
 * @brief Comportamiento del técnico para el Nivel 2.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_2(Sensores sensores) {
  Action accion = IDLE;

  if (!hay_plan) {
      cout << "[TÉCNICO] Cerebro activado: Buscando ruta hacia (" << sensores.BelPosF << ", " << sensores.BelPosC << ")..." << endl;
      Estado estado_inicial;
      estado_inicial.f = sensores.posF;
      estado_inicial.c = sensores.posC;
      estado_inicial.brujula = sensores.rumbo;

      hay_plan = EncontrarPlan_N2(estado_inicial, sensores.BelPosF, sensores.BelPosC, plan);

      if (hay_plan) {
          cout << "[TÉCNICO] ¡Eureka! Plan maestro encontrado. Longitud de la ruta: " << plan.size() << " pasos." << endl;
      } else {
          cout << "[TÉCNICO] ¡ERROR! No hay camino posible." << endl;
          hay_plan = true; 
          return IDLE;
      }
  }

  if (hay_plan && !plan.empty()) {
      accion = plan.front();
      plan.pop_front();
  }

  return accion; 
}



// =========================================================
// === MOTOR DE BÚSQUEDA A* NIVEL 3 (TÉCNICO) ===
// =========================================================

int ComportamientoTecnico::CostoBateria_N3(const EstadoN3& st, Action act) {
    unsigned char terreno = mapaResultado[st.f][st.c];
    int coste = 0;

    // Si tiene zapatillas, el bosque se transita como si fuera camino
    if (terreno == 'B' && st.zapatillas) terreno = 'C';

    if (act == WALK) {
        int cota_origen = mapaCotas[st.f][st.c];
        EstadoN3 destino = AplicaAccion_N3(st, act); 
        int cota_destino = mapaCotas[destino.f][destino.c];
        int dif = cota_destino - cota_origen;

        if (terreno == 'A') { coste = 60; if(dif > 0) coste += 5; else if(dif < 0) coste -= 2; }
        else if (terreno == 'H') { coste = 6; if(dif > 0) coste += 5; else if(dif < 0) coste -= 2; }
        else if (terreno == 'S') { coste = 3; if(dif > 0) coste += 5; else if(dif < 0) coste -= 2; }
        else { coste = 1; }
        
        if (coste < 0) coste = 0; 

    } else if (act == TURN_SL || act == TURN_SR) {
        if (terreno == 'A') coste = 5;
        else if (terreno == 'H') coste = 2;
        else if (terreno == 'S') coste = 1;
        else coste = 1;
    } else if (act == IDLE) {
        coste = 0;
    }
    return coste;
}

int ComportamientoTecnico::Heuristica(const EstadoN3& actual, int dest_f, int dest_c) {
    return std::max(abs(actual.f - dest_f), abs(actual.c - dest_c));
}

ComportamientoTecnico::EstadoN3 ComportamientoTecnico::AplicaAccion_N3(const EstadoN3& st, Action act) {
    EstadoN3 nuevo = st;
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
        // ¡PARCHE ANTICRASH! Comprobamos límites antes de leer el mapa
        if (nuevo.f >= 0 && nuevo.f < mapaResultado.size() && nuevo.c >= 0 && nuevo.c < mapaResultado[0].size()) {
            if (mapaResultado[nuevo.f][nuevo.c] == 'D') {
                nuevo.zapatillas = true;
            }
        }
    }
    return nuevo;
}

bool ComportamientoTecnico::EsValida_N3(const EstadoN3& st, Action act, bool ignorar_entidades) {
    if (act == TURN_SL || act == TURN_SR) return true;

    if (act == WALK) {
        EstadoN3 destino = AplicaAccion_N3(st, act);
        
        if (destino.f < 0 || destino.f >= (int)mapaResultado.size() || 
            destino.c < 0 || destino.c >= (int)mapaResultado[0].size()) {
            return false;
        }

        unsigned char c = mapaResultado[destino.f][destino.c];
        if (c == 'M' || c == 'P') return false; 
        if (c == 'B' && !st.zapatillas) return false; 

        unsigned char entidad = mapaEntidades[destino.f][destino.c];
        if (!ignorar_entidades && entidad != '_' && entidad != '?') return false;

        int dif = mapaCotas[destino.f][destino.c] - mapaCotas[st.f][st.c];
        
        // ¡LA CLAVE! Si es '?', somos optimistas y trazamos la ruta
        if (c != '?' && abs(dif) > 1) return false; 

        return true;
    }
    return false;
}

bool ComportamientoTecnico::EncontrarPlan_N3(const EstadoN3& inicio, int dest_f, int dest_c, std::list<Action>& plan_resultante, bool ignorar_entidades, bool parar_adyacente) {
    plan_resultante.clear();
    std::priority_queue<NodoN3, std::vector<NodoN3>, std::greater<NodoN3>> abiertos;
    std::set<EstadoN3> cerrados;

    NodoN3 n_inicial;
    n_inicial.st = inicio;
    n_inicial.coste_g = 0;
    n_inicial.coste_h = Heuristica(inicio, dest_f, dest_c);
    abiertos.push(n_inicial);

    while (!abiertos.empty()) {
        NodoN3 actual = abiertos.top();
        abiertos.pop();

        if (cerrados.find(actual.st) != cerrados.end()) continue;
        cerrados.insert(actual.st);

        if (parar_adyacente) {
            // ¡PARCHE DIAGONAL! Si la distancia máxima es 1 (incluso diagonal), hemos llegado.
            if (std::max(abs(actual.st.f - dest_f), abs(actual.st.c - dest_c)) <= 1 && (actual.st.f != dest_f || actual.st.c != dest_c)) {
                plan_resultante = actual.secuencia;
                return true;
            }
        } else {
            if (actual.st.f == dest_f && actual.st.c == dest_c) {
                plan_resultante = actual.secuencia;
                return true;
            }
        }

        Action acciones[] = {WALK, TURN_SL, TURN_SR};
        for (Action accion : acciones) {
            if (EsValida_N3(actual.st, accion, ignorar_entidades)) {
                EstadoN3 siguiente = AplicaAccion_N3(actual.st, accion);
                if (cerrados.find(siguiente) == cerrados.end()) {
                    int coste_accion = CostoBateria_N3(actual.st, accion);
                    NodoN3 hijo;
                    hijo.st = siguiente;
                    hijo.secuencia = actual.secuencia;
                    hijo.secuencia.push_back(accion);
                    hijo.coste_g = actual.coste_g + coste_accion;
                    hijo.coste_h = Heuristica(siguiente, dest_f, dest_c);
                    abiertos.push(hijo);
                }
            }
        }
    }
    return false;
}

/**
 * @brief Comportamiento del técnico para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_3(Sensores sensores) {
    Action accion = IDLE;

    if (!hay_plan) {
        cout << "[TÉCNICO] Cerebro A* activado: Buscando ruta económica hacia (" << sensores.BelPosF << ", " << sensores.BelPosC << ")..." << endl;
        EstadoN3 estado_inicial;
        estado_inicial.f = sensores.posF;
        estado_inicial.c = sensores.posC;
        estado_inicial.brujula = sensores.rumbo;
        estado_inicial.zapatillas = false; 
        
        if (mapaResultado[sensores.posF][sensores.posC] == 'D') {
             estado_inicial.zapatillas = true;
        }

        hay_plan = EncontrarPlan_N3(estado_inicial, sensores.BelPosF, sensores.BelPosC, plan);

        if (hay_plan) {
            cout << "[TÉCNICO] ¡Eureka! Plan A* óptimo encontrado. Longitud: " << plan.size() << " pasos." << endl;
        } else {
            cout << "[TÉCNICO] ¡ERROR! No hay camino posible." << endl;
            hay_plan = true; 
            return IDLE;
        }
    }

    if (hay_plan && !plan.empty()) {
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



// =========================================================
// === MÁQUINA DE ESTADOS COOPERATIVA (NIVEL 5) ===
// =========================================================

// Clon exacto del algoritmo del Ingeniero para que ambos deduzcan el mismo plan
bool ComportamientoTecnico::EncontrarPlan_N5_Arquitecto(int start_f, int start_c, std::list<Paso>& plan_resultante) {
    plan_resultante.clear();
    std::queue<NodoN4_Tecnico> abiertos;
    std::set<EstadoN4_Tecnico> cerrados;

    unsigned char start_terr = mapaResultado[start_f][start_c];
    if (start_terr == 'M' || start_terr == 'P') return false; 
    
    int start_H = mapaCotas[start_f][start_c];
    std::vector<int> alturas_inicio;
    
    if (start_terr == 'A') alturas_inicio.push_back(start_H);
    else {
        alturas_inicio.push_back(start_H);
        if (start_H > 0) alturas_inicio.push_back(start_H - 1);
        if (start_H < 9) alturas_inicio.push_back(start_H + 1);
    }

    for (int h : alturas_inicio) {
        EstadoN4_Tecnico st = {start_f, start_c, h};
        NodoN4_Tecnico nodo;
        nodo.st = st;
        Paso p = {start_f, start_c, h - start_H}; 
        nodo.secuencia.push_back(p);
        abiertos.push(nodo);
        cerrados.insert(st);
    }

    int df[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    while (!abiertos.empty()) {
        NodoN4_Tecnico actual = abiertos.front();
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
            if (n_terr == 'M' || n_terr == 'P') continue; 

            int nH = mapaCotas[nf][nc];
            std::vector<int> alturas_vecino;
            
            if (n_terr == 'A') alturas_vecino.push_back(nH); 
            else {
                alturas_vecino.push_back(nH);
                if (nH > 0) alturas_vecino.push_back(nH - 1);
                if (nH < 9) alturas_vecino.push_back(nH + 1);
            }

            for (int nh : alturas_vecino) {
                // GRAVEDAD Y CAMINABILIDAD
                if (actual.st.h >= nh && (actual.st.h - nh) <= 1) {
                    EstadoN4_Tecnico siguiente = {nf, nc, nh};
                    if (cerrados.find(siguiente) == cerrados.end()) {
                        cerrados.insert(siguiente);
                        NodoN4_Tecnico hijo = actual;
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

// =========================================================
// FUNCIONES AUXILIARES DE ALINEACIÓN (NIVEL 5)
// =========================================================

static Orientacion OrientacionHacia_Tec(int fromF, int fromC, int toF, int toC) {
    if (toF < fromF) return norte;
    if (toF > fromF) return sur;
    if (toC > fromC) return este;
    if (toC < fromC) return oeste;
    return norte;
}

static int GirosNecesarios_Tec(Orientacion actual, Orientacion objetivo) {
    return ((int)objetivo - (int)actual + 8) % 8;
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
        plan.clear();
        hay_plan = false;
        plan_n5.clear();
        tramo_n5 = 0;
        dest_f = -1;
        dest_c = -1;
        terraformado_n5 = false;
    }

    if (plan_n5.empty()) {
        std::list<Paso> lista_plan;
        EncontrarPlan_N5_Arquitecto(sensores.BelPosF, sensores.BelPosC, lista_plan);
        for (auto p : lista_plan) plan_n5.push_back(p);
    }

    // SI EL JEFE PITA (COME), REACCIONAMOS AL VUELO
    if (sensores.venpaca) {
        int ing_f = sensores.GotoF;
        int ing_c = sensores.GotoC;
        
        int tramo_encontrado = -1;
        for (int i = 1; i < (int)plan_n5.size(); i++) {
            if (plan_n5[i].fil == ing_f && plan_n5[i].col == ing_c) {
                tramo_encontrado = i - 1; 
                break;
            }
        }

        if (tramo_encontrado >= 0) {
            if (dest_f != plan_n5[tramo_encontrado].fil || dest_c != plan_n5[tramo_encontrado].col) {
                tramo_n5 = tramo_encontrado;
                dest_f = plan_n5[tramo_n5].fil; // El Técnico va aguas arriba
                dest_c = plan_n5[tramo_n5].col;
                cout << "[TÉCNICO] ¡El Jefe llama! Voy al tramo " << tramo_n5 << " en (" << dest_f << ", " << dest_c << ")" << endl;
                estado_obra_tec = TEC_IR_CASILLA;
                plan.clear(); 
                hay_plan = false;
                terraformado_n5 = false;
            }
        }
    }

    if (estado_obra_tec == TEC_ESPERAR_AVISO) {
        return IDLE;
    }

    if (estado_obra_tec == TEC_IR_CASILLA) {
        if (sensores.posF == dest_f && sensores.posC == dest_c) {
            Paso mi_obj = plan_n5[tramo_n5];
            if (!terraformado_n5) {
                terraformado_n5 = true;
                if (mi_obj.op == -1) return DIG; // El Técnico solo puede excavar si hace falta
            }
            estado_obra_tec = TEC_ALINEARSE;
        } else {
            if (!hay_plan) {
                EstadoN3 inicio = {sensores.posF, sensores.posC, sensores.rumbo, false};
                if (mapaResultado[sensores.posF][sensores.posC] == 'D') inicio.zapatillas = true;
                EncontrarPlan_N3(inicio, dest_f, dest_c, plan, true); 
                hay_plan = true;
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
                            return IDLE; // Cedemos el paso temporalmente sin borrar el A-Estrella
                        }
                    }
                }
                plan.pop_front();
                return a;
            } else {
                hay_plan = false;
                return IDLE;
            }
        }
    }

    if (estado_obra_tec == TEC_ALINEARSE) {
        Paso su_obj = plan_n5[tramo_n5 + 1]; // Miramos hacia aguas abajo
        Orientacion ori_deseada = OrientacionHacia_Tec(sensores.posF, sensores.posC, su_obj.fil, su_obj.col);

        if (sensores.rumbo != ori_deseada) {
            int giros = GirosNecesarios_Tec(sensores.rumbo, ori_deseada);
            return (giros <= 4) ? TURN_SR : TURN_SL;
        }

        if (sensores.enfrente) {
            cout << "[TÉCNICO] Contacto visual mutuo. ¡INSTALANDO!" << endl;
            estado_obra_tec = TEC_ESPERAR_AVISO;
            hay_plan = false;
            plan.clear();
            return INSTALL;
        } 
        
        return IDLE; // Esperamos pacientes al Jefe
    }

    return IDLE;
}

/**
 * @brief Comportamiento del técnico para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_6(Sensores sensores) {
    ActualizarMapa(sensores);
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

    if (sensores.tiempo == 0) {
        estado_n6 = 0;
        dest_n6_f = -1; dest_n6_c = -1;
        hay_plan = false; plan.clear();
        intento_orbita_n6 = 0;
    }

    auto es_seguro = [&](Sensores sens) {
        unsigned char real_c = sens.superficie[2];
        if (real_c == 'P' || real_c == 'M' || real_c == 'A') return false;
        if (real_c == 'B' && !tiene_zapatillas) return false;
        if (abs(sens.cota[2] - sens.cota[0]) > 1) return false;
        if (sens.choque) return false;
        if (sens.agentes[2] != '_') return false;
        return true;
    };

    // --- RECEPCIÓN DEL COME ---
    if (sensores.venpaca) {
        if (dest_n6_f != sensores.GotoF || dest_n6_c != sensores.GotoC || estado_n6 == 4) {
            dest_n6_f = sensores.GotoF;
            dest_n6_c = sensores.GotoC;
            estado_n6 = 1;
            plan.clear(); hay_plan = false;
            intento_orbita_n6 = 0;
        }
    }

    // Las 4 casillas ortogonales alrededor del ingeniero
    int df[] = {-1, 1, 0, 0}; // Norte, Sur, Este, Oeste del ingeniero
    int dc[] = {0, 0, 1, -1};

    switch(estado_n6) {
        case 0: { // EXPLORAR hasta recibir COME
            return IDLE;
        }

        case 1: {
            // Elegir la mejor casilla adyacente ortogonal por cercanía
            int mejor_f = -1, mejor_c = -1;
            int mejor_dist = 999999;
            int df[] = {-1, 1, 0, 0};
            int dc[] = {0, 0, 1, -1};
            
            // Si ya estamos en una casilla adyacente ortogonal al ingeniero, ir directo a alinearse
            for (int i = 0; i < 4; i++) {
                int cf = dest_n6_f + df[i];
                int cc = dest_n6_c + dc[i];
                if (sensores.posF == cf && sensores.posC == cc) {
                    estado_n6 = 2;
                    return IDLE;
                }
            }

            // Buscar la casilla adyacente no probada más cercana
            int target_f = dest_n6_f + df[intento_orbita_n6 % 4];
            int target_c = dest_n6_c + dc[intento_orbita_n6 % 4];

            if (!hay_plan) {
                EstadoN3 inicio = {sensores.posF, sensores.posC, sensores.rumbo, tiene_zapatillas};
                unsigned char temp = mapaResultado[dest_n6_f][dest_n6_c];
                mapaResultado[dest_n6_f][dest_n6_c] = 'M';
                EncontrarPlan_N3(inicio, target_f, target_c, plan, true, false);
                mapaResultado[dest_n6_f][dest_n6_c] = temp;
                hay_plan = true;
            }
            if (!plan.empty()) {
                Action a = plan.front();
                if (a == WALK && !es_seguro(sensores)) {
                    hay_plan = false; plan.clear();
                    intento_orbita_n6++;
                    if (intento_orbita_n6 >= 4) { intento_orbita_n6 = 0; return TURN_SR; }
                    return IDLE;
                }
                plan.pop_front();
                return a;
            } else {
                hay_plan = false;
                intento_orbita_n6++;
                if (intento_orbita_n6 >= 4) { intento_orbita_n6 = 0; return TURN_SR; }
                return IDLE;
            }
        }

        case 2: { // ALINEARSE mirando hacia el ingeniero
            Orientacion ori_deseada = OrientacionHacia_Tec(sensores.posF, sensores.posC,
                                                            dest_n6_f, dest_n6_c);
            if (sensores.rumbo != ori_deseada) {
                int giros = GirosNecesarios_Tec(sensores.rumbo, ori_deseada);
                return (giros <= 4) ? TURN_SR : TURN_SL;
            }

            if (sensores.enfrente) {
                // ¡Contacto visual! Instalar
                estado_n6 = 4;
                hay_plan = false; plan.clear();
                dest_n6_f = -1; dest_n6_c = -1;
                return INSTALL;
            } else {
                // Casilla incorrecta, probar la siguiente
                intento_orbita_n6++;
                hay_plan = false; plan.clear();
                if (intento_orbita_n6 >= 4) intento_orbita_n6 = 0;
                estado_n6 = 1;
                return IDLE;
            }
        }

        case 3: // (reservado)
            return IDLE;

        case 4: // ESPERAR siguiente COME
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
 * @brief Comprueba si una casilla es transitable (no es muro ni precipicio)
 */
bool ComportamientoTecnico::es_camino(unsigned char casilla) const {
    // Para el motor, consideramos camino cualquier cosa que no sea un obstáculo duro
    return (casilla != 'M' && casilla != 'P');
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