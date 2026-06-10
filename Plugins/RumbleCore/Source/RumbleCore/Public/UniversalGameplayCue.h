// Copyright (c) 2026 [Menars]. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UniversalGameplayCue.generated.h"

UCLASS()
class RUMBLECORE_API AUniversalGameplayCue : public AActor
{
	GENERATED_BODY()
	
public:
	AUniversalGameplayCue();

	// 1. Works when a looping cue is added (e.g., Make the material green)
	UFUNCTION(BlueprintNativeEvent, Category = "Universal Cue")
	void OnCueAdded(AActor* TargetActor);

	// 2. Works When the Timed Cue is Deleted (e.g., Restore the Material to Its Original State)
	UFUNCTION(BlueprintNativeEvent, Category = "Universal Cue")
	void OnCueRemoved(AActor* TargetActor);

	// 3. Instant Cue Triggered (e.g., Damage taken, Camera shake)
	UFUNCTION(BlueprintNativeEvent, Category = "Universal Cue")
	void OnCueExecuted(AActor* TargetActor, float Magnitude);

	// 4. When a looping cue is updated (e.g., Update the intensity of the material based on the stack count)
	UFUNCTION(BlueprintNativeEvent, Category = "Universal Cue")
	void OnCueUpdated(AActor* TargetActor, int32 NewStackCount);

};
