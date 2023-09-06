// GxSingleCamMonoDlg.cpp : implementation file
//
#include <iostream>
#include "stdafx.h"
#include "GxSingleCamMono.h"
#include "GxSingleCamMonoDlg.h"
#include <math.h>
#include <array>
#include <complex>
#include <cmath>


using namespace std;



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
//相机图像常变量的定义
const int dstWidth=1280;
const int dstHeight=1024;
const double PI=3.1416;
const int ROIHeight=30;
//摄像机内参数
const float kx = 1737.5658;
const float ky = 1737.4270;
const float u0 = 646.7194;
const float v0 = 506.2949;

//空气中激光平面参数
const float a1 = 0.0116;
const float b1 = 4.1499;
const float c1 = 165.6944;

//系统参数
const float refractivityAir = 1;
const float refractivityGlass = 1.52;
const float refractivityWater = 1.333;
const float hc = 27.20;  //摄像机光心到玻璃上表面距离
const float d = 3;       //玻璃厚度

//玻璃平面法向量
const float a = -0.0034;
const float b = -0.0116;
const float c = 0.9999;

//激光平面参数
float A1, B1, C1, D1;
float A2, B2, C2, D2;
float A3, B3, C3, D3;

//全局变量的定义
//vector<Point2i> CenterLaserPoint;
int LaserIndex[dstWidth]={0};
int maxRow;

static int nFrame=0;
int nFrametrue;
//实现线程同步的临界区全局变量
CCriticalSection critical_section;



class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_strProductVersion;
	CString	m_strLegalCopyRight;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_strProductVersion = _T("");
	m_strLegalCopyRight = _T("");
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_STATIC_PRODUCTVERSION, m_strProductVersion);
	DDX_Text(pDX, IDC_STATIC_LEGALCOPYRIGHT, m_strLegalCopyRight);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGxSingleCamMonoDlg dialog
//标准构造函数
CGxSingleCamMonoDlg::CGxSingleCamMonoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGxSingleCamMonoDlg::IDD, pParent)
	, m_bIsSaveImg(FALSE)              //参数初始化表对数据成员进行初始化
	, m_bDevOpened(FALSE)
	, m_bIsSnap(FALSE)
	, m_bTriggerMode(false)
	, m_bTriggerActive(false)
	, m_bTriggerSource(false)
	, m_nImageHeight(0)
	, m_nImageWidth(0)
	, m_nPayLoadSize(0)
	, m_nTriggerMode(0)
	, m_hDevice(NULL)
//	, m_pBmpInfo(NULL)
	, m_pBufferRaw(NULL)
	, m_pImageBuffer(NULL)
//	, m_strFilePath("")
	//, m_pWnd(NULL)
	//, m_hDC(NULL)
	,isFirstFrame(TRUE)
	,isFisnshSecondFrame(FALSE)
	,file("cloudpoint.txt",CFile::modeCreate | CFile::modeWrite)   
	, m_dFrameRate(75)
{
	//{{AFX_DATA_INIT(CGxSingleCamMonoDlg)
	m_dShutterValue = 0.0;
	m_dGainValue = 0.0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// 初始化参数
	memset(&m_stGainFloatRange,0,sizeof(GX_FLOAT_RANGE));
	memset(&m_stShutterFloatRange,0,sizeof(GX_FLOAT_RANGE));
	memset(m_chBmpBuf,0,sizeof(m_chBmpBuf));
}

//析构函数
CGxSingleCamMonoDlg::~CGxSingleCamMonoDlg()
{
	file.Close();

}
void CGxSingleCamMonoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGxSingleCamMonoDlg)
	DDX_Text(pDX, IDC_EDIT_SHUTTER, m_dShutterValue);
	DDX_Text(pDX, IDC_EDIT_GAIN, m_dGainValue);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT_FrameRate, m_dFrameRate);
}

BEGIN_MESSAGE_MAP(CGxSingleCamMonoDlg, CDialog)
	//{{AFX_MSG_MAP(CGxSingleCamMonoDlg)
	ON_WM_QUERYDRAGICON()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BTN_SOFTTRIGGER, OnBnClickedBtnSofttrigger)
	ON_CBN_SELCHANGE(IDC_COMBO_TRIGGER_MODE, OnCbnSelchangeComboTriggerMode)
	ON_BN_CLICKED(IDC_BTN_OPEN_DEVICE, OnBnClickedBtnOpenDevice)
	ON_BN_CLICKED(IDC_BTN_CLOSE_DEVICE, OnBnClickedBtnCloseDevice)
	ON_BN_CLICKED(IDC_BTN_STOP_SNAP, OnBnClickedBtnStopSnap)
	ON_BN_CLICKED(IDC_CHECK_SAVE_IMAGE, OnBnClickedCheckSaveImage)
	ON_BN_CLICKED(IDC_BTN_START_SNAP, OnBnClickedBtnStartSnap)
	ON_CBN_SELCHANGE(IDC_COMBO_TRIGGER_ACTIVE, OnCbnSelchangeComboTriggerActive)
	ON_CBN_SELCHANGE(IDC_COMBO_TRIGGER_SOURCE, OnCbnSelchangeComboTriggerSource)
	ON_EN_KILLFOCUS(IDC_EDIT_SHUTTER, OnKillfocusEditShutter)
	ON_EN_KILLFOCUS(IDC_EDIT_GAIN, OnKillfocusEditGain)
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
	
	ON_EN_KILLFOCUS(IDC_EDIT_FrameRate, &CGxSingleCamMonoDlg::OnEnKillfocusEditFramerate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGxSingleCamMonoDlg message handlers

BOOL CGxSingleCamMonoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// 若初始化设备库失败则直接退出应用程序
	emStatus = GXInitLib();
	if (emStatus != GX_STATUS_SUCCESS)
	{
		ShowErrorString(emStatus);
		exit(0);
	}


	// 更新界面控件
	UpDateUI();

	LaserPlaneRefraction();    //

	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGxSingleCamMonoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		char chFileName[MAX_PATH] = {'\0'};
		GetModuleFileName(NULL, chFileName, MAX_PATH);
		CFileVersion fv(chFileName);
		CAboutDlg objDlgAbout;		
		objDlgAbout.m_strProductVersion = _T("Version: ") + fv.GetProductVersion();
		objDlgAbout.m_strLegalCopyRight = fv.GetLegalCopyright();
		objDlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGxSingleCamMonoDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGxSingleCamMonoDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

//---------------------------------------------------------------------------------
/**
\brief   关闭应用程序函数

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnClose() 
{
	// TODO: Add your message handler code here and/or call default

	GX_STATUS emStatus = GX_STATUS_SUCCESS;
    
	// 若未停采则先停止采集
    if (m_bIsSnap)
    {
		// 发送停止采集命令
		emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);

		// 注销回调
		emStatus = GXUnregisterCaptureCallback(m_hDevice);

		m_bIsSnap = FALSE;

		// 释放为采集图像开辟的图像Buffer
		UnPrepareForShowImg();
    }

	//释放pDC
	//::ReleaseDC(m_pWnd->m_hWnd, m_hDC);

	// 未关闭设备则关闭设备
	if (m_bDevOpened)
	{
		//关闭设备
		emStatus = GXCloseDevice(m_hDevice);
		m_bDevOpened = FALSE;
		m_hDevice    = NULL;
	}
	
	// 关闭设备库
	emStatus = GXCloseLib();

	CDialog::OnClose();
}

//---------------------------------------------------------------------------------
/**
\brief   初始化触发模式Combox控件

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitTriggerModeUI()
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// 判断设备是否支持触发模式
	emStatus = GXIsImplemented(m_hDevice, GX_ENUM_TRIGGER_MODE, &m_bTriggerMode);
	GX_VERIFY(emStatus);
	if (!m_bTriggerMode)
	{
		return;
	}

	// 设备触发模式默认为Off
	emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF);
	GX_VERIFY(emStatus);

	// 初始化触发模式Combox框
	InitEnumUI(GX_ENUM_TRIGGER_MODE, (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_MODE), m_bTriggerMode);
}

//---------------------------------------------------------------------------------
/**
\brief  初始化触发源Combox控件

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitTriggerSourceUI()
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// 判断设备是否支持触发源
	emStatus = GXIsImplemented(m_hDevice, GX_ENUM_TRIGGER_SOURCE, &m_bTriggerSource);
	GX_VERIFY(emStatus);
	if (!m_bTriggerSource)
	{
		return;
	}

	// 初始化触发源Combox框
	InitEnumUI(GX_ENUM_TRIGGER_SOURCE, (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_SOURCE),m_bTriggerSource);
}

//---------------------------------------------------------------------------------
/**
\brief   初始化触发极性Combox控件

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitTriggerActivationUI()
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// 判断设备是否支持触发极性
	emStatus = GXIsImplemented(m_hDevice, GX_ENUM_TRIGGER_ACTIVATION, &m_bTriggerActive);
	GX_VERIFY(emStatus);
	if (!m_bTriggerActive)
	{
		return;
	}

	// 初始化触发极性Combox框
	InitEnumUI(GX_ENUM_TRIGGER_ACTIVATION, (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_ACTIVE), m_bTriggerActive);
}

//---------------------------------------------------------------------------------
/**
\brief   打开设备后调用该函数:初始化UI界面控件的值

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitUI()
{
	GX_STATUS emStatus = GX_STATUS_ERROR;
	int  nValue = 0;
	
	// 触发模式下拉选项Combox控件
	InitTriggerModeUI();

	// 初始化触发源Combox控件
	InitTriggerSourceUI();

	// 初始化触发极性Combox控件
	InitTriggerActivationUI();

	// 初始化曝光时间相关控件
	InitShutterUI();

	// 初始化增益相关控件
	InitGainUI();

	//额外添加的代码
	

	UpdateData(false);
}

//---------------------------------------------------------------------------------
/**
\brief   初始化曝光相关控件: Static:显示范围 Edit:输入值

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitShutterUI()
{
	CStatic     *pStcShutterShow = (CStatic *)GetDlgItem(IDC_STC_SHUTTER_SHOW);
    CEdit       *pEditShutterVal = (CEdit *)GetDlgItem(IDC_EDIT_SHUTTER);
    
	// 判断控件的有效性
	if ((pEditShutterVal == NULL) || (pStcShutterShow == NULL))
	{
		return;
	}

    GX_STATUS      emStatus = GX_STATUS_ERROR;
	CString        strTemp  = "";
	double  dShutterValue   = 0.0;

	// 获取设备曝光时间的浮点型范围
	emStatus = GXGetFloatRange(m_hDevice, GX_FLOAT_EXPOSURE_TIME, &m_stShutterFloatRange);
	GX_VERIFY(emStatus);

	//在static中显示范围
    strTemp.Format("曝光(%.4f~%.4f)%s", m_stShutterFloatRange.dMin, m_stShutterFloatRange.dMax, m_stShutterFloatRange.szUnit);
	pStcShutterShow->SetWindowText(strTemp);

	// 获取当前值并更新到界面
	emStatus = GXGetFloat(m_hDevice, GX_FLOAT_EXPOSURE_TIME, &dShutterValue);
	GX_VERIFY(emStatus);

	m_dShutterValue = dShutterValue;

	UpdateData(FALSE);
}

//---------------------------------------------------------------------------------
/**
\brief   初始化增益相关控件: Static:显示范围 Edit:输入值
 
\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitGainUI()
{
	CStatic     *pStcGain     = (CStatic *)GetDlgItem(IDC_STC_GAIN);
	CEdit       *pEditGainVal = (CEdit *)GetDlgItem(IDC_EDIT_GAIN);

	// 判断控件的有效性
	if ((pStcGain == NULL) || (pEditGainVal == NULL))
	{
		return;
	}
    
	GX_STATUS emStatus = GX_STATUS_ERROR;
	CString   strRange = "";
	double    dGainVal = 0.0;

	// 获取设备增益的浮点型范围
	emStatus = GXGetFloatRange(m_hDevice, GX_FLOAT_GAIN, &m_stGainFloatRange);
	GX_VERIFY(emStatus);

	// 在static中显示范围
    strRange.Format("增益(%.4f~%.4f)%s", m_stGainFloatRange.dMin, m_stGainFloatRange.dMax, m_stGainFloatRange.szUnit);
	pStcGain->SetWindowText(strRange);
	
	// 获取当前值并更新到界面
	emStatus = GXGetFloat(m_hDevice, GX_FLOAT_GAIN, &dGainVal);
	GX_VERIFY(emStatus);

	m_dGainValue = dGainVal;
   
	UpdateData(FALSE);
}

//---------------------------------------------------------------------------------
/**
\brief   初始化组合框UI界面
\param   emFeatureID   [in]    功能码ID
\param   pComboBox     [in]    控件指针
\param   pIsImplement  [in]    是否支持emFeatureID代表的功能

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitEnumUI(GX_FEATURE_ID_CMD emFeatureID, CComboBox *pComboBox, bool bIsImplement)
{
	// 判断控件的有效性
	if ((pComboBox == NULL) || !bIsImplement)
	{
		return;
	}

	GX_ENUM_DESCRIPTION *pEnum       = NULL;
	GX_STATUS emStatus    = GX_STATUS_ERROR;
	size_t    nbufferSize = 0;
	uint32_t  nEntryNum   = 0;
	int64_t   nEnumValue  = -1;
	int       nCurcel     = 0;

	// 获取功能的枚举项个数
	emStatus = GXGetEnumEntryNums(m_hDevice, emFeatureID, &nEntryNum);
    GX_VERIFY(emStatus);

	// 为获取枚举型的功能描述分配空间
	nbufferSize = nEntryNum * sizeof(GX_ENUM_DESCRIPTION);
	pEnum       = new GX_ENUM_DESCRIPTION[nEntryNum];
	if (pEnum == NULL)
	{
		MessageBox("分配缓冲区失败！");
		return;
	}

	// 获取功能的枚举项的描述
	emStatus = GXGetEnumDescription(m_hDevice, emFeatureID, pEnum, &nbufferSize);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		if (pEnum != NULL)
		{  
			delete []pEnum;
			pEnum = NULL;
		}

		return;
	}

	// 获取枚举型的当前值,并设为界面当前显示值
	emStatus = GXGetEnum(m_hDevice, emFeatureID, &nEnumValue);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		if (pEnum != NULL)
		{  
			delete []pEnum;
			pEnum = NULL;
		}

		return;
	}

	// 初始化当前控件的可选项
	pComboBox->ResetContent();
	for (uint32_t i = 0; i < nEntryNum; i++)
	{	
		pComboBox->SetItemData(pComboBox->AddString(pEnum[i].szSymbolic), (uint32_t)pEnum[i].nValue);
		if (pEnum[i].nValue == nEnumValue)
		{
			nCurcel = i;
		}
	}

	// 设置当前值为界面的显示值
	pComboBox->SetCurSel(nCurcel);

	// 释放函数资源
	if (pEnum != NULL)
	{  
		delete []pEnum;
		pEnum = NULL;
    }
}


//---------------------------------------------------------------------------------
/**
\brief   采集图像回调函数
\param   pFrame      回调参数

\return  无
*/
//----------------------------------------------------------------------------------
void __stdcall CGxSingleCamMonoDlg::OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM* pFrame)
{
	CGxSingleCamMonoDlg *pDlg = (CGxSingleCamMonoDlg*)(pFrame->pUserParam);
	int nImageHeight = (int)pDlg->m_nImageHeight;
	int nImageWidth  = (int)pDlg->m_nImageWidth;

	if (pFrame->status == 0)
	{    
		critical_section.Lock();
        //////////////创建图像 //////////////(完成相邻两幅图像的保存)
		if (pDlg->isFirstFrame==TRUE)
		{
			memcpy(pDlg->m_pBufferRaw,pFrame->pImgBuf,pFrame->nImgSize);
			//采集到的图像存入IplImage图像变量中

			for( int j = 0; j < nImageHeight; j++ )
			{
				for ( int i = 0; i < nImageWidth; i++)   
				{		
					pDlg->curentImage->imageData[j*nImageWidth+i] =pDlg->m_pBufferRaw[j*nImageWidth + i]; 	
				}
			}
			pDlg->isFirstFrame=FALSE;
		}
		if (pDlg->isFirstFrame==FALSE)
		{
			memcpy(pDlg->m_pBufferRaw,pFrame->pImgBuf,pFrame->nImgSize);

			
			cvCopy(pDlg->curentImage,pDlg->lastImage,NULL); //图像赋值
			//采集到的图像存入IplImage图像变量currentImage中
			for( int j = 0; j < nImageHeight; j++ )
			{
				for ( int i = 0; i < nImageWidth; i++)   
				{		
					pDlg->curentImage->imageData[j*nImageWidth+i] =pDlg->m_pBufferRaw[j*nImageWidth + i]; 	
				}
			}
			
			pDlg->isFisnshSecondFrame=TRUE;
		}
		
		critical_section.Unlock();
	
	}
		
}


/******************************************
功能：激光平面折射校正
*******************************************/

void CGxSingleCamMonoDlg::LaserPlaneRefraction()
{
	// 1.将z=a*x+b*y+c的标定结果转换为A*x+B*y+C*z+D=0的形式
	A1 = a1 / sqrt(pow(a1, 2) + pow(b1, 2) + 1);
	B1 = b1 / sqrt(pow(a1, 2) + pow(b1, 2) + 1);
	C1 = -1 / sqrt(pow(a1, 2) + pow(b1, 2) + 1);
	D1 = c1 / sqrt(pow(a1, 2) + pow(b1, 2) + 1);

	// 2.第一次折射后的平面方程A*x + B*y + C*z + D = 0的形式
	float o1, m1, n1;
	o1 = b*c*C1 - c*c*B1;
	m1 = c*c*A1 - a*c*C1;
	n1 = a*c*B1 - b*c*A1;

	float L1, lamta1, theta1;
	L1 = (refractivityAir / refractivityGlass)*(a*A1 + b*B1 + c*C1);
	lamta1 = (L1*n1) / (b*n1 - c*m1);
	theta1 = (c*o1 - a*n1) / (b*n1 - c*m1);

	float pho1, fhi1;
	pho1 = (-L1*m1) / (b*n1 - c*m1);
	fhi1 = (a*m1 - b*o1) / (b*n1 - c*m1);

	float i1, j1, k1;
	i1 = 1 + pow(theta1, 2) + pow(fhi1, 2);
	j1 = 2 * lamta1*theta1 + 2 * pho1*fhi1;
	k1 = pow(lamta1, 2) + pow(pho1, 2) - 1;
	
	float A21, B21, C21, A22, B22, C22;
	A21 = (-j1 + sqrt( pow(j1, 2)-4*i1*k1) ) / (2*i1);
	B21 = lamta1 + theta1*A21;
	C21 = pho1 + fhi1*A21;

	A22 = (-j1 - sqrt(pow(j1, 2) - 4 * i1*k1)) / (2 * i1);
	B22 = lamta1 + theta1*A22;
	C22 = pho1 + fhi1*A22;

	if (A21*A1 + B21*B1 + C21*C1 >= A22*A1 + B22*B1 + C22*C1)
	{
		A2 = A21;
		B2 = B21;
		C2 = C21;
	}
	else
	{
		A2 = A22;
		B2 = B22;
		C2 = C22;
	}

	float tempk1;
	tempk1 = (C2*B1 - B2*C1) / (B2*c - C2*b);
	D2 = C2*(D1-tempk1*hc) / (C1 + tempk1*c);


	// 3.第二次折射后的平面方程A*x + B*y + C*z + D = 0的形式
	float o2, m2, n2;
	o2 = b*c*C2 - c*c*B2;
	m2 = c*c*A2 - a*c*C2;
	n2 = a*c*B2 - b*c*A2;

	float L2, lamta2, theta2;
	L2 = (refractivityGlass / refractivityWater)*(a*A2 + b*B2 + c*C2);
	lamta2 = (L2*n2) / (b*n2 - c*m2);
	theta2 = (c*o2 - a*n2) / (b*n2 - c*m2);

	float pho2, fhi2;
	pho2 = (-L2*m2) / (b*n2 - c*m2);
	fhi2 = (a*m2 - b*o2) / (b*n2 - c*m2);

	float i2, j2, k2;
	i2 = 1 + pow(theta2, 2) + pow(fhi2, 2);
	j2 = 2 * lamta2*theta2 + 2 * pho2*fhi2;
	k2 = pow(lamta2, 2) + pow(pho2, 2) - 1;

	float A31, B31, C31, A32, B32, C32;
	A31 = (-j2 + sqrt(pow(j2, 2) - 4 * i2*k2)) / (2 * i2);
	B31 = lamta2 + theta2*A31;
	C31 = pho2 + fhi2*A31;

	A32 = (-j2 - sqrt(pow(j2, 2) - 4 * i2*k2)) / (2 * i2);
	B32 = lamta2 + theta2*A32;
	C32 = pho2 + fhi2*A32;

	if (A31*A2 + B31*B2 + C21*C2 >= A32*A2 + B32*B2 + C32*C2)
	{
		A3 = A31;
		B3 = B31;
		C3 = C31;
	}
	else
	{
		A3 = A32;
		B3 = B32;
		C3 = C32;
	}

	float tempk2;
	tempk2 = (C3*B2 - B3*C2) / (B3*c - C3*b);
	D3 = C3*(D2 - tempk2*(hc + d)) / (C2 + tempk2*c);

}


/******************************************
功能：激光条纹图像处理的整个过程
*******************************************/
BOOL CGxSingleCamMonoDlg::ProcessLaserStripeImage(Mat processImage)
{

	/******************step1:确定激光条纹的感兴趣区域**************************/
	Mat imageLaserROI(2*ROIHeight,dstWidth,CV_8UC1,Scalar::all(0));
	BOOL roiFlag;
	roiFlag= ROIDetermination(processImage,imageLaserROI); //执行时间15ms
	if (roiFlag==FALSE)
	{
		return FALSE;
	} 

	///*******************step2:对感兴趣图像进行中值滤波**************************/
	medianBlur(imageLaserROI,imageLaserROI,3);    //执行时间5ms
	imwrite("imageLaserROI.bmp",imageLaserROI);
	
	///******************step3:在感兴趣区域内确定激光条纹中心线**************************/
	
	vector<Point2f> CenterLaserPoint;
	//方法一：利用梯度法计算每列的梯度
	//LaserCenterLineGradient(imageLaserROI,CenterLaserPoint);      //执行时间28ms
	//方法二：灰度重心法，计算每列的重心（效率高）二值化以后计算灰度重心
	LaserCenterLineCentroidBw(imageLaserROI,CenterLaserPoint);        //执行时间8ms
	//LaserCenterLineCentroid(imageLaserROI,CenterLaserPoint);


	//如果图像中没有得到激光条纹中心线，则直接返回false，不执行后续的计算
	if (CenterLaserPoint.size()<1)
	{
		DrawImg(dstImage, IDC_STATIC_SHOW_FRAME);
		return FALSE;
	}

	m_CenterLaserPoint = CenterLaserPoint;   //赋值给全局变量

	//考虑将灰度图像转换为彩色图像
	Mat imgRGB;
	cvtColor(dstImage, imgRGB, COLOR_GRAY2RGB);

	//绘制提取到的激光条纹中心线红色
	for (int i = 0; i <CenterLaserPoint.size() - 1; i++)
	{
		line(imgRGB, CenterLaserPoint[i], CenterLaserPoint[i + 1], Scalar(0, 0, 255), 2);
	}

	DrawImg(imgRGB, IDC_STATIC_SHOW_FRAME);  //绘制提取线和点的图像

	return TRUE;
	
}

/******************************************
功能：确定激光条纹的感兴趣区域（利用行投影法,加大搜索步长，提升执行效率）
说明：输入是滤波后的图像，输出为感兴趣区域的图像
*******************************************/

BOOL CGxSingleCamMonoDlg::ROIDetermination(Mat &dstImage,Mat &imageLaserROI)
{
	maxRow=dstHeight/8; //每次开始处理前都要赋初值，认为投影和最大的位置在行数的中间
	int laserProjectValue[dstHeight/4];
	memset(laserProjectValue, 0, sizeof(int)*dstHeight/4);
	int i,j;
	//每隔4行灰度值投影一次
	for (i=0;i<dstHeight;i=i+4)
	{
		laserProjectValue[i/4]=0;
		for (j=0;j<dstWidth;j=j+5)
		{ 
			laserProjectValue[i/4]=laserProjectValue[i/4]+dstImage.at<uchar>(i,j);

		}

	}
	//找到投影向量中投影值最大的位置
	int laserProjectMaxValue=laserProjectValue[0];
	for ( i=1;i<dstHeight/4;i++)
	{
		if (laserProjectValue[i]>laserProjectMaxValue)
		{
			maxRow=i;
			laserProjectMaxValue=laserProjectValue[i];
		}

	}
	//确定感兴趣区域
	maxRow=4*maxRow;  //由于是隔行计算的
   //只有满足这个条件，才有可能是正确的激光条纹
	if (maxRow-ROIHeight>0 && maxRow+ROIHeight<dstHeight)
	{
		imageLaserROI=dstImage(Range(maxRow-ROIHeight,maxRow+ROIHeight),Range(0,dstWidth)); 
		return TRUE;
	}

	return FALSE;

}
/******************************************
功能：利用梯度法计算激光条纹中心线(每隔一列计算一次梯度)(速度慢一些)
说明：在激光条纹上边缘下步的10个像素的领域内对下边缘进行搜索，
加快图像处理速度,激光条纹中心点存入到全局变量CenterLaserPoint中
*******************************************/
void CGxSingleCamMonoDlg::LaserCenterLineGradient(Mat imageLaserROI,vector<Point2f> &CenterLaserPoint)
{
	vector<Point2i> UpperLaserPoint;
	vector<Point2i> BottomLaserPoint;
	
	int maxdel, mindel;
	int dstROIWidth=imageLaserROI.cols;
	int dstROIHeight=imageLaserROI.rows;
	int i,j;
	for (j=0;j<dstROIWidth;j=j+2)
	{
		maxdel=0;
		Point2i point;
		for (i=3;i<dstROIHeight-2;i++)
		{
			
			if ( (imageLaserROI.at<uchar>(i,j)+imageLaserROI.at<uchar>(i+1,j)+imageLaserROI.at<uchar>(i+2,j))-(imageLaserROI.at<uchar>(i-1,j)+imageLaserROI.at<uchar>(i-2,j)+imageLaserROI.at<uchar>(i-3,j))>maxdel )
			{
				maxdel=(imageLaserROI.at<uchar>(i,j)+imageLaserROI.at<uchar>(i+1,j)+imageLaserROI.at<uchar>(i+2,j))-(imageLaserROI.at<uchar>(i-1,j)+imageLaserROI.at<uchar>(i-2,j)+imageLaserROI.at<uchar>(i-3,j));
				point.y=i;
				point.x=j;
			}
		
		}
		UpperLaserPoint.push_back(point);
	}
	//在激光条纹上边缘下步的10个像素的领域内对下边缘进行搜索，加快图像处理速度
	for (j=0;j<dstROIWidth;j=j+2)
	{
		mindel=0;
		Point2i point;
		for (i=UpperLaserPoint[j/2].y;i<UpperLaserPoint[j/2].y+10;i++)
		{
			if ( (imageLaserROI.at<uchar>(i,j)+imageLaserROI.at<uchar>(i+1,j)+imageLaserROI.at<uchar>(i+2,j))-(imageLaserROI.at<uchar>(i-1,j)+imageLaserROI.at<uchar>(i-2,j)+imageLaserROI.at<uchar>(i-3,j))<mindel )
			{
				mindel=(imageLaserROI.at<uchar>(i,j)+imageLaserROI.at<uchar>(i+1,j)+imageLaserROI.at<uchar>(i+2,j))-(imageLaserROI.at<uchar>(i-1,j)+imageLaserROI.at<uchar>(i-2,j)+imageLaserROI.at<uchar>(i-3,j));
				point.y=i;
				point.x=j;
			}
		}
		BottomLaserPoint.push_back(point);
	}

	/**************计算得到激光条纹中心线*****************/
	//感兴趣区域内的坐标值
	for (i=0;i<UpperLaserPoint.size();i++)
	{
		Point2f tempPoint;
		tempPoint.x=(UpperLaserPoint[i].x+BottomLaserPoint[i].x)/2;
		tempPoint.y=(UpperLaserPoint[i].y+BottomLaserPoint[i].y)/2;
		//转换为原图像坐标中的值
		if (maxRow-ROIHeight>0)
		{
			tempPoint.y=tempPoint.y+maxRow-ROIHeight;
		}
		CenterLaserPoint.push_back(tempPoint);
	}

}

/******************************************
功能：对二值化后的图像利用灰度重心法计算激光条纹中心线（速度快）
说明：激光条纹中心点存入到全局变量CenterLaserPoint中,
*******************************************/
void CGxSingleCamMonoDlg::LaserCenterLineCentroidBw(Mat imageLaserROI,vector<Point2f> &CenterLaserPoint)
{
	/******************对感兴趣区域进行自适应阈值化分割**********/
	Mat imageLaserROI_Bw;
	double m_threshold=getThreshVal_Otsu_8u(imageLaserROI);

	double thresholdGainTemp = 1.0;
	threshold(imageLaserROI, imageLaserROI_Bw, thresholdGainTemp*m_threshold, 255, 0);
	imwrite("imageLaserROI_Bw.bmp",imageLaserROI_Bw);

	/******************对阈值化分割后的图像利用灰度重心法计算激光条纹中心线**********/
	int dstROIWidth=imageLaserROI.cols;
	int dstROIHeight=imageLaserROI.rows;
	int i,j;
	double sumMolecule, sumDenominator;
	for (i=0;i<dstROIWidth;i++)
	{
		sumMolecule=0;
		sumDenominator=0;
		for (j=0;j<dstROIHeight;j++)
		{
			sumMolecule=sumMolecule+(j+1)*imageLaserROI_Bw.at<uchar>(j,i)/255;
			sumDenominator=sumDenominator+imageLaserROI_Bw.at<uchar>(j,i)/255;

		}
		//这一列阈值化分割之后没有亮值像素的话，则不进行处理
		if (sumDenominator!=0)
		{
			Point2f point;
			point.y=sumMolecule/sumDenominator;
			point.x=i;
			if (maxRow-ROIHeight>0)
			{
				point.y=point.y+maxRow-ROIHeight;
			}
			CenterLaserPoint.push_back(point);

		}
		
	}

	/*Mat midImage(dstHeight,dstWidth,CV_8UC1,Scalar::all(0));
	
	for (int i=0;i<CenterLaserPoint.size();i++)
	{
		midImage.at<uchar>(CenterLaserPoint[i].y,CenterLaserPoint[i].x)=255;

	}
	imwrite("midImage.bmp",midImage);
	DrawImg(midImage,IDC_STATIC_SHOW_FRAME);*/
}


/******************************************
功能：直接对原始图像利用灰度重心法计算激光条纹中心线
说明：激光条纹中心点存入到全局变量CenterLaserPoint中
*******************************************/
void CGxSingleCamMonoDlg::LaserCenterLineCentroid(Mat imageLaserROI,vector<Point2f> &CenterLaserPoint)
{
	/******************对图像利用灰度重心法计算激光条纹中心线**********/
	int dstROIWidth=imageLaserROI.cols;
	int dstROIHeight=imageLaserROI.rows;
	int i,j;
	double sumMolecule, sumDenominator;
	for (i=0;i<dstROIWidth;i++)
	{
		sumMolecule=0;
		sumDenominator=0;
		for (j=0;j<dstROIHeight;j++)
		{
			sumMolecule=sumMolecule+(j+1)*imageLaserROI.at<uchar>(j,i);
			sumDenominator=sumDenominator+imageLaserROI.at<uchar>(j,i);

		}
		//如果这一列灰度值之和为0的话，则不进行处理
		if (sumDenominator!=0)
		{
			Point2f point;
			point.y=sumMolecule/sumDenominator;
			point.x=i;
			if (maxRow-ROIHeight>0)
			{
				point.y=point.y+maxRow-ROIHeight;
			}
			CenterLaserPoint.push_back(point);
		}	
	}

}

/******************************************
功能：利用opencv库函数HoughLines完成直线提取
说明：对HoughLines提取的直线进行分析，找出感兴趣的直线
*******************************************/
bool CGxSingleCamMonoDlg::FindLinesHough(vector<Vec2f>&linesInterest,vector<Point2f> &CenterLaserPoint)
{
	/**********利用激光条纹上中心线上的点填充一副图像*******/
	Mat midImage(dstHeight,dstWidth,CV_8UC1,Scalar::all(0));
	int i,j;
	for (i=0;i<CenterLaserPoint.size();i++)
	{
		midImage.at<uchar>(CenterLaserPoint[i].y,CenterLaserPoint[i].x)=255;

	}
	imwrite("midImage.bmp",midImage);
	//DrawImg(midImage,IDC_STATIC_SHOW_FRAME);
	////霍夫变换函数，函数参数依次为binary,lines,deltaRho,deltaTheta,minVote
	vector<Vec2f> lines;     //第一个元素为pho，第二个元素为theta
	double deltaRho=1;
	double deltaTheta=CV_PI/180;
	int minVote=60;  
	HoughLines(midImage,lines,deltaRho,deltaTheta,minVote);

	//把提取出的直线分为三类(但是实际中由于干扰，可能提取不三类直线，只有提取到三类直线以后才进行后续计算)
	vector<Vec2f> linesOne;
	vector<Vec2f> linesTemp;
	vector<Vec2f> linesTwo;
	vector<Vec2f> linesThree;

	bool featureLinesFlag=false;
	if (lines.size()>=3)
	{
		linesOne.push_back(lines[0]);
		for (int i=1;i<lines.size();i++)
		{
			if (abs(lines[i][1]-linesOne[0][1])<=6*CV_PI/180)    //这个角度5度的值很关键，认为小于5度，即为同一类直线(最开始取值为3，很不稳定)
			{
				linesOne.push_back(lines[i]);
			}
			else
			{
				linesTemp.push_back(lines[i]);
			}

		}

		if (linesTemp.size()>=2)
		{
			linesTwo.push_back(linesTemp[0]);
			for (int i=1;i<linesTemp.size();i++)
			{
				if (abs(linesTemp[i][1]-linesTwo[0][1])<=6*CV_PI/180)
				{
					linesTwo.push_back(linesTemp[i]);
				}
				else
				{
					linesThree.push_back(linesTemp[i]);
				}

			}
			if (linesThree.size()>=1)
			{
				//把theta最大和最小的两条直线作为感兴趣直线
				//vector<Vec2f> linesInterest;

				float theta[3];
				theta[0]=linesOne[0][1];
				theta[1]=linesTwo[0][1];
				theta[2]=linesThree[0][1];
				int maxIndex=0;
				float maxTheta=theta[0];
				int minIndex=0;
				float minTheta=theta[0];
				for (int i=1;i<3;i++)
				{
					if (theta[i]>maxTheta)
					{
						maxTheta=theta[i];
						maxIndex=i;

					}

					if (theta[i]<minTheta)
					{
						minTheta=theta[i];
						minIndex=i;
					}
				}
				maxIndex=maxIndex+1;
				minIndex=minIndex+1;
				if (maxIndex!=1&&minIndex!=1)
				{
					linesInterest.push_back(linesTwo[0]);
					linesInterest.push_back(linesThree[0]);
				}
				if (maxIndex!=2&&minIndex!=2)
				{
					linesInterest.push_back(linesOne[0]);
					linesInterest.push_back(linesThree[0]);
				}
				if (maxIndex!=3&&minIndex!=3)
				{
					linesInterest.push_back(linesOne[0]);
					linesInterest.push_back(linesTwo[0]);
				}

				featureLinesFlag=true;
			}

	    }
	
	}

	return featureLinesFlag;

}

/******************************************
功能：对hough变化直线附近的点进行最小二乘拟合，并绘制出来
说明：
*******************************************/
void CGxSingleCamMonoDlg::GetInterestLine(vector<Vec2f> &linesInterest,vector<Point2f> &CenterLaserPoint)
{
	//对感兴趣直线附近的点进行最小二乘拟合
	double k,b;
	vector<Vec2f> linesInterestPointSlope;
	for (int i=0;i<linesInterest.size();i++)
	{
		if (linesInterest[i][1]!=0) //如果感兴趣直线不是竖直直线
		{
			k=-cos(linesInterest[i][1])/sin(linesInterest[i][1]);
			b=linesInterest[i][0]/sin(linesInterest[i][1]);
			//距离直线近的点加入拟合点集中
			vector<Point2f> fittingPoint;
			for (int j=0;j<CenterLaserPoint.size();j++)
			{
				if (abs(CenterLaserPoint[j].y-k*CenterLaserPoint[j].x-b)/sqrt(1+k*k)<=3)
				{
					Point2f point;
					point.x=CenterLaserPoint[j].x;
					point.y=CenterLaserPoint[j].y;
					fittingPoint.push_back(point);
				}
			}
			//进行最小二乘拟合
			Vec2f pointSlope=LineFitting(fittingPoint);
			linesInterestPointSlope.push_back(pointSlope);
			//在图像中画出拟合得到的直线
			int startx=1;
			int endx=dstWidth;
			int starty,endy;
			starty = int( pointSlope[0]*startx+pointSlope[1]  );
			endy=int(pointSlope[0]*endx+pointSlope[1]);
			Point start(startx,starty);
			Point end(endx,endy);
			line(dstImage,start,end,Scalar(175,0,0),2);
		}
	}
}

/******************************************
功能：根据给定点，进行最小二乘拟合
说明：输入为一组点，返回值为斜率和截距
*******************************************/

Vec2f CGxSingleCamMonoDlg::LineFitting(vector<Point2f> &fittingPoint)
{
	double sum1=0;
	double sum2=0;
	for (int i=0;i<fittingPoint.size();i++)
	{
		sum1=sum1+double(fittingPoint[i].x);
		sum2=sum2+double(fittingPoint[i].y);

	}
	double meanx,meany;
	meanx=sum1/(fittingPoint.size());
	meany=sum2/(fittingPoint.size());
	double sumxx=0;
	double sumxy=0;
	for (int i=0;i<fittingPoint.size();i++)
	{
		sumxx=sumxx+(double(fittingPoint[i].x)-meanx)*(double(fittingPoint[i].x)-meanx);
		sumxy=sumxy+(double(fittingPoint[i].x)-meanx)*(double(fittingPoint[i].y)-meany);

	}
	float k,b;
	if (sumxx != 0)
	{
		k= sumxy/sumxx;
		b= meany-k*meanx;
	}
	Vec2f pointSlope;
	pointSlope[0]=k;
	pointSlope[1]=b;
	return pointSlope;

}
/******************************************
功能：利用最大类间方差法得到图像二值化的阈值
说明：输入是mat类型（8位单通道）图像，输出为得到的阈值（0-255之间的数）
*******************************************/
double CGxSingleCamMonoDlg::getThreshVal_Otsu_8u(const Mat& _src)
{
	Size size = _src.size();
	if ( _src.isContinuous() )
	{
		size.width *= size.height;
		size.height = 1;
	}
	const int N = 256;
	int i, j, h[N] = {0};
	for ( i = 0; i < size.height; i++ )
	{
		const uchar* src = _src.data + _src.step*i;
		for ( j = 0; j <= size.width - 4; j += 4 )
		{
			int v0 = src[j], v1 = src[j+1];
			h[v0]++; h[v1]++;
			v0 = src[j+2]; v1 = src[j+3];
			h[v0]++; h[v1]++;
		}
		for ( ; j < size.width; j++ )
			h[src[j]]++;
	}

	double mu = 0, scale = 1./(size.width*size.height);
	for ( i = 0; i < N; i++ )
		mu += i*h[i];

	mu *= scale;
	double mu1 = 0, q1 = 0;
	double max_sigma = 0, max_val = 0;

	for ( i = 0; i < N; i++ )
	{
		double p_i, q2, mu2, sigma;

		p_i = h[i]*scale;
		mu1 *= q1;
		q1 += p_i;
		q2 = 1. - q1;

		if ( min(q1,q2) < FLT_EPSILON || max(q1,q2) > 1. - FLT_EPSILON )
			continue;

		mu1 = (mu1 + i*p_i)/q1;
		mu2 = (mu - q1*mu1)/q2;
		sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2);
		if ( sigma > max_sigma )
		{
			max_sigma = sigma;
			max_val = i;
		}
	}

	return max_val;
}


//---------------------------------------------------------------------------------
/**
\brief   将m_pImageBuffer中图像显示到界面

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::DrawImg(Mat image,UINT ID)
{
	//利用CvvImage类完成显示
	CDC *pDC = GetDlgItem(ID)->GetDC();     //根据ID获得窗口指针再获取与该窗口关联的上下文指针
	HDC hDC= pDC->GetSafeHdc();            // 获取设备上下文句柄
	CRect rect;
	GetDlgItem(ID)->GetClientRect(&rect);    //获取显示区

	IplImage* img=&IplImage(image);	//将图像转换为IplImage格式，共用同一个内存（浅拷贝）
	CvvImage cimg;
	cimg.CopyOf( img ); //复制图片
	cimg.DrawToHDC( hDC, &rect ); //将图片绘制到显示控件的指定区域内
	ReleaseDC( pDC );
}

//---------------------------------------------------------------------------------
/**
\brief   释放为图像显示准备的资源

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::UnPrepareForShowImg()
{
	if (m_pBufferRaw != NULL)
	{
		delete []m_pBufferRaw;
    	m_pBufferRaw = NULL;
	}
	
	if (m_pImageBuffer != NULL)
	{
		delete []m_pImageBuffer;
		m_pImageBuffer = NULL;
	}
	//想的是对IplImage指针变量Image进行内存释放,必须用cvReleaseImage，不能用delete
	if (image != NULL)
	{
		cvReleaseImage(&image);
	}
	if (lastImage != NULL)
	{
		cvReleaseImage(&lastImage);
	}
	if (curentImage != NULL)
	{
		cvReleaseImage(&curentImage);
	}
	if (lastImageProcess != NULL)
	{
		cvReleaseImage(&lastImageProcess);
	}
	if (curentImageProcess != NULL)
	{
		cvReleaseImage(&curentImageProcess);
	}




}

//---------------------------------------------------------------------------------
/**
\brief   点击"软触发采图"按钮响应函数

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnSofttrigger() 
{
	// TODO: Add your control notification handler code here
	//发送软触发命令(在触发模式为On且开始采集后有效)
	GX_STATUS emStatus = GX_STATUS_ERROR;
	emStatus = GXSendCommand(m_hDevice, GX_COMMAND_TRIGGER_SOFTWARE);

	GX_VERIFY(emStatus);
}

//---------------------------------------------------------------------------------
/**
\brief   切换"触发模式"选项响应函数

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnCbnSelchangeComboTriggerMode() 
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	bool bIsWritable = TRUE;
	int64_t  nCurrentEnumValue = 0;
	CComboBox *pBox    = (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_MODE);

	int      nIndex    = pBox->GetCurSel();                     // 获取当前选项
	int64_t  nEnumVal  = (int64_t)pBox->GetItemData(nIndex);    // 获取当前选项对应的枚举型值

	//判断触发模式枚举值是否可写
	emStatus = GXIsWritable(m_hDevice,GX_ENUM_TRIGGER_MODE,&bIsWritable);
	GX_VERIFY(emStatus);

	//获取当前触发模式枚举值
	emStatus = GXGetEnum(m_hDevice, GX_ENUM_TRIGGER_MODE, &nCurrentEnumValue);
	GX_VERIFY(emStatus);

	if (bIsWritable)
	{
		//设置当前选择的触发模式枚举值
		emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_MODE, nEnumVal);
		GX_VERIFY(emStatus);

		// 更新当前触发模式
		m_nTriggerMode = nEnumVal;
	}
	else
	{
		MessageBox("当前状态不可写，请停止采集后，请重新设置!");
		//判断当前选择的触发模式枚举值与选择之前是否相同，如果不同则进行切换，反之，不进行切换
		if (nCurrentEnumValue != nEnumVal)
		{
			if (GX_TRIGGER_MODE_ON == nEnumVal)
			{
				for (int i = 0;i < pBox->GetCount();i++)
				{
					if (GX_TRIGGER_MODE_OFF == pBox->GetItemData(i))
					{
						pBox->SetCurSel(i);
						break;
					}
				}
			}
			else
			{
				for (int i = 0;i < pBox->GetCount();i++)
				{
					if (GX_TRIGGER_MODE_ON == pBox->GetItemData(i))
					{
						pBox->SetCurSel(i);
						break;
					}
				}
			}
		}
		else
		{
			return;
		}
	}

	UpDateUI();
}

//----------------------------------------------------------------------------------
/**
\brief   将m_pImageBuffer图像数据保存成BMP图片

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::SaveImage()
{
	imwrite("Image.bmp",dstImage);
}

//----------------------------------------------------------------------------------
/**
\brief  获取GxIAPI错误描述,并弹出错误提示对话框
\param  emErrorStatus  [in]   错误码

\return 无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::ShowErrorString(GX_STATUS emErrorStatus)
{
	char*     pchErrorInfo = NULL;
	size_t    nSize        = 0;
	GX_STATUS emStatus     = GX_STATUS_ERROR;

	// 获取错误信息长度，并申请内存空间
	emStatus = GXGetLastError(&emErrorStatus, NULL, &nSize);
	pchErrorInfo = new char[nSize];
	if (NULL == pchErrorInfo)
	{
		return;
	}

	//获取错误信息描述，并显示
	emStatus = GXGetLastError (&emErrorStatus, pchErrorInfo, &nSize);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		MessageBox("GXGetLastError接口调用失败！");
	}
	else
	{
		MessageBox((LPCTSTR)pchErrorInfo);
	}

	// 释放申请的内存空间
	if (NULL != pchErrorInfo)
	{
		delete[] pchErrorInfo;
		pchErrorInfo = NULL;
	}
}

//----------------------------------------------------------------------------------
/**
\brief  刷新UI界面控件使能状态

\return 无返回值
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::UpDateUI()
{
	GetDlgItem(IDC_BTN_OPEN_DEVICE)->EnableWindow(!m_bDevOpened);
	GetDlgItem(IDC_BTN_CLOSE_DEVICE)->EnableWindow(m_bDevOpened);
	GetDlgItem(IDC_BTN_START_SNAP)->EnableWindow(m_bDevOpened && !m_bIsSnap);
	GetDlgItem(IDC_BTN_STOP_SNAP)->EnableWindow(m_bDevOpened && m_bIsSnap);
	GetDlgItem(IDC_BTN_SOFTTRIGGER)->EnableWindow(m_bDevOpened);

	GetDlgItem(IDC_EDIT_SHUTTER)->EnableWindow(m_bDevOpened);
	GetDlgItem(IDC_EDIT_GAIN)->EnableWindow(m_bDevOpened);
	GetDlgItem(IDC_EDIT_FrameRate)->EnableWindow(m_bDevOpened);

	GetDlgItem(IDC_COMBO_TRIGGER_ACTIVE)->EnableWindow(m_bDevOpened && m_bTriggerActive);
	GetDlgItem(IDC_COMBO_TRIGGER_MODE)->EnableWindow(m_bDevOpened &&  m_bTriggerMode);
	GetDlgItem(IDC_COMBO_TRIGGER_SOURCE)->EnableWindow(m_bDevOpened && m_bTriggerSource);
}

//----------------------------------------------------------------------------------
/**
\brief  点击"打开设备"控件响应函数

\return 无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnOpenDevice()
{
	// TODO: Add your control notification handler code here
	GX_STATUS     emStatus = GX_STATUS_SUCCESS;
	uint32_t      nDevNum  = 0;
	GX_OPEN_PARAM stOpenParam;

	// 初始化打开设备用参数,默认打开序号为1的设备
	stOpenParam.accessMode = GX_ACCESS_EXCLUSIVE;
	stOpenParam.openMode   = GX_OPEN_INDEX;
	stOpenParam.pszContent = "1";

	// 枚举设备
	emStatus = GXUpdateDeviceList(&nDevNum, 1000);
	GX_VERIFY(emStatus);

	// 判断当前连接设备个数
	if (nDevNum <= 0)
	{
		MessageBox("未发现设备!");
		return;
	}

	// 如果设备已经打开则关闭,保证相机在初始化出错情况能再次打开
	if (m_hDevice != NULL)
	{
		emStatus = GXCloseDevice(m_hDevice);
		GX_VERIFY(emStatus);
		m_hDevice = NULL;
	}

	// 打开设备
	emStatus = GXOpenDevice(&stOpenParam, &m_hDevice);
	GX_VERIFY(emStatus);
    m_bDevOpened = TRUE;

	// 设置相机的默认参数:采集模式:连续采集,数据格式:8-bit
	emStatus = InitDevice();
	GX_VERIFY(emStatus);

	// 获取设备的宽、高等属性信息
    emStatus = GetDeviceParam();
	GX_VERIFY(emStatus);

	// 获取相机参数,初始化界面控件
	InitUI();

	// 更新界面控件
	UpDateUI();

}

//----------------------------------------------------------------------------------
/**
\brief  点击"关闭设备"控件响应函数

\return 无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnCloseDevice()
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// 未停采时则停止采集
	if (m_bIsSnap)
	{
		//发送停止采集命令
		emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);
		if (emStatus != GX_STATUS_SUCCESS)
		{
			// 错误处理
		}

		//注销回调
		emStatus = GXUnregisterCaptureCallback(m_hDevice);
		if (emStatus != GX_STATUS_SUCCESS)
		{
			// 错误处理
		}

		m_bIsSnap = FALSE;
		// 释放为采集图像开辟的图像Buffer
		UnPrepareForShowImg();

		//关闭定时器
		KillTimer(1);
	}

	//关闭设备
	emStatus = GXCloseDevice(m_hDevice);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		// 错误处理
	}

	m_bDevOpened = FALSE;
	m_hDevice    = NULL;

	// 更新界面UI
	UpDateUI();


}

//----------------------------------------------------------------------------------
/**
\brief  点击"停止采集"控件响应函数

\return 无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnStopSnap()
{
	// TODO: Add your control notification handler code here
	//关闭定时器
	KillTimer(1);

	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	//发送停止采集命令
	emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);
	GX_VERIFY(emStatus);

	//注销回调
	emStatus = GXUnregisterCaptureCallback(m_hDevice);
	GX_VERIFY(emStatus);
   
	m_bIsSnap = FALSE;

	// 释放为采集图像开辟的图像Buffer
	UnPrepareForShowImg();

	// 更新界面UI
	UpDateUI();

	
}

// ---------------------------------------------------------------------------------
/**
\brief   初始化相机:设置默认参数

\return  无
*/
// ----------------------------------------------------------------------------------
GX_STATUS CGxSingleCamMonoDlg::InitDevice()
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	//设置采集模式连续采集
	emStatus = GXSetEnum(m_hDevice,GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
	VERIFY_STATUS_RET(emStatus);

	// 已知当前相机支持哪个8位图像数据格式可以直接设置
	// 例如设备支持数据格式GX_PIXEL_FORMAT_BAYER_GR8,则可按照以下代码实现
	// emStatus = GXSetEnum(m_hDevice, GX_ENUM_PIXEL_FORMAT, GX_PIXEL_FORMAT_BAYER_GR8);
	// VERIFY_STATUS_RET(emStatus);

	// 不清楚当前相机的数据格式时，可以调用以下函数将图像数据格式设置为8Bit
	emStatus = SetPixelFormat8bit(); 
	VERIFY_STATUS_RET(emStatus);

	//额外添加的代码
	//设置相机的采集帧率
	//emStatus = GXSetEnum(m_hDevice, GX_ENUM_ACQUISITION_FRAME_RATE_MODE, 0 );
	//VERIFY_STATUS_RET(emStatus);
	
	/*emStatus = GXSetFloat(m_hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, 75  );
	VERIFY_STATUS_RET(emStatus);*/

	return emStatus;
}

// ---------------------------------------------------------------------------------
/**
\brief   设置相机的数据格式为8bit

\return  emStatus GX_STATUS_SUCCESS:设置成功，其它状态码:设置失败
*/
// ----------------------------------------------------------------------------------
GX_STATUS CGxSingleCamMonoDlg::SetPixelFormat8bit()
{
	GX_STATUS emStatus    = GX_STATUS_SUCCESS;
	int64_t   nPixelSize  = 0;
	uint32_t  nEnmuEntry  = 0;
	size_t    nBufferSize = 0;
	BOOL      bIs8bit     = TRUE;

	GX_ENUM_DESCRIPTION  *pEnumDescription = NULL;
	GX_ENUM_DESCRIPTION  *pEnumTemp        = NULL;

	// 获取像素位深大小
	emStatus = GXGetEnum(m_hDevice, GX_ENUM_PIXEL_SIZE, &nPixelSize);
	VERIFY_STATUS_RET(emStatus);

	// 判断为8bit时直接返回,否则设置为8bit
	if (nPixelSize == GX_PIXEL_SIZE_BPP8)
	{
		return GX_STATUS_SUCCESS;
	}
	else
	{
		// 获取设备支持的像素格式的枚举项个数
		emStatus = GXGetEnumEntryNums(m_hDevice, GX_ENUM_PIXEL_FORMAT, &nEnmuEntry);
		VERIFY_STATUS_RET(emStatus);

		// 为获取设备支持的像素格式枚举值准备资源
		nBufferSize      = nEnmuEntry * sizeof(GX_ENUM_DESCRIPTION);
		pEnumDescription = new GX_ENUM_DESCRIPTION[nEnmuEntry];

		// 获取支持的枚举值
		emStatus         = GXGetEnumDescription(m_hDevice, GX_ENUM_PIXEL_FORMAT, pEnumDescription, &nBufferSize);
		if (emStatus != GX_STATUS_SUCCESS)
		{
			if (pEnumDescription == NULL)
			{
				delete []pEnumDescription;
				pEnumDescription = NULL;
			}
			return emStatus;
		}

		// 遍历设备支持的像素格式,设置像素格式为8bit,
		// 如设备支持的像素格式为Mono10和Mono8则设置其为Mono8
		for (uint32_t i = 0; i<nEnmuEntry; i++)
		{
			if ((pEnumDescription[i].nValue & GX_PIXEL_8BIT) == GX_PIXEL_8BIT)
			{
				emStatus = GXSetEnum(m_hDevice, GX_ENUM_PIXEL_FORMAT, pEnumDescription[i].nValue);
				break;
			}
		}	

		// 释放资源
		if (pEnumDescription != NULL)
		{
			delete []pEnumDescription;
			pEnumDescription = NULL;
		}
	}

	return emStatus;
}

//----------------------------------------------------------------------------------
/**
\brief  获取设备的宽高等属性信息

\return GX_STATUS_SUCCESS:全部获取成功，其它状态码:未成功获取全部
*/
//----------------------------------------------------------------------------------
GX_STATUS CGxSingleCamMonoDlg::GetDeviceParam()
{
	GX_STATUS emStatus       = GX_STATUS_ERROR;
	bool      bIsImplemented = false;

	// 获取图像大小
	emStatus = GXGetInt(m_hDevice, GX_INT_PAYLOAD_SIZE, &m_nPayLoadSize);
	VERIFY_STATUS_RET(emStatus);

	// 获取宽度
	emStatus = GXGetInt(m_hDevice, GX_INT_WIDTH, &m_nImageWidth);
	VERIFY_STATUS_RET(emStatus);

	// 获取高度
	emStatus = GXGetInt(m_hDevice, GX_INT_HEIGHT, &m_nImageHeight);      
	
	//获取相机采集帧率
	/*emStatus = GXGetFloat(m_hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, &m_nAcquisitionFrameRate  );
	CString stringAcquisitionFrame;
	stringAcquisitionFrame.Format("%lf",m_nAcquisitionFrameRate);
	m_ListWords.AddString(stringAcquisitionFrame);
*/
	return emStatus;
}

//----------------------------------------------------------------------------------
/**
\brief  为保存图像分配Buffer,为图像显示准备资源

\return true;为图像显示准备资源成功  false:准备资源失败
*/
//----------------------------------------------------------------------------------
bool  CGxSingleCamMonoDlg::PrepareForShowImg()
{	
	
	//--------------------------------------------------------------------------
	//------------------------图像数据Buffer分配---------------------------------
	//为原始图像数据分配空间
	m_pBufferRaw = new BYTE[(size_t)m_nPayLoadSize];
	if (m_pBufferRaw == NULL)
	{
		return false;
	}

	//为经过翻转后的图像数据分配空间
	m_pImageBuffer = new BYTE[(size_t)(m_nImageWidth * m_nImageHeight)];
	if (m_pImageBuffer == NULL)
	{
		delete []m_pBufferRaw;
		m_pBufferRaw = NULL;

		return false;
	}

	return true;
}

//----------------------------------------------------------------------------------
/**
\brief  点击"保存BMP图像"CheckBox框消息响应函数

\return 无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedCheckSaveImage()
{
	// TODO: Add your control notification handler code here
	m_bIsSaveImg = !m_bIsSaveImg;
}

//----------------------------------------------------------------------------------
/**
\brief  点击"开始采集"控件响应函数

\return 无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnStartSnap()
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_ERROR;
	

	//  创建图像（点击开始采集按钮，创建一次图像，停止采集的时候，需要释放内存）
	image = cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);
	curentImage=cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);
	lastImage=cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);
	lastImageProcess=cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);
	curentImageProcess=cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);

	//下面程序为自带的
	

	// 初始化BMP头信息、分配Buffer为图像采集做准备
	if (!PrepareForShowImg())
	{
		MessageBox("为图像显示准备资源失败!");
        return;
	}
	
	//注册回调
	emStatus = GXRegisterCaptureCallback(m_hDevice, this, OnFrameCallbackFun);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		UnPrepareForShowImg();
		ShowErrorString(emStatus);
		return;
	}

	//发开始采集命令
	emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_START);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		UnPrepareForShowImg();
		ShowErrorString(emStatus);
		return;
	}

	m_bIsSnap = TRUE;

	//// 更新界面UI
	UpDateUI();


}

//---------------------------------------------------------------------------------
/**
\brief   切换"触发极性"选项响应函数

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnCbnSelchangeComboTriggerActive()
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	CComboBox *pBox    = (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_ACTIVE);

	int      nIndex    = pBox->GetCurSel();                   // 获取当前选项
	int64_t  nEnumVal  = (int64_t)pBox->GetItemData(nIndex);  // 获取当前选项对应的枚举型值
    
	//设置当前选择的触发极性的值
	emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_ACTIVATION, nEnumVal);
    GX_VERIFY(emStatus);
}

//---------------------------------------------------------------------------------
/**
\brief   切换"触发源"选项响应函数

\return  无
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnCbnSelchangeComboTriggerSource()
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	CComboBox *pBox    = (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_SOURCE);

	int      nIndex    = pBox->GetCurSel();                   // 获取当前选项
	int64_t  nEnumVal  = (int64_t)pBox->GetItemData(nIndex);  // 获取当前选项对应的枚举型值

	//设置当前选择的触发源的值
	emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_SOURCE, nEnumVal);
	GX_VERIFY(emStatus);
}

//-----------------------------------------------------------------------
/**
\brief    控制曝光时间的Edit框失去焦点的响应函数

\return   无
*/
//-----------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnKillfocusEditShutter() 
{
	// TODO: Add your control notification handler code here
	//判断句柄是否有效，避免设备掉线后关闭程序出现的异常
	if (m_hDevice == NULL)
	{
		return;
	}

	UpdateData();
	GX_STATUS status = GX_STATUS_SUCCESS;

	//判断输入值是否在曝光时间的范围内
	//若大于最大值则将曝光值设为最大值
	if (m_dShutterValue > m_stShutterFloatRange.dMax)
	{
		m_dShutterValue = m_stShutterFloatRange.dMax;
	}
	//若小于最小值将曝光值设为最小值
	if (m_dShutterValue < m_stShutterFloatRange.dMin)
	{
		m_dShutterValue = m_stShutterFloatRange.dMin;
	}

	//设置曝光时间
	status = GXSetFloat(m_hDevice,GX_FLOAT_EXPOSURE_TIME,m_dShutterValue);
	GX_VERIFY(status);

	UpdateData(FALSE);
}

//-----------------------------------------------------------------------
/**
\brief   控制增益的Edit框失去焦点的响应函数

\return  无
*/
//-----------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnKillfocusEditGain() 
{
	// TODO: Add your control notification handler code here
	//判断句柄是否有效，避免设备掉线后关闭程序出现的异常
	if (m_hDevice == NULL)
	{
		return;
	}
	
	UpdateData();
	GX_STATUS status = GX_STATUS_SUCCESS;

	//判断输入值是否在增益值的范围内
	//若输入的值大于最大值则将增益值设置成最大值
	if (m_dGainValue > m_stGainFloatRange.dMax)
	{
		m_dGainValue = m_stGainFloatRange.dMax;
	}
	
	//若输入的值小于最小值则将增益的值设置成最小值
	if (m_dGainValue < m_stGainFloatRange.dMin)
	{
		m_dGainValue = m_stGainFloatRange.dMin;
	}
	
	status = GXSetFloat(m_hDevice, GX_FLOAT_GAIN, m_dGainValue);
	GX_VERIFY(status);

	UpdateData(FALSE);
}

void CGxSingleCamMonoDlg::OnEnKillfocusEditFramerate()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_hDevice == NULL)
	{
		return;
	}

	UpdateData();
	GX_STATUS emStatus = GX_STATUS_ERROR;

	//判断输入值是否在帧率值的范围内
	//若输入的值大于最大值则将帧率值设置成最大值
	if (m_dFrameRate> 75)
	{
		m_dFrameRate = 75;
	}

	//若输入的值小于最小值则将帧率的值设置成最小值
	if (m_dFrameRate < 1)
	{
		m_dFrameRate = 1;
	}
	emStatus = GXSetFloat(m_hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, m_dFrameRate  );
	GX_VERIFY(emStatus);
	UpdateData(FALSE);
}


//-----------------------------------------------------------------------
/**
\brief   分发函数主要处理曝光和增益的Edit框响应回车键的消息
\param   pMsg  消息结构体

\return 成功:TRUE   失败:FALSE
*/
//-----------------------------------------------------------------------
BOOL CGxSingleCamMonoDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	CWnd *pWnd = NULL;
	int   nID = 0;          //< 保存获取的控件ID
	GX_STATUS status = GX_STATUS_SUCCESS;

	//判断是否是键盘回车消息
	if (WM_KEYDOWN == pMsg->message && VK_RETURN == pMsg->wParam)
	{
		//获取当前拥有输入焦点的窗口(控件)指针
		pWnd = GetFocus();

		//获取焦点所处位置的控件ID
		nID = pWnd->GetDlgCtrlID();

		//判断控件ID类型
		switch (nID)
		{

		//如果是曝光EDIT框的ID，设置曝光时间
		case IDC_EDIT_SHUTTER:

        //如果是增益EDIT框的ID，设置增益值
		case IDC_EDIT_GAIN:

		case IDC_EDIT_FrameRate:
			
			//失去焦点
			SetFocus();

			break;

		default:
			break;
		}

		return TRUE;
	}

	if ((WM_KEYDOWN == pMsg->message) && (VK_ESCAPE == pMsg->wParam))
	{
		return TRUE;
	}

	if(pMsg->message==30000)
	{
		switch(pMsg->lParam)
		{
		case FD_READ:
		
			break;
		case FD_CLOSE:
			
			break;
		}
	}


	return CDialog::PreTranslateMessage(pMsg);
}

/******************************************
功能：在定时器处理函数内完成图像处理
说明：避免回调函数阻滞（现在回调函数中只是完成图像采集，存入BYTE型变量m_pBufferRaw中）
*******************************************/
void CGxSingleCamMonoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CString snFrame, snFrame1, snFrameString;


	switch(nIDEvent)
	{
	case 1:
		{
			nFrame++;

			////图像赋值的时候进行上锁
			critical_section.Lock();
			cvCopy(curentImage, image, NULL); //图像赋值
			critical_section.Unlock();
			//dstImage是Mat类型数据,(将IplImage类的image转换为Mat类的dstImage)
			dstImage= cvarrToMat(image);   
			
			CString sXWater, sYWater, sZWater, sXAir, sYAir, sZAir,sPixelX, sPixelY;
			BOOL isProcessed;
			CString strWater,strAir, str,strFrame;

			//////////////图像处理过程///////////////////
	
			isProcessed = ProcessLaserStripeImage(dstImage);

			if (isProcessed == TRUE)
			{
			
				snFrame.Format("%d",nFrame);
				strFrame=snFrame+":is accurately proessed\r\n";
				file.Write(strFrame, strFrame.GetLength()*sizeof(char));


				int nNumberLaserPoint = m_CenterLaserPoint.size();

				for (int i = 0; i < nNumberLaserPoint; i++)
				{
					Vec3f p1;
					p1[0] = (m_CenterLaserPoint[i].x - u0) / kx;
					p1[1] = (m_CenterLaserPoint[i].y - v0) / ky;
					p1[2] = 1;

					Vec3f p01_dir;   //第一段的方向向量
					//float norm_p1 = sqrt( pow(p1[0], 2) + pow(p1[1], 2) + pow(p1[2], 2) );
					//p01_dir[0] = p1[0] / norm_p1;
					//p01_dir[1] = p1[1] / norm_p1;
					//p01_dir[2] = p1[2] / norm_p1;

					float norm_p1 = norm(p1);
					p01_dir = p1 / norm(p1);

					//////1.在玻璃上表面进行折射/////////
					float alpha1 = refractivityAir / refractivityGlass;      //计算alpha1
					Vec3f normalVectorGlass = {a,b,c};
					float p01NormalDot = p01_dir[0] * normalVectorGlass[0] + p01_dir[1] * normalVectorGlass[1] + p01_dir[2] * normalVectorGlass[2];
					float beta1 = sqrt(1 - pow(alpha1, 2)*(1 - pow(p01NormalDot, 2))) - alpha1*p01NormalDot;
					
					Vec3f p12;
					Vec3f p12_dir;   //第一段的方向向量
					p12[0] = alpha1*p01_dir[0] + beta1*normalVectorGlass[0];
					p12[1] = alpha1*p01_dir[1] + beta1*normalVectorGlass[1];
					p12[2] = alpha1*p01_dir[2] + beta1*normalVectorGlass[2];

					//float norm_p2 = sqrt(pow(p12[0], 2) + pow(p12[1], 2) + pow(p12[2], 2));
					//p12_dir[0] = p12[0] / norm_p2;
					//p12_dir[1] = p12[1] / norm_p2;
					//p12_dir[2] = p12[2] / norm_p2;

					float norm_p2 = norm(p12);
					p12_dir = p12 / norm_p2;

					//根据玻璃上表面折射后的向量和玻璃厚度，计算光路达到玻璃下表面的点坐标

					float p12NormalDot = (p12_dir[0] * normalVectorGlass[0] + p12_dir[1] * normalVectorGlass[1] + p12_dir[2] * normalVectorGlass[2]);
					Vec3f p2;
					p2[0] = p1[0] + d*p12_dir[0] / p12NormalDot;
					p2[1] = p1[1] + d*p12_dir[1] / p12NormalDot;
					p2[2] = p1[2] + d*p12_dir[2] / p12NormalDot;
				    
					//////2.在玻璃下表面进行折射/////////
					float alpha2 = refractivityGlass / refractivityWater;      //计算alpha2
					float beta2 = sqrt(1 - pow(alpha2, 2)*(1 - pow(p12NormalDot, 2))) - alpha2*p12NormalDot;

					Vec3f p23;
					Vec3f p23_dir;   //第二段的方向向量
					p23[0] = alpha2*p12_dir[0] + beta2*normalVectorGlass[0];
					p23[1] = alpha2*p12_dir[1] + beta2*normalVectorGlass[1];
					p23[2] = alpha2*p12_dir[2] + beta2*normalVectorGlass[2];

					//float norm_p3 = sqrt(pow(p23[0], 2) + pow(p23[1], 2) + pow(p23[2], 2));
					//p23_dir[0] = p23[0] / norm_p3;
					//p23_dir[1] = p23[1] / norm_p3;
					//p23_dir[2] = p23[2] / norm_p3;

					float norm_p3 = norm(p23);
					p23_dir = p23 / norm_p3;

					//////3.计算特征点在水中和空气中的三维坐标/////////
					centerLaserPointUnderwater_mm.x = -(p23_dir[0])*(A3*p2[0] + B3*p2[1] + C3*p2[2] + D3) / (A3*p23_dir[0] + B3*p23_dir[1] + C3*p23_dir[2]) + p2[0];
					centerLaserPointUnderwater_mm.y = -(p23_dir[1])*(A3*p2[0] + B3*p2[1] + C3*p2[2] + D3) / (A3*p23_dir[0] + B3*p23_dir[1] + C3*p23_dir[2]) + p2[1];
					centerLaserPointUnderwater_mm.z = -(p23_dir[2])*(A3*p2[0] + B3*p2[1] + C3*p2[2] + D3) / (A3*p23_dir[0] + B3*p23_dir[1] + C3*p23_dir[2]) + p2[2];

					centerLaserPointAir_mm.z = -D1*kx*ky / (C1*kx*ky + A1*ky*(m_CenterLaserPoint[i].x - u0) + B1*kx*(m_CenterLaserPoint[i].y - v0));
					centerLaserPointAir_mm.x = centerLaserPointAir_mm.z*(m_CenterLaserPoint[i].x - u0) / kx;
					centerLaserPointAir_mm.y = centerLaserPointAir_mm.z*(m_CenterLaserPoint[i].y - v0) / ky;

					//////4.将特征点的三维坐标保存到文本中/////////
					sXWater.Format("%f", centerLaserPointUnderwater_mm.x);
					sYWater.Format("%f", centerLaserPointUnderwater_mm.y);
					sZWater.Format("%f", centerLaserPointUnderwater_mm.z);
					strWater = sXWater + "," + sYWater + "," + sZWater + ";";

					sXAir.Format("%f", centerLaserPointAir_mm.x);
					sYAir.Format("%f", centerLaserPointAir_mm.y);
					sZAir.Format("%f", centerLaserPointAir_mm.z);
					strAir = sXAir + "," + sYAir + "," + sZAir + "\r\n";
					str= strWater + strAir;
					file.Write(str, str.GetLength()*sizeof(char));

				}

			}

			/************图像保存处理************/
			if (m_bIsSaveImg)
			{
				snFrame1.Format("%d_1.bmp",nFrame);
				snFrameString="./CaptureImage\\"+snFrame1;
				cvSaveImage(snFrameString,image);
	
			}  

			
		}

		break;


	default:
		break;
	}

	CDialog::OnTimer(nIDEvent);
}






