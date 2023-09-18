// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawnPoint.h"
#include "Pickup.h"
#include "../Weapon/Weapon.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	
	StartSpawnPickupTimer((AActor*)nullptr);
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickupSpawnPoint::SpawnPickup()
{
	int32 NumPickupClasses = PickupClasses.Num();

	if (NumPickupClasses > 0)
	{
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

		// 스폰한 픽업액터가 존재하고 서버라면
		// 생성과 동시에 StartSpawnPickupTimer함수를 바인딩한다 : 플레이어가 픽업 액터를 충돌하여 먹게되면
		// 자동으로 사라짐과 동시에 타이머가 작동이되고 다시 리스폰되는 개념이다.
		if (HasAuthority() && SpawnedPickup)
		{
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}

	/*if (NumPickupWeaponClasses > 0 && SpawnedAmmoPickup.Num() == 0)
	{
		int32 Selection = FMath::RandRange(0, NumPickupWeaponClasses - 1);
		SpawnedWeaponPickup = GetWorld()->SpawnActor<AWeapon>(PickupWeaponClasses[Selection], GetActorTransform());

		if (SpawnedAmmoPickup.Num() > 0)
			SpawnedAmmoPickup.Empty();

		int32 AmmoCount = FMath::RandRange(0, SpawnedAmmoCount);

		for (int i = 0; i < AmmoCount; ++i)
		{
			FVector NewLotation = SpawnedWeaponPickup->GetActorLocation() + 30.f * i;
			NewLotation.Z = SpawnedWeaponPickup->GetActorLocation().Z;
			APickup* AmmoPickup = GetWorld()->SpawnActor<APickup>(PickupAmmoClasses[Selection], NewLotation, SpawnedWeaponPickup->GetActorRotation());

			SpawnedAmmoPickup.Add(AmmoPickup);
		}

		if (HasAuthority() && SpawnedWeaponPickup && SpawnedAmmoPickup.Num() > 0)
		{
			SpawnedWeaponPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
			for (auto Pickup : SpawnedAmmoPickup)
			{
				Pickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
			}
		}
	}*/
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	// 서버에서만 스폰
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);

	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinished,
		SpawnTime
	);
}

