// SPDX-License-Identifier: MIT
// Source-only replacement. The optional private Tripo rubble candidate and all
// sampled contact/placement data are omitted, not approximated or re-created.
#pragma once
#if WITH_EDITOR
namespace FRRUVauxStreetRubbleV87
{
inline bool Requested(){return false;}
inline bool Stage(AWarfareFrontlineGameMode* B,AActor* Owner,AActor* Props,const FString& Scene,FString& Error,
    bool(*Ready)(UMaterialInterface*,FString&),TSharedPtr<FJsonObject>* Evidence)
{
    (void)B;(void)Owner;(void)Props;(void)Scene;(void)Ready;(void)Evidence;
    Error=TEXT("This source-only distribution omits the optional Tripo rubble asset data.");
    return false;
}
}
#endif
