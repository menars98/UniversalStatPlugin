// Copyright (c) 2026 [Menars]. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UniversalStatClampPreset.generated.h"

USTRUCT(BlueprintType)
struct FStatClampRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules")
	bool bHasMinimum = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules", meta = (EditCondition = "bHasMinimum"))
	float MinValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules")
	bool bMaxBoundToAnotherStat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules", meta = (EditCondition = "bMaxBoundToAnotherStat"))
	FGameplayTag MaxBoundaryStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules", meta = (EditCondition = "!bMaxBoundToAnotherStat"))
	bool bHasAbsoluteMax = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules", meta = (EditCondition = "bHasAbsoluteMax && !bMaxBoundToAnotherStat"))
	float AbsoluteMaxValue = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules")
	bool bLogClampEvents = false;
};

UCLASS(BlueprintType)
class RUMBLECORE_API UUniversalStatClampPreset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clamp Rules")
	TMap<FGameplayTag, FStatClampRule> StatClampRules;
};
