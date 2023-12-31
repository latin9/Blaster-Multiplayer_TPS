// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}


void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	ENetRole RocalRole = InPawn->GetLocalRole();

	FString Role;
	switch (RocalRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy");
		break;
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	}

	FString RocalRoleString = FString::Printf(TEXT("Local Role : %s"), *Role);
	SetDisplayText(RocalRoleString);
}

void UOverheadWidget::ShowPlayerName(APawn* InPawn)
{
	if (InPawn == nullptr)
	{
		return;
	}
	APlayerState* PlayerState = InPawn->GetPlayerState();

	FString PlayerName = "Default";

	if (PlayerState)
	{
		PlayerName = PlayerState->GetPlayerName();
	}


	SetDisplayText(PlayerName);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();

	Super::NativeDestruct();


}
