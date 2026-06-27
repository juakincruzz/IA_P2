# Práctica 2 - Inteligencia Artificial

Resolución de un problema práctico con **agentes reactivos y deliberativos** para la asignatura **Inteligencia Artificial**.

El proyecto implementa dos agentes autónomos, **Ingeniero** y **Técnico**, capaces de actuar en un entorno dinámico utilizando sensores, memoria interna, planificación de rutas y mecanismos de cooperación.

La práctica obtuvo una calificación final de **9,55/10**.

---

## Descripción

El objetivo principal de la práctica es diseñar agentes capaces de tomar decisiones en distintos niveles de complejidad.

A lo largo de los niveles, los agentes pasan de comportamientos puramente reactivos a estrategias deliberativas más avanzadas, incluyendo:

* Exploración del entorno.
* Construcción de mapas internos.
* Evitación de obstáculos.
* Gestión de colisiones entre agentes.
* Planificación de rutas.
* Optimización de coste energético.
* Instalación cooperativa de tuberías.
* Planificación con restricciones ecológicas.
* Actuación con mapa completo y mapa desconocido.

---

## Agentes implementados

### Ingeniero

El Ingeniero es el agente principal de planificación en varios niveles.

Entre sus responsabilidades destacan:

* Explorar el mapa.
* Planificar rutas hacia objetivos.
* Gestionar acciones con coste de batería.
* Calcular trazados de tuberías.
* Aplicar operaciones de terraformación.
* Coordinarse con el Técnico para instalar tramos.
* Replanificar ante bloqueos o presencia de otros agentes.

### Técnico

El Técnico actúa como agente complementario y cooperativo.

Sus responsabilidades principales son:

* Explorar de forma reactiva.
* Seguir o asistir al Ingeniero según el nivel.
* Evitar colisiones y desbloquear caminos.
* Navegar hacia posiciones asignadas.
* Instalar tuberías cuando recibe la señal adecuada.
* Apartarse tras instalar para permitir el avance del Ingeniero.

---

## Comportamiento por niveles

| Nivel | Descripción                                                                        |
| ----: | ---------------------------------------------------------------------------------- |
|     0 | Comportamiento reactivo basado en sensores, feromonas y evitación de colisiones.   |
|     1 | Exploración con mapa interno y matriz de visitas.                                  |
|     2 | Planificación deliberativa mediante búsqueda en anchura y seguimiento cooperativo. |
|     3 | Planificación con coste de batería mediante A*.                                    |
|     4 | Planificación de red de tuberías con restricciones ecológicas y gravedad.          |
|     5 | Instalación cooperativa de tuberías con mapa completo.                             |
|     6 | Cooperación avanzada con mapa desconocido, exploración, órdenes y replanificación. |

---

## Técnicas utilizadas

### Comportamiento reactivo

En los primeros niveles, los agentes toman decisiones a partir de la información inmediata de sus sensores.

Se utilizan criterios como:

* Tipo de terreno.
* Diferencia de altura.
* Presencia de obstáculos.
* Presencia del otro agente.
* Casillas ya visitadas.
* Preferencia por caminos seguros.
* Mecanismos de desbloqueo.

### Mapa de feromonas

Se utiliza una matriz de visitas para penalizar las casillas ya exploradas y favorecer rutas menos repetidas.

Esto permite reducir ciclos y mejorar la exploración del entorno.

### Búsqueda en anchura

El Ingeniero utiliza búsqueda en anchura para encontrar rutas en el mapa conocido.

El estado incluye:

* Fila.
* Columna.
* Orientación.
* Estado de zapatillas.

La orientación forma parte del estado porque girar también consume acciones.

### A*

Para niveles con coste energético se emplea una planificación basada en A*.

La función de coste considera:

* Tipo de terreno.
* Diferencia de altura.
* Acciones de movimiento.
* Acciones de giro.
* Uso de zapatillas.
* Restricciones de transitabilidad.

La heurística utilizada se basa en la distancia al objetivo.

### Dijkstra para tuberías

En los niveles de tuberías, el Ingeniero calcula rutas de bajo impacto ecológico usando Dijkstra.

El problema considera:

* Coste de instalación.
* Coste de terraformación.
* Restricción de gravedad.
* Presupuesto ecológico máximo.
* Altura de la tubería.
* Operaciones `RAISE` y `DIG`.

### Cooperación entre agentes

En los niveles avanzados, Ingeniero y Técnico se coordinan para instalar tuberías.

El Ingeniero planifica y se posiciona, mientras que el Técnico se desplaza al extremo correspondiente del tramo y ejecuta la instalación cuando recibe la señal.

También se incorporan mecanismos para:

* Evitar bloqueos.
* Detectar instalaciones ya realizadas.
* Reintentar tras fallos.
* Apartarse después de instalar.
* Replanificar si el otro agente impide el movimiento.

---

## Organización del código

Los archivos principales de la implementación son:

```text
ingeniero.cpp
ingeniero.hpp
tecnico.cpp
tecnico.hpp
```

### `ingeniero.cpp`

Contiene la lógica del agente Ingeniero, incluyendo:

* Selección de comportamiento según nivel.
* Exploración reactiva.
* Búsqueda en anchura.
* Planificación deliberativa.
* Planificación de tuberías.
* Dijkstra con impacto ecológico.
* Coordinación con el Técnico.
* Gestión de terraformación e instalación.

### `tecnico.cpp`

Contiene la lógica del agente Técnico, incluyendo:

* Selección de comportamiento según nivel.
* Exploración con feromonas.
* Mecanismos anti-bloqueo.
* Seguimiento del Ingeniero.
* Planificación con A*.
* Navegación hacia tramos de tubería.
* Instalación cooperativa.
* Retirada tras instalación.

---

## Compilación

Para preparar y compilar el proyecto por primera vez:

```bash
./install.sh
```

Para recompilar después de realizar cambios:

```bash
make clean
make -j$(nproc)
```

---

## Ejecución

Clonar el repositorio:

```bash
git clone git@github.com:juakincruzz/IA_P2.git
cd IA_P2
```

Ejecutar la práctica con interfaz gráfica:

```bash
./practica2
```

Ejecutar la práctica sin interfaz gráfica:

```bash
./practica2SG
```

---

## Resultados

La práctica obtuvo una calificación final de:

```text
9,55/10
```

El resultado refleja el correcto funcionamiento de los agentes, la implementación de los distintos niveles de comportamiento y la coordinación entre Ingeniero y Técnico en los escenarios avanzados.

---

## Principales aprendizajes

Durante el desarrollo de esta práctica se trabajaron conceptos clave de Inteligencia Artificial aplicada a agentes:

* Diferencia entre agentes reactivos y deliberativos.
* Representación de estados.
* Planificación de rutas.
* Uso de sensores para construir conocimiento del entorno.
* Gestión de incertidumbre en mapas incompletos.
* Cooperación entre agentes.
* Optimización bajo restricciones.
* Costes asociados a acciones y terrenos.
* Replanificación ante bloqueos.

---

## Conclusión

Esta práctica muestra la evolución desde comportamientos reactivos simples hasta agentes cooperativos capaces de planificar, adaptarse al entorno y resolver tareas complejas.

La combinación de búsqueda, planificación, memoria interna, control de colisiones y cooperación entre agentes permitió resolver los distintos niveles de la práctica con una calificación final de **9,55/10**.

---

## Autor

**Joaquín Cruz Lorenzo**
GitHub: [@juakincruzz](https://github.com/juakincruzz)
