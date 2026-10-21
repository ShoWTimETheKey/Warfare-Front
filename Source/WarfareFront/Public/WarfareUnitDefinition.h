#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WarfareUnitDefinition.generated.h"

class UAnimInstance;
class USkeletalMesh;

UENUM(BlueprintType)
enum class EWarfareEra : uint8
{
    FirstWorldWar,
    SecondWorldWar
};

/** Records accompany the actual mesh/animation/texture license files, not replace them. */
USTRUCT(BlueprintType)
struct FWarfareAssetSource
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Source")
    FString Creator;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Source")
    FString SourceURL;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Source")
    FString LicenseName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Source")
    FString LicenseEvidenceRelativePath;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Source")
    FString RequiredAttribution;
};

/** Authoring schema only. No placeholder soldier is spawned from an empty definition. */
UCLASS(BlueprintType)
class WARFAREFRONT_API UWarfareUnitDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="History")
    EWarfareEra Era = EWarfareEra::FirstWorldWar;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="History")
    FName FactionId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="History")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="History")
    FText WeaponDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="History", meta=(MultiLine=true))
    FText HistoricalReferenceNotes;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TSoftObjectPtr<USkeletalMesh> SoldierMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TSoftClassPtr<UAnimInstance> AnimationClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Review")
    TArray<FWarfareAssetSource> Sources;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Review")
    bool bArtApproved = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Review")
    bool bAnimationApproved = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Review")
    bool bLicenseReviewed = false;
};
