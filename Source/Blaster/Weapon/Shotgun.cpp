// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "../Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"


void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	// 히트스캔의 Fire함수는 필요없고 Weapon의 Fire만 필요하기 때문에 바로 이동
	AWeapon::Fire(FVector());

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
		const FTransform SocketTrnasform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTrnasform.GetLocation();

		// 총알이 여러곳으로 퍼지기 때문에 여러 사람이 한 번에 맞을 수 있다.
		// Key : 총알에 맞은사람, Value : 총알에 몇방 맞았는지
		TMap<ABlasterCharacter*, uint32> HitMap;

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			// 타깃 액터에게 데미지 줌
			// 데미지는 서버에서만 발생해야함
			// 기존 총처럼 모든 탄에대한 충돌처리를 따로따로 처리하는것이 아닌
			// 맞은 개수를 종합하여 한 번에 계산한다.
			if (BlasterCharacter)
			{
				// 기존에 타격한 캐릭터가 있다면 해당 캐릭터의 HitCount 1증가
				if (HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					// 첫타격시
					HitMap.Emplace(BlasterCharacter, 1);
				}
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
			// 샷건은 총알이 여러발이 한 번에 나오기 때문에 소리 조절
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
		// 모든 캐릭터에 대한 데미지
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				// 각 플레이어마다 맞은 횟수만큼 데미지를 준다.
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

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	// 여기서 MuzzleFlashSocket && InstigatorController로 하면 안된다
	// 컨트롤러는 자신의 pawn을 제어하기 위해 있는것이다 즉 플레이어를 위해서만 존재한다
	// 그 의미는 시뮬레이드 프록시에서는 유효하지 않다는것
	// 즉 서버에서 총을 쏘면 다른 클라에서는 해당 파티클이 보이지 않는다.
	// 다른 pc에서 본인을 제외한 다른 플레이어는 전부 시뮬레이드 프록시기 때문에
	if (MuzzleFlashSocket == nullptr)
		return;

	// 라인 트레이스의 시작지점
	const FTransform SocketTrnasform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTrnasform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; ++i)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		const FVector ToEndLoc = EndLoc - TraceStart;
		const FVector TargetPos = FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());

		HitTargets.Add(TargetPos);
	}
}
