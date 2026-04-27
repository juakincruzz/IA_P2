#include "tecnico.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <queue>
#include <set>
#include <map>
#include <vector>
#include <algorithm>



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

// =========================================================================
// FUNCIONES AUXILIARES - NIVEL 0 (Técnico)
// =========================================================================

/**
    * @brief Compruebo accesibilidad por altura para el Técnico.
    *        El Técnico siempre tiene desnivel máximo de 1 (sin mejora por zapatillas).
*/
char ComportamientoTecnico::ViablePorAltura(char casilla, int dif) {
    if (abs(dif) <= 1) return casilla;
    else return 'P';
}

/**
    * @brief Evalúo las 3 casillas frontales y devuelve la dirección más interesante.
    *        Prioridad: U (meta) > D (zapatillas) > C (camino).
*/
int ComportamientoTecnico::VeoCasillaInteresante(char i, char c, char d) {
    if (c == 'U')  return 2;
    if (i == 'U')  return 1;
    if (d == 'U')  return 3;

    // Busco zapatillas si no las tengo aún
    if (!tiene_zapatillas) {
        if (c == 'D') return 2;
        if (i == 'D') return 1;
        if (d == 'D') return 3;
    }

    if (c == 'C') return 2;
    if (i == 'C') return 1;
    if (d == 'C') return 3;

    return 0;
}

// =========================================================================
// NIVEL 0 - COMPORTAMIENTO REACTIVO (Técnico)
// =========================================================================
// Estrategia: exploración guiada por feromonas, simétrica al Ingeniero pero
// con desempate opuesto (recto > derecha > izquierda) para reducir colisiones.
//
// Mecanismos específicos del Técnico:
//   - Protocolo de retroceso: ante deadlock con el Ingeniero, gira 180° y
//     retrocede 3 pasos para despejar el camino.
//   - Escape por visitas: si detecta que lleva muchas visitas en la misma
//     casilla con el Ingeniero cerca, activa retroceso ampliado.
//   - Anti-oscilación en 8 dirs con prioridad C/D/U > S > H (igual que Ingeniero).
//   - Penalización de terreno no-camino (+50 visitas) para volver a 'C'.
// =========================================================================

Action ComportamientoTecnico::ComportamientoTecnicoNivel_0(Sensores sensores) {
    Action accion = IDLE;
    ActualizarMapa(sensores);

    // Recojo zapatillas, paro si estamos en la meta
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;
    if (sensores.superficie[0] == 'U') return IDLE;

    // Protocolo de retroceso: 4 giros (180°) + 3 WALK hacia atrás
    if (retroceder_n0 > 0) {
        retroceder_n0--;

        if (retroceder_n0 >= 3) { last_action = TURN_SR; return TURN_SR; }
        else                    { last_action = WALK;    return WALK;    }
    }

    // Actualizo feromonas y reseteo bloqueo tras avanzar
    if (last_action == WALK) giros_sin_avanzar_n0 = 0;
    matriz_visitas[sensores.posF][sensores.posC]++;

    // Penalizo terreno no-camino para incentivar retorno a 'C'
    unsigned char terreno_actual = sensores.superficie[0];
    if (terreno_actual == 'H' || terreno_actual == 'S') matriz_visitas[sensores.posF][sensores.posC] += 50;

    // Evalúo accesibilidad por altura
    char ci = ViablePorAltura(sensores.superficie[1], sensores.cota[1] - sensores.cota[0]);
    char cc = ViablePorAltura(sensores.superficie[2], sensores.cota[2] - sensores.cota[0]);
    char cd = ViablePorAltura(sensores.superficie[3], sensores.cota[3] - sensores.cota[0]);

    // Anticolisión: marco al Ingeniero como obstáculo
    if (sensores.agentes[1] == 'i') ci = 'P';
    if (sensores.agentes[2] == 'i') cc = 'P';
    if (sensores.agentes[3] == 'i') cd = 'P';

    // Si la meta está adyacente, voy directamente
    if (cc == 'U') { last_action = WALK;    return WALK;    }
    if (ci == 'U') { last_action = TURN_SL; return TURN_SL; }
    if (cd == 'U') { last_action = TURN_SR; return TURN_SR; }

    // Calculo coordenadas adyacentes
    ubicacion actual   = {sensores.posF, sensores.posC, sensores.rumbo};
    ubicacion u_izq    = actual;
    u_izq.brujula      = (Orientacion)((actual.brujula + 7) % 8);
    u_izq              = Delante(u_izq);
    ubicacion u_frente = Delante(actual);
    ubicacion u_der    = actual;
    u_der.brujula      = (Orientacion)((actual.brujula + 1) % 8);
    u_der              = Delante(u_der);

    // Casillas transitables: C/D siempre; S/H si bloqueado o fuera de camino
    bool desesperado = (giros_sin_avanzar_n0 > 5);
    bool fuera_de_camino = (sensores.superficie[0] == 'H' || sensores.superficie[0] == 'S');
    bool ok_i = (ci == 'C' || ci == 'D' || ((desesperado || fuera_de_camino) && (ci == 'S' || ci == 'H')));
    bool ok_c = (cc == 'C' || cc == 'D' || ((desesperado || fuera_de_camino) && (cc == 'S' || cc == 'H')));
    bool ok_d = (cd == 'C' || cd == 'D' || ((desesperado || fuera_de_camino) && (cd == 'S' || cd == 'H')));

    // Consulto feromonas
    int vis_i = ok_i ? matriz_visitas[u_izq.f][u_izq.c]      : 999999;
    int vis_c = ok_c ? matriz_visitas[u_frente.f][u_frente.c] : 999999;
    int vis_d = ok_d ? matriz_visitas[u_der.f][u_der.c]       : 999999;

    // Elijo dirección menos visitada (desempate: recto > derecha > izquierda)
    int min_vis = min({vis_i, vis_c, vis_d});

    int pos = 0;
    if (min_vis < 999999) {
        // Técnico desempata: recto > derecha > izquierda
        if      (vis_c == min_vis) pos = 2;
        else if (vis_d == min_vis) pos = 3;
        else                       pos = 1;
    }

    // Fallback: busco caminos en filas 2-3 del cono de visión
    if (pos == 0) {
        bool hay_izq = false, hay_der = false;
        for (int k = 4; k <= 5; k++)
            if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D') hay_izq = true;
        for (int k = 7; k <= 8; k++)
            if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D') hay_der = true;
        for (int k = 9; k <= 11; k++)
            if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D') hay_izq = true;
        for (int k = 13; k <= 15; k++)
            if (sensores.superficie[k]=='C'||sensores.superficie[k]=='U'||sensores.superficie[k]=='D') hay_der = true;

        if (hay_der && !hay_izq) pos = 3;
        else if (hay_izq && !hay_der) pos = 1;
        else if (hay_der && hay_izq) pos = girar_derecha_n0 ? 3 : 1; 
    }

    // Contador de turnos sin avanzar
    giros_sin_avanzar_n0++;

    // Deadlock con Ingeniero: activo retroceso
    if (pos == 0) {
        bool veo_ing = false;

        for (int k = 1; k <= 3; k++)
            if (sensores.agentes[k] == 'i') veo_ing = true;

        if (veo_ing && giros_sin_avanzar_n0 >= 3) {
            retroceder_n0 = 7;
            giros_sin_avanzar_n0 = 0;
            girar_derecha_n0 = !girar_derecha_n0;
            last_action = TURN_SR;
            return TURN_SR;
        }

        if (giros_sin_avanzar_n0 >= 8) {
            girar_derecha_n0 = !girar_derecha_n0;
            giros_sin_avanzar_n0 = 0;
        }
    }

    // Escape por visitas: si muchas visitas en esta casilla con Ingeniero cerca
    {
        bool veo_ing = false;
        for (int k = 1; k <= 3; k++)
            if (sensores.agentes[k] == 'i') veo_ing = true;

        if (veo_ing && matriz_visitas[sensores.posF][sensores.posC] > 10) {
            retroceder_n0 = 10;
            girar_derecha_n0 = !girar_derecha_n0;
            giros_sin_avanzar_n0 = 0;
            matriz_visitas[sensores.posF][sensores.posC] = 0;
            last_action = TURN_SR;
            return TURN_SR;
        }
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

                    if (dif <= 1) {
                        // Orientarme hacia esa dirección
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

    // Ejecuto acción elegida
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
// FUNCIONES AUXILIARES - NIVEL 1 (Técnico)
// =========================================================================

/** @brief Transitabilidad nivel 1: lista negra (M, P, A, B sin zapatillas). */
bool ComportamientoTecnico::es_transitable_N1(unsigned char c, bool zap) const {
    if (c == 'M' || c == 'P' || c == 'A' || c == '?') return false;
    if (c == 'B') return false;
    return true;
}

/** @brief Filtro de altura para nivel 1 (Técnico siempre máx desnivel 1). */
char ComportamientoTecnico::ViablePorAltura_N1(char casilla, int dif) {
    if (abs(dif) <= 1) return casilla;
    else return 'P';
}

/** @brief Elijo dirección preferente entre 3 casillas. */
int ComportamientoTecnico::VeoCasillaInteresante_N1(char i, char c, char d, bool zap) {
    if (es_transitable_N1(c, zap)) return 2; 
    if (es_transitable_N1(d, zap)) return 3; // Técnico prefiere derecha
    if (es_transitable_N1(i, zap)) return 1; 
    return 0;
}

// =========================================================================
// NIVEL 1 - EXPLORACIÓN CON MAPA DE FEROMONAS (Técnico)
// =========================================================================
// Misma estrategia que el Ingeniero nivel 1 pero con diferencias clave:
//   - Desempate en visitas: recto > izquierda > derecha (igual que Ingeniero)
//   - Giro de callejón sin salida: a la derecha (opuesto al Ingeniero)
//   - El Técnico no mejora su desnivel con zapatillas (siempre máx 1)
//   - El bosque 'B' es transitable si tiene zapatillas
// =========================================================================

Action ComportamientoTecnico::ComportamientoTecnicoNivel_1(Sensores sensores) {
    Action accion = IDLE;
    ActualizarMapa(sensores);

    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

    // Completo giro de 90 grados pendiente (el Técnico gira a la derecha)
    if (giro45Izq > 0) {
        giro45Izq--;
        last_action = TURN_SR; 
        return TURN_SR;
    }

    // Actualizo feromonas
    matriz_visitas[sensores.posF][sensores.posC]++;

    // Evalúo las 3 casillas frontales
    char i = ViablePorAltura_N1(sensores.superficie[1], sensores.cota[1] - sensores.cota[0]);
    char c = ViablePorAltura_N1(sensores.superficie[2], sensores.cota[2] - sensores.cota[0]);
    char d = ViablePorAltura_N1(sensores.superficie[3], sensores.cota[3] - sensores.cota[0]);

    // Anticolisión
    if (sensores.agentes[1] != '_') i = 'P';
    if (sensores.agentes[2] != '_') c = 'P';
    if (sensores.agentes[3] != '_') d = 'P';

    // Calcular coordenadas adyacentes
    ubicacion actual = {sensores.posF, sensores.posC, sensores.rumbo};
    ubicacion u_frente = Delante(actual);
    ubicacion u_izq = actual;
    u_izq.brujula = (Orientacion)((actual.brujula + 7) % 8);
    u_izq = Delante(u_izq);
    ubicacion u_der = actual;
    u_der.brujula = (Orientacion)((actual.brujula + 1) % 8);
    u_der = Delante(u_der);

    // Transitabilidad (nivel 6 reutiliza este código con lista negra adaptada)
    bool trans_c = (sensores.nivel == 6) ? (c != 'M' && c != 'P' && c != 'A' && c != '?' && (tiene_zapatillas || c != 'B')) : es_transitable_N1(c, tiene_zapatillas);
    bool trans_i = (sensores.nivel == 6) ? (i != 'M' && i != 'P' && i != 'A' && i != '?' && (tiene_zapatillas || i != 'B')) : es_transitable_N1(i, tiene_zapatillas);
    bool trans_d = (sensores.nivel == 6) ? (d != 'M' && d != 'P' && d != 'A' && d != '?' && (tiene_zapatillas || d != 'B')) : es_transitable_N1(d, tiene_zapatillas);

    // Consulto feromonas
    int vis_frente = trans_c ? matriz_visitas[u_frente.f][u_frente.c] : 999999;
    int vis_izq = trans_i ? matriz_visitas[u_izq.f][u_izq.c] : 999999;
    int vis_der = trans_d ? matriz_visitas[u_der.f][u_der.c] : 999999;

    // Elijo ruta menos visitada
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





ComportamientoTecnico::Estado ComportamientoTecnico::AplicaAccion_N2(const Estado& st, Action act) {
    Estado nuevo = st;
    if (act == TURN_SL) {
        nuevo.brujula = (Orientacion)((nuevo.brujula + 7) % 8);
    } else if (act == TURN_SR) {
        nuevo.brujula = (Orientacion)((nuevo.brujula + 1) % 8);
    } else if (act == JUMP) {
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

bool ComportamientoTecnico::EsValida_N2(const Estado& st, Action act, bool aguaPermitida) {
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
        if (c == 'M' || c == 'P' || c == 'B') return false; 
        if (c == 'A' && !aguaPermitida) return false;

        unsigned char entidad = mapaEntidades[destino.f][destino.c];
        if (entidad != '_' && entidad != '?') return false;

        int dif = mapaCotas[destino.f][destino.c] - mapaCotas[st.f][st.c];

        if (act == WALK && abs(dif) > 1) return false;
        if (act == JUMP && dif != 2) return false;

        return true;
    }
    return false;
}

bool ComportamientoTecnico::EncontrarPlan_N2(const Estado& inicio, int dest_f, int dest_c, std::list<Action>& plan_resultante, bool aguaPermitida) {
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

        Action acciones[] = {WALK, TURN_SL, TURN_SR};
        
        for (Action accion : acciones) {
            if (EsValida_N2(actual.st, accion, aguaPermitida)) {
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
    if (sensores.superficie[0] == 'D') {
        tiene_zapatillas = true;
    }

    if (sensores.tiempo == 0) {
        desalojo_pendiente_n2 = false;
        ya_reubicado_n2 = false;
    }

    auto casilla_transitable = [&](int idx) {
        unsigned char celda = sensores.superficie[idx];
        int dif = sensores.cota[idx] - sensores.cota[0];
        if (abs(dif) > 1) return false;
        if (sensores.agentes[idx] != '_') return false;
        if (celda == 'M' || celda == 'P' || celda == '?') return false;
        if (celda == 'B' && !tiene_zapatillas) return false;
        return true;
    };

    bool ing_frente = (sensores.agentes[2] == 'i' || sensores.agentes[2] == 'I');
    bool ing_izq = (sensores.agentes[1] == 'i' || sensores.agentes[1] == 'I');
    bool ing_der = (sensores.agentes[3] == 'i' || sensores.agentes[3] == 'I');

    if (ing_izq || ing_der || ing_frente) {
        desalojo_pendiente_n2 = false;
        ya_reubicado_n2 = false;
        if (!ing_frente && casilla_transitable(2)) return WALK;
        if (ing_izq && casilla_transitable(3)) return TURN_SR;
        if (ing_der && casilla_transitable(1)) return TURN_SL;
        if (casilla_transitable(2)) return WALK;
        if (casilla_transitable(3)) return TURN_SR;
        if (casilla_transitable(1)) return TURN_SL;
        return TURN_SR;
    }

    // Si el ingeniero viene por detrás, el técnico nunca lo verá.
    // Hacemos un único desalojo local para liberar posibles pasillos
    // estrechos sin convertir el nivel 2 en un comportamiento errático.
    if (desalojo_pendiente_n2) {
        desalojo_pendiente_n2 = false;
        ya_reubicado_n2 = true;
        if (casilla_transitable(2)) return WALK;
    }

    if (!ya_reubicado_n2) {
        if (casilla_transitable(3)) {
            desalojo_pendiente_n2 = true;
            return TURN_SR;
        }
        if (casilla_transitable(1)) {
            desalojo_pendiente_n2 = true;
            return TURN_SL;
        }
    }

    return IDLE;
}



// =========================================================
// === MOTOR DE BÚSQUEDA A* NIVEL 3 (TÉCNICO) ===
// =========================================================

int ComportamientoTecnico::CostoBateria_N3(const EstadoN3& st, Action act) {
    unsigned char terreno = mapaResultado[st.f][st.c];
    int coste = 0;

    if (act == WALK) {
        int cota_origen = mapaCotas[st.f][st.c];
        EstadoN3 destino = AplicaAccion_N3(st, act); 
        int cota_destino = mapaCotas[destino.f][destino.c];
        int dif = cota_destino - cota_origen;

        if (terreno == 'A') { coste = 60; if(dif > 0) coste += 5; else if(dif < 0) coste -= 2; }
        else if (terreno == 'H') { coste = 6; if(dif > 0) coste += 5; else if(dif < 0) coste -= 2; }
        else if (terreno == 'S') { coste = 3; if(dif > 0) coste += 5; else if(dif < 0) coste -= 2; }
        else { coste = 1; }
        
        if (coste < 1) coste = 1; 

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
    // Distancia Chebyshev multiplicada por el coste mínimo de movimiento (1)
    return max(abs(actual.f - dest_f), abs(actual.c - dest_c));
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

bool ComportamientoTecnico::EsValida_N3(const EstadoN3& st, Action act, bool ignorarentidades, bool agua_permitida) {
    if (act == TURN_SL || act == TURN_SR) return true;
    if (act == WALK) {
        EstadoN3 destino = AplicaAccion_N3(st, act);
        if (destino.f < 0 || destino.f >= (int)mapaResultado.size() ||
            destino.c < 0 || destino.c >= (int)mapaResultado[0].size())
            return false;

        unsigned char c = mapaResultado[destino.f][destino.c];
        
        if (c == 'M' || c == 'P') return false;
        if (c == '?' && !ignorarentidades) return false;
        if (c == 'A' && !agua_permitida) return false;
        if (c == 'B' && !st.zapatillas) return false;

        if (!ignorarentidades) {
            unsigned char entidad = mapaEntidades[destino.f][destino.c];
            if (entidad != '_' && entidad != '?') return false;
        }

        int dif = mapaCotas[destino.f][destino.c] - mapaCotas[st.f][st.c];
        if (c != '?' && abs(dif) > 1) return false;

        return true;
    }
    return false;
}

bool ComportamientoTecnico::EncontrarPlan_N3(const EstadoN3& inicio, int dest_f, int dest_c, std::list<Action>& plan_resultante, bool ignorar_entidades, bool parar_adyacente, bool agua_permitida) {
    plan_resultante.clear();
    std::priority_queue<NodoN3, std::vector<NodoN3>, std::greater<NodoN3>> abiertos;
    std::set<EstadoN3> cerrados;
    std::map<EstadoN3, int> mejor_g;
    struct PredecesorN3 {
        EstadoN3 anterior;
        Action accion = IDLE;
    };
    std::map<EstadoN3, PredecesorN3> padres;

    auto reconstruir_plan = [&](const EstadoN3& objetivo) {
        std::list<Action> reconstruido;
        EstadoN3 cursor = objetivo;

        while (!(cursor == inicio)) {
            auto it = padres.find(cursor);
            if (it == padres.end()) break;
            reconstruido.push_front(it->second.accion);
            cursor = it->second.anterior;
        }

        plan_resultante = std::move(reconstruido);
    };

    NodoN3 n_inicial;
    n_inicial.st = inicio;
    n_inicial.coste_g = 0;
    n_inicial.coste_h = Heuristica(inicio, dest_f, dest_c);
    abiertos.push(n_inicial);
    mejor_g[inicio] = 0;

    while (!abiertos.empty()) {
        NodoN3 actual = abiertos.top();
        abiertos.pop();

        auto it_mejor = mejor_g.find(actual.st);
        if (it_mejor != mejor_g.end() && actual.coste_g > it_mejor->second) continue;
        if (cerrados.find(actual.st) != cerrados.end()) continue;
        cerrados.insert(actual.st);

        if (parar_adyacente) {
            if (abs(actual.st.f - dest_f) + abs(actual.st.c - dest_c) == 1) {
                reconstruir_plan(actual.st);
                return true;
            }
        } else {
            if (actual.st.f == dest_f && actual.st.c == dest_c) {
                reconstruir_plan(actual.st);
                return true;
            }
        }

        Action acciones[] = {WALK, TURN_SL, TURN_SR};
        for (Action accion : acciones) {
            if (EsValida_N3(actual.st, accion, ignorar_entidades, agua_permitida)) {
                EstadoN3 siguiente = AplicaAccion_N3(actual.st, accion);
                int coste_accion = CostoBateria_N3(actual.st, accion);
                int nuevo_g = actual.coste_g + coste_accion;
                auto it_hijo = mejor_g.find(siguiente);
                if (cerrados.find(siguiente) == cerrados.end() &&
                    (it_hijo == mejor_g.end() || nuevo_g < it_hijo->second)) {
                    mejor_g[siguiente] = nuevo_g;
                    padres[siguiente] = {actual.st, accion};
                    NodoN3 hijo;
                    hijo.st = siguiente;
                    hijo.coste_g = nuevo_g;
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
    // Replanificar si el plan anterior causó error
    if (hay_plan && !plan.empty()) {
        if (sensores.posF == ultimaPosFN3 && sensores.posC == ultimaPosCN3 && ultimaAccionN3 == WALK) {
            hay_plan = false; plan.clear();
        }
    }
    ultimaPosFN3 = sensores.posF;
    ultimaPosCN3 = sensores.posC;

    // ¡LA CLAVE ANTI-ROBOS! Solo cogemos las zapatillas si no estamos en el instante 0 
    // (para evitar creer que las tenemos si el Ingeniero nos las quitó al aparecer).
    if (sensores.superficie[0] == 'D' && sensores.tiempo > 0) {
        tiene_zapatillas = true;
    }

    if (!hay_plan) {
        EstadoN3 estadoinicial;
        estadoinicial.f         = sensores.posF;
        estadoinicial.c         = sensores.posC;
        estadoinicial.brujula   = sensores.rumbo;
        estadoinicial.zapatillas = tiene_zapatillas; // ¡OJO! Usamos la variable segura, no miramos el mapa.

        hay_plan = EncontrarPlan_N3(estadoinicial, sensores.BelPosF, sensores.BelPosC, plan, true);
        if (!hay_plan) {
            // Intentar permitiendo agua
            hay_plan = EncontrarPlan_N3(estadoinicial, sensores.BelPosF, sensores.BelPosC, plan, true, false, true);
        }
        if (!hay_plan) return IDLE;
    }

    if (!plan.empty()) {
        Action a = plan.front();
        
        // ¡ANTICHOQUE UNIVERSAL! Si hay un agente delante, esperamos pacientemente.
        if (a == WALK && (sensores.agentes[2] == 'i' || sensores.agentes[2] == 'I')) {
            return IDLE;
        }

        ultimaAccionN3 = a;
        plan.pop_front();
        if (plan.empty()) hay_plan = false;
        return ultimaAccionN3;
    }

    hay_plan = false;
    return IDLE;
}

/**
 * @brief Comportamiento del técnico para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_4(Sensores sensores) {
    // Inicialización al primer instante
    if (sensores.tiempo == 0) {
        plan_n5.clear();
        tramo_n5 = 0;
        hay_plan = false;
        plan.clear();
    }

    // Calcular el plan de tubería si no lo tenemos aún
    if (plan_n5.empty()) {
        std::list<Paso> listaplan;
        // Faltaba añadir el 4º argumento aquí:
        EncontrarPlan_N5_Arquitecto(sensores.BelPosF, sensores.BelPosC, listaplan, sensores.max_ecologico);
        
        for (auto& p : listaplan)
            plan_n5.push_back(p);
        if (plan_n5.empty()) return IDLE;
    }

    // Fin de obra
    if (tramo_n5 >= (int)plan_n5.size()) return IDLE;

    Paso objetivo = plan_n5[tramo_n5];

    // --- FASE 1: Desplazarse a la casilla del tramo ---
    if (sensores.posF != objetivo.fil || sensores.posC != objetivo.col) {
        if (!hay_plan) {
            EstadoN3 inicio;
            inicio.f        = sensores.posF;
            inicio.c        = sensores.posC;
            inicio.brujula  = sensores.rumbo;
            inicio.zapatillas = (mapaResultado[sensores.posF][sensores.posC] == 'D');
            if (!EncontrarPlan_N3(inicio, objetivo.fil, objetivo.col, plan, false))
                EncontrarPlan_N3(inicio, objetivo.fil, objetivo.col, plan, true);
            hay_plan = true;
        }
        if (!plan.empty()) {
            Action a = plan.front();
            if (a == WALK &&
                sensores.agentes[2] != '_' && sensores.agentes[2] != '?')
                return IDLE; // ceder paso
            plan.pop_front();
            return a;
        } else {
            hay_plan = false;
            return IDLE;
        }
    }

    // --- FASE 2: Alinearse mirando al siguiente tramo ---
    if (tramo_n5 + 1 < (int)plan_n5.size()) {
        Paso siguiente = plan_n5[tramo_n5 + 1];
        // Calcular orientación deseada manualmente (sin static helper)
        Orientacion oriDeseada = norte;
        int df = siguiente.fil - sensores.posF;
        int dc = siguiente.col - sensores.posC;
        if      (df < 0 && dc == 0) oriDeseada = norte;
        else if (df > 0 && dc == 0) oriDeseada = sur;
        else if (df == 0 && dc > 0) oriDeseada = este;
        else if (df == 0 && dc < 0) oriDeseada = oeste;
        else if (df < 0 && dc > 0)  oriDeseada = noreste;
        else if (df < 0 && dc < 0)  oriDeseada = noroeste;
        else if (df > 0 && dc > 0)  oriDeseada = sureste;
        else if (df > 0 && dc < 0)  oriDeseada = suroeste;

        if (sensores.rumbo != oriDeseada) {
            int giros = ((int)oriDeseada - (int)sensores.rumbo + 8) % 8;
            return (giros <= 4) ? TURN_SR : TURN_SL;
        }
    }

    // --- FASE 3: Instalar y avanzar ---
    hay_plan = false;
    plan.clear();
    tramo_n5++;
    return INSTALL;
}



// =========================================================
// === MÁQUINA DE ESTADOS COOPERATIVA (NIVEL 5) ===
// =========================================================

// Clon exacto del algoritmo del Ingeniero para que ambos deduzcan el mismo plan
bool ComportamientoTecnico::EncontrarPlan_N5_Arquitecto(int start_f, int start_c, std::list<Paso>& plan_resultante, int limite_eco) {
    plan_resultante.clear();
    std::priority_queue<NodoN4_Tecnico, std::vector<NodoN4_Tecnico>, std::greater<NodoN4_Tecnico>> abiertos;
    std::map<EstadoN4_Tecnico, int> cerrados;

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
        EstadoN4_Tecnico st = {start_f, start_c, h};
        NodoN4_Tecnico nodo;
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
        NodoN4_Tecnico actual = abiertos.top();
        abiertos.pop();

        if (cerrados[actual.st] < actual.impacto) continue;

        if (mapaResultado[actual.st.f][actual.st.c] == 'U') {
            plan_resultante = actual.secuencia;
            return true;
        }

        for (int i = 0; i < 4; i++) {
            int nf = actual.st.f + df[i];
            int nc = actual.st.c + dc[i];

            if (nf < 0 || nf >= (int)mapaResultado.size() || nc < 0 || nc >= (int)mapaResultado[0].size()) continue;

            unsigned char n_terr = mapaResultado[nf][nc];
            if (n_terr == 'M' || n_terr == 'P' || n_terr == '?') continue; 
            if (n_terr == 'B') continue;

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
                    EstadoN4_Tecnico siguiente = {nf, nc, nh};
                    int op = nh - nH;
                    
                    unsigned char actual_terr = mapaResultado[actual.st.f][actual.st.c];
                    int impacto_tramo = imp_install(actual_terr) + imp_install(n_terr) + imp_op(n_terr, op);
                    int nuevo_impacto = actual.impacto + impacto_tramo;

                    if (nuevo_impacto <= limite_eco) {
                        if (cerrados.find(siguiente) == cerrados.end() || cerrados[siguiente] > nuevo_impacto) {
                            cerrados[siguiente] = nuevo_impacto;
                            NodoN4_Tecnico hijo = actual;
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

bool ComportamientoTecnico::EncontrarPlan_N5_Tecnico(int start_f, int start_c, std::list<Paso>& plan_resultante, int limite_eco) {
    plan_resultante.clear();
    std::priority_queue<NodoN4_Tecnico, std::vector<NodoN4_Tecnico>, std::greater<NodoN4_Tecnico>> abiertos;
    std::map<EstadoN4_Tecnico, int> cerrados;

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
        NodoN4_Tecnico actual = abiertos.top();
        abiertos.pop();

        if (cerrados[actual.st] < actual.impacto) continue;

        if (mapaResultado[actual.st.f][actual.st.c] == 'U') {
            plan_resultante = actual.secuencia;
            return true;
        }

        for (int i = 0; i < 4; i++) {
            int nf = actual.st.f + df[i];
            int nc = actual.st.c + dc[i];

            if (nf < 0 || nf >= (int)mapaResultado.size() || nc < 0 || nc >= (int)mapaResultado[0].size()) continue;

            unsigned char n_terr = mapaResultado[nf][nc];
            if (n_terr == 'M' || n_terr == 'P' || n_terr == '?') continue; 
            if (n_terr == 'B') continue;

            int nH = mapaCotas[nf][nc];
            std::vector<int> alturas_vecino;
            
            if (n_terr == 'A') alturas_vecino.push_back(nH); 
            else {
                alturas_vecino.push_back(nH);
                if (nH > 0) alturas_vecino.push_back(nH - 1);
                if (nH < 9) alturas_vecino.push_back(nH + 1);
            }

            for (int nh : alturas_vecino) {
                // GRAVEDAD ESTRICTA
                if (actual.st.h >= nh && (actual.st.h - nh) <= 1) {
                    EstadoN4_Tecnico siguiente = {nf, nc, nh};
                    int op = nh - nH;
                    
                    // IMPACTO SIMPLE
                    unsigned char actual_terr = mapaResultado[actual.st.f][actual.st.c];
                    int impacto_tramo = imp_install(actual_terr) + imp_install(n_terr) + imp_op(n_terr, op);
                    int nuevo_impacto = actual.impacto + impacto_tramo;

                    if (nuevo_impacto <= limite_eco) {
                        if (cerrados.find(siguiente) == cerrados.end() || cerrados[siguiente] > nuevo_impacto) {
                            cerrados[siguiente] = nuevo_impacto;
                            NodoN4_Tecnico hijo = actual;
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

static bool HayTuberiaEntreTecnico(const vector<vector<unsigned char>> &mapaTuberias,
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


bool ComportamientoTecnico::EsValida_N5(const EstadoN3& st, Action act, bool ignorarentidades) {
    if (act == TURN_SL || act == TURN_SR) return true;
    if (act == WALK) {
        EstadoN3 destino = AplicaAccion_N3(st, act);
        if (destino.f < 0 || destino.f >= (int)mapaResultado.size() ||
            destino.c < 0 || destino.c >= (int)mapaResultado[0].size())
            return false;

        unsigned char c = mapaResultado[destino.f][destino.c];
        
        // ¡LA DIFERENCIA! Aquí NO bloqueamos la 'A' (Agua), permitimos cruzar ríos.
        if (c == 'M' || c == 'P' || c == '?') return false;  
        if (c == 'B' && !st.zapatillas) return false;

        if (!ignorarentidades) {
            unsigned char entidad = mapaEntidades[destino.f][destino.c];
            if (entidad != '_' && entidad != '?') return false;
        }

        int dif = mapaCotas[destino.f][destino.c] - mapaCotas[st.f][st.c];
        if (abs(dif) > 1) return false;

        return true;
    }
    return false;
}

bool ComportamientoTecnico::EncontrarPlan_N5_Caminar(EstadoN3 inicio, int dest_f, int dest_c, std::list<Action>& plan_resultante, bool ignorar_entidades, bool parar_adyacente) {
    plan_resultante.clear();
    std::priority_queue<NodoN3, std::vector<NodoN3>, std::greater<NodoN3>> abiertos;
    std::set<EstadoN3> cerrados;
    std::map<EstadoN3, int> mejor_g;
    struct PredecesorN3 {
        EstadoN3 anterior;
        Action accion = IDLE;
    };
    std::map<EstadoN3, PredecesorN3> padres;

    auto reconstruir_plan = [&](const EstadoN3& objetivo) {
        std::list<Action> reconstruido;
        EstadoN3 cursor = objetivo;

        while (!(cursor == inicio)) {
            auto it = padres.find(cursor);
            if (it == padres.end()) break;
            reconstruido.push_front(it->second.accion);
            cursor = it->second.anterior;
        }

        plan_resultante = std::move(reconstruido);
    };

    NodoN3 n_inicial;
    n_inicial.st = inicio;
    n_inicial.coste_g = 0;
    n_inicial.coste_h = Heuristica(inicio, dest_f, dest_c);
    abiertos.push(n_inicial);
    mejor_g[inicio] = 0;

    while (!abiertos.empty()) {
        NodoN3 actual = abiertos.top();
        abiertos.pop();

        auto it_mejor = mejor_g.find(actual.st);
        if (it_mejor != mejor_g.end() && actual.coste_g > it_mejor->second) continue;
        if (cerrados.find(actual.st) != cerrados.end()) continue;
        cerrados.insert(actual.st);

        if (parar_adyacente) {
            if (abs(actual.st.f - dest_f) + abs(actual.st.c - dest_c) == 1) {
                reconstruir_plan(actual.st);
                return true;
            }
        } else {
            if (actual.st.f == dest_f && actual.st.c == dest_c) {
                reconstruir_plan(actual.st);
                return true;
            }
        }

        Action acciones[] = {WALK, TURN_SL, TURN_SR};
        for (Action accion : acciones) {
            // ¡LA CLAVE! Usamos el nuevo radar anfibio EsValida_N5
            if (EsValida_N5(actual.st, accion, ignorar_entidades)) {
                EstadoN3 siguiente = AplicaAccion_N3(actual.st, accion);
                int coste_accion = CostoBateria_N3(actual.st, accion);
                int nuevo_g = actual.coste_g + coste_accion;
                auto it_hijo = mejor_g.find(siguiente);
                if (cerrados.find(siguiente) == cerrados.end() &&
                    (it_hijo == mejor_g.end() || nuevo_g < it_hijo->second)) {
                    mejor_g[siguiente] = nuevo_g;
                    padres[siguiente] = {actual.st, accion};
                    NodoN3 hijo;
                    hijo.st = siguiente;
                    hijo.coste_g = nuevo_g;
                    hijo.coste_h = Heuristica(siguiente, dest_f, dest_c);
                    abiertos.push(hijo);
                }
            }
        }
    }
    return false;
}

/**
    * @brief Comportamiento del técnico para el Nivel 5.
    * @param sensores Datos actuales de los sensores.
    * @return Acción a realizar.
*/
Action ComportamientoTecnico::ComportamientoTecnicoNivel_5(Sensores sensores) {
    if (sensores.tiempo == 0) {
        estado_n6 = 0;
        hay_plan = false; plan.clear();
        plan_n5.clear(); tramo_n5 = 1;
    }

    if (plan_n5.empty()) {
        std::list<Paso> listaplan;
        EncontrarPlan_N5_Arquitecto(sensores.BelPosF, sensores.BelPosC, listaplan, sensores.max_ecologico);
        for (auto& p : listaplan) plan_n5.push_back(p);
        if (plan_n5.size() > 1) {
            destn6_f = plan_n5[0].fil;
            destn6_c = plan_n5[0].col;
            estado_n6 = 1;
        }
    }

    if (estado_n6 == 0 || tramo_n5 >= (int)plan_n5.size()) return IDLE;

    if (estado_n6 == 1) {
        destn6_f = plan_n5[tramo_n5 - 1].fil;
        destn6_c = plan_n5[tramo_n5 - 1].col;
        if (sensores.posF == destn6_f && sensores.posC == destn6_c) {
            estado_n6 = 2;
            hay_plan = false; plan.clear();
            //return IDLE;
        } else {
            // ATAJO: si el destino está a 1 casilla ortogonal, ir directo sin A*
            int df = abs(sensores.posF - destn6_f);
            int dc = abs(sensores.posC - destn6_c);
            if (df + dc == 1) {
                // Orientarse hacia el destino
                Orientacion ori = OrientacionHacia_Tec(sensores.posF, sensores.posC, destn6_f, destn6_c);
                if (sensores.rumbo != ori) {
                    int giros = GirosNecesarios_Tec(sensores.rumbo, ori);
                    return (giros <= 4) ? TURN_SR : TURN_SL;
                }
                // Caminar si no hay agente delante
                if (sensores.agentes[2] == '_') return WALK;
                return IDLE;
            }

            if (!hay_plan) {
                EstadoN3 inicio = {sensores.posF, sensores.posC, sensores.rumbo, tiene_zapatillas};
                unsigned char respaldo_entidad = '_';
                Paso tramo_ing = plan_n5[tramo_n5];
                respaldo_entidad = mapaEntidades[tramo_ing.fil][tramo_ing.col];
                mapaEntidades[tramo_ing.fil][tramo_ing.col] = 'i';
                EncontrarPlan_N5_Caminar(inicio, destn6_f, destn6_c, plan, false, false);
                mapaEntidades[tramo_ing.fil][tramo_ing.col] = respaldo_entidad;
                hay_plan = true;
            }
            if (!plan.empty()) {
                Action a = plan.front();
                if (a == WALK && sensores.agentes[2] != '_') {
                    hay_plan = false;
                    plan.clear();
                    return IDLE;
                }
                plan.pop_front(); return a;
            } else {
                hay_plan = false; return TURN_SR;
            }
        }
        
    }

    if (estado_n6 == 2) {
        Paso tramo_ing = plan_n5[tramo_n5];
        if (HayTuberiaEntreTecnico(mapaTuberias, sensores.posF, sensores.posC,
                                   tramo_ing.fil, tramo_ing.col)) {
            tramo_n5++;
            estado_n6 = 1;
            hay_plan = false;
            plan.clear();
            return IDLE;
        }

        Orientacion ori = OrientacionHacia_Tec(sensores.posF, sensores.posC, tramo_ing.fil, tramo_ing.col);
        if (sensores.rumbo != ori) {
            int giros = GirosNecesarios_Tec(sensores.rumbo, ori);
            return (giros <= 4) ? TURN_SR : TURN_SL;
        }

        if (sensores.enfrente) {
            return INSTALL;
        }
        return IDLE;
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
    bool dbg_oculto_30 = (mapaResultado.size() == 30 && sensores.max_ecologico != 648 &&
                          sensores.max_ecologico != 1000 && sensores.max_ecologico != 1804 &&
                          sensores.max_ecologico != 2364);
    bool dbg_n6 = (sensores.max_ecologico == 2364 || sensores.max_ecologico == 2688 || sensores.max_ecologico == 3533 ||
                   sensores.max_ecologico == 2107 ||
                   sensores.max_ecologico == 1719 || sensores.max_ecologico == 1500 || dbg_oculto_30);
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

    if (sensores.tiempo == 0) {
        estado_n6 = 0; destn6_f = -1; destn6_c = -1;
        retirada_n6 = -1;
        retirada_izq_n6 = (mapaResultado.size() >= 100 && sensores.energia > 8000);
        intento_orbita_n6 = 0;
        install_pendiente_n6 = false;
        eco_ref_install_n6 = -1;
        hay_plan = false; plan.clear(); 
    }

    if (sensores.venpaca) {
        bool destino_cambiado = (destn6_f != sensores.GotoF || destn6_c != sensores.GotoC);
        bool reactivar_misma_orden = (!destino_cambiado && estado_n6 == 0);
        if (destino_cambiado || reactivar_misma_orden) {
            if (dbg_n6) {
                cerr << "[TEC6 CALL] t=" << sensores.tiempo << " estado=" << estado_n6
                     << " dest=(" << sensores.GotoF << "," << sensores.GotoC << ") pos=("
                     << sensores.posF << "," << sensores.posC << ") energia=" << sensores.energia << "\n";
            }
            destn6_f = sensores.GotoF; destn6_c = sensores.GotoC;
            retirada_n6 = -1;
            intento_orbita_n6 = 0;
            install_pendiente_n6 = false;
            eco_ref_install_n6 = -1;
            plan.clear();
            hay_plan = false;
            if (abs(sensores.posF - destn6_f) + abs(sensores.posC - destn6_c) == 1 &&
                !HayTuberiaEntreTecnico(mapaTuberias, sensores.posF, sensores.posC, destn6_f, destn6_c)) {
                estado_n6 = 2;
            } else {
                estado_n6 = 1;
            }
        }
    }

    if (install_pendiente_n6) {
        ubicacion delante = Delante({sensores.posF, sensores.posC, sensores.rumbo});
        bool tuberia_delante = (delante.f >= 0 && delante.f < (int)mapaTuberias.size() &&
                                delante.c >= 0 && delante.c < (int)mapaTuberias[0].size() &&
                                HayTuberiaEntreTecnico(mapaTuberias, sensores.posF, sensores.posC,
                                                       delante.f, delante.c));
        if (sensores.ecologico > eco_ref_install_n6 || tuberia_delante) {
            install_pendiente_n6 = false;
            estado_n6 = 3;
            if (dbg_n6) {
                cerr << "[TEC6 INSTALL OK -> RET] t=" << sensores.tiempo << " pos=("
                     << sensores.posF << "," << sensores.posC << ") eco=" << sensores.ecologico
                     << " energia=" << sensores.energia << "\n";
            }
            retirada_n6 = -1;
            intento_orbita_n6 = 0;
            hay_plan = false;
            plan.clear();
        } else if (!(sensores.agentes[2] == 'i' || sensores.agentes[2] == 'I')) {
            install_pendiente_n6 = false;
        }
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
        if (real_c == 'B' && !tiene_zapatillas) return false;
        if (mapaResultado[nf][nc] != '?') {
            if (abs(mapaCotas[nf][nc] - mapaCotas[sens.posF][sens.posC]) > 1) return false; // El técnico SIEMPRE es 1
        }
        return true;
    };

    switch(estado_n6) {
        case 0:
            if (abs(sensores.posF - sensores.BelPosF) + abs(sensores.posC - sensores.BelPosC) == 1 &&
                !HayTuberiaEntreTecnico(mapaTuberias, sensores.posF, sensores.posC,
                                        sensores.BelPosF, sensores.BelPosC)) {
                Orientacion ori = OrientacionHacia_Tec(sensores.posF, sensores.posC,
                                                       sensores.BelPosF, sensores.BelPosC);
                if (sensores.rumbo != ori) {
                    return (GirosNecesarios_Tec(sensores.rumbo, ori) <= 4) ? TURN_SR : TURN_SL;
                }
                if (sensores.enfrente) {
                    return INSTALL;
                }
            }
            return IDLE; 

        case 1: // IR AL TUBO
            if (sensores.posF == destn6_f && sensores.posC == destn6_c) {
                if (dbg_n6) {
                    cerr << "[TEC6 ARRIVE] t=" << sensores.tiempo << " dest=(" << destn6_f << "," << destn6_c
                         << ") rumbo=" << sensores.rumbo << " energia=" << sensores.energia << "\n";
                }
                estado_n6 = 2; return IDLE;
            } else {
                if (!hay_plan) {
                    EstadoN3 inicio = {sensores.posF, sensores.posC, sensores.rumbo, tiene_zapatillas};
                    hay_plan = EncontrarPlan_N3(inicio, destn6_f, destn6_c, plan, true, false, false);
                    if (!hay_plan) {
                        hay_plan = EncontrarPlan_N3(inicio, destn6_f, destn6_c, plan, true, false, true);
                    }
                }
                if (!plan.empty()) {
                    Action a = plan.front();
                    if (a == WALK) {
                        if (sensores.agentes[2] != '_' || sensores.choque) {
                            hay_plan = false;
                            plan.clear();
                            return IDLE;
                        }
                        if (!es_seguro(sensores)) { hay_plan = false; plan.clear(); return TURN_SR; }
                    }
                    plan.pop_front(); return a;
                } else {
                    hay_plan = false; return TURN_SR;
                }
            }

        case 2: // MIRAR AL JEFE Y ESPERAR A QUE ÉL NOS MIRE
            if (sensores.agentes[2] == 'i' || sensores.agentes[2] == 'I') {
                intento_orbita_n6 = 0;
                ubicacion delante = Delante({sensores.posF, sensores.posC, sensores.rumbo});
                if (delante.f >= 0 && delante.f < (int)mapaTuberias.size() &&
                    delante.c >= 0 && delante.c < (int)mapaTuberias[0].size() &&
                    HayTuberiaEntreTecnico(mapaTuberias, sensores.posF, sensores.posC,
                                           delante.f, delante.c)) {
                    estado_n6 = 3;
                    retirada_n6 = -1;
                    intento_orbita_n6 = 0;
                    hay_plan = false;
                    plan.clear();
                    return IDLE;
                }
            }

            // 'i' minúscula o 'I' mayúscula por si acaso
            if (sensores.agentes[2] == 'i' || sensores.agentes[2] == 'I') {
                if (sensores.enfrente) {
                    install_pendiente_n6 = true;
                    eco_ref_install_n6 = sensores.ecologico;
                    if (dbg_n6) {
                        cerr << "[TEC6 INSTALL] t=" << sensores.tiempo << " pos=(" << sensores.posF << ","
                             << sensores.posC << ") energia=" << sensores.energia << "\n";
                    }
                    return INSTALL;
                } else {
                    return IDLE; // Le veo, solo le miro fijamente hasta que se gire
                }
            }
            intento_orbita_n6++;
            if (intento_orbita_n6 > 8) {
                estado_n6 = 1;
                intento_orbita_n6 = 0;
                hay_plan = false;
                plan.clear();
                return IDLE;
            }
            return TURN_SR; // Giro hasta encontrarlo

        case 3: // RETROCEDER UNA CASILLA PARA DEJAR LIBRE EL SIGUIENTE TRAMO
            {
            if (retirada_n6 < 0) retirada_n6 = (int)sensores.rumbo;
            Orientacion candidatos[3];
            if (retirada_izq_n6) {
                candidatos[0] = (Orientacion)((retirada_n6 + 6) % 8); // lateral izquierdo
                candidatos[1] = (Orientacion)((retirada_n6 + 2) % 8); // lateral derecho
            } else {
                candidatos[0] = (Orientacion)((retirada_n6 + 2) % 8); // lateral derecho
                candidatos[1] = (Orientacion)((retirada_n6 + 6) % 8); // lateral izquierdo
            }
            candidatos[2] = (Orientacion)((retirada_n6 + 4) % 8); // hacia atras, ultimo recurso

            if (intento_orbita_n6 >= 3) {
                estado_n6 = 0;
                intento_orbita_n6 = 0;
                retirada_n6 = -1;
                hay_plan = false;
                plan.clear();
                return IDLE;
            }

            Orientacion objetivo = candidatos[intento_orbita_n6];
            if (sensores.rumbo != objetivo) {
                return (GirosNecesarios_Tec(sensores.rumbo, objetivo) <= 4) ? TURN_SR : TURN_SL;
            }

            if (sensores.agentes[2] == '_' && !sensores.choque && es_seguro(sensores)) {
                estado_n6 = 0;
                intento_orbita_n6 = 0;
                retirada_n6 = -1;
                hay_plan = false;
                plan.clear();
                if (dbg_n6) {
                    cerr << "[TEC6 RET WALK] t=" << sensores.tiempo << " pos=(" << sensores.posF << ","
                         << sensores.posC << ") rumbo=" << sensores.rumbo << " energia=" << sensores.energia << "\n";
                }
                return WALK;
            }

            intento_orbita_n6++;
            if (dbg_n6) {
                cerr << "[TEC6 RET FAIL] t=" << sensores.tiempo << " intento=" << intento_orbita_n6
                     << " pos=(" << sensores.posF << "," << sensores.posC << ") rumbo=" << sensores.rumbo
                     << " ag2=" << sensores.agentes[2] << " choque=" << sensores.choque
                     << " energia=" << sensores.energia << "\n";
            }
            return IDLE;
            }
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
    return max(abs(actual.fila - meta.fila), abs(actual.columna - meta.columna));
}
