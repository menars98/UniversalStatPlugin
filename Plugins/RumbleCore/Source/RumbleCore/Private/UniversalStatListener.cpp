// Copyright (c) 2026 [Menars]. All Rights Reserved.


#include "UniversalStatListener.h"

UUniversalStatListener* UUniversalStatListener::ListenForStatsChange(UUniversalStatsComponent* StatsComponent, TArray<FGameplayTag> StatsToWatch)
{
	// Create a new instance of the listener and set the target component and stats to watch
	UUniversalStatListener* Listener = NewObject<UUniversalStatListener>();
	Listener->TargetComponent = StatsComponent;
	Listener->WatchedStats = StatsToWatch;
	return Listener;
}

UUniversalStatListener* UUniversalStatListener::ListenForStatChange(UUniversalStatsComponent* StatsComponent, FGameplayTag StatToWatch)
{
	UUniversalStatListener* Listener = NewObject<UUniversalStatListener>();
	Listener->TargetComponent = StatsComponent;
	Listener->WatchedStats.Add(StatToWatch); 
	return Listener;
}

void UUniversalStatListener::EndTask()
{
	if (TargetComponent)
	{
		TargetComponent->OnStatChanged.RemoveDynamic(this, &UUniversalStatListener::OnComponentStatChanged);
	}
	SetReadyToDestroy();
}

void UUniversalStatListener::Activate()
{
	// Bind
	if (TargetComponent)
	{
		TargetComponent->OnStatChanged.AddDynamic(this, &UUniversalStatListener::OnComponentStatChanged);
	}
}

void UUniversalStatListener::OnComponentStatChanged(UUniversalStatsComponent* OwningComp, FGameplayTag StatTag, float OldValue, float NewValue)
{
	if (WatchedStats.Contains(StatTag))
	{
		OnStatChanged.Broadcast(this, StatTag, OldValue, NewValue);
	}
}
