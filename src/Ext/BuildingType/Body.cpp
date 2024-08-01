#include "Body.h"

#include <Ext/House/Body.h>
#include <Utilities/GeneralUtils.h>
#include <Ext/SWType/Body.h>

BuildingTypeExt::ExtContainer BuildingTypeExt::ExtMap;

// Assuming SuperWeapon & SuperWeapon2 are used (for the moment)
int BuildingTypeExt::ExtData::GetSuperWeaponCount() const
{
	// The user should only use SuperWeapon and SuperWeapon2 if the attached sw count isn't bigger than 2
	return 2 + this->SuperWeapons.size();
}

int BuildingTypeExt::ExtData::GetSuperWeaponIndex(const int index, HouseClass* pHouse) const
{
	auto idxSW = this->GetSuperWeaponIndex(index);

	if (auto pSuper = pHouse->Supers.GetItemOrDefault(idxSW))
	{
		auto pExt = SWTypeExt::ExtMap.Find(pSuper->Type);

		if (!pExt->IsAvailable(pHouse))
			return -1;
	}

	return idxSW;
}

int BuildingTypeExt::ExtData::GetSuperWeaponIndex(const int index) const
{
	const auto pThis = this->OwnerObject();

	// 2 = SuperWeapon & SuperWeapon2
	if (index < 2)
		return !index ? pThis->SuperWeapon : pThis->SuperWeapon2;
	else if (index - 2 < (int)this->SuperWeapons.size())
		return this->SuperWeapons[index - 2];

	return -1;
}

int BuildingTypeExt::GetEnhancedPower(BuildingClass* pBuilding, HouseClass* pHouse)
{
	int nAmount = 0;
	float fFactor = 1.0f;

	auto const pHouseExt = HouseExt::ExtMap.Find(pHouse);

	for (const auto& [pExt, nCount] : pHouseExt->PowerPlantEnhancers)
	{
		if (pExt->PowerPlantEnhancer_Buildings.Contains(pBuilding->Type))
		{
			fFactor *= std::powf(pExt->PowerPlantEnhancer_Factor, static_cast<float>(nCount));
			nAmount += pExt->PowerPlantEnhancer_Amount * nCount;
		}
	}

	return static_cast<int>(std::round(pBuilding->GetPowerOutput() * fFactor)) + nAmount;
}

int BuildingTypeExt::GetUpgradesAmount(BuildingTypeClass* pBuilding, HouseClass* pHouse) // not including producing upgrades
{
	int result = 0;
	bool isUpgrade = false;
	auto pPowersUp = pBuilding->PowersUpBuilding;

	auto checkUpgrade = [pHouse, pBuilding, &result, &isUpgrade](BuildingTypeClass* pTPowersUp)
	{
		isUpgrade = true;
		for (auto const& pBld : pHouse->Buildings)
		{
			if (pBld->Type == pTPowersUp)
			{
				for (auto const& pUpgrade : pBld->Upgrades)
				{
					if (pUpgrade == pBuilding)
						++result;
				}
			}
		}
	};

	if (pPowersUp[0])
	{
		if (auto const pTPowersUp = BuildingTypeClass::Find(pPowersUp))
			checkUpgrade(pTPowersUp);
	}

	if (auto pBuildingExt = BuildingTypeExt::ExtMap.Find(pBuilding))
	{
		for (auto pTPowersUp : pBuildingExt->PowersUp_Buildings)
			checkUpgrade(pTPowersUp);
	}

	return isUpgrade ? result : -1;
}

// Force occupiers leave, return: whether it should stop right now
bool BuildingTypeExt::CleanUpBuildingSpace(CellStruct topLeftCell, CellStruct foundationCell, HouseClass* pHouse, TechnoClass* pExceptTechno)
{
	// Step 1: Find the technos inside of the building place grid.
	CellStruct infantryCount { 0, 0 };
	std::vector<TechnoClass*> checkedTechnos;
	checkedTechnos.reserve(24);
	std::vector<CellClass*> checkedCells;
	checkedCells.reserve(24);
	const CellStruct cellBottomRight = topLeftCell + foundationCell;

	for (short curX = topLeftCell.X; curX < cellBottomRight.X; ++curX)
	{
		for (short curY = topLeftCell.Y; curY < cellBottomRight.Y; ++curY)
		{
			if (CellClass* const pCell = MapClass::Instance->GetCellAt(CellStruct{curX, curY}))
			{
				ObjectClass* pObject = pCell->FirstObject;

				while (pObject)
				{
					AbstractType const absType = pObject->WhatAmI();

					if (absType == AbstractType::Infantry || absType == AbstractType::Unit)
					{
						TechnoClass* const pCellTechno = static_cast<TechnoClass*>(pObject);
						auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pCellTechno->GetTechnoType());

						if ((!pTypeExt || !pTypeExt->CanBeBuiltOn) && pCellTechno != pExceptTechno)
						{
							if (static_cast<FootClass*>(pCellTechno)->GetCurrentSpeed() <= 0)
							{
								if (absType == AbstractType::Infantry)
									++infantryCount.X;

								checkedTechnos.push_back(pCellTechno);
							}
						}
					}

					pObject = pObject->NextObject;
				}

				checkedCells.push_back(pCell);
			}
		}
	}

	if (checkedTechnos.size() <= 0) // All in moving
		return false;

	// Step 2: Find the cells around the building.
	std::vector<CellClass*> optionalCells;
	optionalCells.reserve(24);
	CellStruct surroundCell { topLeftCell.X - 1, topLeftCell.Y - 1 };

	for (int i = 0; i < 4; ++i)
	{
		const int range = (i % 2) ? foundationCell.Y : foundationCell.X;

		for (int j = 0; j <= range; ++j)
		{
			if (CellClass* const pSurroundCell = MapClass::Instance->GetCellAt(surroundCell))
			{
				if (!pSurroundCell->GetBuilding() && pSurroundCell->IsClearToMove(SpeedType::Amphibious, true, true, -1, MovementZone::Amphibious, -1, false))
					optionalCells.push_back(pSurroundCell);
			}

			if (i % 2)
				surroundCell.Y += static_cast<short>((i / 2) ? -1 : 1);
			else
				surroundCell.X += static_cast<short>((i / 2) ? -1 : 1);
		}
	}

	if (optionalCells.size() <= 0) // There is no place for scattering
		return true;

	// Step 3: Sort the technos by the distance out of the foundation.
	std::sort(&checkedTechnos[0], &checkedTechnos[checkedTechnos.size()],[optionalCells](TechnoClass* pTechnoA, TechnoClass* pTechnoB)
	{
		int minA = INT_MAX;
		int minB = INT_MAX;

		for (auto const& pOptionalCell : optionalCells) // If there are many valid cells at start, it means most of occupiers will near to the edge
		{
			if (minA > 65536) // If distance squared is lower or equal to 256^2, then no need to calculate any more because it is on the edge
			{
				int curA = static_cast<int>(pTechnoA->GetMapCoords().DistanceFromSquared(pOptionalCell->MapCoords));

				if (curA < minA)
					minA = curA;
			}

			if (minB > 65536)
			{
				int curB = static_cast<int>(pTechnoB->GetMapCoords().DistanceFromSquared(pOptionalCell->MapCoords));

				if (curB < minB)
					minB = curB;
			}
		}

		return minA > minB;
	});

	// Step 4: Core, successively find the farthest techno and its closest valid destination.
	std::vector<TechnoClass*> reCheckedTechnos;
	reCheckedTechnos.reserve(12);

	struct InfantryCountInCell // Temporary struct
	{
		CellClass* position;
		int count;
	};
	std::vector<InfantryCountInCell> infantryCells;
	infantryCells.reserve(4);

	struct TechnoWithDestination // Also temporary struct
	{
		TechnoClass* techno;
		CellClass* destination;
	};
	std::vector<TechnoWithDestination> finalOrder;
	finalOrder.reserve(24);

	do
	{
		// Step 4.1: Push the technos discovered just now back to the vector.
		for (auto const& pRecheckedTechno : reCheckedTechnos)
		{
			if (pRecheckedTechno->WhatAmI() == AbstractType::Infantry)
				++infantryCount.X;

			checkedTechnos.push_back(pRecheckedTechno);
		}

		reCheckedTechnos.clear();

		// Step 4.2: Check the techno vector.
		for (auto const& pCheckedTechno : checkedTechnos)
		{
			CellClass* pDestinationCell = nullptr;

			// Step 4.2.1: Search the closest valid cell to be the destination.
			do
			{
				const CellStruct location = pCheckedTechno->GetMapCoords();
				const bool isInfantry = pCheckedTechno->WhatAmI() == AbstractType::Infantry;
				TechnoTypeClass* const pCheckedType = pCheckedTechno->GetTechnoType();

				if (isInfantry) // Try to maximizing cells utilization
				{
					if (infantryCells.size() > 0 && infantryCount.Y >= (infantryCount.X / 3 + (infantryCount.X % 3 ? 1 : 0)))
					{
						std::sort(&infantryCells[0], &infantryCells[infantryCells.size()],[location](InfantryCountInCell cellA, InfantryCountInCell cellB){
							return cellA.position->MapCoords.DistanceFromSquared(location) < cellB.position->MapCoords.DistanceFromSquared(location);
						});

						for (auto& infantryCell : infantryCells)
						{
							if (infantryCell.count < 3 && infantryCell.position->IsClearToMove(pCheckedType->SpeedType, true, true, -1, pCheckedType->MovementZone, -1, false))
							{
								pDestinationCell = infantryCell.position;
								++infantryCell.count;

								break;
							}
						}

						if (pDestinationCell)
							break; // Complete
					}
				}

				std::sort(&optionalCells[0], &optionalCells[optionalCells.size()],[location](CellClass* pCellA, CellClass* pCellB){
					return pCellA->MapCoords.DistanceFromSquared(location) < pCellB->MapCoords.DistanceFromSquared(location);
				});
				const double minDistanceSquared = optionalCells[0]->MapCoords.DistanceFromSquared(location);

				for (auto const& pOptionalCell : optionalCells) // Prioritize selecting empty cells
				{
					if (!pOptionalCell->FirstObject && pOptionalCell->IsClearToMove(pCheckedType->SpeedType, true, true, -1, pCheckedType->MovementZone, -1, false))
					{
						if (isInfantry) // Not need to remove it now
						{
							infantryCells.push_back(InfantryCountInCell{ pOptionalCell, 1 });
							++infantryCount.Y;
						}

						if (pOptionalCell->MapCoords.DistanceFromSquared(location) < (minDistanceSquared * 4)) // Empty cell is not too far
							pDestinationCell = pOptionalCell;

						break;
					}
				}

				if (!pDestinationCell)
				{
					std::vector<CellClass*> deleteCells;
					deleteCells.reserve(8);

					for (auto const& pOptionalCell : optionalCells)
					{
						ObjectClass* pCurObject = pOptionalCell->FirstObject;
						std::vector<TechnoClass*> optionalTechnos;
						optionalTechnos.reserve(4);
						bool valid = true;

						while (pCurObject)
						{
							AbstractType const absType = pCurObject->WhatAmI();

							if (absType == AbstractType::Infantry || absType == AbstractType::Unit)
							{
								TechnoClass* const pCurTechno = static_cast<TechnoClass*>(pCurObject);

								if (pCurTechno->Owner != pHouse) // Means invalid for all
								{
									deleteCells.push_back(pOptionalCell);
									valid = false;
									break;
								}

								optionalTechnos.push_back(pCurTechno);
							}

							pCurObject = pCurObject->NextObject;
						}

						if (valid && pOptionalCell->IsClearToMove(pCheckedType->SpeedType, true, true, -1, pCheckedType->MovementZone, -1, false))
						{
							for (auto const& pOptionalTechno : optionalTechnos)
								reCheckedTechnos.push_back(pOptionalTechno);

							if (isInfantry) // Not need to remove it now
							{
								infantryCells.push_back(InfantryCountInCell{ pOptionalCell, 1 });
								++infantryCount.Y;
							}

							pDestinationCell = pOptionalCell;
							break;
						}
					}

					for (auto const& pDeleteCell : deleteCells) // Mark the invalid cells
					{
						checkedCells.push_back(pDeleteCell);
						optionalCells.erase(std::remove(optionalCells.begin(), optionalCells.end(), pDeleteCell), optionalCells.end());
					}
				}
			}
			while (false);

			// Step 4.2.2: Mark the cell and push back its surrounded cells, then prepare for the command.
			if (pDestinationCell)
			{
				if (std::find(checkedCells.begin(), checkedCells.end(), pDestinationCell) == checkedCells.end())
					checkedCells.push_back(pDestinationCell);

				if (std::find(optionalCells.begin(), optionalCells.end(), pDestinationCell) != optionalCells.end())
				{
					optionalCells.erase(std::remove(optionalCells.begin(), optionalCells.end(), pDestinationCell), optionalCells.end());
					CellStruct searchCell = pDestinationCell->MapCoords - CellStruct { 1, 1 };

					for (int i = 0; i < 4; ++i)
					{
						for (int j = 0; j < 2; ++j)
						{
							if (CellClass* const pSearchCell = MapClass::Instance->GetCellAt(searchCell))
							{
								if (std::find(checkedCells.begin(), checkedCells.end(), pSearchCell) == checkedCells.end()
									&& std::find(optionalCells.begin(), optionalCells.end(), pSearchCell) == optionalCells.end()
									&& !pSearchCell->GetBuilding()
									&& pSearchCell->IsClearToMove(SpeedType::Amphibious, true, true, -1, MovementZone::Amphibious, -1, false))
								{
									optionalCells.push_back(pSearchCell);
								}
							}

							if (i % 2)
								searchCell.Y += static_cast<short>((i / 2) ? -1 : 1);
							else
								searchCell.X += static_cast<short>((i / 2) ? -1 : 1);
						}
					}
				}

				const TechnoWithDestination thisOrder { pCheckedTechno, pDestinationCell };
				finalOrder.push_back(thisOrder);
			}
			else // Can not build
			{
				return true;
			}
		}

		checkedTechnos.clear();
	}
	while (reCheckedTechnos.size());

	// Step 5: Confirm command execution.
	for (auto const& pThisOrder : finalOrder)
	{
		TechnoClass* const pCheckedTechno = pThisOrder.techno;
		CellClass* const pDestinationCell = pThisOrder.destination;
		AbstractType const absType = pCheckedTechno->WhatAmI();

		pCheckedTechno->SetTarget(nullptr);
		pCheckedTechno->SetDestination(nullptr, false);
		pCheckedTechno->ForceMission(Mission::Guard);

		if (absType == AbstractType::Infantry)
		{
			InfantryClass* const pInfantry = static_cast<InfantryClass*>(pCheckedTechno);

			if (pInfantry->IsDeployed())
				pInfantry->PlayAnim(Sequence::Undeploy, true);

			pInfantry->SetDestination(pDestinationCell, false);
			pInfantry->ClickedMission(Mission::Move, nullptr, pDestinationCell, nullptr);
		}
		else if (absType == AbstractType::Unit)
		{
			UnitClass* const pUnit = static_cast<UnitClass*>(pCheckedTechno);

			if (pUnit->Deployed)
				pUnit->Undeploy();

			pUnit->SetDestination(pDestinationCell, false);
			pUnit->QueueMission(Mission::Move, false);
		}
	}

	return false;
}

void BuildingTypeExt::ExtData::Initialize()
{
}

// =============================
// load / save

void BuildingTypeExt::ExtData::LoadFromINIFile(CCINIClass* const pINI)
{
	auto pThis = this->OwnerObject();
	const char* pSection = pThis->ID;
	const char* pArtSection = pThis->ImageFile;
	auto pArtINI = &CCINIClass::INI_Art();

	if (!pINI->GetSection(pSection))
		return;

	INI_EX exINI(pINI);
	INI_EX exArtINI(pArtINI);

	this->PowersUp_Owner.Read(exINI, pSection, "PowersUp.Owner");
	this->PowersUp_Buildings.Read(exINI, pSection, "PowersUp.Buildings");
	this->PowerPlantEnhancer_Buildings.Read(exINI, pSection, "PowerPlantEnhancer.PowerPlants");
	this->PowerPlantEnhancer_Amount.Read(exINI, pSection, "PowerPlantEnhancer.Amount");
	this->PowerPlantEnhancer_Factor.Read(exINI, pSection, "PowerPlantEnhancer.Factor");
	this->Powered_KillSpawns.Read(exINI, pSection, "Powered.KillSpawns");

	if (pThis->PowersUpBuilding[0] == NULL && this->PowersUp_Buildings.size() > 0)
		strcpy_s(pThis->PowersUpBuilding, this->PowersUp_Buildings[0]->ID);

	this->AllowAirstrike.Read(exINI, pSection, "AllowAirstrike");
	this->CanC4_AllowZeroDamage.Read(exINI, pSection, "CanC4.AllowZeroDamage");

	this->InitialStrength_Cloning.Read(exINI, pSection, "InitialStrength.Cloning");

	this->Grinding_AllowAllies.Read(exINI, pSection, "Grinding.AllowAllies");
	this->Grinding_AllowOwner.Read(exINI, pSection, "Grinding.AllowOwner");
	this->Grinding_AllowTypes.Read(exINI, pSection, "Grinding.AllowTypes");
	this->Grinding_DisallowTypes.Read(exINI, pSection, "Grinding.DisallowTypes");
	this->Grinding_Sound.Read(exINI, pSection, "Grinding.Sound");
	this->Grinding_PlayDieSound.Read(exINI, pSection, "Grinding.PlayDieSound");
	this->Grinding_Weapon.Read<true>(exINI, pSection, "Grinding.Weapon");
	this->Grinding_Weapon_RequiredCredits.Read(exINI, pSection, "Grinding.Weapon.RequiredCredits");

	this->DisplayIncome.Read(exINI, pSection, "DisplayIncome");
	this->DisplayIncome_Houses.Read(exINI, pSection, "DisplayIncome.Houses");
	this->DisplayIncome_Offset.Read(exINI, pSection, "DisplayIncome.Offset");

	this->ConsideredVehicle.Read(exINI, pSection, "ConsideredVehicle");
	this->SellBuildupLength.Read(exINI, pSection, "SellBuildupLength");

	if (pThis->NumberOfDocks > 0)
	{
		this->AircraftDockingDirs.clear();
		this->AircraftDockingDirs.resize(pThis->NumberOfDocks);

		Nullable<DirType> nLandingDir;
		nLandingDir.Read(exINI, pSection, "AircraftDockingDir");

		if (nLandingDir.isset())
			this->AircraftDockingDirs[0] = nLandingDir.Get();

		for (int i = 0; i < pThis->NumberOfDocks; ++i)
		{
			char tempBuffer[32];
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "AircraftDockingDir%d", i);
			nLandingDir.Read(exINI, pSection, tempBuffer);

			if (nLandingDir.isset())
				this->AircraftDockingDirs[i] = nLandingDir.Get();
		}
	}

	// Ares tag
	this->SpyEffect_Custom.Read(exINI, pSection, "SpyEffect.Custom");
	if (SuperWeaponTypeClass::Array->Count > 0)
	{
		this->SuperWeapons.Read(exINI, pSection, "SuperWeapons");

		this->SpyEffect_VictimSuperWeapon.Read(exINI, pSection, "SpyEffect.VictimSuperWeapon");
		this->SpyEffect_InfiltratorSuperWeapon.Read(exINI, pSection, "SpyEffect.InfiltratorSuperWeapon");
	}

	if (pThis->MaxNumberOccupants > 10)
	{
		char tempBuffer[32];
		this->OccupierMuzzleFlashes.clear();
		this->OccupierMuzzleFlashes.resize(pThis->MaxNumberOccupants);

		for (int i = 0; i < pThis->MaxNumberOccupants; ++i)
		{
			Nullable<Point2D> nMuzzleLocation;
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "MuzzleFlash%d", i);
			nMuzzleLocation.Read(exArtINI, pArtSection, tempBuffer);
			this->OccupierMuzzleFlashes[i] = nMuzzleLocation.Get(Point2D::Empty);
		}
	}

	this->Refinery_UseStorage.Read(exINI, pSection, "Refinery.UseStorage");

	// PlacementPreview
	{
		this->PlacementPreview.Read(exINI, pSection, "PlacementPreview");
		this->PlacementPreview_Shape.Read(exINI, pSection, "PlacementPreview.Shape");
		this->PlacementPreview_ShapeFrame.Read(exINI, pSection, "PlacementPreview.ShapeFrame");
		this->PlacementPreview_Offset.Read(exINI, pSection, "PlacementPreview.Offset");
		this->PlacementPreview_Remap.Read(exINI, pSection, "PlacementPreview.Remap");
		this->PlacementPreview_Palette.LoadFromINI(pINI, pSection, "PlacementPreview.Palette");
		this->PlacementPreview_Translucency.Read(exINI, pSection, "PlacementPreview.Translucency");
	}

	// Art
	this->ZShapePointMove_OnBuildup.Read(exArtINI, pSection, "ZShapePointMove.OnBuildup");
}

void BuildingTypeExt::ExtData::CompleteInitialization()
{
	auto const pThis = this->OwnerObject();
	UNREFERENCED_PARAMETER(pThis);
}

template <typename T>
void BuildingTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->PowersUp_Owner)
		.Process(this->PowersUp_Buildings)
		.Process(this->PowerPlantEnhancer_Buildings)
		.Process(this->PowerPlantEnhancer_Amount)
		.Process(this->PowerPlantEnhancer_Factor)
		.Process(this->SuperWeapons)
		.Process(this->OccupierMuzzleFlashes)
		.Process(this->Powered_KillSpawns)
		.Process(this->AllowAirstrike)
		.Process(this->CanC4_AllowZeroDamage)
		.Process(this->InitialStrength_Cloning)
		.Process(this->Refinery_UseStorage)
		.Process(this->Grinding_AllowAllies)
		.Process(this->Grinding_AllowOwner)
		.Process(this->Grinding_AllowTypes)
		.Process(this->Grinding_DisallowTypes)
		.Process(this->Grinding_Sound)
		.Process(this->Grinding_PlayDieSound)
		.Process(this->Grinding_Weapon)
		.Process(this->Grinding_Weapon_RequiredCredits)
		.Process(this->DisplayIncome)
		.Process(this->DisplayIncome_Houses)
		.Process(this->DisplayIncome_Offset)
		.Process(this->PlacementPreview)
		.Process(this->PlacementPreview_Shape)
		.Process(this->PlacementPreview_ShapeFrame)
		.Process(this->PlacementPreview_Offset)
		.Process(this->PlacementPreview_Remap)
		.Process(this->PlacementPreview_Palette)
		.Process(this->PlacementPreview_Translucency)
		.Process(this->SpyEffect_Custom)
		.Process(this->SpyEffect_VictimSuperWeapon)
		.Process(this->SpyEffect_InfiltratorSuperWeapon)
		.Process(this->ConsideredVehicle)
		.Process(this->ZShapePointMove_OnBuildup)
		.Process(this->SellBuildupLength)
		.Process(this->AircraftDockingDirs)
		;
}

void BuildingTypeExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<BuildingTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void BuildingTypeExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<BuildingTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool BuildingTypeExt::ExtContainer::Load(BuildingTypeClass* pThis, IStream* pStm)
{
	BuildingTypeExt::ExtData* pData = this->LoadKey(pThis, pStm);

	return pData != nullptr;
};

bool BuildingTypeExt::LoadGlobals(PhobosStreamReader& Stm)
{

	return Stm.Success();
}

bool BuildingTypeExt::SaveGlobals(PhobosStreamWriter& Stm)
{


	return Stm.Success();
}
// =============================
// container

BuildingTypeExt::ExtContainer::ExtContainer() : Container("BuildingTypeClass") { }

BuildingTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x45E50C, BuildingTypeClass_CTOR, 0x6)
{
	GET(BuildingTypeClass*, pItem, EAX);

	BuildingTypeExt::ExtMap.TryAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x45E707, BuildingTypeClass_DTOR, 0x6)
{
	GET(BuildingTypeClass*, pItem, ESI);

	BuildingTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x465300, BuildingTypeClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x465010, BuildingTypeClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(BuildingTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	BuildingTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x4652ED, BuildingTypeClass_Load_Suffix, 0x7)
{
	BuildingTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x46536A, BuildingTypeClass_Save_Suffix, 0x7)
{
	BuildingTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x464A56, BuildingTypeClass_LoadFromINI, 0xA)
DEFINE_HOOK(0x464A49, BuildingTypeClass_LoadFromINI, 0xA)
{
	GET(BuildingTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x364);

	BuildingTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}
