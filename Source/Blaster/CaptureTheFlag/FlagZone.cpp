// Fill out your copyright notice in the Description page of Project Settings.


#include "FlagZone.h"
#include "Components/SphereComponent.h"
#include "../Weapon/Flag.h"
#include "../GameMode/CaptureTheFlagGameMode.h"
#include "../Character/BlasterCharacter.h"

AFlagZone::AFlagZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneSphereComponent"));
	SetRootComponent(ZoneSphere);
}

void AFlagZone::BeginPlay()
{
	Super::BeginPlay();

	ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &AFlagZone::OnSphereOverlap);	
}

void AFlagZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFlag* OverlappingFlag = Cast<AFlag>(OtherActor);

	UWorld* World = GetWorld();
	if (OverlappingFlag && OverlappingFlag->GetTeam() != Team && World)
	{
		ACaptureTheFlagGameMode* GameMode = World->GetAuthGameMode<ACaptureTheFlagGameMode>();
		if (GameMode)
		{
			// 점수 증가
			GameMode->FlagCaptured(OverlappingFlag, this);
		}
		// 깃발 초기화
		OverlappingFlag->ResetFlag();
	}
}
