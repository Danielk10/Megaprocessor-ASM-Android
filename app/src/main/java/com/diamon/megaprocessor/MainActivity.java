package com.diamon.megaprocessor;

import androidx.appcompat.app.AppCompatActivity;
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
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.graphics.Color;

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

public class MainActivity extends AppCompatActivity {

    private static final int REQUEST_WRITE_EXTERNAL_STORAGE = 1001;

    private NativeAssembler assembler;
    private EditText etSource;
    private TextView tvOutput;
    private TextView tvOriginalHex;
    private TextView tvStatus;
    private ExecutorService executorService;
    private Handler mainHandler;
    private Button btnAssemble;
    private Button btnLoad;
    private Button btnExport;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        assembler = new NativeAssembler();
        executorService = Executors.newSingleThreadExecutor();
        mainHandler = new Handler(Looper.getMainLooper());

        etSource = findViewById(R.id.etSourceCode);
        tvOutput = findViewById(R.id.tvOutput);
        tvOriginalHex = findViewById(R.id.tvOriginalHex);
        tvStatus = findViewById(R.id.tvStatus);
        btnLoad = findViewById(R.id.btnLoadExample);
        btnAssemble = findViewById(R.id.btnAssemble);
        btnExport = findViewById(R.id.btnExport);

        registerDefaultIncludes();

        btnLoad.setOnClickListener(v -> loadExample());
        btnAssemble.setOnClickListener(v -> assembleCode());
        btnExport.setOnClickListener(v -> exportFiles());
    }

    private void registerDefaultIncludes() {
        try {
            String defs = readAssetText("Megaprocessor_defs.asm");
            assembler.registerIncludeFile("Megaprocessor_defs.asm", defs);
        } catch (IOException e) {
            setStatus("Warning: unable to load Megaprocessor_defs.asm include", true);
        }
    }

    private String readAssetText(String assetName) throws IOException {
        try (BufferedInputStream bis = new BufferedInputStream(getAssets().open(assetName))) {
            byte[] buffer = new byte[bis.available()];
            int read = bis.read(buffer);
            if (read < 0) {
                return "";
            }
            return new String(buffer, 0, read);
        }
    }

    private void setStatus(final String message, final boolean isError) {
        mainHandler.post(() -> {
            if (tvStatus != null) {
                tvStatus.setText(message);
                tvStatus.setTextColor(isError ? Color.RED : Color.parseColor("#008000"));
                tvStatus.setVisibility(View.VISIBLE);

                mainHandler.postDelayed(() -> {
                    if (tvStatus != null) {
                        tvStatus.setVisibility(View.GONE);
                    }
                }, 3000);
            }
        });
    }

    private void setButtonsEnabled(final boolean enabled) {
        mainHandler.post(() -> {
            if (btnLoad != null) btnLoad.setEnabled(enabled);
            if (btnAssemble != null) btnAssemble.setEnabled(enabled);
            if (btnExport != null) btnExport.setEnabled(enabled);
        });
    }

    private void loadExample() {
        setButtonsEnabled(false);
        setStatus("Loading example...", false);

        executorService.execute(() -> {
            try {
                final String asmContent = readAssetText("example.asm");
                final String hexContent = readAssetText("example.hex");
                final String normalizedReference = normalizeHex(hexContent);
                final SpannableStringBuilder colorizedReference = colorizeHexOptimized(normalizedReference);

                mainHandler.post(() -> {
                    etSource.setText(asmContent);
                    tvOriginalHex.setText(colorizedReference);
                    tvOutput.setText("Generated Output...");
                    setStatus("Example loaded successfully", false);
                    setButtonsEnabled(true);
                });
            } catch (final IOException e) {
                e.printStackTrace();
                mainHandler.post(() -> {
                    setStatus("Error: " + e.getMessage(), true);
                    etSource.setText("// Example Code missing\n");
                    tvOriginalHex.setText("// Original HEX missing\n");
                    setButtonsEnabled(true);
                });
            }
        });
    }

    private void assembleCode() {
        final String source = etSource.getText().toString();

        if (source.trim().isEmpty()) {
            setStatus("Error: Source code is empty", true);
            return;
        }

        setButtonsEnabled(false);
        setStatus("Assembling...", false);

        executorService.execute(() -> {
            try {
                registerDefaultIncludes();
                final String result = assembler.assemble(source);

                if (result.startsWith("ERROR")) {
                    mainHandler.post(() -> {
                        tvOutput.setText(result);
                        setStatus("Assembly failed", true);
                        setButtonsEnabled(true);
                    });
                } else {
                    final String normalizedGenerated = normalizeHex(result);
                    final SpannableStringBuilder colorized = colorizeHexOptimized(normalizedGenerated);
                    final String referenceHex = tvOriginalHex.getText().toString();
                    final boolean isMatch = compareHexSemantically(normalizedGenerated, referenceHex);

                    mainHandler.post(() -> {
                        tvOutput.setText(colorized);
                        setStatus(isMatch ? "Assembly successful: HEX MATCH" : "Assembly successful: HEX MISMATCH", !isMatch);
                        setButtonsEnabled(true);
                    });
                }
            } catch (final Exception e) {
                e.printStackTrace();
                mainHandler.post(() -> {
                    setStatus("Assembly error: " + e.getMessage(), true);
                    setButtonsEnabled(true);
                });
            }
        });
    }

    private void exportFiles() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                        != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                    this,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_WRITE_EXTERNAL_STORAGE
            );
            return;
        }
        exportFilesInternal();
    }

    private void exportFilesInternal() {
        final String hex = tvOutput.getText().toString();

        if (hex.isEmpty() || hex.startsWith("ERROR")) {
            setStatus("Error: Please assemble successfully first!", true);
            return;
        }

        final String lst = assembler.getListing();
        final String timestamp = String.valueOf(System.currentTimeMillis());
        final String hexName = String.format(Locale.US, "megaprocessor_%s.hex", timestamp);
        final String lstName = String.format(Locale.US, "megaprocessor_%s.lst", timestamp);

        setButtonsEnabled(false);
        setStatus("Exporting files to Downloads...", false);

        executorService.execute(() -> {
            boolean hexSaved = saveFileToDownloads(hexName, hex);
            boolean lstSaved = saveFileToDownloads(lstName, lst);

            final boolean success = hexSaved && lstSaved;
            mainHandler.post(() -> {
                if (success) {
                    setStatus("Files saved in Downloads: " + hexName + " / " + lstName, false);
                } else {
                    setStatus("Error: Failed to save files in Downloads", true);
                }
                setButtonsEnabled(true);
            });
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
            if (uri == null) {
                return false;
            }

            try (OutputStream os = getContentResolver().openOutputStream(uri)) {
                if (os == null) {
                    return false;
                }
                os.write(content.getBytes(StandardCharsets.UTF_8));
                os.flush();
            } catch (IOException e) {
                e.printStackTrace();
                getContentResolver().delete(uri, null, null);
                return false;
            }

            ContentValues done = new ContentValues();
            done.put(android.provider.MediaStore.Downloads.IS_PENDING, 0);
            getContentResolver().update(uri, done, null, null);
            return true;
        }

        @SuppressWarnings("deprecation")
        File downloadsDir = android.os.Environment.getExternalStoragePublicDirectory(android.os.Environment.DIRECTORY_DOWNLOADS);
        if (downloadsDir == null) {
            return false;
        }
        if (!downloadsDir.exists() && !downloadsDir.mkdirs()) {
            return false;
        }

        File file = new File(downloadsDir, fileName);
        try (BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(file))) {
            bos.write(content.getBytes(StandardCharsets.UTF_8));
            bos.flush();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == REQUEST_WRITE_EXTERNAL_STORAGE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                exportFilesInternal();
            } else {
                setStatus("Permission denied: cannot write into Downloads", true);
            }
        }
    }

    private String normalizeHex(String hexContent) {
        if (hexContent == null) return "";
        Map<Integer, Byte> image = parseHexImage(hexContent);
        if (image.isEmpty()) return hexContent.trim();

        List<Integer> addresses = new ArrayList<>(image.keySet());
        Collections.sort(addresses);

        StringBuilder normalized = new StringBuilder();
        final int recordSize = 0x20;
        int index = 0;
        while (index < addresses.size()) {
            int start = addresses.get(index);
            List<Byte> data = new ArrayList<>();
            data.add(image.get(start));
            index++;

            while (index < addresses.size() && data.size() < recordSize) {
                int expected = start + data.size();
                if (addresses.get(index) != expected) break;
                data.add(image.get(addresses.get(index)));
                index++;
            }

            normalized.append(formatRecord(start, data));
            normalized.append("\n");
        }
        normalized.append(":00000001FF\n");
        return normalized.toString();
    }

    private String formatRecord(int address, List<Byte> data) {
        int checksum = data.size() + ((address >> 8) & 0xFF) + (address & 0xFF);
        StringBuilder line = new StringBuilder();
        line.append(":");
        line.append(String.format("%02X", data.size()));
        line.append(String.format("%04X", address & 0xFFFF));
        line.append("00");
        for (byte b : data) {
            int unsigned = b & 0xFF;
            checksum += unsigned;
            line.append(String.format("%02X", unsigned));
        }
        int cs = ((~checksum + 1) & 0xFF);
        line.append(String.format("%02X", cs));
        return line.toString();
    }

    private Map<Integer, Byte> parseHexImage(String hexContent) {
        Map<Integer, Byte> image = new LinkedHashMap<>();
        if (hexContent == null || hexContent.trim().isEmpty()) return image;

        String[] lines = hexContent.split("\\r?\\n");
        for (String raw : lines) {
            String line = raw.trim();
            if (!line.startsWith(":")) continue;
            if (line.length() < 11) continue;

            try {
                int count = Integer.parseInt(line.substring(1, 3), 16);
                int address = Integer.parseInt(line.substring(3, 7), 16);
                int type = Integer.parseInt(line.substring(7, 9), 16);
                if (type != 0x00) continue;
                int expectedLength = 11 + (count * 2);
                if (line.length() < expectedLength) continue;
                for (int i = 0; i < count; i++) {
                    int bytePos = 9 + (i * 2);
                    int value = Integer.parseInt(line.substring(bytePos, bytePos + 2), 16);
                    image.put(address + i, (byte) value);
                }
            } catch (Exception ignored) {
            }
        }
        return image;
    }

    private boolean compareHexSemantically(String generated, String reference) {
        Map<Integer, Byte> genImage = parseHexImage(generated);
        Map<Integer, Byte> refImage = parseHexImage(reference);
        return genImage.equals(refImage);
    }

    private SpannableStringBuilder colorizeHexOptimized(String hexContent) {
        if (hexContent == null || hexContent.isEmpty()) {
            return new SpannableStringBuilder();
        }

        SpannableStringBuilder builder = new SpannableStringBuilder();
        String[] lines = hexContent.split("\n");

        final int BLUE = Color.BLUE;
        final int BLACK = Color.BLACK;
        final int GREEN = Color.parseColor("#008000");
        final int GRAY = Color.GRAY;

        for (String line : lines) {
            if (line.isEmpty()) continue;

            int start = builder.length();
            builder.append(line);
            builder.append("\n");
            int len = line.length();

            if (line.startsWith(":") && len >= 11) {
                builder.setSpan(new ForegroundColorSpan(BLUE), start, start + 9, 0);

                if (len > 11) {
                    builder.setSpan(new ForegroundColorSpan(BLACK), start + 9, start + len - 2, 0);
                }

                builder.setSpan(new ForegroundColorSpan(GREEN), start + len - 2, start + len, 0);
            } else if (line.startsWith(":")) {
                builder.setSpan(new ForegroundColorSpan(GRAY), start, start + len, 0);
            }
        }

        return builder;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (executorService != null && !executorService.isShutdown()) {
            executorService.shutdown();
        }
    }
}
