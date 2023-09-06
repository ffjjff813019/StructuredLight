// GxSingleCamMonoDlg.h : header file
//

#if !defined(AFX_GxSingleCamMonoDLG_H__D8D963C2_3BE2_44EC_B0EB_FCE016DDD6FA__INCLUDED_)
#define AFX_GxSingleCamMonoDLG_H__D8D963C2_3BE2_44EC_B0EB_FCE016DDD6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///< 错误提示函数宏定义
#define  GX_VERIFY(emStatus) \
	                        if (emStatus != GX_STATUS_SUCCESS)\
							{\
							ShowErrorString(emStatus); \
							return ;\
							 } 
///< 判断返回值宏定义
#define VERIFY_STATUS_RET(emStatus) \
	                              if (emStatus != GX_STATUS_SUCCESS) \
								   {\
								   return emStatus;\
								   }\

/////////////////////////////////////////////////////////////////////////////
// CGxSingleCamMonoDlg dialog
//相机头文件
#include "GxIAPI.h"
#include "DxImageProc.h"
//图像显示第三方库文件
#include "CvvImage.h"
//OpenCV头文件
#include <opencv2/opencv.hpp>  
#include <math.h>
using namespace cv;
#include <vector>
#include "afxwin.h"


//使用同步类
#include "afxmt.h"

class CGxSingleCamMonoDlg : public CDialog
{
// Construction
public:
	CGxSingleCamMonoDlg(CWnd* pParent = NULL);	// standard constructor
	~CGxSingleCamMonoDlg();//析构函数
public:
	/// 初始化相机:设置默认参数
	GX_STATUS InitDevice();
	/// 设置相机的数据格式为8bit
	GX_STATUS SetPixelFormat8bit();
	/// 获取设备的宽、高等属性信息
	GX_STATUS GetDeviceParam();
    /// 获取设备参数,初始化界面控件
	void InitUI();
	/// 初始化UI界面参数
	void InitUIParam(GX_DEV_HANDLE hDevice);
    /// 刷新UI界面
	void UpDateUI();
	///  释放为图像显示准备资源
	void UnPrepareForShowImg();
	/// 显示图像
	void DrawImg(Mat,UINT);
	/// 激光平面折射校正
	void LaserPlaneRefraction();
    ///激光条纹图像实时处理
	BOOL ProcessLaserStripeImage(Mat);
    //激光条纹感兴趣区域确定
	BOOL ROIDetermination(Mat&,Mat&);         
	//利用梯度法求取激光条纹中心线
	void LaserCenterLineGradient(Mat,vector<Point2f> &CenterLaserPoint);
	 //对图像二值化以后利用灰度重心法求取激光条纹中心线
	void LaserCenterLineCentroidBw (Mat,vector<Point2f> &CenterLaserPoint);
	//直接对原始图像利用灰度重心法求取激光条纹中心线
	void LaserCenterLineCentroid(Mat,vector<Point2f> &CenterLaserPoint); 
	//利用opencv霍夫变换函数求取激光条纹直线
	bool FindLinesHough(vector<Vec2f> &linsInterest,vector<Point2f> &CenterLaserPoint);
	//得到最终感兴趣的直线
	void GetInterestLine(vector<Vec2f> &linsInterest,vector<Point2f> &CenterLaserPoint);
	//根据给定的点，利用最小二乘拟合直线
	Vec2f LineFitting(vector<Point2f> &fittingPoint);
	//Ostu自适应获取阈值
	double getThreshVal_Otsu_8u( const Mat& _src);
	/// 保存图像
	void SaveImage();
	
	/// 获取GxIAPI错误描述,并弹出错误提示对话框
	void ShowErrorString(GX_STATUS emErrorStatus);

	/// 初始化枚举类型UI界面
	void InitEnumUI(GX_FEATURE_ID_CMD emFeatureID, CComboBox *pComboBox, bool bIsImplement);
	
	/// 初始化增益等相关控件
	void InitGainUI();
	
	/// 初始化曝光相关控件
	void InitShutterUI();

	/// 初始化触发模式相关控件
	void InitTriggerModeUI();

	/// 初始化触发源相关控件
	void InitTriggerSourceUI();

	/// 初始化触发极性相关控件
	void InitTriggerActivationUI();

	/// 回调函数
	static void __stdcall OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM* pFrame);

	///  为图像显示准备资源
	bool PrepareForShowImg();



public:
	CFile file;
	IplImage * image;
	IplImage * lastImage;
	IplImage * curentImage;
	IplImage * lastImageProcess;
	IplImage * curentImageProcess;
	Mat dstImage;                         //图像处理用到的Mat变量
	BOOL    isFirstFrame;                  //采集到的是否为第一帧图像
	BOOL    isFisnshSecondFrame;           //是否完成第二帧的采集

	vector<Point2f> m_CenterLaserPoint;    //激光条纹中心特征点集合
	Point2f m_FeaturePoint;                //图像坐标系下特征点坐标
	Point3f centerLaserPointAir_mm;             //空气中特征点在摄像机坐标系下的坐标
	Point3f centerLaserPointUnderwater_mm;      //折射修正后特征点坐标

//	bool featureLinesFlag;
public:
	GX_DEV_HANDLE           m_hDevice;            ///< 设备句柄	
	int64_t                 m_nPayLoadSize;       ///< 图像大小
	int64_t                 m_nImageHeight;       ///< 原始图像高
    int64_t                 m_nImageWidth;        ///< 原始图像宽	
	int64_t                 m_nTriggerMode;       ///< 触发模式

	double                 m_nAcquisitionFrameRate;   ///设置的采集帧率
	double                 m_nCurrentAcquisitionFrameRate; ///当前的采集帧率
	GX_FLOAT_RANGE          m_stShutterFloatRange;///< 保存曝光时间的范围
	GX_FLOAT_RANGE          m_stGainFloatRange;   ///< 保存增益值的范围

	BYTE                   *m_pBufferRaw;        ///< 图像原始数据
	BYTE                   *m_pImageBuffer;      ///< 保存翻转后的图像用于显示用
	char                    m_chBmpBuf[2048];    ///< BIMTAPINFO 存储缓冲区，m_pBmpInfo即指向此缓冲区
	BOOL                    m_bIsSaveImg;        ///< 标识是否保存图像
	BOOL                    m_bDevOpened;        ///< 标识是否已打开设备
	BOOL                    m_bIsSnap;           ///< 标识是否开始采集

	bool                    m_bTriggerMode;      ///< 是否支持触发模式
	bool                    m_bTriggerActive;    ///< 是否支持触发极性
	bool                    m_bTriggerSource;    ///< 是否支持触发源 

 //   CWnd                    *m_pWnd;             ///< 显示图像窗口(控件)指针
//	HDC                     m_hDC;               ///< 绘制图像DC句柄

// Dialog Data
	//{{AFX_DATA(CGxSingleCamMonoDlg)
	enum { IDD = IDD_GxSingleCamMono_DIALOG };
	double	m_dShutterValue;
	double	m_dGainValue;
	
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGxSingleCamMonoDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGxSingleCamMonoDlg)
	virtual BOOL    OnInitDialog();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnClose();
	afx_msg void OnBnClickedBtnSofttrigger();
	afx_msg void OnCbnSelchangeComboTriggerMode();
	afx_msg void OnBnClickedBtnOpenDevice();
	afx_msg void OnBnClickedBtnCloseDevice();
	afx_msg void OnBnClickedBtnStopSnap();
	afx_msg void OnBnClickedCheckSaveImage();
	afx_msg void OnBnClickedBtnStartSnap();

	afx_msg void OnCbnSelchangeComboTriggerActive();
	afx_msg void OnCbnSelchangeComboTriggerSource();

	afx_msg void OnKillfocusEditShutter();
	afx_msg void OnKillfocusEditGain();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	
	
	afx_msg void OnEnKillfocusEditFramerate();
	double m_dFrameRate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GxSingleCamMonoDLG_H__D8D963C2_3BE2_44EC_B0EB_FCE016DDD6FA__INCLUDED_)
