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

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag StatTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value;
};


UCLASS(BlueprintType)
class RUMBLECORE_API UUniversalStatPreset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    TArray<FStatValuePair> InitialStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
    FText PresetName;
};
