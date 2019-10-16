#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#include <direct.h>
#include "resource.h"
using namespace std;

#pragma comment(lib,"winmm.lib")

#define MAXSTAGE 20 // �ִ� �������� ����
#define BW 32 // ��¿� ���� ��Ʈ���� �ʺ�
#define BH 32 // ��¿� ���� ��Ʈ���� ����
#define MAXUNDO 1000 // �̵���� ������ ������ �� �ִ� �ִ� ����
#define LOAD_LEN 21
#define MAXSTRING 512 // ���ڹ迭�� ������ �� �ִ� �ִ� ����
#define NOTICE TEXT("���ϸ�(�������θ� �ۼ�).txt�� �Է��ϼ���.")

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
				MessageBox(HWND_DESKTOP, TEXT("����� Ŀ���� ���� �����ϴ�. �������� 1�� �ʱ�ȭ �մϴ�."), TEXT("�˸�"), MB_OK);
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
					wsprintf(Message, TEXT("%d ���������� Ǯ�����ϴ�. ���� ���������� �̵��մϴ�."), nStage + 1);
				}
				MessageBox(hWnd, Message, TEXT("�˸�"), MB_OK);
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
		case ID_LOADCUSTOM: // �޴����� ����� ���� �� �ҷ������� ���̵� 
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_MAKEMAP), hWnd, MapDlgProc) == IDOK)
			{ // IDD_MAKEMAP�� ����� ���� �� �ҷ����� ��ȭ������ ���̵�
				memset(NameToChar, 0, sizeof(char) * 64);
				WideCharToMultiByte(CP_ACP, 0, CMapName, lstrlen(CMapName), NameToChar, 64, 0, 0); // TCHAR* �ڷ����� char* �ڷ������� ��ȯ
				MapLoader(NameToChar); // �� �̸��� �ش��ϴ� �� ������ �ҷ���.
				CustomFlag = TRUE; // ����� ���� �� �ҷ����⸦ ����ߴ��� ������ �˸��� �÷���
				if (AddStage(NameToChar)) // �ҷ��� ���� ���� ������ ������ ���������� ���� ���������� �߰�
				{
					InitStage(); // �ҷ��� ������ �������� �ʱ�ȭ
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
			MessageBox(hWnd, TEXT("������ : ������\nE-mail : bachagalee@gmail.com"), TEXT("����"), MB_OK);
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
{ // ���� ��ȭ������ �޼����� ó���ϴ� ���ν���. ��޸�����
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
		return TRUE; // ��Ʈ�� ���̺��� ����� �� ���̵� �ִ� ���ڿ��� �ҷ���
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
{ // ����� ���� �� �ҷ����� ��ȭ������ �޼����� ó���ϴ� ���ν���. �����
	FILE* f;
	FILE* IsF;
	TCHAR FileName[64];
	char ExistFile[64];
	int i = 0;
	switch (iMessage)
	{
	case WM_INITDIALOG:
		hMapDlg = hDlg;
		fopen_s(&f, "MapList.txt", "r"); // MapList.txt�� �߰��� ���� �̸� ����� �����ִ� ����
		if (f != NULL)                   // MapList.txt�� ���� �̸��� �� �پ� ��������
		{
			while (TRUE)
			{
				memset(ExistFile, 0, sizeof(char) * 64);
				fscanf_s(f, "%ls", FileName, 64);
				if (feof(f)) break;
				WideCharToMultiByte(CP_ACP, 0, FileName, lstrlen(FileName), ExistFile, 64, 0, 0); // TCHAR* �ڷ����� char* �ڷ������� ��ȯ
				fopen_s(&IsF, ExistFile, "r"); // ���� �̸� �ϳ��� �о�� �������� �ִ��� Ȯ���ϱ� ���� �б� �������� ������ ���� 
				if (IsF != NULL)
				{
					SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_ADDSTRING, i, (LPARAM)FileName); // �������� �����ϸ� ����Ʈ�ڽ��� �߰�
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
		case IDC_CMAPLIST: // ����Ʈ �ڽ��� ���̵�
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				i = SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_GETCURSEL, 0, 0); // ������ ����� ����Ʈ �ڽ��� ���° ��������� i�� ����
				SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_GETTEXT, i, (LPARAM)CMapName); // i��° ��Ͽ� �ִ� ���ڿ��� CMapName�� ����
				SetDlgItemText(hDlg, IDC_LOADMAP, CMapName); // IDC_LOADMAP�� edit�� ���̵�. ������ ����� ���ڿ��� edit�� ���
				return TRUE;
			}
			return FALSE;
		case IDOK:
			GetDlgItemText(hDlg, IDC_LOADMAP, CMapName, 64); // edit�� �ִ� ���ڿ��� �о��
			if (lstrcmp(CMapName, NOTICE) == 0)
			{ // �о�� ���ڿ��� ����Ʈ�� ������ ���ڿ��� �����ϸ� �޼��� �ڽ��� ��� �˸�
				MessageBox(hMapDlg, TEXT("�� �̸��� ����� �Է��ϼ���."), TEXT("�˸�"), MB_OK);
				return TRUE;
			}
			memset(ExistFile, 0, sizeof(char) * 64);
			WideCharToMultiByte(CP_ACP, 0, CMapName, lstrlen(CMapName), ExistFile, 64, 0, 0);
			fopen_s(&f, ExistFile, "r");
			if (f == NULL)
			{ // �о�� ���� �̸��� �б� �������� ���� ���� ������ Ȯ��. ������ �޼��� �ڽ��� ��� �˸�
				MessageBox(hMapDlg, TEXT("���� �����ϴ�."), TEXT("�˸�"), MB_OK);
				return TRUE;
			}
			if (!IsDuplicated(CMapName)) // ������ �� �̸��� �ߺ��Ǵ��� �˻�
			{
				MakeMapList(CMapName); // �ߺ��Ǵ� �̸��� ������ MapList.txt�� ���� 
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
	wsprintf(Message, TEXT("Q:����, R:�ٽ� ����"));
	TextOut(hdc, 700, 30, Message, lstrlen(Message));
	wsprintf(Message, TEXT("N:����, P:����"));
	TextOut(hdc, 700, 50, Message, lstrlen(Message));
	wsprintf(Message, TEXT("Z:���, Y:�����"));
	TextOut(hdc, 700, 70, Message, lstrlen(Message));
	wsprintf(Message, TEXT("S:�Ҹ� �ѱ�/����"));
	TextOut(hdc, 700, 90, Message, lstrlen(Message));
	if (nStage < nMaxStage)
	{
		wsprintf(Message, TEXT("�������� : %d"), nStage + 1);
	}
	TextOut(hdc, 700, 120, Message, lstrlen(Message));
	wsprintf(Message, TEXT("�̵� Ƚ�� : %d"), nMove);
	TextOut(hdc, 700, 140, Message, lstrlen(Message));
	if (RecordsSize[nStage] != 0 && nStage < nMaxStage)
	{
		wsprintf(Message, TEXT("��� ����"));
		TextOut(hdc, 700, 180, Message, lstrlen(Message));
		for (int i = 0; i < RecordsSize[nStage]; i++)
		{
			wsprintf(Message, TEXT("%d�� : %d"), i + 1, Records[nStage][i]);
			TextOut(hdc, 700, 180 + 20 * (i + 1), Message, lstrlen(Message));
		}
	}
}

BOOL TestEnd()
{ // ��� ��ǥ��ġ�� ���ڰ� ��ġ�Ǿ� �ִ��� �˻��ϴ� �Լ�
	int x, y;
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			if (TempStage[y][x] == '.' && ns[y][x] != '!') //  ��� ��ǥ��ġ�� ���ڰ� ��ġ���� �ʾ��� �� 
			{
				return FALSE;
			}
		}
	}
	/*if (nStage < nMaxStage) 
	{
		SortRecords(Records[nStage]);  
	}*/
	SortRecords(Records[nStage]); // ���� ���������� ����� �����ϴ� �Լ�
	return TRUE;
}

void Move(int dir)
{
	int dx = 0, dy = 0; // �̵��� �Ÿ��� ������ �����ϴ� ����
	BOOL bWithPack = FALSE; // ĳ���Ͱ� ȥ�� �̵��� ���� ĳ���Ͱ� ���� �ű� ���� ������ ���带 ���� ���� �÷���
	BOOL Success = FALSE; // ��ǥ��ġ�� �ڽ��� ��ġ�� �� ���带 ���� ���� �÷��� 
	int i;
	switch (dir)
	{
	case VK_LEFT:
		ManBit = 4; // ManBit�� ĳ������ ��Ʈ��. ��,��,��,�� ���� �ٸ� ��Ʈ���� ����
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
	if (ns[ny + dy][nx + dx] != '#') // ns�� �� �̵����� ���ŵǴ� ���� ���������� ��Ȳ�� �����ϴ� �迭
	{ // ���� ĭ�� ���� �ƴ� ��
		if (ns[ny + dy][nx + dx] == 'O' || ns[ny + dy][nx + dx] == '!')
		{ // ���� ĭ�� ���̰ų� ��ǥ��ġ�� ��ġ�� ���� ��
			if (ns[ny + dy * 2][nx + dx * 2] == ' ' || ns[ny + dy * 2][nx + dx * 2] == '.')
			{ // ���� ���� ĭ�� �� �����̰ų� ��ǥ��ġ�� ��
				ErasePack(nx + dx, ny + dy); // ���� ���������� ������ �迭�� ���� �μ��� �־��� ��ǥ�� .���� �Ǵ� ���鹮�ڸ� �����ϴ� �Լ�
				bWithPack = TRUE;
				if (ns[ny + dy * 2][nx + dx * 2] == '.')
				{ // ���� ���� ĭ�� ��ǥ ��ġ�� ��
					Success = TRUE;
					ns[ny + dy * 2][nx + dx * 2] = '!'; // ��ǥ��ġ�� ��ġ�� ���� ��Ÿ���� ���ڸ� ����
				}
				else if (ns[ny + dy * 2][nx + dx * 2] == ' ')
				{ // ���� ���� ĭ�� �� ������ ��
					ns[ny + dy * 2][nx + dx * 2] = 'O'; // ���� ��Ÿ���� ���ڸ� ����
				}
			}
			else
			{
				return;
			}
		}
		nx += dx;
		ny += dy;
		nMove++; // �̵�Ƚ�� ����
		MOVEINFO[UndoIdx].dx = dx; // 
		MOVEINFO[UndoIdx].dy = dy; //
		MOVEINFO[UndoIdx].bWithPack = bWithPack; // �̵� ���/����� ������ �����ϴ� ����ü �迭�� ������ ����
		UndoIdx++;
		MOVEINFO[UndoIdx].dx = -2; // ������ ����� �迭�� ��ġ�� ���� ������� ���� �迭�� ��ġ�� �����ϴ� �� -2
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
	{ // ����� ���� �� �ҷ����⸦ ������� ��
		/*memcpy(TempStage, nCustomStage, sizeof(TempStage));
		AlreadyDoneCheck();
		memcpy(ns, nCustomStage, sizeof(ns));*/
		nStage = nMaxStage - 1; // �ҷ��� ���� ������ ���������� ��
	}
	memcpy(TempStage, arStage[nStage], sizeof(TempStage)); // arStage �迭�� ������ ������ �ִ� ��ü�� ���� �����ϴ� 3���� �迭
	AlreadyDoneCheck(); // !���ڸ� .���ڷ� ��ȯ�� �񱳸� ���� ���� �������� �迭�� �����ϴ� �Լ�
	memcpy(ns, arStage[nStage], sizeof(ns));
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			if (ns[y][x] == '@')
			{ // ĳ������ ��ǥ�� ã�� ���� ��ǥ ������ ����
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
	{ // (x,y)�� ��ǥ�� ���� ��ǥ ��ġ���ٸ�
		ns[y][x] = '.';
	}
	else
	{ // (x,y)�� ��ǥ�� ���� �� �����̾��ٸ�
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
			fprintf(f, "%s", ns[i]); // ���� �������� �迭�� ����
		}
		fprintf(f, "%d", nStage); // ���� ���������� ����
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
	} // ns�迭�� ����Ǿ� �ִ� ���� �о��
	/*if (CustomFlag)
	{
		MapLoader(NameToChar);
	}*/
	LoadAddStage(); // �߰��ߴ� ������������ �ҷ���
	fopen_s(&f, "AddList.txt", "r"); // AddList.txt�� ���� �߰��Ǿ� �ִ� ���� �̸��� ������ �ִ� ����
	char tempstr[64];
	memset(tempstr, 0, sizeof(char) * 64);
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", tempstr, 64);
			fopen_s(&fr, tempstr, "r"); // �� �̸� �ϳ��� �о�� �б� �������� ������ ����
			if (fr == NULL || nStage >= nMaxStage)
			{
				if (fr == NULL)
				{ // AddList.txt�� ������ ���� ���ٸ�
					fclose(f);
					remove("AddList.txt"); // AddList.txt ������ �����Ѵ�
					MessageBox(HWND_DESKTOP, TEXT("�߰��� ���� �ҷ����� �� ������ �߻��߽��ϴ�. �������� 1�� �ʱ�ȭ �մϴ�."), TEXT("�˸�"), MB_OK);
				}
				if (nStage >= nMaxStage)
				{ // ���� ����Ǿ� �ִ� ���������� ��ȣ�� ���� ������ ������ �ִ� ���� ������ ���ں��� ũ�ٸ�
					MessageBox(HWND_DESKTOP, TEXT("������ �߻��߽��ϴ�. ���������� ������ �籸���մϴ�."), TEXT("�˸�"), MB_OK);
				}
				errflag = TRUE; // ���� �߻��� �˸��� �÷���. �߰��ߴ� ���� ����(���)���� �ʱ�ȭ �ϱ� ����
				CustomFlag = FALSE;
				nStage = 0; // �������� 1�� �ʱ�ȭ �Ѵ�.
				break;
			}
			if (fr != NULL) fclose(fr);
			if (feof(f)) break;
		}
		if(f != NULL) fclose(f);
	}
}

void MakeMapList(TCHAR* MapName)
{ // �߰��� �� ���ϸ��� �̾�� �Լ�
	FILE* f;
	fopen_s(&f, "MapList.txt", "a");
	if (f != NULL)
	{
		fprintf(f, "%ls\n", MapName);
		fclose(f);
	}
}

BOOL IsDuplicated(TCHAR* FileName)
{ // �μ��� �־��� �� ���ϸ��� �ߺ��Ǵ��� ���θ� Ȯ���ϴ� �Լ�
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
} // �ߺ��Ǹ� TRUE�� ����, �ߺ����� ������ FALSE�� ����

BOOL MapLoader(char* FileName)
{ // �μ��� �־��� �� ���ϸ����� �� ���Ͽ� �������ִ� ���� �о���� �Լ�
	FILE* f;
	int i = 0;
	fopen_s(&f, FileName, "r");
	if (f == NULL) return FALSE;
	else
	{
		while (TRUE)
		{
			fread_s(nCustomStage[i], sizeof(nCustomStage[i]), sizeof(char), 21, f); // nCustomStage �迭�� ����� ���� ���� �о� �����ϴ� �迭
			nCustomStage[i][20] = 0;
			if (feof(f)) break;
			i++;
		}
		fclose(f);
		return TRUE;
	}
}

void SortRecords(int record[])
{ // ����� �����ϴ� �Լ�. ���� ������ ����� �����ϱ� ������ ���� ���� ���������� ���
	BOOL flag = FALSE; // ����� �����ϴ� �迭�� �� ä�����ִ��� ���θ� �˸��� �÷��� 
	for (int i = 0; i < 5; i++)
	{
		if (record[i] == 0)
		{
			record[i] = nMove;
			RecordsSize[nStage]++; // RecordsSize�� �� ���������� ����� �����ϴ� 2���� �迭. �� ���������� ��� �迭�� �ִ� 5�� ���̸� ����.
			flag = TRUE;
			break;
		}
	}
	if (flag) // �迭�� ������� �����ִٸ� ����Ǿ� �ִ� ����� ������ŭ ��������
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
	{ // �迭�� ������� �������� �ʴٸ� ������ ��ϰ� ���� �� ���� ����̸� ������ ��� ��ġ�� ���� ����� �����ϰ� ��������
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
{ // �� �������� ���� ����� ���̳ʸ� ���Ͽ� ���� �Լ�
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
{ // ���Ͽ� �����ִ� ����� �ҷ��� ��Ϲ迭�� �����ϴ� �Լ�
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
{ // !���ڸ� .���ڷ� �ٲ� �����ϴ� �Լ�
	for (int i = 0; i < 18; i++)
	{
		for (int j = 0; j < 20; j++)
		{
			TempStage[i][j] = (TempStage[i][j] == '!') ? '.' : TempStage[i][j];
		}
	}
}

BOOL AddStage(char* FileName)
{ // �μ��� �־��� �̸��� �����Ͽ� ����� ���� ���������� �߰��ϴ� �Լ�
	if (nMaxStage == MAXSTAGE)
	{ // ���� ������ ������ �ִ� �ִ� �������� ������ 20���� ��
		MessageBox(hWndMain, TEXT("�� �̻� �߰��� �� �����ϴ�."), TEXT("�˸�"), MB_OK);
		CustomFlag = FALSE;
		return FALSE;
	}
	FILE* f;
	char Name[64];
	memset(Name, 0, sizeof(char) * 64);
	fopen_s(&f, "AddList.txt", "r"); // AddList.txt�� ���������� �߰��� ����� ���� ���� �̸��� �����ִ�
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", Name, 64);
			if (feof(f)) break;
			if (strcmp(FileName, Name) == 0) // �μ��� �־��� �̸��� �������� ������ ��
			{
				MessageBox(hMDlg, TEXT("������ ���� �̹� �߰��Ǿ� �ֽ��ϴ�."), TEXT("�˸�"), MB_OK);
				CustomFlag = FALSE;
				fclose(f);
				return FALSE;
			}
		}
		fclose(f);
	}
	fopen_s(&f, "AddList.txt", "a");
	if (f != NULL)
	{ // �μ��� �־��� �̸��� �������� �������� ���� �� AddList.txt�� �̾��
		fprintf(f, "%s\n", FileName);
		fclose(f);
		memcpy(arStage[nMaxStage], nCustomStage, sizeof(arStage[nMaxStage])); // ��ü ���������� �����ϴ� 3���� �迭 arStage��
		nMaxStage++;                                                          // �ҷ��� ����� ���� ���� �����Ѵ�.
	}                                                                         // ���� ������ ��ü �������� ������ ���� 
	return TRUE;
}

void LoadAddStage()
{ // ������ ó�� �������� �� �߰��ߴ� ������������ �ٽ� �ҷ��� ��ü ���������� �����ϴ� �迭�� �����ϴ� �Լ�
	FILE* f;
	FILE* fr;
	char Name[64];
	char rmName[17][64]; // �ִ� �߰��� �� �ִ� ���� ������ 17���̴�.
	int i = 0;
	memset(Name, 0, sizeof(char) * 64);
	fopen_s(&f, "AddList.txt", "r");
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", Name, 64); // �� ���� �̸� �ϳ��� �о�´�
			if (feof(f)) break;
			fopen_s(&fr, Name, "r"); // �о�� �� ������ �б� �������� ����
			if (fr == NULL)
			{ // �о�� �� ������ �������� ������
				strcpy_s(rmName[i], 64, Name); // AddList.txt���� �������� �ʴ� �� ������ �̸��� ����� ���� 2���� �迭�� �����صд� 
				i++;
				continue;
			}
			fclose(fr);
			MapLoader(Name); // �о�� �� ������ �����ϸ� nCustomStage �迭�� ���� �ҷ��� �����Ѵ�
			memcpy(arStage[nMaxStage], nCustomStage, sizeof(arStage[nMaxStage])); 
			nMaxStage++; // ��ü ���������� �����ϴ� arStage �迭�� �ҷ��� ���� �����Ѵ�. ���� ������ ������ ��ü �������� ������ �����Ѵ�
		}
		fclose(f);
	}
	for (int j = 0; j < i; j++)
	{
		Remove("AddList.txt", rmName[j]); // �������� �ʴ� �� ������ �̸��� �����ص� �迭�� �̿��� �������� �ʴ� �� ������ �̸��� �����Ѵ� 
	}
}

void Remove(char* Name, char* rm)
{ // �������� �ʴ� �� ������ �̸��� AddList.txt���� �����ϴ� �Լ�
	FILE* fr;
	FILE* fw;
	char tempstr[64];
	fopen_s(&fr, Name, "r");
	fopen_s(&fw, "temp.txt", "w"); 
	if (fr != NULL && fw != NULL)
	{
		while (TRUE)
		{
			fscanf_s(fr, "%s", tempstr, 64); // AddList.txt���� �̸� �ϳ��� �о�´�
			if (feof(fr)) break;
			if (strcmp(rm, tempstr) != 0) // �������� �ʴ� �� ������ �̸��� �������� ������
			{
				fprintf(fw, "%s\n", tempstr); // temp.txt�� ����. �̷��� �ϸ� �������� �ʴ� �� ������ �̸��� �������� �ʴ´� 
			}
		}
		fclose(fr);
		fclose(fw);
	}
	remove(Name); // AddList.txt�� �����.
	rename("temp.txt", Name); // temp.txt�� ���ϸ��� AddList.txt�� �ٲ۴�
}