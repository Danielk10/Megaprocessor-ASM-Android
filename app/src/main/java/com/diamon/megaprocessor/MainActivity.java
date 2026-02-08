
package com.diamon.megaprocessor;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    private NativeAssembler assembler;
    private EditText etSource;
    private TextView tvOutput;
    private TextView tvOriginalHex;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        assembler = new NativeAssembler();

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
        try {
            // Load Assembly
            InputStream is = getAssets().open("example.asm");
            int size = is.available();
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            etSource.setText(new String(buffer));

            // Load Original Hex
            InputStream isHex = getAssets().open("example.hex");
            int sizeHex = isHex.available();
            byte[] bufferHex = new byte[sizeHex];
            isHex.read(bufferHex);
            isHex.close();
            tvOriginalHex.setText(new String(bufferHex));

        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(this, "Error loading example from Assets", Toast.LENGTH_SHORT).show();
            // Fallback content if file missing
            etSource.setText("// Example Code missing\n");
            tvOriginalHex.setText("// Original HEX missing\n");
        }
    }

    private void assembleCode() {
        String source = etSource.getText().toString();
        String result = assembler.assemble(source);
        tvOutput.setText(result);
        if (result.startsWith("ERROR")) {
            Toast.makeText(this, "Assembly Failed", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(this, "Assembly Successful", Toast.LENGTH_SHORT).show();
        }
    }

    private void exportFiles() {
        String hex = tvOutput.getText().toString();
        String lst = assembler.getListing();

        if (hex.isEmpty() || hex.startsWith("ERROR")) {
            Toast.makeText(this, "Please assemble successfully first!", Toast.LENGTH_SHORT).show();
            return;
        }

        // Save to App Specific External Storage (No permissions needed)
        saveFile("output.hex", hex);
        saveFile("output.lst", lst);
    }

    private void saveFile(String fileName, String content) {
        File path = getExternalFilesDir(null); // App specific external storage
        if (path == null) {
            Toast.makeText(this, "External storage not available", Toast.LENGTH_SHORT).show();
            return;
        }
        File file = new File(path, fileName);

        try {
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(content.getBytes());
            fos.close();
            Toast.makeText(this, "Saved to " + file.getAbsolutePath(), Toast.LENGTH_LONG).show();
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(this, "Error saving " + fileName, Toast.LENGTH_SHORT).show();
        }
    }
}