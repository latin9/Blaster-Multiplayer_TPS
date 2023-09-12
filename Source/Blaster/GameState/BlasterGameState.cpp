// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
	DOREPLIFETIME(ABlasterGameState, RedTeamScore);
	DOREPLIFETIME(ABlasterGameState, BlueTeamScore);
}

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);

		TopScore = ScoringPlayer->GetScore();
	}
	// 동점
	else if(ScoringPlayer->GetScore() == TopScore)
	{
		// 중복 불가능하도록
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	// 선두가 바뀜
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		// 선두가 바뀌었기 때문에 어레이 비워주고 새로운 플레이어 스테이트 추가 및 최고 스코어 점수 갱신
		TopScoringPlayers.Empty();
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABlasterGameState::OnRep_RedTeamScore()
{
	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::AddRedTeamScores()
{
	++RedTeamScore;

	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::AddBlueTeamScores()
{
	++BlueTeamScore;

	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
