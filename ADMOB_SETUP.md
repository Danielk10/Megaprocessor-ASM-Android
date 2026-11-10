# Configuración de AdMob — Megaprocessor ASM Android

La app tiene integrado Google AdMob con:

- `play-services-ads` **25.5.0**
- `user-messaging-platform` (UMP) **4.0.0**
- Banner adaptativo + intersticial con límite de 3 minutos
- Consentimiento RGPD/EE. UU. antes de inicializar MobileAds

## 1. App ID (configurado)

```xml
<string name="admob_app_id">ca-app-pub-5141499161332805~9440816228</string>
```

## 2. Bloques de anuncios (reales, configurados)

En `app/src/main/java/com/diamon/guia/ads/AdConfig.java`:

```java
public static final String BANNER_AD_UNIT_ID = "ca-app-pub-5141499161332805/8369870523";
public static final String INTERSTITIAL_AD_UNIT_ID = "ca-app-pub-5141499161332805/9481652541";
```

Corresponden a los bloques **Banner** e **Intersticial** de la app
`com.diamon.guia` en AdMob.

## 3. Pruebas

- Para probar sin riesgo de clics inválidos, registrar el dispositivo de prueba
  en AdMob (**Configuración → Dispositivos de prueba**) en lugar de volver a los
  IDs de prueba de Google.

## 4. Google Play

- La app declara el permiso `com.google.android.gms.permission.AD_ID`.
- En **Data safety** hay que declarar el uso de identificadores de publicidad y
  la recopilación realizada por Google AdMob.
- La opción **Opciones de privacidad** aparece en el menú solo cuando UMP la
  considera obligatoria.
- La app no tiene compras internas ni suscripciones.
