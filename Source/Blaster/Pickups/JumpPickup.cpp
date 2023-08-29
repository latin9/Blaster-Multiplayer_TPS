// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpPickup.h"
#include "../Character/BlasterCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "../Component/BuffComponent.h"

AJumpPickup::AJumpPickup()
{
	bReplicates = true;

	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("JumpPickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		UBuffComponent* Buff = BlasterCharacter->GetBuffComponent();

		if (Buff)
		{
			Buff->BuffJump(JumpVelocity, JumpBuffTime);
		}
	}

	Destroy();
}

void AJumpPickup::Destroyed()
{
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
	Super::Destroyed();
}
