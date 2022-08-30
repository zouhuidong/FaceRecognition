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

// 二值图，0 或 1
// 用作权重表时，数值不限范围
typedef int BinaryGraph[SIZE][SIZE];

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
			byte r = GetRValue(oldDr[xt + yt * pImg->getwidth()]);
			byte g = GetGValue(oldDr[xt + yt * pImg->getwidth()]);
			byte b = GetBValue(oldDr[xt + yt * pImg->getwidth()]);
			newDr[i * newWidth + j] = RGB(r, g, b);
		}
	}

	return newImg;
}

// 是否为边缘点
// b 灰度差阈值
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

// 图像二值化
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

// 计算某个二值图和权重表的吻合度，返回百分比
double CalcWeight(BinaryGraph& binWeight, int weight_sum, BinaryGraph bin)
{
	// 记录权重得分
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

// 彩色图像转换为灰度图像
void  ColorToGray(IMAGE* pimg)
{
	DWORD* p = GetImageBuffer(pimg);	// 获取显示缓冲区指针
	COLORREF c;

	// 逐个像素点读取计算
	for (int i = pimg->getwidth() * pimg->getheight() - 1; i >= 0; i--)
	{
		c = BGR(p[i]);
		c = (GetRValue(c) * 299 + GetGValue(c) * 587 + GetBValue(c) * 114 + 500) / 1000;
		p[i] = RGB(c, c, c);	// 由于是灰度值，无需再执行 BGR 转换
	}
}

// 寻找人脸
// binWeight	样本权重表
// weight_sum	权重总和
// img			搜索图像
// imgBin		用于展示的 bin 图像
vector<RECT> FindFace(BinaryGraph& binWeight, int weight_sum, IMAGE* img, IMAGE* imgBin)
{
	// 人脸区域
	vector<RECT> vecRct;

	int img_w = img->getwidth();
	int img_h = img->getheight();

	// 搜索矩形边长
	int begin_size = 30;
	int max_size = img_w > img_h ? img_h : img_w;
	int step = 10;		// 边长步进
	int interval;		// 搜索间隔

	// 当前搜索进度
	int x = 0, y = 0;
	int n_size = begin_size;

	for (int n_size = begin_size; n_size < max_size; n_size += step)
	{
		interval = (int)(n_size / 4);
		if (interval == 0)
			interval = 1;
		for (int x = 0, exit_x = 0; x < img_w - 1 && !exit_x; x += interval)
		{
			// 越界回缩
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

				IMAGE region;			// 获取人皮
				SetWorkingImage(img);
				getimage(&region, x, y, n_size, n_size);
				SetWorkingImage();

				// 拉伸人皮
				region = zoomImage(&region, SIZE, SIZE);
				//ImageToSize(SIZE, SIZE, &region);

				// 二值化
				Binarize(&region);
				BinaryGraph* bin = GetBinaryGraph(&region);

				rectangle(x, y, x + n_size, y + n_size);

				//// 获取相似度
				//double s = CmpBin(*binSrc[i], *bin);
				double s = CalcWeight(binWeight, weight_sum, *bin);

				// 判断为人脸
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

	// 读取所有样本脸谱
	vector<string> src_path;
	vector<wstring> src_wpath;
	getFiles(".\\res", src_path);
	for (auto& path : src_path)
		src_wpath.push_back(stow(path));

	// 处理所有样本
	size_t src_num = src_wpath.size();
	IMAGE* pSrcImg = new IMAGE[src_num];
	BinaryGraph* binWeight = (BinaryGraph*)new BinaryGraph;	// 所有样本的叠加权重
	int weight_sum = 0;		// 权重和

	// 初始化
	for (int x = 0; x < SIZE; x++)
		for (int y = 0; y < SIZE; y++)
			(*binWeight)[x][y] = 0;

	// 计算权重
	for (int i = 0; i < src_num; i++)
	{
		loadimage(&pSrcImg[i], src_wpath[i].c_str());
		ColorToGray(&pSrcImg[i]);							// 转灰度
		pSrcImg[i] = zoomImage(&pSrcImg[i], SIZE, SIZE);	// 定大小
		Binarize(&pSrcImg[i]);								// 二值化
		BinaryGraph* binThis = GetBinaryGraph(&pSrcImg[i]);	// 转换为二值图结构

		// 将当前样本的信息叠加到权重表
		for (int x = 0; x < SIZE; x++)
			for (int y = 0; y < SIZE; y++)
				(*binWeight)[x][y] += (*binThis)[x][y] ? 5 : -3;

		DeleteBinaryGraph(binThis);
	}

	// 计算权重和
	for (int x = 0; x < SIZE; x++)
		for (int y = 0; y < SIZE; y++)
			if ((*binWeight)[x][y] > 0)
				weight_sum += (*binWeight)[x][y];

	// 展示一下权重图
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

	//---------- 初始化完毕 -----------

	// 用户输入图像
	wchar_t lpszFace[512] = { 0 };
	printf("pic path: ");
	wscanf_s(L"%ls", lpszFace, 512);
	IMAGE imgFace;
	loadimage(&imgFace, lpszFace);
	IMAGE imgFaceOrigin = imgFace;		// 备份原图像
	ColorToGray(&imgFace);
	IMAGE imgBin = /*Bin2Img(&*/imgFace/*)*/;	// 用于展示的二值图

	SetWindowPos(GetHWnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// 测试单脸搜索
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

	// 找脸
	setlinestyle(PS_SOLID, 2);
	setlinecolor(RED);
	vector<RECT> vecRct = FindFace(*binWeight, weight_sum, &imgFace, &imgBin);
	printf("Done.\n");
	printf("Count: %I64u\n", vecRct.size());

	// 展示
	putimage(0, 0, &imgFaceOrigin);
	for (auto& rct : vecRct)
		rectangle(rct.left, rct.top, rct.right, rct.bottom);

	getmessage(EM_CHAR);

	// 回收内存
	DeleteBinaryGraph(binWeight);
	delete[] pSrcImg;

	closegraph();
	return 0;
}
