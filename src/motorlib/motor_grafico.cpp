#include "motorlib.hpp"

#include <algorithm>
#include <dirent.h>
#include <glui/glui.h>

MonitorJuego monitor;

// variables que determninan la posicion y tamaño de la ventana X
const int IU_ancho_ventana = 1024;
const int IU_alto_ventana = 768;

// Variables que controlan las ventanas y vistas
GLuint ventanaPrincipal, vistaPrincipal, vistaMiniMapa, vistaIU, vistaInfo,
    vistaMensajes;
GLUI *panelSelecMapa, *panelSelecMapaConfig;
GLUI_Listbox *listbox;
GLUI_RadioGroup *group;
GLUI_Button *botonElegirMapa, *botonConfigurar, *botonPaso, *botonEjecucion,
    *botonEjecutar, *botonSalir;
GLUI_EditText *editPosColumna, *editPosFila, *editTextPasos, *editTextRetardo;
GLUI_Checkbox *drawMMTopology;
GLUI_Listbox *listboxMap, *listbox3D;
GLUI_Spinner *spinnerZoom;
GLUI_StaticText *lineaVacia, *info0, *info1, *info2, *info3, *info4, *info5,
    *info6, *info7, *info8, *info9, *info10;
GLUI_Spinner *setup1, *setup20, *setup21, *setup30, *setup31, *setup4, *setup5,
    *setup60, *setup61;
GLUI_Spinner *setup_inst, *setup_ener, *setup_eco;
// GLUI_Spinner *editPosColumna, *editPosFila;

int nPasos = 10, tRetardo = 1, MMmode = 3, MMmode2 = 0, MMmode3 = 0,
    MMmode4 = 0, PosColumna = 1, PosFila = 1, tMap = 100;
int camMode = 0; // 0: First Person, 1: Zenithal
int Check1stPerZenital = 1, CheckTotalTopology = 1, CheckGridFP = 1;
int objtiveSelected = 0;
float zoomZenithal = 55.0f;
int ultimomapaPos = 0, ultimonivel = 0;
int ObjFil1 = -1, ObjFil2 = -1, ObjFil3, ObjCol1 = -1, ObjCol2, ObjCol3;
int ObjInstSim = 3000, ObjEnergia = 3000, ObjUmbEco = 1000;

void irAlJuego(int valor);

/**
 * @brief Divide una cadena de texto en líneas usando '\n' como separador.
 *
 * @param str   Cadena de entrada con posibles saltos de línea.
 * @param strs  Vector de salida donde se almacenan las líneas extraídas.
 */
void Descomponer(string str, vector<string> &strs) {
  while (str.length() != 0) {
    strs.push_back(str.substr(0, str.find('\n')));
    str = str.substr(str.find('\n') + 1, str.length());
  }
}

/**
 * @brief Renderiza una cadena de texto 2D en la ventana OpenGL activa.
 *
 * @param x     Coordenada X (en píxeles) de la posición inicial del texto.
 * @param y     Coordenada Y (en píxeles) de la posición inicial del texto.
 * @param text  Texto a dibujar (cadena terminada en nulo).
 * @param font  Fuente bitmap de GLUT (p.ej. GLUT_BITMAP_HELVETICA_12).
 * @param r     Componente roja del color [0,1].
 * @param g     Componente verde del color [0,1].
 * @param b     Componente azul del color [0,1].
 */
void renderText(float x, float y, const char *text, void *font, float r,
                float g, float b) {
  glDisable(GL_LIGHTING);
  glColor3f(r, g, b);
  glRasterPos2f(x, y);
  while (*text) {
    glutBitmapCharacter(font, *text);
    text++;
  }
}

/**
 * @brief Dibuja un rectángulo relleno con color y transparencia en 2D.
 *
 * @param x  Coordenada X de la esquina superior-izquierda.
 * @param y  Coordenada Y de la esquina superior-izquierda.
 * @param w  Anchura del rectángulo (píxeles).
 * @param h  Altura del rectángulo (píxeles).
 * @param r  Componente roja [0,1].
 * @param g  Componente verde [0,1].
 * @param b  Componente azul [0,1].
 * @param a  Alpha (transparencia) [0,1]. 0 = totalmente transparente.
 */
void renderRect(float x, float y, float w, float h, float r, float g, float b,
                float a) {
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(r, g, b, a);
  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glEnd();
  glDisable(GL_BLEND);
}

/**
 * @brief Dibuja una barra de progreso horizontal con fondo oscuro y borde gris.
 *
 * @param x        Coordenada X de la esquina superior-izquierda.
 * @param y        Coordenada Y de la esquina superior-izquierda.
 * @param w        Anchura total de la barra (píxeles).
 * @param h        Altura de la barra (píxeles).
 * @param percent  Fracción rellena [0.0, 1.0].
 * @param r        Componente roja del color de relleno [0,1].
 * @param g        Componente verde del color de relleno [0,1].
 * @param b        Componente azul del color de relleno [0,1].
 */
void renderBar(float x, float y, float w, float h, float percent, float r,
               float g, float b) {
  renderRect(x, y, w, h, 0.1f, 0.1f, 0.1f, 0.6f);  // Background
  renderRect(x, y, w * percent, h, r, g, b, 0.9f); // Foreground
  glColor3f(0.5, 0.5, 0.5);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glEnd();
}

/*void keyboard(unsigned char key, int x, int y) {
  float a;
  switch(key) {
    case 'w': // UP
      a = monitor.getMapa()->getAngle();
      a = a + 0.01;
      cout << "Y1: " << a << endl;
      monitor.getMapa()->putAngle(a);
      break;

    case 's': // DCHA
      a = monitor.getMapa()->getParamX();
      a = a + 0.01;
      cout << "Y2: " << a << endl;
      monitor.getMapa()->PutParamX(a);
      break;

    case 'a': // IZQDA
      a = monitor.getMapa()->getParamZ();
      a = a + 0.01;
      cout << "Y3: " << a << endl;
      monitor.getMapa()->PutParamZ(a);
      break;

    case 'd': // IZQDA
      a = monitor.getMapa()->getParamZ();
      a = a - 0.01;
      cout << "Y3: " << a << endl;
      monitor.getMapa()->PutParamZ(a);
      break;

    default: // Paranoia
      cout << key << " " << "ERROR\n";
      break;
  }
}*/

//***************************************************************************
// Funcion llamada cuando se produce un cambio en el tamaño de la ventana
//
// el evento manda a la funcion: nuevo ancho, nuevo alto
//***************************************************************************

/**
 * @brief Callback de redimensionado de la ventana principal.
 *
 * Redistribuye automáticamente las subventanas (mapa 2D, zona de mensajes,
 * vista 3D, panel de información de agentes y panel de controles) cada vez
 * que el usuario cambia el tamaño de la ventana X.
 *
 * @param ancho  Nuevo ancho de la ventana en píxeles.
 * @param alto   Nuevo alto de la ventana en píxeles.
 */
void reshape(int ancho, int alto) {
  if ((ancho > 1) and (alto > 1)) {
    // parametros de la ventana principal

    float ratio = ((float)ancho * 0.7) / (float)alto;

    glMatrixMode(GL_PROJECTION);
    glViewport(0, 0, ancho, alto);
    glLoadIdentity();
    gluOrtho2D(0, ancho, alto, 0);
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    // Display vista principal
    // Display vista mini-mapa (The 2D Map on the left)
    int h_msg = 65;          // Height for 3 lines of text
    int w_2d = alto - h_msg; // Height of square 2D map
    if (w_2d > ancho * 0.8)
      w_2d = ancho * 0.8; // Cap width if needed

    glutSetWindow(vistaPrincipal);
    glutPositionWindow(0, 0);
    glutReshapeWindow(w_2d, w_2d);
    glutPostRedisplay();

    // Display vista mensajes (Below 2D map)
    glutSetWindow(vistaMensajes);
    glutPositionWindow(0, w_2d);
    glutReshapeWindow(w_2d, h_msg);
    glutPostRedisplay();

    // Right side split
    int w_right = ancho - w_2d;
    int h_3d = alto * 0.3;
    int h_info = alto * 0.3;         // Fixed height for agent info
    int h_iu = alto - h_3d - h_info; // Remaining for controls

    // Display vista 3D (formerly MiniMapa variable)
    glutSetWindow(vistaMiniMapa);
    glutPositionWindow(w_2d, 0);
    glutReshapeWindow(w_right, h_3d);
    glutPostRedisplay();

    // Display Info (Agent Data) - Move up
    glutSetWindow(vistaInfo);
    glutPositionWindow(w_2d, h_3d);
    glutReshapeWindow(w_right, h_info);
    glutPostRedisplay();

    // Display interfaz (Controls) - Move down, with 10 units displacement
    glutSetWindow(vistaIU);
    glutPositionWindow(w_2d, h_3d + h_info);
    glutReshapeWindow(w_right, h_iu);
    glutPostRedisplay();
  }
}

/**
 * @brief Configura el viewport 3D (perspectiva) para la subventana activa.
 *
 * Establece una proyección frustum estándar y carga la matriz de modelview
 * identidad. Se invoca antes de renderizar contenido 3D.
 */
void ResetViewport() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-10.0, 10.0, -10.0, 10.0, 0.0, 10.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

/**
 * @brief Configura el viewport 2D (ortográfico) para la subventana activa.
 *
 * Establece una proyección ortográfica con coordenadas en píxeles y
 * desactiva iluminación, profundidad y niebla. Se invoca antes de
 * renderizar contenido 2D (HUD, minimapa, panel de información).
 */
void ResetViewport2D() {
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, h, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
}

/**
 * @brief Callback de refresco de la ventana raíz principal.
 *
 * Limpia el buffer de color con fondo gris claro. La ventana raíz actúa
 * únicamente como contenedor de las subventanas; su propio contenido
 * visible es mínimo.
 */
void display_ventPrincipal(void) {
  // Background Color
  glClearColor(0.8, 0.8, 0.8, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glutSwapBuffers();
}

/**
 * @brief Callback de refresco de la subventana principal (vista 2D del mapa).
 *
 * Dibuja el mapa Belkan en la subventana de mayor tamaño. Soporta dos modos
 * de cámara: primera persona (@c camMode==0) y cenital (@c camMode==1).
 * También renderiza el cielo con nubes animadas y actualiza el grid de
 * primera persona según el flag @c CheckGridFP.
 */
void display_vistPrincipal() {
  ResetViewport();

  glMatrixMode(GL_PROJECTION);

  if (monitor.getMapa() != 0) {
    float ratio = ((float)IU_ancho_ventana * 0.7) / (float)IU_alto_ventana;
    gluPerspective(45, ratio, 1, 1000);

    // Clear color (background)
    glClearColor(0.53, 0.81, 0.92, 1.0); // Light sky blue

    // Enable fog for atmospheric depth - Disabled for CPU performance
    glDisable(GL_FOG);
    // GLfloat fogColor[] = {0.73, 0.87, 0.95, 1.0}; // Horizon color
    // glFogfv(GL_FOG_COLOR, fogColor);
    // glFogi(GL_FOG_MODE, GL_EXP2);
    // glFogf(GL_FOG_DENSITY, 0.0020f); // Slightly lighter fog for clarity
    // glHint(GL_FOG_HINT, GL_NICEST);
  } else {
    glClearColor(0.0, 0.0, 0.0, 1);
    glDisable(GL_FOG);
  }

  glMatrixMode(GL_MODELVIEW);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // --- DRAW SKY GRADIENT ---
  if (monitor.getMapa() != 0) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);

    glBegin(GL_QUADS);
    // Horizon (Light Powder Blue)
    glColor3f(0.8, 0.95, 1.0);
    glVertex2f(0, 0);
    glVertex2f(1, 0);
    // Zenith (Bright Sky Blue)
    glColor3f(0.3, 0.7, 1.0);
    glVertex2f(1, 1);
    glVertex2f(0, 1);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Draw stylized "Mario" clouds
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-10, 10, -10, 10, -1, 1);

    glColor3f(1.0, 1.0, 1.0);
    auto drawCloud = [](float x, float y, float size) {
      glBegin(GL_TRIANGLE_FAN);
      glVertex2f(x, y);
      for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(x + cos(angle) * size, y + sin(angle) * size * 0.7);
      }
      glEnd();

      glBegin(GL_TRIANGLE_FAN);
      glVertex2f(x + size * 0.8, y - size * 0.2);
      for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(x + size * 0.8 + cos(angle) * size * 0.7,
                   y - size * 0.2 + sin(angle) * size * 0.5);
      }
      glEnd();

      glBegin(GL_TRIANGLE_FAN);
      glVertex2f(x - size * 0.8, y - size * 0.2);
      for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(x - size * 0.8 + cos(angle) * size * 0.7,
                   y - size * 0.2 + sin(angle) * size * 0.5);
      }
      glEnd();
    };

    // Nubes anuladas por rendimiento CPU/GPU
    // drawCloud(-6, 7, 1.0);
    // drawCloud(-2, 8, 1.2);
    // drawCloud(4, 6, 0.9);
    // drawCloud(8, 7.5, 1.1);

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_FOG); // Desactivado por falta de configuración (evita pantalla negra)
  }
  // -------------------------

  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_DEPTH_TEST);

  glPushMatrix();
  if (monitor.getMapa() != 0) {
    if (monitor.getMapa() != 0 and MMmode2 != 2) { // 2: Ninguno
      if (camMode == 0) {
        monitor.getMapa()->setDrawGridFP(!CheckGridFP);
        monitor.getMapa()->drawFirstPerson(
            MMmode2, monitor.get_active_objetivos(), monitor.getLevel());
      } else
        monitor.getMapa()->drawZenithal(MMmode2, monitor.get_active_objetivos(),
                                        monitor.getLevel(), zoomZenithal);
    }
  }
  glPopMatrix();

  // --- RENDERIZAR HUD (SOBRE EL 3D) ---
  if (monitor.getMapa() != 0 and monitor.numero_entidades() > 0) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Bottom HUD Panel
    float hudY = height * 0.85f;
    float hudH = height * 0.15f;
    // HUD background removed per user request to eliminate "shadow" effect

    /* HUD Elements removed per user request for a completely clean view
    Entidad* eR = monitor.get_entidad(0);
    Entidad* eA = monitor.get_entidad(1);

    // Stats
    char buf[128];
    float batR = (float)eR->getBateria() / 3000.0f;
    float batA = (float)eA->getBateria() / 3000.0f;

    // ingeniero
    renderText(20, hudY + 25, "I (ingeniero)", GLUT_BITMAP_HELVETICA_12,
    0.0, 1.0, 1.0); renderBar(20, hudY + 35, 120, 10, batR, (batR > 0.4 ? 0.0
    : 1.0), (batR > 0.15 ? 1.0 : 0.0), 0.0); sprintf(buf, "E:%d F:%d C:%d",
    eR->getBateria(), eR->getFil(), eR->getCol()); renderText(20, hudY + 60,
    buf, GLUT_BITMAP_HELVETICA_10, 1.0, 1.0, 1.0);

    // Tecnico
    renderText(width - 140, hudY + 25, "T (TECNICO)",
    GLUT_BITMAP_HELVETICA_12, 1.0, 0.8, 0.0); renderBar(width - 140, hudY + 35,
    120, 10, batA, (batA > 0.4 ? 0.0 : 1.0), (batA > 0.15 ? 1.0 : 0.0), 0.0);
    sprintf(buf, "E:%d F:%d C:%d", eA->getBateria(), eA->getFil(),
    eA->getCol()); renderText(width - 140, hudY + 60, buf,
    GLUT_BITMAP_HELVETICA_10, 1.0, 1.0, 1.0);

    // Global
    sprintf(buf, "TIME: %d", eR->getInstantesPendientes());
    renderText(width/2 - 40, hudY + 30, buf, GLUT_BITMAP_HELVETICA_18, 0.9, 0.9,
    0.1); sprintf(buf, "Score: %d", eR->getPuntuacion()); renderText(width/2 -
    35, hudY + 60, buf, GLUT_BITMAP_HELVETICA_12, 1.0, 1.0, 0.0);
    */

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }

  glFlush();
  glutSwapBuffers();
}

/**
 * @brief Callback de refresco de la subventana de minimapa (vista 3D lateral).
 *
 * Renderiza el minimapa 2D del mundo Belkan con los objetivos activos
 * y la topología según los modos @c MMmode y @c MMmode4.
 */
void display_vistMiniMapa() {
  ResetViewport();

  glMatrixMode(GL_MODELVIEW);
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();

  glDisable(GL_LIGHTING);
  glColor3f(1.0, 1.0, 1.0);
  glPushMatrix();
  if (monitor.getMapa() != 0) {
    monitor.getMapa()->drawMM2(monitor.get_active_objetivos(),
                               monitor.getLevel(), MMmode4 == 1, MMmode);
  }
  glPopMatrix();
  glFlush();
  glutSwapBuffers();
}

/**
 * @brief Callback de refresco del panel de información de agentes.
 *
 * Dibuja en modo 2D las barras de batería, posición, orientación, última
 * acción y vectores de visión (terreno, superficies, alturas) del Ingeniero
 * y del Técnico. También muestra el tiempo restante, la puntuación, los
 * objetivos de nivel 7 y el impacto ecológico acumulado.
 */
void display_vistInfoAgentes() {
  ResetViewport2D();
  glClearColor(0.12, 0.12, 0.12, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  int id = 0;

  if (monitor.getMapa() != 0 && monitor.numero_entidades() > (unsigned int)id) {
    Entidad *e = monitor.get_entidad(id);
    char buf[256];
    float w = glutGet(GLUT_WINDOW_WIDTH);

    // Fila 1: Titulo, Valor Bateria y Barra
    renderText(10, 15, "INGENIERO", GLUT_BITMAP_HELVETICA_12, 1.0, 0.0, 0.0);
    int bateria = e->getBateria();
    float batPercent = (float)bateria / monitor.getEnergiaInicial();
    sprintf(buf, "Ene: %d", bateria);
    if (e->getBateria() != 0)
      renderText(w * 0.39f, 15, buf, GLUT_BITMAP_HELVETICA_10, 1.0, 1.0, 1.0);
    else
      renderText(w * 0.39f, 15, buf, GLUT_BITMAP_HELVETICA_10, 1.0, 0.0, 0.0);

    renderBar(w * 0.57f, 7, w * 0.4f, 10, batPercent,
              (batPercent > 0.4 ? 0.0 : 1.0), (batPercent > 0.15 ? 1.0 : 0.0),
              0.0);

    // Fila 2: Posicion, Rumbo, Accion y Zapatillas
    const char *orientaciones[] = {"N", "NE", "E", "SE", "S", "SO", "O", "NO"};
    sprintf(buf, "Pos:%d,%d [%s] Acc:%s %s", e->getFil(), e->getCol(),
            orientaciones[e->getOrientacion()],
            monitor.strAccion(e->getLastAction()).c_str(),
            (e->Has_Zapatillas() ? "[ZAP]" : ""));
    renderText(10, 30, buf, GLUT_BITMAP_HELVETICA_10, 0.9, 0.9, 0.9);

    // Sensores de vision
    auto vision = e->getVision();
    if (vision.size() >= 3) {
      // Fila 3: Entidades (visionAux[1])
      string sEntities = "Sup:";
      for (int i = 0; i < vision[1].size(); i++) {
        if (i == 1 or i == 4 or i == 9)
          sEntities += ' ';
        sEntities += vision[1][i];
      }
      renderText(10, 45, sEntities.c_str(), GLUT_BITMAP_8_BY_13, 0.6, 0.9, 1.0);

      // Fila 4: Terreno (visionAux[0])
      string sTerrain = "Ter:";
      for (int i = 0; i < vision[0].size(); i++) {
        if (i == 1 or i == 4 or i == 9)
          sTerrain += ' ';
        sTerrain += vision[0][i];
      }
      renderText(10, 58, sTerrain.c_str(), GLUT_BITMAP_8_BY_13, 0.8, 0.8, 1.0);

      // Fila 5: Alturas (visionAux1[0])
      string sHeights = "Alt:";
      for (int i = 0; i < vision[2].size(); i++) {
        if (i == 1 or i == 4 or i == 9)
          sHeights += ' ';
        sHeights += vision[2][i] + '0';
      }
      renderText(10, 71, sHeights.c_str(), GLUT_BITMAP_8_BY_13, 0.7, 0.7, 0.7);
    }

    // Tiempo restante y Puntuacion
    int tiemporestante = e->getInstantesPendientes();
    sprintf(buf, "Iteraciones Restantes: %d  ", tiemporestante);
    if (tiemporestante != 0)
      renderText(90, 95, buf, GLUT_BITMAP_HELVETICA_12, 0.6, 0.9, 1.0);
    else
      renderText(90, 95, buf, GLUT_BITMAP_HELVETICA_12, 1.0, 0.0, 0.0);

    // Tiempo restante y Puntuacion
    if (monitor.getLevel() == 7) {
      int puntuacion = e->getPuntuacion();
      int misiones = e->getMisiones();
      sprintf(buf, "Score: %d   Misiones: %d", puntuacion, misiones);

      if (monitor.finJuego())
        renderText(60, 115, buf, GLUT_BITMAP_HELVETICA_12, 1.0, 0.0, 0.0);
      else
        renderText(60, 115, buf, GLUT_BITMAP_HELVETICA_12, 0.6, 0.9, 1.0);
    }

    sprintf(buf, "Imp. Eco: %d (%d)", monitor.getImpactoEcologico(),
            monitor.getMaxImpacto());
    if (monitor.getImpactoEcologico() >= monitor.getMaxImpacto())
      renderText(75, 128, buf, GLUT_BITMAP_HELVETICA_12, 1.0, 0.0, 0.0);
    else
      renderText(75, 128, buf, GLUT_BITMAP_HELVETICA_12, 1.0, 0.5, 0.0);

    double totalTime = (monitor.get_entidad(0)->getTiempo() +
                        monitor.get_entidad(1)->getTiempo()) /
                       CLOCKS_PER_SEC;
    sprintf(buf, "Tiempo: %.2fs", totalTime);
    renderText(75, 142, buf, GLUT_BITMAP_HELVETICA_12, 0.6, 0.9, 1.0);
  }

  int desplazamiento_vertical = 155;
  id = 1;
  if (monitor.getMapa() != 0 && monitor.numero_entidades() > (unsigned int)id) {
    Entidad *e = monitor.get_entidad(id);
    char buf[256];
    float w = glutGet(GLUT_WINDOW_WIDTH);

    // Fila 1: Valor Bateria y Barra
    renderText(10, desplazamiento_vertical + 15, "TECNICO",
               GLUT_BITMAP_HELVETICA_12, 1.0, 0.4, 0.0);
    int bateria = e->getBateria();
    float batPercent = (float)bateria / monitor.getEnergiaInicial();
    sprintf(buf, "Ene: %d", bateria);
    if (e->getBateria() != 0)
      renderText(w * 0.39f, desplazamiento_vertical + 15, buf,
                 GLUT_BITMAP_HELVETICA_10, 1.0, 1.0, 1.0);
    else
      renderText(w * 0.39f, desplazamiento_vertical + 15, buf,
                 GLUT_BITMAP_HELVETICA_10, 1.0, 0.0, 0.0);
    renderBar(w * 0.57f, desplazamiento_vertical + 7, w * 0.4f, 10, batPercent,
              (batPercent > 0.4 ? 0.0 : 1.0), (batPercent > 0.15 ? 1.0 : 0.0),
              0.0);

    // Fila 2: Posicion, Rumbo, Accion y Zapatillas
    const char *orientaciones[] = {"N", "NE", "E", "SE", "S", "SO", "O", "NO"};
    sprintf(buf, "Pos:%d,%d [%s] Acc:%s %s", e->getFil(), e->getCol(),
            orientaciones[e->getOrientacion()],
            monitor.strAccion(e->getLastAction()).c_str(),
            (e->Has_Zapatillas() ? "[ZAP]" : ""));
    renderText(10, desplazamiento_vertical + 30, buf, GLUT_BITMAP_HELVETICA_10,
               0.9, 0.9, 0.9);

    // Sensores de vision
    auto vision = e->getVision();
    if (vision.size() >= 3) {
      // Fila 3: Entidades (visionAux[1])
      string sEntities = "Sup:";
      for (int i = 0; i < vision[1].size(); i++) {
        if (i == 1 or i == 4 or i == 9)
          sEntities += ' ';
        sEntities += vision[1][i];
      }
      renderText(10, desplazamiento_vertical + 45, sEntities.c_str(),
                 GLUT_BITMAP_8_BY_13, 0.6, 0.9, 1.0);

      // Fila 4: Terreno (visionAux[0])
      string sTerrain = "Ter:";
      for (int i = 0; i < vision[0].size(); i++) {
        if (i == 1 or i == 4 or i == 9)
          sTerrain += ' ';
        sTerrain += vision[0][i];
      }
      renderText(10, desplazamiento_vertical + 58, sTerrain.c_str(),
                 GLUT_BITMAP_8_BY_13, 0.8, 0.8, 1.0);

      // Fila 5: Alturas (visionAux1[0])
      string sHeights = "Alt:";
      for (int i = 0; i < vision[2].size(); i++) {
        if (i == 1 or i == 4 or i == 9)
          sHeights += ' ';
        sHeights += vision[2][i] + '0';
      }
      renderText(10, desplazamiento_vertical + 71, sHeights.c_str(),
                 GLUT_BITMAP_8_BY_13, 0.7, 0.7, 0.7);
    }
  }
  glutSwapBuffers();
}

/**
 * @brief Callback de refresco del panel de información general.
 *
 * Delega en @ref display_vistInfoAgentes. Mantenido como punto de extensión
 * para añadir información adicional en el futuro.
 */
void display_vistInfo() { display_vistInfoAgentes(); }

/**
 * @brief Callback de refresco del panel de mensajes del sistema.
 *
 * Renderiza las últimas líneas del log de mensajes del monitor con
 * soporte de scroll. Muestra un máximo de 3 líneas visibles a la vez
 * y dibuja una barra de desplazamiento cuando hay más contenido.
 */
void display_vistMensajes() {
  ResetViewport2D();
  glClearColor(0.08, 0.08, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  const auto &messages = monitor.getMensajes();
  int numMsgs = messages.size();
  int scroll = monitor.getScrollMensajes();

  // Line height 18. Window height 65. Fits ~3 messages.
  int visibleLimit = 3;

  // Ensure scroll is within bounds
  if (scroll < 0)
    scroll = 0;
  if (numMsgs > visibleLimit) {
    if (scroll > numMsgs - visibleLimit)
      scroll = numMsgs - visibleLimit;
  } else {
    scroll = 0;
  }
  monitor.setScrollMensajes(scroll);

  float y = 15;
  int count = 0;
  for (int i = scroll; i < numMsgs && count < visibleLimit; ++i, ++count) {
    renderText(10, y, messages[i].c_str(), GLUT_BITMAP_HELVETICA_12, 1.0, 0.7,
               0.3);
    y += 18;
  }

  // Draw scrollbar if needed
  if (numMsgs > visibleLimit) {
    float barHeight = 65.0f * visibleLimit / numMsgs;
    float barPos = 65.0f * scroll / numMsgs;
    renderRect(688, barPos, 10, barHeight, 0.4, 0.4, 0.5,
               0.8); // Scrollbar handle
  }

  glutSwapBuffers();
}

/**
 * @brief Callback de ratón para la subventana de mensajes.
 *
 * Gestiona el scroll de la lista de mensajes mediante la rueda del ratón:
 * botón 3 (rueda arriba) sube un mensaje, botón 4 (rueda abajo) baja uno.
 *
 * @param button  Botón del ratón pulsado (GLUT_LEFT_BUTTON, 3, 4, …).
 * @param state   Estado del botón (GLUT_DOWN / GLUT_UP).
 * @param x       Coordenada X del cursor al pulsar.
 * @param y       Coordenada Y del cursor al pulsar.
 */
void mouseClickMensajes(int button, int state, int x, int y) {
  if (state == GLUT_DOWN) {
    int scroll = monitor.getScrollMensajes();
    if (button == 3) { // Wheel UP
      if (scroll > 0)
        monitor.setScrollMensajes(scroll - 1);
    } else if (button == 4) { // Wheel DOWN
      const auto &messages = monitor.getMensajes();
      if (scroll < (int)messages.size() - 3)
        monitor.setScrollMensajes(scroll + 1);
    }
    glutPostRedisplay();
  }
}

/**
 * @brief Callback de refresco del panel de interfaz de usuario (controles GLUI).
 *
 * Actualiza el viewport y llama a GLUI para que gestione sus propios controles.
 * El contenido textual se ha migrado a la subventana de información de agentes.
 */
void display_vistIU() {
  ResetViewport();
  GLUI_Master.auto_set_viewport();
  std::vector<std::string> strs;
  string str;

  if (monitor.getMapa() != 0 and monitor.numero_entidades() > 0) {
    /* Las etiquetas de informacion se han sustituido por ventanas graficas
    str = monitor.toString();
    Descomponer(str, strs);
    // ... loop removed
    */
  }
  glutSwapBuffers();
}

/**
 * @brief Callback de temporizador GLUT: fuerza el redibujado de todas las subventanas.
 *
 * Marca como sucias todas las subventanas (mapa, minimapa, IU, info, mensajes)
 * para que GLUT las repinte en el siguiente ciclo. También sincroniza el
 * objetivo activo con los controles de la IU y llama a @ref irAlJuego para
 * avanzar un paso de simulación.
 *
 * @param valor  Parámetro de valor del temporizador (no utilizado).
 */
void update(int valor) {
  glutSetWindow(vistaPrincipal);
  glutPostRedisplay();

  glutSetWindow(vistaMiniMapa);
  glutPostRedisplay();

  glutSetWindow(vistaIU);
  glutPostRedisplay();

  glutSetWindow(vistaInfo);
  glutPostRedisplay();

  glutSetWindow(vistaMensajes);
  glutPostRedisplay();

  // glutKeyboardFunc(keyboard);

  if (monitor.getMapa() != 0) {
    if (ObjFil1 != PosFila or ObjCol1 != PosColumna) {
      monitor.get_n_active_objetivo(0, PosFila, PosColumna);

      ObjFil1 = PosFila;
      ObjCol1 = PosColumna;
    }


    if (monitor.getLevel() < 2) {
      if (editPosFila)
        editPosFila->disable();
      if (editPosColumna)
        editPosColumna->disable();
    } else {
      if (editPosFila)
        editPosFila->enable();
      if (editPosColumna)
        editPosColumna->enable();
    }
  }

  // glutTimerFunc(1, irAlJuego, 0);
  irAlJuego(0);
}

/**
 * @brief Avanza la simulación un paso desde la interfaz gráfica.
 *
 * Invoca @ref lanzar_motor_juego. Si la simulación ha terminado, desactiva
 * los controles de ejecución (Paso, Ejecución, Ejecutar) y activa el
 * botón de selección de mapa. Finalmente programa el siguiente ciclo de
 * actualización mediante @c glutTimerFunc.
 *
 * @param valor  Parámetro del temporizador (no utilizado).
 */
void irAlJuego(int valor) {
  if (lanzar_motor_juego(-1)) {
    botonElegirMapa->enable();
    botonPaso->disable();
    botonEjecucion->disable();
    botonEjecutar->disable();
    botonSalir->disable();

    editTextPasos->disable();
    editTextRetardo->disable();
    botonSalir->enable();
  }
  // glutKeyboardFunc(keyboard);
  glutTimerFunc(1, update, 0);
}

/**
 * @brief Callback del botón "Cancelar" en el diálogo de selección de mapa.
 *
 * Cierra el panel modal de selección de mapa y reactiva los controles
 * principales (elegir mapa, salir).
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonCancelarNuevoMapaCB(int valor) {
  panelSelecMapa->close();

  botonElegirMapa->enable();
  botonSalir->enable();
}

/**
 * @brief Callback del botón "Aceptar" en el diálogo de selección de mapa.
 *
 * Carga el mapa seleccionado en el listbox, valida las posiciones iniciales
 * del Ingeniero y el Técnico, configura el objetivo activo y arranca el juego
 * al nivel seleccionado. Reactiva los controles de simulación y cierra el panel.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonAceptarNuevoMapaCB(int valor) {

  botonElegirMapa->enable();
  monitor.clearMensajes();
  if (monitor.getLevel() == 2 or monitor.getLevel() == 3 or
      monitor.getLevel() == 4 or monitor.getLevel() == 5) {
    listboxMap->disable();
  }

  listboxMap->enable();
  if (botonPaso)
    botonPaso->enable();
  if (botonEjecucion)
    botonEjecucion->enable();
  if (botonEjecutar)
    botonEjecutar->enable();
  botonSalir->enable();
  if (monitor.getLevel() < 2) {
    if (editPosColumna)
      editPosColumna->disable();
    if (editPosFila)
      editPosFila->disable();
  } else {
    if (editPosColumna)
      editPosColumna->enable();
    if (editPosFila)
      editPosFila->enable();
  }
  editTextPasos->enable();
  editTextRetardo->disable();

  int posF, posC, orienta;
  int SposF, SposC, Sorienta;

  ultimomapaPos = listbox->get_int_val();

  if (ultimomapaPos >= 0) {
    char path[255];
    strcpy(path, "./mapas/");
    const char *file = listbox->curr_text.c_str();
    monitor.setMapa(strcat(path, file));

    monitor.reset_objetivos();

    // posicion del agente y del colaborador
    if (monitor.numero_entidades() > 1) {
      posC = monitor.get_entidad(0)->getCol();
      posF = monitor.get_entidad(0)->getFil();
      orienta = monitor.get_entidad(0)->getOrientacion();
      if (!monitor.is_a_valid_cell_like_goal(posF, posC)) {
        monitor.generate_a_valid_cell(posF, posC, orienta);
      }

      SposC = monitor.get_entidad(1)->getCol();
      SposF = monitor.get_entidad(1)->getFil();
      Sorienta = monitor.get_entidad(1)->getOrientacion();
      if (!monitor.is_a_valid_cell_like_goal(SposF, SposC)) {
        monitor.generate_a_valid_cell(SposF, SposC, Sorienta);
      }
    } else if (monitor.numero_entidades() > 0) {
      posC = monitor.get_entidad(0)->getCol();
      posF = monitor.get_entidad(0)->getFil();
      orienta = monitor.get_entidad(0)->getOrientacion();
      if (!monitor.is_a_valid_cell_like_goal(posF, posC)) {
        monitor.generate_a_valid_cell(posF, posC, orienta);
      }
      monitor.generate_a_valid_cell(SposF, SposC, Sorienta);
    } else {
      monitor.generate_a_valid_cell(posF, posC, orienta);
      monitor.generate_a_valid_cell(SposF, SposC, Sorienta);
    }

    ultimonivel = group->get_int_val();

    int f = 0, c = 0;
    if (monitor.getMapa() != 0 && monitor.there_are_active_objetivo()) {
      monitor.get_n_active_objetivo(0, f, c);
      monitor.put_a_new_objetivo_front(f, c);
    } else if (monitor.getMapa() != 0)
      monitor.generate_a_objetive();

    monitor.setMaxImpacto(ObjUmbEco);
    monitor.setEnergiaInicial(ObjEnergia);
    monitor.startGame(ultimonivel, ObjInstSim);
    if (monitor.inicializarJuego()) {
      monitor.inicializar(posF, posC, orienta, SposF, SposC, Sorienta);
      tMap = monitor.juegoInicializado();
      if (ultimonivel == 0 or ultimonivel == 1 or ultimonivel == 6) {
        MMmode = 3;
        listboxMap->set_int_val(3);
        listboxMap->enable();
      }
    }
  } else {
    cout << "No se puede leer el fichero..." << endl;
  }

  panelSelecMapa->close();
}

/**
 * @brief Lista los ficheros de un directorio que coincidan con una extensión.
 *
 * @param name       Ruta del directorio a explorar.
 * @param extension  Cadena de extensión a filtrar (p.ej. ".map").
 * @return Vector de nombres de fichero ordenado alfabéticamente.
 */
vector<string> getFilesList(string name, string extension) {
  vector<string> v;

  DIR *dirp = opendir(name.c_str());
  if (dirp == NULL)
    return v;
  struct dirent *dp = readdir(dirp);
  while (dp) {
    string fname(dp->d_name);
    if (fname.find(extension, 0) < fname.size()) {
      v.push_back(fname);
    }
    dp = readdir(dirp);
  }
  closedir(dirp);
  sort(v.begin(), v.end());

  return v;
}

/**
 * @brief Callback del botón "Ok" del diálogo de configuración avanzada.
 *
 * Aplica los valores de semilla, posiciones iniciales de ambos agentes,
 * casilla objetivo, instantes, energía y umbral ecológico introducidos en
 * los spinners de configuración y lanza el juego con esos parámetros.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonConfigurarSimOK(int valor) {
  botonElegirMapa->enable();
  // listboxMap->disable();
  listbox3D->enable();
  botonPaso->enable();
  botonEjecucion->enable();
  botonEjecutar->enable();
  botonSalir->enable();
  editTextPasos->enable();
  editTextRetardo->disable();
  if (ultimonivel > 1) {
    if (editPosFila)
      editPosFila->disable();
    if (editPosColumna)
      editPosColumna->disable();
  }

  srand(setup1->get_int_val());
  monitor.setMaxImpacto(setup_eco->get_int_val());
  monitor.setEnergiaInicial(setup_ener->get_int_val());
  monitor.startGame(ultimonivel, setup_inst->get_int_val());
  monitor.inicializar();

  monitor.set_n_active_objetivo(0, setup5->get_int_val(), setup4->get_int_val());
  int f, c;
  monitor.get_n_active_objetivo(0, f, c);
  ObjFil1 = PosFila = f;
  ObjCol1 = PosColumna = c;

  monitor.get_entidad(0)->setPosicion(setup30->get_int_val(),
                                      setup20->get_int_val());
  monitor.get_entidad(0)->setOrientacion(
      static_cast<Orientacion>(setup60->get_int_val()));
  monitor.get_entidad(1)->setPosicion(setup31->get_int_val(),
                                      setup21->get_int_val());
  monitor.get_entidad(1)->setOrientacion(
      static_cast<Orientacion>(setup61->get_int_val()));
}

/**
 * @brief Callback del botón "Cancelar" del diálogo de configuración avanzada.
 *
 * Aplica la configuración actual (llama a @ref botonConfigurarSimOK),
 * resetea el modo de minimapa y cierra el panel de configuración.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonConfigurarSimCANCEL(int valor) {
  botonConfigurarSimOK(1);
  MMmode = 0;
  panelSelecMapaConfig->close();
}

/**
 * @brief Callback del botón "Ok y Configurar" en el diálogo de selección de mapa.
 *
 * Carga el mapa seleccionado, genera posiciones válidas para ambos agentes,
 * arranca el juego y abre el panel avanzado de configuración de los parámetros
 * de simulación (semilla, posiciones, objetivo, instantes, energía, umbral).
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonConfigurarNuevoMapaCB(int valor) {
  /* LLamada para cargar el mapa */

  botonElegirMapa->disable();
  listboxMap->disable();
  listbox3D->disable();
  if (botonPaso)
    botonPaso->disable();
  if (botonEjecucion)
    botonEjecucion->disable();
  if (botonEjecutar)
    botonEjecutar->disable();
  botonSalir->enable();
  // editPosColumna->disable();
  // editPosFila->disable();
  editTextPasos->disable();
  editTextRetardo->disable();

  int item = listbox->get_int_val();
  static int posC, posF, orienta, semilla;
  static int SONposC, SONposF, SONorienta;

  if (item > 0) {
    char path[255];
    strcpy(path, "./mapas/");
    const char *file = listbox->curr_text.c_str();
    monitor.reset_objetivos();
    monitor.setMapa(strcat(path, file));

    int nivel = group->get_int_val();

    /* Paso las variables de monitor a variables locales a este metodo */
    semilla = monitor.get_semilla();

    if (monitor.numero_entidades() > 1) {
      posC = monitor.get_entidad(0)->getCol();
      posF = monitor.get_entidad(0)->getFil();
      orienta = monitor.get_entidad(0)->getOrientacion();
      if (!monitor.is_a_valid_cell_like_goal(posF, posC)) {
        monitor.generate_a_valid_cell(posF, posC, orienta);
      }
      SONposC = monitor.get_entidad(1)->getCol();
      SONposF = monitor.get_entidad(1)->getFil();
      SONorienta = monitor.get_entidad(1)->getOrientacion();
      while (!monitor.is_a_valid_cell_like_goal(SONposC, SONposF) or
             (posF == SONposF and posC == SONposC)) {
        monitor.generate_a_valid_cell(SONposC, SONposF, orienta);
      }
    } else {
      do {
        monitor.generate_a_valid_cell(posF, posC, orienta);
        monitor.generate_a_valid_cell(SONposF, SONposC, SONorienta);
      } while (posF == SONposF and posC == SONposC);
    }

    int nObj = 1;
    int f = 0, c = 0, kk;

    if (monitor.getMapa() != 0 && monitor.there_are_active_objetivo()) {
      monitor.get_n_active_objetivo(0, f, c);
    }

    ObjFil1 = f;
    ObjCol1 = c;

    if (monitor.getMapa() != 0 && monitor.is_a_valid_cell_like_goal(f, c))
      monitor.put_a_new_objetivo_front(f, c);
    else if (monitor.getMapa() != 0)
      monitor.generate_a_objetive();

    monitor.startGame(nivel);
    if (monitor.inicializarJuego()) {
      monitor.inicializar(posF, posC, orienta, SONposF, SONposC, SONorienta,
                          semilla);
      tMap = monitor.juegoInicializado();
      if (nivel >= 6) {
        MMmode = 3;
        listboxMap->set_int_val(3);
        listboxMap->enable();
      } else if (nivel == 0 or nivel == 1) {
        MMmode = 3;
        listboxMap->set_int_val(3);
        listboxMap->enable();
      }
    }
  } else {
    cout << "No se puede leer el fichero..." << endl;
  }

  MMmode = 0;
  panelSelecMapaConfig = GLUI_Master.create_glui("Configurar Mapa");

  int num_filas = 50; // default value
  int num_col = 50;   // default value

  if (monitor.getMapa() != 0) {
    num_filas = monitor.getMapa()->getNFils();
    num_col = monitor.getMapa()->getNCols();
  }

  GLUI_Panel *obj_panel = panelSelecMapaConfig->add_panel("Setup");
  setup1 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_panel, "                Semilla ", GLUI_SPINNER_INT, &semilla);

  GLUI_Panel *obj_subpanel0 =
      panelSelecMapaConfig->add_panel_to_panel(obj_panel, "Pos Ingeniero");
  setup30 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel0, "            Fila ", GLUI_SPINNER_INT, &posF);
  setup30->set_int_limits(0, num_filas, GLUI_LIMIT_WRAP);
  setup20 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel0, "     Columna ", GLUI_SPINNER_INT, &posC);
  setup20->set_int_limits(0, num_col, GLUI_LIMIT_WRAP);
  setup60 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel0, " Orientacion ", GLUI_SPINNER_INT, &orienta);
  setup60->set_int_limits(0, 8, GLUI_LIMIT_WRAP);

  GLUI_Panel *obj_subpanel1 =
      panelSelecMapaConfig->add_panel_to_panel(obj_panel, "Pos Tecnico");
  setup31 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel1, "            Fila ", GLUI_SPINNER_INT, &SONposF);
  setup31->set_int_limits(0, num_filas, GLUI_LIMIT_WRAP);
  setup21 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel1, "     Columna ", GLUI_SPINNER_INT, &SONposC);
  setup21->set_int_limits(0, num_col, GLUI_LIMIT_WRAP);
  setup61 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel1, " Orientacion ", GLUI_SPINNER_INT, &SONorienta);
  setup61->set_int_limits(0, 8, GLUI_LIMIT_WRAP);

  GLUI_Panel *obj_subpanel2 =
      panelSelecMapaConfig->add_panel_to_panel(obj_panel, "Casilla Belkanita");
  setup5 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel2, "        Fil ", GLUI_SPINNER_INT, &ObjFil1);
  setup5->set_int_limits(0, num_filas, GLUI_LIMIT_WRAP);
  setup4 = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel2, "    Col  ", GLUI_SPINNER_INT, &ObjCol1);
  setup4->set_int_limits(0, num_col, GLUI_LIMIT_WRAP);

  GLUI_Panel *obj_subpanel3 =
      panelSelecMapaConfig->add_panel_to_panel(obj_panel, "Costes");
  setup_inst = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel3, "Instantes ", GLUI_SPINNER_INT, &ObjInstSim);
  setup_inst->set_int_limits(1, 10000);
  setup_ener = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel3, "Energia   ", GLUI_SPINNER_INT, &ObjEnergia);
  setup_ener->set_int_limits(1, 1000000);
  setup_eco = panelSelecMapaConfig->add_spinner_to_panel(
      obj_subpanel3, "Umbral Eco", GLUI_SPINNER_INT, &ObjUmbEco);
  setup_eco->set_int_limits(1, 1000000);
  panelSelecMapaConfig->add_button("Ok", 1, botonConfigurarSimOK);
  panelSelecMapaConfig->add_button("Finish", 1, botonConfigurarSimCANCEL);

  panelSelecMapa->close();
}

/**
 * @brief Callback del botón "Elegir Mapa" de la barra de controles principal.
 *
 * Abre el panel modal de selección de mapa y nivel. Desactiva temporalmente
 * los controles de simulación (Paso, Ejecución, Ejecutar, Salir) y pobla
 * el listbox con los ficheros @c .map disponibles en la carpeta @c mapas/.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonElegirMapaCB(int valor) {

  panelSelecMapa = GLUI_Master.create_glui("Elige mapa y nivel");

  botonElegirMapa->disable();
  if (botonPaso)
    botonPaso->disable();
  if (botonEjecucion)
    botonEjecucion->disable();
  if (botonEjecutar)
    botonEjecutar->disable();
  botonSalir->disable();
  // editPosColumna->disable();
  // editPosFila->disable();
  editTextPasos->disable();
  editTextRetardo->disable();

  listbox = panelSelecMapa->add_listbox("Mapa  ", &ultimomapaPos);

  int i = 1;
  vector<string> filesPaths = getFilesList("mapas/", ".map");
  vector<string>::const_iterator it = filesPaths.begin();
  while (it != filesPaths.end()) {
    listbox->add_item(i++, (*it).c_str());
    it++;
  }

  listbox->set_int_val(ultimomapaPos);

  GLUI_Panel *obj_panel = panelSelecMapa->add_panel("Nivel");
  group = panelSelecMapa->add_radiogroup_to_panel(obj_panel, &ultimonivel);
  panelSelecMapa->add_radiobutton_to_group(
      group, "Nivel 0: [R] Rastreadores de Alcantarillas   ");
  panelSelecMapa->add_radiobutton_to_group(
      group, "Nivel 1: [R] De Reconocimiento por la Naturaleza  ");
  panelSelecMapa->add_radiobutton_to_group(
      group, "Nivel 2: [D] Corre, Ingeniero, Corre!   ");
  panelSelecMapa->add_radiobutton_to_group(
      group, "Nivel 3: [D] El Tecnico mide sus esfuerzos       ");
  panelSelecMapa->add_radiobutton_to_group(
      group, "Nivel 4: [D] Planifica, Ingeniero, Planifica! ");
  panelSelecMapa->add_radiobutton_to_group(group,
                                           "Nivel 5: [M] A Poner Tuberías! ");
  panelSelecMapa->add_radiobutton_to_group(
      group, "Nivel 6: [M] ¿Que alguien ponga luz? ");

  panelSelecMapa->add_button("Ok", 1, botonAceptarNuevoMapaCB);
  panelSelecMapa->add_button("Ok y Configurar", 2, botonConfigurarNuevoMapaCB);
  panelSelecMapa->add_button("Salir", 3, botonCancelarNuevoMapaCB);
}

/**
 * @brief Callback del botón "Paso" (ejecución paso a paso).
 *
 * Fija el número de pasos pendientes a 1 para que el motor ejecute
 * únicamente un instante de simulación en el siguiente ciclo.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonPasoCB(int valor) { monitor.setPasos(1); }

/**
 * @brief Callback del botón "Ejecución" (ejecutar hasta el final).
 *
 * Fija el número de pasos pendientes al total de instantes de la simulación,
 * de forma que el motor ejecuta todos los ciclos restantes sin pausas.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonEjecucionCB(int valor) {
  monitor.setPasos(monitor.getInstantesInicial());
}

/**
 * @brief Callback del botón "Ejecutar" (ejecución de N pasos).
 *
 * Fija el número de pasos pendientes a @c nPasos, valor configurable
 * por el usuario en el campo de texto de la IU.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonEjecutarCB(int valor) { monitor.setPasos(nPasos); }

/**
 * @brief Callback del spinner de retardo entre pasos.
 *
 * Actualiza el retardo del motor con el valor actual del control @c tRetardo.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void setRetardo(int valor) { monitor.setRetardo(tRetardo); }

/**
 * @brief Callback del spinner de columna del objetivo.
 *
 * Actualiza el objetivo activo (posición 0) del monitor con la fila
 * y columna introducidas (@c PosFila, @c PosColumna), y refresca las
 * variables de seguimiento @c ObjFil1 y @c ObjCol1.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void setPosColumna(int valor) {
  // Cambiando la posición de la columna
  monitor.set_n_active_objetivo(0, PosFila, PosColumna);
  ObjFil1 = PosFila;
  ObjCol1 = PosColumna;
  ObjFil2 = 0;
}

/**
 * @brief Callback del spinner de fila del objetivo.
 *
 * Análogo a @ref setPosColumna. Actualiza el objetivo activo del monitor
 * con la nueva fila seleccionada.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void setPosFila(int valor) {
  // Cambiando la posición de la fila
  monitor.set_n_active_objetivo(0, PosFila, PosColumna);
  ObjFil1 = PosFila;
  ObjCol1 = PosColumna;
  ObjFil2 = 0;
}


/**
 * @brief Callback del checkbox de modo de cámara (Primera persona / Cenital).
 *
 * Cambia @c camMode entre 0 (primera persona) y 1 (cenital) según el
 * valor del checkbox @c Check1stPerZenital. Habilita o deshabilita el
 * spinner de zoom en función del modo seleccionado.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void cb1stPerZenital(int valor) {
  if (Check1stPerZenital)
    camMode = 0;
  else
    camMode = 1;
  // Enable zoom only in zenithal mode
  if (spinnerZoom) {
    if (camMode == 1)
      spinnerZoom->enable();
    else
      spinnerZoom->disable();
  }
}

/**
 * @brief Callback del checkbox de topología total del mapa.
 *
 * Alterna @c MMmode4 entre 0 (topología parcial, solo lo descubierto)
 * y 1 (topología completa del mapa visible).
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void cbTotalTopology(int valor) {
  if (CheckTotalTopology)
    MMmode4 = 0;
  else
    MMmode4 = 1;
}

/**
 * @brief Callback del botón "Salir".
 *
 * Cierra correctamente la sesión Belkan (@ref MonitorJuego::cerrarBelkan)
 * y termina el proceso con código de éxito.
 *
 * @param valor  Parámetro de valor de GLUI (no utilizado).
 */
void botonSalirCB(int valor) {
  monitor.cerrarBelkan();
  exit(0);
}

/**
 * @brief Devuelve la orientación siguiente en sentido horario.
 *
 * Recorre el ciclo: norte → noreste → este → sureste → sur → suroeste
 * → oeste → noroeste → norte.
 *
 * @param x  Orientación actual.
 * @return   Orientación inmediatamente siguiente en sentido horario.
 */
Orientacion Next(Orientacion x) {
  switch (x) {
  case norte:
    return noreste;
    break;
  case noreste:
    return este;
    break;
  case este:
    return sureste;
    break;
  case sureste:
    return sur;
    break;
  case sur:
    return suroeste;
    break;
  case suroeste:
    return oeste;
    break;
  case oeste:
    return noroeste;
    break;
  case noroeste:
    return norte;
    break;
  default:
    return norte; // no debería alcanzarse; el enum cubre los 8 casos
  }
}

/**
 * @brief Devuelve la orientación anterior en sentido antihorario.
 *
 * Recorre el ciclo inverso: norte → noroeste → oeste → suroeste → sur
 * → sureste → este → noreste → norte.
 *
 * @param x  Orientación actual.
 * @return   Orientación inmediatamente anterior en sentido antihorario.
 */
Orientacion Previous(Orientacion x) {
  switch (x) {
  case norte:
    return noroeste;
    break;
  case noroeste:
    return oeste;
    break;
  case oeste:
    return suroeste;
    break;
  case suroeste:
    return sur;
    break;
  case sur:
    return sureste;
    break;
  case sureste:
    return este;
    break;
  case este:
    return noreste;
    break;
  case noreste:
    return norte;
    break;
  default:
    return norte; // no debería alcanzarse; el enum cubre los 8 casos
  }
}

/**
 * @brief Callback de clic de ratón en la subventana del mapa 2D.
 *
 * - **Botón izquierdo**: establece la casilla clicada como nuevo objetivo
 *   activo y sincroniza los controles de fila/columna de la IU.
 * - **Botón derecho**: teletransporta al Ingeniero a la casilla clicada.
 * - **Botón central**: gira la orientación del Ingeniero 45º en sentido horario.
 *
 * Las coordenadas del clic se escalan del espacio de píxeles al espacio
 * del mapa usando el factor @c tMap.
 *
 * @param button  Botón del ratón (GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON, GLUT_MIDDLE_BUTTON).
 * @param state   Estado del botón (GLUT_DOWN / GLUT_UP).
 * @param x       Coordenada X del cursor en píxeles.
 * @param y       Coordenada Y del cursor en píxeles.
 */
void mouseClick(int button, int state, int x, int y) {
  int tx, ty, tw, th;
  GLUI_Master.get_viewport_area(&tx, &ty, &tw, &th);
  int nx = round(x * tMap * 1.0 / tw);
  int ny = round(y * tMap * 1.0 / th);

  if ((button == GLUT_LEFT_BUTTON) and (state == GLUT_DOWN)) {
    if ((nx >= 0) and (nx <= tMap - 1) and (ny >= 0) and (ny <= tMap - 1)) {
      if (editPosFila)
        editPosFila->set_int_val(ny);
      if (editPosColumna)
        editPosColumna->set_int_val(nx);
      setPosFila(ny);
      setPosColumna(nx);
    }
  } else if ((button == GLUT_RIGHT_BUTTON) and (state == GLUT_DOWN)) {
    if ((nx >= 0) and (nx <= tMap - 1) and (ny >= 0) and (ny <= tMap - 1)) {
      monitor.get_entidad(0)->setPosicion(ny, nx);
    }
  } else if ((button == GLUT_MIDDLE_BUTTON) and (state == GLUT_DOWN)) {
    if ((nx >= 0) and (nx <= tMap - 1) and (ny >= 0) and (ny <= tMap - 1)) {
      Orientacion bru = monitor.get_entidad(0)->getOrientacion();
      bru = (Orientacion)((bru + 1) % 8);
      monitor.get_entidad(0)->setOrientacion(bru);
    }
  }
}

/**
 * @brief Inicializa y lanza la interfaz gráfica del motor Belkan.
 *
 * Crea todas las subventanas GLUT (mapa principal, minimapa, información de
 * agentes, mensajes e interfaz de controles GLUI), registra todos los
 * callbacks de refresco, ratón y redimensionado, construye los paneles de
 * la IU y arranca el bucle principal de GLUT (@c glutMainLoop).
 *
 * También configura las entidades del monitor (Ingeniero, Técnico y NPCs),
 * inicializa el mapa por defecto si se proporcionó por línea de comandos,
 * y registra el primer temporizador de actualización.
 *
 * @param argc  Número de argumentos de la línea de comandos.
 * @param argv  Vector de cadenas con los argumentos de la línea de comandos.
 */
void lanzar_motor_grafico(int argc, char **argv) {
  glutInit(&argc, argv);
  // Mode Setting
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  // window size (+gap size)
  glutInitWindowSize(IU_ancho_ventana, IU_alto_ventana);
  float ratio = ((float)IU_ancho_ventana * 0.7) / (float)IU_alto_ventana;
  // Initial position
  glutInitWindowPosition(300, 0);

  // Main Window
  ventanaPrincipal = glutCreateWindow(
      "Practica 2: Agentes Deliberativos/Reactivos. Curso 25/26");
  // Main Window callback function
  glutReshapeFunc(reshape);
  glutDisplayFunc(display_ventPrincipal);

  // World Window and Display
  vistaPrincipal = glutCreateSubWindow(ventanaPrincipal, 0, 0,
                                       IU_ancho_ventana * 0.7, IU_alto_ventana);
  glutDisplayFunc(display_vistMiniMapa);
  glutMouseFunc(mouseClick);
  // glutKeyboardFunc(keyboard);

  // screen Window and Display
  vistaMiniMapa =
      glutCreateSubWindow(ventanaPrincipal, IU_ancho_ventana * 0.7, 0,
                          (IU_ancho_ventana * 0.3) + 1, IU_alto_ventana * 0.28);
  glutDisplayFunc(display_vistPrincipal);

  vistaIU = glutCreateSubWindow(
      ventanaPrincipal, IU_ancho_ventana * 0.7, IU_alto_ventana * 0.28,
      (IU_ancho_ventana * 0.3) + 1, IU_alto_ventana * 0.42);
  glutDisplayFunc(display_vistIU);

  vistaInfo = glutCreateSubWindow(
      ventanaPrincipal, IU_ancho_ventana * 0.7, IU_alto_ventana * 0.70,
      (IU_ancho_ventana * 0.3) + 1, IU_alto_ventana * 0.3);
  glutDisplayFunc(display_vistInfo);

  vistaMensajes = glutCreateSubWindow(ventanaPrincipal, 0, 703, 703, 65);
  glutDisplayFunc(display_vistMensajes);
  glutMouseFunc(mouseClickMensajes);

  glutInitWindowPosition(IU_ancho_ventana * 0.7, 50);
  glutInitWindowSize(50, 50);

  GLUI *panelIU =
      GLUI_Master.create_glui_subwindow(vistaIU, GLUI_SUBWINDOW_TOP);
  panelIU->set_main_gfx_window(vistaIU);

  botonElegirMapa = panelIU->add_button("Nuevo Juego", 0, botonElegirMapaCB);
  botonElegirMapa->set_alignment(GLUI_ALIGN_CENTER);

  GLUI_Panel *row_panel = panelIU->add_panel("", GLUI_PANEL_NONE);
  GLUI_Panel *obj_panel2 =
      panelIU->add_panel_to_panel(row_panel, "Visualizacion");

  MMmode2 = 0;
  listbox3D = panelIU->add_listbox_to_panel(obj_panel2, "3D View ", &MMmode2);
  listbox3D->add_item(0, "Ingeniero");
  listbox3D->add_item(1, "Tecnico");
  listbox3D->add_item(2, "Ninguno");
  listbox3D->enable();

  listboxMap = panelIU->add_listbox_to_panel(obj_panel2, "2D Map  ", &MMmode);
  listboxMap->add_item(0, "Ver");
  listboxMap->add_item(1, "Ingeniero");
  listboxMap->add_item(2, "Tecnico");
  listboxMap->add_item(3, "Conjunto");
  listboxMap->set_int_val(MMmode);
  listboxMap->enable();

  panelIU->add_checkbox_to_panel(obj_panel2, "Grid Off/On", &CheckGridFP);

  panelIU->add_column_to_panel(row_panel, false);

  GLUI_Panel *cam_panel = panelIU->add_panel_to_panel(row_panel, "Mode");
  Check1stPerZenital = 1;
  panelIU->add_checkbox_to_panel(cam_panel, "1stPer/Zenital",
                                 &Check1stPerZenital, -1, cb1stPerZenital);
  CheckTotalTopology = 1;
  panelIU->add_checkbox_to_panel(cam_panel, "Total/Topology",
                                 &CheckTotalTopology, -1, cbTotalTopology);
  spinnerZoom = panelIU->add_spinner_to_panel(cam_panel, "Zoom", GLUI_SPINNER_FLOAT, &zoomZenithal);
  spinnerZoom->set_float_limits(20.0f, 120.0f);
  if (camMode == 0) spinnerZoom->disable(); // FP mode by default

  // lineaVacia = panelIU->add_statictext("");

  GLUI_Panel *obj_panel = panelIU->add_panel("Ir a...");
  // editPosFila = panelIU->add_spinner_to_panel(obj_panel, "   Fila",
  // GLUI_SPINNER_INT, &PosFila); editPosColumna =
  // panelIU->add_spinner_to_panel(obj_panel, "Columna", GLUI_SPINNER_INT,
  // &PosColumna);

  if (monitor.getMapa() != 0) {
    if (!monitor.there_are_active_objetivo()) {
      monitor.put_active_objetivos(1);
    }
    monitor.get_n_active_objetivo(0, PosFila, PosColumna);
  }
  editPosFila = panelIU->add_edittext_to_panel(
      obj_panel, "Fila", GLUI_EDITTEXT_INT, &PosFila, -1, setPosFila);
  editPosFila->set_alignment(GLUI_ALIGN_LEFT);
  editPosFila->set_w(24);
  panelIU->add_column_to_panel(obj_panel, false);
  editPosColumna = panelIU->add_edittext_to_panel(
      obj_panel, "Col", GLUI_EDITTEXT_INT, &PosColumna, -1, setPosColumna);
  editPosColumna->set_alignment(GLUI_ALIGN_LEFT);
  editPosColumna->set_w(24);
  panelIU->add_column_to_panel(obj_panel, false);

  //   editPosFila->set_int_limits(0, 100, GLUI_LIMIT_WRAP);
  //   editPosColumna->set_int_limits(0, 100, GLUI_LIMIT_WRAP);

  // lineaVacia = panelIU->add_statictext("");

  GLUI_Panel *run_panel = panelIU->add_panel("Control");

  editTextPasos = panelIU->add_edittext_to_panel(run_panel, "Pasos",
                                                 GLUI_EDITTEXT_INT, &nPasos);
  editTextPasos->set_int_val(nPasos);
  editTextPasos->set_int_limits(1, 10000000, GLUI_LIMIT_CLAMP);
  editTextPasos->set_alignment(GLUI_ALIGN_CENTER);
  editTextPasos->set_w(24);

  editTextRetardo = panelIU->add_edittext_to_panel(
      run_panel, "Retardo", GLUI_EDITTEXT_INT, &tRetardo, -1, setRetardo);
  editTextRetardo->set_int_val(tRetardo);
  editTextRetardo->set_int_limits(0, 10000000, GLUI_LIMIT_CLAMP);
  editTextRetardo->set_alignment(GLUI_ALIGN_CENTER);
  editTextRetardo->set_w(24);
  setRetardo(tRetardo);

  panelIU->add_column_to_panel(run_panel, true);

  botonPaso = panelIU->add_button_to_panel(run_panel, "Paso", 0, botonPasoCB);
  botonPaso->set_alignment(GLUI_ALIGN_CENTER);
  botonEjecutar =
      panelIU->add_button_to_panel(run_panel, "Ciclo", 0, botonEjecutarCB);
  botonEjecutar->set_alignment(GLUI_ALIGN_CENTER);
  botonEjecucion =
      panelIU->add_button_to_panel(run_panel, "Ejecucion", 0, botonEjecucionCB);
  botonSalir = panelIU->add_button("Salir", 0, botonSalirCB);
  botonSalir->set_alignment(GLUI_ALIGN_CENTER);

  lineaVacia = panelIU->add_statictext("");

  glutTimerFunc(100, irAlJuego, 0);

  //   botonPaso->disable();
  //   botonEjecucion->disable();
  //   botonEjecutar->disable();
  //   editPosColumna->disable();
  //   editPosFila->disable();
  //   editTextPasos->disable();
  //   editTextRetardo->disable();

  glutMainLoop();
}

/**
 * @brief Inicializa y lanza la interfaz gráfica en modo en línea (con parámetros).
 *
 * Equivalente a @ref lanzar_motor_grafico pero recibe todos los parámetros
 * de la simulación ya parseados en la estructura @ref EnLinea. Carga el mapa,
 * posiciona a los agentes, fija el nivel, el objetivo, los límites de
 * instantes, energía e impacto ecológico, y arranca la IU gráfica completa.
 *
 * @param argc       Número de argumentos de la línea de comandos.
 * @param argv       Vector de cadenas con los argumentos.
 * @param argumentos Estructura @ref EnLinea con la configuración ya parseada.
 */
void lanzar_motor_grafico_verOnline(int argc, char **argv,
                                    EnLinea &argumentos) {

  // Poner los valores del juego antes de lanzar el bucle
  const char *dirMapa = argumentos.ubicacion_mapa.c_str();
  monitor.setMapa(dirMapa);
  tMap = monitor.getMapa()->getNFils();
  monitor.setMaxImpacto(argumentos.eco_impact_threshold);
  monitor.setEnergiaInicial(argumentos.initial_energy);
  monitor.setInstantesInicial(argumentos.max_instantes);
  monitor.startGame(argumentos.level, argumentos.max_instantes);
  MMmode3 = 0;
  if (argumentos.level == 6) {
    MMmode2 = 0;
    MMmode = 3;
  } else if (argumentos.level == 0 or argumentos.level == 1) {
    MMmode2 = 0;
    MMmode = 3;
  } else {
    MMmode2 = 0;
    MMmode = 0;
  }
  // Posicion inicial del agente
  monitor.setListObj(argumentos.listaObjetivos);
  monitor.inicializar(
      argumentos.posInicialIngeniero.f, argumentos.posInicialIngeniero.c,
      argumentos.posInicialIngeniero.brujula, argumentos.posInicialTecnico.f,
      argumentos.posInicialTecnico.c, argumentos.posInicialTecnico.brujula,
      argumentos.semilla);

  monitor.get_entidad(0)->setBateria(argumentos.initial_energy);
  monitor.setPasos(0);
  monitor.setRetardo(0);

  glutInit(&argc, argv);
  // Mode Setting
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  // window size (+gap size)
  glutInitWindowSize(IU_ancho_ventana, IU_alto_ventana);
  float ratio = ((float)IU_ancho_ventana * 0.7) / (float)IU_alto_ventana;
  // Initial position
  glutInitWindowPosition(300, 0);

  // Main Window
  ventanaPrincipal =
      glutCreateWindow("Practica 2: Agentes Deliberativos/Reactivos. Curso "
                       "25/26. Version BAJO PARAMETROS");
  // Main Window callback function
  glutReshapeFunc(reshape);
  glutDisplayFunc(display_ventPrincipal);

  // World Window and Display
  vistaPrincipal = glutCreateSubWindow(ventanaPrincipal, 0, 0,
                                       IU_ancho_ventana * 0.7, IU_alto_ventana);
  glutDisplayFunc(display_vistMiniMapa);
  glutMouseFunc(mouseClick);
  // glutKeyboardFunc(keyboard);

  // screen Window and Display
  vistaMiniMapa =
      glutCreateSubWindow(ventanaPrincipal, IU_ancho_ventana * 0.7, 0,
                          (IU_ancho_ventana * 0.3) + 1, IU_alto_ventana * 0.28);
  glutDisplayFunc(display_vistPrincipal);

  vistaIU = glutCreateSubWindow(
      ventanaPrincipal, IU_ancho_ventana * 0.7, IU_alto_ventana * 0.28,
      (IU_ancho_ventana * 0.3) + 1, IU_alto_ventana * 0.42);
  glutDisplayFunc(display_vistIU);

  vistaMensajes = glutCreateSubWindow(ventanaPrincipal, 0, 703, 703, 65);
  glutDisplayFunc(display_vistMensajes);
  glutMouseFunc(mouseClickMensajes);

  vistaInfo = glutCreateSubWindow(
      ventanaPrincipal, IU_ancho_ventana * 0.7, IU_alto_ventana * 0.70,
      (IU_ancho_ventana * 0.3) + 1, IU_alto_ventana * 0.3);
  glutDisplayFunc(display_vistInfo);

  glutInitWindowPosition(IU_ancho_ventana * 0.7, 50);
  glutInitWindowSize(50, 50);

  GLUI *panelIU =
      GLUI_Master.create_glui_subwindow(vistaIU, GLUI_SUBWINDOW_TOP);
  panelIU->set_main_gfx_window(vistaIU);

  botonElegirMapa = panelIU->add_button("Nuevo Juego", 0, botonElegirMapaCB);
  botonElegirMapa->set_alignment(GLUI_ALIGN_CENTER);

  GLUI_Panel *row_panel = panelIU->add_panel("", GLUI_PANEL_NONE);
  GLUI_Panel *obj_panel2 =
      panelIU->add_panel_to_panel(row_panel, "Visualizacion");

  listbox3D = panelIU->add_listbox_to_panel(obj_panel2, "3D View ", &MMmode2);
  listbox3D->add_item(0, "Ingeniero");
  listbox3D->add_item(1, "Tecnico");
  listbox3D->add_item(2, "Ninguno");
  listbox3D->set_alignment(GLUI_ALIGN_CENTER);
  listbox3D->enable();

  listboxMap = panelIU->add_listbox_to_panel(obj_panel2, "2D Map  ", &MMmode);
  listboxMap->add_item(0, "Ver");
  listboxMap->add_item(1, "Ingeniero");
  listboxMap->add_item(2, "Tecnico");
  listboxMap->add_item(3, "Conjunto");
  listboxMap->set_int_val(MMmode);
  listboxMap->set_alignment(GLUI_ALIGN_CENTER);
  listboxMap->enable();

  panelIU->add_checkbox_to_panel(obj_panel2, "Grid Off/On", &CheckGridFP);

  panelIU->add_column_to_panel(row_panel, false);

  GLUI_Panel *cam_panel = panelIU->add_panel_to_panel(row_panel, "Mode");
  Check1stPerZenital = (camMode == 0);
  panelIU->add_checkbox_to_panel(cam_panel, "1stPer/Zenital",
                                 &Check1stPerZenital, -1, cb1stPerZenital);
  CheckTotalTopology = (MMmode4 == 0);
  panelIU->add_checkbox_to_panel(cam_panel, "Total/Topology",
                                 &CheckTotalTopology, -1, cbTotalTopology);
  spinnerZoom = panelIU->add_spinner_to_panel(cam_panel, "Zoom", GLUI_SPINNER_FLOAT, &zoomZenithal);
  spinnerZoom->set_float_limits(20.0f, 120.0f);
  if (camMode == 0) spinnerZoom->disable();

  // lineaVacia = panelIU->add_statictext("");

  GLUI_Panel *obj_panel = panelIU->add_panel("Ir a...");
  // editPosFila = panelIU->add_spinner_to_panel(obj_panel, "   Fila",
  // GLUI_SPINNER_INT, &PosFila); editPosColumna =
  // panelIU->add_spinner_to_panel(obj_panel, "Columna", GLUI_SPINNER_INT,
  // &PosColumna);

  if (!monitor.there_are_active_objetivo()) {
    monitor.put_active_objetivos(1);
  }
  monitor.get_n_active_objetivo(0, PosFila, PosColumna);

  editPosFila = panelIU->add_edittext_to_panel(
      obj_panel, "Fila", GLUI_EDITTEXT_INT, &PosFila, -1, setPosFila);
  editPosFila->set_alignment(GLUI_ALIGN_LEFT);
  editPosFila->set_w(24);
  panelIU->add_column_to_panel(obj_panel, false);
  editPosColumna = panelIU->add_edittext_to_panel(
      obj_panel, "Col", GLUI_EDITTEXT_INT, &PosColumna, -1, setPosColumna);
  editPosColumna->set_alignment(GLUI_ALIGN_LEFT);
  editPosColumna->set_w(24);
  panelIU->add_column_to_panel(obj_panel, false);

  //  editPosFila->set_int_limits(0, 100, GLUI_LIMIT_WRAP);
  //  editPosColumna->set_int_limits(0, 100, GLUI_LIMIT_WRAP);

  // lineaVacia = panelIU->add_statictext("");

  GLUI_Panel *run_panel = panelIU->add_panel("Control");

  editTextPasos = panelIU->add_edittext_to_panel(run_panel, "Pasos",
                                                 GLUI_EDITTEXT_INT, &nPasos);
  editTextPasos->set_int_val(nPasos);
  editTextPasos->set_int_limits(1, 10000000, GLUI_LIMIT_CLAMP);
  editTextPasos->set_alignment(GLUI_ALIGN_CENTER);
  editTextPasos->set_w(24);

  editTextRetardo = panelIU->add_edittext_to_panel(
      run_panel, "Retardo", GLUI_EDITTEXT_INT, &tRetardo, -1, setRetardo);
  editTextRetardo->set_int_val(tRetardo);
  editTextRetardo->set_int_limits(0, 10000000, GLUI_LIMIT_CLAMP);
  editTextRetardo->set_alignment(GLUI_ALIGN_CENTER);
  editTextRetardo->set_w(24);
  setRetardo(tRetardo);

  panelIU->add_column_to_panel(run_panel, true);

  botonPaso = panelIU->add_button_to_panel(run_panel, "Paso", 0, botonPasoCB);
  botonPaso->set_alignment(GLUI_ALIGN_CENTER);
  botonEjecutar =
      panelIU->add_button_to_panel(run_panel, "Ciclo", 0, botonEjecutarCB);
  botonEjecutar->set_alignment(GLUI_ALIGN_CENTER);
  botonEjecucion =
      panelIU->add_button_to_panel(run_panel, "Ejecucion", 0, botonEjecucionCB);
  botonSalir = panelIU->add_button("Salir", 0, botonSalirCB);
  botonSalir->set_alignment(GLUI_ALIGN_CENTER);

  lineaVacia = panelIU->add_statictext("");

  monitor.juegoInicializado();

  botonPaso->enable();
  botonEjecucion->enable();
  botonEjecutar->enable();
  if (monitor.getLevel() < 2) {
    editPosColumna->disable();
    editPosFila->disable();
  } else {
    editPosColumna->enable();
    editPosFila->enable();
  }

  if (monitor.getLevel() == 2 or monitor.getLevel() == 3 or
      monitor.getLevel() == 4 or monitor.getLevel() == 5) {
    listboxMap->disable();
  }

  editTextPasos->enable();
  editTextRetardo->disable();

  // glutTimerFunc(100,update,0);
  glutTimerFunc(100, irAlJuego, 0);

  glutMainLoop();
}
