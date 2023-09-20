// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatOverlay.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "BlasterHUD.h"
#include "CharacterOverlay.h"

void UChatOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	ChatInputText->OnTextCommitted.AddDynamic(this, &UChatOverlay::OnChatTextCommitted);
}

void UChatOverlay::AddChatText(const FString& Text)
{
	UTextBlock* NewTextBlock = NewObject<UTextBlock>(ChatHistoryBox);
	float FontSize = NewTextBlock->GetFont().Size;	 // ��Ʈ ũ��
	UE_LOG(LogTemp, Warning, TEXT("FontSize : %d"), FontSize);

	UCanvasPanelSlot* HistoryBoxSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ChatVerticalBox);
	if (HistoryBoxSlot == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AddChatText Function : HistoryBoxSlot nullptr"));
		return;
	}

	float HistoryBoxSizeX = HistoryBoxSlot->GetSize().X;

	// �ؽ�Ʈ�� HistoryBox�� ũ�⸸ŭ�� �������� �ȴ� �׷���
	// HistoryBoxũ�⿡ ���� �ڵ����� �����ϵ��� �����ؾ��� Index(Count)�� �����ִ°�
	// +1�� ©���°� ������ ������ �δ°�
	int32 NewlineCount = HistoryBoxSizeX / FontSize + 1;

	FString NewMessage = InsertNewMessage(Text, NewlineCount);
	NewTextBlock->SetText(FText::FromString(NewMessage));

	ChatHistoryBox->AddChild(NewTextBlock);
	ChatHistoryBox->ScrollToEnd();
}

void UChatOverlay::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)) : PlayerController;

	if (PlayerController == nullptr)
		return;

	switch (CommitMethod)
	{
	case ETextCommit::OnEnter:
		// �ؽ�Ʈ�� ������� �ʴٸ�
		if (!Text.IsEmpty())
		{
			//UE_LOG(LogTemp, Warning, TEXT("Text : %s"), *Text.ToString());
			PlayerController->SendMessage(Text);
			SetInputText(FText::GetEmpty());
			//AddChatText(Text.ToString());
		}
		else // �ؽ�Ʈ�� ����ִٸ�
		{
			bool bValidHUD = PlayerController->GetBlastertHUD() &&
				PlayerController->GetBlastertHUD()->GetCharacterOverlay() && 
				PlayerController->GetBlastertHUD()->GetChatOverlay()->GetVisibility() == ESlateVisibility::Visible;
			if (bValidHUD)
			{
				PlayerController->SetInputMode(FInputModeGameOnly());
				PlayerController->SetShowMouseCursor(false);
				PlayerController->GetBlastertHUD()->GetChatOverlay()->SetVisibility(ESlateVisibility::Hidden);
				PlayerController->SetUIOnlyModeEnable(!PlayerController->GetUIOnlyModeEnable());
				FSlateApplication::Get().ClearAllUserFocus();
			}
		}
		break;
	case ETextCommit::OnUserMovedFocus:
		break;
	case ETextCommit::OnCleared:
		ChatInputText->SetFocus();
		break;
	}
}

void UChatOverlay::SetInputText(const FText& Text)
{
	ChatInputText->SetText(Text);
}

FString UChatOverlay::InsertNewMessage(const FString& InputString, int32 LineLength)
{
	FString ResultMessage;
	int32 CurrentIndex = 0;

	while (CurrentIndex < InputString.Len())
	{
		int32 SubstringLength = FMath::Min(LineLength, InputString.Len() - CurrentIndex);
		// InputString�� ������ CurrentIndex ���� SubStringLength���̸�ŭ�� string�� �̾Ƴ���.
		FString SubString = InputString.Mid(CurrentIndex, SubstringLength);

		// ���� �޼����� �̾Ƴ� ���̸�ŭ�� �޼����� �����ְ�
		ResultMessage += SubString;
		// ����
		ResultMessage += "\n";

		CurrentIndex += SubstringLength;
	}

	return ResultMessage;
}
