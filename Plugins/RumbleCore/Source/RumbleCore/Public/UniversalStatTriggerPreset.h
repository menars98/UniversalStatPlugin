// Copyright (c) 2026 [Menars]. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UniversalStatTriggerPreset.generated.h"

// 
USTRUCT(BlueprintType)
struct FStatEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag TriggeringStat; 

	UPROPERTY(BlueprintReadOnly)
	float TriggeringValue; 

	// "AActor* Instigator" 
};

UENUM(BlueprintType)
enum class EStatTriggerCondition : uint8
{
	LessThanOrEqual,
	GreaterThanOrEqual,
	EqualTo
};

USTRUCT(BlueprintType)
struct FStatTriggerRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	FGameplayTag StatToWatch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	EStatTriggerCondition Condition = EStatTriggerCondition::LessThanOrEqual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	float Threshold = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	FGameplayTag EventTagToFire; 
};

UCLASS(BlueprintType)
class RUMBLECORE_API UUniversalStatTriggerPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triggers")
	TArray<FStatTriggerRule> TriggerRules;
};
