// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()
		
public:
	AProjectileBullet();

	// 런타임이 아닌 컴파일 타임에서 확인이 된다
	// 에디터 모드라면 1이되어 PostEditChangeProperty를 사용 가능한것이다.
	// 생성자에서 ProjectileMovementComponent->MaxSpeed = InitialSpeed;와 같은 부분을 설정했는데
	// 에디터모드에서는 InitialSpeed를 바꾼다고 MaxSpeed도 같이 변경되지는 않았다
	// 하지만 아래와 같은 방식을 사용하면 변경이 가능하다.
	// 버그를 줄일 수 있음
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	
protected:
	virtual void BeginPlay() override;
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};
