package com.diamon.megaprocessor;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.diamon.megaprocessor.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'megaprocessor' library on application startup.
    static {
        System.loadLibrary("megaprocessor");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
    }

    /**
     * A native method that is implemented by the 'megaprocessor' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}