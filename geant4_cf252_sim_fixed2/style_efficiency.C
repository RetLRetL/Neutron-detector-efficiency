#include "TFile.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TPaveText.h"
#include "TObject.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

void style_efficiency()
{
    // ========================================================================
    // SETTINGS
    // ========================================================================

    const char* INPUT_FILE =
        "simulation_results.root";

    const char* SOURCE_HISTOGRAM =
        "Efficiency";

    const char* DISPLAY_HISTOGRAM =
        "EfficiencyDisplay";

    const char* CANVAS_NAME =
        "EfficiencyDisplayCanvas";

    const int EVENTS_PER_RUN =
        1000;

    const int IONS_PER_EVENT =
        100;

    // ========================================================================
    // Open ROOT file
    // ========================================================================

    TFile* file =
        TFile::Open(
            INPUT_FILE,
            "UPDATE"
        );

    if (!file || file->IsZombie())
    {
        std::cerr
            << "ERROR: cannot open "
            << INPUT_FILE
            << std::endl;

        return;
    }

    // ========================================================================
    // Read original histogram
    // ========================================================================

    TH1D* source = nullptr;

    file->GetObject(
        SOURCE_HISTOGRAM,
        source
    );

    if (!source)
    {
        std::cerr
            << "ERROR: histogram '"
            << SOURCE_HISTOGRAM
            << "' not found."
            << std::endl;

        file->Close();
        delete file;

        return;
    }

    // ========================================================================
    // Create independent copy
    // ========================================================================

    file->Delete(
        Form(
            "%s;*",
            DISPLAY_HISTOGRAM
        )
    );

    TH1D* histogram =
        dynamic_cast<TH1D*>(
            source->Clone(
                DISPLAY_HISTOGRAM
            )
        );

    if (!histogram)
    {
        std::cerr
            << "ERROR: failed to clone histogram."
            << std::endl;

        file->Close();
        delete file;

        return;
    }

    histogram->SetDirectory(nullptr);

    // ========================================================================
    // Convert efficiency and errors to percent
    // ========================================================================

    const int nBins =
        histogram->GetNbinsX();

    for (int bin = 1;
         bin <= nBins;
         ++bin)
    {
        const double value =
            histogram->GetBinContent(bin);

        const double error =
            histogram->GetBinError(bin);

        histogram->SetBinContent(
            bin,
            100.0 * value
        );

        histogram->SetBinError(
            bin,
            100.0 * error
        );
    }

    // ========================================================================
    // Find maximum
    // ========================================================================

    const int maxBin =
        histogram->GetMaximumBin();

    const double maxEfficiency =
        histogram->GetBinContent(maxBin);

    const double maxError =
        histogram->GetBinError(maxBin);

    const double maxThickness =
        histogram->GetXaxis()->GetBinCenter(maxBin);

    // ========================================================================
    // Histogram appearance
    // ========================================================================

    histogram->SetTitle("");

    // ========================================================================
    // Histogram appearance
    // ========================================================================

    histogram->SetFillColor(kBlue);
    histogram->SetFillStyle(1001);

    // Не рисуем контур самой гистограммы.
    histogram->SetLineColor(kBlue);
    histogram->SetLineWidth(1);

    // Точек нет.
    histogram->SetMarkerStyle(0);
    histogram->SetMarkerSize(0);

    // ========================================================================
    // X axis
    // ========================================================================

    histogram->GetXaxis()->SetTitle(
        "PE thickness [cm]"
    );

    histogram->GetXaxis()->SetTitleSize(
        0.045
    );

    histogram->GetXaxis()->SetLabelSize(
        0.040
    );

    // ========================================================================
    // Y axis
    // ========================================================================

    histogram->GetYaxis()->SetTitle("");

    histogram->GetYaxis()->SetLabelSize(
        0.040
    );

    histogram->SetMinimum(0.0);

    double yMaximum =
        1.35 * maxEfficiency;

    if (yMaximum < 1.0)
        yMaximum = 1.0;

    histogram->SetMaximum(yMaximum);

    // ========================================================================
    // Canvas
    // ========================================================================

    file->Delete(
        Form(
            "%s;*",
            CANVAS_NAME
        )
    );

    TCanvas* canvas =
        new TCanvas(
            CANVAS_NAME,
            "Detection efficiency",
            1100,
            750
        );

    canvas->SetLeftMargin(0.12);
    canvas->SetRightMargin(0.05);
    canvas->SetBottomMargin(0.12);
    canvas->SetTopMargin(0.07);

    // ========================================================================
    // Draw filled histogram
    // ========================================================================

    histogram->Draw("HIST");

    // ========================================================================
    // Thin black separators between bins
    // ========================================================================

    TLine* separator = new TLine();

    separator->SetLineColor(kBlack);
    separator->SetLineWidth(1);
 
    for (int bin = 1; bin < nBins; ++bin)
    {
        const double x =
            histogram->GetBinLowEdge(bin + 1);

        const double yLeft =
            histogram->GetBinContent(bin);

        const double yRight =
            histogram->GetBinContent(bin + 1);

        const double yTop =
            std::max(yLeft, yRight);

        separator->DrawLine(
            x,
            0.0,
            x,
            yTop
        );
    }

    // ========================================================================
    // Draw error bars
    // ========================================================================

    histogram->SetLineColor(kBlack);
    histogram->SetLineWidth(3);

    histogram->SetMarkerStyle(0);
    histogram->SetMarkerSize(0);

    histogram->Draw("E1 SAME");

    // ========================================================================
    // Y-axis label
    //
    // Horizontal text, to the left of the Y axis,
    // close to its upper end.
    // ========================================================================

    TLatex yAxisLabel;

    yAxisLabel.SetNDC(true);
    yAxisLabel.SetTextFont(42);
    yAxisLabel.SetTextSize(0.040);
    yAxisLabel.SetTextAlign(13);

    yAxisLabel.DrawLatex(
        0.025,
        0.970,
        "Efficiency [%]"
    );

    // ========================================================================
    // Information text
    //
    // Moved lower and slightly to the left.
    // The frame is placed a little to the right of the text.
    // ========================================================================

    TLatex info;

    info.SetNDC(true);
    info.SetTextFont(42);
    info.SetTextSize(0.042);
    info.SetTextAlign(12);

    // ------------------------------------------------------------
    // Text lines
    // ------------------------------------------------------------

    std::ostringstream runInfo;

    runInfo
        << EVENTS_PER_RUN
        << " events/run, "
        << IONS_PER_EVENT
        << " ions/event";

    std::ostringstream thicknessInfo;

    thicknessInfo
        << "PE thickness: "
        << std::fixed
        << std::setprecision(0)
        << maxThickness
        << " cm";

    std::ostringstream efficiencyInfo;

    efficiencyInfo
        << "Maximum efficiency: "
        << std::fixed
        << std::setprecision(2)
        << maxEfficiency
        << " #pm "
        << maxError
        << " %";

    // ------------------------------------------------------------
    // Draw text.
    //
    // These coordinates are slightly left of the frame.
    // ------------------------------------------------------------

    info.DrawLatex(
        0.525,
        0.835,
        runInfo.str().c_str()
    );

    info.DrawLatex(
        0.525,
        0.765,
        thicknessInfo.str().c_str()
    );

    info.DrawLatex(
        0.525,
        0.710,
        efficiencyInfo.str().c_str()
    );

    // ========================================================================
    // Frame around the information block
    //
    // Slightly shifted to the right relative to the text.
    // ========================================================================

    TPaveText* infoFrame =
        new TPaveText(
            0.520,
            0.675,
            0.920,
            0.875,
            "NDC"
        );

    infoFrame->SetFillStyle(0);
    infoFrame->SetBorderSize(1);
    infoFrame->SetLineWidth(1);

    // No text inside: the text is drawn separately.
    infoFrame->Draw();

    // ========================================================================
    // Save display histogram and canvas
    //
    // Original Efficiency remains untouched.
    // ========================================================================

    file->cd();

    histogram->Write(
        DISPLAY_HISTOGRAM,
        TObject::kOverwrite
    ); 

    canvas->Write(
        CANVAS_NAME,
        TObject::kOverwrite
    );

    file->Close();

    delete file;

    std::cout
        << "Styled efficiency saved to "
        << INPUT_FILE
        << std::endl;

    std::cout
        << "Original histogram '"
        << SOURCE_HISTOGRAM
        << "' was not modified."
        << std::endl;
}