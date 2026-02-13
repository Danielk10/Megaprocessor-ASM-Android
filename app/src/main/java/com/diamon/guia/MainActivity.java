package com.diamon.guia;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.os.Bundle;
import android.Manifest;
import android.content.ContentValues;
import android.content.ClipData;
import android.content.ClipboardManager;
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
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.graphics.Color;
import android.content.Intent;
import android.app.AlertDialog;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.tabs.TabLayout;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import java.io.InputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;

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
    private TextView tvStatus, tvLineNumbers;
    private ExecutorService executorService;
    private Handler mainHandler;
    private FloatingActionButton fabAssemble, fabClear, fabWebSimulator, fabOfficialSite;
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

    private final ActivityResultLauncher<String[]> openFileLauncher = registerForActivityResult(
            new ActivityResultContracts.OpenDocument(),
            uri -> {
                if (uri != null) {
                    handleFileOpen(uri);
                }
            });

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
        tvLineNumbers = findViewById(R.id.tvLineNumbers);
        fabAssemble = findViewById(R.id.fabAssemble);
        fabClear = findViewById(R.id.fabClear);
        fabWebSimulator = findViewById(R.id.fabWebSimulator);
        fabOfficialSite = findViewById(R.id.fabOfficialSite);
        tabLayout = findViewById(R.id.tabLayout);

        fabAssemble.setOnClickListener(v -> assembleCode());
        fabClear.setOnClickListener(v -> showClearConfirmationDialog());
        fabWebSimulator.setOnClickListener(v -> openWebSimulator());
        fabOfficialSite.setOnClickListener(v -> openOfficialSite());

        setupTabLayout();
        setupEditorSyntaxHighlighting();

        // Cargar definiciones iniciales (Asíncrono)
        loadInitialTabs();
    }

    private void loadInitialTabs() {
        // tic_tac_toe_2.asm - Ejemplo del compilador oficial del Megaprocessor
        executorService.execute(() -> {
            try {
                final String exampleAsm = readAssetText("tic_tac_toe_2.asm");
                final String defsAsm = readAssetText("Megaprocessor_defs.asm");

                mainHandler.post(() -> {
                    tabFiles.put(getString(R.string.tab_main), exampleAsm);
                    tabFiles.put("Megaprocessor_defs.asm", defsAsm);

                    // Actualizar el editor solo si la pestaña actual coincide
                    CharSequence currentContent = tabFiles.get(currentTabName);
                    if (currentContent != null) {
                        etSource.setText(currentContent);
                        // Forzar resaltado inicial después de cargar el texto
                        applySyntaxHighlightingToEditor();
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
        if (id == R.id.action_new_tab) {
            showNewTabDialog();
            return true;
        } else if (id == R.id.action_open) {
            openFile();
            return true;
        } else if (id == R.id.action_project_info) {
            startActivity(new Intent(this, ProjectInfoActivity.class));
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
        String aboutText = "<b>Megaprocessor ASM Android</b><br/>" +
                "Versión 1.1.8<br/><br/>" +
                "Aplicación Android con núcleo nativo C++ para ensamblar programas del Megaprocessor, " +
                "generar Intel HEX y listados LST, y verificar compatibilidad con referencias oficiales.<br/><br/>" +
                "Este proyecto facilita aprendizaje de arquitectura de computadores y uso práctico " +
                "del ecosistema Megaprocessor en dispositivos móviles.<br/><br/>" +
                "<b><a href=\"https://www.megaprocessor.com/\">Sitio oficial del proyecto original</a></b><br/>" +
                "<b><a href=\"https://github.com/Danielk10/Megaprocessor-ASM-Android\">Repositorio Android</a></b>";

        android.text.Spanned spanned;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            spanned = android.text.Html.fromHtml(aboutText, android.text.Html.FROM_HTML_MODE_LEGACY);
        } else {
            @SuppressWarnings("deprecation")
            android.text.Spanned legacySpanned = android.text.Html.fromHtml(aboutText);
            spanned = legacySpanned;
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
        String licensesText = "<b>Licencias y créditos</b><br/><br/>" +
                "• Esta app se distribuye bajo licencia Apache 2.0.<br/>" +
                "• El diseño e idea original de Megaprocessor pertenecen a James Newman.<br/>" +
                "• El sitio oficial del proyecto original contiene documentación, arquitectura y simulador web.<br/><br/>" +
                "<b><a href=\"https://www.apache.org/licenses/LICENSE-2.0\">Texto de Apache 2.0</a></b><br/>" +
                "<b><a href=\"https://www.megaprocessor.com/\">Megaprocessor Official Site</a></b>";

        android.text.Spanned spanned;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            spanned = android.text.Html.fromHtml(licensesText, android.text.Html.FROM_HTML_MODE_LEGACY);
        } else {
            @SuppressWarnings("deprecation")
            android.text.Spanned legacySpanned = android.text.Html.fromHtml(licensesText);
            spanned = legacySpanned;
        }

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle(R.string.menu_licenses)
                .setMessage(spanned)
                .setPositiveButton("OK", null)
                .show();

        TextView textView = (TextView) dialog.findViewById(android.R.id.message);
        if (textView != null) {
            textView.setMovementMethod(LinkMovementMethod.getInstance());
        }
    }

    private void openFile() {
        openFileLauncher.launch(new String[] { "*/*" });
    }

    private void handleFileOpen(Uri uri) {
        executorService.execute(() -> {
            try (InputStream is = getContentResolver().openInputStream(uri);
                    BufferedReader reader = new BufferedReader(new InputStreamReader(is, StandardCharsets.UTF_8))) {

                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line).append("\n");
                }

                String content = sb.toString();

                // Get filename
                String fileName = "archivo_externo.asm";
                try (android.database.Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
                    if (cursor != null && cursor.moveToFirst()) {
                        int nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                        if (nameIndex != -1) {
                            fileName = cursor.getString(nameIndex);
                        }
                    }
                }

                final String finalName = fileName;
                mainHandler.post(() -> {
                    addNewTab(finalName, content);
                    setStatus("Archivo cargado: " + finalName, false);
                });

            } catch (Exception e) {
                mainHandler.post(() -> setStatus("Error al leer archivo: " + e.getMessage(), true));
            }
        });
    }

    private String readAssetText(String assetName) throws IOException {
        try (BufferedInputStream bis = new BufferedInputStream(getAssets().open(assetName));
                java.io.ByteArrayOutputStream baos = new java.io.ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096];
            int read;
            while ((read = bis.read(buffer)) != -1) {
                baos.write(buffer, 0, read);
            }
            return new String(baos.toByteArray(), StandardCharsets.UTF_8);
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
                // 1. Cargar definiciones base del sistema si NO están en las pestañas
                if (!tabFiles.containsKey("Megaprocessor_defs.asm")) {
                    try {
                        String defs = readAssetText("Megaprocessor_defs.asm");
                        assembler.registerIncludeFile("Megaprocessor_defs.asm", defs);
                    } catch (IOException e) {
                        mainHandler.post(() -> setStatus("Aviso: no se cargaron defs base", false));
                    }
                }

                // 2. Registrar todos los archivos abiertos (prioridad a ediciones del usuario)
                for (Map.Entry<String, CharSequence> entry : tabFiles.entrySet()) {
                    assembler.registerIncludeFile(entry.getKey(), entry.getValue().toString());
                }

                // 3. Ensamblar
                final String result = assembler.assemble(source);

                if (result.startsWith("ERROR")) {
                    mainHandler.post(() -> {
                        setStatus(getString(R.string.status_failed), true);
                        showErrorDialog(result);
                    });
                } else {
                    lastGeneratedHex = result;
                    lastGeneratedList = assembler.getListing();

                    // Optimization: Process hex coloring in background thread
                    final CharSequence colorizedHex = colorizeHexOptimized(result);

                    mainHandler.post(() -> {
                        setStatus(getString(R.string.status_success), false);
                        showHexPopup(colorizedHex);
                    });
                }
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
                // Guardar contenido de la pestaña que se deja (preservando spans/colores)
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
        TabLayout.Tab tab = tabLayout.newTab();
        tab.setText(name); // Para que onTabUnselected siga funcionando

        View customView = LayoutInflater.from(this).inflate(R.layout.tab_item, null);
        TextView tvTitle = customView.findViewById(R.id.tabTitle);
        ImageButton btnRemove = customView.findViewById(R.id.btnRemoveTab);

        tvTitle.setText(name);
        btnRemove.setOnClickListener(v -> removeTab(tab));

        tab.setCustomView(customView);
        tabLayout.addTab(tab);

        if (tabLayout.getTabCount() == 1) {
            tab.select();
            currentTabName = name;
        }
    }

    private void removeTab(TabLayout.Tab tab) {
        String name = "";
        if (tab.getText() != null) {
            name = tab.getText().toString();
        }

        // Eliminar del layout. Esto disparará los listeners de selección.
        tabLayout.removeTab(tab);

        // Eliminar de nuestra base de datos en memoria
        tabFiles.remove(name);

        // Si ya no quedan pestañas, crear una nueva vacía
        if (tabLayout.getTabCount() == 0) {
            addNewTab("Sin título", "");
        }
    }

    private void switchTab(String name) {
        if (name == null || name.equals(currentTabName))
            return;

        currentTabName = name;
        CharSequence content = tabFiles.get(name);

        // Determinar si ya tiene colores para evitar el lag de re-procesado
        boolean isAlreadyHighlighted = false;
        if (content instanceof Spannable) {
            ForegroundColorSpan[] spans = ((Spannable) content).getSpans(0, content.length(),
                    ForegroundColorSpan.class);
            if (spans != null && spans.length > 0) {
                isAlreadyHighlighted = true;
            }
        }

        isTabSwitching = true;
        etSource.setText(content != null ? content : "");
        isTabSwitching = false;

        // Actualizar números de línea tras el cambio
        updateLineNumbers();

        // Solo resaltar si no estaba cacheado o si es texto plano
        if (!isAlreadyHighlighted && content != null && content.length() > 0) {
            applySyntaxHighlightingToEditor();
        }
    }

    private void showErrorDialog(String error) {
        new AlertDialog.Builder(this)
                .setTitle(R.string.status_failed)
                .setMessage(error)
                .setPositiveButton("OK", null)
                .show();
    }

    private void showClearConfirmationDialog() {
        new AlertDialog.Builder(this)
                .setTitle(R.string.confirm_clear_title)
                .setMessage(R.string.confirm_clear_message)
                .setPositiveButton("Limpiar", (dialog, which) -> {
                    etSource.setText("");
                    if (currentTabName != null) {
                        tabFiles.put(currentTabName, "");
                    }
                })
                .setNegativeButton("Cancelar", null)
                .show();
    }

    private void showHexPopup(CharSequence hexContent) {
        LayoutInflater inflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.popup_hex, null);

        int width = ViewGroup.LayoutParams.MATCH_PARENT;
        int height = ViewGroup.LayoutParams.MATCH_PARENT;
        final PopupWindow popupWindow = new PopupWindow(popupView, width, height, true);

        TextView tvPopupHex = popupView.findViewById(R.id.tvPopupHex);
        Button btnClose = popupView.findViewById(R.id.btnClosePopup);
        Button btnExport = popupView.findViewById(R.id.btnExportPopup);
        Button btnCopy = popupView.findViewById(R.id.btnCopyHexPopup);

        tvPopupHex.setText(hexContent);

        btnClose.setOnClickListener(v -> popupWindow.dismiss());
        btnExport.setOnClickListener(v -> {
            exportFiles();
            popupWindow.dismiss();
        });
        btnCopy.setOnClickListener(v -> copyHexToClipboard(hexContent.toString()));

        popupWindow.showAtLocation(findViewById(android.R.id.content), Gravity.CENTER, 0, 0);
    }

    private void copyHexToClipboard(String hexText) {
        ClipboardManager clipboard = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        if (clipboard == null) {
            setStatus("No se pudo acceder al portapapeles", true);
            return;
        }
        ClipData clip = ClipData.newPlainText("megaprocessor_hex", hexText);
        clipboard.setPrimaryClip(clip);
        setStatus("HEX copiado al portapapeles", false);
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
                    updateLineNumbers();
                    scheduleSyntaxHighlighting();
                }
            }
        });
    }

    private void updateLineNumbers() {
        if (etSource == null || tvLineNumbers == null)
            return;

        String text = etSource.getText().toString();
        int lineCount = 0;
        if (text.isEmpty()) {
            lineCount = 1;
        } else {
            // Contar saltos de línea manualmente para mayor precisión inmediata
            for (int i = 0; i < text.length(); i++) {
                if (text.charAt(i) == '\n') {
                    lineCount++;
                }
            }
            lineCount++; // La última línea no termina en \n habitualmente
        }

        StringBuilder sb = new StringBuilder();
        for (int i = 1; i <= lineCount; i++) {
            sb.append(i).append("\n");
        }
        tvLineNumbers.setText(sb.toString());
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

        final Editable currentEditable = etSource.getText();
        if (currentEditable == null || currentEditable.length() == 0)
            return;

        final String textStr = currentEditable.toString();
        final int cursorStart = etSource.getSelectionStart();
        final int cursorEnd = etSource.getSelectionEnd();

        isApplyingSyntaxHighlight = true;

        // Colores para el resaltado
        final int colorMnemonic = ContextCompat.getColor(this, R.color.syntax_mnemonic);
        final int colorDirective = ContextCompat.getColor(this, R.color.syntax_directive);
        final int colorLabel = ContextCompat.getColor(this, R.color.syntax_label);
        final int colorNumber = ContextCompat.getColor(this, R.color.syntax_number);
        final int colorComment = ContextCompat.getColor(this, R.color.syntax_comment);

        executorService.execute(() -> {
            try {
                // Crear un borrador con los colores aplicados en segundo plano
                final SpannableStringBuilder ssb = new SpannableStringBuilder(textStr);

                applyColorToBuilder(ssb, PATTERN_MNEMONIC, colorMnemonic);
                applyColorToBuilder(ssb, PATTERN_DIRECTIVE, colorDirective);
                applyColorToBuilder(ssb, PATTERN_LABEL, colorLabel);
                applyColorToBuilder(ssb, PATTERN_NUMBER, colorNumber);
                applyColorToBuilder(ssb, PATTERN_COMMENT, colorComment);

                mainHandler.post(() -> {
                    // Verificar si el texto ha cambiado mientras procesábamos
                    if (etSource == null || !etSource.getText().toString().equals(textStr))
                        return;

                    isTabSwitching = true; // Bloquear TextWatcher durante el reemplazo atómico
                    etSource.setText(ssb);
                    // Restaurar cursor
                    if (cursorStart >= 0 && cursorEnd >= 0 && cursorEnd <= ssb.length()) {
                        etSource.setSelection(cursorStart, cursorEnd);
                    }
                    isTabSwitching = false;
                });
            } finally {
                isApplyingSyntaxHighlight = false;
            }
        });
    }

    private void applyColorToBuilder(SpannableStringBuilder ssb, Pattern pattern, int color) {
        Matcher matcher = pattern.matcher(ssb.toString());
        while (matcher.find()) {
            ssb.setSpan(new ForegroundColorSpan(color), matcher.start(), matcher.end(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
    }

    // Ya no lo usamos con el nuevo enfoque atómico
    private void collectSpans(String text, Pattern pattern, int color, List<SpanRange> out) {
    }

    // Ya no lo usamos con el nuevo enfoque atómico
    private static class SpanRange {
        int start, end, color;

        SpanRange(int start, int end, int color) {
            this.start = start;
            this.end = end;
            this.color = color;
        }
    }

    private void openInstructionsDocs() {
        openUrl("https://www.megaprocessor.com/simulator.pdf");
    }

    private void openWebSimulator() {
        openInstructionsDocs();
    }

    private void openOfficialSite() {
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
        final String baseName = getCurrentTabBaseName();
        final String hexName = baseName + ".hex";
        final String lstName = baseName + ".lst";

        setStatus("Exportando a Descargas...", false);
        executorService.execute(() -> {
            boolean hexSaved = saveFileToDownloads(hexName, lastGeneratedHex);
            boolean lstSaved = saveFileToDownloads(lstName, lst);
            mainHandler.post(() -> setStatus(hexSaved && lstSaved ? "Guardado en Descargas" : "Error al guardar",
                    !(hexSaved && lstSaved)));
        });
    }

    private String getCurrentTabBaseName() {
        String tabName = currentTabName;
        if (tabName == null || tabName.trim().isEmpty()) {
            tabName = "programa";
        }
        tabName = tabName.trim();
        int dot = tabName.lastIndexOf('.');
        if (dot > 0) {
            tabName = tabName.substring(0, dot);
        }
        return sanitizeFileName(tabName);
    }

    private String sanitizeFileName(String input) {
        String cleaned = input.replaceAll("[\\/:*?\"<>|]", "_").trim();
        if (cleaned.isEmpty()) {
            return "programa";
        }
        return cleaned;
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
        @SuppressWarnings("deprecation")
        File downloadsDir = android.os.Environment
                .getExternalStoragePublicDirectory(android.os.Environment.DIRECTORY_DOWNLOADS);
        File dir = new File(downloadsDir, "Megaprocessor");
        if (!dir.exists() && !dir.mkdirs())
            return false;
        File file = new File(dir, fileName);
        try (FileOutputStream fos = new FileOutputStream(file)) {
            fos.write(content.getBytes(StandardCharsets.UTF_8));
            return true;
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
