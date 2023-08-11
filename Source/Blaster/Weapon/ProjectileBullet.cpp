// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{
			// Hit되게되면 플레이어의 ReceiveDamage 콜백 함수가 실행이 됨
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this,
				UDamageType::StaticClass());
		}
	}

	// 발사체 파괴는 마지막이 되어야함 : 부모로 들어가서 발사체 파괴함
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
