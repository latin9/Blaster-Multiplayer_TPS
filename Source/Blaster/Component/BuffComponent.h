// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class ABlasterCharacter;

public:	
	UBuffComponent();


private:
	UPROPERTY()
		class ABlasterCharacter* Character;

	// 체력
	bool bHealing = false;
	// 치료 비율
	float HealingRate = 0.f;
	// 치료되어야할 회복량?
	float AmountToHeal = 0.f;

	// 쉴드
	bool bShieldReplenishing = false;
	float ShieldReplenishRate = 0.f;
	float ShieldReplenishAmmount = 0.f;

	// 스피드
	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	// 점프
	FTimerHandle JumpBuffTimer;
	float InitialJumpVelocity;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Heal(float HealAmount, float HealingTime);
	void ReplenishShield(float ShieldAmount, float ReplenishTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffJump(float BuffJumpVelocity, float  BuffTime);
	void SetInitialSpeed(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float JumpVelocity);

protected:
	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);

private:
	void ResetSpeeds();
	void ResetJump();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);
};
