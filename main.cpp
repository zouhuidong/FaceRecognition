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

// 二值图，0 或 1
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

// 图片拉伸
// width, height 拉伸后的图片大小
// img 原图像
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
 *    参考自 http://tieba.baidu.com/p/5218523817
 *    参数说明：pImg 是原图指针，width1 和 height1 是目标图片的尺寸。
 *    函数功能：将图片进行缩放，返回目标图片。可以自定义宽高；也可以只给宽度，程序自动计算高度
 *    返回目标图片
*/
IMAGE zoomImage(IMAGE* pImg, int newWidth, int newHeight = 0)
{
	// 防止越界
	if (newWidth < 0 || newHeight < 0) {
		newWidth = pImg->getwidth();
		newHeight = pImg->getheight();
	}

	// 当参数只有一个时按比例自动缩放
	if (newHeight == 0) {
		// 此处需要注意先*再/。不然当目标图片小于原图时会出错
		newHeight = newWidth * pImg->getheight() / pImg->getwidth();
	}

	// 获取需要进行缩放的图片
	IMAGE newImg(newWidth, newHeight);

	// 分别对原图像和目标图像获取指针
	DWORD* oldDr = GetImageBuffer(pImg);
	DWORD* newDr = GetImageBuffer(&newImg);

	// 赋值 使用双线性插值算法
	for (int i = 0; i < newHeight - 1; i++) {
		for (int j = 0; j < newWidth - 1; j++) {
			int t = i * newWidth + j;
			int xt = j * pImg->getwidth() / newWidth;
			int yt = i * pImg->getheight() / newHeight;
			newDr[i * newWidth + j] = oldDr[xt + yt * pImg->getwidth()];
			// 实现逐行加载图片
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

// 是否为边缘点
// b 灰度差阈值
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

// 图像二值化
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

// 比较两个二值图，返回相似度
double CmpBin(BinaryGraph bin1, BinaryGraph bin2)
{
	int bin1_white = 0, bin1_black = 0;
	for (int i = 0; i < SIZE; i++)
		for (int j = 0; j < SIZE; j++)
			if (bin1[i][j])
				bin1_white++;
			else
				bin1_black++;

	// 轮廓处的权重
	double power_white = 2;
	double power_black = 0.2;
	double similarity = 0;

	// 稍微移动图像进行比较
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
	//文件句柄
	intptr_t hFile = 0;
	//文件信息，声明一个存储文件信息的结构体
	struct _finddata_t fileinfo;
	string p;//字符串，存放路径
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)//若查找成功，则进入
	{
		do
		{
			//如果是目录,迭代之（即文件夹内还有文件夹）
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				//文件名不等于"."&&文件名不等于".."
				//.表示当前目录
				//..表示当前目录的父目录
				//判断时，两者都要忽略，不然就无限递归跳不出去了！
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
			}
			//如果不是,加入列表
			else
			{
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		//_findclose函数结束查找
		_findclose(hFile);
	}
}

int main()
{
	initgraph(640, 480, SHOWCONSOLE);

	SetWindowPos(GetConsoleWindow(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// 读取所有样本脸谱
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

	// 读入待判断脸图像
	wchar_t lpszFace[512] = { 0 };
	printf("face:");
	wscanf_s(L"%ls", lpszFace, 512);
	IMAGE imgFace;
	loadimage(&imgFace, lpszFace);
	imgFace = zoomImage(&imgFace, SIZE, SIZE);

	SetWindowPos(GetHWnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// 快速浏览载入的人脸，并将其二值化
	for (int i = 0; i < src_num; i++)
	{
		PrintImage(pSrcImg[i]);
		Sleep(100);
		Binarize(&pSrcImg[i]);		// 二值化
		PrintImage(Bin2Img(&pSrcImg[i]));
		Sleep(100);
	}

	// 待检测人脸
	PrintImage(imgFace);
	Sleep(300);
	Binarize(&imgFace);
	PrintImage(Bin2Img(&imgFace));
	Sleep(300);

	// 将二值化图像都转为二值图数组
	BinaryGraph** binSrc = new BinaryGraph * [src_num];
	for (int i = 0; i < src_num; i++)
		binSrc[i] = GetBinaryGraph(&pSrcImg[i]);
	BinaryGraph* binFace = GetBinaryGraph(&imgFace);

	// 和每张脸谱对比，得到最大相似度
	double similarity = 0;
	for (int i = 0; i < src_num; i++)
	{
		double s = CmpBin(*binSrc[i], *binFace);
		if (s > similarity)
			similarity = s;
	}
	printf("%.2f %s\n", similarity, similarity > 0.55 ? "Human" : "Not human");

	// 回收内存
	for (int i = 0; i < src_num; i++)
		DeleteBinaryGraph(binSrc[i]);
	DeleteBinaryGraph(binFace);
	delete[] pSrcImg;

	closegraph();
	return 0;
}
