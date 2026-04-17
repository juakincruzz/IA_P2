#include "motorlib/entidad.hpp"

/**
 * @brief Reinicia el estado de la entidad tras una muerte o colisión grave.
 *
 * Si la entidad es un jugador o un técnico, se reactiva inmediatamente
 * (desactivado = 0). En otro caso se le asigna un tiempo aleatorio de
 * inactividad. Se borran los indicadores de choque y de fin, y se
 * contabiliza la muerte si la vida no era cero.
 */
void Entidad::resetEntidad() {
  hitbox = false;
  if (tipo == jugador or subtipo == tecnico) {
    desactivado = 0;
  } else {
    desactivado = aleatorio(7) + 3;
  }

  choque = false;
  colisiones = 0;
  reset = true;
  if (vida != 0)
    muertes_innecesarias++;
  // vida = 0;
  done = false;

  mensaje = true;
  enfrente = false;
  enfrente = false;
  gotoF = -1;
  gotoC = -1;
}

/**
 * @brief Comprueba si una casilla dada coincide con algún objetivo no alcanzado.
 *
 * Recorre la lista de destinos de la entidad y devuelve la posición del
 * objetivo que coincide con las coordenadas indicadas, siempre que
 * dicho objetivo no haya sido ya alcanzado.
 *
 * @param paramF  Fila de la casilla a comprobar.
 * @param paramC  Columna de la casilla a comprobar.
 * @return Índice del objetivo coincidente, o -1 si no se encuentra
 *         o ya ha sido alcanzado.
 */
int Entidad::isMemberObjetivo(unsigned int paramF, unsigned int paramC) {
  int pos = -1;
  for (int i = 0; i < num_destinos and pos == -1; i++) {
    if (destino[2 * i] == paramF and destino[2 * i + 1] == paramC) {
      pos = i;
    }
  }
  if (pos != -1 and alcanzados[pos])
    pos = -1;

  return pos;
}

/**
 * @brief Indica si todos los objetivos de la entidad han sido alcanzados.
 *
 * @return true  si cada uno de los destinos está marcado como alcanzado.
 * @return false si queda al menos un destino sin alcanzar.
 */
bool Entidad::allObjetivosAlcanzados() {
  bool conseguido = true;
  for (int i = 0; i < num_destinos and conseguido; i++) {
    conseguido = conseguido and alcanzados[i];
  }
  return conseguido;
}

/**
 * @brief Indica si se han alcanzado todos los objetivos menos uno.
 *
 * Útil para comprobar si la entidad está a punto de completar su misión
 * (le falta alcanzar un único destino).
 *
 * @return true  si exactamente (num_destinos − 1) objetivos están alcanzados.
 * @return false en caso contrario.
 */
bool Entidad::allLessOneObjetivosAlcanzados() {
  int num = 0;
  for (int i = 0; i < num_destinos; i++)
    if (alcanzados[i])
      num++;

  return (num == num_destinos - 1);
}

/**
 * @brief Marca como alcanzado el objetivo correspondiente a la posición
 *        actual de la entidad.
 *
 * Consulta si la posición actual (f, c) coincide con algún objetivo
 * pendiente y, en caso afirmativo, lo marca como alcanzado.
 */
void Entidad::actualizarAlcanzados() {
  int pos = isMemberObjetivo(f, c);
  if (pos != -1 and !alcanzados[pos]) {
    alcanzados[pos] = true;
  }
}

/**
 * @brief Reinicia todos los objetivos a estado no alcanzado.
 *
 * Se utiliza, por ejemplo, al asignar un nuevo conjunto de objetivos
 * en el nivel 7.
 */
void Entidad::anularAlcanzados() {
  for (int i = 0; i < num_destinos; i++) {
    alcanzados[i] = false;
  }
}

/**
 * @brief Establece la lista de objetivos (destinos) de la entidad.
 *
 * Recibe un vector donde cada par consecutivo de elementos representa
 * las coordenadas (fila, columna) de un destino. Se actualiza el número
 * de destinos y se reinician todos como no alcanzados.
 *
 * @param v  Vector de coordenadas de destino en formato
 *           {f0, c0, f1, c1, …}.
 */
void Entidad::setObjetivos(vector<unsigned int> v) {
  destino = v;
  num_destinos = destino.size() / 2;
  for (int i = 0; i < num_destinos; i++) {
    alcanzados[i] = false;
  }
}

/**
 * @brief Devuelve la fila del objetivo en la posición indicada.
 *
 * @param pos  Índice del objetivo (0-based).
 * @return Fila del objetivo. Termina el programa si @p pos está fuera
 *         de rango.
 */
unsigned int Entidad::getObjFil(int pos) {
  if (pos < num_destinos)
    return destino[2 * pos];
  else {
    std::cout << "Error en getObjFil: intento de acceso a una posición de "
                 "objetivo que no existe\n";
    exit(1);
  }
}

/**
 * @brief Devuelve la columna del objetivo en la posición indicada.
 *
 * @param pos  Índice del objetivo (0-based).
 * @return Columna del objetivo. Termina el programa si @p pos está fuera
 *         de rango.
 */
unsigned int Entidad::getObjCol(int pos) {
  if (pos < num_destinos)
    return destino[2 * pos + 1];
  else {
    std::cout << "Error en getObjFil: intento de acceso a una posición de "
                 "objetivo que no existe\n";
    exit(1);
  }
}


/**
 * @brief Ejecuta el ciclo de decisión de la entidad para un instante
 *        de simulación.
 *
 * Rellena la estructura @c Sensores con la información disponible para
 * la entidad (posición, orientación, visión, energía, etc.) y delega
 * la elección de la acción al comportamiento asociado mediante
 * @c comportamiento->think(sensor). Controla también las condiciones
 * de fin de partida (energía agotada, instantes agotados, tiempo de
 * deliberación excedido) y gestiona el estado de desactivación temporal
 * de las entidades NPC.
 *
 * @param acc     Acción forzada externamente (-1 si se deja decidir
 *                al comportamiento).
 * @param vision  Matriz 3×N con las capas de visión: superficie,
 *                agentes y cotas.
 * @param level   Nivel de juego actual.
 * @return Acción elegida por la entidad para este instante.
 */
Action Entidad::think(int acc, vector<vector<unsigned char>> vision,
                      int level, int impacto_eco, int max_impact) {
  Action accion = IDLE;
  Sensores sensor;

  if (desactivado == 0) {
    vida--;
    hitbox = true;

    sensor.nivel = level;

    sensor.vida = vida;
    sensor.energia = energia;

    sensor.BelPosF = -1;
    sensor.BelPosC = -1;

    sensor.GotoF = gotoF;
    sensor.GotoC = gotoC;

    sensor.choque = choque;
    sensor.reset = reset;

    sensor.venpaca = venpaca;
    sensor.enfrente = enfrente;
    sensor.enfrente = enfrente;

    sensor.posF = f;
    sensor.posC = c;
    sensor.rumbo = orient;
    sensor.ecologico = impacto_eco;
    sensor.max_ecologico = max_impact;

    if (tipo == jugador) // Poner los sensores que se activan al jugador
    {
      // Si estoy en un nivel deliberativo activo que haya un destino (un
      // accidentado)
      if (level > 1) {
        sensor.BelPosF = destino[0];
        sensor.BelPosC = destino[1];
      }

      if (level == 6 and sensor.posF == destino[0] and
          sensor.posC == destino[1]) {
      }
    } else if (subtipo ==
               tecnico) // Poner los sensores que se le activan al tecnico
    {
      if (level > 1) {
        sensor.BelPosF = destino[0];
        sensor.BelPosC = destino[1];
        if (level == 6 and !MeHasLLamado()) {
          sensor.BelPosF = -1;
          sensor.BelPosC = -1;
        }
      }
    }

    if (tipo == jugador or subtipo == tecnico) {
      if (sensor.energia == 0) {
        done = true;
        cout << "Se agoto la energía!" << endl;
      } else if (sensor.vida == 0) {
        done = true;
        cout << "Se agotaron los instantes de simulacion" << endl;
      } else if (getTiempo() > getTiempoMaximo()) {
        done = true;
        cout << "Se agoto el tiempo de deliberacion!" << endl;
      }
    }

    sensor.cota = vision[2];
    sensor.agentes = vision[1];
    sensor.superficie = vision[0];

    sensor.tiempo = getTiempo() / CLOCKS_PER_SEC;

    visionAux = vision;

    if (!done) {
      if (acc == -1)
        accion = comportamiento->think(sensor);
      else
        accion = static_cast<Action>(acc);
    }
    choque = false;
    reset = false;
    mensaje = false;
  } else {
    desactivado--;
  }

  return accion;
}

/**
 * @brief Procesa una interacción externa sobre la entidad.
 *
 * Delega en el método @c interact del comportamiento asociado.
 * Según el valor de retorno del comportamiento, la entidad puede
 * desaparecer temporalmente (retorno 1), morir (retorno 2) o
 * revivir (retorno 3).
 *
 * @param accion  Acción que provoca la interacción.
 * @param valor   Valor numérico asociado a la interacción.
 * @return true  si la interacción provocó un cambio de estado.
 * @return false si no hubo efecto.
 */
bool Entidad::interact(Action accion, int valor) {
  bool out = false;
  int retorno = comportamiento->interact(accion, valor);

  if (retorno == 1) { // Desaparición temporal
    resetEntidad();
    out = true;
  }

  if (retorno == 2) { // Muerte
    hitbox = false;
    desactivado = -1;
    out = true;
  }

  if (retorno == 3) { // Revivir
    hitbox = true;
    desactivado = 0;
    out = true;
  }

  return out;
}

/**
 * @brief Devuelve el carácter que representa el subtipo de la entidad.
 *
 * @return 'i' para ingeniero, 't' para técnico, 'e' para excursionista,
 *         'v' para vándalo, o ' ' si el subtipo no coincide con ninguno.
 */
unsigned char Entidad::getSubTipoChar() {
  unsigned char out = ' ';

  switch (subtipo) {
  case ingeniero:
    out = 'i';
    break;
  case excursionista:
    out = 'e';
    break;
  case tecnico:
    out = 't';
    break;
  case vandalo:
    out = 'v';
    break;
  }

  return out;
}

/**
 * @brief Fija el tiempo que consumirá la siguiente acción.
 *
 * Actualmente asigna siempre un coste temporal de 1 instante,
 * independientemente del tipo de casilla.
 *
 * @param celda  Tipo de casilla sobre la que se encuentra la entidad.
 */
void Entidad::fixTiempo_sig_accion(unsigned char celda) {
  tiempo_sig_accion = 1;
}

/**
 * @brief Calcula y fija el coste en energía (batería) de la siguiente
 *        acción del jugador.
 *
 * El coste depende del tipo de casilla en la que se encuentra la entidad,
 * de la diferencia de altura con la casilla destino (si procede) y de la
 * acción a realizar. Los costes cubren las acciones WALK, JUMP, giros
 * (TURN_SR/TURN_SL), INSTALL, RAISE y DIG; IDLE y COME tienen coste 0.
 *
 * @param celdaJugador  Tipo de casilla sobre la que se encuentra la entidad.
 * @param difAltura     Diferencia de altura entre casilla destino y origen.
 * @param accion        Acción a ejecutar.
 * @return Coste en energía de la acción.
 */
int Entidad::fixBateria_sig_accion_jugador(unsigned char celdaJugador,
                                           int difAltura, Action accion) {
  bateria_sig_accion = 1;
  switch (accion) {
  case IDLE:
  case COME:
    bateria_sig_accion = 0;
    break;

  case WALK:
    switch (celdaJugador) {
    case 'A':
      bateria_sig_accion = 60;
      break;
    case 'H':
      bateria_sig_accion = 6;
      break;
    case 'S':
      bateria_sig_accion = 3;
      break;
    default:
      bateria_sig_accion = 1;
      break;
    }
    if (celdaJugador == 'A' or celdaJugador == 'H' or celdaJugador == 'S') {
      if (difAltura > 0)
        bateria_sig_accion += 5;
      else if (difAltura < 0)
        bateria_sig_accion -= 2;
    }
    break;

  case JUMP:
    switch (celdaJugador) {
    case 'A':
      bateria_sig_accion = 90;
      break;
    case 'H':
      bateria_sig_accion = 10;
      break;
    case 'S':
      bateria_sig_accion = 4;
      break;
    default:
      bateria_sig_accion = 3;
      break;
    }
    if (difAltura > 0)
      bateria_sig_accion += 5;
    else if (difAltura < 0)
      bateria_sig_accion -= 2;
    break;

  case TURN_SR:
  case TURN_SL:
    switch (celdaJugador) {
    case 'A':
      bateria_sig_accion = 5;
      break;
    case 'H':
      bateria_sig_accion = 2;
      break;
    case 'S':
      bateria_sig_accion = 1;
      break;
    default:
      bateria_sig_accion = 1;
      break;
    }
    break;

  case INSTALL:
    switch (celdaJugador) {
    case 'A': bateria_sig_accion = 60; break;
    case 'H': bateria_sig_accion = 45; break;
    case 'B': bateria_sig_accion = 35; break;
    case 'S': bateria_sig_accion = 25; break;
    case 'C':
    case 'U': bateria_sig_accion = 15; break;
    case 'X':
    case '?': bateria_sig_accion = 1;  break;
    default:  bateria_sig_accion = 30; break;
    }
    break;

  case RAISE:
    switch (celdaJugador) {
    case 'A': bateria_sig_accion = 70; break;
    case 'H': bateria_sig_accion = 55; break;
    case 'B': bateria_sig_accion = 45; break;
    case 'S': bateria_sig_accion = 30; break;
    case 'C':
    case 'U': bateria_sig_accion = 10; break;
    case 'X':
    case '?': bateria_sig_accion = 1;  break;
    default:  bateria_sig_accion = 40; break;
    }
    break;

  case DIG:
    switch (celdaJugador) {
    case 'A': bateria_sig_accion = 75; break;
    case 'H': bateria_sig_accion = 65; break;
    case 'B': bateria_sig_accion = 50; break;
    case 'S': bateria_sig_accion = 40; break;
    case 'C':
    case 'U': bateria_sig_accion = 25; break;
    case 'X':
    case '?': bateria_sig_accion = 1;  break;
    default:  bateria_sig_accion = 50; break;
    }
    break;
  }

  return bateria_sig_accion;
}

/**
 * @brief Descuenta de la energía de la entidad el coste previamente
 *        calculado de la siguiente acción.
 *
 * Si la energía resultante es negativa, se fija a 0.
 */
void Entidad::decBateria_sig_accion() {
  energia -= bateria_sig_accion;
  if (energia < 0)
    energia = 0;
}
