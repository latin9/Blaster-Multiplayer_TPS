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

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
		return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

	// 여기서 MuzzleFlashSocket && InstigatorController로 하면 안된다
	// 컨트롤러는 자신의 pawn을 제어하기 위해 있는것이다 즉 플레이어를 위해서만 존재한다
	// 그 의미는 시뮬레이드 프록시에서는 유효하지 않다는것
	// 즉 서버에서 총을 쏘면 다른 클라에서는 해당 파티클이 보이지 않는다.
	// 다른 pc에서 본인을 제외한 다른 플레이어는 전부 시뮬레이드 프록시기 때문에
	if (MuzzleFlashSocket)
	{
		// 라인 트레이스의 시작지점
		FTransform SocketTrnasform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		FVector Start = SocketTrnasform.GetLocation();
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());

		// 애니메이션이 없는 무기일 경우
		// MuzzleFlash를 갖고있다면 Fire Animation이 없다는 의미(애님 없는 무기만 설정해줌)
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTrnasform
			);
		}
		// 여기도 마찬가지
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

		if (!FireHit.bBlockingHit)
			return;

		// 타깃 액터에게 데미지 줌
		// 데미지는 서버에서만 발생해야함
		if (BlasterCharacter && HasAuthority() && InstigatorController)
		{
			UGameplayStatics::ApplyDamage(
				BlasterCharacter,
				Damage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);
		}
		// 충돌지점에 파티클 임펙트 생성
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
		// 애니메이션이 없는 무기일때
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
