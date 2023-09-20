// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../BlasterType/Team.h"
#include "PointZone.generated.h"

UCLASS()
class BLASTER_API APointZone : public AActor
{
	GENERATED_BODY()
	
public:	
	APointZone();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter;

	UPROPERTY(EditAnywhere)
	class USphereComponent* ZoneSphere;

	UPROPERTY()
	class ACapturePointGameMode* GameMode;

	int32 RedTeamCount;
	int32 BlueTeamCount;

	int32 ScoreDelta;


};
