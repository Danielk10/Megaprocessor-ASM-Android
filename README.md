# Megaprocessor ASM - Android NDK

**Ensamblador de código abierto para el Megaprocessor - Versión Android Native (NDK/C++)**

[![Licencia](https://img.shields.io/badge/Licencia-Apache%202.0-blue.svg)](LICENSE)
[![Lenguaje](https://img.shields.io/badge/Lenguaje-C%2B%2B-00599C.svg)](https://isocpp.org/)
[![Plataforma](https://img.shields.io/badge/Plataforma-Android-3DDC84.svg)](https://developer.android.com/ndk)
[![Build System](https://img.shields.io/badge/Build-CMake%20%2B%20Gradle-064F8C.svg)](https://cmake.org/)
[![API Level](https://img.shields.io/badge/API-23%2B-brightgreen.svg)](https://developer.android.com/studio/releases/platforms)

## 📋 Descripción

Este proyecto es una **aplicación Android nativa** que implementa un **ensamblador completo** para el [Megaprocessor](http://www.megaprocessor.com/), un procesador de 16 bits construido con componentes discretos creado por James Newman. El Megaprocessor es un procesador físico gigante donde cada transistor es visible, diseñado con fines educativos para mostrar cómo funciona un CPU por dentro.

Esta aplicación utiliza **Android NDK** para ejecutar el ensamblador en código nativo C++, proporcionando máxima performance y permitiendo reutilizar el core del ensamblador en otras plataformas.

## ✨ Características

### Características del Ensamblador
- ✅ **Análisis léxico completo**: Tokenización de código assembly
- ✅ **Parser sintáctico**: Validación de sintaxis y estructura
- ✅ **Generador de código**: Traducción a bytecode del Megaprocessor
- ✅ **Manejo de etiquetas**: Soporte para saltos y referencias
- ✅ **Detección de errores**: Mensajes claros de errores de sintaxis
- ✅ **Generación de archivos**: Produce archivos .hex y .lst

### Características de la App Android
- 📱 **Interfaz nativa Android**: UI moderna con Material Design
- ⚡ **Performance nativa**: Código C++ compilado con NDK para máxima velocidad
- 💾 **Gestión de archivos**: Lectura/escritura de archivos .asm en almacenamiento
- 📝 **Editor integrado**: Permite editar código assembly directamente
- 🔍 **Visualización de resultados**: Muestra archivos .hex y .lst generados
- 🎯 **API 23+**: Compatible con Android 6.0 Marshmallow en adelante
- 🌗 **Modo oscuro automático**: Soporte Day/Night según el tema del sistema
- 🔗 **Integración web**: Acceso rápido a documentación y sitio del Megaprocessor
- 📤 **Compartir proyecto**: Permite compartir ASM/HEX/LST desde la app

## 🏗️ Arquitectura del Megaprocessor

El Megaprocessor es un procesador de 16 bits con:
- **Arquitectura**: Procesador de 16-bit Load/Store con bus de datos externo de 8-bit
- **Ancho de palabra**: 16 bits
- **Registros**: 4 registros de propósito general (R0, R1, R2, R3) + SP, PS, PC
- **Memoria**: Espacio de direccionamiento de 64KB (0x0000 - 0xFFFF)
- **Set de instrucciones**: Opcodes de 1 byte con operandos variables (ALU, Branches, Memoria, Stack)

Para más información sobre el Megaprocessor, visita: http://www.megaprocessor.com/

## 🚀 Compilación y Ejecución

### Requisitos

- **Android Studio**: Arctic Fox (2020.3.1) o superior
- **Android SDK**: API Level 23 (Android 6.0) mínimo, API Level 36 target
- **Android NDK**: r21 o superior (automático con Android Studio)
- **CMake**: 3.22.1 o superior (incluido con Android Studio)
- **Java**: JDK 11 o superior
- **Gradle**: 8.0+ (incluido con el proyecto)

### Clonar el Repositorio

```bash
git clone https://github.com/Danielk10/Megaprocessor-ASM-Android.git
cd Megaprocessor-ASM-Android
```

### Compilar con Android Studio

1. Abre Android Studio
2. Selecciona `File > Open` y abre la carpeta del proyecto
3. Espera a que Gradle sincronice las dependencias
4. Conecta un dispositivo Android o inicia un emulador
5. Haz clic en `Run` (▶️) para compilar e instalar

### Compilar desde Línea de Comandos

```bash
# Preparar SDK/NDK automáticamente en Linux
./scripts/setup-android-sdk.sh

# En Linux/macOS
./gradlew assembleDebug

# En Windows
gradlew.bat assembleDebug

# Instalar en dispositivo conectado
./gradlew installDebug

# Ejecutar tests
./gradlew test

# Verificar equivalencia HEX (CLI Linux vs archivo de referencia)
./scripts/verify_hex_equivalence.sh

# Verificar HEX + compilar APK; si PASS copia app-debug.apk como app-debug.jpg en raíz
./scripts/verify_hex_and_prepare_apk.sh
```

> Si no tienes Android SDK/NDK instalado en tu entorno Linux, usa `./scripts/setup-android-sdk.sh` para descargar `cmdline-tools`, aceptar licencias e instalar los paquetes requeridos (`platform-tools`, `platforms;android-36`, `build-tools;36.0.0`, `cmake;3.22.1` y `ndk;28.2.13676358`).

### Verificación de Equivalencia HEX (CI local)

El script `scripts/verify_hex_equivalence.sh` ensambla `tic_tac_toe_2.asm` usando un CLI Linux (compilado en tiempo de ejecución desde el core C++), normaliza los HEX y compara el resultado generado contra `app/src/main/assets/tic_tac_toe_2.hex` (que debe ser idéntico al `tic_tac_toe_2.hex` de referencia en raíz).

Comportamiento:
- Devuelve `PASS` cuando los HEX son idénticos.
- Devuelve `FAIL` cuando hay diferencias y muestra:
  - primera línea distinta,
  - dirección del registro Intel HEX donde comienza la divergencia,
  - salida `diff -u` normalizada para diagnóstico.

Estado actual del repositorio: la verificación `./scripts/verify_hex_equivalence.sh` ya produce `PASS` para `tic_tac_toe_2.asm` vs `tic_tac_toe_2.hex` con el ensamblador C++ (Linux/NDK).

También está integrado en **GitHub Actions** (`.github/workflows/verify-hex-equivalence.yml`), lo que asegura que cualquier cambio en el core C++ no rompa la compatibilidad con el binario oficial.

Si quieres publicar el `app-debug.apk` como un **Release** de GitHub:
1. Ve a **Actions > Build and release debug APK**.
2. Pulsa **Run workflow**.
3. Ingresa un `tag_name` (por ejemplo `debug-v1`).
4. (Opcional) Cambia `release_name`.

El workflow compila con `assembleDebug`, sube `app-debug.apk` como artifact de la corrida y también lo publica como asset del release.

El APK generado estará en: `app/build/outputs/apk/debug/app-debug.apk`


## 🧪 Verificación Linux

Para validar el ensamblador en modo **offline** (sin Android SDK/NDK), se incluye un target CLI en `tools/assembler-cli/` que reutiliza `assembler.cpp` y `utils.cpp` sin enlazar con `android` ni `log`.

```bash
cmake -S tools/assembler-cli -B build/assembler-cli
cmake --build build/assembler-cli

# Genera .hex junto al .asm
./build/assembler-cli/assembler-cli ./tic_tac_toe_2.asm

# Genera .hex y .lst
./build/assembler-cli/assembler-cli ./tic_tac_toe_2.asm --lst
```

Opciones útiles:
- `--out <archivo.hex>`: ruta de salida para el `.hex`.
- `--lst-out <archivo.lst>`: ruta de salida para el `.lst` (activa listado).

El ejecutable carga automáticamente `Megaprocessor_defs.asm` si está disponible y también resuelve includes declarados en el `.asm` principal.

## 📖 Uso de la Aplicación

1. **Abrir archivo .asm**: Usa el selector de archivos para cargar un archivo assembly
2. **Editar código**: Modifica el código directamente en la app si es necesario
3. **Ensamblar**: Presiona el botón "Ensamblar" para procesar el código
4. **Ver resultados**: 
   - Archivo `.hex` - Código máquina en formato hexadecimal
   - Archivo `.lst` - Listado con direcciones y código fuente
5. **Guardar**: Presiona **Exportar** para guardar los archivos `.hex` y `.lst` en la carpeta **Descargas** del teléfono

Notas sobre exportación en Android:
- En Android 10+ (API 29+) se usa `MediaStore` para guardar en **Descargas** sin permisos de almacenamiento legados.
- En Android 6–9 (API 23–28), la app solicita permiso de escritura al exportar por primera vez.
- Los archivos se guardan con nombres tipo `<nombre_pestaña>.hex` y `<nombre_pestaña>.lst` para evitar sobreescrituras.

### Ejemplo de Código Assembly

La aplicación incluye archivos de ejemplo como `tic_tac_toe_2.asm`:

```asm
; Ejemplo funcional para Megaprocessor
include "Megaprocessor_defs.asm"

        org 0x0000
start:
        ld.w  r0, #10      ; Cargar 10 en R0
        ld.w  r1, #20      ; Cargar 20 en R1
        add   r2, r0, r1   ; r2 = r0 + r1
        st.w  result, r2   ; Guardar r2 en 'result'
        jmp   loop         ; Bucle infinito

loop:
        jmp   loop

        org 0x1000
result:
        dw    0x0000       ; Espacio para el resultado
```

## 📂 Estructura del Proyecto

```
Megaprocessor-ASM-Android/
├── app/
│   ├── src/main/
│   │   ├── cpp/                 # Core del Ensamblador (C++)
│   │   │   ├── assembler.cpp/.h # Lógica de 2 pasadas
│   │   │   ├── native-lib.cpp   # JNI Bridge
│   │   │   └── utils.cpp/.h     # Helpers
│   │   ├── java/.../            # NativeAssembler.java y MainActivity.java
│   │   ├── assets/              # Megaprocessor_defs.asm, tic_tac_toe_2.asm/hex
│   │   └── res/                 # Layouts UI Android
│   └── build.gradle             # Configuración NDK/Gradle
├── tools/
│   └── assembler-cli/           # Ejecutable CLI puro C++
├── scripts/                     # Automatización (CI, SDK Setup, Verificación)
├── verification/                # Casos de test y resultados (suite de verificación)
├── docs/                        # Documentación adicional
├── .github/workflows/           # CI/CD (GitHub Actions)
├── tic_tac_toe_2.asm/.hex/.lst # Programas de referencia
├── life.asm, snail.asm, ...    # Otros ejemplos reales
├── Megaprocessor_defs.asm      # Definiciones estándar de hardware
├── gradlew.bat / gradlew        # Wrappers de compilación
└── README.md                    # Este archivo
```

## 🛠️ Arquitectura Técnica

### Stack Tecnológico

**Frontend Android:**
- **Lenguaje**: Java (puede migrar a Kotlin)
- **UI**: XML Layouts + ViewBinding
- **Material Design**: AndroidX Material Components
- **API Level**: minSdk 23, targetSdk 36

**Backend Nativo (NDK):**
- **Lenguaje**: C++ (estándar C++11)
- **Build System**: CMake 3.22.1
- **JNI**: Bridge para comunicación Java ↔ C++
- **Compilador**: Clang (incluido con NDK)

### Flujo de Datos

```
┌─────────────────────┐
│   MainActivity      │ (Java)
│   - UI Logic        │
│   - File Handling   │
└──────────┬──────────┘
           │ JNI Call
           ▼
┌─────────────────────┐
│   native-lib.cpp    │ (C++ JNI Bridge)
│   - JNI Methods     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   assembler.cpp     │ (C++ Core)
│   - Lexer           │
│   - Parser          │
│   - Code Generator  │
└─────────────────────┘
```

### Compilación NDK con CMake

El archivo `CMakeLists.txt` configura:
- Estándar C++11
- Flags de optimización
- Linking con bibliotecas Android (log, jnigraphics)
- Targets para múltiples arquitecturas (armeabi-v7a, arm64-v8a, x86, x86_64)

## 🎯 Características Futuras (Roadmap)

- [x] Estructura básica de la aplicación Android
- [x] Integración NDK con CMake
- [x] JNI Bridge funcional
- [x] Ensamblador base funcionando
- [x] Lectura/escritura de archivos
- [x] Generación de archivos .hex y .lst
- [x] Editor de código con syntax highlighting
- [x] Visualizador hexadecimal mejorado
- [ ] Simulador del Megaprocessor integrado
- [ ] Debugger paso a paso
- [ ] Breakpoints en código assembly
- [ ] Visualización de registros y memoria
- [x] Modo oscuro
- [x] Compartir proyectos
- [x] Ejemplos de código incluidos
- [x] Documentación interactiva del set de instrucciones
- [x] Integración con simulador web del Megaprocessor


> Implementado en esta versión: resaltado básico de sintaxis ASM, visualizador HEX con numeración de líneas, botones de compartir proyecto, acceso a documentación y enlace web del Megaprocessor.

## 🔧 Desarrollo

### Configurar Entorno de Desarrollo

1. **Instalar Android Studio**: Descarga desde [developer.android.com](https://developer.android.com/studio)
2. **Instalar NDK y CMake**: 
   - Ve a `Tools > SDK Manager > SDK Tools`
   - Marca `NDK (Side by side)` y `CMake`
   - Haz clic en `Apply`
3. **Configurar variables de entorno** (opcional para CLI):
   ```bash
   export ANDROID_HOME=$HOME/Android/Sdk
   export PATH=$PATH:$ANDROID_HOME/platform-tools
   ```

### Modificar Código Nativo

Después de modificar archivos `.cpp` o `.h`:

1. Gradle detectará automáticamente los cambios
2. CMake recompilará las bibliotecas nativas
3. Haz `Build > Rebuild Project` para asegurar consistencia

### Debugging Nativo

Para debugear código C++:

1. Coloca breakpoints en archivos `.cpp`
2. Usa `Run > Debug 'app'` con debugger nativo
3. Android Studio usará LLDB para debugging nativo
4. Puedes inspeccionar variables C++ directamente

### Testing

```bash
# Tests instrumentados (en dispositivo)
./gradlew connectedAndroidTest

# Tests unitarios
./gradlew test

# Lint checks
./gradlew lint
```

## 📊 Arquitecturas Soportadas

La aplicación compila bibliotecas nativas para:

- ✅ **armeabi-v7a**: ARM 32-bit (Android phones antiguos)
- ✅ **arm64-v8a**: ARM 64-bit (Android phones modernos) 
- ✅ **x86**: Intel 32-bit (Emuladores)
- ✅ **x86_64**: Intel 64-bit (Emuladores modernos)

Esto garantiza compatibilidad con prácticamente cualquier dispositivo Android.

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Si deseas contribuir:

1. Fork el proyecto
2. Crea una rama para tu característica (`git checkout -b feature/nueva-caracteristica`)
3. Commit tus cambios (`git commit -am 'Añadir nueva característica'`)
4. Push a la rama (`git push origin feature/nueva-caracteristica`)
5. Abre un Pull Request

### Guías de Contribución

- **Código Java**: Sigue las convenciones de Android
- **Código C++**: Usa estándar C++11, formato consistente
- **Commits**: Mensajes descriptivos en español o inglés
- **Testing**: Añade tests para nueva funcionalidad
- **Documentación**: Actualiza README si cambias features

## 📄 Licencia

Este proyecto está licenciado bajo la **Apache License 2.0**. Consulta el archivo [LICENSE](LICENSE) para más detalles.

## 👤 Autor

**Daniel Elias Diamon Vazquez**
- GitHub: [@Danielk10](https://github.com/Danielk10)
- Email: danielpdiamon@gmail.com
- Website: [todoandroid.42web.io](https://todoandroid.42web.io/)
- Ubicación: Venezuela
- Especialidades: Desarrollo de juegos 2D (libGDX), Android nativo, Microcontroladores PIC

## 🙏 Agradecimientos

- **James Newman** - Creador del Megaprocessor físico
- Comunidad de desarrolladores de Android NDK
- Proyecto [Megaprocessor-ASM-C](https://github.com/Danielk10/Megaprocessor-ASM-C) - Versión C del ensamblador
- Comunidad de desarrolladores de ensambladores y compiladores
- Android Open Source Project (AOSP)
- Comunidad de software libre y código abierto

## 📚 Recursos Adicionales

### Sobre el Megaprocessor
- [Megaprocessor Official Website](http://www.megaprocessor.com/)
- [Documentación del Set de Instrucciones](http://www.megaprocessor.com/instruction.html)
- [Megaprocessor en YouTube](https://www.youtube.com/watch?v=lNuPy-r1GuQ)

### Android NDK Development
- [Android NDK Documentation](https://developer.android.com/ndk)
- [CMake for Android](https://developer.android.com/ndk/guides/cmake)
- [JNI Guide](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/)
- [Android Studio User Guide](https://developer.android.com/studio/intro)

### Herramientas
- [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)
- [Gradle Build Tool](https://gradle.org/)
- [CMake Documentation](https://cmake.org/documentation/)

## 🐛 Reporte de Bugs

Si encuentras algún bug, por favor abre un [issue](https://github.com/Danielk10/Megaprocessor-ASM-Android/issues) con:
- Descripción del problema
- Pasos para reproducir
- Comportamiento esperado vs. observado
- Versión de Android del dispositivo
- Logs de Android Studio (Logcat)
- Capturas de pantalla si es relevante

## 🔗 Proyectos Relacionados

- [Megaprocessor-ASM-C](https://github.com/Danielk10/Megaprocessor-ASM-C) - Versión en C puro del ensamblador
- [Megaprocessor Official](http://www.megaprocessor.com/) - Procesador físico original

---

**¡Hecho con ❤️ para la comunidad del Megaprocessor y Android!**

**Desarrolla, ensambla y ejecuta código para el Megaprocessor desde tu teléfono Android 📱⚡**
