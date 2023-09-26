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

	// 치유되는 비율
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
	// 치료했으니깐 치료한 만큼을 뺴야한다
	AmountToHeal -= HealThisFrame;

	// 치료해야할 힐량이 0보다 작거나 플레이어 체력이 최대 체력을 넘어간경우
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
	// 치료했으니깐 치료한 만큼을 뺴야한다
	ShieldReplenishAmmount-= ShieldThisFrame;

	// 치료해야할 쉴드가 0보다 작거나 플레이어 쉴드가 최대 쉴드를 넘어간경우
	if (ShieldReplenishAmmount <= 0.f || Character->GetShield() >= Character->GetMaxShield())
	{
		bShieldReplenishing = false;
		ShieldReplenishAmmount = 0.f;
	}
}
