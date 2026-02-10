package com.diamon.megaprocessor;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.text.SpannableStringBuilder;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.graphics.Color;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends AppCompatActivity {

    private NativeAssembler assembler;
    private EditText etSource;
    private TextView tvOutput;
    private TextView tvOriginalHex;
    private TextView tvStatus; // Status indicator instead of Toast
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

        btnLoad.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                loadExample();
            }
        });

        btnAssemble.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                assembleCode();
            }
        });

        btnExport.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                exportFiles();
            }
        });
    }

    private void setStatus(final String message, final boolean isError) {
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (tvStatus != null) {
                    tvStatus.setText(message);
                    tvStatus.setTextColor(isError ? Color.RED : Color.parseColor("#008000"));
                    tvStatus.setVisibility(View.VISIBLE);
                    
                    // Auto-hide after 3 seconds
                    mainHandler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            if (tvStatus != null) {
                                tvStatus.setVisibility(View.GONE);
                            }
                        }
                    }, 3000);
                }
            }
        });
    }

    private void setButtonsEnabled(final boolean enabled) {
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (btnLoad != null) btnLoad.setEnabled(enabled);
                if (btnAssemble != null) btnAssemble.setEnabled(enabled);
                if (btnExport != null) btnExport.setEnabled(enabled);
            }
        });
    }

    private void loadExample() {
        setButtonsEnabled(false);
        setStatus("Loading example...", false);
        
        executorService.execute(new Runnable() {
            @Override
            public void run() {
                BufferedInputStream bis = null;
                BufferedInputStream bisHex = null;
                
                try {
                    // Load Assembly with buffering
                    bis = new BufferedInputStream(getAssets().open("example.asm"));
                    int size = bis.available();
                    byte[] buffer = new byte[size];
                    bis.read(buffer);
                    final String asmContent = new String(buffer);

                    // Load Original Hex with buffering
                    bisHex = new BufferedInputStream(getAssets().open("example.hex"));
                    int sizeHex = bisHex.available();
                    byte[] bufferHex = new byte[sizeHex];
                    bisHex.read(bufferHex);
                    final String hexContent = new String(bufferHex);
                    
                    // Colorize in background (optimized)
                    final SpannableStringBuilder colorized = colorizeHexOptimized(hexContent);

                    // Update UI on main thread
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                etSource.setText(asmContent);
                                tvOriginalHex.setText(colorized);
                                setStatus("Example loaded successfully", false);
                            } catch (Exception e) {
                                setStatus("Error displaying example", true);
                            } finally {
                                setButtonsEnabled(true);
                            }
                        }
                    });

                } catch (final IOException e) {
                    e.printStackTrace();
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            setStatus("Error: " + e.getMessage(), true);
                            etSource.setText("// Example Code missing\n");
                            tvOriginalHex.setText("// Original HEX missing\n");
                            setButtonsEnabled(true);
                        }
                    });
                } finally {
                    try {
                        if (bis != null) bis.close();
                        if (bisHex != null) bisHex.close();
                    } catch (IOException ignored) {}
                }
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
        
        executorService.execute(new Runnable() {
            @Override
            public void run() {
                try {
                    final String result = assembler.assemble(source);

                    if (result.startsWith("ERROR")) {
                        mainHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                tvOutput.setText(result);
                                setStatus("Assembly failed", true);
                                setButtonsEnabled(true);
                            }
                        });
                    } else {
                        // Colorize in background (optimized)
                        final SpannableStringBuilder colorized = colorizeHexOptimized(result);
                        
                        mainHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                tvOutput.setText(colorized);
                                setStatus("Assembly successful", false);
                                setButtonsEnabled(true);
                            }
                        });
                    }
                } catch (final Exception e) {
                    e.printStackTrace();
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            setStatus("Assembly error: " + e.getMessage(), true);
                            setButtonsEnabled(true);
                        }
                    });
                }
            }
        });
    }

    private void exportFiles() {
        final String hex = tvOutput.getText().toString();
        
        if (hex.isEmpty() || hex.startsWith("ERROR")) {
            setStatus("Error: Please assemble successfully first!", true);
            return;
        }
        
        final String lst = assembler.getListing();
        setButtonsEnabled(false);
        setStatus("Exporting files...", false);

        executorService.execute(new Runnable() {
            @Override
            public void run() {
                // Save to App Specific External Storage (No permissions needed)
                boolean hexSaved = saveFile("output.hex", hex);
                boolean lstSaved = saveFile("output.lst", lst);
                
                final boolean success = hexSaved && lstSaved;
                mainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (success) {
                            File path = getExternalFilesDir(null);
                            setStatus("Files saved to: " + (path != null ? path.getAbsolutePath() : "storage"), false);
                        } else {
                            setStatus("Error: Failed to save files", true);
                        }
                        setButtonsEnabled(true);
                    }
                });
            }
        });
    }

    private boolean saveFile(String fileName, String content) {
        File path = getExternalFilesDir(null);
        if (path == null) {
            return false;
        }
        
        File file = new File(path, fileName);
        BufferedOutputStream bos = null;
        
        try {
            bos = new BufferedOutputStream(new FileOutputStream(file));
            bos.write(content.getBytes());
            bos.flush();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        } finally {
            try {
                if (bos != null) bos.close();
            } catch (IOException ignored) {}
        }
    }

    // Optimized version with reduced allocations
    private SpannableStringBuilder colorizeHexOptimized(String hexContent) {
        if (hexContent == null || hexContent.isEmpty()) {
            return new SpannableStringBuilder();
        }
        
        SpannableStringBuilder builder = new SpannableStringBuilder();
        String[] lines = hexContent.split("\n");
        
        // Pre-create color spans to reuse
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
                // :LLAAAATT (Prefix) -> Blue (0 to 9)
                builder.setSpan(new ForegroundColorSpan(BLUE), start, start + 9, 0);

                // Data -> Black (9 to len-2)
                if (len > 11) {
                    builder.setSpan(new ForegroundColorSpan(BLACK), start + 9, start + len - 2, 0);
                }

                // Checksum -> Green (last 2 chars)
                builder.setSpan(new ForegroundColorSpan(GREEN), start + len - 2, start + len, 0);
            } else if (line.startsWith(":")) {
                // Invalid/Short line
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