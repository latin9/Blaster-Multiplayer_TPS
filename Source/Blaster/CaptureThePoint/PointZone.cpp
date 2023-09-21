// Fill out your copyright notice in the Description page of Project Settings.


#include "PointZone.h"
#include "../Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "../GameMode/CapturePointGameMode.h"

APointZone::APointZone()
{
	PrimaryActorTick.bCanEverTick = true;

	ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PointZoneBoxComponent"));
	SetRootComponent(ZoneBox);

	ZoneBox->SetCustomDepthStencilValue(CUSTOM_DEPTH_GREEN);
	ZoneBox->MarkRenderStateDirty();
	ZoneBox->SetRenderCustomDepth(true);
	
}

void APointZone::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	if (World)
	{
		GameMode = World->GetAuthGameMode<ACapturePointGameMode>();
	}

	ZoneBox->OnComponentBeginOverlap.AddDynamic(this, &APointZone::OnBoxBeginOverlap);
	ZoneBox->OnComponentEndOverlap.AddDynamic(this, &APointZone::OnBoxEndOverlap);

}

void APointZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (BlueTeamCount > 0 || RedTeamCount > 0 && BlueTeamCount != RedTeamCount)
	{
		ScoreDelta += DeltaTime;

		if (ScoreDelta >= 1.f)
		{
			if (GameMode)
			{
				ScoreDelta = 0.f;
				GameMode->PointCaptured(BlueTeamCount, RedTeamCount);
			}
		}
	}
	
}

void APointZone::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		if (BlasterCharacter->GetTeam() == ETeam::ET_BlueTeam)
		{
			++BlueTeamCount;
			UE_LOG(LogTemp, Error, TEXT("BlueTeam Begin Overlap"));
		}
		if (BlasterCharacter->GetTeam() == ETeam::ET_RedTeam)
		{
			++RedTeamCount;
			UE_LOG(LogTemp, Error, TEXT("RedTeam Begin Overlap"));
		}
	}
}

void APointZone::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		if (BlasterCharacter->GetTeam() == ETeam::ET_BlueTeam)
		{
			--BlueTeamCount;
			UE_LOG(LogTemp, Error, TEXT("BlueTeam End Overlap"));
		}
		if (BlasterCharacter->GetTeam() == ETeam::ET_RedTeam)
		{
			--RedTeamCount;
			UE_LOG(LogTemp, Error, TEXT("RedTeam End Overlap"));
		}
	}
	ScoreDelta = 0.f;
}
