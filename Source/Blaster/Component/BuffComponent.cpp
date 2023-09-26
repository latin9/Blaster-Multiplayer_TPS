// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "../Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
	

}


void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;

	// ġ���Ǵ� ����
	HealingRate = HealAmount / HealingTime;

	AmountToHeal += HealAmount;

}

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
	bShieldReplenishing = true;

	ShieldReplenishRate = ShieldAmount / ReplenishTime;

	ShieldReplenishAmmount += ShieldAmount;
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	if (Character == nullptr)
		return;

	Character->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,
		this,
		&UBuffComponent::ResetSpeeds,
		BuffTime
	);

	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
	if (Character == nullptr)
		return;

	Character->GetWorldTimerManager().SetTimer(
		JumpBuffTimer,
		this,
		&UBuffComponent::ResetJump,
		BuffTime
	);

	MulticastJumpBuff(BuffJumpVelocity);
}

void UBuffComponent::ResetSpeeds()
{
	if (Character == nullptr || !Character->GetCharacterMovement())
		return;

	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
	/*Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;*/
}

void UBuffComponent::ResetJump()
{
	if (Character == nullptr || !Character->GetCharacterMovement())
		return;

	MulticastJumpBuff(InitialJumpVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity)
{
	Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}


void UBuffComponent::SetInitialSpeed(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::SetInitialJumpVelocity(float JumpVelocity)
{
	InitialJumpVelocity = JumpVelocity;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || Character == nullptr || Character->IsElimmed())
		return;

	const float HealThisFrame = HealingRate * DeltaTime;
	
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0, Character->GetMaxHealth()));
	Character->UpdateHUDHealth();
	// ġ�������ϱ� ġ���� ��ŭ�� �����Ѵ�
	AmountToHeal -= HealThisFrame;

	// ġ���ؾ��� ������ 0���� �۰ų� �÷��̾� ü���� �ִ� ü���� �Ѿ���
	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (!bShieldReplenishing || Character == nullptr || Character->IsElimmed())
		return;

	const float ShieldThisFrame = ShieldReplenishRate * DeltaTime;

	Character->SetShield(FMath::Clamp(Character->GetShield() + ShieldThisFrame, 0, Character->GetMaxHealth()));

	Character->UpdateHUDShield();
	// ġ�������ϱ� ġ���� ��ŭ�� �����Ѵ�
	ShieldReplenishAmmount-= ShieldThisFrame;

	// ġ���ؾ��� ���尡 0���� �۰ų� �÷��̾� ���尡 �ִ� ���带 �Ѿ���
	if (ShieldReplenishAmmount <= 0.f || Character->GetShield() >= Character->GetMaxShield())
	{
		bShieldReplenishing = false;
		ShieldReplenishAmmount = 0.f;
	}
}
