#ifndef COMPORTAMIENTO_H
#define COMPORTAMIENTO_H

#include <list>
#include <vector>

using namespace std;

enum Orientacion {
  norte,
  noreste,
  este,
  sureste,
  sur,
  suroeste,
  oeste,
  noroeste
};
enum Action {
  WALK,
  JUMP,
  TURN_SR,
  COME,
  IDLE,
  PUSH,
  INSTALL,
  DIG,
  RAISE,
  TURN_SL
};

struct Paso {
  int fil;
  int col;
  int op; // -1 bajar la casilla, 0 dejar como está, 1 subir la casilla
};

typedef list<Paso> ListaCasillasPlan;

struct ubicacion {
  int f;
  int c;
  Orientacion brujula;

  bool operator==(const ubicacion &ub) const {
    return (f == ub.f and c == ub.c and brujula == ub.brujula);
  }
};

struct Sensores {
  int nivel;
  int vida;
  int energia;
  bool choque;
  bool reset;
  int posF;
  int posC;
  Orientacion rumbo; // Orientacion
  double tiempo;
  vector<unsigned char> superficie;
  vector<unsigned char> agentes;
  vector<unsigned char> cota;
  int BelPosF;
  int BelPosC;
  bool venpaca;  // Avisa al tecnico que vaya a las coordenadas del ingeniero
  bool enfrente; // Indica que el ingeniero esta en una casilla adyacente al
                 // técnico y ambos tienen orientaciones opuestas
  int GotoF;     // Fila destino enviada por el ingeniero
  int GotoC;     // Columna destino enviada por el ingeniero
  int ecologico; // Nivel de impacto ecológico provocado
  int max_ecologico; // Máximo impacto ecológico permitido
};

class Comportamiento {
public:
  vector<vector<unsigned char>> mapaResultado;
  vector<vector<unsigned char>> mapaEntidades;
  std::vector<std::vector<unsigned char>> mapaCotas;
  std::vector<std::vector<unsigned char>>
      mapaTuberias; // 0=None, 1=N, 4=E, 16=S, 64=W
  ListaCasillasPlan listaPlanCasillas;
  ListaCasillasPlan listaCanalizacionTuberias;

  Comportamiento(unsigned int size);
  Comportamiento(vector<vector<unsigned char>> mapaR,
                 vector<vector<unsigned char>> mapaC);
  Comportamiento(const Comportamiento &comport)
      : mapaEntidades(comport.mapaEntidades),
        mapaResultado(comport.mapaResultado), mapaCotas(comport.mapaCotas),
        listaPlanCasillas(comport.listaPlanCasillas),
        listaCanalizacionTuberias(comport.listaCanalizacionTuberias) {}
  Comportamiento *clone() { return new Comportamiento(*this); }
  virtual ~Comportamiento() {}

  virtual Action think(Sensores sensores);

  virtual int interact(Action accion, int valor);

  vector<vector<unsigned char>> getMapaResultado() { return mapaResultado; }
  vector<vector<unsigned char>> getMapaEntidades() { return mapaEntidades; }
  vector<vector<unsigned char>> getMapaCotas() { return mapaCotas; }
  vector<vector<unsigned char>> getMapaTuberias() { return mapaTuberias; }
  ListaCasillasPlan getMapaPlan() { return listaPlanCasillas; }
  ListaCasillasPlan getCanalizacionPlan() { return listaCanalizacionTuberias; }
};

#endif
