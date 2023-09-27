// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ATeamsGameMode : public ABlasterGameMode
{
	GENERATED_BODY()

public:
	ATeamsGameMode();

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;
	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController,
		class ABlasterPlayerController* AttackerController) override;
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController) override;
	void HandleTeamMatchStarted(AController* ElimmedController);
	
protected:
	// 경기가 시작될때 실행되는 함수
	virtual void HandleMatchHasStarted() override;


};
