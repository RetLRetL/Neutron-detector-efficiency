#pragma once

#include "freya_fission_result.h"

// FREYA integration functions. This is a namespace, not a stateful class:
// the LLNL library owns its internal model state, while the result is
// returned explicitly in FreyaFissionResult.
namespace FreyaFissionGenerator
{
    // Configure FREYA for event-by-event correlated fission and connect its
    // random-number callback to the Geant4 random engine.
    void Initialize();

    // Generate one spontaneous Cf-252 fission.
    FreyaFissionResult GenerateCf252();
}
