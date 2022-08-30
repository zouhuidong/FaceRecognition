#include <graphics.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <io.h>
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")
using namespace std;

#define SIZE		32
#define THRESHOLD	0.55

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
bool isEdgePoint(int x, int y, UINT b, DWORD* pBuf, int w, int h)
{
	POINT t[4] = { {0,1}, {0,-1}, {1,0}, {-1,0} };
	int r = pBuf[x + y * w] & 0xff;
	for (int i = 0; i < 4; i++)
	{
		int nx = x + t[i].x, ny = y + t[i].y;
		if (nx >= 0 && nx < w - 2 && ny >= 0 && ny < h - 2
			&& (int)(pBuf[nx + ny * w] & 0xff) - r >(int)b)
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
			pBuf[i + j * w] = isEdgePoint(i, j, b, GetImageBuffer(img), w, h);

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
	POINT neibor[4] = { {0,1}, {0,-1}, {1,0}, {-1,0} };
	//POINT t[9] = { {0,0}, {0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {-1,-1}, {-1,1}, {1,-1} };
	POINT t[1] = { {0,0} };
	for (int k = 0; k < 1; k++)
	{
		double count = 0;
		for (int i = 0, ni = t[k].x; i >= 0 && i < SIZE; i++, ni++)
			for (int j = 0, nj = t[k].y; j >= 0 && j < SIZE; j++, nj++)
				if (ni >= 0 && ni < SIZE && nj >= 0 && nj < SIZE)
				{
					bool equal = false;
					if (bin1[i][j] == bin2[ni][nj])
					{
						equal = true;
						count += bin1[i][j] ? power_white : power_black;
					}
					else
					{
						for (int m = 0; m < 4; m++)
						{
							int x = i + neibor[m].x;
							int y = j + neibor[m].y;
							if (x >= 0 && x < SIZE && y >= 0 && y < SIZE)
								if (bin1[x][y] == bin2[ni][nj])
								{
									equal = true;
									count += bin1[x][y] ? power_white : power_black;
								}
						}
					}

					if (!equal)
					{
						count -= 0.5;
					}
				}

		double s = count / ((double)bin1_white * power_white + bin1_black * power_black);
		if (s > similarity)
			if (s >= 1)
				similarity = 0.99;
			else
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

void PrintImage(IMAGE img, bool b_zoom = true)
{
	int zoom = 10;
	if (b_zoom)
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

// ��ɫͼ��ת��Ϊ�Ҷ�ͼ��
void  ColorToGray(IMAGE* pimg)
{
	DWORD* p = GetImageBuffer(pimg);	// ��ȡ��ʾ������ָ��
	COLORREF c;

	// ������ص��ȡ����
	for (int i = pimg->getwidth() * pimg->getheight() - 1; i >= 0; i--)
	{
		c = BGR(p[i]);
		c = (GetRValue(c) * 299 + GetGValue(c) * 587 + GetBValue(c) * 114 + 500) / 1000;
		p[i] = RGB(c, c, c);	// �����ǻҶ�ֵ��������ִ�� BGR ת��
	}
}

// Ѱ������
// binSrc	������ֵͼ�����ָ��
// src_num	��������
// img		����ͼ��
// imgBin	����չʾ�� bin ͼ��
vector<RECT> FindFace(BinaryGraph** binSrc, size_t src_num, IMAGE* img, IMAGE* imgBin)
{
	// ��������
	vector<RECT> vecRct;

	int img_w = img->getwidth();
	int img_h = img->getheight();

	// �������α߳�
	int begin_size = 30;
	int max_size = img_w > img_h ? img_h : img_w;
	int step = 10;		// �߳�����
	int interval;		// �������

	// ��ǰ��������
	int x = 0, y = 0;
	int n_size = begin_size;

	for (int n_size = begin_size; n_size < max_size; n_size += step)
	{
		interval = n_size / 2;
		for (int x = 0, exit_x = 0; x < img_w - 1 && !exit_x; x += interval)
		{
			// Խ�����
			if (x + n_size >= img_w)
			{
				x = img_w - n_size - 1;
				exit_x = 1;
			}

			for (int y = 0, exit_y = 0; y < img_h - 1 && !exit_y; y += interval)
			{
				if (y + n_size >= img_h)
				{
					y = img_h - n_size - 1;
					exit_y = 1;
				}

				putimage(0, 0, imgBin);

				IMAGE region;
				SetWorkingImage(img);
				getimage(&region, x, y, n_size, n_size);
				SetWorkingImage();

				//region = zoomImage(&region, SIZE, SIZE);
				ImageToSize(SIZE, SIZE, &region);


				//Binarize(&region);		// ��ֵ��

				rectangle(x, y, x + n_size, y + n_size);

				// ��ȡ��ֵͼ
				BinaryGraph* bin = GetBinaryGraph(&region);

				// ��ÿ�����׶Աȣ��õ�������ƶ�
				double similarity = 0;
				for (int i = 0; i < src_num; i++)
				{
					double s = CmpBin(*binSrc[i], *bin);
					if (s > similarity)
						similarity = s;
				}

				// �ж�Ϊ����
				if (similarity > THRESHOLD)
				{
					vecRct.push_back({ x,y,x + n_size,y + n_size });
					printf("Found human\n");
				}

				DeleteBinaryGraph(bin);
			}
		}
	}

	return vecRct;
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

	// ������������
	size_t src_num = src_wpath.size();
	IMAGE* pSrcImg = new IMAGE[src_num];
	BinaryGraph** binSrc = new BinaryGraph * [src_num];		// ��������ֵ��ͼ��תΪ��ֵͼ����
	for (int i = 0; i < src_num; i++)
	{
		loadimage(&pSrcImg[i], src_wpath[i].c_str());
		ColorToGray(&pSrcImg[i]);							// ת�Ҷ�
		pSrcImg[i] = zoomImage(&pSrcImg[i], SIZE, SIZE);	// ����С
		Binarize(&pSrcImg[i]);								// ��ֵ��
		binSrc[i] = GetBinaryGraph(&pSrcImg[i]);			// ת��Ϊ��ֵͼ�ṹ
	}

	//---------- ��ʼ����� -----------

	// �û�����ͼ��
	wchar_t lpszFace[512] = { 0 };
	printf("pic path: ");
	wscanf_s(L"%ls", lpszFace, 512);
	IMAGE imgFace;
	loadimage(&imgFace, lpszFace);
	IMAGE imgFaceOrigin = imgFace;		// ����ԭͼ��
	ColorToGray(&imgFace);
	Binarize(&imgFace);

	SetWindowPos(GetHWnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	getmessage(EM_CHAR);

	IMAGE imgBin = Bin2Img(&imgFace);

	// ����
	setlinestyle(PS_SOLID, 2);
	setlinecolor(RED);
	vector<RECT> vecRct = FindFace(binSrc, src_num, &imgFace, &imgBin);
	printf("Done.\n");
	printf("Count: %I64u\n", vecRct.size());

	// չʾ
	putimage(0, 0, &imgFaceOrigin);
	for (auto& rct : vecRct)
		rectangle(rct.left, rct.top, rct.right, rct.bottom);

	getmessage(EM_CHAR);



	// �����ڴ�
	for (int i = 0; i < src_num; i++)
		DeleteBinaryGraph(binSrc[i]);
	delete[] pSrcImg;

	closegraph();
	return 0;
}
