// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Weapon/WeaponTypes.h"
#include "WeaponSelectOverlay.generated.h"

UCLASS()
class BLASTER_API UWeaponSelectOverlay : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;

	UFUNCTION()
	void OnWeaponSelectButtonClicked();

	UFUNCTION(Server, Reliable)
	void ServerHandleWeaponSelection();

	void HandleWeaponSelection();
	
private:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	class UButton* RifleButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	class UButton* SniperButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	class UButton* LocketButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	class UButton* ShotgunButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	class UButton* GrenadeButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	class UButton* SMGButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	class UButton* PistolButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* SelectButton;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		EMainWeapon_Type MainWeapons;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		ESubWeapon_Type SubWeapons;
};
