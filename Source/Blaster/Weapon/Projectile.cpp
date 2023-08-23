
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
	// �浹�ص� �ε����� �ʴ´�.
	CollisionBox->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);
	// �Ʒ� �� ���� ä�ο� ���ؼ��� �浹ó�� �Ѵ�.
	// ���� �ε����� ����
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
	
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		// �浹ü�� ��ġ�� ���� Tracer�� �����ϰ� �浹ü�� ���� ��ġ�� �����Ѵ�.
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}


	// �������� ���ε�
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
	FVector NormalImpulse, const FHitResult& Hit)
{
	// �浹�� ������ �ش� ������Ÿ�� ���ʹ� ���ŵǾ����
	// �ش� ���ʹ� bReplicates�� �����̱� ������
	// �������� Destroy�� �����ϸ� Destroyed()�Լ��� ����ǰ�
	// �������� ������ ���͸� �ı��ϴ� ������ ��� Ŭ���̾�Ʈ�� ���޵ȴ�.
	// ���� ������ ����ε� Ŭ�󿡼� ���� �����ų� �ٴڿ� ��� �Ҹ��� �ȵ鸮�� ��ƼŬ�� �� ������
	// Multicast RPC�� ����Ͽ� ó���Ͽ���.
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
	// �θ𿡼� �ı��Ǳ� ������ ���� ó���� �ؾ��Ѵ�.
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController && HasAuthority())
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,		 // ���� ���ؽ�Ʈ ������Ʈ
				Damage,			 // ���̽� ������
				10.f,		 // �ּ� ������
				GetActorLocation(),// ��ġ
				DamageInnerRadius,	// ���� ����
				DamageOuterRadius,	// �ܺ� ����
				1.f,			// �ջ���?
				UDamageType::StaticClass(),	// ������ Ÿ�� Ŭ����
				TArray<AActor*>(),		// IgnoreActor
				this,					// ������ ������ �Ǵ� ����
				FiringController		// ������ ��Ʈ�ѷ� ���� ��忡 ������ �����ϸ鼭 ��Ʈ�ѷ��� ���� ����?
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

