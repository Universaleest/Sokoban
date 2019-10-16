#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#include <direct.h>
#include "resource.h"
using namespace std;

#pragma comment(lib,"winmm.lib")

#define MAXSTAGE 20 // 최대 스테이지 개수
#define BW 32 // 출력에 사용될 비트맵의 너비
#define BH 32 // 출력에 사용될 비트맵의 높이
#define MAXUNDO 1000 // 이동취소 정보를 저장할 수 있는 최대 개수
#define LOAD_LEN 21
#define MAXSTRING 512 // 문자배열에 저장할 수 있는 최대 길이
#define NOTICE TEXT("파일명(영문으로만 작성).txt을 입력하세요.")

void DrawScreen(HDC hdc); 
BOOL TestEnd();
void Move(int dir);
void InitStage();
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit);
void ErasePack(int x, int y);
void Undo();
void Redo();
void Save();
void Load();
void MakeMapList(TCHAR* MapName);
BOOL IsDuplicated(TCHAR* FileName);
BOOL MapLoader(char* FileName);
void SortRecords(int record[]);
void WriteRecords();
void ReadRecords();
void AlreadyDoneCheck();
BOOL AddStage(char* FileName);
void LoadAddStage();
void Remove(char* Name, char* rm);
char ns[18][21];
int nStage;
int nMaxStage = 3;
int nx, ny;
int nMove;
HBITMAP hBit[9];
int ManBit;
char arStage[MAXSTAGE][18][21] = {
	{
		"####################",
		"####################",
		"####################",
		"#####   ############",
		"#####O  ############",
		"#####  O############",
		"###  O O ###########",
		"### # ## ###########",
		"#   # ## #####  ..##",
		"# O  O   @      ..##",
		"##### ### # ##  ..##",
		"#####      #########",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################" 
	},
	{
		"####################",
		"####################",
		"####################",
		"####################",
		"####..  #     ######",
		"####..  # O  O  ####",
		"####..  #O####  ####",
		"####..    @ ##  ####",
		"####..  # #  O #####",
		"######### ##O O ####",
		"###### O  O O O ####",
		"######    #     ####",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################"
	},
	{
		"####################",
		"####################",
		"####################",
		"####################",
		"##########     @####",
		"########## O#O #####",
		"########## O  O#####",
		"###########O O #####",
		"########## O # #####",
		"##....  ## O  O  ###",
		"###...    O  O   ###",
		"##....  ############",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################"
	}
};
char nCustomStage[18][21];
char TempStage[18][21];
BOOL bSound = TRUE;
TCHAR CMapName[64];
char NameToChar[64];
BOOL CustomFlag = FALSE;
int Records[MAXSTAGE][5];
int RecordsSize[MAXSTAGE];
BOOL errflag = FALSE;

typedef struct tag_MoveInfo
{
	char dx : 3;
	char dy : 3;
	char bWithPack : 2;
} MoveInfo;

MoveInfo MOVEINFO[MAXUNDO];
int UndoIdx;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK MapDlgProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
HWND hWndMain, hMDlg, hMapDlg;
LPCTSTR lpszClass = TEXT("Sokoban");

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);
	hWnd = CreateWindow(lpszClass, lpszClass, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	while (GetMessage(&Message, NULL, 0, 0))
	{
		if ((!IsWindow(hMDlg) || !IsDialogMessage(hMDlg, &Message)) || (!IsDialogMessage(hMapDlg, &Message)))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}
	Save();
	WriteRecords();
	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT crt;
	int i;
	TCHAR Message[256];
	switch (iMessage)
	{
	case WM_CREATE:
		hWndMain = hWnd;
		SetRect(&crt, 0, 0, 900, BH * 18.6);
		AdjustWindowRect(&crt, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
		SetWindowPos(hWnd, NULL, 0, 0, crt.right - crt.left, crt.bottom - crt.top, SWP_NOMOVE | SWP_NOZORDER);
		for (i = 0; i < 9; i++)
		{
			hBit[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_WALL + i));
		}
		//nStage = 0;
		Load();
		ReadRecords();
		if (errflag)
		{
			for (int i = 3; i < nMaxStage; i++)
			{
				memset(Records[i], 0, sizeof(int) * 5);
				RecordsSize[i] = 0;
			}
			WriteRecords();
		}
		InitStage();
		/*if (CustomFlag)
		{
			if (!MapLoader(NameToChar))
			{
				MessageBox(HWND_DESKTOP, TEXT("저장된 커스텀 맵이 없습니다. 스테이지 1로 초기화 합니다."), TEXT("알림"), MB_OK);
				nStage = 0;
				CustomFlag = FALSE;
				InitStage();
			}
		}*/
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawScreen(hdc);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
			Move(wParam);
			if (TestEnd())
			{
				if (bSound)
				{
					PlaySound(MAKEINTRESOURCE(IDR_CLEAR), g_hInst, SND_RESOURCE | SND_ASYNC);
				}
				if (nStage < nMaxStage)
				{
					wsprintf(Message, TEXT("%d 스테이지를 풀었습니다. 다음 스테이지로 이동합니다."), nStage + 1);
				}
				MessageBox(hWnd, Message, TEXT("알림"), MB_OK);
				if (nStage < nMaxStage - 1)
				{
					nStage++;
				}
				InitStage();
			}
			break;
		case 'Q':
			DestroyWindow(hWnd);
			break;
		case 'R':
			InitStage();
			break;
		case 'N':
			if (nStage < nMaxStage - 1)
			{
				nStage++;
				InitStage();
			}
			break;
		case 'P':
			CustomFlag = FALSE;
			if (nStage > 0)
			{
				nStage--;
				InitStage();
			}
			break;
		case 'Z':
			Undo();
			break;
		case 'Y':
			Redo();
			break;
		case 'S':
			bSound = !bSound;
			break;
		}
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_LOADCUSTOM: // 메뉴에서 사용자 정의 맵 불러오기의 아이디 
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_MAKEMAP), hWnd, MapDlgProc) == IDOK)
			{ // IDD_MAKEMAP은 사용자 정의 맵 불러오기 대화상자의 아이디
				memset(NameToChar, 0, sizeof(char) * 64);
				WideCharToMultiByte(CP_ACP, 0, CMapName, lstrlen(CMapName), NameToChar, 64, 0, 0); // TCHAR* 자료형을 char* 자료형으로 변환
				MapLoader(NameToChar); // 맵 이름에 해당하는 맵 파일을 불러옴.
				CustomFlag = TRUE; // 사용자 정의 맵 불러오기를 사용했는지 유무를 알리는 플래그
				if (AddStage(NameToChar)) // 불러온 맵을 현재 게임의 마지막 스테이지의 다음 스테이지로 추가
				{
					InitStage(); // 불러온 맵으로 스테이지 초기화
				}
			}
			break;
		case ID_HELP:
			if (!IsWindow(hMDlg))
			{
				hMDlg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG), hWnd, DlgProc);
				ShowWindow(hMDlg, SW_SHOW);
			}
			break;
		case ID_INFO:
			MessageBox(hWnd, TEXT("제작자 : 이제운\nE-mail : bachagalee@gmail.com"), TEXT("정보"), MB_OK);
			break;
		case ID_CLOSE:
			DestroyWindow(hWnd);
			break;
		}
		return 0;
	case WM_DESTROY:
		for (i = 0; i < 9; i++)
		{
			DeleteObject(hBit[i]);
		}
		PostQuitMessage(0);
		return 0;
	}
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{ // 도움말 대화상자의 메세지를 처리하는 프로시저. 모달리스형
	TCHAR str[MAXSTRING];
	switch (iMessage)
	{
	case WM_INITDIALOG:
		LoadString(g_hInst, IDS_SAMPLE, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_SAMPLE, str);
		LoadString(g_hInst, IDS_HOWTO, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_HOWTO, str);
		LoadString(g_hInst, IDS_EMPTY, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_EMPTY, str);
		LoadString(g_hInst, IDS_STUFF, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_STUFF, str);
		LoadString(g_hInst, IDS_GOAL, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_GOAL, str);
		LoadString(g_hInst, IDS_CHARACTER, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_CHARACTER, str);
		LoadString(g_hInst, IDS_WALL, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_WALL, str);
		return TRUE; // 스트링 테이블을 만들어 각 아이디에 있는 문자열을 불러옴
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		case IDOK:
			DestroyWindow(hDlg);
			EndDialog(hDlg, IDOK);
			hMDlg = NULL;
			return TRUE; 
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK MapDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{ // 사용자 정의 맵 불러오기 대화상자의 메세지를 처리하는 프로시저. 모달형
	FILE* f;
	FILE* IsF;
	TCHAR FileName[64];
	char ExistFile[64];
	int i = 0;
	switch (iMessage)
	{
	case WM_INITDIALOG:
		hMapDlg = hDlg;
		fopen_s(&f, "MapList.txt", "r"); // MapList.txt는 추가한 맵의 이름 목록이 쓰여있는 파일
		if (f != NULL)                   // MapList.txt에 맵의 이름이 한 줄씩 쓰여있음
		{
			while (TRUE)
			{
				memset(ExistFile, 0, sizeof(char) * 64);
				fscanf_s(f, "%ls", FileName, 64);
				if (feof(f)) break;
				WideCharToMultiByte(CP_ACP, 0, FileName, lstrlen(FileName), ExistFile, 64, 0, 0); // TCHAR* 자료형을 char* 자료형으로 변환
				fopen_s(&IsF, ExistFile, "r"); // 맵의 이름 하나를 읽어와 맵파일이 있는지 확인하기 위해 읽기 전용으로 파일을 연다 
				if (IsF != NULL)
				{
					SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_ADDSTRING, i, (LPARAM)FileName); // 맵파일이 존재하면 리스트박스에 추가
					i++;
					fclose(IsF);
				}
			}
			fclose(f);
		}
		SetDlgItemText(hDlg, IDC_LOADMAP, NOTICE);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CMAPLIST: // 리스트 박스의 아이디
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				i = SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_GETCURSEL, 0, 0); // 선택한 목록이 리스트 박스의 몇번째 목록인지를 i에 저장
				SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_GETTEXT, i, (LPARAM)CMapName); // i번째 목록에 있는 문자열을 CMapName에 저장
				SetDlgItemText(hDlg, IDC_LOADMAP, CMapName); // IDC_LOADMAP은 edit의 아이디. 선택한 목록의 문자열을 edit에 출력
				return TRUE;
			}
			return FALSE;
		case IDOK:
			GetDlgItemText(hDlg, IDC_LOADMAP, CMapName, 64); // edit에 있는 문자열을 읽어옴
			if (lstrcmp(CMapName, NOTICE) == 0)
			{ // 읽어온 문자열이 디폴트로 설정된 문자열과 동일하면 메세지 박스를 띄워 알림
				MessageBox(hMapDlg, TEXT("맵 이름을 제대로 입력하세요."), TEXT("알림"), MB_OK);
				return TRUE;
			}
			memset(ExistFile, 0, sizeof(char) * 64);
			WideCharToMultiByte(CP_ACP, 0, CMapName, lstrlen(CMapName), ExistFile, 64, 0, 0);
			fopen_s(&f, ExistFile, "r");
			if (f == NULL)
			{ // 읽어온 맵의 이름을 읽기 전용으로 열어 존재 유무를 확인. 없으면 메세지 박스를 띄워 알림
				MessageBox(hMapDlg, TEXT("맵이 없습니다."), TEXT("알림"), MB_OK);
				return TRUE;
			}
			if (!IsDuplicated(CMapName)) // 동일한 맵 이름이 중복되는지 검사
			{
				MakeMapList(CMapName); // 중복되는 이름이 없으면 MapList.txt에 쓴다 
			}
			fclose(f);
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

void DrawScreen(HDC hdc)
{
	int x, y;
	int iBit;
	TCHAR Message[256];
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			switch (ns[y][x])
			{
			case '#':
				iBit = 0;
				break;
			case 'O':
				iBit = 1;
				break;
			case '.':
				iBit = 2;
				break;
			case ' ':
				iBit = 3;
				break;
			case '!':
				iBit = 8;
				break;
			}
			DrawBitmap(hdc, x*BW, y*BH, hBit[iBit]);
		}
	}
	DrawBitmap(hdc, nx*BW, ny*BH, hBit[ManBit]);
	wsprintf(Message, TEXT("Sokoban"));
	TextOut(hdc, 700, 10, Message, lstrlen(Message));
	wsprintf(Message, TEXT("Q:종료, R:다시 시작"));
	TextOut(hdc, 700, 30, Message, lstrlen(Message));
	wsprintf(Message, TEXT("N:다음, P:이전"));
	TextOut(hdc, 700, 50, Message, lstrlen(Message));
	wsprintf(Message, TEXT("Z:취소, Y:재실행"));
	TextOut(hdc, 700, 70, Message, lstrlen(Message));
	wsprintf(Message, TEXT("S:소리 켜기/끄기"));
	TextOut(hdc, 700, 90, Message, lstrlen(Message));
	if (nStage < nMaxStage)
	{
		wsprintf(Message, TEXT("스테이지 : %d"), nStage + 1);
	}
	TextOut(hdc, 700, 120, Message, lstrlen(Message));
	wsprintf(Message, TEXT("이동 횟수 : %d"), nMove);
	TextOut(hdc, 700, 140, Message, lstrlen(Message));
	if (RecordsSize[nStage] != 0 && nStage < nMaxStage)
	{
		wsprintf(Message, TEXT("기록 순위"));
		TextOut(hdc, 700, 180, Message, lstrlen(Message));
		for (int i = 0; i < RecordsSize[nStage]; i++)
		{
			wsprintf(Message, TEXT("%d위 : %d"), i + 1, Records[nStage][i]);
			TextOut(hdc, 700, 180 + 20 * (i + 1), Message, lstrlen(Message));
		}
	}
}

BOOL TestEnd()
{ // 모든 목표위치에 상자가 위치되어 있는지 검사하는 함수
	int x, y;
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			if (TempStage[y][x] == '.' && ns[y][x] != '!') //  모든 목표위치에 상자가 위치되지 않았을 때 
			{
				return FALSE;
			}
		}
	}
	/*if (nStage < nMaxStage) 
	{
		SortRecords(Records[nStage]);  
	}*/
	SortRecords(Records[nStage]); // 현재 스테이지의 기록을 정렬하는 함수
	return TRUE;
}

void Move(int dir)
{
	int dx = 0, dy = 0; // 이동할 거리와 방향을 결정하는 변수
	BOOL bWithPack = FALSE; // 캐릭터가 혼자 이동할 때와 캐릭터가 짐을 옮길 때를 구분해 사운드를 내기 위한 플래그
	BOOL Success = FALSE; // 목표위치에 박스가 위치될 때 사운드를 내기 위한 플래그 
	int i;
	switch (dir)
	{
	case VK_LEFT:
		ManBit = 4; // ManBit는 캐릭터의 비트맵. 상,하,좌,우 별로 다른 비트맵을 지정
		dx = -1;
		break;
	case VK_RIGHT:
		ManBit = 5;
		dx = 1;
		break;
	case VK_UP:
		ManBit = 6;
		dy = -1;
		break;
	case VK_DOWN:
		ManBit = 7;
		dy = 1;
		break;
	}
	if (ns[ny + dy][nx + dx] != '#') // ns는 매 이동마다 갱신되는 현재 스테이지의 상황을 저장하는 배열
	{ // 다음 칸이 벽이 아닐 때
		if (ns[ny + dy][nx + dx] == 'O' || ns[ny + dy][nx + dx] == '!')
		{ // 다음 칸이 짐이거나 목표위치에 위치된 짐일 때
			if (ns[ny + dy * 2][nx + dx * 2] == ' ' || ns[ny + dy * 2][nx + dx * 2] == '.')
			{ // 다음 다음 칸이 빈 공간이거나 목표위치일 때
				ErasePack(nx + dx, ny + dy); // 현재 스테이지를 저장한 배열과 비교해 인수로 주어진 좌표에 .문자 또는 공백문자를 저장하는 함수
				bWithPack = TRUE;
				if (ns[ny + dy * 2][nx + dx * 2] == '.')
				{ // 다음 다음 칸이 목표 위치일 때
					Success = TRUE;
					ns[ny + dy * 2][nx + dx * 2] = '!'; // 목표위치에 위치된 짐을 나타내는 문자를 저장
				}
				else if (ns[ny + dy * 2][nx + dx * 2] == ' ')
				{ // 다음 다음 칸이 빈 공간일 때
					ns[ny + dy * 2][nx + dx * 2] = 'O'; // 짐을 나타내는 문자를 저장
				}
			}
			else
			{
				return;
			}
		}
		nx += dx;
		ny += dy;
		nMove++; // 이동횟수 증가
		MOVEINFO[UndoIdx].dx = dx; // 
		MOVEINFO[UndoIdx].dy = dy; //
		MOVEINFO[UndoIdx].bWithPack = bWithPack; // 이동 취소/재실행 정보를 저장하는 구조체 배열에 정보를 저장
		UndoIdx++;
		MOVEINFO[UndoIdx].dx = -2; // 정보가 저장된 배열에 위치와 아직 저장되지 않은 배열의 위치를 구분하는 수 -2
		if (UndoIdx == MAXUNDO - 1)
		{
			for (i = 100; i < UndoIdx; i++)
			{
				MOVEINFO[i - 100] = MOVEINFO[i];
			}
			for (i = MAXUNDO - 100; i < MAXUNDO; i++)
			{
				MOVEINFO[i].dx = -2;
			}
			UndoIdx -= 100;
		}
		if (bSound)
		{
			PlaySound(MAKEINTRESOURCE(bWithPack ? IDR_WITHPACK : IDR_MOVE), g_hInst, SND_RESOURCE | SND_ASYNC);
		}
		if (bSound && Success)
		{
			PlaySound(MAKEINTRESOURCE(IDR_SUCCESS), g_hInst, SND_RESOURCE | SND_ASYNC);
		}
		InvalidateRect(hWndMain, NULL, FALSE);
	}
}

void InitStage()
{
	int x, y;
	ManBit = 4;
	if (CustomFlag)
	{ // 사용자 정의 맵 불러오기를 사용했을 때
		/*memcpy(TempStage, nCustomStage, sizeof(TempStage));
		AlreadyDoneCheck();
		memcpy(ns, nCustomStage, sizeof(ns));*/
		nStage = nMaxStage - 1; // 불러온 맵은 마지막 스테이지가 됨
	}
	memcpy(TempStage, arStage[nStage], sizeof(TempStage)); // arStage 배열은 게임이 가지고 있는 전체의 맵을 저장하는 3차원 배열
	AlreadyDoneCheck(); // !문자를 .문자로 변환해 비교를 위한 현재 스테이지 배열에 저장하는 함수
	memcpy(ns, arStage[nStage], sizeof(ns));
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			if (ns[y][x] == '@')
			{ // 캐릭터의 좌표를 찾아 현재 좌표 변수에 저장
				nx = x;
				ny = y;
				ns[y][x] = ' ';
			}
		}
	}
	nMove = 0;
	UndoIdx = 0;
	for (x = 0; x < MAXUNDO; x++)
	{
		MOVEINFO[x].dx = -2;
	}
	InvalidateRect(hWndMain, NULL, TRUE);
}

void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit)
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx, by;
	BITMAP bit;
	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);
	GetObject(hBit, sizeof(BITMAP), &bit);
	bx = bit.bmWidth;
	by = bit.bmHeight;
	BitBlt(hdc, x, y, bx, by, MemDC, 0, 0, SRCCOPY);
	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}

void ErasePack(int x, int y)
{
	/*if ((arStage[nStage][y][x] == '.' || arStage[nStage][y][x] == '!') && !CustomFlag) 
	{
		ns[y][x] = '.';
	}
	if ((nCustomStage[y][x] == '.' || nCustomStage[y][x] == '!') && CustomFlag)
	{
		ns[y][x] = '.';
	}
	if ((arStage[nStage][y][x] != '.' && arStage[nStage][y][x] != '!') && !CustomFlag)
	{
		ns[y][x] = ' ';
	}
	if ((nCustomStage[y][x] != '.' && nCustomStage[y][x] != '!') && CustomFlag)
	{
		ns[y][x] = ' ';
	}*/
	if (TempStage[y][x] == '.')
	{ // (x,y)의 좌표가 원래 목표 위치였다면
		ns[y][x] = '.';
	}
	else
	{ // (x,y)의 좌표가 원래 빈 공간이었다면
		ns[y][x] = ' ';
	}
}

void Undo()
{
	if (UndoIdx != 0)
	{
		UndoIdx--;
		if (MOVEINFO[UndoIdx].bWithPack)
		{
			ErasePack(nx + MOVEINFO[UndoIdx].dx, ny + MOVEINFO[UndoIdx].dy);
			if (TempStage[ny][nx] == '.')
			{
				ns[ny][nx] = '!';
			}
			else
			{
				ns[ny][nx] = 'O';
			}
		}
		nx -= MOVEINFO[UndoIdx].dx;
		ny -= MOVEINFO[UndoIdx].dy;
		InvalidateRect(hWndMain, NULL, FALSE);
	}
}

void Redo()
{
	if (MOVEINFO[UndoIdx].dx != -2)
	{
		nx += MOVEINFO[UndoIdx].dx;
		ny += MOVEINFO[UndoIdx].dy;
		if (MOVEINFO[UndoIdx].bWithPack)
		{
			ErasePack(nx, ny);
			if (TempStage[ny + MOVEINFO[UndoIdx].dy][nx + MOVEINFO[UndoIdx].dx] == '.')
			{
				ns[ny + MOVEINFO[UndoIdx].dy][nx + MOVEINFO[UndoIdx].dx] = '!';
			}
			else
			{
				ns[ny + MOVEINFO[UndoIdx].dy][nx + MOVEINFO[UndoIdx].dx] = 'O';
			}
		}
		InvalidateRect(hWndMain, NULL, FALSE);
		UndoIdx++;
	}
}

void Save()
{
	FILE *f; 
	fopen_s(&f, "Save.dat", "wb");
	if (f != NULL)
	{
		for (int i = 0; i < 18; i++)
		{
			fprintf(f, "%s", ns[i]); // 현재 스테이지 배열을 저장
		}
		fprintf(f, "%d", nStage); // 현재 스테이지를 저장
		/*if (CustomFlag)
		{
			fprintf(f, "%s", NameToChar);
		}*/
		fclose(f);
	}
}

void Load()
{
	FILE* f;
	FILE* fr;
	fopen_s(&f, "Save.dat", "rb");
	if (f != NULL)
	{
		for (int i = 0; i < 18; i++)
		{
			fgets(ns[i], LOAD_LEN, f);
		}
		fscanf_s(f, "%d", &nStage);
		/*if (CustomFlag)
		{
			fgets(NameToChar, 64, f);
		}*/
		fclose(f);
	} // ns배열에 저장되어 있던 맵을 읽어옴
	/*if (CustomFlag)
	{
		MapLoader(NameToChar);
	}*/
	LoadAddStage(); // 추가했던 스테이지들을 불러옴
	fopen_s(&f, "AddList.txt", "r"); // AddList.txt는 현재 추가되어 있는 맵의 이름이 쓰여져 있는 파일
	char tempstr[64];
	memset(tempstr, 0, sizeof(char) * 64);
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", tempstr, 64);
			fopen_s(&fr, tempstr, "r"); // 맵 이름 하나를 읽어와 읽기 전용으로 파일을 연다
			if (fr == NULL || nStage >= nMaxStage)
			{
				if (fr == NULL)
				{ // AddList.txt에 쓰여진 것이 없다면
					fclose(f);
					remove("AddList.txt"); // AddList.txt 파일을 삭제한다
					MessageBox(HWND_DESKTOP, TEXT("추가한 맵을 불러오는 중 오류가 발생했습니다. 스테이지 1로 초기화 합니다."), TEXT("알림"), MB_OK);
				}
				if (nStage >= nMaxStage)
				{ // 현재 저장되어 있는 스테이지의 번호가 현재 게임이 가지고 있는 맵의 개수의 숫자보다 크다면
					MessageBox(HWND_DESKTOP, TEXT("오류가 발생했습니다. 스테이지의 순서를 재구성합니다."), TEXT("알림"), MB_OK);
				}
				errflag = TRUE; // 에러 발생을 알리는 플래그. 추가했던 맵의 점수(기록)들을 초기화 하기 위함
				CustomFlag = FALSE;
				nStage = 0; // 스테이지 1로 초기화 한다.
				break;
			}
			if (fr != NULL) fclose(fr);
			if (feof(f)) break;
		}
		if(f != NULL) fclose(f);
	}
}

void MakeMapList(TCHAR* MapName)
{ // 추가한 맵 파일명을 이어쓰는 함수
	FILE* f;
	fopen_s(&f, "MapList.txt", "a");
	if (f != NULL)
	{
		fprintf(f, "%ls\n", MapName);
		fclose(f);
	}
}

BOOL IsDuplicated(TCHAR* FileName)
{ // 인수로 주어진 맵 파일명이 중복되는지 여부를 확인하는 함수
	FILE* f;
	TCHAR name[64];
	fopen_s(&f, "MapList.txt", "r");
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%ls", name, 64);
			if (feof(f)) break;
			if (lstrcmp(name, FileName) == 0)
			{
				return TRUE;
			}
		}
		fclose(f);
	}
	return FALSE;
} // 중복되면 TRUE를 리턴, 중복되지 않으면 FALSE를 리턴

BOOL MapLoader(char* FileName)
{ // 인수로 주어진 맵 파일명으로 맵 파일에 쓰여져있는 맵을 읽어오는 함수
	FILE* f;
	int i = 0;
	fopen_s(&f, FileName, "r");
	if (f == NULL) return FALSE;
	else
	{
		while (TRUE)
		{
			fread_s(nCustomStage[i], sizeof(nCustomStage[i]), sizeof(char), 21, f); // nCustomStage 배열은 사용자 정의 맵을 읽어 저장하는 배열
			nCustomStage[i][20] = 0;
			if (feof(f)) break;
			i++;
		}
		fclose(f);
		return TRUE;
	}
}

void SortRecords(int record[])
{ // 기록을 정렬하는 함수. 적은 숫자의 기록을 정렬하기 때문에 가장 쉬운 버블정렬을 사용
	BOOL flag = FALSE; // 기록을 저장하는 배열이 다 채워져있는지 여부를 알리는 플래그 
	for (int i = 0; i < 5; i++)
	{
		if (record[i] == 0)
		{
			record[i] = nMove;
			RecordsSize[nStage]++; // RecordsSize는 각 스테이지의 기록을 저장하는 2차원 배열. 각 스테이지의 기록 배열은 최대 5의 길이를 가짐.
			flag = TRUE;
			break;
		}
	}
	if (flag) // 배열에 빈공간이 남아있다면 저장되어 있는 기록의 개수만큼 버블정렬
	{
		for (int i = 1; i < RecordsSize[nStage]; i++)
		{
			for (int j = 0; j < RecordsSize[nStage] - i; j++)
			{
				if (record[j] > record[j + 1])
				{
					int temp = record[j];
					record[j] = record[j + 1];
					record[j + 1] = temp;
				}
			}
		}
	}
	else
	{ // 배열에 빈공간이 남아있지 않다면 최하위 기록과 비교해 더 좋은 기록이면 최하위 기록 위치에 현재 기록을 저장하고 버블정렬
		if (record[4] < nMove)
		{
			return;
		}
		record[4] = nMove;
		for (int i = 1; i < RecordsSize[nStage]; i++)
		{
			for (int j = 0; j < RecordsSize[nStage] - i; j++)
			{
				if (record[j] > record[j + 1])
				{
					int temp = record[j];
					record[j] = record[j + 1];
					record[j + 1] = temp;
				}
			}
		}
	}
}

void WriteRecords()
{ // 각 스테이지 별로 기록을 바이너리 파일에 쓰는 함수
	FILE* f;
	fopen_s(&f, "Records.dat", "wb");
	if (f != NULL)
	{
		for (int i = 0; i < nMaxStage; i++)
		{
			fprintf(f, "%d ", RecordsSize[i]);
			for (int j = 0; j < 5; j++)
			{
				fprintf(f, "%d ", Records[i][j]);
			}
		}
		fclose(f);
	}
}

void ReadRecords()
{ // 파일에 쓰여있는 기록을 불러와 기록배열에 저장하는 함수
	FILE* f;
	fopen_s(&f, "Records.dat", "rb");
	if (f != NULL)
	{
		for (int i = 0; i < nMaxStage; i++)
		{
			fscanf_s(f, "%d ", &RecordsSize[i]);
			for (int j = 0; j < 5; j++)
			{
				fscanf_s(f, "%d ", &Records[i][j]);
			}
		}
		fclose(f);
	}
}

void AlreadyDoneCheck()
{ // !문자를 .문자로 바꿔 저장하는 함수
	for (int i = 0; i < 18; i++)
	{
		for (int j = 0; j < 20; j++)
		{
			TempStage[i][j] = (TempStage[i][j] == '!') ? '.' : TempStage[i][j];
		}
	}
}

BOOL AddStage(char* FileName)
{ // 인수로 주어진 이름의 맵파일에 저장된 맵을 스테이지에 추가하는 함수
	if (nMaxStage == MAXSTAGE)
	{ // 현재 게임이 가지고 있는 최대 스테이지 개수가 20개일 때
		MessageBox(hWndMain, TEXT("더 이상 추가할 수 없습니다."), TEXT("알림"), MB_OK);
		CustomFlag = FALSE;
		return FALSE;
	}
	FILE* f;
	char Name[64];
	memset(Name, 0, sizeof(char) * 64);
	fopen_s(&f, "AddList.txt", "r"); // AddList.txt는 스테이지에 추가된 사용자 정의 맵의 이름이 쓰여있다
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", Name, 64);
			if (feof(f)) break;
			if (strcmp(FileName, Name) == 0) // 인수로 주어진 이름의 맵파일이 존재할 때
			{
				MessageBox(hMDlg, TEXT("동일한 맵이 이미 추가되어 있습니다."), TEXT("알림"), MB_OK);
				CustomFlag = FALSE;
				fclose(f);
				return FALSE;
			}
		}
		fclose(f);
	}
	fopen_s(&f, "AddList.txt", "a");
	if (f != NULL)
	{ // 인수로 주어진 이름의 맵파일이 존재하지 않을 때 AddList.txt에 이어쓴다
		fprintf(f, "%s\n", FileName);
		fclose(f);
		memcpy(arStage[nMaxStage], nCustomStage, sizeof(arStage[nMaxStage])); // 전체 스테이지를 저장하는 3차원 배열 arStage에
		nMaxStage++;                                                          // 불러온 사용자 정의 맵을 저장한다.
	}                                                                         // 현재 게임의 전체 스테이지 개수가 증가 
	return TRUE;
}

void LoadAddStage()
{ // 게임을 처음 실행했을 때 추가했던 스테이지들을 다시 불러와 전체 스테이지를 저장하는 배열에 저장하는 함수
	FILE* f;
	FILE* fr;
	char Name[64];
	char rmName[17][64]; // 최대 추가할 수 있는 맵의 개수가 17개이다.
	int i = 0;
	memset(Name, 0, sizeof(char) * 64);
	fopen_s(&f, "AddList.txt", "r");
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", Name, 64); // 맵 파일 이름 하나를 읽어온다
			if (feof(f)) break;
			fopen_s(&fr, Name, "r"); // 읽어온 맵 파일을 읽기 전용으로 연다
			if (fr == NULL)
			{ // 읽어온 맵 파일이 존재하지 않으면
				strcpy_s(rmName[i], 64, Name); // AddList.txt에서 존재하지 않는 맵 파일의 이름을 지우기 위해 2차원 배열에 저장해둔다 
				i++;
				continue;
			}
			fclose(fr);
			MapLoader(Name); // 읽어온 맵 파일이 존재하면 nCustomStage 배열에 맵을 불러와 저장한다
			memcpy(arStage[nMaxStage], nCustomStage, sizeof(arStage[nMaxStage])); 
			nMaxStage++; // 전체 스테이지를 저장하는 arStage 배열에 불러온 맵을 저장한다. 현재 게임이 보유한 전체 스테이지 개수가 증가한다
		}
		fclose(f);
	}
	for (int j = 0; j < i; j++)
	{
		Remove("AddList.txt", rmName[j]); // 존재하지 않는 맵 파일의 이름을 저장해둔 배열을 이용해 존재하지 않는 맵 파일의 이름을 삭제한다 
	}
}

void Remove(char* Name, char* rm)
{ // 존재하지 않는 맵 파일의 이름을 AddList.txt에서 삭제하는 함수
	FILE* fr;
	FILE* fw;
	char tempstr[64];
	fopen_s(&fr, Name, "r");
	fopen_s(&fw, "temp.txt", "w"); 
	if (fr != NULL && fw != NULL)
	{
		while (TRUE)
		{
			fscanf_s(fr, "%s", tempstr, 64); // AddList.txt에서 이름 하나를 읽어온다
			if (feof(fr)) break;
			if (strcmp(rm, tempstr) != 0) // 존재하지 않는 맵 파일의 이름과 동일하지 않으면
			{
				fprintf(fw, "%s\n", tempstr); // temp.txt에 쓴다. 이렇게 하면 존재하지 않는 맵 파일의 이름은 쓰여지지 않는다 
			}
		}
		fclose(fr);
		fclose(fw);
	}
	remove(Name); // AddList.txt를 지운다.
	rename("temp.txt", Name); // temp.txt의 파일명을 AddList.txt로 바꾼다
}