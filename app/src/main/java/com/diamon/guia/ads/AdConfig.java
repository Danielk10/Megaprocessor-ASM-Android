package com.diamon.guia.ads;

/**
 * Configuración central de Google AdMob para Megaprocessor ASM Android.
 *
 * App ID: ca-app-pub-5141499161332805~9440816228
 * (declarado en AndroidManifest.xml como @string/admob_app_id)
 *
 * Bloques reales creados en AdMob:
 * - Banner:      ca-app-pub-5141499161332805/8369870523
 * - Intersticial: ca-app-pub-5141499161332805/9481652541
 */
public final class AdConfig {

    private AdConfig() {
    }

    /** Banner adaptativo anclado en la parte inferior. */
    public static final String BANNER_AD_UNIT_ID = "ca-app-pub-5141499161332805/8369870523";

    /** Intersticial con límite de frecuencia de 3 minutos. */
    public static final String INTERSTITIAL_AD_UNIT_ID = "ca-app-pub-5141499161332805/9481652541";
}
