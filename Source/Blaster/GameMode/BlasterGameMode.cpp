// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "../GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	// 아래의 변수를 true로 설정하면
	// 게임모드의 상태는 기다리는 상태로 남고 실제로 디폴트 폰을 모든 플레이어에게 생성한다.
	// 그러면 이걸 이용해 레벨을 날아다닐 수 있음
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// BlasterGameMode는 게임 메뉴가 있는 게임 시작 맵이 아닌 BlasterMap에서만 사용되기 때문에
	// 게임을 시작할 때부터 BlasterMap에 실제로 들어가기까지 얼마나 많은 시간이 걸렸는지 알 수 있다.
	// 즉 맵에 들어가기까지 걸린 시간
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*iter);
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
	ABlasterPlayerController* AttackerController)
{
	// 공격자 플레이어 상태
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	// 피해자 플레이어 상태
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (AttackerPlayerState == nullptr)  UE_LOG(LogTemp, Warning, TEXT("AttackerPlayerState 유효하지 않음"));
	if (AttackerPlayerState == VictimPlayerState)  UE_LOG(LogTemp, Warning, TEXT("AttackerPlayerState == VictimPlayerState"));

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		TArray<ABlasterPlayerState*> PlayersCurrentlyInTheLead;

		// 현재 상태의 최고 득점자를 얻어놓는다. (왕관 이펙트를 사용하는 사람은 탑 스코어 뿐이기 때문)
		for (auto LeadPlayer : BlasterGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}
		AttackerPlayerState->AddToScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);

		// 승점을 올리고 업데이트 이후 공격한 플레이어가 Top이라면 최고 선두라는 의미 = 왕관 이펙트 생성
		if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());

			if (Leader)
			{
				Leader->MulticastGainedTheLead();
			}
		}

		// 스코어 업데이트 이후 변동되었다면 선두가 뺏겼다는 의미 왕관 제거
		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); ++i)
		{
			if (!BlasterGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				ABlasterCharacter* Loser = Cast<ABlasterCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		// 게임을 나간것이 아닌 맞아서 죽은것이기 때문
		ElimmedCharacter->Elim(false);
	}

	// 모든 플레이어에게 킬로그 정보를 전달
	for (FConstPlayerControllerIterator iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
	{
		ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(*iter);

		if (PlayerController && AttackerPlayerState && VictimPlayerState)
		{
			PlayerController->BroadcastElim(AttackerPlayerState, VictimPlayerState); 
		}
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		// 리셋을 먼저 하고 제거해야된다.
		// 폰의 상태를 초기 상태로 재설정하기 위해 호출되는것
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
		//Cast<ABlasterCharacter>(ElimmedCharacter)->ServerElimDestroyed();
	}

	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		// 플레이어 재시작
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr)
		return;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	// 게임스테이트의 탑 스코을 새로 갱신해줘야 한다. 플레이어가 삭제되면(게임에서 나가면) 해당 탑 스코어는 널이 되기 때문
	if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		BlasterGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}

	ABlasterCharacter* CharacterLeaving = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn());
	// 플레이어가 게임을 종료한것
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}
