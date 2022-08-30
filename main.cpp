#include <graphics.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <io.h>
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")
using namespace std;

#define SIZE		64
#define THRESHOLD	0
#define OUTLINE		10

// ��ֵͼ��0 �� 1
// ����Ȩ�ر�ʱ����ֵ���޷�Χ
typedef int BinaryGraph[SIZE][SIZE];

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
			byte r = GetRValue(oldDr[xt + yt * pImg->getwidth()]);
			byte g = GetGValue(oldDr[xt + yt * pImg->getwidth()]);
			byte b = GetBValue(oldDr[xt + yt * pImg->getwidth()]);
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
void Binarize(IMAGE* img, UINT b = OUTLINE)
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

// ����ĳ����ֵͼ��Ȩ�ر���Ǻ϶ȣ����ذٷֱ�
double CalcWeight(BinaryGraph& binWeight, int weight_sum, BinaryGraph bin)
{
	// ��¼Ȩ�ص÷�
	int score = 0;
	for (int x = 0; x < SIZE; x++)
		for (int y = 0; y < SIZE; y++)
			score += binWeight[x][y] * bin[x][y];
	double b = (double)score / weight_sum;
	return b;
}

BinaryGraph* GetBinaryGraph(IMAGE* img)
{
	BinaryGraph* p = (BinaryGraph*)new BinaryGraph;
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
// binWeight	����Ȩ�ر�
// weight_sum	Ȩ���ܺ�
// img			����ͼ��
// imgBin		����չʾ�� bin ͼ��
vector<RECT> FindFace(BinaryGraph& binWeight, int weight_sum, IMAGE* img, IMAGE* imgBin)
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
		interval = (int)(n_size / 4);
		if (interval == 0)
			interval = 1;
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

				//x = 106;
				//y = 106;
				//n_size = 40;

				putimage(0, 0, img);

				IMAGE region;			// ��ȡ��Ƥ
				SetWorkingImage(img);
				getimage(&region, x, y, n_size, n_size);
				SetWorkingImage();

				// ������Ƥ
				region = zoomImage(&region, SIZE, SIZE);
				//ImageToSize(SIZE, SIZE, &region);

				// ��ֵ��
				Binarize(&region);
				BinaryGraph* bin = GetBinaryGraph(&region);

				rectangle(x, y, x + n_size, y + n_size);

				//// ��ȡ���ƶ�
				//double s = CmpBin(*binSrc[i], *bin);
				double s = CalcWeight(binWeight, weight_sum, *bin);

				// �ж�Ϊ����
				if (s > THRESHOLD)
				{
					vecRct.push_back({ x,y,x + n_size,y + n_size });
					printf("Found human\n");
				}
				printf("%.2f\n", s);

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
	BinaryGraph* binWeight = (BinaryGraph*)new BinaryGraph;	// ���������ĵ���Ȩ��
	int weight_sum = 0;		// Ȩ�غ�

	// ��ʼ��
	for (int x = 0; x < SIZE; x++)
		for (int y = 0; y < SIZE; y++)
			(*binWeight)[x][y] = 0;

	// ����Ȩ��
	for (int i = 0; i < src_num; i++)
	{
		loadimage(&pSrcImg[i], src_wpath[i].c_str());
		ColorToGray(&pSrcImg[i]);							// ת�Ҷ�
		pSrcImg[i] = zoomImage(&pSrcImg[i], SIZE, SIZE);	// ����С
		Binarize(&pSrcImg[i]);								// ��ֵ��
		BinaryGraph* binThis = GetBinaryGraph(&pSrcImg[i]);	// ת��Ϊ��ֵͼ�ṹ

		// ����ǰ��������Ϣ���ӵ�Ȩ�ر�
		for (int x = 0; x < SIZE; x++)
			for (int y = 0; y < SIZE; y++)
				(*binWeight)[x][y] += (*binThis)[x][y] ? 5 : -3;

		DeleteBinaryGraph(binThis);
	}

	// ����Ȩ�غ�
	for (int x = 0; x < SIZE; x++)
		for (int y = 0; y < SIZE; y++)
			if ((*binWeight)[x][y] > 0)
				weight_sum += (*binWeight)[x][y];

	// չʾһ��Ȩ��ͼ
	const int block_size = 5;
	for (int x = 0; x < SIZE; x++)
		for (int y = 0; y < SIZE; y++)
		{
			int gray = (*binWeight)[x][y];
			if (gray < 0)
				setfillcolor(RGB(-gray, 0, 0));
			else
				setfillcolor(RGB(gray, gray, gray));
			solidrectangle(x * block_size, y * block_size, (x + 1) * block_size, (y + 1) * block_size);
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
	IMAGE imgBin = /*Bin2Img(&*/imgFace/*)*/;	// ����չʾ�Ķ�ֵͼ

	SetWindowPos(GetHWnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// ���Ե�������
	IMAGE imgOneFace = zoomImage(&imgFace, SIZE, SIZE);
	Binarize(&imgOneFace);
	IMAGE imgOneFaceBin = Bin2Img(&imgOneFace);
	DWORD* pBufOneFaceBin = GetImageBuffer(&imgOneFaceBin);
	//putimage(0, 0, &imgOneFaceBin);
	setfillcolor(BLUE);
	for (int x = 0; x < SIZE; x++)
		for (int y = 0; y < SIZE; y++)
			if (pBufOneFaceBin[x + y * SIZE])
				solidrectangle(x * block_size, y * block_size, (x + 1) * block_size, (y + 1) * block_size);
	BinaryGraph* gOneFace = GetBinaryGraph(&imgOneFace);
	printf("%.2f", CalcWeight(*binWeight, weight_sum, *gOneFace));
	DeleteBinaryGraph(gOneFace);
	getmessage(EM_CHAR);

	// ����
	setlinestyle(PS_SOLID, 2);
	setlinecolor(RED);
	vector<RECT> vecRct = FindFace(*binWeight, weight_sum, &imgFace, &imgBin);
	printf("Done.\n");
	printf("Count: %I64u\n", vecRct.size());

	// չʾ
	putimage(0, 0, &imgFaceOrigin);
	for (auto& rct : vecRct)
		rectangle(rct.left, rct.top, rct.right, rct.bottom);

	getmessage(EM_CHAR);

	// �����ڴ�
	DeleteBinaryGraph(binWeight);
	delete[] pSrcImg;

	closegraph();
	return 0;
}
