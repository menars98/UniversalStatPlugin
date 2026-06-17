// Copyright (c) 2026 [Menars]. All Rights Reserved.

#include "UniversalCueComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"       
#include "GameFramework/Actor.h"
#include "NiagaraFunctionLibrary.h"

UUniversalCueComponent::UUniversalCueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UUniversalCueComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find UUniversalStatsComponent
	if (UUniversalStatsComponent* StatsComp = GetOwner()->FindComponentByClass<UUniversalStatsComponent>())
	{
		// Bind to Cue System
		StatsComp->OnGameplayEvent.AddDynamic(this, &UUniversalCueComponent::HandleGameplayEvent);
		StatsComp->OnTagGranted.AddDynamic(this, &UUniversalCueComponent::HandleTagGranted);
		StatsComp->OnTagRemoved.AddDynamic(this, &UUniversalCueComponent::HandleTagRemoved);
	}
}

// Instant effects
void UUniversalCueComponent::HandleGameplayEvent(UUniversalStatsComponent* OwningComp, FGameplayTag EventTag, FStatEventPayload Payload)
{
	if (!CuePreset) return;

	for (const FInstantCueInfo& CueInfo : CuePreset->InstantCues)
	{
		if (CueInfo.TriggerTag == EventTag)
		{
			// Ligth Mode: If you just want to spawn a sound effect and particle effect without creating a Cue Actor, you can do it like this:
			if (CueInfo.ParticleEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CueInfo.ParticleEffect, GetOwner()->GetActorLocation());
			}

			// Ligth Mode: If you just want to spawn a sound effect and particle effect without creating a Cue Actor, you can do it like this:
			if (CueInfo.SoundEffect)
			{
				UGameplayStatics::PlaySoundAtLocation(this, CueInfo.SoundEffect, GetOwner()->GetActorLocation());
			}

			// Heavy Mode: If you want to spawn a Cue Actor and have more control over the effect (e.g., Update the intensity of the material based on the stack count), 
			// you can do it like this:
			if (CueInfo.CueClass)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = GetOwner();
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AUniversalGameplayCue* SpawnedCue = GetWorld()->SpawnActor<AUniversalGameplayCue>(
					CueInfo.CueClass, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation(), SpawnParams);

				if (SpawnedCue)
				{
					SpawnedCue->OnCueExecuted(GetOwner(), Payload.TriggeringValue);
				}
			}
		}
	}
}

// Start looping/duration effects
void UUniversalCueComponent::HandleTagGranted(UUniversalStatsComponent* OwningComp, FGameplayTag Tag)
{
	// If we are already playing a cue for this tag, don't add a second one!
	if (!CuePreset || ActiveLoopingCues.Contains(Tag)) return;

	for (const FLoopingCueInfo& CueInfo : CuePreset->LoopingCues)
	{
		if (CueInfo.TriggerTag == Tag && CueInfo.CueClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			// Create actor
			AUniversalGameplayCue* SpawnedCue = GetWorld()->SpawnActor<AUniversalGameplayCue>(
				CueInfo.CueClass, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation(), SpawnParams);

			if (SpawnedCue)
			{
				// Attach the effect to the character (If the character runs, the effect should follow)
				SpawnedCue->AttachToActor(GetOwner(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

				// Broadcast to Blueprint
				SpawnedCue->OnCueAdded(GetOwner());

				// Save it to our map in memory (TMap) so you can delete it when the time is up!
				ActiveLoopingCues.Add(Tag, SpawnedCue);
			}
		}
	}
}

// End looping/duration effects
void UUniversalCueComponent::HandleTagRemoved(UUniversalStatsComponent* OwningComp, FGameplayTag Tag)
{
	// Check if there is a working Cue for this tag on the map (TMap)
	if (AUniversalGameplayCue** FoundCue = ActiveLoopingCues.Find(Tag))
	{
		if (AUniversalGameplayCue* ValidCue = *FoundCue)
		{
			// Give Blueprint a chance to Restore the material to its original state” before deleting it.
			ValidCue->OnCueRemoved(GetOwner());

			// Completely erase the actor from the world
			//ValidCue->Destroy();
		}

		// Remove from the list on the map
		ActiveLoopingCues.Remove(Tag);
	}
}

void UUniversalCueComponent::HandleTagUpdated(UUniversalStatsComponent* OwningComp, FGameplayTag Tag, int32 NewStackCount)
{
	if (AUniversalGameplayCue** FoundCue = ActiveLoopingCues.Find(Tag))
	{
		if (AUniversalGameplayCue* ValidCue = *FoundCue)
		{
			// Broadcast to Cue Actor to update the effect based on the new stack count (Update the intensity of the material based on the stack count)
			ValidCue->OnCueUpdated(GetOwner(), NewStackCount);
		}
	}
}


