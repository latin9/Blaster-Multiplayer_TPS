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
	// Build ���� UMG ����� �߰�, Slate, SlateCore �ּ��� ����
	UFUNCTION()
	void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void SetInputText(const FText& Text);

	FString InsertNewMessage(const FString& InputString, int32 LineLength);
};
