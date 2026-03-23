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
  // 1. Condición de éxito: Si pisamos la Belkanita, nos quedamos quietos.
    if (sensores.superficie[0] == 'U') {
        return IDLE;
    }

    // 2. Recolección pasiva: Si pisamos zapatillas, las guardamos.
    if (sensores.superficie[0] == 'D') {
        tiene_zapatillas = true;
    }

    // 3. Análisis del entorno inmediato (Casilla 2 = justo enfrente)
    char enf = sensores.superficie[2];
    int dif = sensores.cota[2] - sensores.cota[0];

    // 4. Filtro de Supervivencia: Detectar cualquier cosa que nos pueda matar o bloquear
    bool peligro = false;
    
    // Obstáculos físicos
    if (enf == 'M' || enf == 'P') peligro = true; 
    if (abs(dif) > 1) peligro = true; 
    if (sensores.agentes[2] != '_') peligro = true; 
    
    // Trampas de entorno (Esenciales en el mapa 75)
    if (enf == 'A') peligro = true; // El agua fulmina la batería
    if (enf == 'B' && !tiene_zapatillas) peligro = true; // El bosque descalzo agota la energía

    // 5. Motor Reactivo Puro (Determinista)
    if (peligro || sensores.choque) {
        // Giramos SIEMPRE a la derecha. Esto implementa la regla de "seguir la pared"
        // y es matemáticamente imposible que se quede vibrando en bucle en una esquina.
        return TURN_SR;
    } else {
        // Si el camino está despejado y es seguro, avanzamos.
        return WALK;
    }
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
    if (sensores.superficie[0] == 'U') return IDLE;
    
    // Coger zapatillas si las pisamos
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;
    
    // ¡SUPERVIVENCIA! Si estamos sobre una recarga y con menos de la mitad de batería, recargamos
    if (sensores.superficie[0] == 'X' && sensores.energia < 1500) return IDLE;

    char enf = sensores.superficie[2];
    int dif = sensores.cota[2] - sensores.cota[0];
    
    bool obstaculo = (enf == 'M' || enf == 'P' || abs(dif) > 1 || sensores.agentes[2] != '_');
    
    // En Nivel 1 evitamos morir tontamente por el entorno:
    if (enf == 'A') obstaculo = true; // Evitamos el agua
    if (enf == 'B' && !tiene_zapatillas) obstaculo = true; // Evitamos el bosque si vamos descalzos

    if (sensores.choque || obstaculo) {
        // Giro asimétrico para salir de laberintos naturales
        return (rand() % 100 < 70) ? TURN_SR : TURN_SL;
    }
    
    return WALK;
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
  ActualizarMapa(sensores);
    if (sensores.tiempo == 0) ruta_actual.clear();
    
    // Si llegamos a la Belkanita, paramos
    if (sensores.posF == sensores.BelPosF && sensores.posC == sensores.BelPosC) return IDLE;
    
    if (sensores.choque) ruta_actual.clear();

    if (ruta_actual.empty()) {
        estado origen = {sensores.posF, sensores.posC, sensores.rumbo};
        estado destino = {sensores.BelPosF, sensores.BelPosC, 0};
        // Usamos tu búsqueda optimista
        ruta_actual = BusquedaEnAnchuraN6(origen, destino);
        if (ruta_actual.empty()) return (rand() % 2 == 0) ? TURN_SL : TURN_SR;
    }

    if (!ruta_actual.empty()) {
        Action a = ruta_actual.front();
        ruta_actual.pop_front();
        // Antichoques
        if (a == WALK && (sensores.superficie[2] == 'M' || sensores.superficie[2] == 'P')) {
            ruta_actual.clear();
            return IDLE;
        }
        return a;
    }
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
  // Detectar zapatillas
  if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

  // ---------------------------------------------------------------------
    // PARCHE DE SEGURIDAD: Forzamos la inicialización en el instante 0
    // ---------------------------------------------------------------------
    if (sensores.tiempo == 0) {
        hayPlanTuberias = false;
        estado_obra_ing = ING_PLANIFICAR;
        ruta_actual.clear(); 
    }

    // ---------------------------------------------------------------------
    // MÁQUINA DE ESTADOS DEL INGENIERO (JEFE DE OBRA)
    // ---------------------------------------------------------------------
    switch(estado_obra_ing) {
        
        case ING_PLANIFICAR:
            // 1. Calculamos la red de tuberías entera la primera vez
            if (!hayPlanTuberias) {
                planTuberias = PlanificaTuberias(sensores.BelPosF, sensores.BelPosC);
                hayPlanTuberias = true;
                
                if (!planTuberias.empty()) {
                    VisualizaRedTuberias(planTuberias); // Dibujamos el plan
                    
                    cout << "¡Plan calculado ! Tuberías a instalar: " << planTuberias.size() << endl;
                } else {
                    cout << "¡ERROR! PlanificaTuberias ha devuelto una lista vacía." << endl;
                }
            }
            
            // 2. Si ya no quedan tubos, hemos terminado el nivel
            if (planTuberias.empty()) return IDLE; 

            // 3. Sacamos el siguiente tubo a instalar de la lista
            paso_actual = planTuberias.front();
            planTuberias.pop_front();
            estado_obra_ing = ING_IR_CASILLA;
            
            cout << "Jefe de obra: Vamos a poner tubo en " << paso_actual.fil << "," << paso_actual.col << endl;
            return IDLE;

        case ING_IR_CASILLA:
            // 1. ¿Ya estamos en la casilla donde va el tubo?
            if (sensores.posF == paso_actual.fil && sensores.posC == paso_actual.col) {
                estado_obra_ing = ING_TERRAFORMAR;
                return IDLE;
            }
            
            // 2. Si no tenemos ruta hacia esa casilla, la calculamos con nuestro BFS
            if (ruta_actual.empty()) {
                estado origen; 
                origen.fila = sensores.posF; 
                origen.columna = sensores.posC; 
                origen.orientacion = sensores.rumbo;
                
                estado destino; 
                destino.fila = paso_actual.fil; 
                destino.columna = paso_actual.col;
                
                ruta_actual = BusquedaEnAnchura(origen, destino);
                
                if (ruta_actual.empty()) {
                    cout << "¡ATASCO! El Ingeniero no encuentra ruta a la casilla " << destino.fila << "," << destino.columna << endl;
                }
            }
            
            // 3. Si tenemos ruta, damos el siguiente paso
            if (!ruta_actual.empty()) {
                Action a = ruta_actual.front();
                ruta_actual.pop_front();
                return a;
            }
            return IDLE; // Por si nos atascamos momentáneamente

        case ING_TERRAFORMAR:
            // Aplicamos la modificación si el terreno lo requiere
            if (paso_actual.op == 1) {
                paso_actual.op = 0; // Lo ponemos a 0 para no repetir la acción en el siguiente tick
                return RAISE;
            } else if (paso_actual.op == -1) {
                paso_actual.op = 0; 
                return DIG;
            }
            // Si no hay que hacer nada (op == 0), pasamos a avisar al Técnico
            estado_obra_ing = ING_AVISAR_TECNICO;
            return IDLE;

        case ING_AVISAR_TECNICO:
            estado_obra_ing = ING_ESPERAR_TECNICO;
            return COME; // ¡Llamamos al Técnico! Sus sensores venpaca, GotoF y GotoC se encenderán

        case ING_ESPERAR_TECNICO:
            // ¡DOBLE COMPROBACIÓN! Vemos al técnico ('t') Y él nos está mirando ('enfrente')
            if (sensores.agentes[2] == 't' && sensores.enfrente) {
                estado_obra_ing = ING_INSTALAR;
                return INSTALL; 
            }
            return TURN_SR; // Giramos como un radar

        case ING_INSTALAR:
            // El tubo ya está puesto. Volvemos al estado inicial para sacar el siguiente tubo
            estado_obra_ing = ING_PLANIFICAR;
            return COME;
    } // Fin del switch

    // ---------------------------------------------------------------------
    // RETORNO FINAL DE SEGURIDAD
    // (Soluciona el warning del compilador "control reaches end of non-void function")
    // ---------------------------------------------------------------------
    return IDLE;
}
/**
 * @brief Comportamiento del ingeniero para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_6(Sensores sensores)
{
    ActualizarMapa(sensores);

    // ========================================================
    // PANEL DE DEBUG TEMPORAL - INGENIERO
    // ========================================================
    if ((int)sensores.tiempo % 50 == 0) {
        cout << "[DEBUG ING] Tick: " << sensores.tiempo 
             << " | Pos: (" << sensores.posF << "," << sensores.posC << ")"
             << " | Bateria: " << sensores.energia 
             << " | EstadoMente: " << estado_obra_ing_6 
             << " | Pasos Ruta: " << ruta_actual.size() << endl;
    }
    // ========================================================
    
    if (sensores.superficie[0] == 'D') tiene_zapatillas = true;

    // Inicialización en el primer instante
    if (sensores.tiempo == 0) {
        hayPlanTuberias = false;
        fase_construccion = false;
        estado_obra_ing_6 = ING6_EXPLORAR;
        ruta_actual.clear();
        intentos_exploracion = 0;
    }

    switch(estado_obra_ing_6) {

        case ING6_EXPLORAR:
        {
            // --- 1. INTENTAR PLANIFICAR TUBERIAS ---
            if (sensores.BelPosF != -1 && sensores.BelPosC != -1) {
                bool hay_U = false;
                for (int r = 0; r < (int)mapaResultado.size() && !hay_U; r++)
                    for (int c2 = 0; c2 < (int)mapaResultado[0].size() && !hay_U; c2++)
                        if (mapaResultado[r][c2] == 'U') hay_U = true;
                if (hay_U) {
                    list<Paso> plan_candidato = PlanificaTuberias(sensores.BelPosF, sensores.BelPosC);
                    if (!plan_candidato.empty()) {
                        // Comprobamos si podemos caminar hasta el inicio
                        estado origen = {sensores.posF, sensores.posC, sensores.rumbo};
                        estado destino = {sensores.BelPosF, sensores.BelPosC, 0};
                        list<Action> ruta_al_inicio = BusquedaEnAnchuraN6(origen, destino);
                        
                        if (!ruta_al_inicio.empty() || (sensores.posF == sensores.BelPosF && sensores.posC == sensores.BelPosC)) {
                            planTuberias = plan_candidato;
                            hayPlanTuberias = true;
                            fase_construccion = true;
                            VisualizaRedTuberias(planTuberias);
                            
                            // ¡CORRECCIÓN VITAL! Vamos a PLANIFICAR para sacar el primer tubo de la lista
                            ruta_actual.clear();
                            estado_obra_ing_6 = ING6_PLANIFICAR; 
                            
                            cout << "Jefe de obra: ¡PLAN MAESTRO ENCONTRADO! Tubos a instalar: " << planTuberias.size() << endl;
                            return IDLE;
                        }
                    }
                }
            }

            // --- 2. PROTECCION DE ENERGIA ---
            if (sensores.energia < 100) return IDLE;

            // --- 3. EXPLORACION INTELIGENTE ---
            if (ruta_actual.empty()) {
                int dest_f = -1, dest_c = -1;
                int min_dist = 999999;
                
                for (int r = 0; r < (int)mapaResultado.size(); r += 2) {
                    for (int c = 0; c < (int)mapaResultado[0].size(); c += 2) {
                        if (mapaResultado[r][c] == '?') {
                            int dist = abs(sensores.posF - r) + abs(sensores.posC - c);
                            if (dist < min_dist) {
                                min_dist = dist; dest_f = r; dest_c = c;
                            }
                        }
                    }
                }

                if (dest_f != -1) {
                    estado origen = {sensores.posF, sensores.posC, sensores.rumbo};
                    estado destino = {dest_f, dest_c, 0};
                    ruta_actual = BusquedaEnAnchuraN6(origen, destino);
                    
                    if (ruta_actual.empty()) {
                        mapaResultado[dest_f][dest_c] = 'M'; // Poda
                        return (rand() % 2 == 0) ? TURN_SL : TURN_SR;
                    }
                } else {
                    return (rand() % 2 == 0) ? TURN_SL : TURN_SR;
                }
            }

            if (!ruta_actual.empty()) {
                Action a = ruta_actual.front();
                ruta_actual.pop_front();
                
                if (a == WALK) {
                    char enf = sensores.superficie[2];
                    if (enf == 'M' || enf == 'P') {
                        ruta_actual.clear();
                        return (rand() % 2 == 0) ? TURN_SL : TURN_SR; // Evita bucles al chocar
                    }
                    int dif_h = sensores.cota[2] - sensores.cota[0];
                    int max_d = tiene_zapatillas ? 2 : 1;
                    if (abs(dif_h) > max_d) {
                        ruta_actual.clear();
                        return (rand() % 2 == 0) ? TURN_SL : TURN_SR; // Evita bucles
                    }
                }
                return a;
            }
            return IDLE;
        }

        case ING6_PLANIFICAR:
        {
            if (planTuberias.empty()) {
                return IDLE;
            }

            // AHORA SÍ: Inicializamos paso_actual antes de ir a caminar
            paso_actual = planTuberias.front();
            planTuberias.pop_front();
            estado_obra_ing_6 = ING6_IR_CASILLA;
            ruta_actual.clear();
            return IDLE;
        }

        case ING6_IR_CASILLA:
        {
            ActualizarMapa(sensores); 
            
            // Si ya estamos sobre la casilla del tubo, a terraformar
            if (sensores.posF == paso_actual.fil && sensores.posC == paso_actual.col) {
                estado_obra_ing_6 = ING6_TERRAFORMAR;
                return IDLE;
            }
            
            if (ruta_actual.empty()) {
                estado origen = {sensores.posF, sensores.posC, sensores.rumbo};
                estado destino = {paso_actual.fil, paso_actual.col, 0};
                
                ruta_actual = BusquedaEnAnchura(origen, destino);
                if (ruta_actual.empty()) ruta_actual = BusquedaEnAnchuraN6(origen, destino);
                
                if (ruta_actual.empty()) {
                    // ¡PROHIBIDO BORRAR EL PLAN MAESTRO! 
                    // Damos un pequeño giro para actualizar los sensores y lo intentamos en el siguiente tick.
                    return (rand() % 2 == 0) ? TURN_SL : TURN_SR;
                }
            }
            
            if (!ruta_actual.empty()) {
                Action a = ruta_actual.front();
                
                if (a == WALK) {
                    char enf = sensores.superficie[2];
                    if (enf == 'M' || enf == 'P') {
                        ruta_actual.clear();
                        return (rand() % 2 == 0) ? TURN_SL : TURN_SR; 
                    }
                    int dif_h = sensores.cota[2] - sensores.cota[0];
                    int max_d = tiene_zapatillas ? 2 : 1;
                    if (abs(dif_h) > max_d) {
                        ruta_actual.clear();
                        return (rand() % 2 == 0) ? TURN_SL : TURN_SR; 
                    }
                    // ¡NUEVO!: Radar anticolesiones con el Técnico
                    if (sensores.agentes[2] != '_') {
                        ruta_actual.clear();
                        return IDLE; // Nos quedamos quietos esperando a que se aparte
                    }
                }
                
                ruta_actual.pop_front();
                return a;
            }
            return IDLE;
        }

        case ING6_TERRAFORMAR:
            if (paso_actual.op == 1) {
                paso_actual.op = 0;
                return RAISE;
            } else if (paso_actual.op == -1) {
                paso_actual.op = 0;
                return DIG;
            }
            estado_obra_ing_6 = ING6_AVISAR_TECNICO;
            return IDLE;

        case ING6_AVISAR_TECNICO:
            estado_obra_ing_6 = ING6_ESPERAR_TECNICO;
            return COME;

        case ING6_ESPERAR_TECNICO:
            // ¡RADAR ACTIVADO! Giramos continuamente para no darle la espalda al Operario.
            // Sobra muchísima batería, así que el coste de girar nos da igual.
            if (sensores.agentes[2] == 't' && sensores.enfrente) {
                estado_obra_ing_6 = ING6_INSTALAR;
                return INSTALL;
            }
            return TURN_SR;

        case ING6_INSTALAR:
            estado_obra_ing_6 = ING6_PLANIFICAR;
            return COME;
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
        // 1. ¿Llegamos a la meta?
        if (actual.st == destino) { 
          return actual.secuencia; 
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
        if (nf >= 0 && nf < (int)mapaResultado.size() && nc >= 0 && nc < (int)mapaResultado[0].size()) {
            unsigned char celda = mapaResultado[nf][nc];
            
            // Filtro de viabilidad: No muros, no precipicios
            // Bosque solo transitable con zapatillas (para el Ingeniero)
            // Agua SÍ es transitable para el Ingeniero (cuesta mucha energía pero es legal)
            bool transitable_terreno = (celda != 'P' && celda != 'M' && celda != '?');
            if (celda == 'B' && !tiene_zapatillas) transitable_terreno = false;
            
            if (transitable_terreno) {                
                // Filtro de altura: Desnivel máximo 1 sin zapatillas, 2 con zapatillas
                int dif_cota = mapaCotas[nf][nc] - mapaCotas[actual.st.fila][actual.st.columna];
                int max_desnivel = tiene_zapatillas ? 2 : 1;
                if (abs(dif_cota) <= max_desnivel) {
                    
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

// =========================================================================
// BFS PARA NIVEL 6 (trata '?' como casilla transitable tipo camino)
// =========================================================================
list<Action> ComportamientoIngeniero::BusquedaEnAnchuraN6(const estado& origen, const estado& destino) {
    queue<nodo> abierta;
    set<estado> cerrados;

    nodo inicial;
    inicial.st = origen;
    abierta.push(inicial);
    cerrados.insert(origen);

    while (!abierta.empty()) {
        nodo actual = abierta.front();
        abierta.pop();

        if (actual.st == destino) { 
            return actual.secuencia; 
        }

        // HIJO 1: GIRAR IZQUIERDA
        nodo hijo_sl = actual;
        hijo_sl.st.orientacion = (actual.st.orientacion + 7) % 8;
        if (cerrados.find(hijo_sl.st) == cerrados.end()) {
            hijo_sl.secuencia.push_back(TURN_SL);
            cerrados.insert(hijo_sl.st);
            abierta.push(hijo_sl);
        }

        // HIJO 2: GIRAR DERECHA
        nodo hijo_sr = actual;
        hijo_sr.st.orientacion = (actual.st.orientacion + 1) % 8;
        if (cerrados.find(hijo_sr.st) == cerrados.end()) {
            hijo_sr.secuencia.push_back(TURN_SR);
            cerrados.insert(hijo_sr.st);
            abierta.push(hijo_sr);
        }

        // HIJO 3: AVANZAR (WALK)
        nodo hijo_walk = actual;
        int nf = actual.st.fila;
        int nc = actual.st.columna;

        switch(actual.st.orientacion) {
            case 0: nf--; break;
            case 1: nf--; nc++; break;
            case 2: nc++; break;
            case 3: nf++; nc++; break;
            case 4: nf++; break;
            case 5: nf++; nc--; break;
            case 6: nc--; break;
            case 7: nf--; nc--; break;
        }

        if (nf >= 0 && nf < (int)mapaResultado.size() && nc >= 0 && nc < (int)mapaResultado[0].size()) {
            unsigned char celda = mapaResultado[nf][nc];
            
            // En Nivel 6: '?' se trata como transitable (optimista)
            // No pasamos por Muros ni Precipicios
            if (celda != 'P' && celda != 'M') {
                bool transitable = true;
                
                // Si la celda es conocida, comprobamos altura
                if (celda != '?') {
                    int dif_cota = mapaCotas[nf][nc] - mapaCotas[actual.st.fila][actual.st.columna];
                    int max_desnivel = tiene_zapatillas ? 2 : 1;
                    if (abs(dif_cota) > max_desnivel) transitable = false;
                    // Bosque no transitable sin zapatillas
                    if (celda == 'B' && !tiene_zapatillas) transitable = false;
                }
                // Si es '?' asumimos cota similar (optimista) => transitable
                
                if (transitable) {
                    hijo_walk.st.fila = nf;
                    hijo_walk.st.columna = nc;
                    
                    if (cerrados.find(hijo_walk.st) == cerrados.end()) {
                        hijo_walk.secuencia.push_back(WALK);
                        cerrados.insert(hijo_walk.st);
                        abierta.push(hijo_walk);
                    }
                }
            }
        }
    }

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