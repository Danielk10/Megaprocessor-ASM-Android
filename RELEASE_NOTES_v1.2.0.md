# Notas de Lanzamiento - Megaprocessor ASM Android v1.2.0

Esta versión oficial (`v1.2.0`, código de versión `10011`) moderniza integralmente la plataforma y toolchain de compilación del ensamblador nativo para Android, actualizando el soporte a Android 17 (API 37), toolchain CMake y NDK modernos, y configuración de firma oficial.

---

## 🚀 Nuevas Características y Mejoras

* **Actualización del SDK de Android y Plataformas:**
  - `compileSdk` y `targetSdk` actualizados a **API 37 (Android 17 / Cinnamon Bun)**.
  - Compatibilidad mantenida desde `minSdk 23` (Android 6.0 Marshmallow).
  - Android Build Tools actualizado a **37.0.0**.
  - Android NDK actualizado a **30.0.14904198 rc1** (`30.0.14904198`).
  - CMake actualizado a **4.1.2** para compilación del core nativo C++.

* **Modernización del Toolchain de Gradle:**
  - Android Gradle Plugin (AGP) actualizado a **9.2.1**.
  - Gradle Wrapper actualizado a **9.6.0**.
  - `org.gradle.configuration-cache=true` habilitado para acelerar compilaciones incrementales.

* **Actualización de Librerías y Dependencias:**
  - Componentes AndroidX y Material Design actualizados (`appcompat:1.7.1`, `material:1.14.0`, `constraintlayout:2.2.1`).

* **Configuración de Firma Release Oficial:**
  - Integración de firma release automatizada mediante `keystore.properties` (`keystore.jks` con alias `mega`).
  - Soporte para variables de entorno de firma (`SIGNING_STORE_FILE`, etc.) para pipelines de CI/CD.
  - Protección estricta en `.gitignore` para prevenir filtraciones de credenciales.

* **Redirección de Almacenamiento y Caché en `/tmp`:**
  - Redirección completa de la carpeta de compilación hacia `/tmp/calculo` para mantener limpio el espacio de trabajo.
  - Redirección automática de la caché de Gradle (`GRADLE_USER_HOME`) a `/tmp/.gradle` para optimizar entornos con disco persistente limitado (como Cloud Shell).

* **Documentación Técnica Integral:**
  - Incorporación de `GEMINI.md` con las instrucciones completas de instalación del SDK y compilación.
  - Actualización del `README.md` con las nuevas insignias y sección de compilación.

---

## 📦 Artefactos de la Versión

- **`app-release.apk`**: Paquete APK firmado y listo para instalación directa en dispositivos Android (API 23+).
- **`app-release.aab`**: Android App Bundle firmado y optimizado para distribución en Google Play Store.
