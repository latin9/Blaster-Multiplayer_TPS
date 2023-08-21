// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "../Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	// ��Ʈ��ĵ�� Fire�Լ��� �ʿ���� Weapon�� Fire�� �ʿ��ϱ� ������ �ٷ� �̵�
	AWeapon::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
		return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

	// ���⼭ MuzzleFlashSocket && InstigatorController�� �ϸ� �ȵȴ�
	// ��Ʈ�ѷ��� �ڽ��� pawn�� �����ϱ� ���� �ִ°��̴� �� �÷��̾ ���ؼ��� �����Ѵ�
	// �� �ǹ̴� �ùķ��̵� ���Ͻÿ����� ��ȿ���� �ʴٴ°�
	// �� �������� ���� ��� �ٸ� Ŭ�󿡼��� �ش� ��ƼŬ�� ������ �ʴ´�.
	// �ٸ� pc���� ������ ������ �ٸ� �÷��̾�� ���� �ùķ��̵� ���Ͻñ� ������
	if (MuzzleFlashSocket)
	{
		// ���� Ʈ���̽��� ��������
		FTransform SocketTrnasform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		FVector Start = SocketTrnasform.GetLocation();

		uint32 Hits = 0;

		// �Ѿ��� ���������� ������ ������ ���� ����� �� ���� ���� �� �ִ�.
		TMap<ABlasterCharacter*, uint32> HitMap;
		for (uint32 i = 0; i < NumberOfPellets; ++i)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());

			// Ÿ�� ���Ϳ��� ������ ��
			// �������� ���������� �߻��ؾ���
			// ���� ��ó�� ��� ź������ �浹ó���� ���ε��� ó���ϴ°��� �ƴ�
			// ���� ������ �����Ͽ� �� ���� ����Ѵ�.
			if (BlasterCharacter && HasAuthority() && InstigatorController)
			{
				// ������ Ÿ���� ĳ���Ͱ� �ִٸ� �ش� ĳ������ HitCount 1����
				if (HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					// ùŸ�ݽ�
					HitMap.Emplace(BlasterCharacter, 1);
				}
			}
			// �浹������ ��ƼŬ ����Ʈ ����
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}
			// ������ �Ѿ��� �������� �� ���� ������ ������ �Ҹ� ����
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					0.5f,
					FMath::FRandRange(-0.5f, 0.5f)
				);
			}
		}
		// ��� ĳ���Ϳ� ���� ������
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				// �� �÷��̾�� ���� Ƚ����ŭ �������� �ش�.
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
}
