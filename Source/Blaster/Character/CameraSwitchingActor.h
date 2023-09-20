// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraSwitchingActor.generated.h"

UCLASS()
class BLASTER_API ACameraSwitchingActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ACameraSwitchingActor();

protected:
	virtual void BeginPlay() override;


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		class UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
		TArray<class APlayerController*> PlayerControllers;

public:
	void SwitchToPlayerView(class APlayerController* PlayerController);

};
