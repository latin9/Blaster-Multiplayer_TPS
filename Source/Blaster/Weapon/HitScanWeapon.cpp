// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "../Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponTypes.h"
#include "../Component/LagCompensationComponent.h"
#include "../PlayerController/BlasterPlayerController.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

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
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());

		// �ִϸ��̼��� ���� ������ ���
		// MuzzleFlash�� �����ִٸ� Fire Animation�� ���ٴ� �ǹ�(�ִ� ���� ���⸸ ��������)
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTrnasform
			);
		}
		// ���⵵ ��������
		if (FireSound)
		{
			/*UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);*/
			UGameplayStatics::SpawnSoundAttached(
				FireSound,
				RootComponent,
				FName(),
				FVector(ForceInit),
				EAttachLocation::SnapToTarget
			);
		}

		/*if (!FireHit.bBlockingHit)
			return;*/

		// Ÿ�� ���Ϳ��� ������ ��
		// �������� ���������� �߻��ؾ���
		if (BlasterCharacter && InstigatorController)
		{
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
			if (HasAuthority() && bCauseAuthDamage)
			{
				UGameplayStatics::ApplyDamage(
					BlasterCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
			if (!HasAuthority() && bUseServerSideRewind)
			{
				BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
				BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;

				if (BlasterOwnerCharacter && BlasterOwnerController &&
					BlasterOwnerCharacter->GetLagCompensationComponent() &&
					BlasterOwnerCharacter->IsLocallyControlled())
				{
					BlasterOwnerCharacter->GetLagCompensationComponent()->ServerScoreRequest
					(BlasterCharacter,
						Start,
						HitTarget,
						// ���� �ð����� �������� ������ �ð��� ���� �������Ѵ� �׷��� �������� �����Ҷ��� ������ �ð����� ���� ����
						BlasterOwnerController->GetServerTime() - BlasterOwnerController->GetSingleTripTime(),
						this
					);
				}
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
		// �ִϸ��̼��� ���� �����϶�
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
			/*UGameplayStatics::SpawnSoundAttached(
				HitSound,
				RootComponent,
				FName(),
				FireHit.ImpactPoint,
				EAttachLocation::SnapToTarget
			);*/
		}
	}
}


void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		else
		{
			OutHit.ImpactPoint = End;
		}
		FVector ToEndLoc = BeamEnd - TraceStart;
		DrawDebugSphere(GetWorld(), BeamEnd, 16.f, 12, FColor::Red, true);
		DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()),
			FColor::Cyan, true);
		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}
