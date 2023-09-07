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
			// ����
			if (InstigatorPawn->HasAuthority())
			{
				// ����, ȣ��Ʈ - ������ �߻�ü ���
				if (InstigatorPawn->IsLocallyControlled())
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->SetUseServerSideRewind(false);
					SpawnedProjectile->SetDamage(Damage);
				}
				else // ����, ���÷� ������� ���� - �������� ���� �߻�ü ����, SSR ����
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->SetUseServerSideRewind(true);
				}
			}
			else // Ŭ���̾�Ʈ SSR ���
			{
				// Ŭ���̾�Ʈ, ���� ���� - �������� ���� �߻�ü ����, SSR ���
				if (InstigatorPawn->IsLocallyControlled())
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->SetUseServerSideRewind(true);
					SpawnedProjectile->SetTraceStart(SocketTransform.GetLocation());
					SpawnedProjectile->SetInitialVelocity(SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->GetInitialSpeed());
					SpawnedProjectile->SetDamage(Damage);
				}
				else // ���÷� ������� �ʴ� Ŭ���̾�Ʈ - �������� ���� �߻�ü ����, SSR ����
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->SetUseServerSideRewind(false);
				}
			}
		}
		else // SSR ������� ����
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
