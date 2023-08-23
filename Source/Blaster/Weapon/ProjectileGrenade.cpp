// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileGrenade.h"
#include "GameFramework/ProjectileMovementcomponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AProjectileGrenade::AProjectileGrenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	// 총알이 속도에 맞춰서 회전
	// 중력으로 인한 회전이 해당 궤적을 따라 회전함
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	// 발사체가 바운스함(튕김)
	ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::BeginPlay()
{
	// Actor BeginPlay를 호출하는 이유는 부모 클래스인 Projectile의 
	// BeginPlay 내부 코드를 동작하지 않기 위해서이다.
	AActor::BeginPlay();

	SpawnTrailSystem();
	StartDestroyTimer();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}


void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation()
		);
	}
}

void AProjectileGrenade::Destroyed()
{
	// 로켓처럼 익스플로전 데미지
	ExplodeDamage();

	Super::Destroyed();
}