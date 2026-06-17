// Copyright (c) 2026 [Menars]. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UniversalStatPreset.generated.h"

USTRUCT(BlueprintType)
struct FStatValuePair
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Universal Stat")
    FGameplayTag StatTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Universal Stat")
    float Value;
};


UCLASS(BlueprintType)
class RUMBLECORE_API UUniversalStatPreset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Universal Stat")
    TArray<FStatValuePair> InitialStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Universal Stat")
    FText PresetName;
};
