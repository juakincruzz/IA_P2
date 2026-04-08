#include "motorlib/mapa.hpp"
#include "modelos/tuberia/tubocodo.hpp"
#include "modelos/tuberia/tubo_terminal.hpp"
#include "motorlib.hpp"
#include <algorithm>

/**
 * @brief Establece el color OpenGL según la altura del terreno
 *        para la vista topológica del minimapa.
 *
 * Mapea la altura (0-9) a un rango de brillo en escala de grises.
 *
 * @param altura  Valor de altura de la celda.
 */
void Mapa::colorTopologyMM(unsigned char altura)
{
  float h = (float)altura;
  // Height 0 to 9. Map to 0.2 to 1.0 range
  float brillo = 0.2f + (h * 0.088f);
  if (brillo > 1.0f) brillo = 1.0f;
  glColor3f(brillo, brillo, brillo);
}

/**
 * @brief Renderiza el valor numérico de la altura dentro de una celda
 *        del minimapa.
 *
 * Elige color de texto blanco o negro en función del brillo del fondo
 * para asegurar contraste.
 *
 * @param altura  Valor de altura a mostrar.
 */
void Mapa::renderHeightValue(unsigned char altura) {
  char buf[4];
  sprintf(buf, "%d", (int)altura);
  
  // Decide text color based on brightness to ensure contrast
  if (altura > 5) glColor3f(0.0, 0.0, 0.0); // Black for bright background
  else glColor3f(1.0, 1.0, 1.0);           // White for dark background

  // Position text centered-ish in the cell
  // Cells are 1.0x1.0, from -0.5 to 0.5.
  glRasterPos3f(-0.3f, -0.3f, 0.2f); 
  const char* text = buf;
  while (*text) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *text);
    text++;
  }
}

/**
 * @brief Establece el color OpenGL de fondo de una celda en el minimapa
 *        según su tipo de terreno y altura.
 *
 * Cada tipo de celda ('P', 'B', 'C', 'A', etc.) tiene una paleta
 * propia cuyo brillo se modula con la altura.
 *
 * @param celda   Carácter que identifica el tipo de terreno.
 * @param altura  Valor de altura de la celda.
 */
void Mapa::colorCeldaMM(unsigned char celda, unsigned char altura)
{
  float h = altura;
  float brillo = 0.6f + (h / 25.0f); // Adjusted for 0-9 range
  switch (celda)
  {
  case 'P': // Precipicio - Deep Purple/Blue Hole
    glColor3f(0.15 * brillo, 0.1 * brillo, 0.3 * brillo);
    break;
  case 'B': // Bosque - Vibrant "Nintendo" Green
    glColor3f(0.15 * brillo, 0.8 * brillo, 0.1 * brillo);
    break;
  case 'C': // Camino - Warm Dirt
    glColor3f(0.7 * brillo, 0.4 * brillo, 0.2 * brillo);
    break;
  case 'A': // Agua - Vibrant Cyan
    glColor3f(0.0 * brillo, 0.7 * brillo, 1.0 * brillo);
    break;
  case 'S': // Sendero - Lighter warm path
    glColor3f(0.8 * brillo, 0.6 * brillo, 0.3 * brillo);
    break;
  case 'M': // Obstáculo - Mario Brick Orange
    glColor3f(0.9 * brillo, 0.3 * brillo, 0.1 * brillo);
    break;
  case 'H': // Hierba - Bright Lime Green
    glColor3f(0.6 * brillo, 1.0 * brillo, 0.1 * brillo);
    break;
  case 'K': // Bikini - Pink background
    glColor3f(1.0 * brillo, 0.6 * brillo, 0.8 * brillo);
    break;
  case 'U': // Tuberia - Sendero color (ground)
  case 'h':
  case 'v':
  case 'I':
  case 'J':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
    glColor3f(0.8 * brillo, 0.6 * brillo, 0.3 * brillo);
    break;
  case 'Z':
  case 'D': // Sneakers - Soil
    glColor3f(0.5 * brillo, 0.3 * brillo, 0.1 * brillo);
    break;
  case 'X': // Base - Yellow
    glColor3f(1.0 * brillo, 1.0 * brillo, 0.2 * brillo);
    break;
  default:
    glColor3f(0.85, 0.85, 0.90); // Whitish fog for undiscovered '?' cells
    break;
  }
}

// Global static humanoid for icon representation to avoid leaks
static Aldeano3D* icon_humanoid = nullptr;
/**
 * @brief Dibuja un modelo humanoide 3D con color de material premium.
 *
 * Reutiliza una instancia estática de @c Aldeano3D para evitar fugas
 * de memoria. El color distingue ingeniero (rojo) de técnico (naranja).
 *
 * @param r          Componente roja (no utilizada directamente; el
 *                   material se aplica internamente).
 * @param g          Componente verde.
 * @param b          Componente azul.
 * @param is_orange  Si @c true, aplica material naranja (técnico);
 *                   si @c false, material rojo (ingeniero).
 */
static void drawIconHumanoid(float r, float g, float b, bool is_orange = false) {
    if (!icon_humanoid) icon_humanoid = new Aldeano3D("");
    
    glPushAttrib(GL_ENABLE_BIT);
    // Rich Nuanced Materials
    // Ingeniero - Premium Red
    if (!is_orange) { // Ingeniero - Premium Red
        GLfloat ambient[]  = {0.4f, 0.05f, 0.05f, 1.0f};
        GLfloat diffuse[]  = {0.85f, 0.1f, 0.1f, 1.0f};
        GLfloat specular[] = {0.0f, 0.0f, 0.0f, 1.0f}; // No shine
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    } else { // Tecnico - Premium Orange
        GLfloat ambient[]  = {0.45f, 0.2f, 0.0f, 1.0f};
        GLfloat diffuse[]  = {0.95f, 0.55f, 0.1f, 1.0f};
        GLfloat specular[] = {0.0f, 0.0f, 0.0f, 1.0f}; // No shine
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    }
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 30.0f);
    
    // We use draw(1) but glColor will be overridden by materials in many setups, 
    // however for safety with these OBJ models we ensure the material is active.
    glEnable(GL_TEXTURE_2D);
    icon_humanoid->draw(1);
    glPopAttrib();
}

/**
 * @brief Dibuja marcadores visuales de retroalimentación (cruces de
 *        colisión) en una celda del minimapa.
 *
 * @param i        Fila de la celda.
 * @param j        Columna de la celda.
 * @param monitor  Referencia al monitor para consultar los marcadores activos.
 */
void drawVisualMarkersMM(int i, int j, const MonitorJuego& monitor) {
    const auto& markers = monitor.getVisualMarkers();
    for (const auto& vm : markers) {
        if (vm.f == i && vm.c == j) {
            glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
            glDisable(GL_LIGHTING);
            
            if (vm.type == 0 || vm.type == 1) { // Cross (0: Red, 1: Orange)
                if (vm.type == 0) glColor3f(1.0, 0.0, 0.0);
                else glColor3f(1.0, 0.5, 0.0);
                
                glLineWidth(7.0f);
                glBegin(GL_LINES);
                  glVertex3f(-0.4, -0.4, 0.6); glVertex3f(0.4, 0.4, 0.6);
                  glVertex3f(-0.4, 0.4, 0.6); glVertex3f(0.4, -0.4, 0.6);
                glEnd();
            }
            
            glPopAttrib();
            break;
        }
    }
}


/**
 * @brief Dibuja el icono 2D representativo de una celda en el minimapa.
 *
 * Selecciona y renderiza el símbolo gráfico adecuado según el tipo de
 * celda (bosque, agua, tuberia, base, zapatillas…). También dibuja
 * los segmentos de tubería dinámicos y estáticos.
 *
 * @param celda   Tipo de celda.
 * @param altura  Altura de la celda.
 * @param i       Fila de la celda en el mapa.
 * @param j       Columna de la celda en el mapa.
 */
void Mapa::drawIconoCeldaMM(unsigned char celda, unsigned char altura, int i, int j)
{
  float h = altura;
  float b = 0.6f + (h / 25.0f);

  if (celda != '?') {
    if (celda == 'B') { // Drawing a small tree icon
    glColor3f(0.1 * b, 0.25 * b, 0.05 * b); // Darker green for leaves
    glBegin(GL_TRIANGLES);
    glVertex3f(0.0, 0.4, 0.1);
    glVertex3f(-0.3, -0.1, 0.1);
    glVertex3f(0.3, -0.1, 0.1);
 
    glVertex3f(0.0, 0.2, 0.1); // Second layer of the tree
    glVertex3f(-0.4, -0.3, 0.1);
    glVertex3f(0.4, -0.3, 0.1);
    glEnd();
 
    glColor3f(0.3 * b, 0.2 * b, 0.1 * b); // Trunk
    glBegin(GL_QUADS);
    glVertex3f(-0.1, -0.3, 0.1);
    glVertex3f(0.1, -0.3, 0.1);
    glVertex3f(0.1, -0.45, 0.1);
    glVertex3f(-0.1, -0.45, 0.1);
    glEnd();
  }
  else if (celda == 'A') { // Water wave patterns
    glColor3f(0.4 * b, 0.6 * b, 1.0 * b);
    glBegin(GL_LINES);
    glVertex3f(-0.3, 0.2, 0.1); glVertex3f(0.0, 0.2, 0.1);
    glVertex3f(0.1, -0.2, 0.1); glVertex3f(0.4, -0.2, 0.1);
    glVertex3f(-0.1, -0.4, 0.1); glVertex3f(0.2, -0.4, 0.1);
    glEnd();
  }
  else if (celda == 'H') { // Hierba details
    glColor3f(0.25 * b, 0.5 * b, 0.15 * b);
    glPointSize(2.0);
    glBegin(GL_POINTS);
    glVertex3f(-0.2, 0.2, 0.1);
    glVertex3f(0.3, -0.1, 0.1);
    glVertex3f(-0.1, -0.3, 0.1);
    glEnd();
  }
    else if (celda == 'U') { // Pipe Icon (Zenithal View)
      // Outer Rim (Light Red)
      glColor3f(1.0, 0.2, 0.2); // Mario Light Red
      glBegin(GL_TRIANGLE_FAN);
      glVertex3f(0.0, 0.0, 0.1);
      for (int n = 0; n <= 20; n++) {
          float angle = 2.0 * M_PI * n / 20.0;
          glVertex3f(0.35 * cos(angle), 0.35 * sin(angle), 0.1);
      }
      glEnd();

      // Inner Pipe Body (Dark Red)
      glColor3f(0.7, 0.0, 0.0); // Mario Dark Red
      glBegin(GL_TRIANGLE_FAN);
      glVertex3f(0.0, 0.0, 0.11);
      for (int n = 0; n <= 20; n++) {
          float angle = 2.0 * M_PI * n / 20.0;
          glVertex3f(0.25 * cos(angle), 0.25 * sin(angle), 0.11);
      }
      glEnd();
    }
  else if (celda == 'C' || celda == 'S') { // Dirt/Path texture
    glColor3f(0.35 * b, 0.3 * b, 0.2 * b);
    glPointSize(1.5);
    glBegin(GL_POINTS);
    float nx = (float)((i * 17) % 10) / 20.0 - 0.25;
    float ny = (float)((j * 13) % 10) / 20.0 - 0.25;
    // Swapping nx/ny to match i/j role in 2D icons if needed, but original used i/j mix.
    // Let's keep it but ENSURE the point is not breaking the GL state.
    glVertex3f(nx, ny, 0.1);
    glVertex3f(-nx, -ny, 0.1);
    glEnd();
  }
  else if (celda == 'P') { // Precipice texture (diagonal marks)
    glColor3f(0.15 * b, 0.15 * b, 0.18 * b);
    glBegin(GL_LINES);
    glVertex3f(-0.4, 0.4, 0.1); glVertex3f(-0.2, 0.2, 0.1);
    glVertex3f(0.2, -0.2, 0.1); glVertex3f(0.4, -0.4, 0.1);
    glEnd();
  }
  // Simplified: No complex icons for Z and K to avoid visibility issues
  else if (celda == 'X') { // Base - Bullseye pattern (Pink/Yellow CIRCLES)
    float centerX = 0.0, centerY = 0.0;
    int numSegments = 16;
    
    // 1. Outer Pink Circle
    glColor3f(1.0, 0.0, 1.0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(centerX, centerY, 0.1);
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        glVertex3f(centerX + cos(angle) * 0.48, centerY + sin(angle) * 0.48, 0.1);
    }
    glEnd();

    // 2. Middle Yellow Circle
    glColor3f(1.0, 1.0, 0.0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(centerX, centerY, 0.2);
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        glVertex3f(centerX + cos(angle) * 0.3, centerY + sin(angle) * 0.3, 0.2);
    }
    glEnd();

    // 3. Inner Pink Circle
    glColor3f(1.0, 0.0, 1.0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(centerX, centerY, 0.3);
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        glVertex3f(centerX + cos(angle) * 0.12, centerY + sin(angle) * 0.12, 0.3);
    }
    glEnd();
  }
  else if (celda == 'Z' || celda == 'D') { // Sneakers (Deportivas) - LARGE 'Z' ICON
    glColor3f(0.8, 0.0, 0.8); // Vibrant Purple
    glBegin(GL_QUADS);
    // Top bar
    glVertex3f(-0.45, 0.45, 0.2); glVertex3f(0.45, 0.45, 0.2);
    glVertex3f(0.45, 0.25, 0.2); glVertex3f(-0.45, 0.25, 0.2);
    
    // Diagonal bar (approximated with a rotated quad logic using vertices)
    glVertex3f(0.45, 0.35, 0.2); glVertex3f(0.25, 0.45, 0.2);
    glVertex3f(-0.45, -0.35, 0.2); glVertex3f(-0.25, -0.45, 0.2);
    
    // Bottom bar
    glVertex3f(-0.45, -0.25, 0.2); glVertex3f(0.45, -0.25, 0.2);
    glVertex3f(0.45, -0.45, 0.2); glVertex3f(-0.45, -0.45, 0.2);
    glEnd();
  }
  else if (celda == 'K') { // Bikini - Concentric Squares (Green-White-Brown-Black)
    float sizes[] = {0.45, 0.35, 0.25, 0.15};
    float colors[4][3] = {
        {0.0, 0.7, 0.0}, // Green
        {1.0, 1.0, 1.0}, // White
        {0.4, 0.2, 0.0}, // Brown
        {0.0, 0.0, 0.0}  // Black
    };

    for (int k = 0; k < 4; ++k) {
        glColor3f(colors[k][0], colors[k][1], colors[k][2]);
        glBegin(GL_QUADS);
        glVertex3f(-sizes[k], -sizes[k], 0.1 + k * 0.1);
        glVertex3f(sizes[k], -sizes[k], 0.1 + k * 0.1);
        glVertex3f(sizes[k], sizes[k], 0.1 + k * 0.1);
        glVertex3f(-sizes[k], sizes[k], 0.1 + k * 0.1);
        glEnd();
    }
  }

  // Draw Pipes from mapaTuberias (Dynamic Pipes) and static characters ('h', 'v', '1'-'6')
  auto& pipes = monitor.getMapaTuberias();
  if (i >= 0 && i < pipes.size() && j >= 0 && j < pipes[i].size()) {
    unsigned char mask = pipes[i][j];
    
    // Combine static pipe character bits with dynamic mask for unified rendering
    if (celda == 'h') mask |= 4 | 64; // Horizontal (East-West)
    else if (celda == 'v') mask |= 1 | 16; // Vertical (North-South)
    else if (celda == '1') mask |= 1; // Terminal North
    else if (celda == '2') mask |= 16; // Terminal South
    else if (celda == 'I') mask |= 4; // Terminal East
    else if (celda == 'J') mask |= 64; // Terminal West
    else if (celda == '3') mask |= 1 | 4; // Elbow North-East
    else if (celda == '4') mask |= 4 | 16; // Elbow East-South
    else if (celda == '5') mask |= 16 | 64; // Elbow South-West
    else if (celda == '6') mask |= 1 | 64; // Elbow West-North

    if (mask != 0) {
      if (mask & 1) { // North (+Y in 2D space)
          glColor3f(1.0, 0.0, 0.0);
          glBegin(GL_QUADS);
          glVertex3f(-0.12, 0.0, 0.4); glVertex3f(0.12, 0.0, 0.4);
          glVertex3f(0.12, 0.5, 0.4); glVertex3f(-0.12, 0.5, 0.4);
          glEnd();
      }
      if (mask & 4) { // East (+X in 2D space)
          glColor3f(1.0, 0.0, 0.0);
          glBegin(GL_QUADS);
          glVertex3f(0.0, -0.12, 0.4); glVertex3f(0.5, -0.12, 0.4);
          glVertex3f(0.5, 0.12, 0.4); glVertex3f(0.0, 0.12, 0.4);
          glEnd();
      }
      if (mask & 16) { // South (-Y in 2D space)
          glColor3f(1.0, 0.0, 0.0);
          glBegin(GL_QUADS);
          glVertex3f(-0.12, -0.5, 0.4); glVertex3f(0.12, -0.5, 0.4);
          glVertex3f(0.12, 0.0, 0.4); glVertex3f(-0.12, 0.0, 0.4);
          glEnd();
      }
      if (mask & 64) { // West (-X in 2D space)
          glColor3f(1.0, 0.0, 0.0);
          glBegin(GL_QUADS);
          glVertex3f(-0.5, -0.12, 0.4); glVertex3f(0.0, -0.12, 0.4);
          glVertex3f(0.0, 0.12, 0.4); glVertex3f(-0.5, 0.12, 0.4);
          glEnd();
      }
      // Center Junction Dot (slightly darker red to reinforce connection)
      glColor3f(0.8, 0.0, 0.0);
      glBegin(GL_QUADS);
      glVertex3f(-0.15, -0.15, 0.45); glVertex3f(0.15, -0.15, 0.45);
      glVertex3f(0.15, 0.15, 0.45); glVertex3f(-0.15, 0.15, 0.45);
      glEnd();
    }
  }
}
}


/**
 * @brief Dibuja una moneda 3D giratoria (efecto de animación continua).
 *
 * @param size  Factor de escala de la moneda.
 */
static void drawCoin(float size) {
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float angle = t * 180.0f; // Spin animation

    glPushMatrix();
    glRotatef(angle, 0, 1, 0);
    glScalef(size, size, size * 0.2f);
    
    // Gold Material
    GLfloat ambient[]  = {0.4f, 0.3f, 0.0f, 1.0f};
    GLfloat diffuse[]  = {1.0f, 0.85f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);

    glColor3f(1.0f, 0.85f, 0.0f);
    glutSolidSphere(1.0, 16, 16);
    glPopMatrix();
}

/**
 * @brief Dibuja una zapatilla 3D estilizada con animación de vaivén.
 *
 * Incluye suela, cuerpo, parte del tobillo, swoosh y cordones.
 *
 * @param size  Factor de escala base.
 */
static void drawShoe(float size) {
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float bob = 0.5f + 0.5f * sin(t * 3.0f); // Bobbing animation
    float angle = 15.0f * sin(t * 2.0f);    // Gentle tilt

    glPushMatrix();
    glTranslatef(0, 0.5f + bob, 0);
    glRotatef(angle, 0, 1, 0);
    glScalef(size * 0.33f, size * 0.33f, size * 0.33f); // Reduced 4x

    // Shoe Sole (Rounded black base)
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0, -0.4f, 0.3f); // Lowered and shifted forward
    glScalef(0.9f, 0.3f, 2.2f);   // Narrower and shorter
    glutSolidCube(1.0);
    glPopMatrix();

    // Main Body - Rounded Toe (Sphere)
    glColor3f(0.6f, 0.1f, 1.0f);
    glPushMatrix();
    glTranslatef(0, -0.1f, 1.0f);
    glScalef(1.0f, 0.8f, 1.2f);
    glutSolidSphere(0.6, 16, 16);
    glPopMatrix();

    // Main Body - Mid/Back (Rounded Box)
    glPushMatrix();
    glTranslatef(0, 0.1f, -0.2f);
    glScalef(1.0f, 1.2f, 1.8f);
    glutSolidSphere(0.5, 16, 16);
    glPopMatrix();

    // Ankle part (Rising up)
    glPushMatrix();
    glTranslatef(0, 0.6f, -0.6f);
    glScalef(0.9f, 0.8f, 0.9f);
    glutSolidSphere(0.5, 16, 16);
    glPopMatrix();

    // NIKE-STYLE SWOOSH (Simple white mark on the side)
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    glVertex3f(0.55f, 0.2f, -0.2f); // Tail - moved out to 0.55
    glVertex3f(0.55f, -0.1f, 0.3f); // Dip
    glVertex3f(0.55f, 0.15f, 0.8f); // Tip
    glEnd();
    glEnable(GL_LIGHTING);

    // Laces (White dots)
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (float l = 0.0f; l <= 0.8f; l += 0.25f) {
        glVertex3f(0.2f, 0.55f, l);
        glVertex3f(-0.2f, 0.55f, l);
    }
    glEnd();
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

/**
 * @brief Dibuja el objetivo (BelPos) en la vista 2D del minimapa.
 *
 * Renderiza un charco de aceite animado con una tubería central.
 */
static void drawBelPos2D() {
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    int numSegments = 32;
    
    // 1. Oil Puddle (Dark, animated irregular shape)
    glColor3f(0.05f, 0.05f, 0.07f); // Deep Blackish oil
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, 0.05f);
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        // Animation: much slower ripples and irregular radius
        float ripple = 0.25f * sin(1.2f * t + angle * 4.0f) + 0.12f * cos(2.0f * t - angle * 2.0f);
        float radius = 0.42f + ripple;
        glVertex3f(radius * cos(angle), radius * sin(angle), 0.05f);
    }
    glEnd();

    // 2. Pipe in the middle
    // Outer Rim (Red as requested)
    glColor3f(1.0f, 0.2f, 0.2f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, 0.1f);
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        glVertex3f(0.3f * cos(angle), 0.3f * sin(angle), 0.1f);
    }
    glEnd();

    // Inner Hole
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, 0.15f);
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        glVertex3f(0.2f * cos(angle), 0.2f * sin(angle), 0.15f);
    }
    glEnd();
}

/**
 * @brief Dibuja el objetivo (BelPos) en las vistas 3D (primera persona
 *        y cenital).
 *
 * Renderiza un charco de aceite animado sobre el suelo con una tubería
 * 3D roja en el centro.
 */
static void drawBelPos3D() {
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    int numSegments = 32;
    
    // 1. Oil Puddle (Dark, animated irregular shape on the ground)
    glDisable(GL_LIGHTING);
    glColor3f(0.05f, 0.05f, 0.07f); // Deep Blackish oil
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.05f, 0.0f); // Slightly above ground
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        float ripple = 0.25f * sin(6.0f * t + angle * 4.0f) + 0.12f * cos(10.0f * t - angle * 2.0f);
        float radius = 1.8f + ripple;
        glVertex3f(radius * cos(angle), 0.05f, radius * sin(angle));
    }
    glEnd();
    glEnable(GL_LIGHTING);

    // 2. 3D Pipe in the middle
    static Tuberia3D* goalPipe = nullptr;
    if (!goalPipe) goalPipe = new Tuberia3D();

    glPushMatrix();
    glTranslatef(0.0f, 0.25f, 0.0f);
    glScalef(1.2f, 0.6f, 1.2f);
    
    // Color the pipe red
    GLfloat ambient[] = {0.4f, 0.0f, 0.0f, 1.0f};
    GLfloat diffuse[] = {1.0f, 0.2f, 0.2f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);

    goalPipe->draw(1);
    glPopMatrix();
}

/**
 * @brief Dibuja un marcador de ruta con animación de pulso.
 *
 * El color depende del tipo de plan (blanco = ingeniero, amarillo =
 * técnico, rojo = tuberías). En 3D se usan esferas; en 2D, rombos
 * o puntos.
 *
 * @param action    Acción asociada al paso del plan.
 * @param planType  Tipo de plan (0 = ingeniero, 1 = técnico, 2 = tuberías).
 * @param index     Índice del paso (para desfase de la animación).
 * @param x         Coordenada X de posición.
 * @param y         Coordenada Y de posición.
 * @param z         Coordenada Z de posición.
 * @param is3D      Si @c true, renderiza en modo 3D; si @c false, en 2D.
 */
static void drawPathMarker(Action action, int planType, int index, float x, float y, float z, bool is3D) {
    bool isWalk = (action == WALK);

    // Dynamic Animation: pulsing size based on time and index
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float pulse = 0.1f * sin(t * 5.0f - index * 0.4f);
    float size = 0.3f + pulse;

    if (planType == 0) glColor3f(1.0f, 1.0f, 1.0f);      // White (Engineer)
    else if (planType == 1) glColor3f(1.0f, 1.0f, 0.0f); // Yellow (Technician)
    else glColor3f(1.0f, 0.0f, 0.0f);                   // Red (Pipe Plan)

    glPushMatrix();
    glTranslatef(x, y, z);
    
    if (is3D) {
        if (isWalk) {
            glScalef(size * 1.5f, size * 1.5f, size * 1.5f);
            glutSolidSphere(0.4, 8, 8); // Floating dots for walking
        } else {
            glScalef(size, size, size);
            glutSolidSphere(0.3, 8, 8); // Smaller dots for other actions
        }
    } else {
        // 2D Minimap Perspective
        if (isWalk) { // Diamond
            glBegin(GL_QUADS);
            glVertex3f(0.0, -0.3, 0.1); glVertex3f(-0.3, 0.0, 0.1);
            glVertex3f(0.0, 0.3, 0.1); glVertex3f(0.3, 0.0, 0.1);
            glEnd();
        } else { // Small Dot
            glPointSize(3.0f);
            glBegin(GL_POINTS);
            glVertex3f(0, 0, 0.1);
            glEnd();
        }
    }
    
    glPopMatrix();
}

/**
 * @brief Dibuja un segmento de línea sólida entre dos puntos del plan.
 *
 * @param x1        Coordenada X del primer punto.
 * @param y1        Coordenada Y del primer punto.
 * @param z1        Coordenada Z del primer punto.
 * @param x2        Coordenada X del segundo punto.
 * @param y2        Coordenada Y del segundo punto.
 * @param z2        Coordenada Z del segundo punto.
 * @param planType  Tipo de plan (determina el color).
 */
void drawPlanLine(float x1, float y1, float z1, float x2, float y2, float z2, int planType) {
    glDisable(GL_LIGHTING);
    glLineWidth(4.0f);
    if (planType == 0) glColor3f(1.0f, 1.0f, 1.0f);      // White
    else if (planType == 1) glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    else glColor3f(1.0f, 0.0f, 0.0f);                   // Red

    glBegin(GL_LINES);
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y2, z2);
    glEnd();
    glEnable(GL_LIGHTING);
}

/**
 * @brief Dibuja un plan completo en 3D: marcadores en cada paso y
 *        líneas de conexión entre pasos consecutivos.
 *
 * @param p             Lista de casillas del plan.
 * @param planType      Tipo de plan (0 = ingeniero, 1 = técnico, 2 = tuberías).
 * @param filaMed       Fila central del mapa para el cálculo de posición.
 * @param colMed        Columna central del mapa.
 * @param mapaAlturas   Matriz de alturas del terreno.
 */
static void drawPlans3D(const ListaCasillasPlan &p, int planType, int filaMed, int colMed, const vector<vector<unsigned char>> &mapaAlturas) {
    if (p.size() == 0) return;
    
    int idx = 0;
    auto it = p.begin();
    while (it != p.end()) {
      float x1 = ((GLfloat)filaMed - (GLfloat)it->col - 1) * 5.0;
      float z1 = ((GLfloat)colMed - (GLfloat)it->fil - 1) * 5.0;
      float y1 = (mapaAlturas[it->fil][it->col]) * 2.0f;

      // Elevate JUMP actions by 0 units
      if (it->op == JUMP) y1 += 0.0f;

      // Draw Marker
      if (planType != 2) {
        glPushMatrix();
        glTranslatef(x1, y1 + 1.5f, z1);
        drawPathMarker(static_cast<Action>(it->op), planType, idx, 0, 0, 0, true);
        glPopMatrix();
      }

      // Draw Line to next node
      auto nextIt = std::next(it);
      if (nextIt != p.end()) {
        float x2 = ((GLfloat)filaMed - (GLfloat)nextIt->col - 1) * 5.0;
        float z2 = ((GLfloat)colMed - (GLfloat)nextIt->fil - 1) * 5.0;
        float y2 = (mapaAlturas[nextIt->fil][nextIt->col]) * 2.0f;
        
        // Elevate JUMP actions by 0 units
        if (nextIt->op == JUMP) y2 += 0.0f;

        drawPlanLine(x1, y1 + 1.5f, z1, x2, y2 + 1.5f, z2, planType);
      }

      ++it;
      ++idx;
    }
}


/**
 * @brief Establece el color opuesto (contraste) para dibujar texto o
 *        símbolos sobre una celda en el minimapa.
 *
 * @param celda  Tipo de celda para la que se necesita el color de contraste.
 */
void Mapa::colorCeldaOpuestoMM(unsigned char celda)
{
  switch (celda)
  {
  case 'P':
    glColor3f(1, 0, 0);
    break;
  case 'B':
    glColor3f(1, 0, 0);
    break;
  case 'A':
    glColor3f(1.0, 0.0, 0.0);
    break;
  case 'S':
    glColor3f(1, 0, 0);
    break;
  case 'M':
    glColor3f(1, 1, 1);
    break;
  case 'H':
    glColor3f(1, 0, 0);
    break;
  case 'K':
    glColor3f(0, 0, 0);
    break;
  case 'Z':
    glColor3f(1, 1, 1);
    break;
  case 'D':
    glColor3f(1, 1, 1);
    break;
  case 'X':
    glColor3f(0, 0, 0);
    break;
  default:
  glColor3f(1,0,0);
  break;
  }
}

/**
 * @brief Establece un segundo color de contraste (naranja) para
 *        dibujar el técnico sobre celdas del minimapa.
 *
 * @param celda  Tipo de celda.
 */
void Mapa::colorCeldaOpuestoMM2(unsigned char celda)
{
  switch (celda)
  {
  case 'P':
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    break;
  case 'B':
  case 'H':
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    break;
  case 'A':
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    break;
  case 'S':
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    break;
  case 'M':
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    break;
  // case 'H':
  //   glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
  //   break;
  case 'K':
    glColor3f(0, 0, 0);
    break;
  case 'Z':
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    break;
  case 'D':
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    break;
  case 'X':
    glColor3f(0, 0, 0);
    break;
  default:
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    break;
  }
}

/**
 * @brief Aplica la rotación OpenGL correspondiente a una orientación
 *        en el plano 2D del minimapa.
 *
 * @param orienParam  Orientación de la entidad.
 */
void Mapa::OrientacionEntidadMM(Orientacion orienParam)
{
  switch (orienParam)
  {
  case norte:
    glRotatef(90.0, 0, 0, 1);
    break;
  case noreste:
    glRotatef(45.0, 0, 0, 1);
    break;
  case este:
    glRotatef(0.0, 0, 0, 1);
    break;
  case sureste:
    glRotatef(315.0, 0, 0, 1);
    break;
  case sur:
    glRotatef(270.0, 0, 0, 1);
    break;
  case suroeste:
    glRotatef(215.0, 0, 0, 1);
    break;
  case oeste:
    glRotatef(180.0, 0, 0, 1);
    break;
  case noroeste:
    glRotatef(135.0, 0, 0, 1);
    break;
  }
}

/**
 * @brief Aplica la rotación OpenGL correspondiente a una orientación
 *        en la vista de primera persona (eje Y).
 *
 * @param orienParam  Orientación de la entidad.
 */
void Mapa::OrientacionEntidadFP(Orientacion orienParam)
{
  switch (orienParam)
  {
  case norte:
    glRotatef(0.0, 0, 1, 0);
    break;
  case noreste:
    glRotatef(315.0, 0, 1, 0);
    break;
  case este:
    glRotatef(270.0, 0, 1, 0);
    break;
  case sureste:
    glRotatef(215.0, 0, 1, 0);
    break;
  case sur:
    glRotatef(180.0, 0, 1, 0);
    break;
  case suroeste:
    glRotatef(135.0, 0, 1, 0);
    break;
  case oeste:
    glRotatef(90.0, 0, 1, 0);
    break;
  case noroeste:
    glRotatef(45.0, 0, 1, 0);
    break;
  }
}

/**
 * @brief Dibuja los complementos 3D propios de cada tipo de celda en la
 *        vista de primera persona y cenital.
 *
 * Según el tipo de celda, renderiza árboles, ladrillos, agua, tuberías,
 * monedas, zapatillas, muros y otros elementos decorativos.
 *
 * @param celda   Tipo de celda.
 * @param altura  Altura de la celda (carácter ASCII).
 * @param fil     Fila de la celda.
 * @param col     Columna de la celda.
 * @param level   Nivel de juego actual.
 */
void Mapa::complementosCelda(unsigned char celda, unsigned char altura, int fil, int col, int level)
{
  unsigned char h = altura - '0';

  switch (celda)
  {
  case 'P': 
    break;
  case 'B': // Mario Tree: Cylinder trunk + Spheres for foliage
    {
      // Trunk
      glColor3f(0.4, 0.2, 0.1);
      glPushMatrix();
      glScalef(1.0, 8.0, 1.0);
      glRotatef(-90, 1, 0, 0);
      GLUquadric* qobj = gluNewQuadric();
      gluCylinder(qobj, 0.5, 0.5, 1.0, 8, 1);
      glPopMatrix();

      // Foliage - Stacked spheres
      glColor3f(0.2, 0.8, 0.1);
      glPushMatrix(); glTranslatef(0.0, 8.0, 0.0); glutSolidSphere(2.0, 12, 12); glPopMatrix();
      glPushMatrix(); glTranslatef(1.2, 7.5, 0.0); glutSolidSphere(1.5, 12, 12); glPopMatrix();
      glPushMatrix(); glTranslatef(-1.2, 7.5, 0.0); glutSolidSphere(1.5, 12, 12); glPopMatrix();
      glPushMatrix(); glTranslatef(0.0, 7.5, 1.2); glutSolidSphere(1.5, 12, 12); glPopMatrix();
      glPushMatrix(); glTranslatef(0.0, 7.5, -1.2); glutSolidSphere(1.5, 12, 12); glPopMatrix();
      glPushMatrix(); glTranslatef(0.0, 9.5, 0.0); glutSolidSphere(1.5, 12, 12); glPopMatrix();
      
      gluDeleteQuadric(qobj);
    }
    break;
  case 'M': // Mario Brick
    {
      GLfloat ambient[] = {0.6, 0.15, 0.1, 1.0};
      GLfloat diffuse[] = {0.9, 0.3, 0.1, 1.0};
      GLfloat specular[] = {0.2, 0.1, 0.1, 1.0};
      glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
      glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
      glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
      
      glColor3f(0.9, 0.3, 0.1);
      glPushMatrix();
      glTranslatef(0.0, 0.2, 0.0);
      glScalef(5.0, 15.0, 5.0);
      glutSolidCube(1.0);
      glPopMatrix();

      // Bold white/beige grout lines
      glDisable(GL_LIGHTING);
      glColor3f(1.0, 0.9, 0.8);
      glLineWidth(3.0);
      
      float wall_h = 15.0;
      float start_y = -wall_h/2.0 + 0.2;
      float brick_h = 3.0;
      float brick_w = 2.5;

      glBegin(GL_LINES);
      for (float y = start_y + brick_h; y < wall_h/2.0; y += brick_h) {
          glVertex3f(-2.55, y, 2.55); glVertex3f(2.55, y, 2.55);
          glVertex3f(-2.55, y, -2.55); glVertex3f(2.55, y, -2.55);
          glVertex3f(2.55, y, -2.55); glVertex3f(2.55, y, 2.55);
          glVertex3f(-2.55, y, -2.55); glVertex3f(-2.55, y, 2.55);
      }
      glEnd();
      glEnable(GL_LIGHTING);
    }
    break;
  case 'U': // Tuberia - Mario Pipe
    {
      static Tuberia3D* tuberia = nullptr;
      if (!tuberia) tuberia = new Tuberia3D();
      
      glPushMatrix();
      glScalef(0.8, 0.4, 0.8); // Scale to fit cell nicely, Y=0.4 for low height
      tuberia->draw(1);
      glPopMatrix();
    }
    break;
  case 'h': // Horizontal Pipe E-W
    {
      static Tubo3D* tuboH = nullptr;
      if (!tuboH) tuboH = new Tubo3D();
      static VerticalConnector3D* vConn = nullptr;
      if (!vConn) vConn = new VerticalConnector3D();

      glPushMatrix();
      glRotatef(90, 0, 1, 0); // Rotate N-S industrial model to E-W
      tuboH->draw(1);
      glPopMatrix();


      // Check height differences with neighbors to the West (fil, col-1) and East (fil, col+1)
      // West Connector
      if (col > 0 && esTuberia(mapaCompleto[fil][col-1])) {
          float dh = (float)(mapaAlturas[fil][col-1] - '0') - (float)(altura - '0');
          if (dh != 0) {
              glPushMatrix();
              glTranslatef(-2.5, 0.0, 0.0);
              vConn->setConnector(dh * 2.0f);
              vConn->draw(1);
              glPopMatrix();
          }
      }
      // East Connector
      if (col < nCols - 1 && esTuberia(mapaCompleto[fil][col+1])) {
          float dh = (float)(mapaAlturas[fil][col+1] - '0') - (float)(altura - '0');
          if (dh != 0) {
              glPushMatrix();
              glTranslatef(2.5, 0.0, 0.0);
              vConn->setConnector(dh * 2.0f);
              vConn->draw(1);
              glPopMatrix();
          }
      }
    }
    break;
  case 'v': // Vertical Pipe N-S
    {
      static Tubo3D* tuboV = nullptr;
      if (!tuboV) tuboV = new Tubo3D();
      static VerticalConnector3D* vConn = nullptr;
      if (!vConn) vConn = new VerticalConnector3D();

      glPushMatrix();
      tuboV->draw(1);
      glPopMatrix();


      // Neighbors North (fil-1, col) and South (fil+1, col)
      // North Connector
      if (fil > 0 && esTuberia(mapaCompleto[fil-1][col])) {
          float dh = (float)(mapaAlturas[fil-1][col] - '0') - (float)(altura - '0');
          if (dh != 0) {
              glPushMatrix();
              glTranslatef(0.0, 0.0, -2.5);
              vConn->setConnector(dh * 2.0f);
              vConn->draw(1);
              glPopMatrix();
          }
      }
      // South Connector
      if (fil < nFils - 1 && esTuberia(mapaCompleto[fil+1][col])) {
          float dh = (float)(mapaAlturas[fil+1][col] - '0') - (float)(altura - '0');
          if (dh != 0) {
              glPushMatrix();
              glTranslatef(0.0, 0.0, 2.5);
              vConn->setConnector(dh * 2.0f);
              vConn->draw(1);
              glPopMatrix();
          }
      }
    }
    break;
  case 'I': // Terminal East-Center
    {
      static TuboTerminal3D* tuboTerminal = nullptr;
      if (!tuboTerminal) tuboTerminal = new TuboTerminal3D();
      static VerticalConnector3D* vConn = nullptr;
      if (!vConn) vConn = new VerticalConnector3D();

      glPushMatrix();
      glRotatef(90, 0, 1, 0); // North-Center terminal rotated to West-Center (Wait, East-Center is case I?)
      tuboTerminal->draw(1);
      glPopMatrix();


      // Exit is towards East
      if (col < nCols - 1 && esTuberia(mapaCompleto[fil][col+1])) {
          float dh = (float)(mapaAlturas[fil][col+1] - '0') - (float)(altura - '0');
          if (dh != 0) {
              glPushMatrix();
              glTranslatef(2.5, 0.0, 0.0);
              vConn->setConnector(dh * 2.0f);
              vConn->draw(1);
              glPopMatrix();
          }
      }
    }
    break;
  case 'J': // Terminal West-Center
    {
      static TuboTerminal3D* tuboTerminal = nullptr;
      if (!tuboTerminal) tuboTerminal = new TuboTerminal3D();
      static VerticalConnector3D* vConn = nullptr;
      if (!vConn) vConn = new VerticalConnector3D();

      glPushMatrix();
      glRotatef(270, 0, 1, 0); // North-Center terminal rotated to East-Center
      tuboTerminal->draw(1);
      glPopMatrix();


      // Exit is towards West
      if (col > 0 && esTuberia(mapaCompleto[fil][col-1])) {
          float dh = (float)(mapaAlturas[fil][col-1] - '0') - (float)(altura - '0');
          if (dh != 0) {
              glPushMatrix();
              glTranslatef(-2.5, 0.0, 0.0);
              vConn->setConnector(dh * 2.0f);
              vConn->draw(1);
              glPopMatrix();
          }
      }
    }
    break;
  case '1': // Terminal North-Center
    {
      static TuboTerminal3D* tuboTerminal = nullptr;
      if (!tuboTerminal) tuboTerminal = new TuboTerminal3D();
      static VerticalConnector3D* vConn = nullptr;
      if (!vConn) vConn = new VerticalConnector3D();

      glPushMatrix();
      tuboTerminal->draw(1);
      glPopMatrix();


      // Exit is North
      if (fil > 0 && esTuberia(mapaCompleto[fil-1][col])) {
          float dh = (float)(mapaAlturas[fil-1][col] - '0') - (float)(altura - '0');
          if (dh != 0) {
              glPushMatrix();
              glTranslatef(0.0, 0.0, -2.5);
              vConn->setConnector(dh * 2.0f);
              vConn->draw(1);
              glPopMatrix();
          }
      }
    }
    break;
  case '2': // Terminal South-Center
    {
      static TuboTerminal3D* tuboTerminal = nullptr;
      if (!tuboTerminal) tuboTerminal = new TuboTerminal3D();
      static VerticalConnector3D* vConn = nullptr;
      if (!vConn) vConn = new VerticalConnector3D();

      glPushMatrix();
      glRotatef(180, 0, 1, 0); 
      tuboTerminal->draw(1);
      glPopMatrix();


      // Exit is South
      if (fil < nFils - 1 && esTuberia(mapaCompleto[fil+1][col])) {
          float dh = (float)(mapaAlturas[fil+1][col] - '0') - (float)(altura - '0');
          if (dh != 0) {
              glPushMatrix();
              glTranslatef(0.0, 0.0, 2.5);
              vConn->setConnector(dh * 2.0f);
              vConn->draw(1);
              glPopMatrix();
          }
      }
    }
    break;
  case 'Z':
  case 'D':
    drawShoe(1.5f);
    break;
  case '3': // Elbow West-North (Based on cases 3,4,5,6 mappings in MM)
  case '4': 
  case '5': 
  case '6': 
    {
      static TuboCodo3D* tuboCodo = nullptr;
      if (!tuboCodo) tuboCodo = new TuboCodo3D();
      static VerticalConnector3D* vConn = nullptr;
      if (!vConn) vConn = new VerticalConnector3D();

      glPushMatrix();
      if (celda == '4') glRotatef(90, 0, 1, 0);
      else if (celda == '5') glRotatef(180, 0, 1, 0);
      else if (celda == '6') glRotatef(270, 0, 1, 0);
      tuboCodo->draw(1);
      glPopMatrix();

      // Elbows have two exits. 
      // Case '3' (North-East in MM? Wait, let's check MM logic)
      
      /*
      MM cases for elbows:
      '6': West-North
      '3': North-East
      '4': East-South
      '5': South-West
      */
      
      // Let's use the MM orientation for consistency
      int neighbors[4][2] = {
          { -1, 0 }, // North
          { 0, 1 },  // East
          { 1, 0 },  // South
          { 0, -1 }  // West
      };
      
      int exit1 = -1, exit2 = -1;
      if (celda == '6') { exit1 = 0; exit2 = 3; } // North, West
      else if (celda == '3') { exit1 = 0; exit2 = 1; } // North, East
      else if (celda == '4') { exit1 = 1; exit2 = 2; } // East, South
      else if (celda == '5') { exit1 = 2; exit2 = 3; } // South, West
      

      int exits[] = { exit1, exit2 };
      for (int k=0; k<2; ++k) {
          int ex = exits[k];
          int nfil = fil + neighbors[ex][0];
          int ncol = col + neighbors[ex][1];
          if (nfil >= 0 && nfil < nFils && ncol >= 0 && ncol < nCols && esTuberia(mapaCompleto[nfil][ncol])) {
              float dh = (float)(mapaAlturas[nfil][ncol]) - (float)altura;
              if (dh != 0) {
                  glPushMatrix();
                  glTranslatef(neighbors[ex][1]*2.5, 0.0, neighbors[ex][0]*2.5);
                  vConn->setConnector(dh * 2.0f);
                  vConn->draw(1);
                  glPopMatrix();
              }
          }
      }
    }
    break;
  }

  // Draw Pipes from mapaTuberias in 3D
  auto& pipes3D = monitor.getMapaTuberias();
  if (fil >= 0 && fil < pipes3D.size() && col >= 0 && col < pipes3D[fil].size()) {
    unsigned char mask = pipes3D[fil][col];
    if (mask != 0) {
      static TuboTerminal3D* tuboT = nullptr;
      if (!tuboT) tuboT = new TuboTerminal3D();
      static VerticalConnector3D* vConn = nullptr;
      if (!vConn) vConn = new VerticalConnector3D();

      // 1: North (+Z), 4: East (-X), 16: South (-Z), 64: West (+X)
      if (mask & 16) { // South (-Z)
        glPushMatrix();
        tuboT->draw(1);
        if (fil + 1 < pipes3D.size() && pipes3D[fil + 1][col] != 0) {
            float dh = (float)(mapaAlturas[fil + 1][col] - '0') - (float)(altura - '0');
            if (dh != 0) {
                glTranslatef(0.0, 0.0, -2.5);
                vConn->setConnector(dh * 2.0f);
                vConn->draw(1);
            }
        }
        glPopMatrix();
      }
      if (mask & 4) { // East (-X)
        glPushMatrix();
        glRotatef(90, 0, 1, 0);
        tuboT->draw(1);
        if (col + 1 < pipes3D[fil].size() && pipes3D[fil][col + 1] != 0) {
            float dh = (float)(mapaAlturas[fil][col + 1] - '0') - (float)(altura - '0');
            if (dh != 0) {
                glTranslatef(0.0, 0.0, -2.5);
                vConn->setConnector(dh * 2.0f);
                vConn->draw(1);
            }
        }
        glPopMatrix();
      }
      if (mask & 1) { // North (+Z)
        glPushMatrix();
        glRotatef(180, 0, 1, 0);
        tuboT->draw(1);
        if (fil > 0 && pipes3D[fil - 1][col] != 0) {
            float dh = (float)(mapaAlturas[fil - 1][col] - '0') - (float)(altura - '0');
            if (dh != 0) {
                glTranslatef(0.0, 0.0, -2.5);
                vConn->setConnector(dh * 2.0f);
                vConn->draw(1);
            }
        }
        glPopMatrix();
      }
      if (mask & 64) { // West (+X)
        glPushMatrix();
        glRotatef(270, 0, 1, 0);
        tuboT->draw(1);
        if (col > 0 && pipes3D[fil][col - 1] != 0) {
            float dh = (float)(mapaAlturas[fil][col - 1] - '0') - (float)(altura - '0');
            if (dh != 0) {
                glTranslatef(0.0, 0.0, -2.5);
                vConn->setConnector(dh * 2.0f);
                vConn->draw(1);
            }
        }
        glPopMatrix();
      }
    }
  }
}

/**
 * @brief Dibuja la forma 3D simplificada de una entidad según su tipo.
 *
 * Utiliza cilindros para ingeniero ('i') y técnico ('t'), esferas
 * para vándalos ('v') y cubos para excursionistas ('e').
 *
 * @param tipoParam  Carácter que identifica el tipo de entidad.
 */
void Mapa::formaEntidad(unsigned char tipoParam)
{
  RevolucionObj3D *jug = new RevolucionObj3D("ply/cilindro.ply", 3);
  switch (tipoParam)
  {
  case 'i':
    glRotatef(90.0, 1, 0, 0);
    glScalef(1.0, 1.0, 0.75);
    jug->setColor(_vertex3<float>(1.0, 0.0, 0.0));
    jug->draw(1);
    break;

  case 't':
    glRotatef(90.0, 1, 0, 0);
    glScalef(1.0, 1.0, 0.75);
    jug->setColor(_vertex3<float>(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0));
    jug->draw(1);
    break;

  case 'v':
    glColor3f(1.0, 5.0 / 255.0, 238 / 255.0);
    glutSolidSphere(0.25, 16, 16);  // Improved subdivision for smoother appearance
    break;
  case 'e':
    glColor3f(255.0 / 255.0, 159.0 / 255.0, 5.0 / 255.0);
    glutSolidCube(0.6);
    break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
    glColor3f(1.0, 5.0 / 255.0, 238 / 255.0);
    glutSolidSphere(0.5, 16, 16);  // Improved subdivision for smoother appearance
    break;
  }
}

/**
 * @brief Dibuja la ventana de minimapa principal (MM1).
 *
 * Renderiza el mapa completo con colores de celda, iconos, entidades,
 * objetivos y marcadores visuales. Soporta vista topológica y de terreno.
 *
 * @param objetivosActivos  Vector de coordenadas de los objetivos activos.
 * @param level             Nivel de juego actual.
 * @param drawTopology      Si @c true, dibuja la vista topológica
 *                          (escala de grises por altura).
 */
void Mapa::drawMM1(vector<unsigned int> objetivosActivos, int level, bool drawTopology)
{
  unsigned int colMed, filaMed;

  colMed = (mapaCompleto.size() / 2);
  filaMed = (mapaCompleto[0].size() / 2);

  glLoadIdentity();
  glOrtho(-10.0, 10.0, -10.0, 10.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);

  float ratio = 1.0;
  if (mapaCompleto.size() > 15)
  {
    ratio = 20.0 / (float)mapaCompleto.size();
  }

  // Fog of War: get agent's discovered map for levels 0, 1, 6
  vector<vector<unsigned char>> mapaVisible;
  bool useFogMM1 = (level == 0 || level == 1 || level == 6);
  if (useFogMM1 && entidades->size() > 0) {
    mapaVisible = (*entidades)[0]->getMapaResultado();
  }

  for (unsigned int i = 0; i < mapaCompleto.size(); i++)
  {
    for (unsigned int j = 0; j < mapaCompleto[i].size(); j++)
    {
      glPushMatrix();

      glTranslatef(((GLfloat)j - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)i) * ratio, 0.0);
      glScalef(ratio, ratio, ratio);

      // Fog of war check for this cell
      bool isFoggedMM1 = false;
      if (useFogMM1 && i < mapaVisible.size() && j < mapaVisible[0].size() &&
          mapaVisible[i][j] == '?') {
        isFoggedMM1 = true;
      }

      if (drawTopology) {
        colorTopologyMM(mapaAlturas[i][j]);
      } else {
        colorCeldaMM(mapaCompleto[i][j], mapaAlturas[i][j]);
      }

      glBegin(GL_QUADS);
      glVertex3f(-0.5, -0.5, 0.0);
      glVertex3f(-0.5, 0.5, 0.0);
      glVertex3f(0.5, 0.5, 0.0);
      glVertex3f(0.5, -0.5, 0.0);
      glEnd();

      if (isFoggedMM1) {
        // Draw whitish fog overlay on undiscovered cells
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.85, 0.85, 0.90, 0.80);
        glBegin(GL_QUADS);
        glVertex3f(-0.5, -0.5, 0.1);
        glVertex3f(-0.5, 0.5, 0.1);
        glVertex3f(0.5, 0.5, 0.1);
        glVertex3f(0.5, -0.5, 0.1);
        glEnd();
        glDisable(GL_BLEND);
      }

      if (!isFoggedMM1) {
        if (drawTopology) {
          renderHeightValue(mapaAlturas[i][j]);
        }

        if (!drawTopology) {
          drawIconoCeldaMM(mapaCompleto[i][j], mapaAlturas[i][j], i, j);
        }

        // Draw Failed Actions strictly after cell background and icons
        drawVisualMarkersMM(i, j, monitor);
      }

      glPopMatrix();
    }
  }

  // Draw Plans
  auto drawPlanMM = [&](ListaCasillasPlan &p, int planType) {
    if (p.size() == 0) return;
    int idx = 0;
    auto it = p.begin();
    while (it != p.end()) {
      glPushMatrix();
      glTranslatef(((GLfloat)it->col - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)it->fil) * ratio, 0.0);
      glScalef(ratio, ratio, ratio);
      if (planType != 2)
        drawPathMarker(static_cast<Action>(it->op), planType, idx, 0, 0, 0, false);
      glPopMatrix();

      // Draw Line to next node
      auto nextIt = std::next(it);
      if (nextIt != p.end()) {
        glLineWidth(2.0f);
        if (planType == 0) glColor3f(1.0f, 1.0f, 1.0f);      // White
        else if (planType == 1) glColor3f(1.0f, 1.0f, 0.0f); // Yellow
        else glColor3f(1.0f, 0.0f, 0.0f);                   // Red

        glBegin(GL_LINES);
        glVertex3f(((GLfloat)it->col - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)it->fil) * ratio, 0.05);
        glVertex3f(((GLfloat)nextIt->col - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)nextIt->fil) * ratio, 0.05);
        glEnd();
      }

      ++it;
      ++idx;
    }
  };

  ListaCasillasPlan p0 = (*entidades)[0]->getMapaPlan();
  ListaCasillasPlan p1 = (*entidades)[1]->getMapaPlan();
  ListaCasillasPlan p2 = (*entidades)[0]->getCanalizacionPlan();
  drawPlanMM(p0, 0);
  drawPlanMM(p1, 1);
  drawPlanMM(p2, 2);

  if (level > 1)
  {
    for (unsigned int i = 0; i < objetivosActivos.size(); i = i + 3)
    {
      glPushMatrix();
      glTranslatef(((GLfloat)objetivosActivos[i + 1] - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)objetivosActivos[i]) * ratio, 0);
      glScalef(ratio, ratio, ratio);
      
      drawBelPos2D();

      glPopMatrix();
    }
  }

  for (unsigned int i = 0; i < entidades->size(); i++)
  {
    if ((*entidades)[i]->getHitbox())
    {
      glPushMatrix();

      glTranslatef(((GLfloat)(*entidades)[i]->getCol() - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)(*entidades)[i]->getFil()) * ratio, 0.0);

      OrientacionEntidadMM((*entidades)[i]->getOrientacion());

      glScalef(ratio, ratio, ratio);

      formaEntidad((*entidades)[i]->getSubTipoChar());

      glPopMatrix();
    }
  }
}

/**
 * @brief Combina dos mapas de superficie: copia las casillas conocidas
 *        de la fuente en las posiciones desconocidas ('?') del resultado.
 *
 * @param[in,out] resultado  Mapa destino; las casillas '?' se sobrescriben.
 * @param fuente             Mapa origen de donde se copian las casillas.
 */
void Mapa::JoinMapasSuperficie(vector<vector<unsigned char>> &resultado, const vector<vector<unsigned char>> &fuente)
{
  for (int i = 0; i < resultado.size(); i++)
  {
    for (int j = 0; j < resultado[0].size(); j++)
    {
      if (resultado[i][j] == '?')
        resultado[i][j] = fuente[i][j];
    }
  }
}

// Eliminada JoinMapasPlan

/**
 * @brief Dibuja la segunda ventana de minimapa (MM2), que muestra el
 *        mapa visto por un agente concreto o la fusión de ambos.
 *
 * @param objetivosActivos  Coordenadas de los objetivos activos.
 * @param level             Nivel de juego.
 * @param drawTopology      Si @c true, vista topológica.
 * @param mapMode           Modo del mapa: 0 = real, 1 = ingeniero,
 *                          2 = técnico, 3 = fusión.
 */
void Mapa::drawMM2(vector<unsigned int> objetivosActivos, int level, bool drawTopology, int mapMode)
{
  vector<vector<unsigned char>> mapaSuperficie;
  if (mapMode == 0) { // Ver - full map
    mapaSuperficie = mapaCompleto;
  } else if (mapMode == 1) { // Ingeniero
    mapaSuperficie = (*entidades)[0]->getMapaResultado();
  } else if (mapMode == 2) { // Tecnico
    mapaSuperficie = (*entidades)[1]->getMapaResultado();
  } else if (mapMode == 3) { // Conjunto - engineer takes precedence
    mapaSuperficie = (*entidades)[0]->getMapaResultado();
    JoinMapasSuperficie(mapaSuperficie, (*entidades)[1]->getMapaResultado());
  }
  
  unsigned int colMed, filaMed;

  colMed = (mapaCompleto.size() / 2);
  filaMed = (mapaCompleto[0].size() / 2);

  glLoadIdentity();
  glOrtho(-10.0, 10.0, -10.0, 10.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);

  float ratio = 1.0;
  if (mapaCompleto.size() > 15)
  {
    ratio = 20.0 / (float)mapaCompleto.size();
  }

  for (unsigned int i = 0; i < mapaCompleto.size(); i++)
  {
    for (unsigned int j = 0; j < mapaCompleto[i].size(); j++)
    {
      glPushMatrix();

      glTranslatef(((GLfloat)j - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)i) * ratio, 0.0);
      glScalef(ratio, ratio, ratio);

      if (drawTopology) {
        colorTopologyMM(mapaAlturas[i][j]);
      } else {
        colorCeldaMM(mapaSuperficie[i][j], mapaAlturas[i][j]);
      }

      glBegin(GL_QUADS);
      glVertex3f(-0.5, -0.5, 0.0);
      glVertex3f(-0.5, 0.5, 0.0);
      glVertex3f(0.5, 0.5, 0.0);
      glVertex3f(0.5, -0.5, 0.0);
      glEnd();

      if (drawTopology) {
        renderHeightValue(mapaAlturas[i][j]);
      }

      drawIconoCeldaMM(mapaSuperficie[i][j], mapaAlturas[i][j], i, j);
      drawVisualMarkersMM(i, j, monitor);
 
      glPopMatrix();
    }
  }

  // Draw Plan
  auto drawPlanMM2 = [&](ListaCasillasPlan &p, int planType) {
    if (p.size() == 0) return;
    int idx = 0;
    auto it = p.begin();
    while (it != p.end()) {
      glPushMatrix();
      glTranslatef(((GLfloat)it->col - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)it->fil) * ratio, 0.0);
      glScalef(ratio, ratio, ratio);
      if (planType != 2)
        drawPathMarker(static_cast<Action>(it->op), planType, idx, 0, 0, 0, false);
      glPopMatrix();

      // Draw Line to next node
      auto nextIt = std::next(it);
      if (nextIt != p.end()) {
        glLineWidth(2.0f);
        if (planType == 0) glColor3f(1.0f, 1.0f, 1.0f);      // White
        else if (planType == 1) glColor3f(1.0f, 1.0f, 0.0f); // Yellow
        else glColor3f(1.0f, 0.0f, 0.0f);                   // Red

        glBegin(GL_LINES);
        glVertex3f(((GLfloat)it->col - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)it->fil) * ratio, 0.05);
        glVertex3f(((GLfloat)nextIt->col - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)nextIt->fil) * ratio, 0.05);
        glEnd();
      }

      ++it;
      ++idx;
    }
  };

  ListaCasillasPlan pl0 = (*entidades)[0]->getMapaPlan();
  ListaCasillasPlan pl1 = (*entidades)[1]->getMapaPlan();
  ListaCasillasPlan pl2 = (*entidades)[0]->getCanalizacionPlan();
  drawPlanMM2(pl0, 0);
  drawPlanMM2(pl1, 1);
  drawPlanMM2(pl2, 2);

  if (level > 1)
  {
    for (unsigned int i = 0; i < objetivosActivos.size(); i = i + 3)
    {
      glPushMatrix();
      glTranslatef(((GLfloat)objetivosActivos[i + 1] - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)objetivosActivos[i]) * ratio, 0);
      glScalef(ratio, ratio, ratio);
      
      drawBelPos2D();
      glPopMatrix();
    }
  }

  for (unsigned int i = 0; i < entidades->size(); i++)
  {
    // cout << "Entidad " << i << " tipo " << (*entidades)[i]->getSubTipoChar() << " (" << (*entidades)[i]->getFil() << ", " << (*entidades)[i]->getCol() << endl;
    if ((*entidades)[i]->getHitbox())
    {
      glPushMatrix();

      glTranslatef(((GLfloat)(*entidades)[i]->getCol() - (GLfloat)filaMed) * ratio, ((GLfloat)colMed - (GLfloat)(*entidades)[i]->getFil()) * ratio, 0.0);

      OrientacionEntidadMM((*entidades)[i]->getOrientacion());

      glScalef(ratio, ratio, ratio);

      formaEntidad((*entidades)[i]->getSubTipoChar());

      glPopMatrix();
    }
  }
}

struct TrailNode {
  float x, z;
  float alpha;
};
static vector<TrailNode> agent_trail;

/**
 * @brief Dibuja una rejilla de alambre (wireframe) sobre las celdas
 *        visibles.
 *
 * Añade líneas de borde para mejorar la percepción de profundidad en
 * 3D.
 */
void Mapa::drawWireframeOverlay() {
    if (!drawGridFP) return;
    glDisable(GL_LIGHTING);
    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(1.0f);
    glutWireCube(1.002f);
    glEnable(GL_LIGHTING);
}

/**
 * @brief Renderiza la vista en primera persona centrada en una entidad.
 *
 * Dibuja el terreno 3D con sus complementos, entidades, objetivos,
 * planes de ruta y la cámara ubicada en la posición del agente
 * seleccionado.
 *
 * @param entidad            Índice de la entidad en la que se sitúa la cámara.
 * @param objetivosActivos   Coordenadas de los objetivos activos.
 * @param level              Nivel de juego actual.
 */
void Mapa::drawFirstPerson(int entidad, vector<unsigned int> objetivosActivos, int level)
{
  unsigned int colMed, filaMed;

  colMed = (mapaCompleto.size() / 2);
  filaMed = (mapaCompleto[0].size() / 2);

  if (entidades->size() > 0)
  {
    z = (*entidades)[entidad]->getFil() + 1;
    x = (*entidades)[entidad]->getCol() + 1;

    switch ((*entidades)[entidad]->getOrientacion())
    {
    case norte: angulo = 180.0; break;
    case noreste: angulo = 215.0; break;
    case este: angulo = 270.0; break;
    case sureste: angulo = 315.0; break;
    case sur: angulo = 0.0; break;
    case suroeste: angulo = 45.0; break;
    case oeste: angulo = 90.0; break;
    case noroeste: angulo = 135.0; break;
    }
  }

  // Update Trail
  if (entidades->size() > 0 && (*entidades)[entidad]->getTipo() == jugador) {
    float curX = ((GLfloat)filaMed - (GLfloat)x) * 5.0;
    float curZ = ((GLfloat)colMed - (GLfloat)z) * 5.0;
    if (agent_trail.empty() || abs(agent_trail.back().x - curX) > 0.5 || abs(agent_trail.back().z - curZ) > 0.5) {
      TrailNode tn = {curX, curZ, 1.0f};
      agent_trail.push_back(tn);
      if (agent_trail.size() > 20) agent_trail.erase(agent_trail.begin());
    }
  }
  for (auto& tn : agent_trail) tn.alpha -= 0.001f;


  // Altura del suelo donde está el agente
  float ground_Y = (mapaAlturas[z - 1][x - 1]) * 2.0f;
  if (mapaCompleto[z - 1][x - 1] == 'A') {
    ground_Y -= 0.9f; // Sink camera in water (just above surface)
  }

  // Camara: Variables de control parametrizadas
  float units_elevated = 2; // Altura de la camara
  float units_behind = -1.0;   // Distancia detras del personaje
  float units_focus = 2.0;    // Distancia de enfoque delante

  float ang_rad = (angulo * 2.0 * M_PI) / 360.0;
  float eyeX_base = ((GLfloat)filaMed - (GLfloat)x) * 5.0; 
  float eyeZ_base = ((GLfloat)colMed - (GLfloat)z) * 5.0;  

  gluLookAt(
      eyeX_base - units_behind * sin(ang_rad), units_elevated + ground_Y + Y2, eyeZ_base + units_behind * cos(ang_rad), // Position (Eye)
      eyeX_base + units_focus * sin(ang_rad), (units_elevated - 0.1) + ground_Y - Y1, eyeZ_base - units_focus * cos(ang_rad), // Focus (Center)
      0.0, 1.0, 0.0); // Up vector

  // Optimized rendering range: Reduced from 40 to 25
  int fs = (int)x - 25;
  int fi = (int)x + 25;
  int ci = (int)z - 25;
  int cd = (int)z + 25;

  Luz luz(GL_LIGHT0);
  luz.setPosicion(_vertex3<float>(0.5, 1.0, 0.5)); // Directional Sun: Fixed angle everywhere
  luz.encender();
  luz.draw();

  // --- Draw Clouds (Pixel-Art Clusters) ---
  /* Nubes anuladas por rendimiento CPU/GPU
  glDisable(GL_LIGHTING);
  glDisable(GL_FOG); // Ensure clouds are not washed out by fog
  glColor3f(0.95, 0.95, 1.0); // Very light blueish white
  
  // Draw clouds in a grid to cover the sky
  for (int cloudX = -3; cloudX <= 3; cloudX++) {
    for (int cloudZ = -3; cloudZ <= 3; cloudZ++) {
      glPushMatrix();
      // Position clouds at Y=65, spread them out widely
      glTranslatef(cloudX * 180.0, 65.0, cloudZ * 180.0);
      
      // Main central block
      glPushMatrix();
      glScalef(60.0, 10.0, 60.0);
      glutSolidCube(1.0);
      glPopMatrix();
      
      // Side blocks for asymmetrical pixel-art look
      glPushMatrix();
      glTranslatef(30.0, -3.0, 20.0);
      glScalef(40.0, 8.0, 30.0);
      glutSolidCube(1.0);
      glPopMatrix();

      glPushMatrix();
      glTranslatef(-25.0, 2.0, -25.0);
      glScalef(35.0, 7.0, 45.0);
      glutSolidCube(1.0);
      glPopMatrix();
      
      glPopMatrix();
    }
  }
  glEnable(GL_FOG);
  glEnable(GL_LIGHTING);
  */
  // ----------------------------------------

  // Draw Trail
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for (const auto& tn : agent_trail) {
    if (tn.alpha <= 0) continue;
    glPushMatrix();
    glTranslatef(tn.x, 0.22, tn.z);
    glColor4f(0.5, 0.4, 0.2, tn.alpha * 0.5f);
    glScalef(2.0, 0.05, 2.0);
    glutSolidCube(1.0); drawWireframeOverlay();
    glPopMatrix();
  }
  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);


  static float water_offset = 0.0f;
  // water_offset += 0.005f; // Comentado para aligerar la simulación

  // Fog of War: get agent's discovered map for levels 0, 1, 6
  vector<vector<unsigned char>> mapaVisible;
  bool useFog = (level == 0 || level == 1 || level == 6);
  if (useFog && entidades->size() > (unsigned)entidad) {
    mapaVisible = (*entidades)[entidad]->getMapaResultado();
  }

  for (int i = fs; i < fi; i++)
  {
    for (int j = ci; j < cd; j++)
    {
      unsigned char celdaActual;
      unsigned char alturaActual;

      // Handle Map Boundaries
      if (i < 0 || i >= (int)mapaCompleto[0].size() || j < 0 || j >= (int)mapaCompleto.size()) {
          celdaActual = 'P';
          alturaActual = '0';
      } else {
          celdaActual = mapaCompleto[j][i];
          alturaActual = mapaAlturas[j][i];
      }
      
      // Check if this cell is an active goal
      bool isGoal = false;
      if (level > 1) {
          for (unsigned int k = 0; k < objetivosActivos.size(); k += 3) {
              if (objetivosActivos[k] == j && objetivosActivos[k+1] == i) {
                  isGoal = true;
                  break;
              }
          }
      }

      // LOD Optimization: Calculate distance to camera
      float cellX = ((GLfloat)filaMed - (GLfloat)i - 1) * 5.0;
      float cellZ = ((GLfloat)colMed - (GLfloat)j - 1) * 5.0;
      float distSq = (cellX - eyeX_base)*(cellX - eyeX_base) + (cellZ - eyeZ_base)*(cellZ - eyeZ_base);
      bool isNear = distSq < 10000.0f; // 100 units threshold

      // Suelo
      glPushMatrix();

      // Fog of war check
      bool isFogged = false;
      if (useFog && j >= 0 && j < (int)mapaVisible.size() &&
          i >= 0 && i < (int)mapaVisible[0].size() &&
          mapaVisible[j][i] == '?') {
        isFogged = true;
        glColor3f(0.15, 0.12, 0.20); // Darken undiscovered terrain
      }

      int h = alturaActual;
      float h_scale = 0.5f + (h / 10.0f); 
      float alt_Y = h * 2.0f;

      // If it's a precipice, force height to level 0
      if (celdaActual == 'P') {
          alt_Y = 0.0f;
      }

      const float Y_BASE = -10.0f;
      float pilar_h, pilar_center;

      if (celdaActual == 'P') // Precipice - Black Hole
      {
        glDisable(GL_LIGHTING);
        glColor3f(0.0, 0.0, 0.0);
        float top_Y = -0.05f; // Slightly below level 0 surface
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;

        glTranslatef(cellX, pilar_center, cellZ);
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0); drawWireframeOverlay();
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'K') // Bikini - metallic/shiny material
      {
        if (isNear) {
          GLfloat ambient[] = {0.3, 0.3, 0.0, 1.0};
          GLfloat diffuse[] = {0.8, 0.8, 0.2, 1.0};
          GLfloat specular[] = {1.0, 1.0, 0.5, 1.0};
          glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
          glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
          glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
          glMaterialf(GL_FRONT, GL_SHININESS, 60.0);
        }
        
        float top_Y = 0.25f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;

        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(mapaCompleto[j][i], mapaAlturas[j][i]);
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0); drawWireframeOverlay();
      }
      else if (mapaCompleto[j][i] == 'A') // Agua - Deep Ocean Blue (Procedural Waves)
      {
        if (isNear) {
          GLfloat ambient[] = {0.0, 0.1, 0.4, 1.0};
          GLfloat diffuse[] = {0.0, 0.5, 0.8, 1.0};
          GLfloat specular[] = {1.0, 1.0, 1.0, 1.0};
          glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
          glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
          glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
          glMaterialf(GL_FRONT, GL_SHININESS, 128.0);
        }
        
        float top_Y = -0.05f + alt_Y; // Slightly lower than ground
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;

        glTranslatef(cellX, pilar_center, cellZ);
        
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        colorCeldaMM(mapaCompleto[j][i], mapaAlturas[j][i]);
        glutSolidCube(1.0); drawWireframeOverlay();
        glPopMatrix();
        glEnable(GL_LIGHTING);

        // Animated Waves (Stay at the surface)
        if (isNear) {
          glDisable(GL_LIGHTING);
          glColor3f(0.4, 0.8, 1.0);
          glBegin(GL_LINES);
          for (float w = -2.0; w <= 2.0; w += 1.0) {
             float offset = sin(water_offset + i + j + w) * 0.5;
             glVertex3f(w + offset, (pilar_h/2.0f) + 0.01, -2.0); 
             glVertex3f(w + 0.5 + offset, (pilar_h/2.0f) + 0.01, 2.0);
          }
          glEnd();
          glEnable(GL_LIGHTING);
        }
      }
      else if (celdaActual == 'C') // Camino
      {
        float top_Y = 0.25f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glDisable(GL_LIGHTING);
        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(celdaActual, alturaActual);
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0); 
        glutSolidCube(1.0); drawWireframeOverlay();
        glPopMatrix();
        
        // Brown Tiles for Camino ('C') - 4x4 Grid
        if (isNear) {
          float surface_Y = pilar_h/2.0f;
          float tileSize = 1.15f; // Slightly smaller than 5.0/4 (1.25) to leave gaps/mortar
          
          for (int row = 0; row < 4; row++) {
              for (int col = 0; col < 4; col++) {
                  float px = -2.5f + col * 1.25f + 0.625f;
                  float pz = -2.5f + row * 1.25f + 0.625f;
                  
                  // Deterministic brown variation
                  if ((row + col) % 2 == 0) glColor3f(0.5, 0.35, 0.2); // Medium brown
                  else glColor3f(0.45, 0.3, 0.15);                   // Darker brown
                  
                  glBegin(GL_QUADS);
                  glVertex3f(px - tileSize/2.0f, surface_Y + 0.01f, pz - tileSize/2.0f);
                  glVertex3f(px + tileSize/2.0f, surface_Y + 0.01f, pz - tileSize/2.0f);
                  glVertex3f(px + tileSize/2.0f, surface_Y + 0.01f, pz + tileSize/2.0f);
                  glVertex3f(px - tileSize/2.0f, surface_Y + 0.01f, pz + tileSize/2.0f);
                  glEnd();
                  
                  // Subtle grout/mortar line
                  glColor3f(0.2, 0.15, 0.1); 
                  glLineWidth(1.0);
                  glBegin(GL_LINE_LOOP);
                  glVertex3f(px - tileSize/2.0f, surface_Y + 0.015f, pz - tileSize/2.0f);
                  glVertex3f(px + tileSize/2.0f, surface_Y + 0.015f, pz - tileSize/2.0f);
                  glVertex3f(px + tileSize/2.0f, surface_Y + 0.015f, pz + tileSize/2.0f);
                  glVertex3f(px - tileSize/2.0f, surface_Y + 0.015f, pz + tileSize/2.0f);
                  glEnd();
              }
          }
        }
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'S' || celdaActual == 'U') // Sendero / Pipe Ground
      {
        float top_Y = 0.25f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glDisable(GL_LIGHTING);
        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(celdaActual, alturaActual);
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0); drawWireframeOverlay();
        glPopMatrix();

        // Pavement Stones for Sendero (Dirt Brown)
        if (isNear) {
          float surface_Y = pilar_h/2.0f;
          for (int s = 0; s < 6; s++) {
              float px = (((i * 17 + j * 11 + s * 13) % 100) / 100.0f) * 4.2f - 2.1f;
              float pz = (((i * 11 + j * 17 + s * 7) % 100) / 100.0f) * 4.2f - 2.1f;
              float size = 0.4f + ((i + j + s) % 3) * 0.2f;
              
              if (s % 2 == 0) glColor3f(0.4, 0.3, 0.2); // Darker brown
              else glColor3f(0.5, 0.4, 0.3);           // Lighter brown
              
              glBegin(GL_TRIANGLE_FAN);
              glVertex3f(px, surface_Y + 0.01f, pz);
              for (int n = 0; n <= 6; n++) {
                  float angle = 2.0 * M_PI * n / 6.0;
                  glVertex3f(px + size * cos(angle), surface_Y + 0.01f, pz + size * sin(angle));
              }
              glEnd();
          }
        }
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'B') // Bosque
      {
        float top_Y = 0.6f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glDisable(GL_LIGHTING);
        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(celdaActual, alturaActual);
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0); drawWireframeOverlay();
        
        // Procedural Leaves (at the top)
        if (isNear && !isGoal) {
          glColor3f(0.1, 0.3, 0.0);
          glPointSize(3.0);
          glBegin(GL_POINTS);
          for (int p=0; p<5; p++) {
             float px = ((i*17 + j*31 + p*13) % 100) / 25.0 - 2.0;
             float pz = ((i*31 + j*17 + p*7) % 100) / 25.0 - 2.0;
             glVertex3f(px, pilar_h/2.0f + 0.01f, pz);
          }
          glEnd();
        }
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'H') // Hierba
      {
        float top_Y = 0.4f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glDisable(GL_LIGHTING);
        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(celdaActual, alturaActual);
        
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0); drawWireframeOverlay();
        glPopMatrix();

        // Procedural Flowers (Only if near)
        if (isNear) {
          glPointSize(3.0f);
          glBegin(GL_POINTS);
          for (int p = 0; p < 3; p++) {
            if ((i * 13 + j * 7 + p) % 2 == 0) glColor3f(1.0, 1.0, 1.0);
            else glColor3f(1.0, 1.0, 0.0);
            
            float fx = (float)((i * 17 + j * 31 + p * 23) % 100) / 25.0f - 2.0f;
            float fz = (float)((i * 31 + j * 17 + p * 19) % 100) / 25.0f - 2.0f;
            glVertex3f(fx, pilar_h/2.0f + 0.015f, fz);
          }
          glEnd();
        }
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'D' || celdaActual == 'Z') // Zapatillas
      {
        float top_Y = 0.2f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        
        // Base Block (Pillar)
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glColor3f(0.47 * h_scale, 0.38 * h_scale, 0.28 * h_scale);
        glutSolidCube(1.0); drawWireframeOverlay();
        glPopMatrix();
 
        // 3D Giant 'Z' (On the surface)
        if (isNear) {
          glDisable(GL_LIGHTING);
          glColor3f(0.6, 0.0, 1.0);
          float surface_Y = pilar_h/2.0f + 0.01f;
          
          glBegin(GL_QUADS);
          glVertex3f(-2.0, surface_Y, -2.0); glVertex3f(2.0, surface_Y, -2.0);
          glVertex3f(2.0, surface_Y, -1.0);  glVertex3f(-2.0, surface_Y, -1.0);
          glVertex3f(-2.0, surface_Y, 1.0);  glVertex3f(2.0, surface_Y, 1.0);
          glVertex3f(2.0, surface_Y, 2.0);   glVertex3f(-2.0, surface_Y, 2.0);
          glVertex3f(1.0, surface_Y, -1.0);  glVertex3f(2.0, surface_Y, -1.0);
          glVertex3f(-1.0, surface_Y, 1.0);  glVertex3f(-2.0, surface_Y, 1.0);
          glEnd();
          glEnable(GL_LIGHTING);
        }
      }
      else if (celdaActual == 'X') // Base - Diana
      {
        float top_Y = 0.2f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        
        // Base Block (Pillar)
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glColor3f(0.85 * h_scale, 0.75 * h_scale, 0.55 * h_scale);
        glutSolidCube(1.0); drawWireframeOverlay();
        glPopMatrix();
 
        // 3D Diana (On the surface)
        if (isNear) {
          glDisable(GL_LIGHTING);
          float surface_base = pilar_h/2.0f;
          float heights[] = {surface_base + 0.01f, surface_base + 0.02f, surface_base + 0.03f}; 
          float radii[] = {2.2, 1.4, 0.6};
          
          for (int k = 0; k < 3; ++k) {
            if (k % 2 == 0) glColor3f(1.0, 0.0, 1.0);
            else glColor3f(1.0, 1.0, 0.0);
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0.0, heights[k], 0.0);
            for (int n = 0; n <= 20; ++n) {
              float angle = 2.0 * M_PI * n / 20.0;
              glVertex3f(radii[k] * cos(angle), heights[k], radii[k] * sin(angle));
            }
            glEnd();
          }
          
          // Procedural Grainy Sand
          glColor3f(0.6*h_scale, 0.5*h_scale, 0.3*h_scale);
          glPointSize(2.0);
          glBegin(GL_POINTS);
          for (int p=0; p<4; p++) {
             float px = ((i*13 + j*7 + p*19) % 100) / 20.0 - 2.5;
             float pz = ((i*7 + j*13 + p*23) % 100) / 20.0 - 2.5;
             glVertex3f(px, surface_base + 0.01f, pz);
          }
          glEnd();
          glEnable(GL_LIGHTING);
        }
      }
      else // Default terrain - Sandy soil
      {
        float top_Y = 0.2f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        
        if (isNear) {
          GLfloat ambient[] = {0.45, 0.35, 0.2, 1.0};
          GLfloat diffuse[] = {0.7, 0.55, 0.35, 1.0};
          GLfloat specular[] = {0.15, 0.15, 0.15, 1.0};
          glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
          glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
          glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
          glMaterialf(GL_FRONT, GL_SHININESS, 12.0);
        }
        
        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(celdaActual, alturaActual);
        
        if (isNear && celdaActual != 'B' && celdaActual != 'P' && celdaActual != 'M' && 
            celdaActual != 'Z' && celdaActual != 'D' && celdaActual != 'X') {
            glColor3f(0.75*h_scale, 0.6*h_scale, 0.4*h_scale);
            
            glDisable(GL_LIGHTING);
            glPointSize(2.0);
            glBegin(GL_POINTS);
            for (int p=0; p<5; p++) {
               float px = ((i*13 + j*7 + p*19) % 100) / 20.0 - 2.5;
               float pz = ((i*7 + j*13 + p*23) % 100) / 20.0 - 2.5;
               glVertex3f(px, pilar_h/2.0f + 0.01f, pz);
            }
            glEnd();
            glEnable(GL_LIGHTING);
         }
 
        // Dibujamos el pilar con su propio escalado
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0); drawWireframeOverlay();
        glPopMatrix();
   
        // Marker drawing moved outside loop
      }

       // Fog of war: tall translucent fog curtain for undiscovered cells
       if (isFogged) {
         glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT);
         glDisable(GL_TEXTURE_2D);
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         
         float baseY = pilar_h / 2.0f;
         float t = water_offset * 0.4f;
         float cell_phase = (float)(i * 7 + j * 13);
         
         glEnable(GL_LIGHTING);
         GLfloat fog_ambient[] = {0.85f, 0.85f, 0.90f, 0.60f};
         GLfloat fog_diffuse[] = {0.92f, 0.92f, 0.96f, 0.55f};
         GLfloat fog_specular[] = {0.0f, 0.0f, 0.0f, 0.0f};
         glMaterialfv(GL_FRONT, GL_AMBIENT, fog_ambient);
         glMaterialfv(GL_FRONT, GL_DIFFUSE, fog_diffuse);
         glMaterialfv(GL_FRONT, GL_SPECULAR, fog_specular);
         glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);

         // Tall fog curtain cube with constant top height
         glPushMatrix();
         float swayX = sin(t * 0.5f + cell_phase) * 0.15f;
         float swayZ = cos(t * 0.4f + cell_phase + 2.0f) * 0.15f;
         float fog_top_Y = 22.0f;
         float fog_H = 30.0f;
         float relative_center = (fog_top_Y - fog_H/2.0f) - pilar_center;
         glTranslatef(swayX, relative_center, swayZ);
         glColor4f(0.90, 0.90, 0.95, 0.50);
         glScalef(5.2, fog_H, 5.2); // Wide as cell, very tall
         glutSolidCube(1.0);
         glPopMatrix();

         glPopAttrib();
       }
 
       glPopMatrix();
 
       // Complementos and markers: skip if fogged
       if (!isFogged && isNear) {
         glPushMatrix();
         float surface_offset = 0.2f; // Default
         if (celdaActual == 'B') surface_offset = 0.6f;
         else if (celdaActual == 'H') surface_offset = 0.4f;
         else if (celdaActual == 'K' || celdaActual == 'S' || celdaActual == 'U') surface_offset = 0.25f;
         
         glTranslatef(cellX, surface_offset + alt_Y, cellZ);
         complementosCelda(celdaActual, alturaActual, j, i, level);
         glPopMatrix();
         
         // Draw Visual Markers for actions/collisions in 3D
         const auto& markers = monitor.getVisualMarkers();
         for (const auto& vm : markers) {
             if (vm.f == j && vm.c == i) {
                 glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
                 glPushMatrix();
                 glTranslatef(cellX, alt_Y + 1.5f, cellZ);
                 glDisable(GL_LIGHTING);
                 
                  if (vm.type == 0 || vm.type == 1) { // Cross (0: Red, 1: Orange)
                      if (vm.type == 0) glColor3f(1.0, 0.0, 0.0);
                      else glColor3f(1.0, 0.5, 0.0);

                      glLineWidth(12.0f);
                      glBegin(GL_LINES);
                        glVertex3f(-1.0, -1.0, 0.0); glVertex3f(1.0, 1.0, 0.0);
                        glVertex3f(-1.0, 1.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
                      glEnd();
                  }
                 
                 glPopMatrix();
                 glPopAttrib();
                 break;
             }
         }
       }
    }
  }

   // --- Draw 3D Plan markers and lines ---
  drawPlans3D((*entidades)[0]->getMapaPlan(), 0, filaMed, colMed, mapaAlturas);
  drawPlans3D((*entidades)[1]->getMapaPlan(), 1, filaMed, colMed, mapaAlturas);
  drawPlans3D((*entidades)[0]->getCanalizacionPlan(), 2, filaMed, colMed, mapaAlturas);

  // --- Draw Goal Markers ---
  if (level > 1) {
    for (unsigned int i = 0; i < objetivosActivos.size(); i += 3) {
      int goalFil = objetivosActivos[i];
      int goalCol = objetivosActivos[i+1];

      glPushMatrix();
      float goalX = ((GLfloat)filaMed - (GLfloat)goalCol - 1) * 5.0;
      float goalZ = ((GLfloat)colMed - (GLfloat)goalFil - 1) * 5.0;
      float goal_alt_Y = (mapaAlturas[goalFil][goalCol]) * 2.0f;

      unsigned char celdaGoal = mapaCompleto[goalFil][goalCol];
      float surf_offset = 0.2f;
      if (celdaGoal == 'B') surf_offset = 0.6f;
      else if (celdaGoal == 'H') surf_offset = 0.4f;
      else if (celdaGoal == 'K' || celdaGoal == 'S' || celdaGoal == 'U') surf_offset = 0.25f;
      else if (celdaGoal == 'A') surf_offset = -0.05f;

      glTranslatef(goalX, goal_alt_Y + surf_offset, goalZ);
      drawBelPos3D();
      glPopMatrix();
    }
  }
 
    // Dibujamos las entidades
    for (unsigned int i = 0; i < entidades->size(); i++)
    {
       z = (*entidades)[i]->getFil() + 1;
       x = (*entidades)[i]->getCol() + 1;
 
       glPushMatrix();
       float ent_alt_Y = (mapaAlturas[z - 1][x - 1]) * 2.0f;
       
       // Surface offset adjustment: Most blocks have a top_Y = alt_Y + 0.25
       // Except Forest ('B') which is alt_Y + 0.6
       float surf_offset = 0.25f;
       if (mapaCompleto[z - 1][x - 1] == 'B') surf_offset = 0.6f;
       if (mapaCompleto[z - 1][x - 1] == 'A') {
         ent_alt_Y -= 1.5f; // Sink character in water (half body above surface)
         surf_offset = -0.05f;
       }
       
       glTranslatef(((GLfloat)filaMed - (GLfloat)x) * 5.0, ent_alt_Y + surf_offset, ((GLfloat)colMed - (GLfloat)z) * 5.0); 

       OrientacionEntidadFP((*entidades)[i]->getOrientacion());

       // Ensure no white shine on characters
       GLfloat no_specular[] = {0.0, 0.0, 0.0, 1.0};
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, no_specular);
       glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

       if (i == 0) {
         // Rescatador (Mario)
         (*entidades)[i]->draw(1);
       } else if (i == 1) {
         // tecnico (Luigi)
         (*entidades)[i]->draw(1);
       } else {
         // NPCs: Nuanced Gray
         GLfloat ambient[]  = {0.3f, 0.3f, 0.3f, 1.0f};
         GLfloat diffuse[]  = {0.6f, 0.6f, 0.6f, 1.0f};
         glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
         glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
         glColor3f(0.6, 0.6, 0.6);
         (*entidades)[i]->draw(1);
       }
 
       glPopMatrix();
    }
 }

/**
 * @brief Dibuja una cuña tridimensional ("quesito") usada como
 *        indicador del ángulo de visión.
 *
 * @param r      Radio de la cuña.
 * @param h      Altura de la cuña.
 * @param r_col  Componente roja del color.
 * @param g_col  Componente verde del color.
 * @param b_col  Componente azul del color.
 */
static void drawQuesito(float r, float h, float r_col, float g_col, float b_col) {
    glDisable(GL_LIGHTING);
    glColor3f(r_col, g_col, b_col);
    
    // Top
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, h, 0.0f); // Center high
    for(int i=150; i<=210; i+=5) {
        float a = i * M_PI / 180.0f;
        glVertex3f(r * sin(a), h, r * cos(a));
    }
    glEnd();

    // Bottom
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, 0.0f); // Center low
    for(int i=150; i<=210; i+=5) {
        float a = i * M_PI / 180.0f;
        glVertex3f(r * sin(a), 0.0f, r * cos(a));
    }
    glEnd();

    // Sides (Arc)
    glBegin(GL_QUAD_STRIP);
    for(int i=150; i<=210; i+=5) {
        float a = i * M_PI / 180.0f;
        float x = r * sin(a);
        float z = r * cos(a);
        glVertex3f(x, h, z);
        glVertex3f(x, 0.0f, z);
    }
    glEnd();

    // Sides (Flat radial) - Closing the wedge
    glBegin(GL_QUADS);
    // Left face
    float a_left = 150.0f * M_PI / 180.0f;
    glVertex3f(0.0f, h, 0.0f);
    glVertex3f(r*sin(a_left), h, r*cos(a_left));
    glVertex3f(r*sin(a_left), 0.0f, r*cos(a_left));
    glVertex3f(0.0f, 0.0f, 0.0f);

    // Right face
    float a_right = 210.0f * M_PI / 180.0f;
    glVertex3f(0.0f, h, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(r*sin(a_right), 0.0f, r*cos(a_right));
    glVertex3f(r*sin(a_right), h, r*cos(a_right));
    glEnd();

    glEnable(GL_LIGHTING);
}

/**
 * @brief Renderiza la vista cenital (vista de pájaro) centrada en una
 *        entidad.
 *
 * Dibuja el terreno 3D, complementos, entidades, objetivos, planes
 * de ruta y la cuña del ángulo de visión del agente desde arriba.
 *
 * @param entidad            Índice de la entidad central.
 * @param objetivosActivos   Coordenadas de los objetivos activos.
 * @param level              Nivel de juego actual.
 * @param zoom               Nivel de zoom de la cámara.
 */
void Mapa::drawZenithal(int entidad, vector<unsigned int> objetivosActivos, int level, float zoom)
{
  unsigned int colMed, filaMed;

  colMed = (mapaCompleto.size() / 2);
  filaMed = (mapaCompleto[0].size() / 2);

  if (entidades->size() > 0)
  {
    z = (*entidades)[entidad]->getFil() + 1;
    x = (*entidades)[entidad]->getCol() + 1;
  }

  // Camera: Positioned above looking straight down
  float eyeX_base = ((GLfloat)filaMed - (GLfloat)x) * 5.0; // Corrected: filaMed is for Cols/X
  float eyeZ_base = ((GLfloat)colMed - (GLfloat)z) * 5.0;  // Corrected: colMed is for Rows/Z

  float camHeight = zoom;
  gluLookAt(
      eyeX_base, camHeight, eyeZ_base, // Position (Eye)
      eyeX_base, 0.0, eyeZ_base,        // Focus (Center) - Straight down
      0.0, 0.0, 1.0);                   // Up vector: +Z is North, North is UP on screen

  // Rendering range scales with zoom
  int range = (int)(35.0f * zoom / 55.0f);

  Luz luz(GL_LIGHT0);
  luz.setPosicion(_vertex3<float>(0.5, 1.0, 0.5)); // Directional Sun: Fixed angle everywhere
  luz.encender();
  luz.draw();
  int fs = (int)x - range;
  int fi = (int)x + range;
  int ci = (int)z - range;
  int cd = (int)z + range;

  static float water_offset_zenith = 0.0f;
  // water_offset_zenith += 0.005f; // Comentado para aligerar la simulación

  // Fog of War: get agent's discovered map for levels 0, 1, 6
  vector<vector<unsigned char>> mapaVisible;
  bool useFog = (level == 0 || level == 1 || level == 6);
  if (useFog && entidades->size() > (unsigned)entidad) {
    mapaVisible = (*entidades)[entidad]->getMapaResultado();
  }

  for (int i = fs; i < fi; i++)
  {
    for (int j = ci; j < cd; j++)
    {
      unsigned char celdaActual;
      unsigned char alturaActual;

      // Handle Map Boundaries
      if (i < 0 || i >= (int)mapaCompleto[0].size() || j < 0 || j >= (int)mapaCompleto.size()) {
          celdaActual = 'P';
          alturaActual = '0';
      } else {
          celdaActual = mapaCompleto[j][i];
          alturaActual = mapaAlturas[j][i];
      }

      // Check if this cell is an active goal
      bool isGoal = false;
      if (level > 1) {
          for (unsigned int k = 0; k < objetivosActivos.size(); k += 3) {
              if (objetivosActivos[k] == j && objetivosActivos[k+1] == i) {
                  isGoal = true;
                  break;
              }
          }
      }

      // LOD Optimization: Calculate distance to camera (horizontal)
      float cellX = ((GLfloat)filaMed - (GLfloat)i - 1) * 5.0;
      float cellZ = ((GLfloat)colMed - (GLfloat)j - 1) * 5.0;
      float distSq = (cellX - eyeX_base)*(cellX - eyeX_base) + (cellZ - eyeZ_base)*(cellZ - eyeZ_base);
      bool isNear = distSq < 10000.0f; // 100 units threshold

      // Suelo
      glPushMatrix();

      // Fog of war check
      bool isFogged = false;
      if (useFog && j >= 0 && j < (int)mapaVisible.size() &&
          i >= 0 && i < (int)mapaVisible[0].size() &&
          mapaVisible[j][i] == '?') {
        isFogged = true;
        glColor3f(0.15, 0.12, 0.20); // Darken undiscovered terrain
      }

      int h = alturaActual;
      float h_scale = 0.5f + (h / 10.0f); 
      float alt_Y = h * 2.0f;

      if (celdaActual == 'P') {
          alt_Y = 0.0f;
      }

      const float Y_BASE = -10.0f;
      float pilar_h, pilar_center;

      if (celdaActual == 'P') // Precipice - Black Hole
      {
        glDisable(GL_LIGHTING);
        glColor3f(0.0, 0.0, 0.0);
        float top_Y = -0.05f;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0);
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'K') // Bikini
      {
        float top_Y = 0.25f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(celdaActual, alturaActual);
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0);
      }
      else if (celdaActual == 'A') // Agua
      {
        float top_Y = -0.05f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        colorCeldaMM(celdaActual, alturaActual);
        glutSolidCube(1.0);
        glPopMatrix();

        // --- Animated Waves (Only if near) ---
        if (isNear) {
          glColor3f(0.5, 0.9, 1.0); 
          float surface_base = pilar_h / 2.0f;
          glBegin(GL_LINES);
          for (float w = -2.0; w <= 2.0; w += 1.0) {
             float offset = sin(water_offset_zenith + i + j + w) * 0.5;
             glVertex3f(w + offset, surface_base + 0.015f, -2.0); 
             glVertex3f(w + 0.5 + offset, surface_base + 0.015f, 2.0);
          }
          glEnd();
        }
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'B') // Bosque - Match 1st Person
      {
        float top_Y = 0.6f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        
        glDisable(GL_LIGHTING);
        colorCeldaMM(celdaActual, alturaActual);
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0);
        glPopMatrix();
        
        // Procedural Leaves (Only if near)
        if (isNear) {
          glColor3f(0.1, 0.3, 0.0);
          glPointSize(3.0);
          glBegin(GL_POINTS);
          for (int p=0; p<5; p++) {
             float px = ((i*17 + j*31 + p*13) % 100) / 25.0 - 2.0;
             float pz = ((i*31 + j*17 + p*7) % 100) / 25.0 - 2.0;
             glVertex3f(px, pilar_h/2.0f + 0.01f, pz);
          }
          glEnd();
        }
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'H') // Hierba - Match 1st Person
      {
        float top_Y = 0.4f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        
        glDisable(GL_LIGHTING);
        colorCeldaMM(celdaActual, alturaActual);
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0);
        glPopMatrix();

        // Procedural Flowers (Only if near)
        if (isNear) {
          glPointSize(3.0f);
          glBegin(GL_POINTS);
          for (int p = 0; p < 3; p++) {
            if ((i * 13 + j * 7 + p) % 2 == 0) glColor3f(1.0, 1.0, 1.0);
            else glColor3f(1.0, 1.0, 0.0);
            
            float fx = (float)((i * 17 + j * 31 + p * 23) % 100) / 25.0f - 2.0f;
            float fz = (float)((i * 31 + j * 17 + p * 19) % 100) / 25.0f - 2.0f;
            glVertex3f(fx, pilar_h/2.0f + 0.015f, fz);
          }
          glEnd();
        }
        glEnable(GL_LIGHTING);
      }
      else if (celdaActual == 'X') // Base - Diana
      {
        float top_Y = 0.2f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        colorCeldaMM(celdaActual, alturaActual);
        glutSolidCube(1.0);
        glPopMatrix();
 
        if (isNear) {
          glDisable(GL_LIGHTING);
          float surface_Y = pilar_h/2.0f + 0.02f;
          float radii[] = {2.2, 1.4, 0.6};
          for (int k = 0; k < 3; ++k) {
            if (k % 2 == 0) glColor3f(1.0, 0.0, 1.0);
            else glColor3f(1.0, 1.0, 0.0);
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0.0, surface_Y + (k*0.01f), 0.0);
            for (int n = 0; n <= 20; ++n) {
              float angle = 2.0 * M_PI * n / 20.0;
              glVertex3f(radii[k] * cos(angle), surface_Y + (k*0.01f), radii[k] * sin(angle));
            }
            glEnd();
          }
          glEnable(GL_LIGHTING);
        }
      }
      else if (celdaActual == 'C') // Camino (Tiles)
      {
        float top_Y = 0.2f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(celdaActual, alturaActual);
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0);
        glPopMatrix();

        if (isNear) {
          glDisable(GL_LIGHTING);
          float surface_Y = pilar_h/2.0f + 0.02f;
          float tileSize = 1.15f; 
          for (int r = 0; r < 4; r++) {
              for (int c = 0; c < 4; c++) {
                  float px = -2.5f + c * 1.25f + 0.625f;
                  float pz = -2.5f + r * 1.25f + 0.625f;
                  if ((r + c) % 2 == 0) glColor3f(0.5, 0.35, 0.2);
                  else glColor3f(0.45, 0.3, 0.15);
                  glBegin(GL_QUADS);
                  glVertex3f(px - tileSize/2.0f, surface_Y + 0.01f, pz - tileSize/2.0f);
                  glVertex3f(px + tileSize/2.0f, surface_Y + 0.01f, pz - tileSize/2.0f);
                  glVertex3f(px + tileSize/2.0f, surface_Y + 0.01f, pz + tileSize/2.0f);
                  glVertex3f(px - tileSize/2.0f, surface_Y + 0.01f, pz + tileSize/2.0f);
                  glEnd();
              }
          }
          glEnable(GL_LIGHTING);
        }
      }
      else if (celdaActual == 'S' || celdaActual == 'U') // Sendero / Pipe Ground
      {
        float top_Y = 0.25f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        colorCeldaMM(celdaActual, alturaActual);
        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0);
        glPopMatrix();

        // Pavement Stones for Sendero / Pipe Ground
        if (isNear) {
          glDisable(GL_LIGHTING);
          float surface_Y = pilar_h/2.0f + 0.02f;
          for (int s = 0; s < 6; s++) {
              float px = (((i * 17 + j * 11 + s * 13) % 100) / 100.0f) * 4.2f - 2.1f;
              float pz = (((i * 11 + j * 17 + s * 7) % 100) / 100.0f) * 4.2f - 2.1f;
              float size = 0.4f + ((i + j + s) % 3) * 0.2f;
              
              if (s % 2 == 0) glColor3f(0.4, 0.3, 0.2); // Darker brown
              else glColor3f(0.5, 0.4, 0.3);           // Lighter brown
              
              glBegin(GL_TRIANGLE_FAN);
              glVertex3f(px, surface_Y + 0.01f, pz);
              for (int n = 0; n <= 6; n++) {
                  float angle = 2.0 * M_PI * n / 6.0;
                  glVertex3f(px + size * cos(angle), surface_Y + 0.01f, pz + size * sin(angle));
              }
              glEnd();
          }
          glEnable(GL_LIGHTING);
        }
      }
      else // Generic Terrain
      {
        float top_Y = 0.2f + alt_Y;
        pilar_h = top_Y - Y_BASE;
        pilar_center = (top_Y + Y_BASE) / 2.0f;
        glTranslatef(cellX, pilar_center, cellZ);
        
        colorCeldaMM(celdaActual, alturaActual);
        if (celdaActual != 'Z' && celdaActual != 'D') {
           glColor3f(0.75*h_scale, 0.6*h_scale, 0.4*h_scale);
        }

        glPushMatrix();
        glScalef(5.0, pilar_h, 5.0);
        glutSolidCube(1.0);
        glPopMatrix();
        
        if (isNear && celdaActual != 'Z' && celdaActual != 'D') {
          glDisable(GL_LIGHTING);
          glColor3f(0.6*h_scale, 0.5*h_scale, 0.3*h_scale);
          glPointSize(2.0);
          glBegin(GL_POINTS);
          for (int p=0; p<5; p++) {
             float px = ((i*13 + j*7 + p*19) % 100) / 20.0 - 2.5;
             float pz = ((i*7 + j*13 + p*23) % 100) / 20.0 - 2.5;
             glVertex3f(px, pilar_h/2.0f + 0.01f, pz);
          }
          glEnd();
          glEnable(GL_LIGHTING);
        }

        // Marker drawing moved outside loop
      }

      // Fog of war: tall translucent fog curtain for undiscovered cells
      if (isFogged) {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        float baseY = pilar_h / 2.0f;
        float t = water_offset_zenith * 0.4f;
        float cell_phase = (float)(i * 7 + j * 13);
        
        glEnable(GL_LIGHTING);
        GLfloat fog_ambient[] = {0.85f, 0.85f, 0.90f, 0.60f};
        GLfloat fog_diffuse[] = {0.92f, 0.92f, 0.96f, 0.55f};
        GLfloat fog_specular[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glMaterialfv(GL_FRONT, GL_AMBIENT, fog_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, fog_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, fog_specular);
        glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);

        // Tall fog curtain cube with constant top height
        glPushMatrix();
        float swayX = sin(t * 0.5f + cell_phase) * 0.15f;
        float swayZ = cos(t * 0.4f + cell_phase + 2.0f) * 0.15f;
        float fog_top_Y = 22.0f;
        float fog_H = 30.0f;
        float relative_center = (fog_top_Y - fog_H/2.0f) - pilar_center;
        glTranslatef(swayX, relative_center, swayZ);
        glColor4f(0.90, 0.90, 0.95, 0.50);
        glScalef(5.2, fog_H, 5.2);
        glutSolidCube(1.0);
        glPopMatrix();

        glPopAttrib();
      }

      glPopMatrix();

      // Complementos and markers: skip if fogged
      if (!isFogged && isNear) {
        glPushMatrix();
        float surface_offset = 0.2f; // Default
        if (celdaActual == 'B') surface_offset = 0.6f;
        else if (celdaActual == 'H') surface_offset = 0.4f;
        else if (celdaActual == 'K' || celdaActual == 'S' || celdaActual == 'U') surface_offset = 0.25f;

        glTranslatef(cellX, surface_offset + alt_Y, cellZ);
          complementosCelda(celdaActual, alturaActual, j, i, level);
        glPopMatrix();
        
        // Draw Visual Markers for actions/collisions in 3D Zenithal
        const auto& markers = monitor.getVisualMarkers();
        for (const auto& vm : markers) {
            if (vm.f == i && vm.c == j) { // i=row, j=col in drawZenithal
                glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
                glPushMatrix();
                glTranslatef(cellX, alt_Y + 1.5f, cellZ);
                glDisable(GL_LIGHTING);
                
                if (vm.type == 0 || vm.type == 1) { // Cross (0: Red, 1: Orange)
                    if (vm.type == 0) glColor3f(1.0, 0.0, 0.0);
                    else glColor3f(1.0, 0.5, 0.0);
                    
                    glLineWidth(12.0f);
                    glBegin(GL_LINES);
                      glVertex3f(-1.0, -1.0, 0.0); glVertex3f(1.0, 1.0, 0.0);
                      glVertex3f(-1.0, 1.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
                    glEnd();
                }
                
                glPopMatrix();
                glPopAttrib();
                break;
            }
        }
      }
    }
  }

  // Entities
  for (unsigned int i = 0; i < entidades->size(); i++)
  {
     z = (*entidades)[i]->getFil() + 1;
     x = (*entidades)[i]->getCol() + 1;
     glPushMatrix();
     float ent_alt_Y = (mapaAlturas[z - 1][x - 1]) * 1.5f;

     glTranslatef(((GLfloat)filaMed - (GLfloat)x) * 5.0, 1.0 + ent_alt_Y, ((GLfloat)colMed - (GLfloat)z) * 5.0); 
     
     glDisable(GL_TEXTURE_2D); // Ensure solid colors


     if (i == 0) {
       // Rescatador: Big Red Quesito (5x larger -> r=2.0)
       OrientacionEntidadFP((*entidades)[i]->getOrientacion());
       drawQuesito(2.0f, 0.5f, 1.0f, 0.0f, 0.0f);
     } else if (i == 1) {
       // tecnico: Orange Quesito (5x larger -> r=2.0)
       OrientacionEntidadFP((*entidades)[i]->getOrientacion());
       drawQuesito(2.0f, 0.5f, 1.0f, 0.65f, 0.0f); 
     } else {
       // Other entities (NPCs) - Keep as is or generic
       glDisable(GL_LIGHTING);
       glColor3f(0.6, 0.6, 0.6);
       OrientacionEntidadFP((*entidades)[i]->getOrientacion());
       (*entidades)[i]->draw(1);
       glEnable(GL_LIGHTING);
     }

     glEnable(GL_TEXTURE_2D);
     glPopMatrix();
  }

  // --- Draw 3D Plan markers and lines ---
  drawPlans3D((*entidades)[0]->getMapaPlan(), 0, filaMed, colMed, mapaAlturas);
  drawPlans3D((*entidades)[1]->getMapaPlan(), 1, filaMed, colMed, mapaAlturas);
  drawPlans3D((*entidades)[0]->getCanalizacionPlan(), 2, filaMed, colMed, mapaAlturas);

  // --- Draw Goal Markers ---
  if (level > 1) {
    for (unsigned int i = 0; i < objetivosActivos.size(); i += 3) {
      int goalFil = objetivosActivos[i];
      int goalCol = objetivosActivos[i+1];

      glPushMatrix();
      float goalX = ((GLfloat)filaMed - (GLfloat)goalCol - 1) * 5.0;
      float goalZ = ((GLfloat)colMed - (GLfloat)goalFil - 1) * 5.0;
      float goal_alt_Y = (mapaAlturas[goalFil][goalCol]) * 2.0f;

      unsigned char celdaGoalZ = mapaCompleto[goalFil][goalCol];
      float surf_offsetZ = 0.2f;
      if (celdaGoalZ == 'B') surf_offsetZ = 0.6f;
      else if (celdaGoalZ == 'H') surf_offsetZ = 0.4f;
      else if (celdaGoalZ == 'K' || celdaGoalZ == 'S' || celdaGoalZ == 'U') surf_offsetZ = 0.25f;
      else if (celdaGoalZ == 'A') surf_offsetZ = -0.05f;

      glTranslatef(goalX, goal_alt_Y + surf_offsetZ, goalZ);
      drawBelPos3D();
      glPopMatrix();
    }
  }
}

/**
 * @brief Devuelve el índice de la entidad que ocupa una celda determinada.
 *
 * @param f  Fila de la celda.
 * @param c  Columna de la celda.
 * @return Índice de la entidad, o -1 si la celda está desocupada.
 */
int Mapa::QuienEnCasilla(int f, int c)
{
  int out = -1; // Desocupada
  unsigned int i = 0;
  while ((i < entidades->size()) and (out == -1))
  {
    if (((*entidades)[i]->getFil() == f and (*entidades)[i]->getCol() == c) and ((*entidades)[i]->getHitbox()))
    {
      out = i;
    }
    i++;
  }

  return out;
}

/**
 * @brief Comprueba si la casilla que está delante de una entidad está
 *        ocupada por otra entidad.
 *
 * @param entidad  Índice de la entidad cuya casilla frontal se comprueba.
 * @return Índice de la entidad que ocupa la casilla, o -1 si está libre.
 */
int Mapa::casillaOcupada(unsigned int entidad)
{
  unsigned int f = (*entidades)[entidad]->getFil();
  unsigned int c = (*entidades)[entidad]->getCol();

  switch ((*entidades)[entidad]->getOrientacion())
  {
  case norte:
    f--;
    break;
  case noreste:
    f--;
    c++;
    break;
  case este:
    c++;
    break;
  case sureste:
    f++;
    c++;
    break;
  case sur:
    f++;
    break;
  case suroeste:
    f++;
    c--;
    break;
  case oeste:
    c--;
    break;
  case noroeste:
    f--;
    c--;
    break;
  }

  return QuienEnCasilla(f, c);
}

/**
 * @brief Calcula las coordenadas de la celda situada N casillas por
 *        delante de una entidad en su dirección actual.
 *
 * @param entidad   Índice de la entidad.
 * @param casillas  Número de casillas de avance.
 * @return Par (fila, columna) de la celda resultante.
 */
pair<int, int> Mapa::NCasillasDelante(unsigned int entidad, int casillas)
{
  int out = -1;
  unsigned int f = (*entidades)[entidad]->getFil();
  unsigned int c = (*entidades)[entidad]->getCol();

  switch ((*entidades)[entidad]->getOrientacion())
  {
  case norte:
    for (int i = 0; i < casillas; i++)
    {
      f--;
    }
    break;
  case noreste:
    for (int i = 0; i < casillas; i++)
    {
      f--;
      c++;
    }
    break;
  case este:
    for (int i = 0; i < casillas; i++)
    {
      c++;
    }
    break;
  case sureste:
    for (int i = 0; i < casillas; i++)
    {
      f++;
      c++;
    }
    break;
  case sur:
    for (int i = 0; i < casillas; i++)
    {
      f++;
    }
    break;
  case suroeste:
    for (int i = 0; i < casillas; i++)
    {
      f++;
      c--;
    }
    break;
  case oeste:
    for (int i = 0; i < casillas; i++)
    {
      c--;
    }
    break;
  case noroeste:
    for (int i = 0; i < casillas; i++)
    {
      f--;
      c--;
    }
    break;
  }

  pair<int, int> r;
  r.first = f;
  r.second = c;
  return r;
}

/**
 * @brief Comprueba si una entidad puede ejecutar la acción JUMP (correr)
 *        avanzando @p avance_size casillas.
 *
 * Verifica las casillas intermedias y la casilla destino en busca de
 * obstáculos, precipicios u otras entidades.
 *
 * @param entidad      Índice de la entidad.
 * @param avance_size  Número de casillas que comprende el salto.
 * @return 0 si es posible; 1 = muro, 2 = precipicio, 3 = técnico,
 *         4 = excursionista, 5 = vándalo, 6 = árbol.
 */
int Mapa::EsPosibleCorrer(unsigned int entidad, int avance_size)
{
  int out = -4;

  unsigned int *f = new unsigned int[avance_size + 1];
  unsigned int *c = new unsigned int[avance_size + 1];

  f[0] = (*entidades)[entidad]->getFil();
  c[0] = (*entidades)[entidad]->getCol();

  int accesible = 0;

  for (int i = 1; i < avance_size + 1 and accesible == 0; i++)
  {
    switch ((*entidades)[entidad]->getOrientacion())
    {
    case norte:
      f[i] = f[i - 1] - 1;
      c[i] = c[i - 1];
      break;

    case noreste:
      f[i] = f[i - 1] - 1;
      c[i] = c[i - 1] + 1;
      break;

    case este:
      f[i] = f[i - 1];
      c[i] = c[i - 1] + 1;
      break;

    case sureste:
      f[i] = f[i - 1] + 1;
      c[i] = c[i - 1] + 1;
      break;

    case sur:
      f[i] = f[i - 1] + 1;
      c[i] = c[i - 1];
      break;

    case suroeste:
      f[i] = f[i - 1] + 1;
      c[i] = c[i - 1] - 1;
      break;

    case oeste:
      f[i] = f[i - 1];
      c[i] = c[i - 1] - 1;
      break;

    case noroeste:
      f[i] = f[i - 1] - 1;
      c[i] = c[i - 1] - 1;
      break;
    }

    if (mapaCompleto[f[i]][c[i]] == 'M')
      accesible = 1;
    else if (mapaCompleto[f[i]][c[i]] == 'P')
      accesible = 2;
    else if (mapaCompleto[f[i]][c[i]] == 'B')
      accesible = 6;
    else
    {
      unsigned int j = 0;
      while ((j < entidades->size()) and (accesible == 0))
      {
        if ((*entidades)[j]->getFil() == f[i] and (*entidades)[j]->getCol() == c[i] and (*entidades)[j]->getHitbox() and entidad != j)
        {
          if ((*entidades)[j]->getSubTipo() == tecnico)
            accesible = 3;
          else if ((*entidades)[j]->getSubTipo() == vandalo)
            accesible = 5;
          else
            accesible = 4;
        }
        j++;
      }
    }
  }

  return accesible;
}

/**
 * @brief Comprueba si hay entidades en las tres casillas siguientes
 *        en la dirección de lanzamiento (PUSH) de una entidad.
 *
 * Si no encuentra entidad, busca una casilla transitable ('S' o 'H')
 * entre las tres posiciones y devuelve sus coordenadas.
 *
 * @param entidad    Índice de la entidad que lanza.
 * @param[out] fil   Fila de la casilla destino del empujón.
 * @param[out] col   Columna de la casilla destino del empujón.
 * @return Índice de la entidad encontrada, o códigos negativos indicando
 *         la distancia de la casilla transitable más lejana.
 */
int Mapa::casillaOcupadaThrow(unsigned int entidad, int &fil, int &col)
{
  int out = -4;

  unsigned int f1 = (*entidades)[entidad]->getFil();
  unsigned int f2 = (*entidades)[entidad]->getFil();
  unsigned int f3 = (*entidades)[entidad]->getFil();

  unsigned int c1 = (*entidades)[entidad]->getCol();
  unsigned int c2 = (*entidades)[entidad]->getCol();
  unsigned int c3 = (*entidades)[entidad]->getCol();

  switch ((*entidades)[entidad]->getOrientacion())
  {
  case norte:
    f1--;
    f2 -= 2;
    f3 -= 3;
    break;

  case noreste:
    f1--;
    f2 -= 2;
    f3 -= 3;
    c1++;
    c2 += 2;
    c3 += 3;
    break;

  case este:
    c1++;
    c2 += 2;
    c3 += 3;
    break;

  case sureste:
    f1++;
    f2 += 2;
    f3 += 3;
    c1++;
    c2 += 2;
    c3 += 3;
    break;

  case sur:
    f1++;
    f2 += 2;
    f3 += 3;
    break;

  case suroeste:
    f1++;
    f2 += 2;
    f3 += 3;
    c1--;
    c2 -= 2;
    c3 -= 3;
    break;

  case oeste:
    c1--;
    c2 -= 2;
    c3 -= 3;
    break;

  case noroeste:
    f1--;
    f2 -= 2;
    f3 -= 3;
    c1--;
    c2 -= 2;
    c3 -= 3;
    break;
  }

  unsigned int i = 0;
  while ((i < entidades->size()) and (out == -1))
  {
    if ((*entidades)[i]->getFil() == f1 and (*entidades)[i]->getCol() == c1 and (*entidades)[i]->getHitbox() and entidad != i)
    {
      out = i;
    }

    if ((*entidades)[i]->getFil() == f2 and (*entidades)[i]->getCol() == c2 and (*entidades)[i]->getHitbox() and entidad != i)
    {
      out = i;
    }

    if ((*entidades)[i]->getFil() == f3 and (*entidades)[i]->getCol() == c3 and (*entidades)[i]->getHitbox() and entidad != i)
    {
      out = i;
    }
    i++;
  }

  if (out == -1)
  {
    if (mapaCompleto[f3][c3] == 'S' or mapaCompleto[f3][c3] == 'H')
    {
      fil = f3;
      col = c3;
      out = -3;
    }
    else if (mapaCompleto[f2][c2] == 'S' or mapaCompleto[f2][c2] == 'H')
    {
      fil = f2;
      col = c2;
      out = -2;
    }
    else if (mapaCompleto[f1][c1] == 'S' or mapaCompleto[f1][c1] == 'H')
    {
      fil = f1;
      col = c1;
      out = -1;
    }
  }

  return out;
}

/**
 * @brief Gira la cámara del visor a la izquierda.
 *
 * @param grados  Grados de rotación (90 o 45).
 */
void Mapa::girarCamaraIzquierda(int grados)
{
  if (grados == 90)
  {
    angulo -= 90.0;
    if (angulo < 0.0)
    {
      angulo = 270.0;
    }
  }
  else if (grados == 45)
  {
    angulo -= 45.0;
    if (angulo < 0.0)
    {
      angulo = 315.0;
    }
  }
}

/**
 * @brief Gira la cámara del visor a la derecha.
 *
 * @param grados  Grados de rotación (90 o 45).
 */
void Mapa::girarCamaraDerecha(int grados)
{
  if (grados == 90)
  {
    angulo += 90.0;
    if (angulo >= 360.0)
    {
      angulo = 0.0;
    }
  }
  else if (grados == 45)
  {
    angulo += 45.0;
    if (angulo >= 360.0)
    {
      angulo = 360.0 - angulo;
    }
  }
}

/**
 * @brief Devuelve el carácter que identifica a la entidad presente
 *        en una celda.
 *
 * @param f  Fila de la celda.
 * @param c  Columna de la celda.
 * @return 'i' (ingeniero), 't' (técnico), 'e' (excursionista),
 *         'v' (vándalo) o '_' si la celda está libre.
 */
unsigned char Mapa::entidadEnCelda(unsigned int f, unsigned int c)
{
  unsigned char out = '_';
  bool encontrado = false;
  unsigned int aux = 0, i = 0;

  while ((i < entidades->size()) and (!encontrado))
  {
    if ((*entidades)[i]->getFil() == f and (*entidades)[i]->getCol() == c and (*entidades)[i]->getHitbox())
    {
      encontrado = true;
      aux = i;
    }
    i++;
  }

  if (encontrado and (*entidades)[aux]->getHitbox())
  {
    switch ((*entidades)[aux]->getSubTipo())
    {
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
  }

  return out;
}

/**
 * @brief Construye los vectores sensoriales de visión para una entidad.
 *
 * Genera tres capas de 16 elementos cada una (superficie, agentes y
 * cotas) correspondientes a la casilla actual y las 15 casillas
 * del cono de visión frontal (3 filas de profundidad) según la
 * orientación de la entidad.
 *
 * @param Entidad  Índice de la entidad.
 * @return Matriz 3×16: [0] = superficie, [1] = agentes, [2] = cotas.
 */
vector<vector<unsigned char>> Mapa::vision(unsigned int Entidad)
{
  unsigned int fil = (*entidades)[Entidad]->getFil();
  unsigned int col = (*entidades)[Entidad]->getCol();
  Orientacion orient = (*entidades)[Entidad]->getOrientacion();
  bool _jugador = (*entidades)[Entidad]->getTipo() == jugador;

  vector<vector<unsigned char>> fov;
  vector<unsigned char> aux(16);
  fov.push_back(aux);
  fov.push_back(aux);
  fov.push_back(aux);

  fov[0][0] = getCelda(fil, col);       // Terreno
  fov[1][0] = entidadEnCelda(fil, col); // Agentes
  fov[2][0] = alturaEnCelda(fil, col);  // Algura

  int index = 1;

  switch (orient)
  {
  case norte:
    for (int f = 1; f <= 3; f++)
    {
      for (int c = -f; c <= f; c++)
      {
        fov[0][index] = getCelda(fil - f, col + c);
        fov[1][index] = entidadEnCelda(fil - f, col + c);
        fov[2][index] = alturaEnCelda(fil - f, col + c);
        index++;
      }
    }
    break;
  case noreste:
    fov[0][1] = getCelda(fil - 1, col);
    fov[0][2] = getCelda(fil - 1, col + 1);
    fov[0][3] = getCelda(fil, col + 1);

    fov[0][4] = getCelda(fil - 2, col);
    fov[0][5] = getCelda(fil - 2, col + 1);
    fov[0][6] = getCelda(fil - 2, col + 2);
    fov[0][7] = getCelda(fil - 1, col + 2);
    fov[0][8] = getCelda(fil, col + 2);

    fov[0][9] = getCelda(fil - 3, col);
    fov[0][10] = getCelda(fil - 3, col + 1);
    fov[0][11] = getCelda(fil - 3, col + 2);
    fov[0][12] = getCelda(fil - 3, col + 3);
    fov[0][13] = getCelda(fil - 2, col + 3);
    fov[0][14] = getCelda(fil - 1, col + 3);
    fov[0][15] = getCelda(fil, col + 3);

    fov[1][1] = entidadEnCelda(fil - 1, col);
    fov[1][2] = entidadEnCelda(fil - 1, col + 1);
    fov[1][3] = entidadEnCelda(fil, col + 1);

    fov[1][4] = entidadEnCelda(fil - 2, col);
    fov[1][5] = entidadEnCelda(fil - 2, col + 1);
    fov[1][6] = entidadEnCelda(fil - 2, col + 2);
    fov[1][7] = entidadEnCelda(fil - 1, col + 2);
    fov[1][8] = entidadEnCelda(fil, col + 2);

    fov[1][9] = entidadEnCelda(fil - 3, col);
    fov[1][10] = entidadEnCelda(fil - 3, col + 1);
    fov[1][11] = entidadEnCelda(fil - 3, col + 2);
    fov[1][12] = entidadEnCelda(fil - 3, col + 3);
    fov[1][13] = entidadEnCelda(fil - 2, col + 3);
    fov[1][14] = entidadEnCelda(fil - 1, col + 3);
    fov[1][15] = entidadEnCelda(fil, col + 3);

    fov[2][1] = alturaEnCelda(fil - 1, col);
    fov[2][2] = alturaEnCelda(fil - 1, col + 1);
    fov[2][3] = alturaEnCelda(fil, col + 1);

    fov[2][4] = alturaEnCelda(fil - 2, col);
    fov[2][5] = alturaEnCelda(fil - 2, col + 1);
    fov[2][6] = alturaEnCelda(fil - 2, col + 2);
    fov[2][7] = alturaEnCelda(fil - 1, col + 2);
    fov[2][8] = alturaEnCelda(fil, col + 2);

    fov[2][9] = alturaEnCelda(fil - 3, col);
    fov[2][10] = alturaEnCelda(fil - 3, col + 1);
    fov[2][11] = alturaEnCelda(fil - 3, col + 2);
    fov[2][12] = alturaEnCelda(fil - 3, col + 3);
    fov[2][13] = alturaEnCelda(fil - 2, col + 3);
    fov[2][14] = alturaEnCelda(fil - 1, col + 3);
    fov[2][15] = alturaEnCelda(fil, col + 3);
    break;
  case este:
    for (int c = 1; c <= 3; c++)
    {
      for (int f = -c; f <= c; f++)
      {
        fov[0][index] = getCelda(fil + f, col + c);
        fov[1][index] = entidadEnCelda(fil + f, col + c);
        fov[2][index] = alturaEnCelda(fil + f, col + c);
        index++;
      }
    }
    break;
  case sureste:
    fov[0][1] = getCelda(fil, col + 1);
    fov[0][2] = getCelda(fil + 1, col + 1);
    fov[0][3] = getCelda(fil + 1, col);

    fov[0][4] = getCelda(fil, col + 2);
    fov[0][5] = getCelda(fil + 1, col + 2);
    fov[0][6] = getCelda(fil + 2, col + 2);
    fov[0][7] = getCelda(fil + 2, col + 1);
    fov[0][8] = getCelda(fil + 2, col);

    fov[0][9] = getCelda(fil, col + 3);
    fov[0][10] = getCelda(fil + 1, col + 3);
    fov[0][11] = getCelda(fil + 2, col + 3);
    fov[0][12] = getCelda(fil + 3, col + 3);
    fov[0][13] = getCelda(fil + 3, col + 2);
    fov[0][14] = getCelda(fil + 3, col + 1);
    fov[0][15] = getCelda(fil + 3, col);

    fov[1][1] = entidadEnCelda(fil, col + 1);
    fov[1][2] = entidadEnCelda(fil + 1, col + 1);
    fov[1][3] = entidadEnCelda(fil + 1, col);

    fov[1][4] = entidadEnCelda(fil, col + 2);
    fov[1][5] = entidadEnCelda(fil + 1, col + 2);
    fov[1][6] = entidadEnCelda(fil + 2, col + 2);
    fov[1][7] = entidadEnCelda(fil + 2, col + 1);
    fov[1][8] = entidadEnCelda(fil + 2, col);

    fov[1][9] = entidadEnCelda(fil, col + 3);
    fov[1][10] = entidadEnCelda(fil + 1, col + 3);
    fov[1][11] = entidadEnCelda(fil + 2, col + 3);
    fov[1][12] = entidadEnCelda(fil + 3, col + 3);
    fov[1][13] = entidadEnCelda(fil + 3, col + 2);
    fov[1][14] = entidadEnCelda(fil + 3, col + 1);
    fov[1][15] = entidadEnCelda(fil + 3, col);

    fov[2][1] = alturaEnCelda(fil, col + 1);
    fov[2][2] = alturaEnCelda(fil + 1, col + 1);
    fov[2][3] = alturaEnCelda(fil + 1, col);

    fov[2][4] = alturaEnCelda(fil, col + 2);
    fov[2][5] = alturaEnCelda(fil + 1, col + 2);
    fov[2][6] = alturaEnCelda(fil + 2, col + 2);
    fov[2][7] = alturaEnCelda(fil + 2, col + 1);
    fov[2][8] = alturaEnCelda(fil + 2, col);

    fov[2][9] = alturaEnCelda(fil, col + 3);
    fov[2][10] = alturaEnCelda(fil + 1, col + 3);
    fov[2][11] = alturaEnCelda(fil + 2, col + 3);
    fov[2][12] = alturaEnCelda(fil + 3, col + 3);
    fov[2][13] = alturaEnCelda(fil + 3, col + 2);
    fov[2][14] = alturaEnCelda(fil + 3, col + 1);
    fov[2][15] = alturaEnCelda(fil + 3, col);

    break;
  case sur:
    for (int f = 1; f <= 3; f++)
    {
      for (int c = -f; c <= f; c++)
      {
        fov[0][index] = getCelda(fil + f, col - c);
        fov[1][index] = entidadEnCelda(fil + f, col - c);
        fov[2][index] = alturaEnCelda(fil + f, col - c);
        index++;
      }
    }
    break;
  case suroeste:
    fov[0][1] = getCelda(fil + 1, col);
    fov[0][2] = getCelda(fil + 1, col - 1);
    fov[0][3] = getCelda(fil, col - 1);

    fov[0][4] = getCelda(fil + 2, col);
    fov[0][5] = getCelda(fil + 2, col - 1);
    fov[0][6] = getCelda(fil + 2, col - 2);
    fov[0][7] = getCelda(fil + 1, col - 2);
    fov[0][8] = getCelda(fil, col - 2);

    fov[0][9] = getCelda(fil + 3, col);
    fov[0][10] = getCelda(fil + 3, col - 1);
    fov[0][11] = getCelda(fil + 3, col - 2);
    fov[0][12] = getCelda(fil + 3, col - 3);
    fov[0][13] = getCelda(fil + 2, col - 3);
    fov[0][14] = getCelda(fil + 1, col - 3);
    fov[0][15] = getCelda(fil, col - 3);

    fov[1][1] = entidadEnCelda(fil + 1, col);
    fov[1][2] = entidadEnCelda(fil + 1, col - 1);
    fov[1][3] = entidadEnCelda(fil, col - 1);

    fov[1][4] = entidadEnCelda(fil + 2, col);
    fov[1][5] = entidadEnCelda(fil + 2, col - 1);
    fov[1][6] = entidadEnCelda(fil + 2, col - 2);
    fov[1][7] = entidadEnCelda(fil + 1, col - 2);
    fov[1][8] = entidadEnCelda(fil, col - 2);

    fov[1][9] = entidadEnCelda(fil + 3, col);
    fov[1][10] = entidadEnCelda(fil + 3, col - 1);
    fov[1][11] = entidadEnCelda(fil + 3, col - 2);
    fov[1][12] = entidadEnCelda(fil + 3, col - 3);
    fov[1][13] = entidadEnCelda(fil + 2, col - 3);
    fov[1][14] = entidadEnCelda(fil + 1, col - 3);
    fov[1][15] = entidadEnCelda(fil, col - 3);

    fov[2][1] = alturaEnCelda(fil + 1, col);
    fov[2][2] = alturaEnCelda(fil + 1, col - 1);
    fov[2][3] = alturaEnCelda(fil, col - 1);

    fov[2][4] = alturaEnCelda(fil + 2, col);
    fov[2][5] = alturaEnCelda(fil + 2, col - 1);
    fov[2][6] = alturaEnCelda(fil + 2, col - 2);
    fov[2][7] = alturaEnCelda(fil + 1, col - 2);
    fov[2][8] = alturaEnCelda(fil, col - 2);

    fov[2][9] = alturaEnCelda(fil + 3, col);
    fov[2][10] = alturaEnCelda(fil + 3, col - 1);
    fov[2][11] = alturaEnCelda(fil + 3, col - 2);
    fov[2][12] = alturaEnCelda(fil + 3, col - 3);
    fov[2][13] = alturaEnCelda(fil + 2, col - 3);
    fov[2][14] = alturaEnCelda(fil + 1, col - 3);
    fov[2][15] = alturaEnCelda(fil, col - 3);
    break;
  case oeste:
    for (int c = 1; c <= 3; c++)
    {
      for (int f = -c; f <= c; f++)
      {
        fov[0][index] = getCelda(fil - f, col - c);
        fov[1][index] = entidadEnCelda(fil - f, col - c);
        fov[2][index] = alturaEnCelda(fil - f, col - c);
        index++;
      }
    }
    break;
  case noroeste:
    fov[0][1] = getCelda(fil, col - 1);
    fov[0][2] = getCelda(fil - 1, col - 1);
    fov[0][3] = getCelda(fil - 1, col);

    fov[0][4] = getCelda(fil, col - 2);
    fov[0][5] = getCelda(fil - 1, col - 2);
    fov[0][6] = getCelda(fil - 2, col - 2);
    fov[0][7] = getCelda(fil - 2, col - 1);
    fov[0][8] = getCelda(fil - 2, col);

    fov[0][9] = getCelda(fil, col - 3);
    fov[0][10] = getCelda(fil - 1, col - 3);
    fov[0][11] = getCelda(fil - 2, col - 3);
    fov[0][12] = getCelda(fil - 3, col - 3);
    fov[0][13] = getCelda(fil - 3, col - 2);
    fov[0][14] = getCelda(fil - 3, col - 1);
    fov[0][15] = getCelda(fil - 3, col);

    fov[1][1] = entidadEnCelda(fil, col - 1);
    fov[1][2] = entidadEnCelda(fil - 1, col - 1);
    fov[1][3] = entidadEnCelda(fil - 1, col);

    fov[1][4] = entidadEnCelda(fil, col - 2);
    fov[1][5] = entidadEnCelda(fil - 1, col - 2);
    fov[1][6] = entidadEnCelda(fil - 2, col - 2);
    fov[1][7] = entidadEnCelda(fil - 2, col - 1);
    fov[1][8] = entidadEnCelda(fil - 2, col);

    fov[1][9] = entidadEnCelda(fil, col - 3);
    fov[1][10] = entidadEnCelda(fil - 1, col - 3);
    fov[1][11] = entidadEnCelda(fil - 2, col - 3);
    fov[1][12] = entidadEnCelda(fil - 3, col - 3);
    fov[1][13] = entidadEnCelda(fil - 3, col - 2);
    fov[1][14] = entidadEnCelda(fil - 3, col - 1);
    fov[1][15] = entidadEnCelda(fil - 3, col);

    fov[2][1] = alturaEnCelda(fil, col - 1);
    fov[2][2] = alturaEnCelda(fil - 1, col - 1);
    fov[2][3] = alturaEnCelda(fil - 1, col);

    fov[2][4] = alturaEnCelda(fil, col - 2);
    fov[2][5] = alturaEnCelda(fil - 1, col - 2);
    fov[2][6] = alturaEnCelda(fil - 2, col - 2);
    fov[2][7] = alturaEnCelda(fil - 2, col - 1);
    fov[2][8] = alturaEnCelda(fil - 2, col);

    fov[2][9] = alturaEnCelda(fil, col - 3);
    fov[2][10] = alturaEnCelda(fil - 1, col - 3);
    fov[2][11] = alturaEnCelda(fil - 2, col - 3);
    fov[2][12] = alturaEnCelda(fil - 3, col - 3);
    fov[2][13] = alturaEnCelda(fil - 3, col - 2);
    fov[2][14] = alturaEnCelda(fil - 3, col - 1);
    fov[2][15] = alturaEnCelda(fil - 3, col);
    break;
  }

  return fov;
}

/**
 * @brief Indica si un carácter de celda corresponde a un elemento de
 *        tubería (estático o terminal).
 *
 * @param c  Carácter de la celda.
 * @return true si es un tipo de tubería ('U', 'h', 'v', 'I', 'J',
 *         '1'–'6').
 */
bool Mapa::esTuberia(unsigned char c) {
    return (c == 'U' || c == 'h' || c == 'v' || c == 'I' || c == 'J' || 
            c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6');
}
