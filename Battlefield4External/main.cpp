#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>

#include "offsets.h"
#include "structs.h"
#include "espWindow.h"

const float pi = 3.14159;
const float rToD = (180 / pi); // radians to degrees
const float dToR = (pi / 180); // degrees to radians

DWORD GetProcessIdByName(const wchar_t* procName)
{
	HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (processSnapshot != INVALID_HANDLE_VALUE) 
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(PROCESSENTRY32);

		Process32First(processSnapshot, &procEntry);

		do 
		{
			if (wcscmp(procName, procEntry.szExeFile) == 0) 
			{
				CloseHandle(processSnapshot);
				return procEntry.th32ProcessID;
			}
		} while (Process32Next(processSnapshot, &procEntry));

		CloseHandle(processSnapshot);
	}

	return 0;
}


uintptr_t GetModuleAddressByName(const wchar_t* moduleName, DWORD procId)
{
	HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procId);
	if (moduleSnapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(MODULEENTRY32);

		Module32First(moduleSnapshot, &modEntry);

		do
		{
			if (wcscmp(moduleName, modEntry.szModule) == 0)
			{
				CloseHandle(moduleSnapshot);
				return (uintptr_t)modEntry.modBaseAddr;
			}
		} while (Module32Next(moduleSnapshot, &modEntry));

		CloseHandle(moduleSnapshot);
	}

	return 0;
}

void MoveMouse(float deltaX, float deltaY)
{
	INPUT input;
	ZeroMemory(&input, sizeof(input));

	input.type = INPUT_MOUSE;

	MOUSEINPUT mouseInput;
	ZeroMemory(&mouseInput, sizeof(mouseInput));

	mouseInput.dwFlags = MOUSEEVENTF_MOVE;

	if (deltaX > 0 && deltaX < 1) { deltaX = 1; }
	if (deltaX < 0 && deltaX > -1) { deltaX = -1; }

	if (deltaY > 0 && deltaY < 1) { deltaY = 1; }
	if (deltaY < 0 && deltaY > -1) { deltaY = -1; }

	mouseInput.dx = deltaX;
	mouseInput.dy = deltaY;

	input.mi = mouseInput;
	SendInput(1, &input, sizeof(INPUT));
}

uintptr_t GetPlayerListContainer(HANDLE bf4Handle, uintptr_t bf4BaseAddress)
{
	uintptr_t playerListContainerPtr = bf4BaseAddress + playerListContainerOffset;
	uintptr_t playerListContainer = 0;
	ReadProcessMemory(bf4Handle, (void*)playerListContainerPtr, &playerListContainer, sizeof(playerListContainer), nullptr);

	return playerListContainer;
}

uintptr_t GetPlayerList(HANDLE bf4Handle, uintptr_t playerListContainer)
{
	uintptr_t playerListPtr = playerListContainer + playerListOffset;
	uintptr_t playerList = 0;
	ReadProcessMemory(bf4Handle, (void*)playerListPtr, &playerList, sizeof(playerList), nullptr);

	return playerList;
}

uintptr_t GetPlayer(HANDLE bf4Handle, uintptr_t playerList, int index)
{
	uintptr_t playerPtr = playerList + (index * sizeof(uintptr_t));
	uintptr_t player = 0;
	ReadProcessMemory(bf4Handle, (void*)playerPtr, &player, sizeof(player), nullptr);

	return player;
}

uintptr_t GetLocalPlayer(HANDLE bf4Handle, uintptr_t bf4BaseAddress)
{
	uintptr_t playerListContainer = GetPlayerListContainer(bf4Handle, bf4BaseAddress);
	if (playerListContainer == 0) { return 0; }

	uintptr_t localPlayerPtr = playerListContainer + localPlayerOffset;
	uintptr_t localPlayer = 0;
	ReadProcessMemory(bf4Handle, (void*)localPlayerPtr, &localPlayer, sizeof(localPlayer), nullptr);

	return localPlayer;
}

Vector3 GetBonePosition(HANDLE bf4Handle, uintptr_t player, int boneIndex)
{
	Vector3 bonePos = {};
	
	uintptr_t infoContainerContainerPtr = player + infoContainerContainerOffset;
	uintptr_t infoContainerContainer = 0;
	if (!ReadProcessMemory(bf4Handle, (void*)infoContainerContainerPtr, &infoContainerContainer, sizeof(infoContainerContainer), nullptr)) { return bonePos; }

	uintptr_t infoContainerPtr = infoContainerContainer;
	uintptr_t infoContainer = 0;
	if (!ReadProcessMemory(bf4Handle, (void*)infoContainerPtr, &infoContainer, sizeof(infoContainer), nullptr)) { return bonePos; }

	uintptr_t bonePosAddress = infoContainer + boneListOffset + (boneIndex * 0x40);
	ReadProcessMemory(bf4Handle, (void*)bonePosAddress, &bonePos, sizeof(bonePos), nullptr);

	return bonePos;
}

ViewAngles GetLocalPlayerViewAngles(HANDLE bf4Handle, uintptr_t bf4BaseAddress)
{
	uintptr_t localPlayerViewAnglesAddress = bf4BaseAddress + localPlayerViewAnglesOffset;
	ViewAngles localPlayerViewAngles = {};
	ReadProcessMemory(bf4Handle, (void*)localPlayerViewAnglesAddress, &localPlayerViewAngles, sizeof(localPlayerViewAngles), nullptr);

	return localPlayerViewAngles;
}

ViewAngles GetLocalPlayerFov(HANDLE bf4Handle, uintptr_t bf4BaseAddress)
{
	uintptr_t fovContainerPtr = bf4BaseAddress + fovContainerOffset;
	uintptr_t fovContainer = 0;
	ReadProcessMemory(bf4Handle, (void*)fovContainerPtr, &fovContainer, sizeof(fovContainer), nullptr);

	uintptr_t vertFovAddress = fovContainer + fovOffset;
	float vertFov = 0;
	ReadProcessMemory(bf4Handle, (void*)vertFovAddress, &vertFov, sizeof(vertFov), nullptr);

	float horizFov = (-0.002 * vertFov * vertFov) + (1.32 * vertFov) + 2.8;

	ViewAngles result;
	result.pitch = vertFov;
	result.yaw = horizFov;

	return result;
}

bool IsPlayerValid(HANDLE bf4Handle, uintptr_t player)
{
	uintptr_t infoContainerContainerPtr = player + infoContainerContainerOffset;
	uintptr_t infoContainerContainer = 0;
	if (!ReadProcessMemory(bf4Handle, (void*)infoContainerContainerPtr, &infoContainerContainer, sizeof(infoContainerContainer), nullptr)) { return false; }

	return infoContainerContainer != 0;
}

char GetPlayerTeam(HANDLE bf4Handle, uintptr_t player)
{
	uintptr_t teamAddress = player + teamOffset;
	char team = 0;
	ReadProcessMemory(bf4Handle, (void*)teamAddress, &team, sizeof(team), nullptr);

	return team;
}

ViewAngles CalculateDeltaViewAngles(Vector3 originPos, Vector3 targetPos, ViewAngles currentViewAngles)
{
	ViewAngles result = {};
	
	Vector3 diff = originPos - targetPos;
	float distance = sqrt((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));
	if (distance == 0) { return result; }

	float targetPitch = asin(diff.y / distance) * rToD;
	float targetYaw = 270 - (atan2(diff.z, diff.x) * rToD);

	float pitchDiff = currentViewAngles.pitch - targetPitch;

	float yawDiff = currentViewAngles.yaw - targetYaw;
	if (yawDiff > 180) { yawDiff = -(360 - yawDiff); }
	if (yawDiff < -180) { yawDiff = (360 + yawDiff); }

	result.pitch = pitchDiff;
	result.yaw = yawDiff;

	return result;
}

uintptr_t GetClosestPlayerToCrosshair(HANDLE bf4Handle, uintptr_t bf4BaseAddress, bool targetSameTeam)
{
	uintptr_t playerListContainer = GetPlayerListContainer(bf4Handle, bf4BaseAddress);
	if (playerListContainer == 0) { return 0; }

	uintptr_t playerList = GetPlayerList(bf4Handle, playerListContainer);
	if (playerList == 0) { return 0; }

	uintptr_t localPlayer = GetLocalPlayer(bf4Handle, bf4BaseAddress);
	if (localPlayer == 0) { return 0; }

	Vector3 localPlayerPos = GetBonePosition(bf4Handle, localPlayer, headBoneIndex);
	ViewAngles localPlayerViewAngles = GetLocalPlayerViewAngles(bf4Handle, bf4BaseAddress);

	uintptr_t result = 0;
	float minScreenDistance = 99999;

	for (int i = 0; i < 64; i++)
	{
		uintptr_t currentPlayer = GetPlayer(bf4Handle, playerList, i);

		if (currentPlayer == localPlayer || !IsPlayerValid(bf4Handle, currentPlayer)) { continue; }
		if (!targetSameTeam && GetPlayerTeam(bf4Handle, localPlayer) == GetPlayerTeam(bf4Handle, currentPlayer)) { continue; }

		Vector3 currentPlayerPos = GetBonePosition(bf4Handle, currentPlayer, headBoneIndex);

		ViewAngles viewAnglesDiff = CalculateDeltaViewAngles(localPlayerPos, currentPlayerPos, localPlayerViewAngles);

		float screenDistance = sqrt((viewAnglesDiff.yaw * viewAnglesDiff.yaw) + (viewAnglesDiff.pitch * viewAnglesDiff.pitch));

		if (screenDistance < minScreenDistance)
		{
			minScreenDistance = screenDistance;
			result = currentPlayer;
		}
	}

	return result;
}

bool Aimbot(HANDLE bf4Handle, uintptr_t bf4BaseAddress, uintptr_t targetPlayer, Vector3 lastPos, float aimbotStrength, bool aimForHead)
{
	uintptr_t playerListContainer = GetPlayerListContainer(bf4Handle, bf4BaseAddress);
	if (playerListContainer == 0) { return false; }

	uintptr_t localPlayer = GetLocalPlayer(bf4Handle, bf4BaseAddress);
	if (localPlayer == 0) { return false; }

	ViewAngles localPlayerViewAngles = GetLocalPlayerViewAngles(bf4Handle, bf4BaseAddress);

	Vector3 localPlayerPos = GetBonePosition(bf4Handle, localPlayer, headBoneIndex);
	Vector3 targetPlayerPos = GetBonePosition(bf4Handle, targetPlayer, aimForHead ? headBoneIndex : upperBodyBoneIndex);

	Vector3 targetPlayerVelocity = targetPlayerPos - lastPos;

	targetPlayerPos = targetPlayerPos + (targetPlayerVelocity * 0.04);

	ViewAngles viewAnglesDiff = CalculateDeltaViewAngles(localPlayerPos, targetPlayerPos, localPlayerViewAngles);

	MoveMouse(viewAnglesDiff.yaw * aimbotStrength, -viewAnglesDiff.pitch * aimbotStrength);

	return true;
}

bool Esp(HANDLE bf4Handle, uintptr_t bf4BaseAddress, HWND bf4Window, HWND espWindow, bool targetSameTeam)
{
	RECT bf4WindowDimensions = {};
	if (!GetWindowRect(bf4Window, &bf4WindowDimensions)) { return false; }

	int windowWidth = bf4WindowDimensions.right - bf4WindowDimensions.left;
	int windowHeight = bf4WindowDimensions.bottom - bf4WindowDimensions.top;
	
	SetWindowPos(espWindow, HWND_TOP, bf4WindowDimensions.left, bf4WindowDimensions.top, windowWidth, windowHeight, SWP_SHOWWINDOW);
	
	uintptr_t playerListContainer = GetPlayerListContainer(bf4Handle, bf4BaseAddress);
	if (playerListContainer == 0) { return false; }

	uintptr_t playerList = GetPlayerList(bf4Handle, playerListContainer);
	if (playerList == 0) { return false; }

	uintptr_t localPlayer = GetLocalPlayer(bf4Handle, bf4BaseAddress);
	if (localPlayer == 0) { return false; }

	Vector3 localPlayerPos = GetBonePosition(bf4Handle, localPlayer, headBoneIndex);
	ViewAngles localPlayerViewAngles = GetLocalPlayerViewAngles(bf4Handle, bf4BaseAddress);
	ViewAngles localPlayerFov = GetLocalPlayerFov(bf4Handle, bf4BaseAddress);

	for (int i = 0; i < 64; i++)
	{
		RECT rect = {};
		
		uintptr_t currentPlayer = GetPlayer(bf4Handle, playerList, i);

		if (currentPlayer == localPlayer || !IsPlayerValid(bf4Handle, currentPlayer)) { SetRect(i, rect); continue; }
		if (!targetSameTeam && GetPlayerTeam(bf4Handle, localPlayer) == GetPlayerTeam(bf4Handle, currentPlayer)) { SetRect(i, rect); continue; }

		Vector3 currentPlayerPos = GetBonePosition(bf4Handle, currentPlayer, headBoneIndex);

		ViewAngles viewAnglesDiff = CalculateDeltaViewAngles(localPlayerPos, currentPlayerPos, localPlayerViewAngles);

		float screenY = -viewAnglesDiff.pitch / localPlayerFov.pitch;
		screenY = (screenY + 1) / 2;
		screenY *= windowHeight;

		float screenX = viewAnglesDiff.yaw / localPlayerFov.yaw;
		screenX = (screenX + 1) / 2;
		screenX *= windowWidth;

		Vector3 diff = localPlayerPos - currentPlayerPos;
		float distance = sqrt((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));
		int boxSize = 250 / distance;
		if (boxSize < 4) { boxSize = 4; }

		rect.left = screenX - (boxSize / 2);
		rect.right = rect.left + boxSize;
		rect.top = screenY - (boxSize / 2);
		rect.bottom = rect.top + boxSize;

		SetRect(i, rect);
	}

	UpdateWindow(espWindow);
	InvalidateRect(espWindow, NULL, TRUE);
	
	return true;
}

int main()
{
	DWORD bf4ProcId = GetProcessIdByName(L"bf4.exe");
	if (bf4ProcId == 0) 
	{ 
		std::cout << "Failed to get process id of bf4.exe.\n";
		std::cin.get();
		return 0; 
	}

	HANDLE bf4Handle = OpenProcess(PROCESS_VM_READ, 0, bf4ProcId);
	if (bf4Handle == INVALID_HANDLE_VALUE) 
	{ 
		std::cout << "Failed to open the bf4.exe process.\n";
		std::cin.get();
		return 0; 
	}

	uintptr_t bf4BaseAddress = GetModuleAddressByName(L"bf4.exe", bf4ProcId);
	if (bf4BaseAddress == 0) 
	{ 
		std::cout << "Failed to get address of the bf4.exe module.\n";
		std::cin.get();
		return 0; 
	}
	
	std::cout << "Ins - exit.\n";
	std::cout << "F1 - adjust aimbot strength.\n";
	std::cout << "F2 - toggle esp.\n";
	std::cout << "F3 - toggle target players on same team.\n";
	std::cout << "F4 - toggle aim for head/upper body.\n";
	std::cout << "Middle mouse button - aimbot player closest to crosshair.\n\n";

	bool targetSameTeam = false;

	bool enableEsp = false;
	HWND bf4Window = 0;
	HWND espWindow = 0;

	uintptr_t targetPlayer = 0;
	Vector3 lastTargetPlayerPos = {};
	float aimbotStrength = 1000;
	bool aimForHead = true;
	bool enableAimbot = false;

	while (!GetAsyncKeyState(VK_INSERT))
	{
		if (GetAsyncKeyState(VK_F1) & 1)
		{
			std::cout << "Current aimbot strength is " << aimbotStrength << '\n';
			std::cout << "Enter new aimbot strength value: ";
			std::cin >> aimbotStrength;
			std::cout << "Aimbot strength has been set to " << aimbotStrength << '\n';
		}

		if (IsPlayerValid(bf4Handle, GetLocalPlayer(bf4Handle, bf4BaseAddress)) && (GetAsyncKeyState(VK_F2) & 1))
		{
			enableEsp = !enableEsp;

			if (enableEsp)
			{
				bf4Window = GetBF4Window(bf4ProcId);
				espWindow = CreateEspWindow();

				if (bf4Window == 0 || espWindow == 0) 
				{ 
					enableEsp = false;
				}
				else 
				{
					std::cout << "ESP enabled.\n";
				}
			}
			else 
			{
				ClearRects();
				UpdateWindow(espWindow);
				InvalidateRect(espWindow, NULL, TRUE);
				std::cout << "ESP disabled.\n";
			}
		}

		if (GetAsyncKeyState(VK_F3) & 1)
		{
			targetSameTeam = !targetSameTeam;

			if (targetSameTeam) { std::cout << "Targeting players on same team.\n"; }
			else { std::cout << "Not targeting players on same team.\n"; }
		}

		if (GetAsyncKeyState(VK_F4) & 1)
		{
			aimForHead = !aimForHead;

			if (aimForHead) { std::cout << "Aiming for head.\n"; }
			else { std::cout << "Aiming for upper body.\n"; }
		}

		if (enableEsp) 
		{
			enableEsp = IsPlayerValid(bf4Handle, GetLocalPlayer(bf4Handle, bf4BaseAddress)) && Esp(bf4Handle, bf4BaseAddress, bf4Window, espWindow, targetSameTeam);
			if (!enableEsp) 
			{ 
				ClearRects();
				UpdateWindow(espWindow);
				InvalidateRect(espWindow, NULL, TRUE);
			}
		}
		
		if (IsPlayerValid(bf4Handle, GetLocalPlayer(bf4Handle, bf4BaseAddress)) && (GetAsyncKeyState(VK_MBUTTON) & 1))
		{
			targetPlayer = GetClosestPlayerToCrosshair(bf4Handle, bf4BaseAddress, targetSameTeam);

			if (targetPlayer != 0) 
			{ 
				enableAimbot = !enableAimbot;

				if (enableAimbot) 
				{
					uintptr_t nameAddress = targetPlayer + playerNameOffset;
					char playerName[20];
					if (!ReadProcessMemory(bf4Handle, (void*)nameAddress, playerName, 20, nullptr)) { continue; }

					lastTargetPlayerPos = GetBonePosition(bf4Handle, targetPlayer, aimForHead ? headBoneIndex : upperBodyBoneIndex);

					std::cout << "Targeting " << playerName << '\n';
				}
			}
			else 
			{ 
				enableAimbot = false;
				std::cout << "Failed to find a player to aimbot.\n";
			}
		}

		if (enableAimbot)
		{
			float localPlayerVertFov = GetLocalPlayerFov(bf4Handle, bf4BaseAddress).pitch;
			enableAimbot = IsPlayerValid(bf4Handle, GetLocalPlayer(bf4Handle, bf4BaseAddress)) && Aimbot(bf4Handle, bf4BaseAddress, targetPlayer, lastTargetPlayerPos, aimbotStrength / localPlayerVertFov, aimForHead);
		}

		Sleep(5);
	}

	return 0;
}