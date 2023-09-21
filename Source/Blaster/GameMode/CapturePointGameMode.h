// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TeamsGameMode.h"
#include "CapturePointGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ACapturePointGameMode : public ATeamsGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController,
		class ABlasterPlayerController* AttackerController) override;

	void PointCaptured(float BlueTeamCount, float RedTeamCount);
	
};
