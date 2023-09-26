// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()
	

public:
	// 위치를 알아내기 위한 박스
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* AnnouncementAttackerBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AnnouncementAttackerText;

	// 위치를 알아내기 위한 박스
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* AnnouncementVictimBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AnnouncementVictimText;

	// 위치를 알아내기 위한 박스
	UPROPERTY(meta = (BindWidget))
		class UHorizontalBox* ElimAnnouncementImageBox;

	void SetElimAnnouncementText(FString AtttackerName, FString VictimName);


};
