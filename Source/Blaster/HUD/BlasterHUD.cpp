// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Announcement.h"
#include "ElimAnnouncement.h"
#include "ChatOverlay.h"
#include "WeaponSelectOverlay.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();

}

void ABlasterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();

	if (PlayerController && CharacterOverlayClass)
	{
		// ĳ���� �������� ���� ����
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::AddAnnouncement()
{
	APlayerController* PlayerController = GetOwningPlayerController();

	if (PlayerController && AnnouncementClass && Announcement == nullptr)
	{
		// ĳ���� �������� ���� ����
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}

void ABlasterHUD::AddElimAnnouncement(FString Attacker, FString Victim)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;

	if (OwningPlayer && ElimAnnouncementClass)
	{
		UElimAnnouncement* ElimAnnouncementWidget = CreateWidget<UElimAnnouncement>(OwningPlayer, ElimAnnouncementClass);

		if (ElimAnnouncementWidget)
		{
			ElimAnnouncementWidget->AddToViewport();
			ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim);

			// ������ ��µǾ��ִ� �޽����� �ִٸ� ��ġ�� �ű��.
			for (UElimAnnouncement* Msg: ElimMessages)
			{
				if (Msg && Msg->AnnouncementAttackerBox)
				{
					// UWidgetLayoutLibrary�� Ȱ���Ͽ� �ش� ������ ��ġ�� �˾Ƴ� �� �ִ�.
					UCanvasPanelSlot* AttackCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->AnnouncementAttackerBox);
					UCanvasPanelSlot* VictimCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->AnnouncementVictimBox);
					UCanvasPanelSlot* KillImageCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->ElimAnnouncementImageBox);
					ElimAnnouncementChangePos(AttackCanvasSlot);
					ElimAnnouncementChangePos(VictimCanvasSlot);
					ElimAnnouncementChangePos(KillImageCanvasSlot);
				}
			}
			ElimMessages.Add(ElimAnnouncementWidget);

			FTimerHandle ElimMsgTimer;
			FTimerDelegate ElimMsgDelegate;
			ElimMsgDelegate.BindUFunction(this, FName("ElimAnnouncementTimerFinished"), ElimAnnouncementWidget);
			GetWorldTimerManager().SetTimer(
				ElimMsgTimer,
				ElimMsgDelegate,
				ElimAnnouncementTime,
				false
			);

		}
	}
}

void ABlasterHUD::AddChatOverlay()
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;

	if (OwningPlayer && ChatOverlayClass && ChatOverlay == nullptr)
	{
		ChatOverlay = CreateWidget<UChatOverlay>(OwningPlayer, ChatOverlayClass);
		if (ChatOverlay)
			ChatOverlay->AddToViewport();
	}
}

void ABlasterHUD::AddWeaponSelectOverlay()
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;

	if (OwningPlayer && WeaponSelectOverlayClass)
	{
		WeaponSelectOverlay = CreateWidget<UWeaponSelectOverlay>(OwningPlayer, WeaponSelectOverlayClass);
		if (WeaponSelectOverlay)
		{
			WeaponSelectOverlay->AddToViewport();
		}
	}
}

void ABlasterHUD::ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove)
{
	if (MsgToRemove)
	{
		MsgToRemove->RemoveFromParent();
	}
}

void ABlasterHUD::ElimAnnouncementChangePos(UCanvasPanelSlot* Slot)
{
	if (Slot)
	{
		FVector2D Pos = Slot->GetPosition();
		FVector2D NewPos(Pos.X, Pos.Y + Slot->GetSize().Y);
		Slot->SetPosition(NewPos);
	}
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		// �� �߾� ��ġ
		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairCenter)
		{
			// ���ʹ� �߾� �����̴� Spread�� ���� x
			FVector2D Spread(0.f, 0.f);
			DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairLeft)
		{
			// ������ ��, �Ʒ��� ���� ���� �ʴ´�.
			FVector2D Spread(-SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairRight)
		{
			// �������� ��, �Ʒ��� ���� ���� �ʴ´�.
			FVector2D Spread(SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairTop)
		{
			// ���� ��, �쿡 ���� ���� �ʴ´�.
			// UV��ǥ�⶧���� ���Ʒ� �ٲ�ߵ� ���� -
			FVector2D Spread(0.f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairBottom)
		{
			// �Ʒ����� ��, �쿡 ���� ���� �ʴ´�.
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
	}
}
void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	// �ؽ��ĸ� ȭ���� �� �߾ӿ� �׸��� ���ؼ� ViewportCent�� �ؽ�ó ũ���� 1/2��ŭ -���ش�
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);

	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y,
		TextureWidth, TextureHeight, 0.f, 0.f, 1.f, 1.f, CrosshairColor);
}
