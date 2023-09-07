// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "../HUD/BlasterHUD.h"
#include "../HUD/CharacterOverlay.h"
#include "Components/Progressbar.h"
#include "Components/TextBlock.h"
#include "../Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "../GameMode/BlasterGameMode.h"
#include "../HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "../Component/CombatComponent.h"
#include "../GameState/BlasterGameState.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = GetHUD<ABlasterHUD>();
	
	ServerCheckMatchState();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}
void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();

	CheckPing(DeltaTime);
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;

	if (HighPingRunningTime >= CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;

		if (PlayerState)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerState->GetCompressedPing() * 4 : %d"), PlayerState->GetCompressedPing() * 4);
			// 언리얼 엔진의 GetCompressedPing함수는 핑을 계산하고 그 핑을 4로 나누어 uint8로 압축한다.
			// 그래서 진정한 핑을 얻으려면 4를 곱해야한다.
			// 단 GetPingInMilliseconds함수는 핑 그자체 원본을 갖고온다.
			// 임계치의 핑보다 높다면 SSR을 사용할것이고 그보다 작다면 SSR을 사용하지 않는다.
			if (PlayerState->GetCompressedPing() * 4 > HightPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}

	bool bIsHighPingAnimationPlaying = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation
		&& BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->HighPingAnimation);

	if (bIsHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;

		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

// 핑이 높다면?
void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	// 5초마다 서버와 클라 델타 계산할 수 있도록 구성
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HighPingImage
		&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation;
	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->HighPingImage->SetOpacity(1.f);
		BlasterHUD->GetCharacterOverlay()->PlayAnimation(BlasterHUD->GetCharacterOverlay()->HighPingAnimation, 0.f, 5);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HighPingImage
		&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation;
	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->HighPingAnimation))
		{
			BlasterHUD->GetCharacterOverlay()->StopAnimation(BlasterHUD->GetCharacterOverlay()->HighPingAnimation);
		}
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	// 클라이언트에서 서버로 호출을 하고 서버에서 실행되는것이기 떄문에 게임모드를 얻어올 수 있다.
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		// 플레이어 컨트롤러에게 게임모드에서 설정한 각 매치 상태의 지속시간을 알려주는것이다.
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();

		// 해당 정보들을 다시 클라이언트로 전송해야된다.
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		/*if (BlasterHUD && MatchState == MatchState::WaitingToStart)
		{
			BlasterHUD->AddAnnouncement();
		}*/
	}
}
void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	// 서버와 동일하게 진행해야하기 때문에 서버의 정보를 얻어온것
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	// 매치상태 지정
	OnMatchStateSet(MatchState);

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);

	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HealthBar
		&& BlasterHUD->GetCharacterOverlay()->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->GetCharacterOverlay()->HealthBar->SetPercent(HealthPercent);
		// 정수로 반올림해야함
		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->GetCharacterOverlay()->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->ShieldBar
		&& BlasterHUD->GetCharacterOverlay()->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->GetCharacterOverlay()->ShieldBar->SetPercent(ShieldPercent);
		// 정수로 반올림해야함
		FString ShieldText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->GetCharacterOverlay()->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() && 
		BlasterHUD->GetCharacterOverlay()->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreAmount = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->GetCharacterOverlay()->ScoreAmount->SetText(FText::FromString(ScoreAmount));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->GetCharacterOverlay()->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->GetCharacterOverlay()->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->GetCharacterOverlay()->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->GetCharacterOverlay()->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetAnnouncement() &&
		BlasterHUD->GetAnnouncement()->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->GetAnnouncement()->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->GetAnnouncement()->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GrenadeText;
	if (bHUDValid)
	{
		FString GrenadeText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->GetCharacterOverlay()->GrenadeText->SetText(FText::FromString(GrenadeText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;

	else if (MatchState == MatchState::InProgress)
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	else if (MatchState == MatchState::Cooldown)
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;

		if (BlasterGameMode)
		{
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->GetCharacterOverlay())
		{
			CharacterOverlay = BlasterHUD->GetCharacterOverlay();

			if (CharacterOverlay)
			{
				if (bInitializeHealth)
					SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield)
					SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore)
					SetHUDScore(HUDScore);
				if (bInitializeDefeats)
					SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo)
					SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo)
					SetHUDWeaponAmmo(HUDWeaponAmmo);
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
				{
					if (bInitializeGrenades)
						SetHUDGrenades(BlasterCharacter->GetCombatComponent()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	// 서버는 단순히 요청을 수신한 다음 클라이언트 RPC를 호출한다
	// 이때 요청 시간과 함꺼ㅔ 자신의 시간을 되돌려 보낸다.
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	// 왕복시간
	// 클라이언트가 서버에 RPC를 보낸 후 얼마나 많은 시간이 흘렀는지 알 수 있다.
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	// 단일 시간 왕복의 절반
	SingleTripTime = 0.5f * RoundTripTime;
	// 클라이언트에서 서버의 현재 시간을 계산
	// 서버의 시간은 총 RPC / 2만큼의 시간을 서버 시간에 더해주면 된다.
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	//서버라면 그냥 서버 시간을 리턴하고
	if (HasAuthority())
		return GetWorld()->GetTimeSeconds();
	else
		// 서버가 아니라면 현재 시간에서 클라 서버사이값을 더한다.
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	// 매치 상태가 InProgress(인게임 진행)이 되었다면 그때 오버레이를 생성한다.
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	// 매치 상태가 InProgress(인게임 진행)이 되었다면 그때 오버레이를 생성한다.
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}


void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->GetCharacterOverlay() == nullptr)
			BlasterHUD->AddCharacterOverlay();
		// 매치가 시작되면 기존에 있던 Announcement Widget은 안보이게 설정
		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->GetCharacterOverlay()->RemoveFromParent();
		bool bHUDValid = BlasterHUD->GetAnnouncement() &&
			BlasterHUD->GetAnnouncement()->AnnouncementText &&
			BlasterHUD->GetAnnouncement()->InfoText;
		// 매치가 시작되면 기존에 있던 Announcement Widget은 안보이게 설정
		if (bHUDValid)
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Start In : ");
			BlasterHUD->GetAnnouncement()->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}
				// 우승한 사람이 본인 한명일때
				else if(TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}
				// 우승한 사람이 한명인데 본인이 아닐때?
				else if(TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner : \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win : \n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
				BlasterHUD->GetAnnouncement()->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	// 여기서 bDisableGameplay을 설정하는 이유는
	// 시뮬 프록시나 신뢰할 만한 캐릭터와 같은 녀석은 Blaster HUD가 널이다
	// 이러한 캐릭터들은 HUD요소를 볼 뷰포트나 화면이 없기 때문
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());

	if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
	{
		BlasterCharacter->SetDisableGameplay(true);
		// 플레이어가 공격을 누르고 있는 상황일 수 있으니 false로 변경
		BlasterCharacter->GetCombatComponent()->FireButtonPressed(false);
	}
}
