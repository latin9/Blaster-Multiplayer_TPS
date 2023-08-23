
#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementcomponent.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "../Character/BlasterCharacter.h"
#include "../Blaster.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// 충돌해도 부딪히지 않는다.
	CollisionBox->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);
	// 아래 두 개의 채널에 대해서는 충돌처리 한다.
	// 벽에 부딪히기 때문
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
	
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		// 충돌체의 위치에 대한 Tracer를 생성하고 충돌체의 세계 위치를 유지한다.
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}


	// 서버에만 바인딩
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
	FVector NormalImpulse, const FHitResult& Hit)
{
	// 충돌이 됐으면 해당 프로젝타일 액터는 제거되어야함
	// 해당 액터는 bReplicates된 엑터이기 때문에
	// 서버에서 Destroy를 실행하면 Destroyed()함수가 실행되고
	// 서버에서 복제된 액터를 파괴하는 행위는 모든 클라이언트에 전달된다.
	// 위가 원래의 방법인데 클라에서 벽에 가깝거나 바닥에 쏘면 소리가 안들리고 파티클이 안 보여서
	// Multicast RPC를 사용하여 처리하였다.
	ServerDestroyFunc();

	Destroy();
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime
	);
}

void AProjectile::DestroyTimerFinished()
{
	ServerDestroyFunc();
	Destroy();
}

void AProjectile::ExplodeDamage()
{
	// 부모에서 파괴되기 때문에 먼저 처리를 해야한다.
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController && HasAuthority())
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,		 // 월드 콘텍스트 오브젝트
				Damage,			 // 베이스 데미지
				10.f,		 // 최소 데미지
				GetActorLocation(),// 위치
				DamageInnerRadius,	// 내부 범위
				DamageOuterRadius,	// 외부 범위
				1.f,			// 손상낙하?
				UDamageType::StaticClass(),	// 데미지 타입 클래스
				TArray<AActor*>(),		// IgnoreActor
				this,					// 데미지 원인이 되는 액터
				FiringController		// 선동자 컨트롤러 게임 모드에 정보를 전달하면서 컨트롤러에 접근 가능?
			);
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::Destroyed()
{
	Super::Destroyed();
}

void AProjectile::ServerDestroyFunc_Implementation()
{
	MulticastDestroyFunc();
}

void AProjectile::MulticastDestroyFunc_Implementation()
{
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

