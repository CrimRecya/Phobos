#include "Body.h"

#include <TacticalClass.h>
#include <SpawnManagerClass.h>
#include <FactoryClass.h>
#include <SuperClass.h>
#include <Ext/SWType/Body.h>

#include <Utilities/EnumFunctions.h>

void TechnoExt::DrawSelfHealPips(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBounds)
{
	if (!RulesExt::Global()->GainSelfHealAllowMultiplayPassive && pThis->Owner->Type->MultiplayPassive)
		return;

	bool drawPip = false;
	bool isInfantryHeal = false;
	int selfHealFrames = 0;

	if (auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType()))
	{
		if (pExt->SelfHealGainType.isset() && pExt->SelfHealGainType.Get() == SelfHealGainType::None)
			return;

		bool hasInfantrySelfHeal = pExt->SelfHealGainType.isset() && pExt->SelfHealGainType.Get() == SelfHealGainType::Infantry;
		bool hasUnitSelfHeal = pExt->SelfHealGainType.isset() && pExt->SelfHealGainType.Get() == SelfHealGainType::Units;
		bool isOrganic = false;

		if (pThis->WhatAmI() == AbstractType::Infantry ||
			pThis->GetTechnoType()->Organic && pThis->WhatAmI() == AbstractType::Unit)
		{
			isOrganic = true;
		}

		if (pThis->Owner->InfantrySelfHeal > 0 && (hasInfantrySelfHeal || (isOrganic && !hasUnitSelfHeal)))
		{
			drawPip = true;
			selfHealFrames = RulesClass::Instance->SelfHealInfantryFrames;
			isInfantryHeal = true;
		}
		else if (pThis->Owner->UnitsSelfHeal > 0 && (hasUnitSelfHeal || (pThis->WhatAmI() == AbstractType::Unit && !isOrganic)))
		{
			drawPip = true;
			selfHealFrames = RulesClass::Instance->SelfHealUnitFrames;
		}
	}

	if (drawPip)
	{
		Valueable<Point2D> pipFrames;
		bool isSelfHealFrame = false;
		int xOffset = 0;
		int yOffset = 0;

		if (Unsorted::CurrentFrame % selfHealFrames <= 5
			&& pThis->Health < pThis->GetTechnoType()->Strength)
		{
			isSelfHealFrame = true;
		}

		if (pThis->WhatAmI() == AbstractType::Unit || pThis->WhatAmI() == AbstractType::Aircraft)
		{
			auto& offset = RulesExt::Global()->Pips_SelfHeal_Units_Offset.Get();
			pipFrames = RulesExt::Global()->Pips_SelfHeal_Units;
			xOffset = offset.X;
			yOffset = offset.Y + pThis->GetTechnoType()->PixelSelectionBracketDelta;
		}
		else if (pThis->WhatAmI() == AbstractType::Infantry)
		{
			auto& offset = RulesExt::Global()->Pips_SelfHeal_Infantry_Offset.Get();
			pipFrames = RulesExt::Global()->Pips_SelfHeal_Infantry;
			xOffset = offset.X;
			yOffset = offset.Y + pThis->GetTechnoType()->PixelSelectionBracketDelta;
		}
		else
		{
			auto pType = static_cast<BuildingClass*>(pThis)->Type;
			int fHeight = pType->GetFoundationHeight(false);
			int yAdjust = -Unsorted::CellHeightInPixels / 2;

			auto& offset = RulesExt::Global()->Pips_SelfHeal_Buildings_Offset.Get();
			pipFrames = RulesExt::Global()->Pips_SelfHeal_Buildings;
			xOffset = offset.X + Unsorted::CellWidthInPixels / 2 * fHeight;
			yOffset = offset.Y + yAdjust * fHeight + pType->Height * yAdjust;
		}

		int pipFrame = isInfantryHeal ? pipFrames.Get().X : pipFrames.Get().Y;

		Point2D position = { pLocation->X + xOffset, pLocation->Y + yOffset };

		auto flags = BlitterFlags::bf_400 | BlitterFlags::Centered;

		if (isSelfHealFrame)
			flags = flags | BlitterFlags::Darken;

		DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPS_SHP,
		pipFrame, &position, pBounds, flags, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}
}

void TechnoExt::DrawInsignia(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBounds)
{
	Point2D offset = *pLocation;

	SHPStruct* pShapeFile = FileSystem::PIPS_SHP;
	int defaultFrameIndex = -1;

	auto pTechnoType = pThis->GetTechnoType();
	auto pOwner = pThis->Owner;

	if (pThis->IsDisguised() && !pThis->IsClearlyVisibleTo(HouseClass::CurrentPlayer) && !(HouseClass::IsCurrentPlayerObserver()
		|| EnumFunctions::CanTargetHouse(RulesExt::Global()->DisguiseBlinkingVisibility, HouseClass::CurrentPlayer, pOwner)))
	{
		if (auto const pType = TechnoTypeExt::GetTechnoType(pThis->Disguise))
		{
			pTechnoType = pType;
			pOwner = pThis->DisguisedAsHouse;
		}
	}

	TechnoTypeExt::ExtData* pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pTechnoType);

	bool isVisibleToPlayer = (pOwner && pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
		|| HouseClass::IsCurrentPlayerObserver()
		|| pTechnoTypeExt->Insignia_ShowEnemy.Get(RulesExt::Global()->EnemyInsignia);

	if (!isVisibleToPlayer)
		return;

	bool isCustomInsignia = false;

	if (SHPStruct* pCustomShapeFile = pTechnoTypeExt->Insignia.Get(pThis))
	{
		pShapeFile = pCustomShapeFile;
		defaultFrameIndex = 0;
		isCustomInsignia = true;
	}

	VeterancyStruct* pVeterancy = &pThis->Veterancy;
	auto insigniaFrames = pTechnoTypeExt->InsigniaFrames.Get();
	int insigniaFrame = insigniaFrames.X;
	int frameIndex = pTechnoTypeExt->InsigniaFrame.Get(pThis);

	if (pTechnoType->Gunner)
	{
		int weaponIndex = pThis->CurrentWeaponNumber;

		if (auto const pCustomShapeFile = pTechnoTypeExt->Insignia_Weapon[weaponIndex].Get(pThis))
		{
			pShapeFile = pCustomShapeFile;
			defaultFrameIndex = 0;
			isCustomInsignia = true;
		}

		int frame = pTechnoTypeExt->InsigniaFrame_Weapon[weaponIndex].Get(pThis);

		if (frame != -1)
			frameIndex = frame;

		auto& frames = pTechnoTypeExt->InsigniaFrames_Weapon[weaponIndex];

		if (frames != Vector3D<int>(-1, -1, -1))
			insigniaFrames = frames;
	}

	if (pVeterancy->IsVeteran())
	{
		defaultFrameIndex = !isCustomInsignia ? 14 : defaultFrameIndex;
		insigniaFrame = insigniaFrames.Y;
	}
	else if (pVeterancy->IsElite())
	{
		defaultFrameIndex = !isCustomInsignia ? 15 : defaultFrameIndex;
		insigniaFrame = insigniaFrames.Z;
	}

	frameIndex = frameIndex == -1 ? insigniaFrame : frameIndex;

	if (frameIndex == -1)
		frameIndex = defaultFrameIndex;

	if (frameIndex != -1 && pShapeFile)
	{
		switch (pThis->WhatAmI())
		{
		case AbstractType::Infantry:
			offset += RulesExt::Global()->DrawInsignia_AdjustPos_Infantry;
			break;
		case AbstractType::Building:
			if (RulesExt::Global()->DrawInsignia_AdjustPos_BuildingsAnchor.isset())
				offset = GetBuildingSelectBracketPosition(pThis, RulesExt::Global()->DrawInsignia_AdjustPos_BuildingsAnchor) + RulesExt::Global()->DrawInsignia_AdjustPos_Buildings;
			else
				offset += RulesExt::Global()->DrawInsignia_AdjustPos_Buildings;
			break;
		default:
			offset += RulesExt::Global()->DrawInsignia_AdjustPos_Units;
			break;
		}

		DSurface::Temp->DrawSHP(
			FileSystem::PALETTE_PAL, pShapeFile, frameIndex, &offset, pBounds, BlitterFlags(0xE00), 0, -2, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	return;
}

void TechnoExt::DrawFactoryProgress(TechnoClass* pThis, RectangleStruct* pBounds)
{
	if (pThis->WhatAmI() != AbstractType::Building)
		return;

	RulesExt::ExtData* const pRulesExt = RulesExt::Global();

	if (!pRulesExt->FactoryProgressDisplay)
		return;

	BuildingClass* const pBuilding = abstract_cast<BuildingClass*>(pThis);
	BuildingTypeClass* const pBuildingType = pBuilding->Type;
	HouseClass* const pHouse = pBuilding->Owner;
	FactoryClass* pPrimaryFactory = nullptr;
	FactoryClass* pSecondaryFactory = nullptr;

	if (pHouse->IsControlledByHuman())
	{
		if (!pBuilding->IsPrimaryFactory)
			return;

		switch (pBuildingType->Factory)
		{
		case AbstractType::BuildingType:
			pPrimaryFactory = pHouse->GetPrimaryFactory(AbstractType::BuildingType, false, BuildCat::DontCare);
			pSecondaryFactory = pHouse->GetPrimaryFactory(AbstractType::BuildingType, false, BuildCat::Combat);
			break;
		case AbstractType::InfantryType:
			pPrimaryFactory = pHouse->GetPrimaryFactory(AbstractType::InfantryType, false, BuildCat::Combat);
			break;
		case AbstractType::UnitType:
			pPrimaryFactory = pHouse->GetPrimaryFactory(AbstractType::UnitType, pBuildingType->Naval, BuildCat::Combat);
			break;
		case AbstractType::AircraftType:
			pPrimaryFactory = pHouse->GetPrimaryFactory(AbstractType::AircraftType, false, BuildCat::Combat);
			break;
		default:
			return;
		}
	}
	else // AIs have no Primary factories
	{
		pPrimaryFactory = pBuilding->Factory;

		if (!pPrimaryFactory)
			return;
	}

	const bool havePrimary = pPrimaryFactory && pPrimaryFactory->Object;
	const bool haveSecondary = pSecondaryFactory && pSecondaryFactory->Object;

	if (!havePrimary && !haveSecondary)
		return;

	const int maxLength = pBuildingType->GetFoundationHeight(false) * 15 >> 1;
	const Point2D location =
		TechnoExt::GetBuildingSelectBracketPosition(pBuilding, BuildingSelectBracketPosition::Top)
		+ Point2D { 5, (3 + pBuildingType->PixelSelectionBracketDelta) }
		+ pRulesExt->FactoryProgressDisplay_Offset.Get();

	if (havePrimary)
	{
		Point2D position = location;

		DrawFrameStruct pDraw
		{
			Math::clamp(static_cast<int>((static_cast<double>(pPrimaryFactory->GetProgress()) / 54) * maxLength), 0, maxLength),
			pRulesExt->FactoryProgressDisplay_Pips,
			0,
			-1,
			maxLength,
			0,
			pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(FileSystem::PIPS_SHP),
			FileSystem::PIPS_SHP,
			FileSystem::PALETTE_PAL,
			&position,
			pBounds
		};

		TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
	}

	if (haveSecondary)
	{
		Point2D position = havePrimary ? location + Point2D { 6, 3 } : location;

		DrawFrameStruct pDraw
		{
			Math::clamp(static_cast<int>((static_cast<double>(pSecondaryFactory->GetProgress()) / 54) * maxLength), 0, maxLength),
			pRulesExt->FactoryProgressDisplay_Pips,
			0,
			-1,
			maxLength,
			0,
			pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(FileSystem::PIPS_SHP),
			FileSystem::PIPS_SHP,
			FileSystem::PALETTE_PAL,
			&position,
			pBounds
		};

		TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
	}
}

void TechnoExt::DrawSuperProgress(TechnoClass* pThis, RectangleStruct* pBounds)
{
	if (pThis->WhatAmI() != AbstractType::Building)
		return;

	RulesExt::ExtData* const pRulesExt = RulesExt::Global();

	if (!pRulesExt->MainSWProgressDisplay)
		return;

	HouseClass* const pOwner = pThis->Owner;

	if (pOwner == HouseClass::FindSpecial() || pOwner == HouseClass::FindNeutral() || pOwner == HouseClass::FindCivilianSide())
		return;

	BuildingClass* const pBuilding = abstract_cast<BuildingClass*>(pThis);
	BuildingTypeClass* const pBuildingType = pBuilding->Type;
	const int superIndex = pBuildingType->SuperWeapon;
	SuperClass* const pSuper = (superIndex != -1) ? pThis->Owner->Supers.GetItem(superIndex) : nullptr;
	const int timeLeft = pSuper->RechargeTimer.TimeLeft;

	if (!pSuper || !timeLeft || !SWTypeExt::ExtMap.Find(pSuper->Type)->IsAvailable(pThis->Owner))
		return;

	const int maxLength = pBuildingType->GetFoundationHeight(false) * 15 >> 1;
	Point2D position = TechnoExt::GetBuildingSelectBracketPosition(pBuilding, BuildingSelectBracketPosition::Top) + Point2D { 5, 3 + pBuildingType->PixelSelectionBracketDelta };
	position += pRulesExt->MainSWProgressDisplay_Offset.Get();

	DrawFrameStruct pDraw
	{
		Math::clamp(static_cast<int>((static_cast<double>(timeLeft - pSuper->RechargeTimer.GetTimeLeft()) / timeLeft) * maxLength), 0, maxLength),
		pRulesExt->MainSWProgressDisplay_Pips,
		0,
		-1,
		maxLength,
		0,
		pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(FileSystem::PIPS_SHP),
		FileSystem::PIPS_SHP,
		FileSystem::PALETTE_PAL,
		&position,
		pBounds
	};

	TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
}

void TechnoExt::DrawIronCurtainProgress(TechnoClass* pThis, RectangleStruct* pBounds)
{
	if (!pThis->IsIronCurtained() || !pThis->IronCurtainTimer.TimeLeft)
		return;

	RulesExt::ExtData* const pRulesExt = RulesExt::Global();

	if (!pRulesExt->InvulnerableDisplay)
		return;

	if (pThis->WhatAmI() == AbstractType::Building)
	{
		BuildingClass* const pBuilding = abstract_cast<BuildingClass*>(pThis);
		BuildingTypeClass* const pBuildingType = pBuilding->Type;
		const int maxLength = pBuildingType->GetFoundationHeight(false) * 15 >> 1;
		const HealthState healthStatus = pBuilding->GetHealthStatus();
		Point2D position = TechnoExt::GetBuildingSelectBracketPosition(pBuilding, BuildingSelectBracketPosition::Top) + Point2D{ -1, pBuildingType->PixelSelectionBracketDelta };
		position += pRulesExt->InvulnerableDisplay_Buildings_Offset.Get();

		DrawFrameStruct pDraw
		{
			Math::clamp(static_cast<int>((static_cast<double>(pThis->IronCurtainTimer.GetTimeLeft()) / pThis->IronCurtainTimer.TimeLeft) * maxLength), 0, maxLength),
			(pThis->ForceShielded ? pRulesExt->InvulnerableDisplay_Buildings_Pips.Get().X : pRulesExt->InvulnerableDisplay_Buildings_Pips.Get().Y),
			Math::clamp(static_cast<int>(pBuilding->GetHealthPercentage() * maxLength), 0, maxLength),
			((pThis->IsSelected || pThis->IsMouseHovering) ? (healthStatus == HealthState::Green ? 1 : (healthStatus == HealthState::Yellow ? 2 : 4)) : 0),
			maxLength,
			0,
			pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(FileSystem::PIPS_SHP),
			FileSystem::PIPS_SHP,
			FileSystem::PALETTE_PAL,
			&position,
			pBounds
		};

		TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
	}
	else
	{
		const int maxLength = pThis->WhatAmI() != AbstractType::Infantry ? 17 : 8;
		Point2D position = TechnoExt::GetFootSelectBracketPosition(pThis, Anchor(HorizontalPosition::Left, VerticalPosition::Top)) + Point2D{ -1, pThis->GetTechnoType()->PixelSelectionBracketDelta + 2 };
		position += pRulesExt->InvulnerableDisplay_Others_Offset.Get();

		DrawFrameStruct pDraw
		{
			Math::clamp(static_cast<int>((static_cast<double>(pThis->IronCurtainTimer.GetTimeLeft()) / pThis->IronCurtainTimer.TimeLeft) * maxLength), 0, maxLength),
			(pThis->ForceShielded ? pRulesExt->InvulnerableDisplay_Others_Pips.Get().X : pRulesExt->InvulnerableDisplay_Others_Pips.Get().Y),
			0,
			-1,
			maxLength,
			-1,
			pRulesExt->ProgressDisplay_Others_PipsShape.Get(FileSystem::PIPS_SHP),
			FileSystem::PIPBRD_SHP,
			FileSystem::PALETTE_PAL,
			&position,
			pBounds
		};

		TechnoExt::DrawVanillaStyleFootBar(&pDraw);
	}
}

void TechnoExt::DrawVanillaStyleFootBar(DrawFrameStruct* pDraw)
{
	Point2D* pLocation = pDraw->Location;

	if (pDraw->BrdFrame >= 0)
	{
		pLocation->X += 18;
		DSurface::Temp->DrawSHP(pDraw->Palette, pDraw->BrdSHP, pDraw->BrdFrame, pLocation, pDraw->Bounds, BlitterFlags(0xE00), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		pLocation->X -= 16;
	}
	else
	{
		pLocation->X += 2;
	}

	pLocation->Y += 1;
	int length = pDraw->TopLength;

	if (pDraw->TopFrame >= 0)
	{
		for (int drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X += 2)
			DSurface::Temp->DrawSHP(pDraw->Palette, pDraw->PipSHP, pDraw->TopFrame, pLocation, pDraw->Bounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	length = pDraw->MidLength - length;

	if (pDraw->MidFrame >= 0)
	{
		for (int drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X += 2)
			DSurface::Temp->DrawSHP(pDraw->Palette, pDraw->PipSHP, pDraw->MidFrame, pLocation, pDraw->Bounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}
}

void TechnoExt::DrawVanillaStyleBuildingBar(DrawFrameStruct* pDraw)
{
	Point2D* pLocation = pDraw->Location;
	int length = pDraw->TopLength;

	if (pDraw->TopFrame >= 0)
	{
		for (int drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X -= 4, pLocation->Y += 2)
			DSurface::Temp->DrawSHP(pDraw->Palette, pDraw->PipSHP, pDraw->TopFrame, pLocation, pDraw->Bounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	length = pDraw->MidLength - length;

	if (pDraw->MidFrame >= 0)
	{
		for (int drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X -= 4, pLocation->Y += 2)
			DSurface::Temp->DrawSHP(pDraw->Palette, pDraw->BrdSHP, pDraw->MidFrame, pLocation, pDraw->Bounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	length = length >= 0 ? pDraw->MaxLength - pDraw->TopLength - length : pDraw->MaxLength - pDraw->TopLength;

	if (pDraw->BrdFrame >= 0)
	{
		for (int drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X -= 4, pLocation->Y += 2)
			DSurface::Temp->DrawSHP(pDraw->Palette, pDraw->BrdSHP, pDraw->BrdFrame, pLocation, pDraw->Bounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}
}

Point2D TechnoExt::GetScreenLocation(TechnoClass* pThis)
{
	return TacticalClass::Instance->CoordsToClient(pThis->GetCoords()).first;
}

Point2D TechnoExt::GetFootSelectBracketPosition(TechnoClass* pThis, Anchor anchor)
{
	int length = 17;
	Point2D position = GetScreenLocation(pThis);

	if (pThis->WhatAmI() == AbstractType::Infantry)
		length = 8;

	RectangleStruct bracketRect =
	{
		position.X - length + (length == 8) + 1,
		position.Y - 28 + (length == 8),
		length * 2,
		length * 3
	};

	return anchor.OffsetPosition(bracketRect);
}

Point2D TechnoExt::GetBuildingSelectBracketPosition(TechnoClass* pThis, BuildingSelectBracketPosition bracketPosition)
{
	const auto pBuildingType = static_cast<BuildingTypeClass*>(pThis->GetTechnoType());
	Point2D position = GetScreenLocation(pThis);
	CoordStruct dim2 = CoordStruct::Empty;
	pBuildingType->Dimension2(&dim2);
	dim2 = { -dim2.X / 2, dim2.Y / 2, dim2.Z };
	Point2D positionFix = TacticalClass::CoordsToScreen(dim2);

	const int foundationWidth = pBuildingType->GetFoundationWidth();
	const int foundationHeight = pBuildingType->GetFoundationHeight(false);
	const int height = pBuildingType->Height * 12;
	const int lengthW = foundationWidth * 7 + foundationWidth / 2;
	const int lengthH = foundationHeight * 7 + foundationHeight / 2;

	position.X += positionFix.X + 3 + lengthH * 4;
	position.Y += positionFix.Y + 4 - lengthH * 2;

	switch (bracketPosition)
	{
	case BuildingSelectBracketPosition::Top:
		break;
	case BuildingSelectBracketPosition::LeftTop:
		position.X -= lengthH * 4;
		position.Y += lengthH * 2;
		break;
	case BuildingSelectBracketPosition::LeftBottom:
		position.X -= lengthH * 4;
		position.Y += lengthH * 2 + height;
		break;
	case BuildingSelectBracketPosition::Bottom:
		position.Y += lengthW * 2 + lengthH * 2 + height;
		break;
	case BuildingSelectBracketPosition::RightBottom:
		position.X += lengthW * 4;
		position.Y += lengthW * 2 + height;
		break;
	case BuildingSelectBracketPosition::RightTop:
		position.X += lengthW * 4;
		position.Y += lengthW * 2;
	default:
		break;
	}

	return position;
}

void TechnoExt::ProcessDigitalDisplays(TechnoClass* pThis)
{
	if (!Phobos::Config::DigitalDisplay_Enable)
		return;

	const auto pType = pThis->GetTechnoType();
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	if (pTypeExt->DigitalDisplay_Disable)
		return;

	const auto pExt = TechnoExt::ExtMap.Find(pThis);
	int length = 17;
	ValueableVector<DigitalDisplayTypeClass*>* pDisplayTypes = nullptr;

	if (!pTypeExt->DigitalDisplayTypes.empty())
	{
		pDisplayTypes = &pTypeExt->DigitalDisplayTypes;
	}
	else
	{
		switch (pThis->WhatAmI())
		{
		case AbstractType::Building:
		{
			pDisplayTypes = &RulesExt::Global()->Buildings_DefaultDigitalDisplayTypes;
			const auto pBuildingType = static_cast<BuildingTypeClass*>(pThis->GetTechnoType());
			const int height = pBuildingType->GetFoundationHeight(false);
			length = height * 7 + height / 2;
			break;
		}
		case AbstractType::Infantry:
		{
			pDisplayTypes = &RulesExt::Global()->Infantry_DefaultDigitalDisplayTypes;
			length = 8;
			break;
		}
		case AbstractType::Unit:
		{
			pDisplayTypes = &RulesExt::Global()->Vehicles_DefaultDigitalDisplayTypes;
			break;
		}
		case AbstractType::Aircraft:
		{
			pDisplayTypes = &RulesExt::Global()->Aircraft_DefaultDigitalDisplayTypes;
			break;
		}
		default:
			return;
		}
	}

	for (DigitalDisplayTypeClass*& pDisplayType : *pDisplayTypes)
	{
		if (HouseClass::IsCurrentPlayerObserver() && !pDisplayType->VisibleToHouses_Observer)
			continue;

		if (!HouseClass::IsCurrentPlayerObserver() && !EnumFunctions::CanTargetHouse(pDisplayType->VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer))
			continue;

		int value = -1;
		int maxValue = 0;

		GetValuesForDisplay(pThis, pDisplayType->InfoType, value, maxValue);

		if (value == -1 || maxValue == 0)
			continue;

		if (pDisplayType->ValueScaleDivisor > 1)
		{
			value = Math::max(value / pDisplayType->ValueScaleDivisor, value != 0 ? 1 : 0);
			maxValue = Math::max(maxValue / pDisplayType->ValueScaleDivisor, maxValue != 0 ? 1 : 0);
		}

		const bool isBuilding = pThis->WhatAmI() == AbstractType::Building;
		const bool isInfantry = pThis->WhatAmI() == AbstractType::Infantry;
		const bool hasShield = pExt->Shield != nullptr && !pExt->Shield->IsBrokenAndNonRespawning();
		Point2D position = pThis->WhatAmI() == AbstractType::Building ?
			GetBuildingSelectBracketPosition(pThis, pDisplayType->AnchorType_Building)
			: GetFootSelectBracketPosition(pThis, pDisplayType->AnchorType);
		position.Y += pType->PixelSelectionBracketDelta;

		if (pDisplayType->InfoType == DisplayInfoType::Shield)
			position.Y += pExt->Shield->GetType()->BracketDelta;

		pDisplayType->Draw(position, length, value, maxValue, isBuilding, isInfantry, hasShield);
	}
}

void TechnoExt::GetValuesForDisplay(TechnoClass* pThis, DisplayInfoType infoType, int& value, int& maxValue)
{
	const auto pType = pThis->GetTechnoType();
	const auto pExt = TechnoExt::ExtMap.Find(pThis);

	switch (infoType)
	{
	case DisplayInfoType::Health:
	{
		value = pThis->Health;
		maxValue = pType->Strength;
		break;
	}
	case DisplayInfoType::Shield:
	{
		if (pExt->Shield == nullptr || pExt->Shield->IsBrokenAndNonRespawning())
			return;

		value = pExt->Shield->GetHP();
		maxValue = pExt->Shield->GetType()->Strength.Get();
		break;
	}
	case DisplayInfoType::Ammo:
	{
		if (pType->Ammo <= 0)
			return;

		value = pThis->Ammo;
		maxValue = pType->Ammo;
		break;
	}
	case DisplayInfoType::MindControl:
	{
		if (pThis->CaptureManager == nullptr)
			return;

		value = pThis->CaptureManager->ControlNodes.Count;
		maxValue = pThis->CaptureManager->MaxControlNodes;
		break;
	}
	case DisplayInfoType::Spawns:
	{
		if (pThis->SpawnManager == nullptr || pType->Spawns == nullptr || pType->SpawnsNumber <= 0)
			return;

		value = pThis->SpawnManager->CountAliveSpawns();
		maxValue = pType->SpawnsNumber;
		break;
	}
	case DisplayInfoType::Passengers:
	{
		if (pType->Passengers <= 0)
			return;

		value = pThis->Passengers.NumPassengers;
		maxValue = pType->Passengers;
		break;
	}
	case DisplayInfoType::Tiberium:
	{
		if (pType->Storage <= 0)
			return;

		value = static_cast<int>(pThis->Tiberium.GetTotalAmount());
		maxValue = pType->Storage;
		break;
	}
	case DisplayInfoType::Experience:
	{
		value = static_cast<int>(pThis->Veterancy.Veterancy * RulesClass::Instance->VeteranRatio * pType->GetCost());
		maxValue = static_cast<int>(2.0 * RulesClass::Instance->VeteranRatio * pType->GetCost());
		break;
	}
	case DisplayInfoType::Occupants:
	{
		if (pThis->WhatAmI() != AbstractType::Building)
			return;

		const auto pBuildingType = abstract_cast<BuildingTypeClass*>(pType);
		const auto pBuilding = abstract_cast<BuildingClass*>(pThis);

		if (!pBuildingType->CanBeOccupied)
			return;

		value = pBuilding->Occupants.Count;
		maxValue = pBuildingType->MaxNumberOccupants;
		break;
	}
	case DisplayInfoType::GattlingStage:
	{
		if (!pType->IsGattling)
			return;

		value = pThis->GattlingValue == 0 ? 0 : pThis->CurrentGattlingStage + 1;
		maxValue = pType->WeaponStages;
		break;
	}
	case DisplayInfoType::ROF:
	{
		if (!pThis->GetWeapon(0) || !pThis->GetWeapon(0)->WeaponType)
			return;

		value = pThis->RearmTimer.GetTimeLeft();
		maxValue = pThis->ChargeTurretDelay;
		break;
	}
	case DisplayInfoType::Reload:
	{
		if (pType->Ammo <= 0)
			return;

		value = (pThis->Ammo >= pType->Ammo) ? 0 : pThis->ReloadTimer.GetTimeLeft();
		maxValue = pThis->ReloadTimer.TimeLeft;
		break;
	}
	case DisplayInfoType::SpawnTimer:
	{
		if (pThis->SpawnManager == nullptr || pType->Spawns == nullptr || pType->SpawnsNumber <= 0)
			return;

		value = 0;
		maxValue = pThis->SpawnManager->RegenRate;

		for (int i = 0; i < pType->SpawnsNumber; i++)
		{
			if (pThis->SpawnManager->SpawnedNodes[i]->Status == SpawnNodeStatus::Dead)
			{
				int thisValue = pThis->SpawnManager->SpawnedNodes[i]->SpawnTimer.GetTimeLeft();

				if (thisValue < value)
					value = thisValue;
			}
		}

		break;
	}
	case DisplayInfoType::GattlingTimer:
	{
		if (!pType->IsGattling)
			return;

		const int thisStage = pThis->CurrentGattlingStage;

		if (pThis->Veterancy.IsElite())
		{
			if (thisStage > 0)
			{
				value = pThis->GattlingValue - pType->EliteStage[thisStage - 1];
				maxValue = pType->EliteStage[thisStage] - pType->EliteStage[thisStage - 1];
			}
			else
			{
				value = pThis->GattlingValue;
				maxValue = pType->EliteStage[thisStage];
			}
		}
		else
		{
			if (thisStage > 0)
			{
				value = pThis->GattlingValue - pType->WeaponStage[thisStage - 1];
				maxValue = pType->WeaponStage[thisStage] - pType->WeaponStage[thisStage - 1];
			}
			else
			{
				value = pThis->GattlingValue;
				maxValue = pType->WeaponStage[thisStage];
			}
		}

		break;
	}
	case DisplayInfoType::ProduceCash:
	{
		if (pThis->WhatAmI() != AbstractType::Building)
			return;

		const auto pBuildingType = abstract_cast<BuildingTypeClass*>(pType);
		const auto pBuilding = abstract_cast<BuildingClass*>(pThis);

		if (pBuildingType->ProduceCashAmount <= 0)
			return;

		value = pBuilding->CashProductionTimer.GetTimeLeft();
		maxValue = pBuilding->CashProductionTimer.TimeLeft;
		break;
	}
	case DisplayInfoType::PassengerKill:
	{
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		if (!pTypeExt || !pTypeExt->PassengerDeletionType)
			return;

		value = pExt->PassengerDeletionTimer.GetTimeLeft();
		maxValue = pExt->PassengerDeletionTimer.TimeLeft;
		break;
	}
	case DisplayInfoType::AutoDeath:
	{
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		if (!pTypeExt || !pTypeExt->AutoDeath_Behavior.isset())
			return;

		if (pTypeExt->AutoDeath_AfterDelay > 0)
		{
			value = pExt->AutoDeathTimer.GetTimeLeft();
			maxValue = pExt->AutoDeathTimer.TimeLeft;
		}
		else if (pTypeExt->AutoDeath_OnAmmoDepletion && pType->Ammo > 0)
		{
			value = pThis->Ammo;
			maxValue = pType->Ammo;
		}

		break;
	}
	case DisplayInfoType::SuperWeapon:
	{
		if (pThis->WhatAmI() != AbstractType::Building || !pThis->Owner)
			return;

		const auto pBuildingType = abstract_cast<BuildingTypeClass*>(pType);
		const auto pBuildingTypeExt = BuildingTypeExt::ExtMap.Find(pBuildingType);
		SuperClass* pSuper = nullptr;

		if (pBuildingType->SuperWeapon != -1)
			pSuper = pThis->Owner->Supers.GetItem(pBuildingType->SuperWeapon);
		else if (pBuildingType->SuperWeapon2 != -1)
			pSuper = pThis->Owner->Supers.GetItem(pBuildingType->SuperWeapon2);
		else if (pBuildingTypeExt->SuperWeapons.size() > 0)
			pSuper = pThis->Owner->Supers.GetItem(pBuildingTypeExt->SuperWeapons[0]);

		if (pSuper)
		{
			value = pSuper->RechargeTimer.GetTimeLeft();
			maxValue = pSuper->RechargeTimer.TimeLeft;
		}

		break;
	}
	default:
	{
		value = pThis->Health;
		maxValue = pType->Strength;
		break;
	}
	}
}
