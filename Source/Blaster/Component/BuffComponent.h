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

	// 스피드
	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Heal(float HealAmount, float HealingTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void SetInitialSpeed(float BaseSpeed, float CrouchSpeed);

protected:
	void HealRampUp(float DeltaTime);

private:
	void ResetSpeeds();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);
};
