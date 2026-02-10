package com.diamon.megaprocessor;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

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
    private ExecutorService executorService;
    private Handler mainHandler;

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
        Button btnLoad = findViewById(R.id.btnLoadExample);
        Button btnAssemble = findViewById(R.id.btnAssemble);
        Button btnExport = findViewById(R.id.btnExport);

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

    private void loadExample() {
        executorService.execute(new Runnable() {
            @Override
            public void run() {
                try {
                    // Load Assembly
                    InputStream is = getAssets().open("example.asm");
                    int size = is.available();
                    byte[] buffer = new byte[size];
                    is.read(buffer);
                    is.close();
                    final String asmContent = new String(buffer);

                    // Load Original Hex
                    InputStream isHex = getAssets().open("example.hex");
                    int sizeHex = isHex.available();
                    byte[] bufferHex = new byte[sizeHex];
                    isHex.read(bufferHex);
                    isHex.close();
                    final String hexContent = new String(bufferHex);
                    
                    // Colorize in background
                    final android.text.SpannableStringBuilder colorized = colorizeHex(hexContent);

                    // Update UI on main thread
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            etSource.setText(asmContent);
                            tvOriginalHex.setText(colorized);
                        }
                    });

                } catch (IOException e) {
                    e.printStackTrace();
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(MainActivity.this, "Error loading example from Assets", Toast.LENGTH_SHORT).show();
                            etSource.setText("// Example Code missing\n");
                            tvOriginalHex.setText("// Original HEX missing\n");
                        }
                    });
                }
            }
        });
    }

    private void assembleCode() {
        final String source = etSource.getText().toString();
        
        executorService.execute(new Runnable() {
            @Override
            public void run() {
                final String result = assembler.assemble(source);

                if (result.startsWith("ERROR")) {
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            tvOutput.setText(result);
                            Toast.makeText(MainActivity.this, "Assembly Failed", Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    // Colorize in background
                    final android.text.SpannableStringBuilder colorized = colorizeHex(result);
                    
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            tvOutput.setText(colorized);
                            Toast.makeText(MainActivity.this, "Assembly Successful", Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            }
        });
    }

    private void exportFiles() {
        final String hex = tvOutput.getText().toString();
        final String lst = assembler.getListing();

        if (hex.isEmpty() || hex.startsWith("ERROR")) {
            Toast.makeText(this, "Please assemble successfully first!", Toast.LENGTH_SHORT).show();
            return;
        }

        executorService.execute(new Runnable() {
            @Override
            public void run() {
                // Save to App Specific External Storage (No permissions needed)
                saveFile("output.hex", hex);
                saveFile("output.lst", lst);
            }
        });
    }

    private void saveFile(String fileName, String content) {
        File path = getExternalFilesDir(null); // App specific external storage
        if (path == null) {
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(MainActivity.this, "External storage not available", Toast.LENGTH_SHORT).show();
                }
            });
            return;
        }
        File file = new File(path, fileName);

        try {
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(content.getBytes());
            fos.close();
            final String successMsg = "Saved to " + file.getAbsolutePath();
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(MainActivity.this, successMsg, Toast.LENGTH_LONG).show();
                }
            });
        } catch (IOException e) {
            e.printStackTrace();
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(MainActivity.this, "Error saving " + fileName, Toast.LENGTH_SHORT).show();
                }
            });
        }
    }

    // Helper for Notepad++ style syntax highlighting
    private android.text.SpannableStringBuilder colorizeHex(String hexContent) {
        android.text.SpannableStringBuilder builder = new android.text.SpannableStringBuilder();
        String[] lines = hexContent.split("\n");

        for (String line : lines) {
            if (line.isEmpty())
                continue;
            int start = builder.length();
            builder.append(line);
            builder.append("\n"); // Restore newline

            if (line.startsWith(":")) {
                int len = line.length();
                // Standard Intel HEX: :LLAAAATT[DD...]CC
                // Min length: :LLAAAATTCC (1+2+4+2+2 = 11 chars)

                if (len >= 11) {
                    // :LLAAAATT (Prefix) -> Blue (Indices 0 to 9)
                    // This covers : (1), Length (2), Address (4), Type (2) = 9 chars total
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.BLUE),
                            start, start + 9, 0);

                    // Data -> Black (From index 9 up to checksum)
                    // Only apply if there is data (len > 11)
                    if (len > 11) {
                        builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.BLACK),
                                start + 9, start + len - 2, 0);
                    }

                    // CC (Checksum) -> Green (Last 2 chars)
                    builder.setSpan(
                            new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#008000")),
                            start + len - 2, start + len, 0);
                } else {
                    // Invalid/Short line - just color it gray or black to avoid crash
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.GRAY),
                            start, start + len, 0);
                }
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