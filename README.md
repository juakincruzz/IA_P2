# Práctica 2: Resolución de un problema práctico con agentes reactivos/deliberativos

## Prerrequisitos

### 1. Configurar el entorno de trabajo
Asegúrate de tener instalado y configurado todo lo necesario para compilar y ejecutar la práctica:

- Compilador de C++ compatible con el proyecto.
- `cmake` y `make`.
- Dependencias del sistema requeridas por el repositorio.

---

### 2. Preparar tu repositorio local
Clona tu repositorio y sitúate en la carpeta de trabajo:

```bash
git clone git@github.com:juakincruzz/IA.git
cd IA
```

Si trabajas con HTTPS, usa la URL correspondiente en lugar de SSH.

---

### 3. Compilar el proyecto
Puedes compilar usando el script de instalación o compilación manual:

```bash
./install.sh
```

Si necesitas recompilar tras cambios:

```bash
make clean
make -j$(nproc)
```

---

### 4. Ejecutar la práctica
Según la configuración disponible en el repositorio, puedes ejecutar:

```bash
./practica2
```

o sin interfaz gráfica:

```bash
./practica2SG
```

---

### 5. Guardar y subir cambios
Cada vez que avances en la práctica, guarda tus cambios con Git:

```bash
git add .
git commit -m "Actualizando práctica 2"
git push origin main
```

Este flujo (`add`, `commit`, `push`) debe repetirse durante el desarrollo.

---

### 6. (Opcional) Mantener sincronización con el repositorio base
Si trabajas con un repositorio enlazado a otro de referencia, puedes añadir `upstream`:

```bash
git remote add upstream git@github.com:ugr-ccia-IA/practica2.git
```

Y actualizar cuando haya cambios:

```bash
git pull upstream main
git push origin main
```

---

## Realización de la práctica

Esta práctica consiste en la **resolución de un problema práctico con agentes reactivos/deliberativos**, aplicando técnicas de Inteligencia Artificial para el diseño, implementación y evaluación del comportamiento del agente en el entorno propuesto.

Durante el desarrollo se ha trabajado en:

- Modelado del problema y del entorno.
- Implementación de comportamiento **reactivo**.
- Implementación de comportamiento **deliberativo**.
- Comparación de enfoques y resultados.
- Validación del rendimiento en distintos escenarios.

---

## Resultados

- **Calificación final:** **9,55/10**

> La nota refleja un desempeño sólido en el diseño del agente, la implementación técnica y el cumplimiento de los objetivos de la práctica.

---

## Más información

Para detalles adicionales (preguntas frecuentes, decisiones de implementación o notas técnicas), consulta la documentación complementaria del repositorio (por ejemplo, `FAQ.md` si está disponible).

---

## Autor

**Joaquín Cruz Lorenzo**  
GitHub: [@juakincruzz](https://github.com/juakincruzz)
