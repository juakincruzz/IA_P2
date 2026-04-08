// Bugfix: Energia inicial hardcodeada solucionada
#include "motorlib.hpp"
#include <fstream>

MonitorJuego monitor;

int main(int argc, char **argv)
{
  EnLinea argumentos;
  // Valores por defecto
  argumentos.ubicacion_mapa = "./mapas/mapa30.map";
  argumentos.semilla = 0;
  argumentos.level = 0;
  argumentos.posInicialIngeniero.f = -1;
  argumentos.posInicialIngeniero.c = -1;
  argumentos.posInicialIngeniero.brujula = norte;
  argumentos.posInicialTecnico.f = -1;
  argumentos.posInicialTecnico.c = -1;
  argumentos.posInicialTecnico.brujula = norte;
  argumentos.max_instantes = 3000;
  argumentos.initial_energy = 3000;
  argumentos.eco_impact_threshold = 1000;

  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " -m <mapa> -seed <semilla> -n <nivel> -i <fI> <cI> <oI> -t <fT> <cT> <oT> -Tiempo <instantes> -Energia <energia> -Ambiental <impacto> -O [fO cO]*\n";
  }
  else
  {
    for (int i = 1; i < argc; i++)
    {
      if (string(argv[i]) == "-m" or string(argv[i]) == "-mapa")
      {
        argumentos.ubicacion_mapa = argv[i + 1];
        i++;
      }
      else if (string(argv[i]) == "-seed")
      {
        argumentos.semilla = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-n" or string(argv[i]) == "-nivel")
      {
        argumentos.level = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-i")
      {
        argumentos.posInicialIngeniero.f = atoi(argv[i + 1]);
        argumentos.posInicialIngeniero.c = atoi(argv[i + 2]);
        argumentos.posInicialIngeniero.brujula = static_cast<Orientacion>(atoi(argv[i + 3]));
        i += 3;
      }
      else if (string(argv[i]) == "-t")
      {
        argumentos.posInicialTecnico.f = atoi(argv[i + 1]);
        argumentos.posInicialTecnico.c = atoi(argv[i + 2]);
        argumentos.posInicialTecnico.brujula = static_cast<Orientacion>(atoi(argv[i + 3]));
        i += 3;
      }
      else if (string(argv[i]) == "-Tiempo")
      {
        argumentos.max_instantes = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-Energia")
      {
        argumentos.initial_energy = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-Ambiental")
      {
        argumentos.eco_impact_threshold = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-O")
      {
         i++;
         while (i < argc and string(argv[i]).find("-") != 0) {
            pair<int, int> aux;
            aux.first = atoi(argv[i]);
            aux.second = atoi(argv[i+1]);
            argumentos.listaObjetivos.push_back(aux);
            i += 2;
         }
         i--;
      }
    }

    monitor.setMapa(argumentos.ubicacion_mapa.c_str());
    srand(argumentos.semilla);
    monitor.setMaxImpacto(argumentos.eco_impact_threshold);
    monitor.setEnergiaInicial(argumentos.initial_energy);
    monitor.setInstantesInicial(argumentos.max_instantes);
    monitor.startGame(argumentos.level, argumentos.max_instantes);

    ifstream ifile;
    ifile.open(argumentos.ubicacion_mapa, ios::in);

    unsigned int colSize, filSize;
    ifile >> colSize;
    ifile >> filSize;

    vector<vector<unsigned char>> mapAux;
    vector<unsigned char> colAux(colSize);

    for (unsigned int i = 0; i < filSize; i++)
    {
      mapAux.push_back(colAux);
    }

    for (unsigned int i = 0; i < filSize; i++)
    {
      for (unsigned int j = 0; j < colSize; j++)
      {
        ifile >> mapAux[i][j];
      }
    }
    ifile.close();
    
    // Reviso si es una posicion valida del ingeniero
    if (argumentos.posInicialIngeniero.f != -1)
    {
      if (mapAux[argumentos.posInicialIngeniero.f][argumentos.posInicialIngeniero.c] == 'P' or mapAux[argumentos.posInicialIngeniero.f][argumentos.posInicialIngeniero.c] == 'M' or mapAux[argumentos.posInicialIngeniero.f][argumentos.posInicialIngeniero.c] == 'B')
      {
        cout << "Posicion (" << argumentos.posInicialIngeniero.f << "," << argumentos.posInicialIngeniero.c << ") no es de tipo válido para el ingeniero" << endl;
        return 0;
      }
    }

    // Reviso si es una posicion valida del tecnico
    if (argumentos.posInicialTecnico.f != -1)
    {
      if (mapAux[argumentos.posInicialTecnico.f][argumentos.posInicialTecnico.c] == 'P' or mapAux[argumentos.posInicialTecnico.f][argumentos.posInicialTecnico.c] == 'M' or mapAux[argumentos.posInicialTecnico.f][argumentos.posInicialTecnico.c] == 'B')
      {
        cout << "Posicion (" << argumentos.posInicialTecnico.f << "," << argumentos.posInicialTecnico.c << ") no es de tipo válido para el tecnico" << endl;
        return 0;
      }
    }

    for(auto it = argumentos.listaObjetivos.begin(); it != argumentos.listaObjetivos.end(); ++it){
      if (mapAux[it->first][it->second] == 'P' or mapAux[it->first][it->second] == 'M' or mapAux[it->first][it->second] == 'B')
      {
        cout << "Posicion (" << it->first << "," << it->second << ") no es de tipo válido para el objetivo" << endl;
      }
    }

    monitor.setListObj(argumentos.listaObjetivos);

    monitor.inicializar(argumentos.posInicialIngeniero.f, argumentos.posInicialIngeniero.c, argumentos.posInicialIngeniero.brujula, argumentos.posInicialTecnico.f, argumentos.posInicialTecnico.c, argumentos.posInicialTecnico.brujula, argumentos.semilla);
    
    monitor.get_entidad(0)->setOrientacion(argumentos.posInicialIngeniero.brujula);
    monitor.get_entidad(0)->setBateria(argumentos.initial_energy);
    
    monitor.get_entidad(1)->setOrientacion(argumentos.posInicialTecnico.brujula);
    monitor.get_entidad(1)->setBateria(argumentos.initial_energy);
    
    monitor.setPasos(argumentos.max_instantes);
    monitor.setRetardo(0);

    monitor.juegoInicializado();

    lanzar_motor_juego2(monitor);
  }
  exit(EXIT_SUCCESS);
}
