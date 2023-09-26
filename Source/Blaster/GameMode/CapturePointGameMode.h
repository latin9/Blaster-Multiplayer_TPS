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

private:
	class ABlasterGameState* BlasterGameState;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController,
		class ABlasterPlayerController* AttackerController) override;

	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController) override;

	void PointCaptured(float BlueTeamCount, float RedTeamCount);
	
};
