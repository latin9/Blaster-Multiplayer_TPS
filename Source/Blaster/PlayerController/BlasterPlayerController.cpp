// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "../HUD/BlasterHUD.h"
#include "../HUD/CharacterOverlay.h"
#include "Components/Progressbar.h"
#include "Components/TextBlock.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = GetHUD<ABlasterHUD>();
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HealthBar
		&& BlasterHUD->GetCharacterOverlay()->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->GetCharacterOverlay()->HealthBar->SetPercent(HealthPercent);
		// 정수로 반올림해야함
		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->GetCharacterOverlay()->HealthText->SetText(FText::FromString(HealthText));
	}
}
