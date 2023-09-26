// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponSelectOverlay.h"
#include "Components/Button.h"
#include "../Character/BlasterCharacter.h"
#include "../Component/CombatComponent.h"
#include "../Weapon/Weapon.h"
#include "../GameMode/CapturePointGameMode.h"

bool UWeaponSelectOverlay::Initialize()
{
	if (!Super::Initialize())
		return false;

	SelectButton->OnClicked.AddDynamic(this, &UWeaponSelectOverlay::OnWeaponSelectButtonClicked);

	return true;
}

void UWeaponSelectOverlay::OnWeaponSelectButtonClicked()
{
	HandleWeaponSelection();
}

void UWeaponSelectOverlay::ServerHandleWeaponSelection_Implementation()
{
	HandleWeaponSelection();
}

void UWeaponSelectOverlay::HandleWeaponSelection()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return;
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController == nullptr)
		return;
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(PlayerController->GetCharacter());
	if (BlasterCharacter == nullptr)
		return;

	BlasterCharacter->ServerHandleWeaponSelection(MainWeapons, SubWeapons);

	RemoveFromParent();
	PlayerController->SetInputMode(FInputModeGameOnly());
	PlayerController->SetShowMouseCursor(false);
}
