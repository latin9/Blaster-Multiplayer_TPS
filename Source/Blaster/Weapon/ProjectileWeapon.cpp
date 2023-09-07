// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

	UWorld* World = GetWorld();
	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// Muzzle Flash Socket to hit location from TracUnderCrosshairs
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;
		
		AProjectile* SpawnedProjectile = nullptr;

		if (bUseServerSideRewind)
		{
			// 서버
			if (InstigatorPawn->HasAuthority())
			{
				// 서버, 호스트 - 복제된 발사체 사용
				if (InstigatorPawn->IsLocallyControlled())
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->SetUseServerSideRewind(false);
					SpawnedProjectile->SetDamage(Damage);
				}
				else // 서버, 로컬로 제어되지 않음 - 복제되지 않은 발사체 생성, SSR 있음
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->SetUseServerSideRewind(true);
				}
			}
			else // 클라이언트 SSR 사용
			{
				// 클라이언트, 로컬 제어 - 복제되지 않은 발사체 생성, SSR 사용
				if (InstigatorPawn->IsLocallyControlled())
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->SetUseServerSideRewind(true);
					SpawnedProjectile->SetTraceStart(SocketTransform.GetLocation());
					SpawnedProjectile->SetInitialVelocity(SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->GetInitialSpeed());
					SpawnedProjectile->SetDamage(Damage);
				}
				else // 로컬로 제어되지 않는 클라이언트 - 복제되지 않은 발사체 생성, SSR 없음
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->SetUseServerSideRewind(false);
				}
			}
		}
		else // SSR 사용하지 않음
		{
			if (InstigatorPawn->HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(false);
				SpawnedProjectile->SetDamage(Damage);
			}
		}
	}
}
