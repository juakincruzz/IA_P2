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


// Niveles avanzados (Uso de búsqueda)
/**
 * @brief Comportamiento del ingeniero para el Nivel 2 (búsqueda).
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_2(Sensores sensores) {
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


// =========================================================
// === MOTOR DE PLANIFICACIÓN DE TUBERÍAS (NIVEL 4) ===
// =========================================================

bool ComportamientoIngeniero::EncontrarPlan_N4(int start_f, int start_c, std::list<Paso>& plan_resultante) {
    plan_resultante.clear();
    std::queue<NodoN4> abiertos;
    std::set<EstadoN4> cerrados;

    unsigned char start_terr = mapaResultado[start_f][start_c];
    if (start_terr == 'M' || start_terr == 'P') return false; // Muros y precipicios prohibidos
    
    int start_H = mapaCotas[start_f][start_c];
    std::vector<int> alturas_inicio;
    
    // Generar alturas posibles para la casilla inicial
    if (start_terr == 'A') {
        alturas_inicio.push_back(start_H); // En el agua no se puede excavar ni elevar
    } else {
        alturas_inicio.push_back(start_H);
        if (start_H > 0) alturas_inicio.push_back(start_H - 1); // Opción: Excavar
        if (start_H < 9) alturas_inicio.push_back(start_H + 1); // Opción: Elevar
    }

    // Inicializar la búsqueda con las alturas posibles del inicio
    for (int h : alturas_inicio) {
        EstadoN4 st = {start_f, start_c, h};
        NodoN4 nodo;
        nodo.st = st;
        // La estructura Paso guarda la modificación: h - start_H (-1 = DIG, 0 = NADA, 1 = RAISE)
        Paso p = {start_f, start_c, h - start_H}; 
        nodo.secuencia.push_back(p);
        
        abiertos.push(nodo);
        cerrados.insert(st);
    }

    // Movimientos ortogonales permitidos para la tubería
    int df[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    while (!abiertos.empty()) {
        NodoN4 actual = abiertos.front();
        abiertos.pop();

        // 1. ¿Hemos llegado a la Planta de Tratamiento ('U')?
        if (mapaResultado[actual.st.f][actual.st.c] == 'U') {
            plan_resultante = actual.secuencia;
            return true;
        }

        // 2. Expandir la tubería a los vecinos ortogonales
        for (int i = 0; i < 4; i++) {
            int nf = actual.st.f + df[i];
            int nc = actual.st.c + dc[i];

            // Comprobar límites
            if (nf < 0 || nf >= mapaResultado.size() || nc < 0 || nc >= mapaResultado[0].size()) continue;

            unsigned char n_terr = mapaResultado[nf][nc];
            if (n_terr == 'M' || n_terr == 'P') continue; 

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
                // ¡LA LEY DE LA GRAVEDAD! El agua fluye si la altura actual es mayor o igual a la siguiente
                if (actual.st.h >= nh) {
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
    return false; // No hay plan posible
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_4(Sensores sensores) {
    if (!plan_tuberias_hecho) {
        cout << "[INGENIERO] Cerebro Arquitectónico activado." << endl;
        cout << "[INGENIERO] Planificando red desde (" << sensores.BelPosF << ", " << sensores.BelPosC << ") hasta la planta 'U'..." << endl;
        
        bool exito = EncontrarPlan_N4(sensores.BelPosF, sensores.BelPosC, plan_tuberias);

        if (exito) {
            cout << "[INGENIERO] ¡Plan Maestro de Canalización Completado! Tramos necesarios: " << plan_tuberias.size() << endl;
            
            // ¡ARREGLO! El código real de los profesores solo pide el plan, sin guion bajo y con 1 parámetro.
            VisualizaRedTuberias(plan_tuberias);
        } else {
            cout << "[INGENIERO] ¡ERROR CRÍTICO! Es físicamente imposible trazar la tubería por gravedad." << endl;
        }
        plan_tuberias_hecho = true;
    }

    // En el Nivel 4 no nos movemos, solo pensamos y enviamos el plano.
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

