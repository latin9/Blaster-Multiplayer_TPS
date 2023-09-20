// Fill out your copyright notice in the Description page of Project Settings.


#include "PointZone.h"
#include "../Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "../GameMode/CapturePointGameMode.h"

APointZone::APointZone()
{
	PrimaryActorTick.bCanEverTick = true;

}

void APointZone::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	if (World)
	{
		GameMode = World->GetAuthGameMode<ACapturePointGameMode>();
	}

	ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &APointZone::OnSphereOverlap);
}

void APointZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (BlueTeamCount > 0)
		{

		}
	}
}

void APointZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(OtherActor) : BlasterCharacter;

	if (BlasterCharacter)
	{
		if (BlasterCharacter->GetTeam() == ETeam::ET_BlueTeam)
		{
			++BlueTeamCount;
		}
		else if (BlasterCharacter->GetTeam() == ETeam::ET_RedTeam)
		{
			++RedTeamCount;
		}
	}
}
