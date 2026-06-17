// Copyright (c) 2026 [Menars]. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "GameplayTagContainer.h"
#include "UniversalStatEffect.generated.h"

class UUniversalStatsComponent;

// Effect type
UENUM(BlueprintType)
enum class EEffectDurationType : uint8
{
	Instant,		// Instant (Ex: Heal, Damage)
	HasDuration,	// Duration (Ex: Speed Boost, Shield)
	Infinite		// Infinite (Ex: Curse, Blessing)
};

UENUM(BlueprintType)
enum class EEffectStackingType : uint8
{
	None,				
	RefreshDuration,	// It doesn't stack, but when a new one is cast, it resets the duration of the old one.
	AddStack			// It stacks
};

// Struct to hold information about a stat modifier, used in UUniversalStatEffect
USTRUCT(BlueprintType)
struct FStatModifierInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
	FGameplayTag StatTag; // Which Stat

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
	float Magnitude = 0.f; // How much it modifies the stat (can be positive or negative)
};


UCLASS(Blueprintable, BlueprintType, Abstract)
class RUMBLECORE_API UUniversalStatEffect : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Duration")
	EEffectDurationType DurationType = EEffectDurationType::Instant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Duration", meta = (EditCondition = "DurationType == EEffectDurationType::HasDuration"))
	float Duration = 0.f;

	// If its greater than 0, it will apply the effect every Period seconds. (Ex: 10 damage every 1 second for 5 seconds)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Duration")
	float Period = 0.f;

	// What modifiers this effect applies to stats. (Ex: Speed +50%, Health -20)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Modifiers")
	TArray<FStatModifierInfo> Modifiers;

	// Identity (What is the name of this Effect? E.g.: Effect.Damage.Fire or Effect.Debuff.Poison)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Identity")
	FGameplayTag AssetTag;

	// State: Conditions that stick to the character, which we will use for the Cue (Visual/Audio) system 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Identity")
	FGameplayTagContainer GrantedTags;

	// Visual Tags
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Gameplay Cues")
	FGameplayTagContainer GameplayCues;

	// Tags that MUST be included in the target (Otherwise, this effect will not work)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Application Requirements")
	FGameplayTagContainer TargetMustHaveTags;

	// Tags that must NOT be included in the target (Immune system)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Application Requirements")
	FGameplayTagContainer TargetMustNotHaveTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Application", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ApplicationChance = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Stacking")
	EEffectStackingType StackingType = EEffectStackingType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Application Requirements")
	FGameplayTagContainer RemoveEffectsWithTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Stacking", meta = (EditCondition = "StackingType == EEffectStackingType::AddStack"))
	int32 MaxStacks = 1;

	// --- FUNCTIONS ---

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Effect Execution")
	float CalculateModifierMagnitude(UUniversalStatsComponent* TargetComponent, AActor* Instigator, float EffectLevel, FGameplayTag StatTag, float BaseMagnitude) const;
};

USTRUCT(BlueprintType)
struct FUniversalEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spec")
	TSubclassOf<UUniversalStatEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spec")
	float Level = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spec")
	AActor* Instigator = nullptr;

	FUniversalEffectSpec() {}
	FUniversalEffectSpec(TSubclassOf<UUniversalStatEffect> InClass, float InLevel = 1.f, AActor* InInstigator = nullptr)
		: EffectClass(InClass), Level(InLevel), Instigator(InInstigator) {
	}
};