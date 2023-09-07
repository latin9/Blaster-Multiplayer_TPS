// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

	UFUNCTION(Server, Reliable)
		void ServerDestroyFunc();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDestroyFunc();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void SpawnTrailSystem();
	void StartDestroyTimer();
	void DestroyTimerFinished();
	void ExplodeDamage();

protected:
	float Damage = 20.f;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;


	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

	// Server-side rewind�� ���� ����
	bool bUseServerSideRewind = false;
	// FVector_NetQuantize�� �Ҽ��� ���� �ڸ����� 0�̴�
	// ������Ҵ� �ִ� 20��Ʈ.
	// ��ȿ ���� : 2 ^ 20 = +/ -1,048,576
	FVector_NetQuantize TraceStart;
	//	FVector_NetQuantize100�� ���е��� �Ҽ��� ���� 2�ڸ��Դϴ�.
	//	������Ҵ� �ִ� 30��Ʈ.
	//	��ȿ ���� : 2 ^ 30 / 100 = +/ -10,737,418.24
	// FVector_NetQuantize100�� ����ϴ� ������ �� ������ �۾��� ���ؼ��̴�.
	// �ʱ� �ӵ��� �ӵ��� �ƴ϶� ���������� �����ϱ� �����̴�.
	FVector_NetQuantize100 InitialVelocity;
	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;
private:


	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	class UParticleSystemComponent* TracerComponent;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

public:	
	FORCEINLINE void SetUseServerSideRewind(bool _Enable) { bUseServerSideRewind = _Enable; }
	FORCEINLINE bool GetUseServerSideRewind() const { return bUseServerSideRewind; }
	FORCEINLINE void SetTraceStart(const FVector_NetQuantize& _Start) { TraceStart = _Start; }
	FORCEINLINE void SetInitialVelocity(const FVector_NetQuantize100& _Velocity) { InitialVelocity = _Velocity; }
	FORCEINLINE float GetInitialSpeed() const { return InitialSpeed; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE void SetDamage(float _Damage) { Damage = _Damage; }
};
