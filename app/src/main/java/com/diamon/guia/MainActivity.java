package com.diamon.guia;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.os.Bundle;
import android.Manifest;
import android.content.ContentValues;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.text.SpannableStringBuilder;
import android.text.Spannable;
import android.text.Editable;
import android.text.TextWatcher;
import android.text.style.ForegroundColorSpan;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.graphics.Color;
import android.content.Intent;
import android.app.AlertDialog;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.tabs.TabLayout;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MainActivity extends AppCompatActivity {

    private static final int REQUEST_WRITE_EXTERNAL_STORAGE = 1001;

    private NativeAssembler assembler;
    private EditText etSource;
    private TextView tvStatus;
    private ExecutorService executorService;
    private Handler mainHandler;
    private FloatingActionButton fabAssemble;
    private TabLayout tabLayout;

    // Gestión de pestañas: Nombre del archivo -> Contenido (Cacheando Spannables
    // para rendimiento)
    private final Map<String, CharSequence> tabFiles = new LinkedHashMap<>();
    private String currentTabName = "";

    private boolean isApplyingSyntaxHighlight = false;
    private boolean isTabSwitching = false;
    private String lastGeneratedHex = "";
    private String lastGeneratedList = "";

    // Optimizaciones de Resaltado
    private final Handler syntaxHandler = new Handler(Looper.getMainLooper());
    private Runnable syntaxRunnable;
    private static final long HIGHLIGHT_DELAY = 300; // ms

    private static final Pattern PATTERN_MNEMONIC = Pattern.compile(
            "(?i)\\b(MOVE|AND|XOR|OR|ADD|SUB|CMP|LD|ST|BCC|BCS|BNE|BEQ|BVC|BVS|BPL|BMI|BGE|BLT|BGT|BLE|BUC|BUS|BHI|BLS|JMP|JSR|POP|PUSH|RET|RETI|TRAP|NOP|SXT|ABS|INV|NEG|CLR|INC|DEC|ADDQ|TEST|R0|R1|R2|R3|SP|PS|PC)\\b");
    private static final Pattern PATTERN_DIRECTIVE = Pattern.compile("(?i)\\b(ORG|EQU|DB|DW|DL|DM|DS|INCLUDE)\\b");
    private static final Pattern PATTERN_LABEL = Pattern.compile("(?m)^\\s*[A-Za-z_][A-Za-z0-9_]*:");
    private static final Pattern PATTERN_NUMBER = Pattern.compile("\\b(0x[0-9A-Fa-f]+|\\d+)\\b");
    private static final Pattern PATTERN_COMMENT = Pattern.compile("(;.*$|//.*$)");

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        assembler = new NativeAssembler();
        executorService = Executors.newSingleThreadExecutor();
        mainHandler = new Handler(Looper.getMainLooper());

        etSource = findViewById(R.id.etSourceCode);
        tvStatus = findViewById(R.id.tvStatus);
        fabAssemble = findViewById(R.id.fabAssemble);
        tabLayout = findViewById(R.id.tabLayout);

        fabAssemble.setOnClickListener(v -> assembleCode());

        setupTabLayout();
        setupEditorSyntaxHighlighting();

        // Cargar definiciones iniciales (Asíncrono)
        loadInitialTabs();
    }

    private void loadInitialTabs() {
        // tic_tac_toe_2.asm - Ejemplo del compilador oficial del Megaprocessor
        executorService.execute(() -> {
            try {
                final String exampleAsm = readAssetText("example.asm");
                final String defsAsm = readAssetText("Megaprocessor_defs.asm");

                mainHandler.post(() -> {
                    tabFiles.put(getString(R.string.tab_main), exampleAsm);
                    tabFiles.put("Megaprocessor_defs.asm", defsAsm);

                    // Actualizar el editor solo si la pestaña actual coincide
                    CharSequence currentContent = tabFiles.get(currentTabName);
                    if (currentContent != null) {
                        isTabSwitching = true;
                        etSource.setText(currentContent);
                        isTabSwitching = false;
                    }
                });
            } catch (IOException e) {
                mainHandler.post(() -> setStatus("Error al cargar archivos iniciales", true));
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_save) {
            exportFiles();
            return true;
        } else if (id == R.id.action_new_tab) {
            showNewTabDialog();
            return true;
        } else if (id == R.id.action_about) {
            showAbout();
            return true;
        } else if (id == R.id.action_licenses) {
            showLicenses();
            return true;
        } else if (id == R.id.action_privacy) {
            startActivity(new Intent(this, PrivacyPolicyActivity.class));
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void showAbout() {
        String aboutText = "Megaprocessor ASM Android<br/>" +
                "Versión 1.0.8<br/><br/>" +
                "Autor: Daniel Diamon (Danielk10)<br/>" +
                "<b><a href=\"https://github.com/Danielk10/Megaprocessor-ASM-Android\">Repositorio Oficial</a></b>";

        android.text.Spanned spanned;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            spanned = android.text.Html.fromHtml(aboutText, android.text.Html.FROM_HTML_MODE_LEGACY);
        } else {
            spanned = android.text.Html.fromHtml(aboutText);
        }

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle(R.string.menu_about)
                .setMessage(spanned)
                .setPositiveButton("OK", null)
                .show();

        TextView textView = (TextView) dialog.findViewById(android.R.id.message);
        if (textView != null) {
            textView.setMovementMethod(LinkMovementMethod.getInstance());
        }
    }

    private void showLicenses() {
        new AlertDialog.Builder(this)
                .setTitle(R.string.menu_licenses)
                .setMessage(R.string.license_content)
                .setPositiveButton("OK", null)
                .show();
    }

    private void registerDefaultIncludes() {
        try {
            String defs = readAssetText("Megaprocessor_defs.asm");
            assembler.registerIncludeFile("Megaprocessor_defs.asm", defs);
        } catch (IOException e) {
            setStatus("Error: no se pudo cargar Megaprocessor_defs.asm", true);
        }
    }

    private String readAssetText(String assetName) throws IOException {
        try (BufferedInputStream bis = new BufferedInputStream(getAssets().open(assetName))) {
            byte[] buffer = new byte[bis.available()];
            int read = bis.read(buffer);
            if (read < 0)
                return "";
            return new String(buffer, 0, read);
        }
    }

    private void setStatus(final String message, final boolean isError) {
        mainHandler.post(() -> {
            if (tvStatus != null) {
                tvStatus.setText(message);
                tvStatus.setBackgroundColor(isError ? Color.parseColor("#B00020") : Color.parseColor("#43A047"));
                tvStatus.setVisibility(View.VISIBLE);

                mainHandler.postDelayed(() -> {
                    if (tvStatus != null)
                        tvStatus.setVisibility(View.GONE);
                }, 3000);
            }
        });
    }

    private void assembleCode() {
        if (currentTabName == null || currentTabName.isEmpty())
            return;

        // Guardar contenido actual con spans antes de ensamblar
        tabFiles.put(currentTabName, new SpannableStringBuilder(etSource.getText()));

        final CharSequence sourceSeq = tabFiles.get(currentTabName);
        if (sourceSeq == null || sourceSeq.toString().trim().isEmpty()) {
            setStatus("El código está vacío", true);
            return;
        }
        final String source = sourceSeq.toString();

        setStatus(getString(R.string.status_assembling), false);

        executorService.execute(() -> {
            try {
                // Registrar todos los archivos abiertos como includes
                for (Map.Entry<String, CharSequence> entry : tabFiles.entrySet()) {
                    assembler.registerIncludeFile(entry.getKey(), entry.getValue().toString());
                }

                // Asegurar que las defs del sistema estén siempre
                registerDefaultIncludes();

                final String result = assembler.assemble(source);

                mainHandler.post(() -> {
                    if (result.startsWith("ERROR")) {
                        setStatus(getString(R.string.status_failed), true);
                        showErrorDialog(result);
                    } else {
                        lastGeneratedHex = result;
                        lastGeneratedList = assembler.getListing();
                        setStatus(getString(R.string.status_success), false);
                        showHexPopup(result);
                    }
                });
            } catch (final Exception e) {
                mainHandler.post(() -> setStatus("Error crítico: " + e.getMessage(), true));
            }
        });
    }

    private void setupTabLayout() {
        tabLayout.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                switchTab(tab.getText().toString());
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {
                // Guardar contenido de la pestaña que se deja (manteniendo el resaltado)
                if (tab.getText() != null && etSource != null) {
                    tabFiles.put(tab.getText().toString(), new SpannableStringBuilder(etSource.getText()));
                }
            }

            @Override
            public void onTabReselected(TabLayout.Tab tab) {
            }
        });

        // Menú contextual o botón para añadir pestañas podría ir aquí,
        // por ahora añadimos las básicas
        addNewTab(getString(R.string.tab_main), "");
        addNewTab("Megaprocessor_defs.asm", "");
    }

    private void addNewTab(String name, String content) {
        tabFiles.put(name, content);
        TabLayout.Tab tab = tabLayout.newTab().setText(name);
        tabLayout.addTab(tab);
        if (tabLayout.getTabCount() == 1) {
            tab.select();
            currentTabName = name;
        }
    }

    private void switchTab(String name) {
        if (name.equals(currentTabName))
            return;
        currentTabName = name;
        CharSequence content = tabFiles.get(name);
        isTabSwitching = true;
        etSource.setText(content != null ? content : "");
        isTabSwitching = false;
    }

    private void showErrorDialog(String error) {
        new AlertDialog.Builder(this)
                .setTitle(R.string.status_failed)
                .setMessage(error)
                .setPositiveButton("OK", null)
                .show();
    }

    private void showHexPopup(String hex) {
        LayoutInflater inflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.popup_hex, null);

        int width = ViewGroup.LayoutParams.MATCH_PARENT;
        int height = ViewGroup.LayoutParams.MATCH_PARENT;
        final PopupWindow popupWindow = new PopupWindow(popupView, width, height, true);

        TextView tvPopupHex = popupView.findViewById(R.id.tvPopupHex);
        Button btnClose = popupView.findViewById(R.id.btnClosePopup);
        Button btnExport = popupView.findViewById(R.id.btnExportPopup);

        tvPopupHex.setText(colorizeHexOptimized(normalizeHex(hex)));

        btnClose.setOnClickListener(v -> popupWindow.dismiss());
        btnExport.setOnClickListener(v -> {
            exportFiles();
            popupWindow.dismiss();
        });

        popupWindow.showAtLocation(findViewById(android.R.id.content), Gravity.CENTER, 0, 0);
    }

    private void showNewTabDialog() {
        EditText etName = new EditText(this);
        etName.setHint("nombre_archivo.asm");
        new AlertDialog.Builder(this)
                .setTitle("Nuevo Archivo ASM")
                .setView(etName)
                .setPositiveButton("Crear", (dialog, which) -> {
                    String name = etName.getText().toString().trim();
                    if (!name.isEmpty()) {
                        if (!name.endsWith(".asm"))
                            name += ".asm";
                        addNewTab(name, "");
                    }
                })
                .setNegativeButton("Cancelar", null)
                .show();
    }

    private void setupEditorSyntaxHighlighting() {
        etSource.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable editable) {
                if (!isTabSwitching) {
                    scheduleSyntaxHighlighting();
                }
            }
        });
    }

    private void scheduleSyntaxHighlighting() {
        if (syntaxRunnable != null) {
            syntaxHandler.removeCallbacks(syntaxRunnable);
        }
        syntaxRunnable = this::applySyntaxHighlightingToEditor;
        syntaxHandler.postDelayed(syntaxRunnable, HIGHLIGHT_DELAY);
    }

    private void applySyntaxHighlightingToEditor() {
        if (isApplyingSyntaxHighlight || isTabSwitching || etSource == null)
            return;
        Editable text = etSource.getText();
        if (text == null || text.length() == 0)
            return;

        isApplyingSyntaxHighlight = true;
        try {
            ForegroundColorSpan[] spans = text.getSpans(0, text.length(), ForegroundColorSpan.class);
            for (ForegroundColorSpan span : spans)
                text.removeSpan(span);

            highlightWithPattern(text, PATTERN_MNEMONIC, ContextCompat.getColor(this, R.color.syntax_mnemonic));
            highlightWithPattern(text, PATTERN_DIRECTIVE, ContextCompat.getColor(this, R.color.syntax_directive));
            highlightWithPattern(text, PATTERN_LABEL, ContextCompat.getColor(this, R.color.syntax_label));
            highlightWithPattern(text, PATTERN_NUMBER, ContextCompat.getColor(this, R.color.syntax_number));
            highlightWithPattern(text, PATTERN_COMMENT, ContextCompat.getColor(this, R.color.syntax_comment));
        } finally {
            isApplyingSyntaxHighlight = false;
        }
    }

    private void highlightWithPattern(Editable text, Pattern pattern, int color) {
        Matcher matcher = pattern.matcher(text);
        while (matcher.find()) {
            text.setSpan(new ForegroundColorSpan(color), matcher.start(), matcher.end(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
    }

    private void openInstructionsDocs() {
        openUrl("https://www.megaprocessor.com/architecture.html");
    }

    private void openWebSimulator() {
        openUrl("https://www.megaprocessor.com/");
    }

    private void openUrl(String url) {
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
        } catch (Exception e) {
            setStatus("No se pudo abrir el enlace", true);
        }
    }

    private void exportFiles() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q &&
                ContextCompat.checkSelfPermission(this,
                        Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[] { Manifest.permission.WRITE_EXTERNAL_STORAGE },
                    REQUEST_WRITE_EXTERNAL_STORAGE);
            return;
        }
        exportFilesInternal();
    }

    private void exportFilesInternal() {
        if (lastGeneratedHex.isEmpty() || lastGeneratedHex.startsWith("ERROR")) {
            setStatus("Ensambla con éxito primero", true);
            return;
        }

        final String lst = assembler.getListing();
        final String timestamp = String.valueOf(System.currentTimeMillis());
        final String hexName = "megaprocessor_" + timestamp + ".hex";
        final String lstName = "megaprocessor_" + timestamp + ".list";

        setStatus("Exportando a Descargas...", false);
        executorService.execute(() -> {
            boolean hexSaved = saveFileToDownloads(hexName, lastGeneratedHex);
            boolean lstSaved = saveFileToDownloads(lstName, lst);
            mainHandler.post(() -> setStatus(hexSaved && lstSaved ? "Guardado en Descargas" : "Error al guardar",
                    !(hexSaved && lstSaved)));
        });
    }

    private boolean saveFileToDownloads(String fileName, String content) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            ContentValues values = new ContentValues();
            values.put(android.provider.MediaStore.Downloads.DISPLAY_NAME, fileName);
            values.put(android.provider.MediaStore.Downloads.MIME_TYPE, "application/octet-stream"); // Forzamos tipo
                                                                                                     // binario para
                                                                                                     // evitar .txt
            values.put(android.provider.MediaStore.Downloads.RELATIVE_PATH,
                    android.os.Environment.DIRECTORY_DOWNLOADS + "/Megaprocessor");
            values.put(android.provider.MediaStore.Downloads.IS_PENDING, 1);
            Uri uri = getContentResolver().insert(android.provider.MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
            if (uri == null)
                return false;
            try (OutputStream os = getContentResolver().openOutputStream(uri)) {
                os.write(content.getBytes(StandardCharsets.UTF_8));
                os.flush();
            } catch (IOException e) {
                getContentResolver().delete(uri, null, null);
                return false;
            }
            ContentValues done = new ContentValues();
            done.put(android.provider.MediaStore.Downloads.IS_PENDING, 0);
            getContentResolver().update(uri, done, null, null);
            return true;
        }
        try {
            File downloadsDir = android.os.Environment
                    .getExternalStoragePublicDirectory(android.os.Environment.DIRECTORY_DOWNLOADS);
            File dir = new File(downloadsDir, "Megaprocessor");
            if (!dir.exists() && !dir.mkdirs())
                return false;
            File file = new File(dir, fileName);
            try (FileOutputStream fos = new FileOutputStream(file)) {
                fos.write(content.getBytes(StandardCharsets.UTF_8));
                return true;
            }
        } catch (IOException e) {
            return false;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_WRITE_EXTERNAL_STORAGE && grantResults.length > 0
                && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            exportFilesInternal();
        }
    }

    private String normalizeHex(String hexContent) {
        if (hexContent == null)
            return "";
        Map<Integer, Byte> image = parseHexImage(hexContent);
        if (image.isEmpty())
            return hexContent.trim();
        List<Integer> addresses = new ArrayList<>(image.keySet());
        Collections.sort(addresses);
        StringBuilder normalized = new StringBuilder();
        int recordSize = 0x10;
        int index = 0;
        while (index < addresses.size()) {
            int start = addresses.get(index);
            List<Byte> data = new ArrayList<>();
            data.add(image.get(start));
            index++;
            while (index < addresses.size() && data.size() < recordSize) {
                if (addresses.get(index) != start + data.size())
                    break;
                data.add(image.get(addresses.get(index)));
                index++;
            }
            normalized.append(formatRecord(start, data)).append("\n");
        }
        normalized.append(":00000001FF\n");
        return normalized.toString();
    }

    private String formatRecord(int address, List<Byte> data) {
        int checksum = data.size() + ((address >> 8) & 0xFF) + (address & 0xFF);
        StringBuilder line = new StringBuilder(":").append(String.format("%02X%04X00", data.size(), address & 0xFFFF));
        for (byte b : data) {
            int u = b & 0xFF;
            checksum += u;
            line.append(String.format("%02X", u));
        }
        line.append(String.format("%02X", ((~checksum + 1) & 0xFF)));
        return line.toString();
    }

    private Map<Integer, Byte> parseHexImage(String hex) {
        Map<Integer, Byte> image = new LinkedHashMap<>();
        if (hex == null)
            return image;
        for (String raw : hex.split("\\r?\\n")) {
            String line = raw.trim();
            if (!line.startsWith(":") || line.length() < 11)
                continue;
            try {
                int count = Integer.parseInt(line.substring(1, 3), 16);
                int address = Integer.parseInt(line.substring(3, 7), 16);
                if (Integer.parseInt(line.substring(7, 9), 16) != 0)
                    continue;
                for (int i = 0; i < count; i++) {
                    int val = Integer.parseInt(line.substring(9 + i * 2, 11 + i * 2), 16);
                    image.put(address + i, (byte) val);
                }
            } catch (Exception ignored) {
            }
        }
        return image;
    }

    private SpannableStringBuilder colorizeHexOptimized(String hex) {
        SpannableStringBuilder builder = new SpannableStringBuilder();
        String[] lines = hex.split("\n");
        for (int i = 0; i < lines.length; i++) {
            String line = lines[i];
            if (line.isEmpty())
                continue;
            int start = builder.length();
            builder.append(String.format(Locale.US, "%04d | %s\n", i + 1, line));

            int hexStart = start + 7; // Tras el prefijo "0001 | "
            int lineLen = line.length();

            if (line.startsWith(":") && lineLen >= 1) {
                // Prefijo : (azul) y metadatos
                int headerEnd = Math.min(hexStart + 9, hexStart + lineLen);
                builder.setSpan(new ForegroundColorSpan(Color.BLUE), hexStart, headerEnd, 0);

                // Datos (negro)
                if (lineLen > 11) {
                    builder.setSpan(new ForegroundColorSpan(Color.BLACK), hexStart + 9, hexStart + lineLen - 2, 0);
                    // Checksum (verde)
                    builder.setSpan(new ForegroundColorSpan(Color.parseColor("#008000")), hexStart + lineLen - 2,
                            hexStart + lineLen, 0);
                } else if (lineLen > 9) {
                    // Solo checksum si es corto (poco probable pero posible)
                    builder.setSpan(new ForegroundColorSpan(Color.parseColor("#008000")), hexStart + lineLen - 2,
                            hexStart + lineLen, 0);
                }
            }
        }
        return builder;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (executorService != null)
            executorService.shutdown();
    }
}
