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

void UChatOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	ChatInputText->OnTextCommitted.AddDynamic(this, &UChatOverlay::OnChatTextCommitted);
}

void UChatOverlay::AddChatText(const FString& Text)
{
	UTextBlock* NewTextBlock = NewObject<UTextBlock>(ChatHistoryBox);
	float FontSize = NewTextBlock->Font.Size;	 // 폰트 크기
	UE_LOG(LogTemp, Warning, TEXT("FontSize : %d"), FontSize);

	UCanvasPanelSlot* HistoryBoxSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ChatVerticalBox);
	if (HistoryBoxSlot == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AddChatText Function : HistoryBoxSlot nullptr"));
		return;
	}

	float HistoryBoxSizeX = HistoryBoxSlot->GetSize().X;

	// +1은 짤리는거 방지용 여유를 두는것
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
		// 텍스트가 비어있지 않다면
		if (!Text.IsEmpty())
		{
			//UE_LOG(LogTemp, Warning, TEXT("Text : %s"), *Text.ToString());
			PlayerController->SendMessage(Text);
			SetInputText(FText::GetEmpty());
			//AddChatText(Text.ToString());
		}
		break;
	case ETextCommit::OnUserMovedFocus:
		break;
	case ETextCommit::OnCleared:
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
		// InputString의 내용중 CurrentIndex 부터 SubStringLength길이만큼의 string을 뽑아낸다.
		FString SubString = InputString.Mid(CurrentIndex, SubstringLength);

		// 최종 메세지에 뽑아낸 길이만큼의 메세지를 더해주고
		ResultMessage += SubString;
		// 개행
		ResultMessage += "\n";

		CurrentIndex += SubstringLength;
	}

	return ResultMessage;
}
