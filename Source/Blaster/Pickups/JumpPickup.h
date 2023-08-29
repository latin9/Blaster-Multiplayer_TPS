// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "JumpPickup.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AJumpPickup : public APickup
{
	GENERATED_BODY()

public:
	AJumpPickup();

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

public:
	virtual void Destroyed() override;

private:
	UPROPERTY(EditAnywhere)
	float JumpVelocity = 4000.f;

	UPROPERTY(EditAnywhere)
	float JumpBuffTime = 30.f;
	
};
