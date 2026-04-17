#include "motorlib/monitor.hpp"
#include "modelos/goomba.hpp"
#include "modelos/luigi.hpp"
#include "modelos/mario.hpp"
#include "modelos/toad.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

/**
 * @brief Cambia la orientación de una entidad a partir de un código numérico.
 *
 * Convierte el valor numérico (0-7) en la enumeración @c Orientacion
 * correspondiente y la asigna a la entidad indicada.
 *
 * @param entidad      Índice de la entidad cuya orientación se modifica.
 * @param orientacion  Código numérico de orientación (0 = norte, 1 = noreste, …, 7 = noroeste).
 */
void MonitorJuego::cambiarOrientacion(unsigned char entidad,
                                      unsigned char orientacion) {
  Orientacion aux;
  switch (orientacion) {
  case 0:
    aux = norte;
    break;
  case 1:
    aux = noreste;
    break;
  case 2:
    aux = este;
    break;
  case 3:
    aux = sureste;
    break;
  case 4:
    aux = sur;
    break;
  case 5:
    aux = suroeste;
    break;
  case 6:
    aux = oeste;
    break;
  case 7:
    aux = noroeste;
    break;
  }
  entidades[entidad]->setOrientacion(aux);
}

/**
 * @brief Carga un mapa desde un fichero y lo asigna al monitor.
 *
 * Lee el fichero indicado, que contiene las dimensiones del mapa seguidas
 * de la matriz de terreno (caracteres) y la matriz de alturas (dígitos).
 * Inicializa la estructura de tuberías e instancia el objeto @c Mapa.
 *
 * @param file  Ruta al fichero de mapa (formato texto).
 */
void MonitorJuego::setMapa(const char *file) {
  ifstream ifile;
  ifile.open(file, ios::in);

  unsigned int colSize, filSize;
  ifile >> colSize;
  ifile >> filSize;

  vector<vector<unsigned char>> mapAuxTerreno;
  vector<vector<unsigned char>> mapAuxAlturas;

  vector<unsigned char> colAuxTerreno(colSize);
  vector<unsigned char> colAuxAlturas(colSize);

  for (unsigned int i = 0; i < filSize; i++) {
    mapAuxTerreno.push_back(colAuxTerreno);
    mapAuxAlturas.push_back(colAuxAlturas);
  }

  // Lee la parte de terreno del fichero
  for (unsigned int i = 0; i < filSize; i++) {
    for (unsigned int j = 0; j < colSize; j++) {
      ifile >> mapAuxTerreno[i][j];
    }
  }

  // Lee la parte de alturas del terreno del fichero
  for (unsigned int i = 0; i < filSize; i++) {
    for (unsigned int j = 0; j < colSize; j++) {
      unsigned char h;
      ifile >> h;
      mapAuxAlturas[i][j] = h - '0';
    }
  }
  ifile.close();

  initialize_tuberias(filSize, colSize);
  mapa = new Mapa(mapAuxTerreno, mapAuxAlturas, &entidades);
}

/**
 * @brief Comprueba si se ha solicitado iniciar el juego y lo pone en marcha.
 *
 * Devuelve @c true la primera vez que se invoca tras solicitar el inicio,
 * reiniciando el indicador interno. En llamadas posteriores devuelve @c false.
 *
 * @return true si el juego acaba de inicializarse; false en caso contrario.
 */
bool MonitorJuego::inicializarJuego() {
  bool aux = empezarJuego;
  if (empezarJuego) {
    empezarJuego = false;
    resultados = false;
  }
  return aux;
}

/**
 * @brief Genera aleatoriamente una celda válida (ni precipicio ni muro) y
 *        una orientación.
 *
 * @param[out] pos_fila  Fila de la celda generada.
 * @param[out] pos_col   Columna de la celda generada.
 * @param[out] ori       Orientación aleatoria (0-7).
 */
void MonitorJuego::generate_a_valid_cell(int &pos_fila, int &pos_col,
                                         int &ori) {
  pos_col = -1;
  pos_fila = -1;
  char celdaRand = '_';
  do {
    pos_fila = aleatorio(getMapa()->getNFils() - 1);
    pos_col = aleatorio(getMapa()->getNCols() - 1);

    celdaRand = getMapa()->getCelda(pos_fila, pos_col);
  } while ((celdaRand == 'P' or celdaRand == 'M'));
  ori = aleatorio(7);
}

/**
 * @brief Comprueba si una celda es apta como objetivo.
 *
 * Una celda es válida como objetivo si no es muro, precipicio y se
 * encuentra al menos a 3 casillas del borde del mapa.
 *
 * @param f  Fila de la celda.
 * @param c  Columna de la celda.
 * @return true si la celda es válida como objetivo.
 */
bool MonitorJuego::is_a_valid_cell_like_goal(int f, int c) {
  if (f < 3 or f + 3 >= getMapa()->getNFils())
    return false;
  if (c < 3 or c + 3 >= getMapa()->getNCols())
    return false;
  char cell = getMapa()->getCelda(f, c);
  if (cell == 'M' or cell == 'P')
    return false;
  return true;
}

/**
 * @brief Genera un objetivo aleatorio y lo añade a la lista de objetivos.
 *
 * Busca una celda aleatoria que no sea precipicio, muro ni bosque
 * y la inserta al final de la lista @c objetivos.
 */
void MonitorJuego::generate_a_objetive() {
  int pos_col = -1, pos_fila = -1;
  char celdaRand = '_';
  do {
    pos_fila = aleatorio(getMapa()->getNFils() - 1);
    pos_col = aleatorio(getMapa()->getNCols() - 1);

    celdaRand = getMapa()->getCelda(pos_fila, pos_col);
  } while ((celdaRand == 'P' or celdaRand == 'M' or celdaRand == 'B'));

  pair<int, int> punto;
  punto.first = pos_fila;
  punto.second = pos_col;
  objetivos.push_back(punto);
}

/**
 * @brief Inserta un objetivo al principio de la lista de objetivos.
 *
 * @param fila     Fila del nuevo objetivo.
 * @param columna  Columna del nuevo objetivo.
 */
void MonitorJuego::put_a_new_objetivo_front(int fila, int columna) {
  pair<int, int> punto;
  punto.first = fila;
  punto.second = columna;
  objetivos.push_front(punto);
}

/**
 * @brief Transfiere objetivos de la cola al vector de objetivos activos.
 *
 * Extrae @p number objetivos de la lista @c objetivos, descartando
 * aquellos que coincidan con la posición actual del ingeniero o del
 * técnico, y los fija como objetivos activos.
 *
 * @param number  Número de objetivos a activar simultáneamente.
 */
void MonitorJuego::put_active_objetivos(int number) {
  vector<unsigned int> v;

  if (mapa != 0) {
    while (objetivos.size() < number) {
      generate_a_objetive();
    }
    for (int i = 0; i < number; i++) {
      auto it = objetivos.begin();
      if (this->get_entidades()->size() > 1) {
        while (it != objetivos.end() &&
               ((this->get_entidad(0)->getFil() == it->first &&
                 this->get_entidad(0)->getCol() == it->second) ||
                (this->get_entidad(1)->getFil() == it->first &&
                 this->get_entidad(1)->getCol() == it->second))) {
          objetivos.erase(objetivos.begin());
          if (objetivos.empty())
            generate_a_objetive();
          it = objetivos.begin();
        }
      }
      v.push_back(it->first);
      v.push_back(it->second);

      objetivos.erase(objetivos.begin());
    }
  } else {
    v.push_back(3);
    v.push_back(3);
  }
  objetivosActivos = v;
}

/**
 * @brief Obtiene las coordenadas del primer objetivo activo.
 *
 * @param n          Índice del objetivo (actualmente se ignora y siempre
 *                   se devuelve el primero).
 * @param[out] posFila  Fila del objetivo.
 * @param[out] posCol   Columna del objetivo.
 */
void MonitorJuego::get_n_active_objetivo(int n, int &posFila, int &posCol) {
  if (2 * n + 1 < objetivosActivos.size()) {
    posFila = objetivosActivos[2 * n];
    posCol = objetivosActivos[2 * n + 1];
  }
}

/**
 * @brief Establece las coordenadas del primer objetivo activo y las
 *        asigna al ingeniero.
 *
 * @param n        Índice del objetivo (actualmente se ignora).
 * @param posFila  Nueva fila del objetivo.
 * @param posCol   Nueva columna del objetivo.
 */
void MonitorJuego::set_n_active_objetivo(int n, int posFila, int posCol) {
  if (2 * n + 1 < objetivosActivos.size()) {
    objetivosActivos[2 * n] = posFila;
    objetivosActivos[2 * n + 1] = posCol;
  }

  for (unsigned int i = 0; i < entidades.size(); i++) {
    entidades[i]->setObjetivos(objetivosActivos);
  }
}

/**
 * @brief Indica si hay al menos un objetivo activo.
 *
 * @return true si el vector de objetivos activos no está vacío.
 */
bool MonitorJuego::there_are_active_objetivo() {
  return (objetivosActivos.size() > 0);
}

/**
 * @brief Devuelve el número de objetivos activos.
 *
 * Cada objetivo ocupa dos posiciones en el vector (fila, columna),
 * por lo que el número se calcula como tamaño / 2.
 *
 * @return Cantidad de objetivos activos.
 */
int MonitorJuego::get_number_active_objetivos() {
  return (objetivosActivos.size() / 2);
}

/**
 * @brief Devuelve una copia del vector de objetivos activos.
 *
 * @return Vector con las coordenadas de los objetivos activos
 *         en formato {f0, c0, f1, c1, …}.
 */
vector<unsigned int> MonitorJuego::get_active_objetivos() {
  vector<unsigned int> copia = objetivosActivos;
  return copia;
}

/**
 * @brief Decrementa el contador de pasos restantes y comprueba las
 *        condiciones de finalización de la partida.
 *
 * Actualiza los marcadores visuales, verifica si alguno de los
 * agentes ha completado su objetivo o ha agotado la vida/energía,
 * y gestiona la transición a la pantalla de resultados. En niveles
 * ≥ 5 también comprueba el impacto ecológico y el tiempo de deliberación.
 */
void MonitorJuego::decPasos() {
  updateVisualMarkers();
  if (get_entidad(0)->fin() or get_entidad(1)->fin()) {
    if (nivel < 5) {
      jugando = false;
      resultados = true;
    } else {
      // Nivel 5: Terminate on success (fin flag) or failure
      // (death/timeout/battery)
      if (get_entidad(0)->fin() or get_entidad(1)->fin() or
          !get_entidad(0)->vivo() or !get_entidad(1)->vivo() or
          get_entidad(0)->getBateria() == 0 or
          get_entidad(1)->getBateria() == 0 or
          impacto_ecologico >= maxImpacto or
          get_entidad(0)->getTiempo() + get_entidad(1)->getTiempo() >
              get_entidad(0)->getTiempoMaximo()) {
        jugando = false;
        resultados = true;
      } else {
        // Nuevo destino
        if (get_entidad(0)->allObjetivosAlcanzados()) {
          /*put_active_objetivos(3);
          get_entidad(0)->anularAlcanzados();
          get_entidad(0)->setObjetivos(get_active_objetivos());*/
          // setObjCol(pos_col);
          // setObjFil(pos_fila);
          // if (editPosC != NULL)
          // editPosC->set_int_val(pos_col);
          // if (editPosF != NULL)
          // editPosF->set_int_val(pos_fila);
          // cout << "Nuevo objetivo: (" << pos_fila << "," << pos_col << ")" <<
          // endl;
          get_entidad(0)->resetFin();
        }

        if (pasos > 0) {
          pasos--;
          pasosTotales++;
        }
      }
    }
  } else {
    if (pasos > 0) {
      pasos--;
      pasosTotales++;
    }

    if (!get_entidad(0)->vivo() or get_entidad(0)->getBateria() == 0 or
        !get_entidad(1)->vivo() or get_entidad(1)->getBateria() == 0 or
        impacto_ecologico >= maxImpacto) {
      jugando = false;
      resultados = true;
    }
  }
}

/**
 * @brief Comprueba si una entidad puede ver una celda desde su posición
 *        y orientación actuales.
 *
 * La visibilidad se calcula según el campo de visión de 3 casillas
 * en el cono frontal de la orientación del agente.
 *
 * @param num_entidad  Índice de la entidad.
 * @param fil          Fila de la celda objetivo.
 * @param col          Columna de la celda objetivo.
 * @return true si la celda está dentro del campo de visión.
 */
bool MonitorJuego::CanHeSeesThisCell(int num_entidad, int fil, int col) {
  bool salida = false;
  int agenteF = get_entidad(num_entidad)->getFil();
  int agenteC = get_entidad(num_entidad)->getCol();
  Orientacion agenteRumbo = get_entidad(num_entidad)->getOrientacion();

  int df = agenteF - fil;
  int dc = agenteC - col;
  int absf = (df < 0 ? -df : df);
  int absc = (dc < 0 ? -dc : dc);

  if (absf > 3 or absc > 3)
    return false;

  switch (agenteRumbo) {
  case norte:
    if (agenteF > fil and absc <= absf) {
      salida = true;
    }
    break;
  case noreste:
    if (agenteF >= fil and dc <= 0) {
      salida = true;
    }
    break;
  case este:
    if (agenteC < col and absf <= absc) {
      salida = true;
    }
    break;
  case sureste:
    if (agenteF <= fil and dc <= 0) {
      salida = true;
    }
    break;
  case sur:
    if (agenteF < fil and absc <= absf) {
      salida = true;
    }
    break;
  case suroeste:
    if (agenteF <= fil and dc >= 0) {
      salida = true;
    }
    break;
  case oeste:
    if (agenteC > col and absf <= absc) {
      salida = true;
    }
    break;
  case noroeste:
    if (agenteF >= fil and dc >= 0) {
      salida = true;
    }
    break;
  }
  return salida;
}

/**
 * @brief Inicializa el estado completo del juego: mapa, entidades, objetivos
 *        y tuberías.
 *
 * Coloca al ingeniero y al técnico en las posiciones indicadas (o aleatorias
 * si valen −1), crea los comportamientos adecuados al nivel, genera los
 * objetivos iniciales y, en el nivel 7, añade excursionistas y vándalos.
 *
 * @param pos_filaJ  Fila inicial del ingeniero (−1 = aleatoria).
 * @param pos_colJ   Columna inicial del ingeniero (−1 = aleatoria).
 * @param brujJ      Orientación inicial del ingeniero (−1 = aleatoria).
 * @param pos_filaS  Fila inicial del técnico (−1 = aleatoria).
 * @param pos_colS   Columna inicial del técnico (−1 = aleatoria).
 * @param brujS      Orientación inicial del técnico (−1 = aleatoria).
 * @param seed       Semilla utilizada para la generación aleatoria.
 */
void MonitorJuego::inicializar(int pos_filaJ, int pos_colJ, int brujJ,
                               int pos_filaS, int pos_colS, int brujS,
                               int seed) {
  clear();
  // cout << "Estoy en MonitorJuego::inicializar\n";
  int tama = getMapa()->getNCols();
  initialize_tuberias(getMapa()->getNFils(), getMapa()->getNCols());

  int nexcursionistas = 4;
  int nvandalos = 4;
  // int nexcursionistas = tama / 10;
  // int nvandalos = tama / 10;
  unsigned char celdaRand;

  // Se construye una lisa con 300 objetivos
  if (nivel == 7) {
    while (objetivos.size() < 300) {
      generate_a_objetive();
    }
  }

  // Primero SIEMPRE se coloca al ingeniero. SIEMPRE.
  if ((pos_filaJ == -1) or (pos_colJ == -1)) {
    do {
      pos_filaJ = aleatorio(getMapa()->getNFils() - 1);
      pos_colJ = aleatorio(getMapa()->getNCols() - 1);
      celdaRand = getMapa()->getCelda(pos_filaJ, pos_colJ);
    } while ((celdaRand == 'P') or (celdaRand == 'M') or (celdaRand == 'B') or
             (getMapa()->entidadEnCelda(pos_filaJ, pos_colJ) != '_'));
  } else {
    // para hacer que la secuencia de recogida de numeros aleatorios sea
    // independiente si da o no las coordenadas al principio
    int kkx = aleatorio(getMapa()->getNFils() - 1);
    int kky = aleatorio(getMapa()->getNCols() - 1);
  }

  // El segundo SIEMPRE es el tecnico. Siempre. SIEMPRE.
  if ((pos_filaS == -1) or (pos_colS == -1)) {
    do {
      pos_filaS = aleatorio(getMapa()->getNFils() - 1);
      pos_colS = aleatorio(getMapa()->getNCols() - 1);
      celdaRand = getMapa()->getCelda(pos_filaS, pos_colS);
    } while ((celdaRand == 'P') or (celdaRand == 'M') or (celdaRand == 'B') or
             (getMapa()->entidadEnCelda(pos_filaS, pos_colS) != '_'));
  } else {
    // para hacer que la secuencia de recogida de numeros aleatorios sea
    // independiente si da o no las coordenadas al principio
    int kkx = aleatorio(getMapa()->getNFils() - 1);
    int kky = aleatorio(getMapa()->getNCols() - 1);
  }

  // Pongo los primeros objetivos objetivosActivos
  put_active_objetivos(1);

  // Para los niveles 2, 3, 4 y 5 hago visible el mapa
  if ((nivel == 2) or (nivel == 3) or (nivel == 4) or (nivel == 5)) {
    vector<vector<unsigned char>> mAux(
        getMapa()->getNFils(),
        vector<unsigned char>(getMapa()->getNCols(), '?'));
    for (int i = 0; i < getMapa()->getNFils(); i++)
      for (int j = 0; j < getMapa()->getNCols(); j++)
        mAux[i][j] = getMapa()->getCelda(i, j);

    vector<vector<unsigned char>> mAuxC(
        getMapa()->getNFils(), vector<unsigned char>(getMapa()->getNCols(), 0));
    for (int i = 0; i < getMapa()->getNFils(); i++)
      for (int j = 0; j < getMapa()->getNCols(); j++)
        mAuxC[i][j] = getMapa()->alturaEnCelda(i, j);

    // Esto coloca a los agentes en el mapa

    nueva_entidad(new Entidad(
        jugador, ingeniero, static_cast<Orientacion>(brujJ), pos_filaJ,
        pos_colJ, new Mario3D(""), new ComportamientoIngeniero(mAux, mAuxC), 1,
        objetivosActivos, energiaInicial, maxPasos));
    nueva_entidad(new Entidad(npc, tecnico, static_cast<Orientacion>(brujS),
                              pos_filaS, pos_colS, new Luigi3D(""),
                              new ComportamientoTecnico(mAux, mAuxC), 1,
                              objetivosActivos, energiaInicial, maxPasos));
  } else {
    // Esto coloca al jugador en el mapa
    if (brujJ == -1)
      brujJ = rand() % 8;
    if (brujS == -1)
      brujS = rand() % 8;

    nueva_entidad(
        new Entidad(jugador, ingeniero, static_cast<Orientacion>(brujJ),
                    pos_filaJ, pos_colJ, new Mario3D(""),
                    new ComportamientoIngeniero(getMapa()->getNFils()), 1,
                    objetivosActivos, energiaInicial, maxPasos));
    nueva_entidad(new Entidad(npc, tecnico, static_cast<Orientacion>(brujS),
                              pos_filaS, pos_colS, new Luigi3D(""),
                              new ComportamientoTecnico(getMapa()->getNFils()),
                              1, objetivosActivos, energiaInicial, maxPasos));
  }

  ReAparicionesEntidad(0, pos_filaJ, pos_colJ, static_cast<Orientacion>(brujJ));
  ReAparicionesEntidad(1, pos_filaS, pos_colS, static_cast<Orientacion>(brujS));
  get_entidad(0)->setLastAction(IDLE);
  get_entidad(1)->setLastAction(IDLE);

  // Para el último nivel genero los excursionistas y vandalos
  int pos_filaO, pos_colO;
  if (nivel == 7) {
    for (int i = 0; i < nexcursionistas; i++) {
      do {
        pos_filaO = aleatorio(getMapa()->getNFils() - 1);
        pos_colO = aleatorio(getMapa()->getNCols() - 1);
        celdaRand = getMapa()->getCelda(pos_filaO, pos_colO);
      } while ((celdaRand == 'P') or (celdaRand == 'M') or
               (getMapa()->entidadEnCelda(pos_filaO, pos_colO) != '_'));
      nueva_entidad(new Entidad(
          npc, excursionista, static_cast<Orientacion>(aleatorio(7)), pos_filaO,
          pos_colO, new Toad3D(""), new ComportamientoExcursionista(), 1,
          objetivosActivos, energiaInicial, maxPasos));
    }

    for (int i = 0; i < nvandalos; i++) {
      do {
        pos_filaO = aleatorio(getMapa()->getNFils() - 1);
        pos_colO = aleatorio(getMapa()->getNCols() - 1);
        celdaRand = getMapa()->getCelda(pos_filaO, pos_colO);
      } while ((celdaRand == 'P') or (celdaRand == 'M') or
               (getMapa()->entidadEnCelda(pos_filaO, pos_colO) != '_'));
      nueva_entidad(new Entidad(
          npc, vandalo, static_cast<Orientacion>(aleatorio(7)), pos_filaO,
          pos_colO, new Goomba3D(""), new ComportamientoVandalo(), 1,
          objetivosActivos, energiaInicial, maxPasos));
    }
  }

  get_entidad(0)->setVision(getMapa()->vision(0));
  get_entidad(1)->setVision(getMapa()->vision(1));

  // Se conoce en todos los niveles la posición y orientación de los dos.
  // if ((nivel == 0) or (nivel == 1) or (nivel == 2) or (nivel == 3))
  get_entidad(0)->notify();
  get_entidad(1)->notify();

  /* quitar esto una vez verificado */
  // PintaEstadoMonitor();

  // Avisos de colocación incorrecta de los agentes al principio del juego según
  // los niveles
  switch (nivel) {
  case 0:
    if (getMapa()->getCelda(get_entidad(0)->getFil(),
                            get_entidad(0)->getCol()) != 'C')
      std::cout
          << "Error en la colocación del agente Ingeniero en el nivel 0\n";
    if (getMapa()->getCelda(get_entidad(1)->getFil(),
                            get_entidad(1)->getCol()) != 'C')
      std::cout << "Error en la colocación del agente Técnico en el nivel 0\n";
    break;
  case 1:
    if (getMapa()->getCelda(get_entidad(0)->getFil(),
                            get_entidad(0)->getCol()) != 'C' and
        getMapa()->getCelda(get_entidad(0)->getFil(),
                            get_entidad(0)->getCol()) != 'S')
      std::cout
          << "Error en la colocación del agente Ingeniero en el nivel 1\n";
    if (getMapa()->getCelda(get_entidad(1)->getFil(),
                            get_entidad(1)->getCol()) != 'C' and
        getMapa()->getCelda(get_entidad(1)->getFil(),
                            get_entidad(1)->getCol()) != 'S')
      std::cout << "Error en la colocación del agente Técnico en el nivel 1\n";
    break;
  }
}

/**
 * @brief Recoloca una entidad en el mapa con una posición y orientación
 *        determinadas.
 *
 * Activa la hitbox de la entidad y le retira las zapatillas. Si la celda
 * de destino es una casilla especial ('X' o 'D'), aplica su efecto
 * (recarga de batería o obtención de zapatillas).
 *
 * @param Entidad  Índice de la entidad.
 * @param fila     Fila de reaparición.
 * @param columna  Columna de reaparición.
 * @param brujula  Orientación de reaparición.
 */
void MonitorJuego::ReAparicionesEntidad(int Entidad, int fila, int columna,
                                        Orientacion brujula) {
  get_entidad(Entidad)->setPosicion(fila, columna);
  get_entidad(Entidad)->setOrientacion(brujula);
  get_entidad(Entidad)->setHitbox(true);
  get_entidad(Entidad)->Cogio_Zapatillas(false);
  switch (getMapa()->getCelda(fila, columna)) {
  case 'X': // Casilla Rosa (Recarga)
    get_entidad(Entidad)->increaseBateria(10);
    break;

  case 'D': // Casilla Morada (Zapatillas)
    get_entidad(Entidad)->Cogio_Zapatillas(true);
    break;
  }
}

/**
 * @brief Imprime por consola el estado actual del monitor de juego.
 *
 * Muestra posición y orientación de todas las entidades, así como
 * las coordenadas de los objetivos del ingeniero. Útil para depuración.
 */
void MonitorJuego::PintaEstadoMonitor() {
  cout << "*********************************************\n";
  cout << "Pos Fil: " << get_entidad(0)->getFil() << endl;
  cout << "Pos Col: " << get_entidad(0)->getCol() << endl;
  cout << "Brujula: " << get_entidad(0)->getOrientacion() << endl;
  for (int i = 0; i < get_entidad(0)->getNumObj(); i++) {
    cout << "Obj " << i << " : F: " << get_entidad(0)->getObjFil(i)
         << "  C: " << get_entidad(0)->getObjCol(i) << endl;
  }
  cout << "excursionistas: " << numero_entidades() << endl;
  for (int i = 1; i < numero_entidades(); i++) {
    cout << "  Pos Fil: " << get_entidad(i)->getFil() << endl;
    cout << "  Pos Col: " << get_entidad(i)->getCol() << endl;
    cout << "  Brujula: " << get_entidad(i)->getOrientacion() << endl << endl;
  }
  cout << "*********************************************\n";
}

/**
 * @brief Calcula el porcentaje de coincidencia entre el mapa real y
 *        el mapa construido por los agentes.
 *
 * Combina los mapas resultado del ingeniero y del técnico y compara
 * cada casilla con el mapa real. Las casillas marcadas como '?' se
 * consideran neutras; las incorrectas restan.
 *
 * @return Porcentaje de aciertos sobre el total de casillas.
 */
double MonitorJuego::CoincidenciaConElMapa() {
  int aciertos = 0, totalCasillas = 0;
  vector<vector<unsigned char>> resultado = get_entidad(0)->getMapaResultado();
  getMapa()->JoinMapasSuperficie(resultado, get_entidad(1)->getMapaResultado());

  for (unsigned int i = 0; i < getMapa()->getNFils(); i++) {
    for (unsigned int j = 0; j < getMapa()->getNCols(); j++) {
      if (getMapa()->getCelda(i, j) == resultado[i][j]) {
        aciertos++;
      } else if (resultado[i][j] !=
                 '?') { // Puso un valor distinto de desconocido en
                        // mapaResultado y no acertó
        aciertos--;
      }
      totalCasillas++;
    }
  }
  return (aciertos * 100.0 / totalCasillas);
}

/**
 * @brief Calcula el porcentaje de coincidencia solo para casillas de
 *        camino ('C') y sendero ('S').
 *
 * Análoga a @ref CoincidenciaConElMapa pero restringida a las casillas
 * transitables principales.
 *
 * @return Porcentaje de aciertos sobre el total de casillas C/S.
 */
double MonitorJuego::CoincidenciaConElMapaCaminosYSenderos() {
  int aciertos = 0, totalCasillas = 0;
  vector<vector<unsigned char>> resultado = get_entidad(0)->getMapaResultado();
  getMapa()->JoinMapasSuperficie(resultado, get_entidad(1)->getMapaResultado());

  for (unsigned int i = 0; i < getMapa()->getNFils(); i++) {
    for (unsigned int j = 0; j < getMapa()->getNCols(); j++) {
      if (getMapa()->getCelda(i, j) == 'C' or
          getMapa()->getCelda(i, j) == 'S') {
        if (getMapa()->getCelda(i, j) == resultado[i][j]) {
          aciertos++;
        } else if (resultado[i][j] !=
                   '?') { // Puso un valor distinto de desconocido en
                          // mapaResultado y no acertó
          aciertos--;
        }
        totalCasillas++;
      }
    }
  }
  return (aciertos * 100.0 / totalCasillas);
}

/**
 * @brief Convierte una acción en su representación textual.
 *
 * @param accion  Acción a convertir.
 * @return Cadena con el nombre de la acción ("WALK", "JUMP", etc.).
 */
string MonitorJuego::strAccion(Action accion) {
  string out = "";

  switch (accion) {
  case WALK:
    out = "WALK";
    break;
  case JUMP:
    out = "JUMP";
    break;
  case TURN_SR:
    out = "TURN_SR";
    break;
  case PUSH:
    out = "PUSH";
    break;
  case COME:
    out = "COME";
    break;
  case IDLE:
    out = "IDLE";
    break;
  case INSTALL:
    out = "INSTALL";
    break;
  case DIG:
    out = "DIG";
    break;
  case RAISE:
    out = "RAISE";
    break;
  case TURN_SL:
    out = "TURN_SL";
    break;
  }

  return out;
}

/**
 * @brief Convierte una orientación en su nombre en español.
 *
 * @param x  Orientación a convertir.
 * @return Cadena descriptiva ("norte", "sureste", etc.).
 */
string paraDonde(const Orientacion &x) {
  string aux;
  switch (x) {
  case norte:
    aux = "norte";
    break;
  case noreste:
    aux = "noreste";
    break;
  case este:
    aux = "este ";
    break;
  case sureste:
    aux = "sureste ";
    break;
  case sur:
    aux = "sur  ";
    break;
  case suroeste:
    aux = "suroeste  ";
    break;
  case oeste:
    aux = "oeste";
    break;
  case noroeste:
    aux = "noroeste";
    break;
  }
  return aux;
}

/**
 * @brief Formatea un número entero con justificación a la derecha.
 *
 * @param numero   Valor a formatear.
 * @param longitud Ancho total del campo.
 * @return Cadena con el número justificado.
 */
std::string justificarDerechaNumero(int numero, int longitud) {
  std::stringstream ss;
  ss << std::setw(longitud) << std::right << numero;
  return ss.str();
}

/**
 * @brief Formatea una cadena de texto con justificación a la derecha.
 *
 * @param texto    Texto a formatear.
 * @param longitud Ancho total del campo.
 * @return Cadena justificada.
 */
std::string justificarDerechaCadena(std::string texto, int longitud) {
  std::stringstream ss;
  ss << std::setw(longitud) << std::right << texto;
  return ss.str();
}

/**
 * @brief Formatea un número real con justificación a la derecha.
 *
 * @param numero  Valor a formatear.
 * @param p       Ancho total del campo.
 * @return Cadena con el número formateado.
 */
std::string formatearReal(double numero, int p) {
  std::stringstream ss;
  ss << std::right << std::setw(p) << numero;
  return ss.str();
}

/**
 * @brief Genera una representación textual del estado actual de la
 *        simulación.
 *
 * Incluye tiempo restante, energía, posición, orientación, última
 * acción, tiempo de deliberación, estado de zapatillas, misiones,
 * puntuación y los vectores sensoriales de ambos agentes.
 *
 * @return Cadena multilínea con el resumen del estado.
 */
string MonitorJuego::toString() {
  string aux, aux1, saux;

  string str;

  aux = justificarDerechaNumero(get_entidad(0)->getInstantesPendientes(), 10);
  str += "Tiempo restante: " + aux + "\n";
  str += "                  ingeniero   |       tecnico\n";
  str += "Energia   " +
         justificarDerechaNumero(get_entidad(0)->getBateria(), 16) + "    | " +
         justificarDerechaNumero(get_entidad(1)->getBateria(), 16) + "\n";

  string paDondeR = paraDonde(get_entidad(0)->getOrientacion());
  string paDondeA = paraDonde(get_entidad(1)->getOrientacion());

  aux = "F:" + justificarDerechaNumero(get_entidad(0)->getFil(), 2) +
        " C:" + justificarDerechaNumero(get_entidad(0)->getCol(), 2) + " " +
        paDondeR;
  aux1 = "F:" + justificarDerechaNumero(get_entidad(1)->getFil(), 2) +
         " C:" + justificarDerechaNumero(get_entidad(1)->getCol(), 2) + " " +
         paDondeA;
  str += "Pos       " + justificarDerechaCadena(aux, 16) + " |" +
         justificarDerechaCadena(aux1, 16) + "\n";

  Action ac = get_entidad(0)->getLastAction();
  str += "Ultima Accion " + justificarDerechaCadena(strAccion(ac), 12);
  ac = get_entidad(1)->getLastAction();
  str += " | " + justificarDerechaCadena(strAccion(ac), 12) + "\n";

  str += "Tiempo        " +
         formatearReal(get_entidad(0)->getTiempo() / CLOCKS_PER_SEC, 13) +
         " | " +
         formatearReal(get_entidad(1)->getTiempo() / CLOCKS_PER_SEC, 12) + "\n";

  string saux2;
  if (get_entidad(0)->Has_Zapatillas()) {
    saux2 = " ZAPATILLAS ";
  } else {
    saux2 = "----------- ";
  }

  string saux3;
  if (get_entidad(1)->Has_Zapatillas()) {
    saux3 = " ZAPATILLAS ";
  } else {
    saux3 = "----------- ";
  }

  str += "     " + justificarDerechaCadena(saux2, 12) + " | " +
         justificarDerechaCadena(saux3, 12) + "\n";

  str +=
      "Misiones: " + justificarDerechaNumero(get_entidad(0)->getMisiones(), 3) +
      "  Puntuacion:  " +
      justificarDerechaNumero(get_entidad(0)->getPuntuacion(), 3) + "\n";

  vector<vector<unsigned char>> visionAux = getMapa()->vision(0);
  vector<vector<unsigned char>> visionAux1 = getMapa()->vision(1);

  if (visionAux.size() > 0) {
    // str += "************ Vision **************\n";

    // Vector Sensorial que contiene a los agentes
    for (unsigned int i = 0; i < visionAux[1].size(); i++) {
      str += visionAux[1][i];
    }

    str += " | ";

    for (unsigned int i = 0; i < visionAux1[1].size(); i++) {
      str += visionAux1[1][i];
    }
    str += "\n";

    // Vector Sensorial que contiene el terreno
    for (unsigned int i = 0; i < visionAux[0].size(); i++) {
      str += visionAux[0][i];
    }

    str += " | ";

    for (unsigned int i = 0; i < visionAux1[0].size(); i++) {
      str += visionAux1[0][i];
    }

    str += "\n";

    // Vector Sensorial que contiene la altura
    for (unsigned int i = 0; i < visionAux[0].size(); i++) {
      str += (char)(visionAux[2][i] + '0');
    }

    str += " | ";

    for (unsigned int i = 0; i < visionAux1[0].size(); i++) {
      str += (char)(visionAux1[2][i] + '0');
    }

    str += "\n";
  }

  return str;
}

/**
 * @brief Aplica los efectos de casillas especiales en las posiciones
 *        iniciales de los agentes.
 *
 * Si el ingeniero o el técnico comienzan en una casilla de zapatillas
 * ('D'), se les otorgan automáticamente.
 *
 * @param f       Fila inicial del ingeniero.
 * @param c       Columna inicial del ingeniero.
 * @param fcolab  Fila inicial del técnico.
 * @param ccolab  Columna inicial del técnico.
 */
void MonitorJuego::init_casillas_especiales(unsigned int f, unsigned int c,
                                            unsigned int fcolab,
                                            unsigned int ccolab) {
  unsigned char celda_inicial, celda_colab;
  celda_inicial = getMapa()->getCelda(f, c);
  celda_colab = getMapa()->getCelda(fcolab, ccolab);
  if (celda_inicial == 'D') {
    get_entidad(0)->Cogio_Zapatillas(true);
  }

  if (celda_colab == 'D') {
    get_entidad(1)->Cogio_Zapatillas(true);
  }
}

/**
 * @brief Vacía tanto la lista de objetivos pendientes como el vector
 *        de objetivos activos.
 */
void MonitorJuego::reset_objetivos() {
  if (!objetivos.empty()) {
    objetivos.clear();
  }
  if (!objetivosActivos.empty()) {
    objetivosActivos.clear();
  }
}

/**
 * @brief Comprueba si existe una conexión de tuberías desde la casilla
 *        de inicio hasta una casilla terminal ('U').
 *
 * Realiza una búsqueda en anchura (BFS) siguiendo las conexiones
 * codificadas en @c mapaTuberias (bits: 1 = N, 4 = E, 16 = S, 64 = O).
 *
 * @param startF  Fila de la casilla de inicio.
 * @param startC  Columna de la casilla de inicio.
 * @return true si se alcanza una casilla 'U' desde la de inicio.
 */
bool MonitorJuego::checkPipeConnection(int startF, int startC) {
  if (startF < 0 || startF >= (int)mapa->getNFils() || startC < 0 ||
      startC >= (int)mapa->getNCols())
    return false;

  std::queue<std::pair<int, int>> q;
  std::set<std::pair<int, int>> visited;

  q.push({startF, startC});
  visited.insert({startF, startC});

  while (!q.empty()) {
    std::pair<int, int> curr = q.front();
    q.pop();

    int f = curr.first;
    int c = curr.second;

    // Si la casilla actual es el destino FINAL ('U'), hemos tenido éxito.
    // Solo permitimos esto si NO es la casilla de inicio, o si la casilla de inicio
    // tiene al menos un bit de tubería puesto (para evitar victorias con 0 tuberías)
    if (mapa->getCelda(f, c) == 'U') {
      if (f != startF || c != startC || mapaTuberias[f][c] != 0) {
        return true;
      }
    }

    unsigned char mask = mapaTuberias[f][c];

    // Norte (bit 1) -> Sur (bit 16)
    if ((mask & 1) && f > 0) {
      int nf = f - 1, nc = c;
      if (visited.find({nf, nc}) == visited.end()) {
        if (mapa->getCelda(nf, nc) == 'U' || (mapaTuberias[nf][nc] & 16)) {
          visited.insert({nf, nc});
          q.push({nf, nc});
        }
      }
    }
    // Este (bit 4) -> Oeste (bit 64)
    if ((mask & 4) && c < (int)mapa->getNCols() - 1) {
      int nf = f, nc = c + 1;
      if (visited.find({nf, nc}) == visited.end()) {
        if (mapa->getCelda(nf, nc) == 'U' || (mapaTuberias[nf][nc] & 64)) {
          visited.insert({nf, nc});
          q.push({nf, nc});
        }
      }
    }
    // Sur (bit 16) -> Norte (bit 1)
    if ((mask & 16) && f < (int)mapa->getNFils() - 1) {
      int nf = f + 1, nc = c;
      if (visited.find({nf, nc}) == visited.end()) {
        if (mapa->getCelda(nf, nc) == 'U' || (mapaTuberias[nf][nc] & 1)) {
          visited.insert({nf, nc});
          q.push({nf, nc});
        }
      }
    }
    // Oeste (bit 64) -> Este (bit 4)
    if ((mask & 64) && c > 0) {
      int nf = f, nc = c - 1;
      if (visited.find({nf, nc}) == visited.end()) {
        if (mapa->getCelda(nf, nc) == 'U' || (mapaTuberias[nf][nc] & 4)) {
          visited.insert({nf, nc});
          q.push({nf, nc});
        }
      }
    }
  }

  return false;
}

/**
 * @brief Valida el plan de canalización propuesto por el ingeniero
 *        en el nivel 4.
 *
 * Comprueba que el plan no esté vacío, comience en la casilla del ingeniero,
 * que cada paso sea adyacente al anterior, que las alturas
 * sean consistentes (misma altura o descenso de 1) y que el plan
 * termine en una casilla de tipo 'U'.
 *
 * @return true si el plan es correcto; false con mensaje de error
 *         si hay alguna violación.
 */
bool MonitorJuego::checkLevel4() {
  Entidad *eng = entidades[0];
  ListaCasillasPlan plan = eng->getCanalizacionPlan();

  if (plan.empty()) {
    addMensaje("Sistema", "Error Nivel 4: El plan presentado está vacío.");
    return false;
  }

  // Comprobación de inicio en la casilla de la Belkanita (objetivo)
  int objF = -1, objC = -1;
  get_n_active_objetivo(0, objF, objC);
  auto it = plan.begin();
  if (it->fil != objF or it->col != objC) {
    stringstream ss;
    ss << "Error Nivel 4: El plan debe comenzar en la casilla de la Belkanita (" 
       << objF << "," << objC << ").";
    addMensaje("Sistema", ss.str());
    return false;
  }

  int last_h = (int)mapa->alturaEnCelda(it->fil, it->col) + it->op;
  int curr_f = it->fil;
  int curr_c = it->col;

  // Avanzar al segundo elemento para comprobar adyacencia y alturas
  auto it_check = it;
  ++it_check;

  for (; it_check != plan.end(); ++it_check) {
    int h = (int)mapa->alturaEnCelda(it_check->fil, it_check->col) + it_check->op;

    // Comprobación de adyacencia
    int dist = abs(it_check->fil - curr_f) + abs(it_check->col - curr_c);
    if (dist > 1) {
      stringstream ss;
      ss << "Error Nivel 4: Salto detectado en (" << it_check->fil << "," << it_check->col << ").";
      addMensaje("Sistema", ss.str());
      return false;
    }

    // Comprobación de consistencia de altura
    if (!(last_h == h || h == last_h - 1)) {
      stringstream ss;
      ss << "Error Nivel 4: Altura inconsistente en (" << it_check->fil << "," << it_check->col << ").";
      addMensaje("Sistema", ss.str());
      return false;
    }

    curr_f = it_check->fil;
    curr_c = it_check->col;
    last_h = h;
  }

  // Comprobar que termina en una casilla 'U'
  auto last_it = plan.end();
  --last_it;
  if (mapa->getCelda(last_it->fil, last_it->col) != 'U') {
    addMensaje("Sistema", "Error Nivel 4: El plan debe terminar en una Planta de Tratamiento ('U').");
    return false;
  }

  // Validación de impacto ecológico unificada
  int simulated_impact = calcularImpactoPlan(plan);
  if (simulated_impact > maxImpacto) {
    stringstream ss;
    ss << "Error Nivel 4: Se ha superado el impacto ecológico máximo permitido (" 
       << simulated_impact << " > " << maxImpacto << ").";
    addMensaje("Sistema", ss.str());
    return false;
  }

  return true;
}

/**
 * @brief Calcula el impacto ecológico total de un plan de canalización.
 * 
 * La lógica suma el coste de DIG/RAISE en cada punto y el coste de INSTALL
 * en ambos extremos de cada segmento de la tubería.
 * 
 * @param plan Plan de canalización a evaluar.
 * @return Impacto ecológico total calculado.
 */
int MonitorJuego::calcularImpactoPlan(const ListaCasillasPlan &plan) {
  int total_impact = 0;
  if (plan.empty()) return 0;

  auto it = plan.begin();
  // Primer punto
  unsigned char celda = mapa->getCelda(it->fil, it->col);
  if (it->op == -1) total_impact += getCosteEco(DIG,   celda);
  if (it->op ==  1) total_impact += getCosteEco(RAISE, celda);

  auto prev = it;
  ++it;
  for (; it != plan.end(); ++it) {
    celda = mapa->getCelda(it->fil, it->col);
    // Impacto de alteración del terreno en el punto actual
    if (it->op == -1) total_impact += getCosteEco(DIG,   celda);
    if (it->op ==  1) total_impact += getCosteEco(RAISE, celda);

    // Impacto de instalación entre el punto anterior y el actual (contabilizado en ambos)
    unsigned char celda_prev = mapa->getCelda(prev->fil, prev->col);
    total_impact += getCosteEco(INSTALL, celda_prev);
    total_impact += getCosteEco(INSTALL, celda);
    
    prev = it;
  }

  return total_impact;
}
/**
 * @brief Elimina los marcadores visuales de acción fallida en una celda.
 *
 * Borra de la lista de marcadores visuales aquellos de tipo 0
 * (cruz roja de fallo) que coincidan con la posición dada.
 *
 * @param f  Fila de la celda.
 * @param c  Columna de la celda.
 */
void MonitorJuego::clearFailedAction(int f, int c) {
  visualMarkers.erase(std::remove_if(visualMarkers.begin(), visualMarkers.end(),
                                     [f, c](const VisualMarker &vm) {
                                       return vm.f == f && vm.c == c &&
                                              vm.type == 0;
                                     }),
                      visualMarkers.end());
}

/**
 * @brief Devuelve el impacto ecológico de una acción sobre un tipo
 *        de celda.
 *
 * Los costes varían según la acción (INSTALL, RAISE, DIG) y el tipo
 * de terreno. Acciones no contempladas devuelven 0.
 *
 * @param accion  Acción ejecutada.
 * @param celda   Tipo de celda sobre la que se ejecuta.
 * @return Valor del impacto ecológico.
 */
int MonitorJuego::getCosteEco(Action accion, unsigned char celda) {
  switch (accion) {
    case INSTALL:
      if (celda == 'A') return 50;
      if (celda == 'H') return 45;
      if (celda == 'S') return 25;
      if (celda == 'C' || celda == 'U') return 15;
      return 30;
    case RAISE:
      if (celda == 'A') return 1000000;
      if (celda == 'H') return 55;
      if (celda == 'S') return 30;
      if (celda == 'C' || celda == 'U') return 10;
      return 40;
    case DIG:
      if (celda == 'A') return 1000000;
      if (celda == 'H') return 65;
      if (celda == 'S') return 40;
      if (celda == 'C' || celda == 'U') return 25;
      return 50;
    default:
      return 0;
  }
}
