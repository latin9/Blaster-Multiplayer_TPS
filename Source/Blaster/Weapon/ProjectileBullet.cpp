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
	// 총알이 속도에 맞춰서 회전
	// 중력으로 인한 회전이 해당 궤적을 따라 회전함
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 변경된 프로퍼티의 이름을 가져온다.
	FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 변경된 프로퍼티에 따라 작업을 수행
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			// InitialSpeed를 에디터에서 수정하면 자동으로 ProjectileMovementComponent->InitialSpeed, MaxSpeed 값 둘다 변경이 된다.
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
	//PathParams.bTraceWithChannel = true;	// 충돌로 추적하는 경우 TraceChannel을 사용할지 여부입니다.
	//PathParams.bTraceWithCollision = true;	// 충돌을 차단하고 첫 번째 히트에서 중지하기 위해 경로를 따라 추적할지 여부입니다.
	//PathParams.DrawDebugTime = 5.f;			// 디버그 라인의 기간
	//PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;	// 디버그 드로잉 기간 옵션
	//PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;	// 추적 시작 시 초기 발사 속도입니다.(방향과 속도)
	//PathParams.MaxSimTime = 4.f;			// 가상 발사체의 최대 시뮬레이션 시간
	//PathParams.ProjectileRadius = 5.f;		// 충돌을 추적할 때 사용되는 발사체 반경. <= 0이면 선 추적이 대신 사용.
	//PathParams.SimFrequency = 30.f;			// 품질 10 ~ 30 사이를 권장
	//PathParams.StartLocation = GetActorLocation();	// 시작 위치
	//PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;	// 사용할 트레이스 채널
	//PathParams.ActorsToIgnore.Add(this);	//충돌 추적 시 무시할 액터

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
				// Hit되게되면 플레이어의 ReceiveDamage 콜백 함수가 실행이 됨
				UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this,
					UDamageType::StaticClass());

				// 발사체 파괴는 마지막이 되어야함 : 부모로 들어가서 발사체 파괴함
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
