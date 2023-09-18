// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	APickupSpawnPoint();

protected:
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupClasses;

	/*UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class AWeapon>> PickupWeaponClasses;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupAmmoClasses;*/
	
	UPROPERTY()
	class APickup* SpawnedPickup;
	/*UPROPERTY()
	class AWeapon* SpawnedWeaponPickup;
	UPROPERTY()
	TArray<class APickup*> SpawnedAmmoPickup;*/

protected:
	void SpawnPickup();
	void SpawnPickupTimerFinished();
	// OnDestroyed에 바인딩하기 위해 AActor* DestroyedActor 인자를 넣은것
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

private:
	FTimerHandle SpawnPickupTimer; 

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

	/*UPROPERTY(EditAnywhere)
	int32 SpawnedAmmoCount;*/



public:


};
