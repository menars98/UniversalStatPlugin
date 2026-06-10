// Copyright (c) 2026 [Menars]. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameplayTagContainer.h"
#include "UniversalStatsComponent.h"
#include "UniversalStatListener.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnStatChangedAsyncPin, UUniversalStatListener*, AsyncTask, FGameplayTag, StatTag, float, OldValue, float, NewValue);
UCLASS(BlueprintType)
class RUMBLECORE_API UUniversalStatListener : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	// Output Pin
	UPROPERTY(BlueprintAssignable)
	FOnStatChangedAsyncPin OnStatChanged;

	// Function to create an instance of this async node and set it up to listen to the specified component and stats
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Universal Stats|UI"))
	static UUniversalStatListener* ListenForStatsChange(UUniversalStatsComponent* StatsComponent, TArray<FGameplayTag> StatsToWatch);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Universal Stats|UI"))
	static UUniversalStatListener* ListenForStatChange(UUniversalStatsComponent* StatsComponent, FGameplayTag StatToWatch);

	UFUNCTION(BlueprintCallable, Category = "Universal Stats|UI")
	void EndTask();

	// BlueprintAsyncActionBase interface
	virtual void Activate() override;

private:
	UPROPERTY()
	UUniversalStatsComponent* TargetComponent;

	TArray<FGameplayTag> WatchedStats;

	// Callback function for when the component's stat change event is triggered. We will check if the changed stat is one we're watching, and if so, broadcast our own event to Blueprint.
	UFUNCTION()
	void OnComponentStatChanged(UUniversalStatsComponent* OwningComp, FGameplayTag StatTag, float OldValue, float NewValue);
};
