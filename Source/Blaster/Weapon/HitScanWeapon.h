// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()


public:
	virtual void Fire(const FVector& HitTarget) override;
	
protected:
	// 샷건 분산 알고리즘
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

protected:
	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* HitSound;
	
	// 기본적으로 데미지는 발사체가 갖고있는 부분이지만
	// HitScaneWeapon은 발사체가 아닌 라인트레이스로 데미지를 주는 형식이기 때문에
	// 따로 데미지를 갖고있는것
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;
private:

	UPROPERTY(EditAnywhere)
	class UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	class USoundCue* FireSound;

	// Trace end with scatter
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;
	
	// 분산 시스템에 사용할 구체 반지름
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	// 분산 시스템 적용할건지
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;
};
