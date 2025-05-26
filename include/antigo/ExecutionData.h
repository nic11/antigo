#pragma once

#include "antigo/Context.h"

namespace antigo {

struct ExecutionData;
ExecutionData& GetCurrentExecutionData();

bool HasExceptionWitness();
ResolvedContext PopExceptionWitness();

bool HasExceptionWitnessOrphan();
ResolvedContext PopExceptionWitnessOrphan();

}
