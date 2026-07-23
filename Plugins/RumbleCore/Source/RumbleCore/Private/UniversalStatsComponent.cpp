// Copyright (c) 2026 [Menars]. All Rights Reserved.


#include "UniversalStatsComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "UniversalStatClampPreset.h"
#include "RumbleCore.h"
#include "UniversalStatTriggerPreset.cpp"
#include "UniversalStatEffect.h"

UUniversalStatsComponent::UUniversalStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UUniversalStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UUniversalStatsComponent, ReplicatedStats);
	DOREPLIFETIME(UUniversalStatsComponent, ReplicatedTags);
}

void UUniversalStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority() || ActiveEffects.IsEmpty()) return;

	// We are setting up the loop in reverse so that the array does not crash when deleting expired items
	for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
	{
		FActiveStatEffect& ActiveEffect = ActiveEffects[i];
		const UUniversalStatEffect* Def = ActiveEffect.EffectDef;

		// 1. PERIOD (TIME-BASED DAMAGE/HEAL) CONTROL
		if (Def->Period > 0.f)
		{
			ActiveEffect.PeriodTimer -= DeltaTime;
			if (ActiveEffect.PeriodTimer <= 0.f)
			{
				for (const FStatModifierInfo& Mod : Def->Modifiers)
				{
					ProcessEffectModifier(ActiveEffect.Spec, Def, Mod);

				}
				// Reset the counter
				ActiveEffect.PeriodTimer = Def->Period;

				UpdateReplicatedTags();
			}
		}

		// 2. DURATION CONTROL
		if (Def->DurationType == EEffectDurationType::HasDuration)
		{
			ActiveEffect.TimeRemaining -= DeltaTime;
			if (ActiveEffect.TimeRemaining <= 0.f)
			{
				// --- A. RollBack ---
				if (Def->Period <= 0.f)
				{
					for (const FStatModifierInfo& Mod : Def->Modifiers)
					{
						float RevertValue = -Mod.Magnitude;
						SetStatValue(Mod.StatTag, GetStatValue(Mod.StatTag) + RevertValue);
					}
				}

				// --- B. Broadcast to UI & Cue ---
				for (const FGameplayTag& Tag : Def->GrantedTags)
				{
					OnTagRemoved.Broadcast(this, Tag);
				}
				for (const FGameplayTag& CueTag : Def->GameplayCues)
				{
					OnTagRemoved.Broadcast(this, CueTag);
				}

				// --- C. Delete ---
				ActiveEffects.RemoveAtSwap(i);

				UpdateReplicatedTags();
			}
		}
	}
}

float UUniversalStatsComponent::GetEffectModifierMagnitude(TSubclassOf<UUniversalStatEffect> EffectClass, FGameplayTag StatTag)
{
	if (!EffectClass) return 0.f;

	// Read from CDO since we only care about the default modifiers defined in the class, not any runtime changes
	const UUniversalStatEffect* CDO = EffectClass->GetDefaultObject<UUniversalStatEffect>();

	for (const FStatModifierInfo& Mod : CDO->Modifiers)
	{
		if (Mod.StatTag == StatTag)
		{
			return Mod.Magnitude;
		}
	}
	return 0.f; 
}

TMap<FGameplayTag, float> UUniversalStatsComponent::GetAllCurrentStats() const
{
	return StatsCache;
}

bool UUniversalStatsComponent::CanAffordEffect(TSubclassOf<UUniversalStatEffect> EffectClass) const
{
	if (!EffectClass) return false;

	// Read CDO since we only care about the default modifiers defined in the class, not any runtime changes
	const UUniversalStatEffect* CDO = EffectClass->GetDefaultObject<UUniversalStatEffect>();

	for (const FStatModifierInfo& Mod : CDO->Modifiers)
	{
		// If the modifier is negative, it means we need to pay a cost. We check if we can afford that cost by comparing it to our current stat value.
		if (Mod.Magnitude < 0.f)
		{
			float CurrentStat = GetStatValue(Mod.StatTag);
			float AbsoluteCost = FMath::Abs(Mod.Magnitude);

			// If we don't have enough of the stat to pay the cost, we cannot afford this effect, so we return false immediately.
			if (CurrentStat < AbsoluteCost)
			{
				return false;
			}
		}
	}
	return true;
}

float UUniversalStatsComponent::GetTimeRemainingForTag(FGameplayTag Tag) const
{
	float MaxTime = 0.f;
	// Scan through all active effects to find any that grant the specified tag, and return the longest remaining time among them. 
	// This is useful for cooldowns, for example, to show the correct remaining time on the UI.
	for (const FActiveStatEffect& Effect : ActiveEffects)
	{
		// If this effect grants the tag we are looking for
		if (Effect.EffectDef && (Effect.EffectDef->GrantedTags.HasTag(Tag) || Effect.EffectDef->AssetTag.MatchesTag(Tag)))

		{
			// If there are multiple effects with the same tag, take the one with the longest remaining time
			if (Effect.TimeRemaining > MaxTime)
			{
				MaxTime = Effect.TimeRemaining;
			}
		}
	}
	return MaxTime; // If no active effect grants the tag, this will return 0, which can be used by the UI to hide cooldown timers, for example.
}

TMap<FGameplayTag, float> UUniversalStatsComponent::GetStatsForSave() const
{
	return StatsCache;
}

void UUniversalStatsComponent::RestoreStatsFromSave(const TMap<FGameplayTag, float>& SavedStats)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	// 1. Load the saved stats into our cache. This will be our new source of truth for all stat values.
	StatsCache = SavedStats;

	// 2. Clear the ReplicatedStats array, which is what the UI and clients use to display stat values. We will repopulate this based on our updated cache.
	ReplicatedStats.Empty();

	// 3. Broadcast stat changes for all stats in the cache. Since we don't know the old values, we will just pass the new value as both old and new
	for (const auto& Pair : StatsCache)
	{
		ReplicatedStats.Add(FUniversalStatEntry(Pair.Key, Pair.Value));

		// Since we don't know the old values, we will just pass the new value as both old and new
		OnStatChanged.Broadcast(this, Pair.Key, Pair.Value, Pair.Value);
	}
}

int32 UUniversalStatsComponent::RemoveEffectsByClass(TSubclassOf<UUniversalStatEffect> EffectClassToRemove, int32 StacksToRemove)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !EffectClassToRemove) return 0;
	int32 RemovedCount = 0;

	for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
	{
		// 1.Is this effect an instance of the class we want to remove? We use IsA to allow removing by parent class as well
		if (ActiveEffects[i].EffectDef && ActiveEffects[i].EffectDef->IsA(EffectClassToRemove))
		{
			// 2. Remove completely if StacksToRemove is -1 (default) or if the current stack count is less than or equal to the stacks we want to remove. (Full Removal)
			if (StacksToRemove == -1 || ActiveEffects[i].CurrentStackCount <= StacksToRemove)
			{
				// --- Rollback ---
				if (ActiveEffects[i].EffectDef->Period <= 0.f)
				{
					for (const FStatModifierInfo& Mod : ActiveEffects[i].EffectDef->Modifiers)
					{
						float RevertValue = -Mod.Magnitude;
						//
						SetStatValue(Mod.StatTag, GetStatValue(Mod.StatTag) + RevertValue);
					}
				}

				// --- Broadcast ---
				for (const FGameplayTag& Tag : ActiveEffects[i].EffectDef->GrantedTags)
				{
					OnTagRemoved.Broadcast(this, Tag);
				}
				for (const FGameplayTag& CueTag : ActiveEffects[i].EffectDef->GameplayCues)
				{
					OnTagRemoved.Broadcast(this, CueTag);
				}

				// --- Remove From Memory ---
				ActiveEffects.RemoveAtSwap(i);
				RemovedCount++;
			}
			// 3. Or just reduce the stack count if we have more stacks than we want to remove. (Partial Removal)
			else
			{
				ActiveEffects[i].CurrentStackCount -= StacksToRemove;

				// Broadcast
				for (const FGameplayTag& CueTag : ActiveEffects[i].EffectDef->GameplayCues)
				{
					OnTagUpdated.Broadcast(this, CueTag, ActiveEffects[i].CurrentStackCount);
				}
				RemovedCount++; 
			}
		}
	}

	if (RemovedCount > 0)
	{
		UpdateReplicatedTags();
	}

	return RemovedCount;
}

int32 UUniversalStatsComponent::RemoveEffectsWithTag(FGameplayTag TagToRemove, int32 StacksToRemove)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !TagToRemove.IsValid()) return 0;
	int32 RemovedCount = 0;

	// For safety, we loop in reverse since we might be deleting items from the array while iterating through it
	for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
	{
		if (ActiveEffects[i].EffectDef)
		{

			bool bMatchesAssetTag = ActiveEffects[i].EffectDef->AssetTag.MatchesTag(TagToRemove);
			bool bMatchesGrantedTag = ActiveEffects[i].EffectDef->GrantedTags.HasTag(TagToRemove);

			if (bMatchesAssetTag || bMatchesGrantedTag)
			{
				// Deleting totally?
				if (StacksToRemove == -1 || ActiveEffects[i].CurrentStackCount <= StacksToRemove)
				{
					// 1. Rollback
					if (ActiveEffects[i].EffectDef->Period <= 0.f)
					{
						for (const FStatModifierInfo& Mod : ActiveEffects[i].EffectDef->Modifiers)
						{
							SetStatValue(Mod.StatTag, GetStatValue(Mod.StatTag) - Mod.Magnitude); 
						}
					}

					// 2. Broadcast
					for (const FGameplayTag& Tag : ActiveEffects[i].EffectDef->GrantedTags)
					{
						OnTagRemoved.Broadcast(this, Tag);
					}
					for (const FGameplayTag& CueTag : ActiveEffects[i].EffectDef->GameplayCues)
					{
						OnTagRemoved.Broadcast(this, CueTag);
					}

					// 3. Delete from memory
					ActiveEffects.RemoveAtSwap(i);
					RemovedCount++;
				}

				else
				{
					ActiveEffects[i].CurrentStackCount -= StacksToRemove;
					// Broadcast
					for (const FGameplayTag& CueTag : ActiveEffects[i].EffectDef->GameplayCues)
					{
						OnTagUpdated.Broadcast(this, CueTag, ActiveEffects[i].CurrentStackCount);
					}
					RemovedCount++;
				}
			}
		}
	}

	if (RemovedCount > 0)
	{
		UpdateReplicatedTags();
	}

	return RemovedCount;
}

void UUniversalStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority() && !StatPresets.IsEmpty())
	{
		InitializeFromPresets();
	}
}

void UUniversalStatsComponent::InitializeFromPresets()
{
	for (UUniversalStatPreset* Preset : StatPresets)
	{
		if (!Preset) continue;

		for (const FStatValuePair& StatPair : Preset->InitialStats)
		{
			if (!StatPair.StatTag.IsValid()) continue;

			// Set stat if it already exists, otherwise add it. This allows later presets to override earlier ones if there are duplicates.
			if (HasStat(StatPair.StatTag))
			{
				SetStatValue(StatPair.StatTag, StatPair.Value);
			}
			else
			{
				AddStat(StatPair.StatTag, StatPair.Value);
			}
		}
	}
}

void UUniversalStatsComponent::AddStat(FGameplayTag StatTag, float InitialValue)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || HasStat(StatTag)) return;

	StatsCache.Add(StatTag, InitialValue);
	UpdateArrayFromCache(StatTag, InitialValue);

	// Broadcast for server too
	OnStatChanged.Broadcast(this, StatTag, InitialValue, InitialValue);
}

float UUniversalStatsComponent::GetStatValue(FGameplayTag StatTag) const
{
	if (const float* FoundValue = StatsCache.Find(StatTag))
	{
		return *FoundValue;
	}
	return 0.f;
}

bool UUniversalStatsComponent::HasStat(FGameplayTag StatTag) const
{
	return StatsCache.Contains(StatTag);
}

FGuid UUniversalStatsComponent::ApplyEffectToSelf(const FUniversalEffectSpec& EffectSpec)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !EffectSpec.EffectClass) return FGuid();

	const UUniversalStatEffect* EffectDef = EffectSpec.EffectClass->GetDefaultObject<UUniversalStatEffect>();

	if (!CanApplyEffect(EffectDef)) return FGuid();

	// Chance Control
	if (EffectDef->ApplicationChance < 1.0f && FMath::FRand() > EffectDef->ApplicationChance)
	{
		return FGuid();
	}

	// Instant effects
	// To fix guid duplication, we will not create a new active effect for instant effects. Instead, we will just execute the modifiers and return a new guid.
	// This is because instant effects do not have a duration or stacking, so there is no need to keep track of them in the ActiveEffects array.
	if (EffectDef->DurationType == EEffectDurationType::Instant)
	{
		ExecuteEffectModifiers(EffectSpec, EffectDef);

		FStatEventPayload DummyPayload;
		for (const FGameplayTag& CueTag : EffectDef->GameplayCues)
		{
			OnGameplayEvent.Broadcast(this, CueTag, DummyPayload);
		}

		return FGuid::NewGuid();
	}

	// Stacking 
	FGuid StackedGuid = HandleEffectStacking(EffectDef);
	if (StackedGuid.IsValid())
	{
		return StackedGuid; 
	}

	// Create a new active effect and add to list
	FActiveStatEffect NewActiveEffect(EffectSpec, EffectDef);

	if (EffectDef->DurationType == EEffectDurationType::HasDuration)
	{
		NewActiveEffect.TimeRemaining = EffectDef->Duration;
	}

	// If no period is set, we execute the modifiers immediately. Otherwise, we will execute them in Tick when the period timer goes off.
	if (EffectDef->Period <= 0.f)
	{
		ExecuteEffectModifiers(EffectSpec, EffectDef);
	}
	else
	{
		NewActiveEffect.PeriodTimer = EffectDef->Period; // Set timer
	}

	// Add to list
	ActiveEffects.Add(NewActiveEffect);

	// Broadcast logic tags
	for (const FGameplayTag& Tag : EffectDef->GrantedTags)
	{
		OnTagGranted.Broadcast(this, Tag);
	}
	// Broadcast visual tags
	for (const FGameplayTag& CueTag : EffectDef->GameplayCues)
	{
		OnTagGranted.Broadcast(this, CueTag);
	}

	if (!EffectDef->RemoveEffectsWithTags.IsEmpty())
	{
		for (const FGameplayTag& TagToKill : EffectDef->RemoveEffectsWithTags)
		{
			RemoveEffectsWithTag(TagToKill);
		}
	}

	UpdateReplicatedTags();

	return NewActiveEffect.EffectHandle;
}

void UUniversalStatsComponent::AdjustStatForMaxChange(FGameplayTag AffectedStatTag, FGameplayTag MaxStatTag, float NewMaxValue, EStatAdjustmentType AdjustmentType)
{
	// Guards
	if (!GetOwner() || !GetOwner()->HasAuthority() || !HasStat(AffectedStatTag) || !HasStat(MaxStatTag)) return;

	const float OldMaxValue = GetStatValue(MaxStatTag);

	if (!FMath::IsNearlyEqual(OldMaxValue, NewMaxValue))
	{
		const float CurrentValue = GetStatValue(AffectedStatTag);
		float NewDelta = 0.f;

		if (AdjustmentType == EStatAdjustmentType::Proportional)
		{
			// Proportional
			NewDelta = (OldMaxValue > 0.f) ? ((CurrentValue * NewMaxValue) / OldMaxValue) - CurrentValue : NewMaxValue;
		}
		else if (AdjustmentType == EStatAdjustmentType::Delta)
		{
			// Flat
			NewDelta = NewMaxValue - OldMaxValue;
		}

		// Modify affected stat by the Delta. This will ensure that the affected stat maintains the same percentage of its max if the max changes.
		ModifyStat(AffectedStatTag, NewDelta);
	}
}

bool UUniversalStatsComponent::HasActiveTag(FGameplayTag TagToCheck) const
{
	if (BaseTags.HasTag(TagToCheck)) return true;

	// We go through all the active effects above us and check if this character has this tag.
	for (const FActiveStatEffect& ActiveEffect : ActiveEffects)
	{
		if (ActiveEffect.EffectDef && ActiveEffect.EffectDef->GrantedTags.HasTag(TagToCheck))
		{
			return true;
		}
	}
	return false;
}

void UUniversalStatsComponent::ClearAllEffects()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (const FActiveStatEffect& ActiveEffect : ActiveEffects)
	{
		if (ActiveEffect.EffectDef)
		{
			// 1. ROLLBACK 
			if (ActiveEffect.EffectDef->Period <= 0.f)
			{
				for (const FStatModifierInfo& Mod : ActiveEffect.EffectDef->Modifiers)
				{
					SetStatValue(Mod.StatTag, GetStatValue(Mod.StatTag) - Mod.Magnitude);
				}
			}

			// 2. Broadcast
			for (const FGameplayTag& Tag : ActiveEffect.EffectDef->GrantedTags)
			{
				OnTagRemoved.Broadcast(this, Tag);
			}
			for (const FGameplayTag& CueTag : ActiveEffect.EffectDef->GameplayCues)
			{
				OnTagRemoved.Broadcast(this, CueTag);
			}
		}
	}

	// 3. Clear from memory
	ActiveEffects.Empty();
	UpdateReplicatedTags(); 
}

void UUniversalStatsComponent::AddLooseTag(FGameplayTag TagToAdd)
{
	if (GetOwner() && GetOwner()->HasAuthority() && TagToAdd.IsValid())
	{
		BaseTags.AddTag(TagToAdd);
		OnTagGranted.Broadcast(this, TagToAdd); 

		UpdateReplicatedTags();
	}
}

void UUniversalStatsComponent::RemoveLooseTag(FGameplayTag TagToRemove)
{
	if (GetOwner() && GetOwner()->HasAuthority() && TagToRemove.IsValid())
	{
		BaseTags.RemoveTag(TagToRemove);
		OnTagRemoved.Broadcast(this, TagToRemove); 

		UpdateReplicatedTags();
	}
}

void UUniversalStatsComponent::SetStatValue(FGameplayTag StatTag, float NewValue)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !HasStat(StatTag)) return;

	if (FMath::IsNearlyEqual(StatsCache[StatTag], NewValue)) return; 

	float OldValue = StatsCache[StatTag];

	StatsCache[StatTag] = NewValue;
	UpdateArrayFromCache(StatTag, NewValue);

	// For Server
	// This event for ui
	OnStatChanged.Broadcast(this, StatTag, OldValue, NewValue);
	// This event for gameplay events
	EvaluateTriggers(StatTag, OldValue, NewValue);
}

void UUniversalStatsComponent::ModifyStat(FGameplayTag StatTag, float DeltaValue)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !HasStat(StatTag)) return;

	float CurrentValue = GetStatValue(StatTag);
	float DesiredValue = CurrentValue + DeltaValue;

	// --- CUSTOM LOGIC ---
	DesiredValue = PreStatChange(StatTag, DesiredValue);

	// --- BUILT-IN RULES ---
	DesiredValue = ApplyClampRules(StatTag, DesiredValue);

	if (!FMath::IsNearlyEqual(CurrentValue, DesiredValue))
	{
		SetStatValue(StatTag, DesiredValue);
	}
}

void UUniversalStatsComponent::OnRep_ActiveTags(const FGameplayTagContainer& OldTags)
{
	// Broadcast new tags that we gained
	for (auto It = ReplicatedTags.CreateConstIterator(); It; ++It)
	{
		if (!OldTags.HasTagExact(*It))
		{
			OnTagGranted.Broadcast(this, *It);
		}
	}

	// Broadcast old tags that we lost
	for (auto It = OldTags.CreateConstIterator(); It; ++It)
	{
		if (!ReplicatedTags.HasTagExact(*It))
		{
			OnTagRemoved.Broadcast(this, *It);
		}
	}
}

void UUniversalStatsComponent::UpdateReplicatedTags()
{
	// We rebuild the ReplicatedTags array from scratch every time we update it, 
	// by going through all active effects and appending their granted tags and gameplay cues to the BaseTags. 
	// This way, we ensure that ReplicatedTags always has the full list of tags that should be active on this character based on the current active effects.
	FGameplayTagContainer NewTags = BaseTags;
	for (const FActiveStatEffect& ActiveEffect : ActiveEffects)
	{
		if (ActiveEffect.EffectDef)
		{
			NewTags.AppendTags(ActiveEffect.EffectDef->GrantedTags);
			NewTags.AppendTags(ActiveEffect.EffectDef->GameplayCues);
		}
	}
	ReplicatedTags = NewTags;
}

void UUniversalStatsComponent::OnRep_Stats(const TArray<FUniversalStatEntry>& OldStats)
{
	for (const FUniversalStatEntry& NewEntry : ReplicatedStats)
	{
		// 1. Update map
		StatsCache.Add(NewEntry.StatTag, NewEntry.Value);

		// 2. Search for the same stat in old array to compare values.
		const FUniversalStatEntry* FoundOld = OldStats.FindByPredicate(
			[&NewEntry](const FUniversalStatEntry& Entry) {
				return Entry == NewEntry;
			}
		);

		// 3. If not found in old array or value is different, broadcast the change. 
		// This allows us to only broadcast changes that actually changed value, and not every stat on the component every time ReplicatedStats is updated.
		if (!FoundOld || !FMath::IsNearlyEqual(FoundOld->Value, NewEntry.Value))
		{
			float OldVal = FoundOld ? FoundOld->Value : NewEntry.Value;
			OnStatChanged.Broadcast(this, NewEntry.StatTag, OldVal, NewEntry.Value);
		}
	}
}

// After updating TMap, helper function that updates the Replicated array
void UUniversalStatsComponent::UpdateArrayFromCache(FGameplayTag StatTag, float NewValue)
{
	bool bFound = false;
	for (FUniversalStatEntry& Entry : ReplicatedStats)
	{
		if (Entry.StatTag == StatTag)
		{
			Entry.Value = NewValue;
			bFound = true;
			break;
		}
	}

	// If it's not in the series, add it
	if (!bFound)
	{
		ReplicatedStats.Add(FUniversalStatEntry(StatTag, NewValue));
	}
}

float UUniversalStatsComponent::ApplyClampRules(FGameplayTag StatTag, float DesiredValue) const
{
	if (!ClampPreset)
	{
		return DesiredValue;
	}
	// If there is no rule for this stat, return desired value as is.
	if (const FStatClampRule* Rule = ClampPreset->StatClampRules.Find(StatTag))
	{
		if (!Rule->bEnabled)
		{
			return DesiredValue;
		}

		float ClampedValue = DesiredValue;

		if (Rule->bHasMinimum)
		{
			ClampedValue = FMath::Max(ClampedValue, Rule->MinValue);
		}

		if (Rule->bMaxBoundToAnotherStat)
		{
			if (Rule->MaxBoundaryStatTag.IsValid() && HasStat(Rule->MaxBoundaryStatTag))
			{
				float DynamicMax = GetStatValue(Rule->MaxBoundaryStatTag);
				ClampedValue = FMath::Min(ClampedValue, DynamicMax);

				//Debug
				if (Rule->bLogClampEvents && ClampedValue != DesiredValue)
				{
					UE_LOG(LogRumbleCore, Verbose, TEXT("[Clamp] %s clamped to DynamicMax %f (bound to %s)"),
						*StatTag.ToString(), ClampedValue, *Rule->MaxBoundaryStatTag.ToString());
				}
			}
		}
		else if (Rule->bHasAbsoluteMax)
		{
			ClampedValue = FMath::Min(ClampedValue, Rule->AbsoluteMaxValue);
		}

		return ClampedValue;
	}

	return DesiredValue;
}

void UUniversalStatsComponent::ProcessEffectModifier(const FUniversalEffectSpec& Spec, const UUniversalStatEffect* EffectDef, const FStatModifierInfo& Mod)
{
	float FinalMagnitude = Mod.Magnitude;

	// Take modifier from the effect definition and calculate the final magnitude based on the target's current stats if needed. 
	if (EffectDef)
	{
		// 
		FinalMagnitude = EffectDef->CalculateModifierMagnitude(this, Spec.Instigator, Spec.Level, Mod.StatTag, Mod.Magnitude);
	}

	if (!FMath::IsNearlyZero(FinalMagnitude))
	{
		ModifyStat(Mod.StatTag, FinalMagnitude);
	}
}

void UUniversalStatsComponent::EvaluateTriggers(FGameplayTag StatTag, float OldValue, float NewValue)
{
	if (!TriggerPreset) return;

	for (const FStatTriggerRule& Rule : TriggerPreset->TriggerRules)
	{
		// If this rule is about the stat that changed
		if (Rule.StatToWatch == StatTag)
		{
			bool bOldConditionMet = false;
			bool bNewConditionMet = false;

			switch (Rule.Condition)
			{
			case EStatTriggerCondition::LessThanOrEqual:
				bOldConditionMet = OldValue <= Rule.Threshold;
				bNewConditionMet = NewValue <= Rule.Threshold;
				break;
			case EStatTriggerCondition::GreaterThanOrEqual:
				bOldConditionMet = OldValue >= Rule.Threshold;
				bNewConditionMet = NewValue >= Rule.Threshold;
				break;
			case EStatTriggerCondition::EqualTo:
				bOldConditionMet = FMath::IsNearlyEqual(OldValue, Rule.Threshold);
				bNewConditionMet = FMath::IsNearlyEqual(NewValue, Rule.Threshold);
				break;
			}

			// EDGE-TRIGGERING
			// If the condition was not met before but is met now, we trigger the event. 
			// This prevents continuous triggering if the stat value fluctuates around the threshold.
			if (!bOldConditionMet && bNewConditionMet)
			{
				FStatEventPayload Payload;
				Payload.TriggeringStat = StatTag;
				Payload.TriggeringValue = NewValue;

				OnGameplayEvent.Broadcast(this, Rule.EventTagToFire, Payload);
			}
		}
	}
}

FGameplayTagContainer UUniversalStatsComponent::GetAllActiveTags() const
{
	FGameplayTagContainer AllTags = BaseTags;

	for (const FActiveStatEffect& ActiveEffect : ActiveEffects)
	{
		if (ActiveEffect.EffectDef)
		{
			AllTags.AppendTags(ActiveEffect.EffectDef->GrantedTags);
		}
	}
	return AllTags;
}

bool UUniversalStatsComponent::CanApplyEffect(const UUniversalStatEffect* EffectDef) const
{
	if (!EffectDef) return false;
	//V1.1 Removed Chance func and moved to ApplyEffectToSelf to fix double roll issue. Now we just check if the target meets the tag requirements for the effect.

	// Tag Control
	FGameplayTagContainer TargetTags = GetAllActiveTags();

	if (!EffectDef->TargetMustHaveTags.IsEmpty() && !TargetTags.HasAll(EffectDef->TargetMustHaveTags))
	{
		return false; 
	}
	// Like immunities
	if (!EffectDef->TargetMustNotHaveTags.IsEmpty() && TargetTags.HasAny(EffectDef->TargetMustNotHaveTags))
	{
		return false; 
	}

	return true;
}

FGuid UUniversalStatsComponent::HandleEffectStacking(const UUniversalStatEffect* EffectDef)
{
	if (EffectDef->StackingType == EEffectStackingType::None) return FGuid(); 

	for (FActiveStatEffect& ActiveEffect : ActiveEffects)
	{
		if (EffectDef->AssetTag.IsValid() && ActiveEffect.EffectDef && ActiveEffect.EffectDef->AssetTag == EffectDef->AssetTag)
		{
			// Found same effect
			if (EffectDef->StackingType == EEffectStackingType::RefreshDuration)
			{
				ActiveEffect.TimeRemaining = EffectDef->Duration; // Refresh duration

				for (const FGameplayTag& CueTag : EffectDef->GameplayCues)
				{
					OnTagUpdated.Broadcast(this, CueTag, ActiveEffect.CurrentStackCount);
				}

				return ActiveEffect.EffectHandle; 

			}
			else if (EffectDef->StackingType == EEffectStackingType::AddStack)
			{
				ActiveEffect.CurrentStackCount = FMath::Min(ActiveEffect.CurrentStackCount + 1, EffectDef->MaxStacks); 
				ActiveEffect.TimeRemaining = EffectDef->Duration; 

				for (const FGameplayTag& CueTag : EffectDef->GameplayCues)
				{
					OnTagUpdated.Broadcast(this, CueTag, ActiveEffect.CurrentStackCount);
				}

				return ActiveEffect.EffectHandle;
			}
		}
	}
	return FGuid(); 
}

void UUniversalStatsComponent::ExecuteEffectModifiers(const FUniversalEffectSpec& Spec, const UUniversalStatEffect* EffectDef)
{
	if (!EffectDef) return;
	for (const FStatModifierInfo& Mod : EffectDef->Modifiers)
	{
		ProcessEffectModifier(Spec, EffectDef, Mod);
	}
}

float UUniversalStatsComponent::PreStatChange_Implementation(FGameplayTag StatTag, float AttemptedValue)
{
	return AttemptedValue;
}