#pragma once
#include "CoreMinimal.h"

class UStaticMesh;
class FJsonObject;
class AWarfareFrontlineGameMode;
class UStaticMeshComponent;

namespace WarfareFRRUGermanHostAcceptanceV82
{
// Core original A7V/Beute contact and both weapon classes passed in the boxed
// 2447ca63 review. Exact packaged scene/persistence checks remain release gates.
inline constexpr bool FormalGermanHostAcceptance=true;
}

namespace FRRUGermanStaticContactV82
{
// Read-only CPU/collision/material fingerprint, identical in Editor and Cook.
// It never calls Cache(), the Editor snapshot bridge, or an asset write API.
TSharedPtr<FJsonObject> RuntimeStaticMeshSnapshot(UStaticMesh* Mesh);
// Read an already staged and strictly validated ingress terrain component.
// Never creates an apron or accepts a generic scene actor as terrain.
UStaticMeshComponent* ValidatedSceneApronV87(const AWarfareFrontlineGameMode* Battle,UStaticMeshComponent* Ground,TSharedPtr<FJsonObject>& Evidence);
#if WITH_EDITOR
// Candidate=nullptr is a read-only authored-source preflight. A non-null
// receiver-only candidate permits only matching invalid authored bases.
TSharedPtr<FJsonObject> RepairSourceInvalidBasisCandidateV82(UStaticMesh* Source,UStaticMesh* Candidate);
#endif
}
