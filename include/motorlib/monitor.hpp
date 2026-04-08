#ifndef MONITOR_H
#define MONITOR_H

#include "motorlib/entidad.hpp"
#include "motorlib/mapa.hpp"
#include "motorlib/util.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#include <glui/glui.h>
#pragma GCC diagnostic pop

#include <list>

#include <algorithm>
#include <deque>
#include <queue>
#include <set>
#include <iostream>
#include <sstream>

struct VisualMarker {
  int f, c, d, type; // 0: Cruz Roja (acción fallida), 1: ¡Naranja! (colisión)
};

// Clase para monitorizar y controlar la ejecucion de todo el juego
// Es necesaria si se quiere independizar el dibujado de los modelo del
// comportamiento de los agentes.

class MonitorJuego {
private:
  int semilla;
  list<pair<int, int>> objetivos;
  vector<unsigned int> objetivosActivos;
  vector<bool> alcanzados;
  vector<Entidad *> entidades;
  Mapa *mapa;
  std::vector<std::vector<unsigned char>> mapaTuberias;
  std::vector<VisualMarker> visualMarkers;
  std::deque<std::string> mensajes;
  int scrollMensajes = 0;

  bool apagarBelkan = false; // Notificar al motor de juego que sigua corriendo
  bool jugando = false;      // Notificar al motor de juego que ejecute los
                             // pensamientos (thinks)
  bool empezarJuego =
      false; // Notificar al motor de juego que inicie un nuevo juego

  bool resultados = false;

  int pasos = 0;
  int pasosTotales = 0;
  int maxPasos = 3000;

  int retardo = 5;
  int delay_call_on = 0; // Tiempo que permanece activa la llamada al Técnico

  int nivel = 1;

  bool recarga_pisada_ingeniero = false; // Para informar en el nivel 0.
  bool recarga_pisada_tecnico = false;
  bool recarga_abandonada_ingeniero = false; // Para informar en el nivel 0.
  bool recarga_abandonada_tecnico = false;

  bool camino_abandonado_ingeniero = false;
  bool camino_abandonado_tecnico = false;
  int impacto_ecologico = 0;
  int maxImpacto = 1000;
  int energiaInicial = 3000;
  int InstantesInicial = 3000;

  bool reset_activado = false;

public:
  MonitorJuego(int x = 3000) {
    maxPasos = x;
    mapa = nullptr;
    impacto_ecologico = 0;
  }
  ~MonitorJuego() {}

  void nueva_entidad(Entidad *entidad) { entidades.push_back(entidad); }
  unsigned int numero_entidades() { return entidades.size(); }

  void clear() { entidades.clear(); }
  Entidad *get_entidad(unsigned entidad) { return entidades[entidad]; }
  vector<Entidad *> *get_entidades() { return &entidades; }
  void cambiarPosicion(unsigned char entidad, unsigned char x,
                       unsigned char y) {
    entidades[entidad]->setPosicion(x, y);
  }
  void cambiarOrientacion(unsigned char entidad, unsigned char orientacion);

  Mapa *getMapa() { return mapa; }
  void setMapa(const char *file);
  void setMaxImpacto(int v) { maxImpacto = v; }
  int getMaxImpacto() { return maxImpacto; }
  void setEnergiaInicial(int v) { energiaInicial = v; }
  int getEnergiaInicial() { return energiaInicial; }
  void setInstantesInicial(int v) { InstantesInicial = v; }
  int getInstantesInicial() { return InstantesInicial; }
  void startGame(
      int valor = 1,
      int pMaxPasos = 3000) { // Indica que hay un nuevo juego que inicializar
    empezarJuego = true;
    pasosTotales = 0;
    pasos = 0;
    nivel = valor;
    maxPasos = pMaxPasos;
    reset_activado = false;
  }
  bool inicializarJuego(); // Mira si tiene que inicializar un juego
  int juegoInicializado() {
    jugando = true;
    return mapa->getNFils();
  }

  bool continuarBelkan() { return !apagarBelkan; }
  void cerrarBelkan() { apagarBelkan = true; }

  bool mostrarResultados() { return resultados; }
  void setMostrarResultados(bool valor) { resultados = valor; }

  bool jugar() { return jugando; }

  void girarJugadorIzquierda(int grados) { mapa->girarCamaraIzquierda(grados); }
  void girarJugadorDerecha(int grados) { mapa->girarCamaraDerecha(grados); }

  void setPasos(int Npasos) { pasos = Npasos; }
  void decPasos();
  int getPasos() { return pasos; }

  bool finJuego() {
    return (((maxPasos - pasosTotales) <= 0) or
            (impacto_ecologico >= maxImpacto) or
            (get_entidad(0)->getBateria() == 0) or
            (get_entidad(1)->getBateria() == 0) or
            (!get_entidad(0)->vivo()) or
            (!get_entidad(1)->vivo()));
  }
  void finalizarJuego() { pasosTotales = maxPasos; }

  void setRetardo(int tRetardo) { retardo = tRetardo; }
  int getRetardo() { return retardo; }

  int getLevel() { return nivel; }

  void setResetActivado(bool valor) { reset_activado = valor; }
  bool getResetActivado() const { return reset_activado; }

  void setListObj(list<pair<int, int>> &obj) {
    if (obj.size() > 0)
      objetivos = obj;
  }
  list<pair<int, int>> getListObj() { return objetivos; }

  void generate_a_valid_cell(int &pos_fila, int &pos_col, int &ori);
  bool is_a_valid_cell_like_goal(int f, int c);
  void generate_a_objetive();
  void put_a_new_objetivo_front(int fila, int columna);
  void put_active_objetivos(int number);
  void get_n_active_objetivo(int n, int &posFila, int &posCol);
  void set_n_active_objetivo(int n, int posFila, int posCol);
  bool there_are_active_objetivo();
  int get_number_active_objetivos();
  vector<unsigned int> get_active_objetivos();

  int isMemberObjetivo(unsigned int paramF, unsigned int paramC);
  bool allObjetivosAlcanzados();
  void actualizarAlcanzados();
  void anularAlcanzados();

  void inicializar(int pos_filaJ = -1, int pos_colJ = -1, int brujJ = -1,
                   int pos_filaS = -1, int pos_colS = -1, int brujS = -1,
                   int seed = 1);
  void ReAparicionesEntidad(int Entidad, int fila, int columna,
                            Orientacion brujula);

  int get_semilla() const { return semilla; }
  void set_semilla(int seed) { semilla = seed; }

  bool CanHeSeesThisCell(int num_entidad, int fil, int col);

  void PintaEstadoMonitor();
  double CoincidenciaConElMapa();
  double CoincidenciaConElMapaCaminosYSenderos();

  string toString();
  void init_casillas_especiales(unsigned int f, unsigned int c,
                                unsigned int fcolab, unsigned int ccolab);
  void initialize_tuberias(int nf, int nc) {
    mapaTuberias.assign(nf, std::vector<unsigned char>(nc, 0));
  }
  std::vector<std::vector<unsigned char>> &getMapaTuberias() {
    return mapaTuberias;
  }
  void reset_objetivos();

  string strAccion(Action accion);

  inline bool getRecargaPisadaIngeniero() { return recarga_pisada_ingeniero; }
  inline bool getRecargaPisadaTecnico() { return recarga_pisada_tecnico; }
  inline bool getRecargaAbandonadaIngeniero() {
    return recarga_abandonada_ingeniero;
  }
  inline bool getRecargaAbandonadaTecnico() {
    return recarga_abandonada_tecnico;
  }
  inline void setRecargaPisadaIngeniero(bool valor) {
    recarga_pisada_ingeniero = valor;
  }
  inline void setRecargaPisadaTecnico(bool valor) {
    recarga_pisada_tecnico = valor;
  }
  inline void setRecargaAbandonadaIngeniero(bool valor) {
    recarga_abandonada_ingeniero = valor;
  }
  inline void setRecargaAbandonadaTecnico(bool valor) {
    recarga_abandonada_tecnico = valor;
  }

  inline bool getCaminoAbandonadoIngeniero() {
    return camino_abandonado_ingeniero;
  }
  inline void setCaminoAbandonadoIngeniero(bool valor) {
    camino_abandonado_ingeniero = valor;
  }
  inline bool getCaminoAbandonadoTecnico() { return camino_abandonado_tecnico; }
  inline void setCaminoAbandonadoTecnico(bool valor) {
    camino_abandonado_tecnico = valor;
  }

  inline int getImpactoEcologico() { return impacto_ecologico; }
  inline void addImpactoEcologico(int valor) {
    impacto_ecologico += valor;
    if (impacto_ecologico >= maxImpacto) {
      addMensaje("Sistema", "Se superó el máximo impacto ecologico permitido");
      std::cout << "Se superó el máximo impacto ecologico permitido" << std::endl;
    }
  }
  inline void resetImpactoEcologico() { impacto_ecologico = 0; }
  int getCosteEco(Action accion, unsigned char celda);

  inline void setDelayCallOn(int valor) { delay_call_on = valor; }
  inline bool AnularCallOn() { return delay_call_on == 0; }
  inline void decCallON() {
    if (delay_call_on > 0)
      delay_call_on--;
  }
  bool checkPipeConnection(int startF, int startC);
  bool checkLevel4();
  int calcularImpactoPlan(const ListaCasillasPlan &plan);

  void addFailedAction(int f, int c, int duration = 2) {
    visualMarkers.push_back({f, c, duration, 0});
  }
  void addCollisionMarker(int f, int c, int duration = 2) {
    visualMarkers.push_back({f, c, duration, 1});
  }
  void clearFailedAction(int f, int c);
  void updateVisualMarkers() {
    for (auto &vm : visualMarkers)
      vm.d--;
    visualMarkers.erase(
        std::remove_if(visualMarkers.begin(), visualMarkers.end(),
                       [](const VisualMarker &vm) { return vm.d <= 0; }),
        visualMarkers.end());
  }
  const std::vector<VisualMarker> &getVisualMarkers() const {
    return visualMarkers;
  }
  // Alias heredado por compatibilidad con el código existente durante la
  // refactorización
  const std::vector<VisualMarker> &getFailedActions() const {
    return visualMarkers;
  }

  int getPasosTotales() const { return pasosTotales; }

  void addMensaje(string msg) { mensajes.push_back(msg); }

  void clearMensajes() {
    mensajes.clear();
    scrollMensajes = 0;
  }

  int getScrollMensajes() const { return scrollMensajes; }
  void setScrollMensajes(int s) { scrollMensajes = s; }

  void addMensaje(const string &agente, const string &msg) {
    stringstream ss;
    ss << "[" << pasosTotales << "] [" << agente << "] " << msg;
    addMensaje(ss.str());
  }

  const std::deque<string> &getMensajes() const { return mensajes; }
};
#endif
