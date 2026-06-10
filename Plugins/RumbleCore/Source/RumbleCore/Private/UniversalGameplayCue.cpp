// Copyright (c) 2026 [Menars]. All Rights Reserved.


#include "UniversalGameplayCue.h"

AUniversalGameplayCue::AUniversalGameplayCue()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AUniversalGameplayCue::OnCueUpdated_Implementation(AActor* TargetActor, int32 NewStackCount)
{
}

void AUniversalGameplayCue::OnCueAdded_Implementation(AActor* TargetActor)
{
}

void AUniversalGameplayCue::OnCueRemoved_Implementation(AActor* TargetActor)
{
}

void AUniversalGameplayCue::OnCueExecuted_Implementation(AActor* TargetActor, float Magnitude)
{
}


