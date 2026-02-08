package com.diamon.megaprocessor;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;
import com.diamon.megaprocessor.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Instanciar el ensamblador nativo
        NativeAssembler assembler = new NativeAssembler();

        // Código de prueba
        String assemblyCode = "START: MOV R1, R0\n" +
                "       ADD R1, R2\n" +
                "       NOP\n" +
                "       BNE START\n";

        // Llamar al método nativo
        String result = assembler.assemble(assemblyCode);

        // Mostrar resultado
        TextView tv = binding.sampleText;
        tv.setText("Ensamblado:\n" + result);
    }
}