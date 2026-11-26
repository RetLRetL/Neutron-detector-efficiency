#include <TFile.h>
#include <TTree.h>
#include <TSystem.h>
#include <TString.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
// SETTINGS
// ============================================================================

const int PE_THICKNESS_CM = 1;
const int EVENTS_PER_RUN = 2000;

const char* EXECUTABLE = "./main";
const char* OUTPUT_FILE = "neutron_spectra.root";

// ============================================================================
// ENERGY PARSER
//
// This macro is intentionally independent of run_scan.C.
// It runs one simulation and saves only the energies printed as:
//
//     detected: <energy in eV>
//
// The C++ setting Settings::OUTPUT_DETECTED_NEUTRON_ENERGIES must be true,
// and the executable must be rebuilt after changing settings.h.
// ============================================================================

void energy_parce()
{
    const TString command = Form(
        "%s %d %d 2>&1",
        EXECUTABLE,
        PE_THICKNESS_CM,
        EVENTS_PER_RUN
    );

    std::cout << "Running: " << command << std::endl;

    const TString output = gSystem->GetFromPipe(command);

    std::vector<double> detectedEnergy;
    Long64_t detectedDecays = -1;

    std::istringstream stream(output.Data());
    std::string line;

    while (std::getline(stream, line))
    {
        // Remove leading whitespace before parsing.
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos)
            continue;
        line.erase(0, first);

        double energyEV = 0.0;
        if (std::sscanf(line.c_str(), "detected: %lf", &energyEV) == 1)
        {
            detectedEnergy.push_back(energyEV);
            continue;
        }

        // This counter is used only to validate the parsed energy count;
        // this macro does not calculate or store the efficiency.
        const std::string key = "Detected decays";
        const std::size_t keyPos = line.find(key);
        if (keyPos != std::string::npos)
        {
            const std::size_t colon = line.find(':', keyPos + key.size());
            if (colon != std::string::npos)
            {
                const std::string value = line.substr(colon + 1);
                char* end = nullptr;
                const long long parsed = std::strtoll(value.c_str(), &end, 10);
                if (end != value.c_str())
                    detectedDecays = static_cast<Long64_t>(parsed);
            }
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "NEUTRON ENERGY PARSING\n";
    std::cout << "========================================\n";
    std::cout << "PE thickness          : " << PE_THICKNESS_CM << " cm\n";
    std::cout << "Events per run        : " << EVENTS_PER_RUN << "\n";
    std::cout << "Detected decays       : " << detectedDecays << "\n";
    std::cout << "Parsed detected energies: " << detectedEnergy.size() << "\n";

    if (detectedDecays < 0)
    {
        std::cerr
            << "ERROR: could not find the 'Detected decays' summary line. "
            << "The simulation may have failed; ROOT output was not created.\n";
        std::cerr << output << std::endl;
        return;
    }

    if (detectedDecays >= 0 &&
        static_cast<Long64_t>(detectedEnergy.size()) != detectedDecays)
    {
        std::cerr
            << "WARNING: parsed energy count differs from DetectedDecays. "
            << "Check Settings::OUTPUT_DETECTED_NEUTRON_ENERGIES in settings.h "
            << "and rebuild with ./compile.sh.\n";
    }

    if (detectedDecays > 0 && detectedEnergy.empty())
    {
        std::cerr
            << "ERROR: no 'detected:' lines were found although detected decays "
            << "were reported. Enable Settings::OUTPUT_DETECTED_NEUTRON_ENERGIES "
            << "in settings.h, rebuild, and run this macro again.\n";
        return;
    }

    // Keep the energy data in its own ROOT file, separate from
    // simulation_results.root created by run_scan.C.
    TFile file(OUTPUT_FILE, "RECREATE");
    if (file.IsZombie())
    {
        std::cerr << "ERROR: cannot create ROOT file " << OUTPUT_FILE << std::endl;
        return;
    }

    Int_t peThicknessCm = PE_THICKNESS_CM;
    Long64_t nEvents = EVENTS_PER_RUN;

    TTree tree("neutrons", "Detected-neutron energy spectrum");
    tree.Branch("PEThickness", &peThicknessCm);
    tree.Branch("NEvents", &nEvents);
    tree.Branch("detected_energy", &detectedEnergy);
    tree.Fill();
    tree.Write();
    file.Close();

    std::cout << "ROOT file             : " << OUTPUT_FILE << "\n";
    std::cout << "========================================\n";
}
