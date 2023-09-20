// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpEndAnimNotify.h"
#include "../Character/BlasterCharacter.h"
#include "Components/AudioComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"

UJumpEndAnimNotify::UJumpEndAnimNotify()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> Tile(TEXT("SoundCue'/Game/Asset/Sounds/Foley/SCue_Foley_Jumpland_Tile.SCue_Foley_Jumpland_Tile'"));

	if (Tile.Succeeded())
		m_Tile = Tile.Object;


	static ConstructorHelpers::FObjectFinder<USoundBase> Grass(TEXT("SoundCue'/Game/Asset/Sounds/Foley/SCue_Foley_Jumpland_Grass.SCue_Foley_Jumpland_Grass'"));

	if (Grass.Succeeded())
		m_Grass = Grass.Object;


	static ConstructorHelpers::FObjectFinder<USoundBase> Metal(TEXT("SoundCue'/Game/Asset/Sounds/Foley/SCue_Foley_Jumpland_Metal.SCue_Foley_Jumpland_Metal'"));

	if (Metal.Succeeded())
		m_Metal = Metal.Object;


	static ConstructorHelpers::FObjectFinder<USoundBase> Dirt(TEXT("SoundCue'/Game/Asset/Sounds/Footsteps/SCue_FS_Dirt.SCue_FS_Dirt'"));

	if (Dirt.Succeeded())
		m_Dirt = Dirt.Object;


	static ConstructorHelpers::FObjectFinder<USoundBase> Snow(TEXT("SoundCue'/Game/Asset/Sounds/Footsteps/SCue_FS_Snow.SCue_FS_Snow'"));

	if (Snow.Succeeded())
		m_Snow = Snow.Object;

}

void UJumpEndAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	ABlasterCharacter* Character = MeshComp->GetOwner<ABlasterCharacter>();

	if (!Character || !Character->IsValidLowLevel())
	{
		return;
	}

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.AddIgnoredComponent(Character->GetCapsuleComponent());
	CollisionParams.bReturnPhysicalMaterial = true;

	FVector StartPos = MeshComp->GetOwner()->GetActorLocation();
	FVector EndPos = StartPos;
	EndPos.Z -= 300.f;

	FHitResult HitResult;

	// 바닥으로 Lay를 쏴 피직스 메테리얼을 판단후 해당 Step Sound를 출력하도록 구현
	if (Character->GetWorld()->LineTraceSingleByChannel(HitResult, StartPos, EndPos,
		ECollisionChannel::ECC_Visibility, CollisionParams))
	{
		m_StepSurface = UGameplayStatics::GetSurfaceType(HitResult);

		if (!m_StepSurface)
		{
			UE_LOG(LogTemp, Error, TEXT("FootStep Surface Null!"));
		}

		Sound = SelectSoundBase();
		if (Sound == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("FootStep Sound Null!"));
			return;
		}

		Super::Notify(MeshComp, Animation, EventReference);
	}


}

TObjectPtr<USoundBase> UJumpEndAnimNotify::SelectSoundBase()
{
	switch (m_StepSurface)
	{
	case SurfaceType_Default:
		break;
	case SurfaceType1:
		return  m_Tile;
		break;
	case SurfaceType2:
		return  m_Grass;
		break;
	case SurfaceType3:
		return  m_Metal;
		break;
	case SurfaceType4:
		return m_Grass;
		break;
	case SurfaceType5:
		return m_Grass;
		break;
	case SurfaceType6:
		break;
	}

	return nullptr;
}

