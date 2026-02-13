package com.diamon.guia;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

public class ProjectInfoActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_project_info);

        Toolbar toolbar = findViewById(R.id.toolbar_project_info);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }

        setupLink(R.id.linkOfficialSite, "https://www.megaprocessor.com/");
        setupLink(R.id.linkInstructionSet, "https://www.megaprocessor.com/instruction_set.html");
        setupLink(R.id.linkWebSimulator, "https://www.megaprocessor.com/simulator.html");
        setupLink(R.id.linkGitHubProject, "https://github.com/Danielk10/Megaprocessor-ASM-Android");
    }

    private void setupLink(int viewId, String url) {
        TextView link = findViewById(viewId);
        link.setOnClickListener(v -> startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url))));
    }

    @Override
    public boolean onSupportNavigateUp() {
        getOnBackPressedDispatcher().onBackPressed();
        return true;
    }
}
