// Copyright (c) 2026 [Menars]. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UniversalStatsComponent.h" 
#include "UniversalCuePreset.h"
#include "UniversalCueComponent.generated.h"

class AUniversalGameplayCue;
class UNiagaraSystem;
class USoundBase;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RUMBLECORE_API UUniversalCueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUniversalCueComponent();

	// Data Asset
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Universal Cues")
	UUniversalCuePreset* CuePreset;

protected:
	virtual void BeginPlay() override;

	// Handlers for Gameplay Events and Tag changes
	UFUNCTION()
	void HandleGameplayEvent(UUniversalStatsComponent* OwningComp, FGameplayTag EventTag, FStatEventPayload Payload);

	UFUNCTION()
	void HandleTagGranted(UUniversalStatsComponent* OwningComp, FGameplayTag Tag);

	UFUNCTION()
	void HandleTagRemoved(UUniversalStatsComponent* OwningComp, FGameplayTag Tag);

	UFUNCTION()
	void HandleTagUpdated(UUniversalStatsComponent* OwningComp, FGameplayTag Tag, int32 NewStackCount);
private:
	// The list we keep in memory to delete looping effects later
	UPROPERTY()
	TMap<FGameplayTag, class AUniversalGameplayCue*> ActiveLoopingCues;

		
};
