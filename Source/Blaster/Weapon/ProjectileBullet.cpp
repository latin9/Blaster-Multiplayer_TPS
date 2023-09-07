// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "../Component/LagCompensationComponent.h"
#include "GameFramework/ProjectileMovementcomponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	// �Ѿ��� �ӵ��� ���缭 ȸ��
	// �߷����� ���� ȸ���� �ش� ������ ���� ȸ����
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// ����� ������Ƽ�� �̸��� �����´�.
	FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// ����� ������Ƽ�� ���� �۾��� ����
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			// InitialSpeed�� �����Ϳ��� �����ϸ� �ڵ����� ProjectileMovementComponent->InitialSpeed, MaxSpeed �� �Ѵ� ������ �ȴ�.
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	//FPredictProjectilePathParams PathParams;
	//PathParams.bTraceWithChannel = true;	// �浹�� �����ϴ� ��� TraceChannel�� ������� �����Դϴ�.
	//PathParams.bTraceWithCollision = true;	// �浹�� �����ϰ� ù ��° ��Ʈ���� �����ϱ� ���� ��θ� ���� �������� �����Դϴ�.
	//PathParams.DrawDebugTime = 5.f;			// ����� ������ �Ⱓ
	//PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;	// ����� ����� �Ⱓ �ɼ�
	//PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;	// ���� ���� �� �ʱ� �߻� �ӵ��Դϴ�.(����� �ӵ�)
	//PathParams.MaxSimTime = 4.f;			// ���� �߻�ü�� �ִ� �ùķ��̼� �ð�
	//PathParams.ProjectileRadius = 5.f;		// �浹�� ������ �� ���Ǵ� �߻�ü �ݰ�. <= 0�̸� �� ������ ��� ���.
	//PathParams.SimFrequency = 30.f;			// ǰ�� 10 ~ 30 ���̸� ����
	//PathParams.StartLocation = GetActorLocation();	// ���� ��ġ
	//PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;	// ����� Ʈ���̽� ä��
	//PathParams.ActorsToIgnore.Add(this);	//�浹 ���� �� ������ ����

	//FPredictProjectilePathResult PathResult;

	//UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller);
		if (OwnerController)
		{
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				// Hit�ǰԵǸ� �÷��̾��� ReceiveDamage �ݹ� �Լ��� ������ ��
				UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this,
					UDamageType::StaticClass());

				// �߻�ü �ı��� �������� �Ǿ���� : �θ�� ���� �߻�ü �ı���
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}
			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompensationComponent() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
			{
				OwnerCharacter->GetLagCompensationComponent()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart, 
					InitialVelocity, 
					OwnerController->GetServerTime() - OwnerController->GetSingleTripTime()
				);
			}

		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
