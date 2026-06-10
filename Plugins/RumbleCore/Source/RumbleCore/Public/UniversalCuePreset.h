// Copyright (c) 2026 [Menars]. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UniversalGameplayCue.h"
#include "UniversalCuePreset.generated.h"

// 1. For Instant Effects
USTRUCT(BlueprintType)
struct FInstantCueInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TriggerTag;

	// 1. We will use this class to spawn a Cue Actor when the tag is added, and destroy it when the tag is removed. 
	// (e.g., Make the material green when the "Poison" tag is added, and restore the material to its original state when the "Poison" tag is removed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AUniversalGameplayCue> CueClass;

	// 2. We will spawn the particle effect and sound effect when the tag is added, and destroy them when the tag is removed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UNiagaraSystem* ParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USoundBase* SoundEffect = nullptr;
};
// 2. For Looping Effects
USTRUCT(BlueprintType)
struct FLoopingCueInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TriggerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AUniversalGameplayCue> CueClass;
};

UCLASS()
class RUMBLECORE_API UUniversalCuePreset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// Instant effects (Sword strike, Taking damage, Death explosion, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cues|Instant")
	TArray<FInstantCueInfo> InstantCues;

	// Temporary effects (Poison bubbles, Burning flames, etc. remain in the loop until the tag is deleted)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cues|Looping")
	TArray<FLoopingCueInfo> LoopingCues;
};
