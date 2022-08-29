#include <graphics.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <io.h>
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")
using namespace std;

#define SIZE 32

// ��ֵͼ��0 �� 1
typedef int BinaryGraph[SIZE][SIZE];

string wtos(const wstring& ws)
{
	_bstr_t t = ws.c_str();
	char* pchar = (char*)t;
	string result = pchar;
	return result;
}

wstring stow(const string& s)
{
	_bstr_t t = s.c_str();
	wchar_t* pwchar = (wchar_t*)t;
	wstring result = pwchar;
	return result;
}

// ͼƬ����
// width, height ������ͼƬ��С
// img ԭͼ��
void ImageToSize(int width, int height, IMAGE* img)
{
	IMAGE* pOldImage = GetWorkingImage();
	SetWorkingImage(img);
	IMAGE temp_image(width, height);
	StretchBlt(
		GetImageHDC(&temp_image), 0, 0, width, height,
		GetImageHDC(img), 0, 0,
		getwidth(), getheight(),
		SRCCOPY
	);
	Resize(img, width, height);
	putimage(0, 0, &temp_image);
	SetWorkingImage(pOldImage);
}

/*
 *    �ο��� http://tieba.baidu.com/p/5218523817
 *    ����˵����pImg ��ԭͼָ�룬width1 �� height1 ��Ŀ��ͼƬ�ĳߴ硣
 *    �������ܣ���ͼƬ�������ţ�����Ŀ��ͼƬ�������Զ����ߣ�Ҳ����ֻ����ȣ������Զ�����߶�
 *    ����Ŀ��ͼƬ
*/
IMAGE zoomImage(IMAGE* pImg, int newWidth, int newHeight = 0)
{
	// ��ֹԽ��
	if (newWidth < 0 || newHeight < 0) {
		newWidth = pImg->getwidth();
		newHeight = pImg->getheight();
	}

	// ������ֻ��һ��ʱ�������Զ�����
	if (newHeight == 0) {
		// �˴���Ҫע����*��/����Ȼ��Ŀ��ͼƬС��ԭͼʱ�����
		newHeight = newWidth * pImg->getheight() / pImg->getwidth();
	}

	// ��ȡ��Ҫ�������ŵ�ͼƬ
	IMAGE newImg(newWidth, newHeight);

	// �ֱ��ԭͼ���Ŀ��ͼ���ȡָ��
	DWORD* oldDr = GetImageBuffer(pImg);
	DWORD* newDr = GetImageBuffer(&newImg);

	// ��ֵ ʹ��˫���Բ�ֵ�㷨
	for (int i = 0; i < newHeight - 1; i++) {
		for (int j = 0; j < newWidth - 1; j++) {
			int t = i * newWidth + j;
			int xt = j * pImg->getwidth() / newWidth;
			int yt = i * pImg->getheight() / newHeight;
			newDr[i * newWidth + j] = oldDr[xt + yt * pImg->getwidth()];
			// ʵ�����м���ͼƬ
			byte r = (GetRValue(oldDr[xt + yt * pImg->getwidth()]) +
				GetRValue(oldDr[xt + yt * pImg->getwidth() + 1]) +
				GetRValue(oldDr[xt + (yt + 1) * pImg->getwidth()]) +
				GetRValue(oldDr[xt + (yt + 1) * pImg->getwidth() + 1])) / 4;
			byte g = (GetGValue(oldDr[xt + yt * pImg->getwidth()]) +
				GetGValue(oldDr[xt + yt * pImg->getwidth()] + 1) +
				GetGValue(oldDr[xt + (yt + 1) * pImg->getwidth()]) +
				GetGValue(oldDr[xt + (yt + 1) * pImg->getwidth()]) + 1) / 4;
			byte b = (GetBValue(oldDr[xt + yt * pImg->getwidth()]) +
				GetBValue(oldDr[xt + yt * pImg->getwidth()] + 1) +
				GetBValue(oldDr[xt + (yt + 1) * pImg->getwidth()]) +
				GetBValue(oldDr[xt + (yt + 1) * pImg->getwidth() + 1])) / 4;
			newDr[i * newWidth + j] = RGB(r, g, b);
		}
	}

	return newImg;
}

// �Ƿ�Ϊ��Ե��
// b �ҶȲ���ֵ
bool isEdgePoint(int x, int y, UINT b)
{
	POINT t[4] = { {0,1}, {0,-1}, {1,0}, {-1,0} };
	int r = RGBtoGRAY(getpixel(x, y)) & 0xff;
	for (int i = 0; i < 4; i++)
	{
		int nx = x + t[i].x, ny = y + t[i].y;
		if (nx >= 0 && nx < getwidth() - 2 && ny >= 0 && ny < getheight() - 2
			&& ((int)RGBtoGRAY(getpixel(nx, ny)) & 0xff) - r >(int)b)
			return true;
	}
	return false;
}

// ͼ���ֵ��
void Binarize(IMAGE* img, UINT b = 20)
{
	int w = img->getwidth();
	int h = img->getheight();
	IMAGE bin(w, h);
	IMAGE* pOld = GetWorkingImage();
	SetWorkingImage(img);

	DWORD* pBuf = GetImageBuffer(&bin);
	for (int i = 0; i < w; i++)
		for (int j = 0; j < h; j++)
			pBuf[i + j * w] = isEdgePoint(i, j, b);

	SetWorkingImage(pOld);
	*img = bin;
}

// �Ƚ�������ֵͼ���������ƶ�
double CmpBin(BinaryGraph bin1, BinaryGraph bin2)
{
	int bin1_white = 0, bin1_black = 0;
	for (int i = 0; i < SIZE; i++)
		for (int j = 0; j < SIZE; j++)
			if (bin1[i][j])
				bin1_white++;
			else
				bin1_black++;

	// ��������Ȩ��
	double power_white = 2;
	double power_black = 0.2;
	double similarity = 0;

	// ��΢�ƶ�ͼ����бȽ�
	POINT t[9] = { {0,0}, {0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {-1,-1}, {-1,1}, {1,-1} };
	for (int k = 0; k < 9; k++)
	{
		double count = 0;
		for (int i = 0, ni = t[k].x; i >= 0 && i < SIZE; i++, ni++)
			for (int j = 0, nj = t[k].y; j >= 0 && j < SIZE; j++, nj++)
				if (ni >= 0 && ni < SIZE && nj >= 0 && nj < SIZE)
				{
					if (bin1[i][j] == bin2[ni][nj])
					{
						count += bin1[i][j] ? power_white : power_black;
					}
					else
					{
						count -= 0.2;
					}
				}

		double s = count / ((double)bin1_white * power_white + bin1_black * power_black);
		if (s > similarity)
			similarity = s;
	}

	return similarity;
}

BinaryGraph* GetBinaryGraph(IMAGE* img)
{
	BinaryGraph* p = (BinaryGraph*)new int[SIZE][SIZE];
	ZeroMemory(p, SIZE * SIZE * sizeof(int));
	DWORD* pBuf = GetImageBuffer(img);
	for (int i = 0; i < SIZE; i++)
		for (int j = 0; j < SIZE; j++)
			(*p)[i][j] = pBuf[i + j * SIZE];
	return p;
}

void DeleteBinaryGraph(BinaryGraph* p)
{
	delete[] p;
}

IMAGE Bin2Img(IMAGE* p)
{
	IMAGE img = *p;
	DWORD* pBuf = GetImageBuffer(&img);
	for (int i = 0; i < img.getwidth() * img.getheight(); i++)
		pBuf[i] *= 200;
	return img;
}

void PrintImage(IMAGE img)
{
	int zoom = 10;
	ImageToSize(SIZE * zoom, SIZE * zoom, &img);
	cleardevice();
	putimage(0, 0, &img);
}

void getFiles(string path, vector<string>& files)
{
	//�ļ����
	intptr_t hFile = 0;
	//�ļ���Ϣ������һ���洢�ļ���Ϣ�Ľṹ��
	struct _finddata_t fileinfo;
	string p;//�ַ��������·��
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)//�����ҳɹ��������
	{
		do
		{
			//�����Ŀ¼,����֮�����ļ����ڻ����ļ��У�
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				//�ļ���������"."&&�ļ���������".."
				//.��ʾ��ǰĿ¼
				//..��ʾ��ǰĿ¼�ĸ�Ŀ¼
				//�ж�ʱ�����߶�Ҫ���ԣ���Ȼ�����޵ݹ�������ȥ�ˣ�
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
			}
			//�������,�����б�
			else
			{
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		//_findclose������������
		_findclose(hFile);
	}
}

int main()
{
	initgraph(640, 480, SHOWCONSOLE);

	SetWindowPos(GetConsoleWindow(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// ��ȡ������������
	vector<string> src_path;
	vector<wstring> src_wpath;
	getFiles(".\\res", src_path);
	for (auto& path : src_path)
		src_wpath.push_back(stow(path));

	size_t src_num = src_wpath.size();
	IMAGE* pSrcImg = new IMAGE[src_num];
	for (int i = 0; i < src_num; i++)
	{
		loadimage(&pSrcImg[i], src_wpath[i].c_str());
		pSrcImg[i] = zoomImage(&pSrcImg[i], SIZE, SIZE);
	}

	// ������ж���ͼ��
	wchar_t lpszFace[512] = { 0 };
	printf("face:");
	wscanf_s(L"%ls", lpszFace, 512);
	IMAGE imgFace;
	loadimage(&imgFace, lpszFace);
	imgFace = zoomImage(&imgFace, SIZE, SIZE);

	SetWindowPos(GetHWnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// �������������������������ֵ��
	for (int i = 0; i < src_num; i++)
	{
		PrintImage(pSrcImg[i]);
		Sleep(100);
		Binarize(&pSrcImg[i]);		// ��ֵ��
		PrintImage(Bin2Img(&pSrcImg[i]));
		Sleep(100);
	}

	// ���������
	PrintImage(imgFace);
	Sleep(300);
	Binarize(&imgFace);
	PrintImage(Bin2Img(&imgFace));
	Sleep(300);

	// ����ֵ��ͼ��תΪ��ֵͼ����
	BinaryGraph** binSrc = new BinaryGraph * [src_num];
	for (int i = 0; i < src_num; i++)
		binSrc[i] = GetBinaryGraph(&pSrcImg[i]);
	BinaryGraph* binFace = GetBinaryGraph(&imgFace);

	// ��ÿ�����׶Աȣ��õ�������ƶ�
	double similarity = 0;
	for (int i = 0; i < src_num; i++)
	{
		double s = CmpBin(*binSrc[i], *binFace);
		if (s > similarity)
			similarity = s;
	}
	printf("%.2f %s\n", similarity, similarity > 0.55 ? "Human" : "Not human");

	// �����ڴ�
	for (int i = 0; i < src_num; i++)
		DeleteBinaryGraph(binSrc[i]);
	DeleteBinaryGraph(binFace);
	delete[] pSrcImg;

	closegraph();
	return 0;
}
