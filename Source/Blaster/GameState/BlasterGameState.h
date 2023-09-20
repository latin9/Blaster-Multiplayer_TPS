// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class ABlasterPlayerState* ScoringPlayer);

	// 게임이 종료되는 즉시 알림 위젯에서 모든 클라이언트들에게 최고 득점을 한 플레이어가 누군지 알려주기 위함
	UPROPERTY(Replicated)
	TArray<class ABlasterPlayerState*> TopScoringPlayers;

	// 팀
	TArray<class ABlasterPlayerState*> RedTeam;

	TArray<class ABlasterPlayerState*> BlueTeam;
	
	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;
	
	UFUNCTION()
	void OnRep_RedTeamScore();

	UFUNCTION()
	void OnRep_BlueTeamScore();

	void AddRedTeamScores();
	void AddBlueTeamScores();

	void AddRedTeamScores(float Scores);
	void AddBlueTeamScores(float Scores);




private:
	float TopScore = 0.f;
};
