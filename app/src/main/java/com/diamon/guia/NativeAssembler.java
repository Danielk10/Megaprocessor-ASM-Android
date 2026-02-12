package com.diamon.guia;

public class NativeAssembler {
    static {
        System.loadLibrary("megaprocessor");
    }

    /**
     * Ensambla el código fuente dado y devuelve el código máquina en formato Hex
     * o un mensaje de error comenzando con "ERROR:".
     *
     * @param sourceCode El código ensamblador completo.
     * @return String con el resultado (Intel Hex) o error.
     */
    public native String assemble(String sourceCode);

    public native String getListing();

    public native void registerIncludeFile(String includeName, String includeContent);
}
