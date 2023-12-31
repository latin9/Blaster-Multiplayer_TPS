// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "../HUD/BlasterHUD.h"
#include "../HUD/CharacterOverlay.h"
#include "Components/Progressbar.h"
#include "Components/TextBlock.h"
#include "../Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "../GameMode/BlasterGameMode.h"
#include "../GameMode/CapturePointGameMode.h"
#include "../HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "../Component/CombatComponent.h"
#include "../GameState/BlasterGameState.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"
#include "../HUD/ReturnToMainMenu.h"
#include "../HUD/ChatOverlay.h"
#include "Components/EditableTextBox.h"
#include "../BlasterType/Announcement.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = GetHUD<ABlasterHUD>();

	ServerCheckMatchState();
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
		return;

	InputComponent->BindAction(TEXT("Quit"), IE_Pressed, this, &ABlasterPlayerController::ShowReturnToMainMenu);
	InputComponent->BindAction(TEXT("UIMode"), IE_Pressed, this, &ABlasterPlayerController::ChangeUIMode);
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bShowTeamScores);
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
			//UE_LOG(LogTemp, Warning, TEXT("PlayerState->GetCompressedPing() * 4 : %d"), PlayerState->GetCompressedPing() * 4);
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

void ABlasterPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr)
		return;

	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void ABlasterPlayerController::ChangeUIMode()
{
	bUIOnlyModeEnable = !bUIOnlyModeEnable;
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValide = BlasterHUD && BlasterHUD->GetChatOverlay() && 
		BlasterHUD->GetChatOverlay()->GetChatInputText();

	if (!bHUDValide)
		return;

	if (bUIOnlyModeEnable)
	{
		SetInputMode(FInputModeGameAndUI());
		SetShowMouseCursor(true);
		BlasterHUD->GetChatOverlay()->SetVisibility(ESlateVisibility::Visible);
		BlasterHUD->GetChatOverlay()->GetChatInputText()->SetFocus();
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		BlasterHUD->GetChatOverlay()->SetVisibility(ESlateVisibility::Hidden);
		FSlateApplication::Get().ClearAllUserFocus();
	}
}

void ABlasterPlayerController::OnRep_ShowTeamScores()
{
	UE_LOG(LogTemp, Error, TEXT("OnRep_ShowTeamScores Start"));
	if (bShowTeamScores)
	{
		InitTeamScores();
		//HideHUDScore();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("bShowTeamScores is false"));
		HideTeamScores();
	}
}

void ABlasterPlayerController::LocalWeaponSelectOverlay()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	if (BlasterHUD == nullptr)
		return;

	BlasterHUD->AddWeaponSelectOverlay();
	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);
}

void ABlasterPlayerController::MulticastWeaponSelectOverlay_Implementation()
{
	LocalWeaponSelectOverlay();
}

void ABlasterPlayerController::ClientWeaponSelectOverlay_Implementation()
{
	LocalWeaponSelectOverlay();
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
		FString ShieldText = FString::Printf(TEXT("%d / %d"), FMath::FloorToInt(Shield), FMath::CeilToInt(MaxShield));
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

void ABlasterPlayerController::HideHUDScore()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->ScoreAmount &&
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount &&
		BlasterHUD->GetCharacterOverlay()->ScoreText &&
		BlasterHUD->GetCharacterOverlay()->DefeatsText;
	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->ScoreAmount->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->ScoreText->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->DefeatsText->SetText(FText());
	} 
	else
	{
		bHideHUDScore = true;
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

void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
	/*if (Attacker && Victim)
	{
		BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

		if (BlasterHUD)
		{
			BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}*/
}

void ABlasterPlayerController::SendMessage(const FText& Message)
{
	// 메세지 앞에 플레이어 이름을 넣어야한다.

	PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;

	if (PlayerState)
	{
		FString UserName = PlayerState->GetPlayerName();
		FString NewMessage = FString::Printf(TEXT("%s : %s"), *UserName, *Message.ToString());

		ServerSendChatMessage(NewMessage);
	}
}

void ABlasterPlayerController::HideTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay() && 
		BlasterHUD->GetCharacterOverlay()->RedTeamScore &&
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore &&
		BlasterHUD->GetCharacterOverlay()->ScoreSpacerText;

	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->RedTeamScore->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->ScoreSpacerText->SetText(FText());
	}
}

void ABlasterPlayerController::InitTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->RedTeamScore &&
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore &&
		BlasterHUD->GetCharacterOverlay()->ScoreSpacerText;

	if (bHUDValid)
	{
		FString Zero("0");
		FString Spacer(":");
		BlasterHUD->GetCharacterOverlay()->RedTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->GetCharacterOverlay()->ScoreSpacerText->SetText(FText::FromString(Spacer));
	}
}

void ABlasterPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->RedTeamScore;

	if (bHUDValid)
	{
		FString Score = FString::Printf(TEXT("%d"), RedScore);
		BlasterHUD->GetCharacterOverlay()->RedTeamScore->SetText(FText::FromString(Score));
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore;

	if (bHUDValid)
	{
		FString Score = FString::Printf(TEXT("%d"), BlueScore);
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore->SetText(FText::FromString(Score));
	}
}


void ABlasterPlayerController::SwitchViewToOtherPlayer()
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;

	if (BlasterGameMode)
	{
		//// 플레이어 컨트롤러의 렌더링 뷰 타겟을 변경할 플레이어 인덱스를 얻습니다.
		//int32 NewPlayerIndex = BlasterGameMode->GetNextPlayerToView();

		//// 해당 인덱스에 해당하는 플레이어 컨트롤러를 가져옵니다.
		//ABlasterPlayerController* NewPlayerController = BlasterGameMode->GetPlayerControllerByIndex(NewPlayerIndex);

		//if (NewPlayerController)
		//{
		//	// 현재 플레이어 컨트롤러의 렌더링 뷰 타겟을 변경합니다.
		//	SetViewTargetWithBlend(NewPlayerController, YourBlendTime, YourBlendFunc, YourBlendExp);
		//}
	}
}

void ABlasterPlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetPawn()->GetWorld(), APlayerController::StaticClass(), OutActors);

	// 서버에서 모든 클라이언트에게 전달한다.
	for (auto Actor : OutActors)
	{
		ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(Actor);

		if (PlayerController)
		{
			PlayerController->ClientSendChatMessage(Message);
		}
	}
}

void ABlasterPlayerController::ClientSendChatMessage_Implementation(const FString& Message)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	if (BlasterHUD && BlasterHUD->GetChatOverlay())
	{
		BlasterHUD->GetChatOverlay()->AddChatText(Message);
	}
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	if (Attacker && Victim)
	{
		BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

		if (BlasterHUD)
		{
			BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
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
				/*if (bHideHUDScore)
					HideHUDScore();*/
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
				{
					if (bInitializeGrenades)
						SetHUDGrenades(BlasterCharacter->GetCombatComponent()->GetGrenades());
				}
			}
		}
	}
	if (bShowTeamScores && bBeginValid)
	{
		bBeginValid = false;
		InitTeamScores();
	}
	else if(!bShowTeamScores && bBeginValid)
	{
		bBeginValid = false;
		HideTeamScores();
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

void ABlasterPlayerController::OnMatchStateSet(FName State, bool bTeamMatch)
{
	MatchState = State;

	// 매치 상태가 InProgress(인게임 진행)이 되었다면 그때 오버레이를 생성한다.
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamMatch);
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



void ABlasterPlayerController::HandleMatchHasStarted(bool bTeamMatch)
{
	bBeginValid = true;
	// HandleMatchHasStarted는 서버에서만 실행이 된다 그래서
	// bShowTeamScores라는 변수를 복제하여 OnRep_ShowTeamScores 함수를이용하여
	// 클라에서도 스코어 텍스트의 가려짐 판단을 할 수 있도록 여기서 매개변수로 들어온 값을 넣어주는것
	// 근데 안돼서 임시조치로 bBeginValid = true; 설정해서 PollInit에서 HUD Hide설정
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("bShowTeamScores : %d"), bShowTeamScores);
		// 기존 값이랑 동일하게 되면 OnRep함수가 실행이 안된다 변경되야만 하기 때문에 강제로 반대로 바꿔준다
		bShowTeamScores = !bShowTeamScores;
		UE_LOG(LogTemp, Error, TEXT("bShowTeamScores : %d"), bShowTeamScores);
		// 그 이후 변경할 값으로 변경
		bShowTeamScores = bTeamMatch;
		UE_LOG(LogTemp, Error, TEXT("bShowTeamScores : %d"), bShowTeamScores);
	}

	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->GetCharacterOverlay() == nullptr)
			BlasterHUD->AddCharacterOverlay();

		if (BlasterHUD->GetChatOverlay() == nullptr)
		{
			BlasterHUD->AddChatOverlay();
			if (BlasterHUD->GetChatOverlay())
				BlasterHUD->GetChatOverlay()->SetVisibility(ESlateVisibility::Hidden);
		}

		SetInputMode(FInputModeGameOnly());

		// 매치가 시작되면 기존에 있던 Announcement Widget은 안보이게 설정
		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}

		// 점령전 or 팀데스매치
		if (BlasterHUD->GetWeaponSelectOverlay() == nullptr && bShowTeamScores)
		{
			BlasterHUD->AddWeaponSelectOverlay();
			SetInputMode(FInputModeUIOnly());
			SetShowMouseCursor(true);
		}

		if (!HasAuthority())
			return;
		if (bTeamMatch)
		{
			InitTeamScores();
			//HideHUDScore();
		}
		else
		{
			HideTeamScores();
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
			FString AnnouncementText = Announcement::NewMatchStartsIn;
			BlasterHUD->GetAnnouncement()->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				
				//FString InfoTextString = GetInfoText(TopPlayers, BlasterPlayerState);
				FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(BlasterGameState) : GetInfoText(TopPlayers, BlasterPlayerState);

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

FString ABlasterPlayerController::GetInfoText(const TArray<class ABlasterPlayerState*>& TopScorePlayerStates, ABlasterPlayerState* BlasterPlayerState)
{
	FString InfoTextString;
	if (TopScorePlayerStates.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	// 우승한 사람이 본인 한명일때
	else if (TopScorePlayerStates.Num() == 1 && TopScorePlayerStates[0] == BlasterPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	// 우승한 사람이 한명인데 본인이 아닐때?
	else if (TopScorePlayerStates.Num() == 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
	}
	else if (TopScorePlayerStates.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		InfoTextString.Append(FString("\n"));
		for (auto TiedPlayer : TopScorePlayerStates)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}

	return InfoTextString;
}

FString ABlasterPlayerController::GetTeamsInfoText(ABlasterGameState* BlasterGameState)
{
	if (BlasterGameState == nullptr)
		return FString();

	FString InfoTextString;

	const int32 RedTeamScore = BlasterGameState->RedTeamScore;
	const int32 BlueTeamScore = BlasterGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWind);
		InfoTextString.Append(Announcement::RedTeam);
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(TEXT("\n"));
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if(BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d"), *Announcement::RedTeam, RedTeamScore));
	}

	return InfoTextString;
}