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
    private Button btnExport;
    private Button btnShare;
    private Button btnDocs;
    private Button btnWebSim;
    private boolean isApplyingSyntaxHighlight = false;
    private String lastGeneratedHex = "";

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
        btnExport = findViewById(R.id.btnExport);
        btnShare = findViewById(R.id.btnShare);
        btnDocs = findViewById(R.id.btnDocs);
        btnWebSim = findViewById(R.id.btnWebSim);

        fabAssemble.setOnClickListener(v -> assembleCode());
        btnExport.setOnClickListener(v -> exportFiles());
        btnShare.setOnClickListener(v -> shareProject());
        btnDocs.setOnClickListener(v -> openInstructionsDocs());
        btnWebSim.setOnClickListener(v -> openWebSimulator());

        setupEditorSyntaxHighlighting();

        // Auto-load example
        loadExample();
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
        new AlertDialog.Builder(this)
                .setTitle(R.string.menu_about)
                .setMessage("Megaprocessor ASM Android\nVersión 1.0\n\nDesarrollado por Daniel Diamon")
                .setPositiveButton("OK", null)
                .show();
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

    private void loadExample() {
        setStatus(getString(R.string.status_loading), false);
        executorService.execute(() -> {
            try {
                final String asmContent = readAssetText("example.asm");
                mainHandler.post(() -> {
                    etSource.setText(asmContent);
                    applySyntaxHighlightingToEditor();
                });
            } catch (final IOException e) {
                mainHandler.post(() -> setStatus("Error al cargar ejemplo", true));
            }
        });
    }

    private void assembleCode() {
        final String source = etSource.getText().toString();
        if (source.trim().isEmpty()) {
            setStatus("El código está vacío", true);
            return;
        }

        setStatus(getString(R.string.status_assembling), false);

        executorService.execute(() -> {
            try {
                registerDefaultIncludes();
                final String result = assembler.assemble(source);

                mainHandler.post(() -> {
                    if (result.startsWith("ERROR")) {
                        setStatus(getString(R.string.status_failed), true);
                        showErrorDialog(result);
                    } else {
                        lastGeneratedHex = result;
                        setStatus(getString(R.string.status_success), false);
                        showHexPopup(result);
                    }
                });
            } catch (final Exception e) {
                mainHandler.post(() -> setStatus("Error crítico: " + e.getMessage(), true));
            }
        });
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

        int width = LinearLayout.LayoutParams.MATCH_PARENT;
        int height = LinearLayout.LayoutParams.MATCH_PARENT;
        final PopupWindow popupWindow = new PopupWindow(popupView, width, height, true);

        TextView tvPopupHex = popupView.findViewById(R.id.tvPopupHex);
        Button btnClose = popupView.findViewById(R.id.btnClosePopup);

        tvPopupHex.setText(colorizeHexOptimized(normalizeHex(hex)));
        btnClose.setOnClickListener(v -> popupWindow.dismiss());

        popupWindow.showAtLocation(findViewById(android.R.id.content), Gravity.CENTER, 0, 0);
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
                applySyntaxHighlightingToEditor();
            }
        });
    }

    private void applySyntaxHighlightingToEditor() {
        if (isApplyingSyntaxHighlight || etSource == null)
            return;
        Editable text = etSource.getText();
        if (text == null)
            return;

        isApplyingSyntaxHighlight = true;
        ForegroundColorSpan[] spans = text.getSpans(0, text.length(), ForegroundColorSpan.class);
        for (ForegroundColorSpan span : spans)
            text.removeSpan(span);

        // Mnemonics & Registers
        highlightRegex(text,
                "(?i)\\b(MOVE|AND|XOR|OR|ADD|SUB|CMP|LD|ST|BCC|BCS|BNE|BEQ|BVC|BVS|BPL|BMI|BGE|BLT|BGT|BLE|BUC|BUS|BHI|BLS|JMP|JSR|POP|PUSH|RET|RETI|TRAP|NOP|SXT|ABS|INV|NEG|CLR|INC|DEC|ADDQ|TEST)\\b",
                ContextCompat.getColor(this, R.color.syntax_mnemonic));
        highlightRegex(text, "(?i)\\b(R0|R1|R2|R3|SP|PS|PC)\\b", ContextCompat.getColor(this, R.color.syntax_mnemonic));

        // Directives
        highlightRegex(text, "(?i)\\b(ORG|EQU|DB|DW|DL|DM|DS|INCLUDE)\\b",
                ContextCompat.getColor(this, R.color.syntax_directive));

        // Labels
        highlightRegex(text, "(?m)^\\s*[A-Za-z_][A-Za-z0-9_]*:", ContextCompat.getColor(this, R.color.syntax_label));

        // Numbers
        highlightRegex(text, "\\b(0x[0-9A-Fa-f]+|\\d+)\\b", ContextCompat.getColor(this, R.color.syntax_number));

        // Comments
        highlightRegex(text, "(;.*$|//.*$)", ContextCompat.getColor(this, R.color.syntax_comment));

        isApplyingSyntaxHighlight = false;
    }

    private void highlightRegex(Editable text, String regex, int color) {
        Pattern pattern = Pattern.compile(regex);
        Matcher matcher = pattern.matcher(text.toString());
        while (matcher.find()) {
            text.setSpan(new ForegroundColorSpan(color), matcher.start(), matcher.end(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
    }

    private void shareProject() {
        String source = etSource.getText().toString();
        String hex = lastGeneratedHex;
        String lst = assembler.getListing();

        if (source.trim().isEmpty()) {
            setStatus("Nada que compartir", true);
            return;
        }

        String payload = "# Megaprocessor Project\n\n## ASM\n" + source + "\n\n## HEX\n" + hex + "\n\n## LST\n" + lst;
        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_SUBJECT, "Proyecto Megaprocessor");
        intent.putExtra(Intent.EXTRA_TEXT, payload);
        startActivity(Intent.createChooser(intent, "Compartir vía"));
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
        final String lstName = "megaprocessor_" + timestamp + ".lst";

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
            values.put(android.provider.MediaStore.Downloads.MIME_TYPE, "text/plain");
            values.put(android.provider.MediaStore.Downloads.RELATIVE_PATH, android.os.Environment.DIRECTORY_DOWNLOADS);
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
            File dir = android.os.Environment
                    .getExternalStoragePublicDirectory(android.os.Environment.DIRECTORY_DOWNLOADS);
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
            if (line.startsWith(":")) {
                builder.setSpan(new ForegroundColorSpan(Color.BLUE), start + 7, start + 16, 0);
                if (line.length() > 11) {
                    builder.setSpan(new ForegroundColorSpan(Color.BLACK), start + 16, start + 7 + line.length() - 2, 0);
                }
                builder.setSpan(new ForegroundColorSpan(Color.parseColor("#008000")), start + 7 + line.length() - 2,
                        start + 7 + line.length(), 0);
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
