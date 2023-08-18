#pragma once

#include "main.h"

class CUserLoadout
{
public:
	CUserLoadout()
	{
	}

	std::vector<std::vector<int>> m_Loadouts;
};

class CUserBuyMenu
{
public:
	CUserBuyMenu(std::vector<int>& slots)
	{
		items = slots;
	}

	std::vector<int> items;
};

class CUserCostumeLoadout
{
public:
	CUserCostumeLoadout()
	{
		m_nHeadCostumeID = 0;
		m_nBackCostumeID = 0;
		m_nArmCostumeID = 0;
		m_nPelvisCostumeID = 0;
		m_nFaceCostumeID = 0;
		m_nTattooID = 0;
		m_nPetCostumeID = 0;
	}

	CUserCostumeLoadout(int head, int back, int arm, int pelvis, int face, int tattoo, int pet)
	{
		m_nHeadCostumeID = head;
		m_nBackCostumeID = back;
		m_nArmCostumeID = arm;
		m_nPelvisCostumeID = pelvis;
		m_nFaceCostumeID = face;
		m_nTattooID = tattoo;
		m_nPetCostumeID = pet;
	}

	int m_nHeadCostumeID;
	int m_nBackCostumeID;
	int m_nArmCostumeID;
	int m_nPelvisCostumeID;
	int m_nFaceCostumeID;
	std::map<int, int> m_ZombieSkinCostumeID;
	int m_nTattooID;
	int m_nPetCostumeID;
};
