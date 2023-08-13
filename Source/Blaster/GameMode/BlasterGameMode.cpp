// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, 
	ABlasterPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
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
