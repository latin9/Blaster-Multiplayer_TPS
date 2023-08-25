// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "../Character/BlasterCharacter.h"
#include "../Component/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		UCombatComponent* Combat = BlasterCharacter->GetCombatComponent();

		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}

	Destroy();
}
