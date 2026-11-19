#include <TFile.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <iostream>
#include <algorithm>

void difference()
{
    // Открываем файлы
    TFile *file1 = TFile::Open("geantonly.root", "READ");
    TFile *file2 = TFile::Open("freya.root", "READ");

    if (!file1 || file1->IsZombie() ||
        !file2 || file2->IsZombie()) {
        std::cerr << "Error opening ROOT files" << std::endl;
        return;
    }

    // Получаем гистограммы
    TH1 *source1 = nullptr;
    TH1 *source2 = nullptr;

    file1->GetObject("Efficiency", source1);
    file2->GetObject("Efficiency", source2);

    if (!source1 || !source2) {
        std::cerr << "Histogram not found" << std::endl;
        return;
    }

    // Клонируем гистограммы, чтобы отвязать их от файлов
    TH1 *hist1 = (TH1*)source1->Clone("hist_geant");
    TH1 *hist2 = (TH1*)source2->Clone("hist_freya");

    hist1->SetDirectory(nullptr);
    hist2->SetDirectory(nullptr);

    file1->Close();
    file2->Close();

    // Создаём холст
    TCanvas *canvas = new TCanvas("canvas", "Efficiency comparison", 900, 700);

    // Маркеры и их цвета
    hist1->SetLineColor(kBlue);
    hist1->SetMarkerColor(kBlue);

    hist2->SetLineColor(kRed);
    hist2->SetMarkerColor(kRed);

    hist1->SetMarkerStyle(20);
    hist2->SetMarkerStyle(21);

    hist1->SetMarkerSize(0.8);
    hist2->SetMarkerSize(0.8);

    // Сначала рисуем контуры
    hist1->Draw("HIST");
    hist2->Draw("HIST SAME");

    // Затем погрешности
    hist1->Draw("E1 SAME");
    hist2->Draw("E1 SAME");

    // Легенда
    TLegend *legend = new TLegend(0.55, 0.75, 0.88, 0.88);

    legend->AddEntry(hist1, "With Geant4 Cf-decay", "lep");
    legend->AddEntry(hist2, "With LLNL Cf-decay", "lep");

    legend->Draw();

    canvas->SetLogy();

    canvas->Update();
}