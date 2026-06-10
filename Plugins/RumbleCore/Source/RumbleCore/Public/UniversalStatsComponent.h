// Copyright (c) 2026 [Menars]. All Rights Reserved.
// Universal Stat & Effect Framework - Developed for Unreal Engine 5.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UniversalStatPreset.h"
#include "UniversalStatClampPreset.h"
#include "UniversalStatTriggerPreset.h"
#include "UniversalStatEffect.h"
#include "UniversalStatsComponent.generated.h"

USTRUCT(BlueprintType)
struct FActiveStatEffect
{
	GENERATED_BODY()

	// Unique handle for this active effect instance. We will use this to identify and remove specific effects when needed (e.g. dispels, cleanses).
	UPROPERTY(BlueprintReadOnly)
	FGuid EffectHandle;

	// Level, instigator & class
	UPROPERTY(BlueprintReadOnly)
	FUniversalEffectSpec Spec;

	// Not the object itself, just a read-only pointer to its “Default Copy (CDO)”!
	UPROPERTY()
	const UUniversalStatEffect* EffectDef = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentStackCount = 1;

	float TimeRemaining = 0.f;
	float PeriodTimer = 0.f;

	FActiveStatEffect() {}
	FActiveStatEffect(const FUniversalEffectSpec& InSpec, const UUniversalStatEffect* InDef)
		: Spec(InSpec), EffectDef(InDef)
	{
		EffectHandle = FGuid::NewGuid();
	}

	bool operator==(const FActiveStatEffect& Other) const
	{
		return EffectHandle == Other.EffectHandle;
	}
};

UENUM(BlueprintType)
enum class EStatAdjustmentType : uint8
{
	Proportional, // If stat is 50 and we apply a 10% increase, new value will be 55. If we apply a 10% decrease, new value will be 45.
	Delta         // If stat is 50 and we apply a 10 increase, new value will be 60. If we apply a 10 decrease, new value will be 40.
};

// Data structure for a single stat entry. We will use this for replication and keep a TMap cache for fast access on the component.
USTRUCT(BlueprintType)
struct FUniversalStatEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag StatTag;

	UPROPERTY(BlueprintReadOnly)
	float Value = 0.f;

	FUniversalStatEntry() {}
	FUniversalStatEntry(FGameplayTag InTag, float InValue) : StatTag(InTag), Value(InValue) {}

	bool operator==(const FUniversalStatEntry& Other) const
	{
		return StatTag == Other.StatTag;
	}
};

// --- DELEGATES ---
// For stat change events. We will broadcast this after the stat value is actually changed, so listeners can react to the new value.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnStatChangedSignature, UUniversalStatsComponent*, OwningComp, FGameplayTag, StatTag, float, OldValue, float, NewValue);
// For conditional gameplay events that are triggered by certain stat changes (e.g. health reaching 0). 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGameplayEventSignature, UUniversalStatsComponent*, OwningComp, FGameplayTag, EventTag, FStatEventPayload, Payload);
// For tag change events. We will broadcast this when tags are added or removed (e.g. from effects), so listeners can react to the new tag state.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTagChangedSignature, UUniversalStatsComponent*, OwningComp, FGameplayTag, Tag);
// For tag update events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTagUpdatedSignature, UUniversalStatsComponent*, OwningComp, FGameplayTag, Tag, int32, NewStackCount);
UCLASS(Blueprintable ,ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RUMBLECORE_API UUniversalStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUniversalStatsComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Universal Stats|Events")
	FOnStatChangedSignature OnStatChanged;

	UPROPERTY(BlueprintAssignable, Category = "Universal Stats|Events")
	FOnGameplayEventSignature OnGameplayEvent;

	UPROPERTY(BlueprintAssignable, Category = "Universal Stats|Tags")
	FOnTagChangedSignature OnTagGranted;

	UPROPERTY(BlueprintAssignable, Category = "Universal Stats|Tags")
	FOnTagChangedSignature OnTagRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Universal Stats|Tags")
	FOnTagUpdatedSignature OnTagUpdated;

	// Data Assets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Universal Stats|Triggers")
	UUniversalStatTriggerPreset* TriggerPreset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Universal Stats|Initialization")
	TArray<UUniversalStatPreset*> StatPresets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Universal Stats|Clamping")
	UUniversalStatClampPreset* ClampPreset;

	// Innate tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Universal Stats|Tags")
	FGameplayTagContainer BaseTags;

	// --- BASE FUNCTIONS ---

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Initialization")
	void InitializeFromPresets();

	// Creates new stat 
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats")
	void AddStat(FGameplayTag StatTag, float InitialValue);

	// Getter
	UFUNCTION(BlueprintPure, Category = "Universal Stats")
	float GetStatValue(FGameplayTag StatTag) const;

	// Setter
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats")
	void SetStatValue(FGameplayTag StatTag, float NewValue);

	// Modify stat by adding DeltaValue to current value
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats")
	void ModifyStat(FGameplayTag StatTag, float DeltaValue);

	// Check if stat exists
	UFUNCTION(BlueprintPure, Category = "Universal Stats")
	bool HasStat(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Effects")
	FGuid ApplyEffectToSelf(const FUniversalEffectSpec& EffectSpec);

	// Adjust stat value when its max value changes.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Helpers")
	void AdjustStatForMaxChange(FGameplayTag AffectedStatTag, FGameplayTag MaxStatTag, float NewMaxValue, EStatAdjustmentType AdjustmentType);

	// STEP 1 (MANAGEMENT): To delete a specific Effect (or tag group) above us
	// Example: When drinking the antidote, we will send and delete the “Effect.Debuff.Poison” tag. It returns how many were deleted.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Effects")
	int32 RemoveEffectsWithTag(FGameplayTag TagToRemove, int32 StacksToRemove = -1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Effects")
	int32 RemoveEffectsByClass(TSubclassOf<UUniversalStatEffect> EffectClassToRemove, int32 StacksToRemove = -1);

	// Active states on character (e.g., is State.Frozen present?)
	UFUNCTION(BlueprintPure, Category = "Universal Stats|Tags")
	bool HasActiveTag(FGameplayTag TagToCheck) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Effects")
	void ClearAllEffects();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Tags")
	void AddLooseTag(FGameplayTag TagToAdd);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Tags")
	void RemoveLooseTag(FGameplayTag TagToRemove);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Universal Stats|Filters")
	float PreStatChange(FGameplayTag StatTag, float AttemptedValue);

	UFUNCTION(BlueprintPure, Category = "Universal Stats|Helpers")
	static float GetEffectModifierMagnitude(TSubclassOf<UUniversalStatEffect> EffectClass, FGameplayTag StatTag);

	UFUNCTION(BlueprintPure, Category = "Universal Stats|Queries")
	TMap<FGameplayTag, float> GetAllCurrentStats() const;

	UFUNCTION(BlueprintPure, Category = "Universal Stats|Helpers")
	bool CanAffordEffect(TSubclassOf<UUniversalStatEffect> EffectClass) const;

	// This function will return the remaining time for the first effect instance that has the specified tag.
	UFUNCTION(BlueprintPure, Category = "Universal Stats|Effects")
	float GetTimeRemainingForTag(FGameplayTag Tag) const;

	// This function will return the remaining time for the first effect instance of the specified class.
	UFUNCTION(BlueprintPure, Category = "Universal Stats|Save")
	TMap<FGameplayTag, float> GetStatsForSave() const;

	// This function will restore stats from a saved state. It will only update the stats that are present in the SavedStats map, and leave the rest unchanged. This allows for partial restores (e.g. only health and mana).
	// NOTE: Active effects (buffs/debuffs/DoTs) are NOT persisted. Only current stat values are saved.
	// Effects with remaining duration will be lost on load. This is by design for V1.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Universal Stats|Save")
	void RestoreStatsFromSave(const TMap<FGameplayTag, float>& SavedStats);

protected:

	virtual void BeginPlay() override;

	// Replicated array of stats. We will use this for replication and update StatsCache from this when needed.
	UPROPERTY(ReplicatedUsing = OnRep_Stats)
	TArray<FUniversalStatEntry> ReplicatedStats;

	// Replicated tags from active effects. We will use this for replication and update the active tag list from this when needed. 
	// This is separate from BaseTags since those are innate and won't change, while these are dynamic and can change frequently.
	UPROPERTY(ReplicatedUsing = OnRep_ActiveTags)
	FGameplayTagContainer ReplicatedTags;

	// List of active effects (Buff/Debuff) currently on us
	UPROPERTY()
	TArray<FActiveStatEffect> ActiveEffects;

	// For fast access, not replicated. We will update ReplicatedStats from this cache when needed.
	TMap<FGameplayTag, float> StatsCache;

	TMap<FGameplayTag, FStatClampRule> CachedClampRules;

	// For fast access, not replicated. We will update ReplicatedTags from this cache when needed.
	UFUNCTION()
	void OnRep_ActiveTags(const FGameplayTagContainer& OldTags);

	// Update the replicated tags from the active effects. This should be called on the server whenever an effect is added or removed.
	void UpdateReplicatedTags();

	// Called on clients when ReplicatedStats is updated. We will update StatsCache from ReplicatedStats here.
	UFUNCTION()
	void OnRep_Stats(const TArray<FUniversalStatEntry>& OldStats);

	void UpdateArrayFromCache(FGameplayTag StatTag, float NewValue);

	UFUNCTION()
	float ApplyClampRules(FGameplayTag StatTag, float DesiredValue) const;

	// To prevent code duplication: A helper function that will pass through the filter before each Modifier is applied
	void ProcessEffectModifier(const FUniversalEffectSpec& Spec, const UUniversalStatEffect* EffectDef, const FStatModifierInfo& Mod);

	// Helper functions for ApplyEffectToSelf:

	// 1. Tags
	UFUNCTION(BlueprintPure, Category = "Universal Stats|Tags")
	FGameplayTagContainer GetAllActiveTags() const;

	// 2. Chance
	bool CanApplyEffect(const UUniversalStatEffect* EffectDef) const;

	// 3. Stacking 
	FGuid HandleEffectStacking(const UUniversalStatEffect* EffectDef);

	// 4. 
	void ExecuteEffectModifiers(const FUniversalEffectSpec& Spec, const UUniversalStatEffect* EffectDef);
private:

	void EvaluateTriggers(FGameplayTag StatTag, float OldValue, float NewValue);

};
