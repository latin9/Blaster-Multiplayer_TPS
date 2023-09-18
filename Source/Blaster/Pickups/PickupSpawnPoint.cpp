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

		// ������ �Ⱦ����Ͱ� �����ϰ� �������
		// ������ ���ÿ� StartSpawnPickupTimer�Լ��� ���ε��Ѵ� : �÷��̾ �Ⱦ� ���͸� �浹�Ͽ� �԰ԵǸ�
		// �ڵ����� ������� ���ÿ� Ÿ�̸Ӱ� �۵��̵ǰ� �ٽ� �������Ǵ� �����̴�.
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
	// ���������� ����
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

