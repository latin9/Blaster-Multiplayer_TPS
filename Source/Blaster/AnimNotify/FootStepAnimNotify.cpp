// Fill out your copyright notice in the Description page of Project Settings.


#include "FootStepAnimNotify.h"
#include "../Character/BlasterCharacter.h"
#include "Components/AudioComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"

UFootStepAnimNotify::UFootStepAnimNotify()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> Tile(TEXT("SoundCue'/Game/Asset/Sounds/Footsteps/SCue_FS_Tile.SCue_FS_Tile'"));

	if (Tile.Succeeded())
		m_Tile = Tile.Object;


	static ConstructorHelpers::FObjectFinder<USoundBase> Grass(TEXT("SoundCue'/Game/Asset/Sounds/Footsteps/SCue_FS_Grass.SCue_FS_Grass'"));

	if (Grass.Succeeded())
		m_Grass = Grass.Object;


	static ConstructorHelpers::FObjectFinder<USoundBase> Metal(TEXT("SoundCue'/Game/Asset/Sounds/Footsteps/SCue_FS_Metal.SCue_FS_Metal'"));

	if (Metal.Succeeded())
		m_Metal = Metal.Object;


	static ConstructorHelpers::FObjectFinder<USoundBase> Dirt(TEXT("SoundCue'/Game/Asset/Sounds/Footsteps/SCue_FS_Dirt.SCue_FS_Dirt'"));

	if (Dirt.Succeeded())
		m_Dirt = Dirt.Object;


	static ConstructorHelpers::FObjectFinder<USoundBase> Snow(TEXT("SoundCue'/Game/Asset/Sounds/Footsteps/SCue_FS_Snow.SCue_FS_Snow'"));

	if (Snow.Succeeded())
		m_Snow = Snow.Object;

}

void UFootStepAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, 
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

	double Length = Character->GetVelocity().Length();

	// �޸��� or �ȱ� �ӵ��� ���� �߼Ҹ� ������ ����
	double OutValue = UKismetMathLibrary::MapRangeClamped(Length, 200.0, 600.0, 0.0, 1.0);

	// �����ͺ��� ����
	double StepSoundM = 0.5;

	OutValue *= StepSoundM;

	FHitResult HitResult;

	// �ٴ����� Lay�� �� ������ ���׸����� �Ǵ��� �ش� Step Sound�� ����ϵ��� ����
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
		VolumeMultiplier = OutValue;
		Super::Notify(MeshComp, Animation, EventReference);
	}
}

TObjectPtr<USoundBase> UFootStepAnimNotify::SelectSoundBase()
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
		return m_Dirt;
		break;
	case SurfaceType5:
		return m_Snow;
		break;
	case SurfaceType6:	// ���
		break;
	}

	return nullptr;
}

