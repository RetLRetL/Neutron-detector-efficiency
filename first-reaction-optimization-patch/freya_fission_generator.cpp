#include "freya_fission_generator.h"

#include "fissionEvent.h"
#include "Randomize.hh"
#include "G4Exception.hh"
#include "G4ios.hh"

#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>

namespace
{
    constexpr int kCf252ZA = 98252;
    constexpr int kSpontaneousFission = 0;
    constexpr std::size_t kFreyaErrorBufferSize = 10000;

    // FREYA's discrete level-lifetime data are tabulated in seconds. The API
    // documents get*Age() as an emission age but does not restate the unit;
    // keep the conversion isolated here in case the local 2.0.5 build uses a
    // different convention. The current implementation interprets it as s.
    constexpr double kFreyaAgeToSeconds = 1.0;

    std::once_flag gFreyaInitializationFlag;

    // FREYA accepts a plain double (*)() callback. Using Geant4's engine
    // keeps FREYA sampling in the same random stream as the rest of Geant4.
    double Geant4RandomNumber()
    {
        return G4UniformRand();
    }

    [[noreturn]] void FatalFreyaError(const std::string& message)
    {
        G4Exception(
            "FreyaFissionGenerator",
            "FREYA001",
            FatalException,
            message.c_str()
        );

        // FatalException normally terminates the run. Keep a hard fallback
        // in case a custom Geant4 exception handler returns from G4Exception.
        std::abort();
    }

    std::string ResolveFreyaDataPath()
    {
        // Respect an explicitly configured path first.
        if (const char* configuredPath = std::getenv("FREYADATAPATH"))
        {
            if (*configuredPath != '\0')
            {
                const std::filesystem::path path(configuredPath);
                if (!std::filesystem::exists(path / "react.dat"))
                {
                    FatalFreyaError(
                        "FREYADATAPATH is set, but react.dat was not found in: "
                        + path.string()
                    );
                }
                return path.string();
            }
        }

        // Default to the installation layout used in this project.
        // Users with another installation can override it by exporting
        // FREYADATAPATH before starting the program.
        const char* home = std::getenv("HOME");
        if (!home || *home == '\0')
        {
            FatalFreyaError(
                "HOME is not set and FREYADATAPATH is not configured."
            );
        }

        std::filesystem::path freyaRoot =
            std::filesystem::path(home) / "llnl" / "fission_v2.0.5";

        // FREYA_ROOT can also be supplied at runtime when the installation
        // does not use the default path. FREYADATAPATH remains the most
        // direct override and takes precedence above.
        if (const char* configuredRoot = std::getenv("FREYA_ROOT"))
        {
            if (*configuredRoot != '\0')
                freyaRoot = configuredRoot;
        }

        const std::filesystem::path defaultPath =
            freyaRoot / "data_freya";

        if (!std::filesystem::exists(defaultPath / "react.dat"))
        {
            FatalFreyaError(
                "FREYA data were not found. Expected react.dat in: "
                + defaultPath.string()
                + ". Set FREYADATAPATH to the directory containing FREYA data."
            );
        }

        return defaultPath.string();
    }

    void CheckFreyaErrors(fissionEvent& fission)
    {
        std::array<char, kFreyaErrorBufferSize> errorBuffer{};
        int errorLength = static_cast<int>(errorBuffer.size());

        fission.getFREYAerrors(&errorLength, errorBuffer.data());

        if (errorLength > 1)
        {
            const std::size_t messageLength =
                ::strnlen(errorBuffer.data(), errorBuffer.size());

            const std::string message(
                errorBuffer.data(),
                messageLength
            );

            FatalFreyaError(
                "The FREYA library reported an error while generating "
                "Cf-252 fission:\n" + message
            );
        }
    }

    FreyaParticleData MakeParticle(
        FreyaParticleType type,
        double energyMeV,
        double directionX,
        double directionY,
        double directionZ,
        double emissionAgeSeconds
    )
    {
        if (!std::isfinite(energyMeV) || energyMeV <= 0.0)
        {
            FatalFreyaError(
                "FREYA returned a non-positive or non-finite particle energy."
            );
        }

        const double directionNorm = std::sqrt(
            directionX * directionX
            + directionY * directionY
            + directionZ * directionZ
        );

        if (!std::isfinite(directionNorm) || directionNorm <= 0.0)
        {
            FatalFreyaError(
                "FREYA returned an invalid particle direction."
            );
        }

        if (!std::isfinite(emissionAgeSeconds) || emissionAgeSeconds < 0.0)
        {
            FatalFreyaError(
                "FREYA returned an invalid particle emission age."
            );
        }

        FreyaParticleData particle;
        particle.type = type;
        particle.kineticEnergyMeV = energyMeV;
        particle.direction = {{
            directionX / directionNorm,
            directionY / directionNorm,
            directionZ / directionNorm
        }};
        particle.emissionTimeSeconds = emissionAgeSeconds;

        return particle;
    }
}

namespace FreyaFissionGenerator
{
    void Initialize()
    {
        std::call_once(
            gFreyaInitializationFlag,
            []()
            {
                const std::string dataPath = ResolveFreyaDataPath();

                // FREYA reads its data directory from this environment
                // variable. Set it before the first fissionEvent is created.
                if (setenv("FREYADATAPATH", dataPath.c_str(), 1) != 0)
                {
                    FatalFreyaError(
                        "Could not set the FREYADATAPATH environment variable."
                    );
                }

                // Option 3 is the full FREYA event-by-event mode: neutron and
                // photon multiplicities and energies are correlated.
                fissionEvent::setCorrelationOption(3);

                // Use Geant4's RNG, rather than FREYA's independent srand48
                // stream, so Geant4 run seeds control both generators.
                fissionEvent::setRNGd(&Geant4RandomNumber);

                G4cout << "FREYA data path : " << dataPath << G4endl;
                G4cout << "FREYA mode      : correlated event-by-event (option 3)"
                       << G4endl;
            }
        );
    }

    FreyaFissionResult GenerateCf252()
    {
        Initialize();

        // The isotope is given as ZA = 98*1000 + 252.
        // fissiontype = 0 means spontaneous fission.
        // In FREYA mode the nubar and incident-energy arguments are not used;
        // FREYA determines the neutron and photon multiplicities itself.
        constexpr double fissionTimeSeconds = 0.0;
        constexpr double unusedNubar = 0.0;
        constexpr double unusedIncidentEnergyMeV = 0.0;

        fissionEvent fission(
            kCf252ZA,
            fissionTimeSeconds,
            unusedNubar,
            unusedIncidentEnergyMeV,
            kSpontaneousFission
        );

        CheckFreyaErrors(fission);

        FreyaFissionResult result;
        result.neutronMultiplicity = fission.getNeutronNu();
        result.photonMultiplicity = fission.getPhotonNu();

        if (result.neutronMultiplicity < 0 || result.photonMultiplicity < 0)
        {
            FatalFreyaError(
                "FREYA did not return valid neutron/photon multiplicities "
                "for spontaneous Cf-252 fission. Check FREYADATAPATH and "
                "the installed FREYA data files."
            );
        }

        result.particles.reserve(
            static_cast<std::size_t>(result.neutronMultiplicity)
            + static_cast<std::size_t>(result.photonMultiplicity)
        );

        // Copy all neutron data before the next fissionEvent is generated.
        for (int i = 0; i < result.neutronMultiplicity; ++i)
        {
            result.particles.push_back(
                MakeParticle(
                    FreyaParticleType::Neutron,
                    fission.getNeutronEnergy(i),
                    fission.getNeutronDircosu(i),
                    fission.getNeutronDircosv(i),
                    fission.getNeutronDircosw(i),
                    fission.getNeutronAge(i) * kFreyaAgeToSeconds
                )
            );
        }

        // Photons are part of the same FREYA fission event. Their correlated
        // energies, directions and emission ages are copied in the same way.
        for (int i = 0; i < result.photonMultiplicity; ++i)
        {
            result.particles.push_back(
                MakeParticle(
                    FreyaParticleType::Gamma,
                    fission.getPhotonEnergy(i),
                    fission.getPhotonDircosu(i),
                    fission.getPhotonDircosv(i),
                    fission.getPhotonDircosw(i),
                    fission.getPhotonAge(i) * kFreyaAgeToSeconds
                )
            );
        }

        return result;
    }
}
