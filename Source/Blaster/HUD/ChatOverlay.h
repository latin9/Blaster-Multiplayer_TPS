// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UChatOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void AddChatText(const FString& Text);

private:
	UPROPERTY()
	class ABlasterPlayerController* PlayerController;

	UPROPERTY(Meta = (BindWidget))
	class UVerticalBox* ChatVerticalBox;

	UPROPERTY(Meta = (BindWidget))
	class UScrollBox* ChatHistoryBox;

	UPROPERTY(Meta = (BindWidget))
	class UEditableTextBox* ChatInputText;


private:
	// Build 파일 UMG 모듈을 추가, Slate, SlateCore 주석을 제거
	UFUNCTION()
	void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void SetInputText(const FText& Text);

	FString InsertNewMessage(const FString& InputString, int32 LineLength);
};
