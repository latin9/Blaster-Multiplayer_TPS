// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthText;
	
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ShieldBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ShieldText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ScoreAmount;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DefeatsAmount;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CarriedAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MatchCountdownText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* GrenadeText;

	UPROPERTY(meta = (BindWidget))
	class UImage* HighPingImage;

	//Transient = 디스크에 직렬화되지 않음을 의미
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* HighPingAnimation;
};
