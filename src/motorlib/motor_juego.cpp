#include "motorlib.hpp"

#include <unistd.h>

extern MonitorJuego monitor;

/**
 * @brief Procesa la acción elegida por el agente Ingeniero y aplica sus efectos
 *        sobre el mapa y el estado del juego.
 *
 * Comprueba primero si el ingeniero dispone de energía suficiente para ejecutar
 * la acción. Si no la tiene, la acción se cancela y la batería queda a 0.
 * En caso contrario, resuelve el movimiento (WALK, JUMP), los giros (TURN_SR,
 * TURN_SL), las interacciones cooperativas (INSTALL, COME) y las acciones de
 * modificación del terreno (DIG, RAISE). Gestiona colisiones con muros, árboles,
 * precipicios y otros agentes, así como las condiciones de éxito de nivel (llegar
 * al objetivo, completar misiones en nivel 7, etc.).
 *
 * @param celdaJ_inicial  Tipo de casilla sobre la que está el ingeniero antes de actuar.
 * @param celdaJ_fin      Tipo de casilla destino (la que se encuentra delante del ingeniero).
 * @param difAltJ         Diferencia de altura entre casilla destino y casilla origen
 *                        (positivo = destino más alto, negativo = destino más bajo).
 * @param accion          Acción elegida por el ingeniero (WALK, JUMP, TURN_SR, etc.).
 * @param x               Fila de la casilla destino en el mapa.
 * @param y               Columna de la casilla destino en el mapa.
 * @return true  si la acción se completó con éxito y el agente se movió o actuó.
 * @return false si hubo colisión, falta de energía u otro impedimento.
 */
bool actuacionIngeniero(unsigned char celdaJ_inicial, unsigned char celdaJ_fin,
                        int difAltJ, Action accion, unsigned int x,
                        unsigned int y) {
  int posibleElAvance;
  unsigned int mix, miy;
  unsigned char celdaRand;
  bool salida = false;
  int gasto;
  int error = 0; // 0 = NoError | 1 = NoEnergiaSuficiente | 2 = Colision

  // Primero evaluo si hay energía suficiente para realizar las acciones del
  // ingeniero.

  gasto = monitor.get_entidad(0)->fixBateria_sig_accion_jugador(
      celdaJ_inicial, difAltJ, accion);

  // std::cout << "Accion Ingeniero: " << accion << "   Accion Colaborador: " <<
  // accionColaborador << endl; std::cout << "Gasto Ingeniero: " <<
  // gasto_Ingeniero << "   Gasto Colaborador: " << gasto_colaborador << " Gasto
  // Total: " << gasto << endl;

  if (gasto > monitor.get_entidad(0)->getBateria()) {
    monitor.addMensaje("Ingeniero", "Falta energia");
    error = 1;
    monitor.get_entidad(0)->setBateria(0);

    return false;
  }

  switch (accion) {
  case WALK:
    if (monitor.getMapa()->casillaOcupada(0) == -1 and
        ((abs(difAltJ) <= 1) or
         (monitor.get_entidad(0)->Has_Zapatillas() and
          (abs(difAltJ) <= 2)))) // Casilla destino desocupada
                                 // monitor.get_entidad(0)->seAostio();
    {
      switch (celdaJ_fin) {
      case 'B': // Arbol
      case 'M': // Muro
        monitor.get_entidad(0)->seAostio();
        monitor.addCollisionMarker(x, y);
        if (celdaJ_fin == 'M')
          monitor.addMensaje("Ingeniero", "Choco con muro");
        else
          monitor.addMensaje("Ingeniero", "Choco con arbol");
        break;
      case 'P': // Precipicio
        monitor.addMensaje("Ingeniero", "Cayo al precipicio");
        monitor.get_entidad(0)->resetEntidad();
        monitor.get_entidad(0)->setHitbox(true);
        monitor.setResetActivado(true);
        monitor.finalizarJuego();
        monitor.setMostrarResultados(true);
        break;
      case 'X': // Casilla Rosa (Puesto Base)
        monitor.get_entidad(0)->setPosicion(x, y);
        salida = true;
        break;
      case 'D': // Casilla Morada (Zapatillas)
        // tomo la zapatilla
        monitor.get_entidad(0)->Cogio_Zapatillas(true);
        monitor.get_entidad(0)->setPosicion(x, y);
        salida = true;
        break;
      default:
        monitor.get_entidad(0)->setPosicion(x, y);
        salida = true;
        break;
      }
      if (monitor.getLevel() > 1) {
        if (monitor.getLevel() == 2 and
            monitor.get_entidad(0)->isMemberObjetivo(x, y) != -1) {
          // acaba de completar todos los objetivos.
          monitor.addMensaje("Ingeniero", "Objetivo alcanzado");
          // El ingeniero llegó a la casilla objetivo.
          monitor.get_entidad(0)->setCompletoLosObjetivos();
          monitor.get_entidad(0)->incrMisiones();
          monitor.get_entidad(0)->setFin(true);

          monitor.finalizarJuego();
          monitor.setMostrarResultados(true);
        } else if (monitor.getLevel() == 7 and
                   monitor.get_entidad(0)->isMemberObjetivo(x, y) != -1 ) {
          monitor.addMensaje("Ingeniero", "Objetivo alcanzado");
          monitor.get_entidad(0)->setCompletoLosObjetivos();
          monitor.put_active_objetivos(1);
          monitor.get_entidad(0)->anularAlcanzados();
          monitor.get_entidad(0)->incrMisiones();
          monitor.get_entidad(0)->incrPuntuacion(2);
          monitor.get_entidad(1)->AsignarCall_ON(
              false); // Se deja de llamar al tecnico

          for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
            monitor.get_entidad(i)->setObjetivos(
                monitor.get_active_objetivos());
          }
        } else if (monitor.getLevel() == 7 and
                   monitor.get_entidad(0)->isMemberObjetivo(x, y) != -1 and
                   monitor.CanHeSeesThisCell(
                       1, monitor.get_entidad(0)->getObjFil(0),
                       monitor.get_entidad(0)->getObjCol(0))) {
          monitor.put_active_objetivos(1);
          monitor.get_entidad(0)->anularAlcanzados();
          monitor.get_entidad(0)->incrMisiones();
          monitor.get_entidad(0)->incrPuntuacion(7);
          monitor.get_entidad(1)->AsignarCall_ON(
              false); // Se deja de llamar al tecnico

          for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
            monitor.get_entidad(i)->setObjetivos(
                monitor.get_active_objetivos());
          }
        }
      }
      // monitor.get_entidad(0)->fixBateria_sig_accion(celdaJ_inicial, accion);
    } else if (monitor.getMapa()->casillaOcupada(0) !=
               -1) // Choca contra otro agente
    {
      // Choca contra una entidad
      monitor.get_entidad(0)->seAostio();
      monitor.addCollisionMarker(x, y);
      if (monitor.get_entidad(monitor.getMapa()->casillaOcupada(0))
              ->getSubTipo() == excursionista) {
        monitor.get_entidad(0)->perderPV(0);
        monitor.addMensaje("Ingeniero", "Choco con excursionista");
      } else if (monitor.get_entidad(monitor.getMapa()->casillaOcupada(0))
                     ->getSubTipo() == tecnico) {
        monitor.get_entidad(0)->perderPV(0);
        monitor.addMensaje("Ingeniero", "Choco con tecnico");
      } else if (monitor.get_entidad(monitor.getMapa()->casillaOcupada(0))
                     ->getSubTipo() == vandalo) {
        // Opcion reiniciarse en otro punto del mapa
        /*do
        {
          mix = aleatorio(monitor.getMapa()->getNFils() - 1);
          miy = aleatorio(monitor.getMapa()->getNCols() - 1);

          celdaRand = monitor.getMapa()->getCelda(mix, miy);
        } while (celdaRand == 'P' or celdaRand == 'M' or
        monitor.getMapa()->entidadEnCelda(mix, miy) != '_');

        monitor.get_entidad(0)->resetEntidad();
        monitor.get_entidad(0)->setPosicion(mix, miy);
        monitor.get_entidad(0)->setOrientacion(static_cast<Orientacion>(aleatorio(7)));
        monitor.get_entidad(0)->setHitbox(true);
        monitor.get_entidad(0)->Cogio_Zapatillas(false);*/

        // Opcion simplemente choca contra el vandalo
        monitor.get_entidad(0)->perderPV(0);
        monitor.addMensaje("Ingeniero", "Choco con vandalo");
      }
      salida = false;
    } else // Choca o cae por la diferencia de altura entre casilla inicial y
           // final
    {
      if (difAltJ > 0) // Choca porque la casilla destino está demasiado alta.
      {
        monitor.get_entidad(0)->seAostio();
        monitor.addCollisionMarker(x, y);
        monitor.addMensaje("Ingeniero", "Choco (casilla alta)");
      } else // Se cae porque la casilla destino está demasiado baja.
      {
        monitor.addMensaje("Ingeniero", "Cayo al vacio (desnivel)");
        monitor.get_entidad(0)->resetEntidad();
        monitor.get_entidad(0)->setHitbox(true);
        monitor.setResetActivado(true);
        monitor.finalizarJuego();
        monitor.setMostrarResultados(true);
      }
    }
    break;

  case JUMP:
    posibleElAvance = monitor.getMapa()->EsPosibleCorrer(0, 2);

    if (posibleElAvance != 0) { // No se ha podido avanzar.
      int impactF, impactC;
      if (monitor.getMapa()->EsPosibleCorrer(0, 1) != 0) {
        pair<int, int> c1 = monitor.getMapa()->NCasillasDelante(0, 1);
        impactF = c1.first;
        impactC = c1.second;
      } else {
        impactF = x;
        impactC = y; // Already calculated at distance 2
      }

      switch (posibleElAvance) {
      case 1: // Muro
        monitor.get_entidad(0)->seAostio();
        monitor.addCollisionMarker(impactF, impactC);
        monitor.addMensaje("Ingeniero", "Choco con muro");
        break;

      case 6: // Arbol
        monitor.get_entidad(0)->seAostio();
        monitor.addCollisionMarker(impactF, impactC);
        monitor.addMensaje("Ingeniero", "Choco con arbol");
        break;

      case 2: // Precipicio
        monitor.addMensaje("Ingeniero", "Cayo al precipicio");
        monitor.get_entidad(0)->resetEntidad();
        monitor.get_entidad(0)->setHitbox(true);
        monitor.setResetActivado(true);
        monitor.finalizarJuego();
        monitor.setMostrarResultados(true);
        break;

      case 3: // tecnico
        monitor.get_entidad(0)->seAostio();
        monitor.addCollisionMarker(impactF, impactC);
        monitor.addMensaje("Ingeniero", "Choco con tecnico");
        break;

      case 4: // excursionista
        monitor.get_entidad(0)->seAostio();
        monitor.addCollisionMarker(impactF, impactC);
        monitor.addMensaje("Ingeniero", "Choco con excursionista");
        break;

      case 5: // vandalo
        monitor.get_entidad(0)->seAostio();
        monitor.addCollisionMarker(impactF, impactC);
        monitor.addMensaje("Ingeniero", "Choco con excursionista");
        break;
      }
    } else if (posibleElAvance == 0 and
               ((abs(difAltJ) <= 1) or
                (monitor.get_entidad(0)->Has_Zapatillas() and
                 (abs(difAltJ) <= 2)))) { // Es posible correr
      switch (celdaJ_fin) {
      case 'X': // Puesto Base (Recarga)
        // monitor.get_entidad(0)->increaseBateria(10);
        monitor.get_entidad(0)->setPosicion(x, y);
        salida = true;
        break;
      case 'D': // Casilla Morada (Zapatillas)
        // tomo la zapatilla
        monitor.get_entidad(0)->Cogio_Zapatillas(true);
        // pierdo el bikini
        monitor.get_entidad(0)->setPosicion(x, y);
        salida = true;
        break;
      case 'A': // Agua
      default:
        monitor.get_entidad(0)->setPosicion(x, y);
        salida = true;
        break;
      }
      if (monitor.getLevel() > 1) {
        if (monitor.getLevel() < 4 and
            monitor.get_entidad(0)->isMemberObjetivo(x, y) != -1) {
          // acaba de completar todos los objetivos.
          monitor.addMensaje("Ingeniero", "Objetivo alcanzado");
          // El ingeniero llegó a la casilla objetivo.
          monitor.get_entidad(0)->setCompletoLosObjetivos();
          monitor.get_entidad(0)->incrMisiones();
          if (monitor.getLevel() == 2) monitor.get_entidad(0)->setFin(true);

          monitor.finalizarJuego();
          monitor.setMostrarResultados(true);
        } else if (monitor.getLevel() == 7 and
                   monitor.get_entidad(0)->isMemberObjetivo(x, y) != -1 ) {
          monitor.addMensaje("Ingeniero", "Objetivo alcanzado");
          monitor.get_entidad(0)->setCompletoLosObjetivos();
          monitor.put_active_objetivos(1);
          monitor.get_entidad(0)->anularAlcanzados();
          monitor.get_entidad(0)->incrMisiones();
          monitor.get_entidad(0)->incrPuntuacion(2);
          monitor.get_entidad(1)->AsignarCall_ON(
              false); // Se deja de llamar al tecnico

          for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
            monitor.get_entidad(i)->setObjetivos(
                monitor.get_active_objetivos());
          }
        } else if (monitor.getLevel() == 7 and
                   monitor.get_entidad(0)->isMemberObjetivo(x, y) != -1 and
                   monitor.CanHeSeesThisCell(
                       1, monitor.get_entidad(0)->getObjFil(0),
                       monitor.get_entidad(0)->getObjCol(0))) {
          monitor.put_active_objetivos(1);
          monitor.get_entidad(0)->anularAlcanzados();
          monitor.get_entidad(0)->incrMisiones();
          monitor.get_entidad(0)->incrPuntuacion(7);
          monitor.get_entidad(1)->AsignarCall_ON(
              false); // Se deja de llamar al tecnico

          for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
            monitor.get_entidad(i)->setObjetivos(
                monitor.get_active_objetivos());
          }
        }
      }
    } else { // Se puede correr pero el desnivel de altura entre casillas es
             // demasiado grande.
      if (difAltJ > 0) // Choca porque la casilla destino está demasiado alta.
      {
        monitor.get_entidad(0)->seAostio();
        monitor.addCollisionMarker(x, y);
        monitor.addMensaje(
            "Ingeniero ha chocado (por diferencia de altura entre casillas)");
      } else // Se cae porque la casilla destino está demasiado baja.
      {
        monitor.addMensaje(
            "ERROR CRITICO: Ingeniero ha dado un salto al vacío. Demasiada "
            "diferencia de altura entre casillas\n");
        monitor.addMensaje("       FIN DE LA SIMULACION\n");
        monitor.get_entidad(0)->resetEntidad();
        monitor.get_entidad(0)->setHitbox(true);
        monitor.setResetActivado(true);
        monitor.finalizarJuego();
        monitor.setMostrarResultados(true);
      }
    }
    break;

  case TURN_SR:
    monitor.get_entidad(0)->giro45Dch();
    monitor.girarJugadorDerecha(45);
    // monitor.get_entidad(1)->SetActionSent(IDLE);
    if (monitor.getLevel() == 7 and
        monitor.get_entidad(0)->getFil() ==
            monitor.get_entidad(0)->getObjFil(0) and
        monitor.get_entidad(0)->getCol() ==
            monitor.get_entidad(0)->getObjCol(0) and
        monitor.CanHeSeesThisCell(1, monitor.get_entidad(0)->getObjFil(0),
                                  monitor.get_entidad(0)->getObjCol(0))) {
      monitor.put_active_objetivos(1);
      monitor.get_entidad(0)->anularAlcanzados();
      monitor.get_entidad(0)->incrMisiones();
      monitor.get_entidad(0)->incrPuntuacion(7);
      monitor.get_entidad(1)->AsignarCall_ON(
          false); // Se deja de llamar al tecnico

      for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
        monitor.get_entidad(i)->setObjetivos(monitor.get_active_objetivos());
      }
    }
    salida = false;
    break;

  case TURN_SL:
    monitor.get_entidad(0)->giro45Izq();
    monitor.girarJugadorIzquierda(45);
    // monitor.get_entidad(0)->fixBateria_sig_accion(celdaJ_inicial, accion);
    // monitor.get_entidad(1)->SetActionSent(IDLE);

    if (monitor.getLevel() == 7 and
        monitor.get_entidad(0)->getFil() ==
            monitor.get_entidad(0)->getObjFil(0) and
        monitor.get_entidad(0)->getCol() ==
            monitor.get_entidad(0)->getObjCol(0) and
        monitor.CanHeSeesThisCell(1, monitor.get_entidad(0)->getObjFil(0),
                                  monitor.get_entidad(0)->getObjCol(0))) {
      monitor.put_active_objetivos(1);
      monitor.get_entidad(0)->anularAlcanzados();
      monitor.get_entidad(0)->incrMisiones();
      monitor.get_entidad(0)->incrPuntuacion(7);
      monitor.get_entidad(1)->AsignarCall_ON(
          false); // Se deja de llamar al tecnico

      for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
        monitor.get_entidad(i)->setObjetivos(monitor.get_active_objetivos());
      }
    }

    salida = true;
    break;

  case PUSH:
    monitor.addMensaje("La acción PUSH no es aplicable al agente ingeniero\n");
    break;

  case INSTALL: {
    bool eng_enfrente = monitor.get_entidad(0)->getEnfrente();
    bool tec_enfrente = monitor.get_entidad(1)->getEnfrente();
    Action tec_action = monitor.get_entidad(1)->getLastAction();

    if (eng_enfrente && tec_enfrente && tec_action == INSTALL) {
      unsigned int fe = monitor.get_entidad(0)->getFil();
      unsigned int ce = monitor.get_entidad(0)->getCol();
      unsigned int ft = monitor.get_entidad(1)->getFil();
      unsigned int ct = monitor.get_entidad(1)->getCol();
      unsigned char he = monitor.getMapa()->alturaEnCelda(fe, ce);
      unsigned char ht = monitor.getMapa()->alturaEnCelda(ft, ct);

      if (he >= (int)ht - 1 && he <= ht) {
        unsigned char bit_e = 0, bit_t = 0;
        if (ft == fe - 1 && ct == ce) {
          bit_e = 1;
          bit_t = 16;
        } // El Técnico está al Norte
        else if (ft == fe + 1 && ct == ce) {
          bit_e = 16;
          bit_t = 1;
        } // El Técnico está al Sur
        else if (ft == fe && ct == ce + 1) {
          bit_e = 4;
          bit_t = 64;
        } // El Técnico está al Este
        else if (ft == fe && ct == ce - 1) {
          bit_e = 64;
          bit_t = 4;
        } // El Técnico está al Oeste

        if (bit_e != 0) {
          monitor.getMapaTuberias()[fe][ce] |= bit_e;
          monitor.getMapaTuberias()[ft][ct] |= bit_t;
          monitor.get_entidad(0)->getComportamiento()->mapaTuberias[fe][ce] |=
              bit_e;
          monitor.get_entidad(0)->getComportamiento()->mapaTuberias[ft][ct] |=
              bit_t;
          monitor.get_entidad(1)->getComportamiento()->mapaTuberias[fe][ce] |=
              bit_e;
          monitor.get_entidad(1)->getComportamiento()->mapaTuberias[ft][ct] |=
              bit_t;

          // Cálculo del impacto ecológico para INSTALL
          int impactoTotal =
              monitor.getCosteEco(INSTALL,
                                  monitor.getMapa()->getCelda(fe, ce)) +
              monitor.getCosteEco(INSTALL, monitor.getMapa()->getCelda(ft, ct));
          monitor.addImpactoEcologico(impactoTotal);

          salida = true;

          if (monitor.getLevel() == 5 or monitor.getLevel() == 6) {
            int startF = monitor.get_entidad(0)->getObjFil(0);
            int startC = monitor.get_entidad(0)->getObjCol(0);
            if (monitor.checkPipeConnection(startF, startC)) {
              monitor.get_entidad(0)->setCompletoLosObjetivos();
              monitor.setMostrarResultados(true);
              monitor.finalizarJuego();
            }
          }

          monitor.clearFailedAction(fe, ce);
          monitor.clearFailedAction(ft, ct);
        }
      } else {
        monitor.addMensaje("INSTALL: Alturas erroneas");
        monitor.addFailedAction(fe, ce);
        monitor.addFailedAction(ft, ct);
      }
    } else {
      monitor.addMensaje("INSTALL: Falta coord/no enfrentados");
      monitor.addFailedAction(monitor.get_entidad(0)->getFil(),
                              monitor.get_entidad(0)->getCol());
      // También marcar al técnico si al menos están enfrentados pero la acción
      // es errónea
      if (eng_enfrente && tec_enfrente) {
        monitor.addFailedAction(monitor.get_entidad(1)->getFil(),
                                monitor.get_entidad(1)->getCol());
      }
    }
  } break;

  case COME:
    monitor.get_entidad(1)->AsignarCall_ON(true);
    monitor.setDelayCallOn(1);
    monitor.get_entidad(1)->setGoto(monitor.get_entidad(0)->getFil(),
                                    monitor.get_entidad(0)->getCol());
    break;

  case IDLE:
    if (celdaJ_inicial == 'X' and monitor.getLevel() == 7 or
        monitor.getLevel() == 1) { // Casilla Rosa (Recarga)
      monitor.get_entidad(0)->increaseBateria(10);
    }
    // monitor.get_entidad(0)->fixBateria_sig_accion(celdaJ_inicial, accion);
    // monitor.get_entidad(1)->SetActionSent(IDLE);

    if (monitor.getLevel() == 7 and
        monitor.get_entidad(0)->getFil() ==
            monitor.get_entidad(0)->getObjFil(0) and
        monitor.get_entidad(0)->getCol() ==
            monitor.get_entidad(0)->getObjCol(0) and
        monitor.CanHeSeesThisCell(1, monitor.get_entidad(0)->getObjFil(0),
                                  monitor.get_entidad(0)->getObjCol(0))) {
      monitor.put_active_objetivos(1);
      monitor.get_entidad(0)->anularAlcanzados();
      monitor.get_entidad(0)->incrMisiones();
      monitor.get_entidad(0)->incrPuntuacion(7);
      monitor.get_entidad(1)->AsignarCall_ON(
          false); // Se deja de llamar al tecnico
      for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
        monitor.get_entidad(i)->setObjetivos(monitor.get_active_objetivos());
      }
    }

    salida = true;
    break;

  case DIG: {
    unsigned int f = monitor.get_entidad(0)->getFil();
    unsigned int c = monitor.get_entidad(0)->getCol();
    unsigned char alt = monitor.getMapa()->alturaEnCelda(f, c);
    unsigned char celda = monitor.getMapa()->getCelda(f, c);
    if (celda == 'A') {
      monitor.addMensaje("Ingeniero", "DIG: No aplicable sobre agua");
    } else if (alt > 1) {
      monitor.getMapa()->setAltura(f, c, alt - 1);
      monitor.get_entidad(0)->getComportamiento()->mapaCotas[f][c] = alt - 1;
      // Añadir el impacto ecológico de la acción DIG
      monitor.addImpactoEcologico(
          monitor.getCosteEco(DIG, monitor.getMapa()->getCelda(f, c)));
    }
  }
    salida = true;
    break;

  case RAISE: {
    unsigned int f = monitor.get_entidad(0)->getFil();
    unsigned int c = monitor.get_entidad(0)->getCol();
    unsigned char alt = monitor.getMapa()->alturaEnCelda(f, c);
    unsigned char celda = monitor.getMapa()->getCelda(f, c);
    if (celda == 'A') {
      monitor.addMensaje("Ingeniero", "RAISE: No aplicable sobre agua");
    } else if (alt < 9) {
      monitor.getMapa()->setAltura(f, c, alt + 1);
      monitor.get_entidad(0)->getComportamiento()->mapaCotas[f][c] = alt + 1;

      monitor.addImpactoEcologico(
          monitor.getCosteEco(RAISE, monitor.getMapa()->getCelda(f, c)));
    }
  }
    salida = true;
    break;

    break;
  }

  return salida;
}

/**
 * @brief Procesa la acción elegida por el agente Técnico y aplica sus efectos
 *        sobre el mapa y el estado del juego.
 *
 * Análoga a @ref actuacionIngeniero pero con las reglas propias del técnico:
 * puede atravesar árboles si lleva zapatillas, no puede hacer JUMP ni PUSH,
 * y su condición de éxito en nivel 3 es llegar a su casilla objetivo.
 * También gestiona colisiones, diferencias de altura y el reporte de
 * mensajes al monitor.
 *
 * @param celdaJ_inicial  Tipo de casilla sobre la que está el técnico antes de actuar.
 * @param celdaJ_fin      Tipo de casilla destino.
 * @param difAltJ         Diferencia de altura entre casilla destino y origen.
 * @param accion          Acción elegida por el técnico.
 * @param x               Fila de la casilla destino en el mapa.
 * @param y               Columna de la casilla destino en el mapa.
 * @return true  si la acción se completó con éxito.
 * @return false si hubo colisión, falta de energía u otro impedimento.
 */
bool actuacionTecnico(unsigned char celdaJ_inicial, unsigned char celdaJ_fin,
                      int difAltJ, Action accion, unsigned int x,
                      unsigned int y) {
  int posibleElAvance;
  unsigned int mix, miy;
  unsigned char celdaRand;
  bool salida = false;
  int gasto;
  int error = 0; // 0 = NoError | 1 = NoEnergiaSuficiente | 2 = Colision

  // Primero evaluo si hay energía suficiente para realizar las acciones del
  // ingeniero.

  gasto = monitor.get_entidad(1)->fixBateria_sig_accion_jugador(
      celdaJ_inicial, difAltJ, accion);

  // std::cout << "Accion Ingeniero: " << accion << "   Accion Colaborador: " <<
  // accionColaborador << endl; std::cout << "Gasto Ingeniero: " <<
  // gasto_Ingeniero << "   Gasto Colaborador: " << gasto_colaborador << " Gasto
  // Total: " << gasto << endl;

  if (gasto > monitor.get_entidad(1)->getBateria()) {
    monitor.addMensaje("Tecnico", "Falta energia");
    error = 1;
    monitor.get_entidad(1)->setBateria(0);

    return false;
  }

  switch (accion) {
  case WALK:
    if (monitor.getMapa()->casillaOcupada(1) == -1 and
        abs(difAltJ) <= 1) // Casilla destino desocupada
    {
      switch (celdaJ_fin) {
      case 'M': // Muro
        monitor.get_entidad(1)->seAostio();
        monitor.addCollisionMarker(x, y);
        monitor.addMensaje("Tecnico", "Choco con muro");
        break;
      case 'B': // Arbol
        if (!monitor.get_entidad(1)->Has_Zapatillas()) {
          monitor.get_entidad(1)->seAostio();
          monitor.addCollisionMarker(x, y);
          monitor.addMensaje("Tecnico", "Choco con arbol");
        } else {
          monitor.get_entidad(1)->setPosicion(x, y);
          salida = true;
        }
        break;
      case 'P': // Precipicio
        monitor.addMensaje("Tecnico", "Cayo al precipicio");
        monitor.get_entidad(1)->resetEntidad();
        monitor.get_entidad(1)->setHitbox(true);
        monitor.setResetActivado(true);
        monitor.finalizarJuego();
        monitor.setMostrarResultados(true);
        break;
      case 'X': // Casilla Rosa (Puesto Base)
        monitor.get_entidad(1)->setPosicion(x, y);
        salida = true;
        break;
      case 'D': // Casilla Morada (Zapatillas)
        // tomo la zapatilla
        monitor.get_entidad(1)->Cogio_Zapatillas(true);
        monitor.get_entidad(1)->setPosicion(x, y);
        salida = true;
        break;
      default:
        monitor.get_entidad(1)->setPosicion(x, y);
        salida = true;
        break;
      }
      if (monitor.get_entidad(1)->isMemberObjetivo(x, y) != -1 and
          monitor.get_entidad(1)->allLessOneObjetivosAlcanzados()) {
        if (monitor.getLevel() == 3) {
          // El tecnico llegó a la casilla objetivo.
          monitor.addMensaje("Tecnico", "Objetivo alcanzado");
          monitor.get_entidad(1)->setCompletoLosObjetivos();
          monitor.get_entidad(1)->setFin(true);
          monitor.finalizarJuego();
          monitor.setMostrarResultados(true);
        } else if (monitor.getLevel() == 2 or monitor.getLevel() == 7) {
          monitor.addMensaje("Tecnico", "Info: Ing debe llegar al obj");
        }
      }
      // monitor.get_entidad(0)->fixBateria_sig_accion(celdaJ_inicial, accion);
    } else if (abs(difAltJ) > 1) {
      std::cout
          << "Demasiada altura entre las casillas\n"; // Hay demasiada altura
                                                      // entre casillas
      if (difAltJ > 0) // Choca porque la casilla destino está demasiado alta.
      {
        monitor.get_entidad(1)->seAostio();
        monitor.addCollisionMarker(x, y);
        monitor.addMensaje("Tecnico", "Choco (casilla alta)");
      } else // Se cae porque la casilla destino está demasiado baja.
      {
        monitor.addMensaje("Tecnico", "Cayo al vacio (desnivel)");
        monitor.get_entidad(1)->resetEntidad();
        monitor.get_entidad(1)->setHitbox(true);
        monitor.setResetActivado(true);
        monitor.finalizarJuego();
        monitor.setMostrarResultados(true);
      }
    } else if (monitor.getMapa()->casillaOcupada(1) !=
               -1) // Choca contra otro agente
    {
      // Choca contra una entidad
      monitor.get_entidad(1)->seAostio();
      monitor.addCollisionMarker(x, y);
      if (monitor.get_entidad(monitor.getMapa()->casillaOcupada(1))
              ->getSubTipo() == excursionista) {
        monitor.get_entidad(1)->perderPV(0);
        monitor.addMensaje("Tecnico", "Choco con excursionista");
      } else if (monitor.get_entidad(monitor.getMapa()->casillaOcupada(1))
                     ->getTipo() == jugador) {
        monitor.get_entidad(1)->perderPV(0);
        monitor.addMensaje("Tecnico", "Choco con ingeniero");
      } else if (monitor.get_entidad(monitor.getMapa()->casillaOcupada(1))
                     ->getSubTipo() == vandalo) {
        // Opcion reiniciarse en otro punto del mapa
        /*do
        {
          mix = aleatorio(monitor.getMapa()->getNFils() - 1);
          miy = aleatorio(monitor.getMapa()->getNCols() - 1);

          celdaRand = monitor.getMapa()->getCelda(mix, miy);
        } while (celdaRand == 'P' or celdaRand == 'M' or
        monitor.getMapa()->entidadEnCelda(mix, miy) != '_');

        monitor.get_entidad(1)->resetEntidad();
        monitor.get_entidad(1)->setPosicion(mix, miy);
        monitor.get_entidad(1)->setOrientacion(static_cast<Orientacion>(aleatorio(7)));
        monitor.get_entidad(1)->setHitbox(true);
        monitor.get_entidad(1)->Cogio_Zapatillas(false);*/

        // Opcion simplemente choca contra el vandalo
        monitor.get_entidad(1)->perderPV(0);
        monitor.addMensaje("Tecnico", "Choco con vandalo");
      }
      salida = false;
    }

    break;

  case JUMP:
    monitor.addMensaje("Tecnico", "Info: Accion JUMP no aplicable");
    break;

  case TURN_SR:
    monitor.get_entidad(1)->giro45Dch();
    monitor.girarJugadorDerecha(45);
    salida = false;
    break;

  case TURN_SL:
    monitor.get_entidad(1)->giro45Izq();
    monitor.girarJugadorIzquierda(45);
    salida = false;
    break;

  case PUSH:
    monitor.addMensaje("Tecnico", "Info: Accion PUSH no aplicable");
    break;

  case COME:
    monitor.addMensaje("Tecnico", "Info: Accion COME no aplicable");
    break;

    break;

  case INSTALL:
    salida = true;
    break;

  case DIG:
    monitor.addMensaje("Tecnico", "Info: Accion DIG no aplicable");
    break;

  case RAISE:
    monitor.addMensaje("Tecnico", "Info: Accion RAISE no aplicable");
    break;

  case IDLE:
    if (celdaJ_inicial == 'X' and
        (monitor.getLevel() == 1 or
         monitor.getLevel() == 7)) { // Casilla Rosa (Recarga)
      monitor.get_entidad(1)->increaseBateria(10);
    }

    salida = true;
    break;
  }

  return salida;
}

/**
 * @brief Procesa la acción de una entidad no jugadora (NPC).
 *
 * Gestiona el comportamiento de los NPC del juego. Los excursionistas solo
 * pueden caminar (WALK) y girar (TURN_SR / TURN_SL). Los vándalos tienen las
 * mismas capacidades de movimiento y además pueden empujar al Ingeniero mediante
 * la acción PUSH, que se aplica con un 50 % de probabilidad si la casilla
 * detrás del Ingeniero está libre.
 *
 * @param entidad  Índice de la entidad NPC en la lista de entidades del monitor.
 * @param celda    Tipo de casilla destino (la que está frente al NPC).
 * @param accion   Acción a ejecutar.
 * @param x        Fila de la casilla destino.
 * @param y        Columna de la casilla destino.
 * @return true  si la acción se realizó con éxito.
 * @return false en caso contrario.
 */
bool actuacionNPC(unsigned int entidad, unsigned char celda, Action accion,
                  unsigned int x, unsigned int y) {
  bool out = false;

  // Lógica común a excursionista y vándalo para WALK y giros
  auto accionesComunes = [&]() {
    switch (accion) {
    case WALK:
      if (celda != 'P' && celda != 'M' &&
          monitor.getMapa()->casillaOcupada(entidad) == -1) {
        monitor.get_entidad(entidad)->setPosicion(x, y);
        out = true;
      }
      break;
    case TURN_SR:
      monitor.get_entidad(entidad)->giro45Dch();
      out = true;
      break;
    case TURN_SL:
      monitor.get_entidad(entidad)->giro45Izq();
      out = true;
      break;
    default:
      break;
    }
  };

  switch (monitor.get_entidad(entidad)->getSubTipo()) {

  case excursionista:
    accionesComunes();
    break;

  case vandalo:
    if (accion == PUSH) {
      // El vándalo empuja al Ingeniero si lo tiene delante
      monitor.addMensaje("Vandalo empujo a Ingeniero");
      bool ingeniero_delante = (monitor.getMapa()->casillaOcupada(entidad) == 0);
      monitor.get_entidad(0)->seAostio();
      monitor.addCollisionMarker(monitor.get_entidad(0)->getFil(),
                                 monitor.get_entidad(0)->getCol());
      if (ingeniero_delante) {
        pair<int, int> casilla = monitor.getMapa()->NCasillasDelante(entidad, 2);
        bool casilla_libre =
            monitor.getMapa()->QuienEnCasilla(casilla.first, casilla.second) == -1 &&
            monitor.getMapa()->getCelda(casilla.first, casilla.second) != 'M' &&
            monitor.getMapa()->getCelda(casilla.first, casilla.second) != 'P' &&
            monitor.getMapa()->getCelda(casilla.first, casilla.second) != 'B';
        if (casilla_libre && aleatorio(1) == 0) { // Solo ocurre la mitad de las veces
          std::cout << "\tEl empujón ha sido efectivo\n";
          monitor.get_entidad(0)->setPosicion(casilla.first, casilla.second);
          monitor.get_entidad(0)->Increment_Empujones();
          monitor.get_entidad(entidad)->giroIzq();
          monitor.get_entidad(entidad)->giro45Izq();
        }
      }
      out = true;
    } else {
      accionesComunes();
    }
    break;

  default:
    break;
  }

  return out;
}

/**
 * @brief Calcula las coordenadas de la casilla adyacente en la dirección indicada.
 *
 * Dado un punto (f, c) en el mapa y una orientación de brújula, devuelve las
 * coordenadas del vecino inmediato en esa dirección. Se utiliza para determinar
 * qué casilla tiene una entidad justo delante de sí.
 *
 * @param f       Fila del punto de partida.
 * @param c       Columna del punto de partida.
 * @param brujula Orientación de la brújula (norte, noreste, este, …, noroeste).
 * @return pair<int,int> con {fila, columna} de la casilla vecina.
 */
pair<int, int> NextCoordenadas(int f, int c, Orientacion brujula) {
  switch (brujula) {
  case norte:    f--; break;
  case noreste:  f--; c++; break;
  case este:          c++; break;
  case sureste:  f++; c++; break;
  case sur:      f++; break;
  case suroeste: f++; c--; break;
  case oeste:         c--; break;
  case noroeste: f--; c--; break;
  }
  return {f, c};
}

/**
 * @brief Despacha la acción de una entidad hacia su función de actuación específica.
 *
 * Determina la casilla actual y la casilla destino de la entidad, calcula la
 * diferencia de altura entre ambas y llama a @ref actuacionIngeniero,
 * @ref actuacionTecnico o @ref actuacionNPC según el subtipo de la entidad.
 * Para el Ingeniero con acción JUMP la casilla destino se calcula a dos pasos.
 *
 * @param entidad  Índice de la entidad en la lista del monitor.
 * @param accion   Acción que la entidad desea ejecutar.
 * @return true  si la acción fue ejecutada con éxito.
 * @return false si no pudo realizarse (colisión, falta de energía, etc.).
 */
bool actuacion(unsigned int entidad, Action accion) {
  unsigned int f = monitor.get_entidad(entidad)->getFil();
  unsigned int c = monitor.get_entidad(entidad)->getCol();

  unsigned char celda_inicial  = monitor.getMapa()->getCelda(f, c);
  unsigned char altura_inicial = monitor.getMapa()->alturaEnCelda(f, c);

  // Casilla destino: un paso adelante (dos para JUMP del Ingeniero)
  pair<unsigned int, unsigned int> coord =
      NextCoordenadas(f, c, monitor.get_entidad(entidad)->getOrientacion());
  if (accion == JUMP && monitor.get_entidad(entidad)->getSubTipo() == ingeniero)
    coord = NextCoordenadas(coord.first, coord.second,
                            monitor.get_entidad(entidad)->getOrientacion());
  f = coord.first;
  c = coord.second;

  unsigned char celda_fin  = monitor.getMapa()->getCelda(f, c);
  unsigned char altura_fin = monitor.getMapa()->alturaEnCelda(f, c);
  int           difAlt     = static_cast<int>(altura_fin) - static_cast<int>(altura_inicial);

  switch (monitor.get_entidad(entidad)->getSubTipo()) {
  case ingeniero:    return actuacionIngeniero(celda_inicial, celda_fin, difAlt, accion, f, c);
  case tecnico:      return actuacionTecnico  (celda_inicial, celda_fin, difAlt, accion, f, c);
  case excursionista:
  case vandalo:      return actuacionNPC(entidad, celda_fin, accion, f, c);
  default:           return false;
  }
}

/**
 * @brief Ejecuta un ciclo completo de simulación (un instante de juego).
 *
 * Implementa el bucle principal de la simulación Belkan siguiendo el orden:
 *   1. Preparar la visión de cada agente y detectar si están enfrentados.
 *   2. Pensar: cada entidad llama a su función @c think() para decidir su acción.
 *   3. Actuar: cada entidad ejecuta su acción mediante @ref actuacion.
 *   4. Actualizar sensores, decrementar batería y contadores de pasos.
 *   5. Comprobar las condiciones de fin de nivel y fijar @c mostrarResultados
 *      cuando el juego ha terminado.
 *
 * @param monitor  Referencia al monitor global del juego.
 * @param acc      Parámetro de acción manual pasado en modo paso a paso
 *                 (interfaz gráfica). Valor -1 en modo automático.
 */
void nucleo_motor_juego(MonitorJuego &monitor, int acc) {
  vector<vector<vector<unsigned char>>> estado;

  // ── Preparar visión de todos los agentes ─────────────────────────────────
  for (unsigned int i = 0; i < monitor.numero_entidades(); i++)
    monitor.get_entidad(i)->setEnfrente(false);

  // Detectar si los dos agentes se miran en casillas adyacentes ortogonales
  if (monitor.getMapa()->vision(0)[1][2] == 't' &&
      monitor.getMapa()->vision(1)[1][2] == 'i') {
    monitor.get_entidad(0)->setEnfrente(true);
    monitor.get_entidad(1)->setEnfrente(true);
  }

  for (unsigned int i = 0; i < monitor.numero_entidades(); i++)
    estado.push_back(monitor.getMapa()->vision(i));

  // ── 1. Pensar: cada entidad decide su acción ──────────────────────────────
  for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
    Action accion;
    if (i == 0) {
      if (monitor.get_entidad(0)->ready()) {
        monitor.init_casillas_especiales(
            monitor.get_entidad(0)->getFil(), monitor.get_entidad(0)->getCol(),
            monitor.get_entidad(1)->getFil(), monitor.get_entidad(1)->getCol());
        clock_t t0 = clock();
        accion = monitor.get_entidad(0)->think(acc, estado[0], monitor.getLevel(), monitor.getImpactoEcologico(), monitor.getMaxImpacto());
        clock_t t1 = clock();
        monitor.get_entidad(0)->addTiempo(t1 - t0);
        monitor.get_entidad(0)->setLastAction(accion);
      } else {
        monitor.get_entidad(0)->setLastAction(IDLE);
      }
    } else {
      clock_t t0 = clock();
      accion = monitor.get_entidad(i)->think(acc, estado[i], monitor.getLevel(), monitor.getImpactoEcologico(), monitor.getMaxImpacto());
      clock_t t1 = clock();
      monitor.get_entidad(i)->addTiempo(t1 - t0);
      monitor.get_entidad(i)->setLastAction(accion);
    }
  }

  // ── 2. Actuar: cada entidad ejecuta su acción ────────────────────────────
  for (unsigned int i = 0; i < monitor.numero_entidades(); i++) {
    if (i == 0) {
      if (monitor.get_entidad(0)->ready())
        actuacion(0, monitor.get_entidad(0)->getLastAction());
      else
        monitor.get_entidad(0)->perderPV(0);
    } else {
      actuacion(i, monitor.get_entidad(i)->getLastAction());
    }
  }

  // ── 3. Actualizar sensores y contadores ──────────────────────────────────
  for (unsigned int i = 0; i < monitor.numero_entidades(); i++)
    monitor.get_entidad(i)->setVision(monitor.getMapa()->vision(i));

  monitor.get_entidad(0)->decBateria_sig_accion();
  monitor.get_entidad(1)->decBateria_sig_accion();

  if (monitor.AnularCallOn()) {
    monitor.get_entidad(1)->AsignarCall_ON(false);
    monitor.decCallON();
  } else {
    monitor.decCallON();
    cout << "Se mantiene activo el COME al Tech\n";
  }

  monitor.decPasos();

  // ── 4. Verificar condiciones de fin de nivel ──────────────────────────────
  if (monitor.get_entidad(0)->fin() || monitor.get_entidad(1)->fin())
    return;

  switch (monitor.getLevel()) {

  case 0: { // Dos agentes deben llegar juntos a su Planta de Tratamiento de Residuos ('U')
    unsigned char celdaI = monitor.getMapa()->getCelda(
        monitor.get_entidad(0)->getFil(), monitor.get_entidad(0)->getCol());
    unsigned char celdaT = monitor.getMapa()->getCelda(
        monitor.get_entidad(1)->getFil(), monitor.get_entidad(1)->getCol());

    // Registro de pisada / abandono de Planta de Tratamiento de Residuos — Ingeniero
    if (celdaI == 'U' && !monitor.getRecargaPisadaIngeniero()) {
      monitor.setRecargaPisadaIngeniero(true);
      monitor.setRecargaAbandonadaIngeniero(false);
      monitor.addMensaje("Ingeniero", "Planta de Tratamiento de Residuos alcanzada");
    } else if (celdaI != 'U' && monitor.getRecargaPisadaIngeniero() &&
               !monitor.getRecargaAbandonadaIngeniero()) {
      monitor.setRecargaAbandonadaIngeniero(true);
      monitor.setRecargaPisadaIngeniero(false);
      monitor.addMensaje("Ingeniero", "Planta de Tratamiento de Residuos abandonada");
    }

    // Registro de pisada / abandono de Planta de Tratamiento de Residuos — Técnico
    if (celdaT == 'U' && !monitor.getRecargaPisadaTecnico()) {
      monitor.setRecargaPisadaTecnico(true);
      monitor.setRecargaAbandonadaTecnico(false);
      monitor.addMensaje("Tecnico", "Planta de Tratamiento de Residuos alcanzada");
    } else if (celdaT != 'U' && monitor.getRecargaPisadaTecnico() &&
               !monitor.getRecargaAbandonadaTecnico()) {
      monitor.setRecargaAbandonadaTecnico(true);
      monitor.setRecargaPisadaTecnico(false);
      monitor.addMensaje("Tecnico", "Planta de Tratamiento de Residuos abandonada");
    }

    // Aviso de salida de camino — Ingeniero
    if (celdaI != 'U' && celdaI != 'D' && celdaI != 'C' &&
        !monitor.getCaminoAbandonadoIngeniero()) {
      monitor.setCaminoAbandonadoIngeniero(true);
      monitor.addMensaje("Ingeniero", "Aviso: Fuera de camino");
    }

    // Aviso de salida de camino — Técnico
    bool tecnicoFueraCamino = (celdaT != 'U' && celdaT != 'D' && celdaT != 'C' &&
        (!monitor.get_entidad(1)->Has_Zapatillas() || celdaT != 'B'));
    if (tecnicoFueraCamino && !monitor.getCaminoAbandonadoTecnico()) {
      monitor.setCaminoAbandonadoTecnico(true);
      monitor.addMensaje("Tecnico", "Aviso: Fuera de camino");
    }

    // Condición de éxito: ambos en su Planta de Tratamiento de Residuos simultáneamente
    if (celdaI == 'U' && celdaT == 'U') {
      monitor.addMensaje("Sistema", "Mision completada: Planta de Tratamiento de Residuos");
      if (monitor.getRecargaPisadaIngeniero())
        monitor.addMensaje("Ingeniero", "Llego a planta de tratamiento de residuos");
      if (monitor.getRecargaPisadaTecnico())
        monitor.addMensaje("Tecnico", "Llego a planta de tratamiento de residuos");
      if (monitor.getRecargaAbandonadaIngeniero())
        monitor.addMensaje("Ingeniero", "Abandono planta de tratamiento de residuos");
      if (monitor.getRecargaAbandonadaTecnico())
        monitor.addMensaje("Tecnico", "Abandono planta de tratamiento de residuos");
      if (monitor.getCaminoAbandonadoIngeniero())
        monitor.addMensaje("Ingeniero", "Aviso: Fuera de camino");
      if (monitor.getCaminoAbandonadoTecnico())
        monitor.addMensaje("Tecnico", "Aviso: Fuera de camino");

      monitor.finalizarJuego();
      monitor.setMostrarResultados(true);
    }
    break;
  }

  case 1: // Termina cuando se agotan ciclos, tiempo o energía
    if (monitor.finJuego())
      monitor.setMostrarResultados(true);
    break;
  case 2: // Termina cuando el Ingeniero llega al objetivo
  case 3: // Termina cuando el Técnico llega al objetivo
    break;

  case 4: // Termina cuando el Ingeniero presenta un plan de tuberías
    if (monitor.get_entidad(0)->getCanalizacionPlan().size() > 0) {
      if (monitor.checkLevel4()) {
        monitor.addMensaje("Sistema", "¡Nivel 4 completado con Exito!");
        monitor.get_entidad(0)->setCompletoLosObjetivos();
        monitor.get_entidad(0)->setFin(true);
      } else {
        monitor.addMensaje("Sistema", "Error Nivel 4: El plan presentado no es válido.");
        monitor.get_entidad(0)->setFin(false);
      }
      monitor.finalizarJuego();
      monitor.setMostrarResultados(true);
    }
    break;

  case 5: // Termina cuando se agotan ciclos, tiempo o energía
  case 6: // Mismo comportamiento de finalización que el Nivel 5
    if (monitor.finJuego())
      monitor.setMostrarResultados(true);
    break;
  }
}



/**
 * @brief Muestra por consola y registra en el monitor los resultados finales
 *        de la simulación, adaptados al nivel en curso.
 *
 * El contenido de la salida varía según el nivel:
 *  - **Niveles 0–1**: resultado, energías, instantes consumidos, tiempo y
 *    porcentaje de mapa descubierto.
 *  - **Nivel 2**: resultado, longitud de camino e energía del Ingeniero, tiempo.
 *  - **Nivel 3**: resultado, longitud de camino y energía del Técnico, tiempo.
 *  - **Nivel 4**: resultado, longitud de red de tuberías planificada e impacto
 *    ecológico simulado a partir del plan.
 *  - **Niveles 5–6**: resultado, instantes, energías, impacto ecológico y
 *    longitud real de la red de tuberías instalada.
 *  - **Nivel 7 y siguientes**: resultado, instantes restantes, tiempo, energías,
 *    colisiones, porcentaje de mapa, misiones/puntuación e impacto ecológico.
 *
 * Al terminar, desactiva el flag @c mostrarResultados del monitor.
 *
 * @param monitor  Referencia al monitor global del juego.
 */
void ImprimirResultadosJuego(MonitorJuego &monitor) {
  stringstream ss;
  int nivel = monitor.getLevel();

  // Macro local para imprimir una línea en consola y añadirla al monitor
  auto imprime = [&](const string &msg) {
    monitor.addMensaje("Sistema", msg);
    cout << msg << endl;
  };
  auto flush = [&]() { imprime(ss.str()); ss.str(""); };

  switch (nivel) {

  // ── Niveles 0 y 1: Reconocimiento ─────────────────────────────────────────
  case 0:
  case 1: {
    bool exito;
    if (nivel == 0) {
      exito = monitor.get_entidad(0)->vivo() &&
              monitor.get_entidad(1)->vivo() &&
              monitor.get_entidad(0)->getBateria() > 0 &&
              monitor.get_entidad(1)->getBateria() > 0;
    } else {
      exito = !monitor.getResetActivado();
    }
    ss << "Nivel " << nivel << (exito ? " completado con Exito!" : " NO completado.");
    flush();
    ss << "Energia Ingeniero: " << monitor.get_entidad(0)->getBateria();
    flush();
    ss << "Energia Tecnico:   " << monitor.get_entidad(1)->getBateria();
    flush();
    ss << "Instantes consumidos: "
       << monitor.getInstantesInicial() - monitor.get_entidad(0)->getInstantesPendientes();
    flush();
    ss << "Tiempo: "
       << (monitor.get_entidad(0)->getTiempo() + monitor.get_entidad(1)->getTiempo()) / CLOCKS_PER_SEC
       << "s";
    flush();
    ss << "Descubierto mapa (Caminos y Senderos): "
       << monitor.CoincidenciaConElMapaCaminosYSenderos() << "%";
    flush();
    ss << "Descubierto Mapa: " << monitor.CoincidenciaConElMapa() << "%";
    flush();
    break;
  }

  // ── Nivel 2: Corre, Ingeniero, Corre ──────────────────────────────────────
  case 2: {
    ss << "Nivel 2 " << (monitor.get_entidad(0)->SeHanConseguidoLosObjetivos() ? "completado con Exito!" : "NO completado.");
    flush();
    ss << "Longitud camino Ingeniero: "
       << monitor.getInstantesInicial() - monitor.get_entidad(0)->getInstantesPendientes();
    flush();
    ss << "Consumo Energia Ingeniero: "
       << monitor.getEnergiaInicial() - monitor.get_entidad(0)->getBateria();
    flush();
    ss << "Instantes consumidos: "
       << monitor.getInstantesInicial() - monitor.get_entidad(0)->getInstantesPendientes();
    flush();
    ss << "Tiempo: "
       << (monitor.get_entidad(0)->getTiempo() + monitor.get_entidad(1)->getTiempo()) / CLOCKS_PER_SEC
       << "s";
    flush();
    break;
  }

  // ── Nivel 3: El Técnico mide sus esfuerzos ────────────────────────────────
  case 3: {
    ss << "Nivel 3 " << (monitor.get_entidad(1)->SeHanConseguidoLosObjetivos() ? "completado con Exito!" : "NO completado.");
    flush();
    ss << "Longitud camino Tecnico: "
       << monitor.getInstantesInicial() - monitor.get_entidad(1)->getInstantesPendientes();
    flush();
    ss << "Consumo Energia Tecnico: "
       << monitor.getEnergiaInicial() - monitor.get_entidad(1)->getBateria();
    flush();
    ss << "Instantes consumidos: "
       << monitor.getInstantesInicial() - monitor.get_entidad(0)->getInstantesPendientes();
    flush();
    ss << "Tiempo: "
       << (monitor.get_entidad(0)->getTiempo() + monitor.get_entidad(1)->getTiempo()) / CLOCKS_PER_SEC
       << "s";
    flush();
    break;
  }

  // ── Nivel 4: Planifica, Ingeniero, Planifica ──────────────────────────────
  case 4: {
    ss << "Nivel 4 " << (monitor.get_entidad(0)->SeHanConseguidoLosObjetivos() ? "completado con Exito!" : "NO completado.");
    flush();
    ss << "Longitud red tuberias: " << monitor.get_entidad(0)->getCanalizacionPlan().size();
    flush();

    int simulated_impact = 0;
    ListaCasillasPlan plan = monitor.get_entidad(0)->getCanalizacionPlan();
    simulated_impact = monitor.calcularImpactoPlan(plan);
    ss << "Impacto Ecologico: " << simulated_impact;
    flush();
    break;
  }

  // ── Niveles 5 y 6: A Poner Tuberías ──────────────────────────────────────
  case 5:
  case 6: {
    bool exito = monitor.get_entidad(0)->SeHanConseguidoLosObjetivos() || monitor.get_entidad(1)->SeHanConseguidoLosObjetivos();
    ss << "Nivel " << nivel << (exito ? " completado con Exito! Conexion de tuberias establecida."
                                      : " NO completado.");
    flush();
    ss << "Instantes consumidos: "
       << monitor.getInstantesInicial() - monitor.get_entidad(0)->getInstantesPendientes();
    flush();
    ss << "Consumo Energia Ingeniero: "
       << monitor.getEnergiaInicial() - monitor.get_entidad(0)->getBateria();
    flush();
    ss << "Consumo Energia Tecnico:   "
       << monitor.getEnergiaInicial() - monitor.get_entidad(1)->getBateria();
    flush();
    ss << "Impacto Ecologico: " << monitor.getImpactoEcologico();
    flush();

    int longitud_red = 0;
    auto &mapaTuberias = monitor.getMapaTuberias();
    for (int i = 0; i < (int)mapaTuberias.size(); i++)
      for (int j = 0; j < (int)mapaTuberias[i].size(); j++)
        if (mapaTuberias[i][j] != 0) longitud_red++;
    ss << "Longitud red tuberias: " << longitud_red;
    flush();
    break;
  }

  // ── Nivel 7 (y resto): Modo multiobjetivo ────────────────────────────────
  default: {
    bool exito = monitor.get_entidad(0)->fin() || monitor.get_entidad(1)->fin();
    ss << "Nivel " << nivel << (exito ? " completado con Exito!" : " NO completado.");
    flush();
    ss << "Instantes restantes: " << monitor.get_entidad(0)->getInstantesPendientes();
    flush();
    ss << "Tiempo: "
       << (monitor.get_entidad(0)->getTiempo() + monitor.get_entidad(1)->getTiempo()) / CLOCKS_PER_SEC
       << "s";
    flush();
    ss << "Energia Ingeniero: " << monitor.get_entidad(0)->getBateria();
    flush();
    ss << "Energia Tecnico:   " << monitor.get_entidad(1)->getBateria();
    flush();
    ss << "Colisiones: "
       << monitor.get_entidad(0)->getColisiones() + monitor.get_entidad(1)->getColisiones();
    flush();
    ss << "Descubierto mapa (Caminos y Senderos): "
       << monitor.CoincidenciaConElMapaCaminosYSenderos() << "%";
    flush();
    ss << "Descubierto Mapa: " << monitor.CoincidenciaConElMapa() << "%";
    flush();
    ss << "Objetivos: (" << monitor.get_entidad(0)->getMisiones() << ") "
       << monitor.get_entidad(0)->getPuntuacion();
    flush();
    ss << "Impacto Ecologico: " << monitor.getImpactoEcologico();
    flush();
    break;
  }

  } // fin switch

  monitor.setMostrarResultados(false);
}



/**
 * @brief Avanza la simulación un paso (o la finaliza) en modo gráfico interactivo.
 *
 * Comprueba si el juego debe continuar ejecutándose según el estado del monitor.
 * Si hay pasos pendientes y el juego no ha terminado, invoca @ref nucleo_motor_juego.
 * Si el monitor indica que deben mostrarse los resultados, llama a
 * @ref ImprimirResultadosJuego y devuelve @c true para que la interfaz gráfica
 * desactive los controles de simulación.
 *
 * Esta función está pensada para ser llamada de forma repetida desde el temporizador
 * de la interfaz gráfica (GLUT/GLUI), procesando un instante por llamada.
 *
 * @param acc  Parámetro de acción manual en modo paso a paso; -1 en ejecución libre.
 * @return true  si el juego ha finalizado y se han mostrado los resultados.
 * @return false si la simulación sigue en curso o no ha comenzado.
 */
bool lanzar_motor_juego(int acc) {
  bool out = false;

  if (monitor.continuarBelkan()) {
    if (monitor.jugar()) {
      if ((monitor.getPasos() != 0) and (!monitor.finJuego())) {
        nucleo_motor_juego(monitor, acc);
      }
    }

    if (monitor.mostrarResultados()) {
      ImprimirResultadosJuego(monitor);
      out = true;
    }
  }
  return out;
}

/**
 * @brief Ejecuta la simulación completa en modo sin gráficos (batch/SG).
 *
 * Corre el bucle principal de la simulación hasta que el juego termina
 * o el monitor indica que no se debe continuar. Cada iteración del bucle
 * llama a @ref nucleo_motor_juego con @c acc = -1 (sin acción manual).
 * Al terminar, si procede, muestra los resultados mediante
 * @ref ImprimirResultadosJuego.
 *
 * @param monitor  Referencia al monitor global del juego.
 */
void lanzar_motor_juego2(MonitorJuego &monitor) {

  // monitor.get_entidad(0)->setObjetivo(monitor.getObjFil(),
  // monitor.getObjCol());

  while (!monitor.finJuego() && monitor.jugar()) {
    nucleo_motor_juego(monitor, -1);
  }

  if (monitor.mostrarResultados()) {
    ImprimirResultadosJuego(monitor);
  }
}
