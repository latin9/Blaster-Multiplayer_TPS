// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementcomponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	// �Ѿ��� �ӵ��� ���缭 ȸ��
	// �߷����� ���� ȸ���� �ش� ������ ���� ȸ����
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{
			// Hit�ǰԵǸ� �÷��̾��� ReceiveDamage �ݹ� �Լ��� ������ ��
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this,
				UDamageType::StaticClass());
		}
	}

	// �߻�ü �ı��� �������� �Ǿ���� : �θ�� ���� �߻�ü �ı���
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
