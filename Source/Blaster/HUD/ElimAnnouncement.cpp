// Fill out your copyright notice in the Description page of Project Settings.


#include "ElimAnnouncement.h"
#include "Components/TextBlock.h"

void UElimAnnouncement::SetElimAnnouncementText(FString AttackerName, FString VictimName)
{
	if (AnnouncementAttackerText)
	{
		AnnouncementAttackerText->SetText(FText::FromString(AttackerName));
	}

	if (AnnouncementVictimText)
	{
		AnnouncementVictimText->SetText(FText::FromString(VictimName));
	}
}
