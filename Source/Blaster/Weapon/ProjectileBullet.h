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

	// ��Ÿ���� �ƴ� ������ Ÿ�ӿ��� Ȯ���� �ȴ�
	// ������ ����� 1�̵Ǿ� PostEditChangeProperty�� ��� �����Ѱ��̴�.
	// �����ڿ��� ProjectileMovementComponent->MaxSpeed = InitialSpeed;�� ���� �κ��� �����ߴµ�
	// �����͸�忡���� InitialSpeed�� �ٲ۴ٰ� MaxSpeed�� ���� ��������� �ʾҴ�
	// ������ �Ʒ��� ���� ����� ����ϸ� ������ �����ϴ�.
	// ���׸� ���� �� ����
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	
protected:
	virtual void BeginPlay() override;
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};
