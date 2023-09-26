// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

private:
	UPROPERTY(meta = (BindWidget))
	class UButton* ReturnButton;

	UPROPERTY(meta = (BindWidget))
	class USlider* MouseSensitivityBar;

	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* MouseSensitivityValue;

	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	UPROPERTY()
	class APlayerController* PlayerController;

protected:
	virtual bool Initialize() override;

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	UFUNCTION()
	void OnPlayerLeftGame();

	UFUNCTION()
	void ChangeSensitivityBar(float Value);

	UFUNCTION()
	void ChangeSensitivityTextValue(const FText& Text);

public:
	void MenuSetup();
	void MenuTearDown();

private:
	UFUNCTION()
	void ReturnButtonClicked();
	
};
