#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "winmm.lib")//time

//imgui
#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx9.h"
#include "imgui\imgui_impl_win32.h"

//detours
#include "detours.X86\detours.h"
#if defined _M_X64
#pragma comment(lib, "detours.X64/detours.lib")
#elif defined _M_IX86
#pragma comment(lib, "detours.X86/detours.lib")
#endif
/*
#include "DXSDK\d3dx9.h"
#if defined _M_X64
#pragma comment(lib, "DXSDK/x64/d3dx9.lib") 
#elif defined _M_IX86
#pragma comment(lib, "DXSDK/x86/d3dx9.lib")
#endif
*/
//DX Includes
#include <DirectXMath.h>
using namespace DirectX;

#pragma warning (disable: 4244) 

//==========================================================================================================================

//features

//item states
bool chams = 1;			
int textesp = 1;
int circleesp = 1;
int lineesp = 1;
//int chamstype = 0;
//int chamtype = 1;
int aimbot = 1;
int aimkey = 2;
DWORD Daimkey = VK_SHIFT;
int aimsens = 1;
int aimfov = 4;
int aimheight = 12;
int preaim = 1;
bool autoshoot = 0;
unsigned int asdelay = 49;
bool IsPressed = false;
DWORD astime = timeGetTime();

//==========================================================================================================================

HMODULE Hand;

UINT Stride;
D3DVERTEXBUFFER_DESC vdesc;

IDirect3DVertexDeclaration9* pDecl;
D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
UINT numElements;

UINT mStartregister;
UINT mVectorCount;

IDirect3DVertexShader9* vShader;
UINT vSize;

IDirect3DPixelShader9* pShader;
UINT pSize;

UINT tolerance = 1160;//treasure chest

IDirect3DTexture9 *texture;
D3DSURFACE_DESC sDesc;
DWORD qCRC;
D3DLOCKED_RECT pLockedRect;

//Viewport
D3DVIEWPORT9 Viewport;
float ViewportWidth;
float ViewportHeight;
float ScreenCX;
float ScreenCY;

int countnum = 1;

LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow;

bool ShowMenu = false;
bool info = true;
bool is_imgui_initialised = false;
bool is_wndproc_initialised = false;
DWORD wndproctime = timeGetTime();
bool aim = false;

//==========================================================================================================================

// getdir & log
using namespace std;
char dlldir[320];
char* GetDirFile(char *name)
{
	static char pldir[320];
	strcpy_s(pldir, dlldir);
	strcat_s(pldir, name);
	return pldir;
}

void Log(const char *fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetDirFile((PCHAR)"logg.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}
/*
DWORD QuickChecksum(DWORD *pData, int size)
{
	if (!pData) { return 0x0; }

	DWORD sum;
	DWORD tmp;
	sum = *pData;

	for (int i = 1; i < (size / 4); i++)
	{
		tmp = pData[i];
		tmp = (DWORD)(sum >> 29) + tmp;
		tmp = (DWORD)(sum >> 17) + tmp;
		sum = (DWORD)(sum << 3) ^ tmp;
	}

	return sum;
}
*/
// The main window handle of the game.
//HWND game_hwnd = FindWindowA(0, "Deadpool");
//HWND game_hwnd = FindWindowA(0, "Star Wars Battlefront II");

// The main window handle of the game.
HWND game_hwnd = NULL;
/*
// Used to find windows belonging to the game process.
BOOL CALLBACK find_game_hwnd(HWND hwnd, LPARAM game_pid) {
	// Skip windows not belonging to the game process.
	DWORD hwnd_pid = NULL;

	GetWindowThreadProcessId(hwnd, &hwnd_pid);

	if (hwnd_pid != game_pid)
		return TRUE;

	// Set the target window handle and stop the callback.
	game_hwnd = hwnd;

	return FALSE;
}
*/
const char *VariableText(const char *format, ...) {
	va_list argptr;
	va_start(argptr, format);

	char buffer[2048];
	vsprintf_s(buffer, format, argptr); // It should use vsnprintf but meh doesn't matter here

	va_end(argptr);

	return buffer;
}

//==========================================================================================================================

//calc distance
float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct ModelEspInfo_t
{
	float pOutX, pOutY;
	INT iTeam;
	float RealDistance;
	float CrosshairDistance;
};
std::vector<ModelEspInfo_t>ModelEspInfo;

struct vec2
{ float x, y; };

struct vec3
{ float x, y, z; };

struct vec4
{ float x, y, z, w; };

void AddModels(LPDIRECT3DDEVICE9 Device, int iTeam)
{
	DirectX::XMMATRIX pProjection;
	DirectX::XMMATRIX pView;
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);//preaim, aimheight

	Device->GetVertexShaderConstantF(0, (float*)&pProjection, 4);
	Device->GetVertexShaderConstantF(230, (float*)&pView, 4);//230

	DirectX::XMFLOAT4 pOut2d;
	DirectX::XMMATRIX pWorld = DirectX::XMMatrixIdentity();

	//distance
	DirectX::XMMATRIX pWorldViewProj = pProjection * pView;
	float w = Pos.m128_f32[0] * pWorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * pWorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * pWorldViewProj.r[2].m128_f32[3] + pWorldViewProj.r[3].m128_f32[3];

	//normal
	DirectX::XMVECTOR pOut = DirectX::XMVector3Project(Pos, 0, 0, ViewportWidth, ViewportHeight, 0, 1, pProjection, pView, pWorld); //normal

	DirectX::XMStoreFloat4(&pOut2d, pOut);

	ModelEspInfo_t pModelEspInfo;
	pModelEspInfo = { static_cast<float>(pOut2d.x), static_cast<float>(pOut2d.y), iTeam, static_cast<float>(w*0.1f) };
	ModelEspInfo.push_back(pModelEspInfo);
}


//==========================================================================================================================

HRESULT GenerateTexture(IDirect3DDevice9 *pDevice, IDirect3DTexture9 **ppD3Dtex, DWORD colour32)
{
	if (FAILED(pDevice->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);

	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD *pDst16 = (WORD*)d3dlr.pBits;

	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;

	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}

//==========================================================================================================================
/*
//alternative way to generate texture
LPDIRECT3DTEXTURE9 texRed;
LPDIRECT3DTEXTURE9 texGreen;

const BYTE bRed[60] = 
{ 
	0x42, 0x4D, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 
	0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xFF, 0x00, 0x00, 0x01 
};

const BYTE bGreen[60] =
{
	0x42, 0x4D, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0xA0, 0x00, 0x00, 0xFF, 0xFF
};
*/
//==========================================================================================================================

void SaveCfg()
{
	ofstream fout;
	fout.open(GetDirFile((PCHAR)"cdimgui.ini"), ios::trunc);
	fout << "chams " << chams << endl;
	//fout << "chamtypes " << chamtype << endl;
	fout << "textesp " << textesp << endl;
	fout << "circleesp " << circleesp << endl;
	fout << "lineesp " << lineesp << endl;
	fout << "aimbot " << aimbot << endl;
	fout << "aimkey " << aimkey << endl;
	fout << "aimsens " << aimsens << endl;
	fout << "aimfov " << aimfov << endl;
	fout << "aimheight " << aimheight << endl;
	fout << "preaim " << preaim << endl;
	fout << "autoshoot " << autoshoot << endl;
	fout.close();
}

void LoadCfg()
{
	ifstream fin;
	string Word = "";
	fin.open(GetDirFile((PCHAR)"cdimgui.ini"), ifstream::in);
	fin >> Word >> chams;
	//fin >> Word >> chamtype;
	fin >> Word >> textesp;
	fin >> Word >> circleesp;
	fin >> Word >> lineesp;
	fin >> Word >> aimbot;
	fin >> Word >> aimkey;
	fin >> Word >> aimsens;
	fin >> Word >> aimfov;
	fin >> Word >> aimheight;
	fin >> Word >> preaim;
	fin >> Word >> autoshoot;
	fin.close();
}

//==========================================================================================================================
