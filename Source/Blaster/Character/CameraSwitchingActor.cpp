// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraSwitchingActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

ACameraSwitchingActor::ACameraSwitchingActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	SetRootComponent(CameraComponent);
	PlayerControllers.Empty();

}

void ACameraSwitchingActor::BeginPlay()
{
	Super::BeginPlay();
	

}

void ACameraSwitchingActor::SwitchToPlayerView(APlayerController* PlayerController)
{
	if (!PlayerController) return;

	PlayerController->SetViewTarget(this);
}
