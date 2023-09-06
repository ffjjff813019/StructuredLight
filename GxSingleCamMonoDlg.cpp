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
//���ͼ�񳣱����Ķ���
const int dstWidth=1280;
const int dstHeight=1024;
const double PI=3.1416;
const int ROIHeight=30;
//������ڲ���
const float kx = 1737.5658;
const float ky = 1737.4270;
const float u0 = 646.7194;
const float v0 = 506.2949;

//�����м���ƽ�����
const float a1 = 0.0116;
const float b1 = 4.1499;
const float c1 = 165.6944;

//ϵͳ����
const float refractivityAir = 1;
const float refractivityGlass = 1.52;
const float refractivityWater = 1.333;
const float hc = 27.20;  //��������ĵ������ϱ������
const float d = 3;       //�������

//����ƽ�淨����
const float a = -0.0034;
const float b = -0.0116;
const float c = 0.9999;

//����ƽ�����
float A1, B1, C1, D1;
float A2, B2, C2, D2;
float A3, B3, C3, D3;

//ȫ�ֱ����Ķ���
//vector<Point2i> CenterLaserPoint;
int LaserIndex[dstWidth]={0};
int maxRow;

static int nFrame=0;
int nFrametrue;
//ʵ���߳�ͬ�����ٽ���ȫ�ֱ���
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
//��׼���캯��
CGxSingleCamMonoDlg::CGxSingleCamMonoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGxSingleCamMonoDlg::IDD, pParent)
	, m_bIsSaveImg(FALSE)              //������ʼ��������ݳ�Ա���г�ʼ��
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

	// ��ʼ������
	memset(&m_stGainFloatRange,0,sizeof(GX_FLOAT_RANGE));
	memset(&m_stShutterFloatRange,0,sizeof(GX_FLOAT_RANGE));
	memset(m_chBmpBuf,0,sizeof(m_chBmpBuf));
}

//��������
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

	// ����ʼ���豸��ʧ����ֱ���˳�Ӧ�ó���
	emStatus = GXInitLib();
	if (emStatus != GX_STATUS_SUCCESS)
	{
		ShowErrorString(emStatus);
		exit(0);
	}


	// ���½���ؼ�
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
\brief   �ر�Ӧ�ó�����

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnClose() 
{
	// TODO: Add your message handler code here and/or call default

	GX_STATUS emStatus = GX_STATUS_SUCCESS;
    
	// ��δͣ������ֹͣ�ɼ�
    if (m_bIsSnap)
    {
		// ����ֹͣ�ɼ�����
		emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);

		// ע���ص�
		emStatus = GXUnregisterCaptureCallback(m_hDevice);

		m_bIsSnap = FALSE;

		// �ͷ�Ϊ�ɼ�ͼ�񿪱ٵ�ͼ��Buffer
		UnPrepareForShowImg();
    }

	//�ͷ�pDC
	//::ReleaseDC(m_pWnd->m_hWnd, m_hDC);

	// δ�ر��豸��ر��豸
	if (m_bDevOpened)
	{
		//�ر��豸
		emStatus = GXCloseDevice(m_hDevice);
		m_bDevOpened = FALSE;
		m_hDevice    = NULL;
	}
	
	// �ر��豸��
	emStatus = GXCloseLib();

	CDialog::OnClose();
}

//---------------------------------------------------------------------------------
/**
\brief   ��ʼ������ģʽCombox�ؼ�

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitTriggerModeUI()
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// �ж��豸�Ƿ�֧�ִ���ģʽ
	emStatus = GXIsImplemented(m_hDevice, GX_ENUM_TRIGGER_MODE, &m_bTriggerMode);
	GX_VERIFY(emStatus);
	if (!m_bTriggerMode)
	{
		return;
	}

	// �豸����ģʽĬ��ΪOff
	emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF);
	GX_VERIFY(emStatus);

	// ��ʼ������ģʽCombox��
	InitEnumUI(GX_ENUM_TRIGGER_MODE, (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_MODE), m_bTriggerMode);
}

//---------------------------------------------------------------------------------
/**
\brief  ��ʼ������ԴCombox�ؼ�

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitTriggerSourceUI()
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// �ж��豸�Ƿ�֧�ִ���Դ
	emStatus = GXIsImplemented(m_hDevice, GX_ENUM_TRIGGER_SOURCE, &m_bTriggerSource);
	GX_VERIFY(emStatus);
	if (!m_bTriggerSource)
	{
		return;
	}

	// ��ʼ������ԴCombox��
	InitEnumUI(GX_ENUM_TRIGGER_SOURCE, (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_SOURCE),m_bTriggerSource);
}

//---------------------------------------------------------------------------------
/**
\brief   ��ʼ����������Combox�ؼ�

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitTriggerActivationUI()
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// �ж��豸�Ƿ�֧�ִ�������
	emStatus = GXIsImplemented(m_hDevice, GX_ENUM_TRIGGER_ACTIVATION, &m_bTriggerActive);
	GX_VERIFY(emStatus);
	if (!m_bTriggerActive)
	{
		return;
	}

	// ��ʼ����������Combox��
	InitEnumUI(GX_ENUM_TRIGGER_ACTIVATION, (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_ACTIVE), m_bTriggerActive);
}

//---------------------------------------------------------------------------------
/**
\brief   ���豸����øú���:��ʼ��UI����ؼ���ֵ

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitUI()
{
	GX_STATUS emStatus = GX_STATUS_ERROR;
	int  nValue = 0;
	
	// ����ģʽ����ѡ��Combox�ؼ�
	InitTriggerModeUI();

	// ��ʼ������ԴCombox�ؼ�
	InitTriggerSourceUI();

	// ��ʼ����������Combox�ؼ�
	InitTriggerActivationUI();

	// ��ʼ���ع�ʱ����ؿؼ�
	InitShutterUI();

	// ��ʼ��������ؿؼ�
	InitGainUI();

	//������ӵĴ���
	

	UpdateData(false);
}

//---------------------------------------------------------------------------------
/**
\brief   ��ʼ���ع���ؿؼ�: Static:��ʾ��Χ Edit:����ֵ

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitShutterUI()
{
	CStatic     *pStcShutterShow = (CStatic *)GetDlgItem(IDC_STC_SHUTTER_SHOW);
    CEdit       *pEditShutterVal = (CEdit *)GetDlgItem(IDC_EDIT_SHUTTER);
    
	// �жϿؼ�����Ч��
	if ((pEditShutterVal == NULL) || (pStcShutterShow == NULL))
	{
		return;
	}

    GX_STATUS      emStatus = GX_STATUS_ERROR;
	CString        strTemp  = "";
	double  dShutterValue   = 0.0;

	// ��ȡ�豸�ع�ʱ��ĸ����ͷ�Χ
	emStatus = GXGetFloatRange(m_hDevice, GX_FLOAT_EXPOSURE_TIME, &m_stShutterFloatRange);
	GX_VERIFY(emStatus);

	//��static����ʾ��Χ
    strTemp.Format("�ع�(%.4f~%.4f)%s", m_stShutterFloatRange.dMin, m_stShutterFloatRange.dMax, m_stShutterFloatRange.szUnit);
	pStcShutterShow->SetWindowText(strTemp);

	// ��ȡ��ǰֵ�����µ�����
	emStatus = GXGetFloat(m_hDevice, GX_FLOAT_EXPOSURE_TIME, &dShutterValue);
	GX_VERIFY(emStatus);

	m_dShutterValue = dShutterValue;

	UpdateData(FALSE);
}

//---------------------------------------------------------------------------------
/**
\brief   ��ʼ��������ؿؼ�: Static:��ʾ��Χ Edit:����ֵ
 
\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitGainUI()
{
	CStatic     *pStcGain     = (CStatic *)GetDlgItem(IDC_STC_GAIN);
	CEdit       *pEditGainVal = (CEdit *)GetDlgItem(IDC_EDIT_GAIN);

	// �жϿؼ�����Ч��
	if ((pStcGain == NULL) || (pEditGainVal == NULL))
	{
		return;
	}
    
	GX_STATUS emStatus = GX_STATUS_ERROR;
	CString   strRange = "";
	double    dGainVal = 0.0;

	// ��ȡ�豸����ĸ����ͷ�Χ
	emStatus = GXGetFloatRange(m_hDevice, GX_FLOAT_GAIN, &m_stGainFloatRange);
	GX_VERIFY(emStatus);

	// ��static����ʾ��Χ
    strRange.Format("����(%.4f~%.4f)%s", m_stGainFloatRange.dMin, m_stGainFloatRange.dMax, m_stGainFloatRange.szUnit);
	pStcGain->SetWindowText(strRange);
	
	// ��ȡ��ǰֵ�����µ�����
	emStatus = GXGetFloat(m_hDevice, GX_FLOAT_GAIN, &dGainVal);
	GX_VERIFY(emStatus);

	m_dGainValue = dGainVal;
   
	UpdateData(FALSE);
}

//---------------------------------------------------------------------------------
/**
\brief   ��ʼ����Ͽ�UI����
\param   emFeatureID   [in]    ������ID
\param   pComboBox     [in]    �ؼ�ָ��
\param   pIsImplement  [in]    �Ƿ�֧��emFeatureID����Ĺ���

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::InitEnumUI(GX_FEATURE_ID_CMD emFeatureID, CComboBox *pComboBox, bool bIsImplement)
{
	// �жϿؼ�����Ч��
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

	// ��ȡ���ܵ�ö�������
	emStatus = GXGetEnumEntryNums(m_hDevice, emFeatureID, &nEntryNum);
    GX_VERIFY(emStatus);

	// Ϊ��ȡö���͵Ĺ�����������ռ�
	nbufferSize = nEntryNum * sizeof(GX_ENUM_DESCRIPTION);
	pEnum       = new GX_ENUM_DESCRIPTION[nEntryNum];
	if (pEnum == NULL)
	{
		MessageBox("���仺����ʧ�ܣ�");
		return;
	}

	// ��ȡ���ܵ�ö���������
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

	// ��ȡö���͵ĵ�ǰֵ,����Ϊ���浱ǰ��ʾֵ
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

	// ��ʼ����ǰ�ؼ��Ŀ�ѡ��
	pComboBox->ResetContent();
	for (uint32_t i = 0; i < nEntryNum; i++)
	{	
		pComboBox->SetItemData(pComboBox->AddString(pEnum[i].szSymbolic), (uint32_t)pEnum[i].nValue);
		if (pEnum[i].nValue == nEnumValue)
		{
			nCurcel = i;
		}
	}

	// ���õ�ǰֵΪ�������ʾֵ
	pComboBox->SetCurSel(nCurcel);

	// �ͷź�����Դ
	if (pEnum != NULL)
	{  
		delete []pEnum;
		pEnum = NULL;
    }
}


//---------------------------------------------------------------------------------
/**
\brief   �ɼ�ͼ��ص�����
\param   pFrame      �ص�����

\return  ��
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
        //////////////����ͼ�� //////////////(�����������ͼ��ı���)
		if (pDlg->isFirstFrame==TRUE)
		{
			memcpy(pDlg->m_pBufferRaw,pFrame->pImgBuf,pFrame->nImgSize);
			//�ɼ�����ͼ�����IplImageͼ�������

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

			
			cvCopy(pDlg->curentImage,pDlg->lastImage,NULL); //ͼ��ֵ
			//�ɼ�����ͼ�����IplImageͼ�����currentImage��
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
���ܣ�����ƽ������У��
*******************************************/

void CGxSingleCamMonoDlg::LaserPlaneRefraction()
{
	// 1.��z=a*x+b*y+c�ı궨���ת��ΪA*x+B*y+C*z+D=0����ʽ
	A1 = a1 / sqrt(pow(a1, 2) + pow(b1, 2) + 1);
	B1 = b1 / sqrt(pow(a1, 2) + pow(b1, 2) + 1);
	C1 = -1 / sqrt(pow(a1, 2) + pow(b1, 2) + 1);
	D1 = c1 / sqrt(pow(a1, 2) + pow(b1, 2) + 1);

	// 2.��һ��������ƽ�淽��A*x + B*y + C*z + D = 0����ʽ
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


	// 3.�ڶ���������ƽ�淽��A*x + B*y + C*z + D = 0����ʽ
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
���ܣ���������ͼ�������������
*******************************************/
BOOL CGxSingleCamMonoDlg::ProcessLaserStripeImage(Mat processImage)
{

	/******************step1:ȷ���������Ƶĸ���Ȥ����**************************/
	Mat imageLaserROI(2*ROIHeight,dstWidth,CV_8UC1,Scalar::all(0));
	BOOL roiFlag;
	roiFlag= ROIDetermination(processImage,imageLaserROI); //ִ��ʱ��15ms
	if (roiFlag==FALSE)
	{
		return FALSE;
	} 

	///*******************step2:�Ը���Ȥͼ�������ֵ�˲�**************************/
	medianBlur(imageLaserROI,imageLaserROI,3);    //ִ��ʱ��5ms
	imwrite("imageLaserROI.bmp",imageLaserROI);
	
	///******************step3:�ڸ���Ȥ������ȷ����������������**************************/
	
	vector<Point2f> CenterLaserPoint;
	//����һ�������ݶȷ�����ÿ�е��ݶ�
	//LaserCenterLineGradient(imageLaserROI,CenterLaserPoint);      //ִ��ʱ��28ms
	//���������Ҷ����ķ�������ÿ�е����ģ�Ч�ʸߣ���ֵ���Ժ����Ҷ�����
	LaserCenterLineCentroidBw(imageLaserROI,CenterLaserPoint);        //ִ��ʱ��8ms
	//LaserCenterLineCentroid(imageLaserROI,CenterLaserPoint);


	//���ͼ����û�еõ��������������ߣ���ֱ�ӷ���false����ִ�к����ļ���
	if (CenterLaserPoint.size()<1)
	{
		DrawImg(dstImage, IDC_STATIC_SHOW_FRAME);
		return FALSE;
	}

	m_CenterLaserPoint = CenterLaserPoint;   //��ֵ��ȫ�ֱ���

	//���ǽ��Ҷ�ͼ��ת��Ϊ��ɫͼ��
	Mat imgRGB;
	cvtColor(dstImage, imgRGB, COLOR_GRAY2RGB);

	//������ȡ���ļ������������ߺ�ɫ
	for (int i = 0; i <CenterLaserPoint.size() - 1; i++)
	{
		line(imgRGB, CenterLaserPoint[i], CenterLaserPoint[i + 1], Scalar(0, 0, 255), 2);
	}

	DrawImg(imgRGB, IDC_STATIC_SHOW_FRAME);  //������ȡ�ߺ͵��ͼ��

	return TRUE;
	
}

/******************************************
���ܣ�ȷ���������Ƶĸ���Ȥ����������ͶӰ��,�Ӵ���������������ִ��Ч�ʣ�
˵�����������˲����ͼ�����Ϊ����Ȥ�����ͼ��
*******************************************/

BOOL CGxSingleCamMonoDlg::ROIDetermination(Mat &dstImage,Mat &imageLaserROI)
{
	maxRow=dstHeight/8; //ÿ�ο�ʼ����ǰ��Ҫ����ֵ����ΪͶӰ������λ�����������м�
	int laserProjectValue[dstHeight/4];
	memset(laserProjectValue, 0, sizeof(int)*dstHeight/4);
	int i,j;
	//ÿ��4�лҶ�ֵͶӰһ��
	for (i=0;i<dstHeight;i=i+4)
	{
		laserProjectValue[i/4]=0;
		for (j=0;j<dstWidth;j=j+5)
		{ 
			laserProjectValue[i/4]=laserProjectValue[i/4]+dstImage.at<uchar>(i,j);

		}

	}
	//�ҵ�ͶӰ������ͶӰֵ����λ��
	int laserProjectMaxValue=laserProjectValue[0];
	for ( i=1;i<dstHeight/4;i++)
	{
		if (laserProjectValue[i]>laserProjectMaxValue)
		{
			maxRow=i;
			laserProjectMaxValue=laserProjectValue[i];
		}

	}
	//ȷ������Ȥ����
	maxRow=4*maxRow;  //�����Ǹ��м����
   //ֻ������������������п�������ȷ�ļ�������
	if (maxRow-ROIHeight>0 && maxRow+ROIHeight<dstHeight)
	{
		imageLaserROI=dstImage(Range(maxRow-ROIHeight,maxRow+ROIHeight),Range(0,dstWidth)); 
		return TRUE;
	}

	return FALSE;

}
/******************************************
���ܣ������ݶȷ����㼤������������(ÿ��һ�м���һ���ݶ�)(�ٶ���һЩ)
˵�����ڼ��������ϱ�Ե�²���10�����ص������ڶ��±�Ե����������
�ӿ�ͼ�����ٶ�,�����������ĵ���뵽ȫ�ֱ���CenterLaserPoint��
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
	//�ڼ��������ϱ�Ե�²���10�����ص������ڶ��±�Ե�����������ӿ�ͼ�����ٶ�
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

	/**************����õ���������������*****************/
	//����Ȥ�����ڵ�����ֵ
	for (i=0;i<UpperLaserPoint.size();i++)
	{
		Point2f tempPoint;
		tempPoint.x=(UpperLaserPoint[i].x+BottomLaserPoint[i].x)/2;
		tempPoint.y=(UpperLaserPoint[i].y+BottomLaserPoint[i].y)/2;
		//ת��Ϊԭͼ�������е�ֵ
		if (maxRow-ROIHeight>0)
		{
			tempPoint.y=tempPoint.y+maxRow-ROIHeight;
		}
		CenterLaserPoint.push_back(tempPoint);
	}

}

/******************************************
���ܣ��Զ�ֵ�����ͼ�����ûҶ����ķ����㼤�����������ߣ��ٶȿ죩
˵���������������ĵ���뵽ȫ�ֱ���CenterLaserPoint��,
*******************************************/
void CGxSingleCamMonoDlg::LaserCenterLineCentroidBw(Mat imageLaserROI,vector<Point2f> &CenterLaserPoint)
{
	/******************�Ը���Ȥ�����������Ӧ��ֵ���ָ�**********/
	Mat imageLaserROI_Bw;
	double m_threshold=getThreshVal_Otsu_8u(imageLaserROI);

	double thresholdGainTemp = 1.0;
	threshold(imageLaserROI, imageLaserROI_Bw, thresholdGainTemp*m_threshold, 255, 0);
	imwrite("imageLaserROI_Bw.bmp",imageLaserROI_Bw);

	/******************����ֵ���ָ���ͼ�����ûҶ����ķ����㼤������������**********/
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
		//��һ����ֵ���ָ�֮��û����ֵ���صĻ����򲻽��д���
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
���ܣ�ֱ�Ӷ�ԭʼͼ�����ûҶ����ķ����㼤������������
˵���������������ĵ���뵽ȫ�ֱ���CenterLaserPoint��
*******************************************/
void CGxSingleCamMonoDlg::LaserCenterLineCentroid(Mat imageLaserROI,vector<Point2f> &CenterLaserPoint)
{
	/******************��ͼ�����ûҶ����ķ����㼤������������**********/
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
		//�����һ�лҶ�ֵ֮��Ϊ0�Ļ����򲻽��д���
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
���ܣ�����opencv�⺯��HoughLines���ֱ����ȡ
˵������HoughLines��ȡ��ֱ�߽��з������ҳ�����Ȥ��ֱ��
*******************************************/
bool CGxSingleCamMonoDlg::FindLinesHough(vector<Vec2f>&linesInterest,vector<Point2f> &CenterLaserPoint)
{
	/**********���ü����������������ϵĵ����һ��ͼ��*******/
	Mat midImage(dstHeight,dstWidth,CV_8UC1,Scalar::all(0));
	int i,j;
	for (i=0;i<CenterLaserPoint.size();i++)
	{
		midImage.at<uchar>(CenterLaserPoint[i].y,CenterLaserPoint[i].x)=255;

	}
	imwrite("midImage.bmp",midImage);
	//DrawImg(midImage,IDC_STATIC_SHOW_FRAME);
	////����任������������������Ϊbinary,lines,deltaRho,deltaTheta,minVote
	vector<Vec2f> lines;     //��һ��Ԫ��Ϊpho���ڶ���Ԫ��Ϊtheta
	double deltaRho=1;
	double deltaTheta=CV_PI/180;
	int minVote=60;  
	HoughLines(midImage,lines,deltaRho,deltaTheta,minVote);

	//����ȡ����ֱ�߷�Ϊ����(����ʵ�������ڸ��ţ�������ȡ������ֱ�ߣ�ֻ����ȡ������ֱ���Ժ�Ž��к�������)
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
			if (abs(lines[i][1]-linesOne[0][1])<=6*CV_PI/180)    //����Ƕ�5�ȵ�ֵ�ܹؼ�����ΪС��5�ȣ���Ϊͬһ��ֱ��(�ʼȡֵΪ3���ܲ��ȶ�)
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
				//��theta������С������ֱ����Ϊ����Ȥֱ��
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
���ܣ���hough�仯ֱ�߸����ĵ������С������ϣ������Ƴ���
˵����
*******************************************/
void CGxSingleCamMonoDlg::GetInterestLine(vector<Vec2f> &linesInterest,vector<Point2f> &CenterLaserPoint)
{
	//�Ը���Ȥֱ�߸����ĵ������С�������
	double k,b;
	vector<Vec2f> linesInterestPointSlope;
	for (int i=0;i<linesInterest.size();i++)
	{
		if (linesInterest[i][1]!=0) //�������Ȥֱ�߲�����ֱֱ��
		{
			k=-cos(linesInterest[i][1])/sin(linesInterest[i][1]);
			b=linesInterest[i][0]/sin(linesInterest[i][1]);
			//����ֱ�߽��ĵ������ϵ㼯��
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
			//������С�������
			Vec2f pointSlope=LineFitting(fittingPoint);
			linesInterestPointSlope.push_back(pointSlope);
			//��ͼ���л�����ϵõ���ֱ��
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
���ܣ����ݸ����㣬������С�������
˵��������Ϊһ��㣬����ֵΪб�ʺͽؾ�
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
���ܣ����������䷽��õ�ͼ���ֵ������ֵ
˵����������mat���ͣ�8λ��ͨ����ͼ�����Ϊ�õ�����ֵ��0-255֮�������
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
\brief   ��m_pImageBuffer��ͼ����ʾ������

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::DrawImg(Mat image,UINT ID)
{
	//����CvvImage�������ʾ
	CDC *pDC = GetDlgItem(ID)->GetDC();     //����ID��ô���ָ���ٻ�ȡ��ô��ڹ�����������ָ��
	HDC hDC= pDC->GetSafeHdc();            // ��ȡ�豸�����ľ��
	CRect rect;
	GetDlgItem(ID)->GetClientRect(&rect);    //��ȡ��ʾ��

	IplImage* img=&IplImage(image);	//��ͼ��ת��ΪIplImage��ʽ������ͬһ���ڴ棨ǳ������
	CvvImage cimg;
	cimg.CopyOf( img ); //����ͼƬ
	cimg.DrawToHDC( hDC, &rect ); //��ͼƬ���Ƶ���ʾ�ؼ���ָ��������
	ReleaseDC( pDC );
}

//---------------------------------------------------------------------------------
/**
\brief   �ͷ�Ϊͼ����ʾ׼������Դ

\return  ��
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
	//����Ƕ�IplImageָ�����Image�����ڴ��ͷ�,������cvReleaseImage��������delete
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
\brief   ���"������ͼ"��ť��Ӧ����

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnSofttrigger() 
{
	// TODO: Add your control notification handler code here
	//������������(�ڴ���ģʽΪOn�ҿ�ʼ�ɼ�����Ч)
	GX_STATUS emStatus = GX_STATUS_ERROR;
	emStatus = GXSendCommand(m_hDevice, GX_COMMAND_TRIGGER_SOFTWARE);

	GX_VERIFY(emStatus);
}

//---------------------------------------------------------------------------------
/**
\brief   �л�"����ģʽ"ѡ����Ӧ����

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnCbnSelchangeComboTriggerMode() 
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	bool bIsWritable = TRUE;
	int64_t  nCurrentEnumValue = 0;
	CComboBox *pBox    = (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_MODE);

	int      nIndex    = pBox->GetCurSel();                     // ��ȡ��ǰѡ��
	int64_t  nEnumVal  = (int64_t)pBox->GetItemData(nIndex);    // ��ȡ��ǰѡ���Ӧ��ö����ֵ

	//�жϴ���ģʽö��ֵ�Ƿ��д
	emStatus = GXIsWritable(m_hDevice,GX_ENUM_TRIGGER_MODE,&bIsWritable);
	GX_VERIFY(emStatus);

	//��ȡ��ǰ����ģʽö��ֵ
	emStatus = GXGetEnum(m_hDevice, GX_ENUM_TRIGGER_MODE, &nCurrentEnumValue);
	GX_VERIFY(emStatus);

	if (bIsWritable)
	{
		//���õ�ǰѡ��Ĵ���ģʽö��ֵ
		emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_MODE, nEnumVal);
		GX_VERIFY(emStatus);

		// ���µ�ǰ����ģʽ
		m_nTriggerMode = nEnumVal;
	}
	else
	{
		MessageBox("��ǰ״̬����д����ֹͣ�ɼ�������������!");
		//�жϵ�ǰѡ��Ĵ���ģʽö��ֵ��ѡ��֮ǰ�Ƿ���ͬ�������ͬ������л�����֮���������л�
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
\brief   ��m_pImageBufferͼ�����ݱ����BMPͼƬ

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::SaveImage()
{
	imwrite("Image.bmp",dstImage);
}

//----------------------------------------------------------------------------------
/**
\brief  ��ȡGxIAPI��������,������������ʾ�Ի���
\param  emErrorStatus  [in]   ������

\return ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::ShowErrorString(GX_STATUS emErrorStatus)
{
	char*     pchErrorInfo = NULL;
	size_t    nSize        = 0;
	GX_STATUS emStatus     = GX_STATUS_ERROR;

	// ��ȡ������Ϣ���ȣ��������ڴ�ռ�
	emStatus = GXGetLastError(&emErrorStatus, NULL, &nSize);
	pchErrorInfo = new char[nSize];
	if (NULL == pchErrorInfo)
	{
		return;
	}

	//��ȡ������Ϣ����������ʾ
	emStatus = GXGetLastError (&emErrorStatus, pchErrorInfo, &nSize);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		MessageBox("GXGetLastError�ӿڵ���ʧ�ܣ�");
	}
	else
	{
		MessageBox((LPCTSTR)pchErrorInfo);
	}

	// �ͷ�������ڴ�ռ�
	if (NULL != pchErrorInfo)
	{
		delete[] pchErrorInfo;
		pchErrorInfo = NULL;
	}
}

//----------------------------------------------------------------------------------
/**
\brief  ˢ��UI����ؼ�ʹ��״̬

\return �޷���ֵ
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
\brief  ���"���豸"�ؼ���Ӧ����

\return ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnOpenDevice()
{
	// TODO: Add your control notification handler code here
	GX_STATUS     emStatus = GX_STATUS_SUCCESS;
	uint32_t      nDevNum  = 0;
	GX_OPEN_PARAM stOpenParam;

	// ��ʼ�����豸�ò���,Ĭ�ϴ����Ϊ1���豸
	stOpenParam.accessMode = GX_ACCESS_EXCLUSIVE;
	stOpenParam.openMode   = GX_OPEN_INDEX;
	stOpenParam.pszContent = "1";

	// ö���豸
	emStatus = GXUpdateDeviceList(&nDevNum, 1000);
	GX_VERIFY(emStatus);

	// �жϵ�ǰ�����豸����
	if (nDevNum <= 0)
	{
		MessageBox("δ�����豸!");
		return;
	}

	// ����豸�Ѿ�����ر�,��֤����ڳ�ʼ������������ٴδ�
	if (m_hDevice != NULL)
	{
		emStatus = GXCloseDevice(m_hDevice);
		GX_VERIFY(emStatus);
		m_hDevice = NULL;
	}

	// ���豸
	emStatus = GXOpenDevice(&stOpenParam, &m_hDevice);
	GX_VERIFY(emStatus);
    m_bDevOpened = TRUE;

	// ���������Ĭ�ϲ���:�ɼ�ģʽ:�����ɼ�,���ݸ�ʽ:8-bit
	emStatus = InitDevice();
	GX_VERIFY(emStatus);

	// ��ȡ�豸�Ŀ��ߵ�������Ϣ
    emStatus = GetDeviceParam();
	GX_VERIFY(emStatus);

	// ��ȡ�������,��ʼ������ؼ�
	InitUI();

	// ���½���ؼ�
	UpDateUI();

}

//----------------------------------------------------------------------------------
/**
\brief  ���"�ر��豸"�ؼ���Ӧ����

\return ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnCloseDevice()
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// δͣ��ʱ��ֹͣ�ɼ�
	if (m_bIsSnap)
	{
		//����ֹͣ�ɼ�����
		emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);
		if (emStatus != GX_STATUS_SUCCESS)
		{
			// ������
		}

		//ע���ص�
		emStatus = GXUnregisterCaptureCallback(m_hDevice);
		if (emStatus != GX_STATUS_SUCCESS)
		{
			// ������
		}

		m_bIsSnap = FALSE;
		// �ͷ�Ϊ�ɼ�ͼ�񿪱ٵ�ͼ��Buffer
		UnPrepareForShowImg();

		//�رն�ʱ��
		KillTimer(1);
	}

	//�ر��豸
	emStatus = GXCloseDevice(m_hDevice);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		// ������
	}

	m_bDevOpened = FALSE;
	m_hDevice    = NULL;

	// ���½���UI
	UpDateUI();


}

//----------------------------------------------------------------------------------
/**
\brief  ���"ֹͣ�ɼ�"�ؼ���Ӧ����

\return ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnStopSnap()
{
	// TODO: Add your control notification handler code here
	//�رն�ʱ��
	KillTimer(1);

	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	//����ֹͣ�ɼ�����
	emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);
	GX_VERIFY(emStatus);

	//ע���ص�
	emStatus = GXUnregisterCaptureCallback(m_hDevice);
	GX_VERIFY(emStatus);
   
	m_bIsSnap = FALSE;

	// �ͷ�Ϊ�ɼ�ͼ�񿪱ٵ�ͼ��Buffer
	UnPrepareForShowImg();

	// ���½���UI
	UpDateUI();

	
}

// ---------------------------------------------------------------------------------
/**
\brief   ��ʼ�����:����Ĭ�ϲ���

\return  ��
*/
// ----------------------------------------------------------------------------------
GX_STATUS CGxSingleCamMonoDlg::InitDevice()
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	//���òɼ�ģʽ�����ɼ�
	emStatus = GXSetEnum(m_hDevice,GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
	VERIFY_STATUS_RET(emStatus);

	// ��֪��ǰ���֧���ĸ�8λͼ�����ݸ�ʽ����ֱ������
	// �����豸֧�����ݸ�ʽGX_PIXEL_FORMAT_BAYER_GR8,��ɰ������´���ʵ��
	// emStatus = GXSetEnum(m_hDevice, GX_ENUM_PIXEL_FORMAT, GX_PIXEL_FORMAT_BAYER_GR8);
	// VERIFY_STATUS_RET(emStatus);

	// �������ǰ��������ݸ�ʽʱ�����Ե������º�����ͼ�����ݸ�ʽ����Ϊ8Bit
	emStatus = SetPixelFormat8bit(); 
	VERIFY_STATUS_RET(emStatus);

	//������ӵĴ���
	//��������Ĳɼ�֡��
	//emStatus = GXSetEnum(m_hDevice, GX_ENUM_ACQUISITION_FRAME_RATE_MODE, 0 );
	//VERIFY_STATUS_RET(emStatus);
	
	/*emStatus = GXSetFloat(m_hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, 75  );
	VERIFY_STATUS_RET(emStatus);*/

	return emStatus;
}

// ---------------------------------------------------------------------------------
/**
\brief   ������������ݸ�ʽΪ8bit

\return  emStatus GX_STATUS_SUCCESS:���óɹ�������״̬��:����ʧ��
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

	// ��ȡ����λ���С
	emStatus = GXGetEnum(m_hDevice, GX_ENUM_PIXEL_SIZE, &nPixelSize);
	VERIFY_STATUS_RET(emStatus);

	// �ж�Ϊ8bitʱֱ�ӷ���,��������Ϊ8bit
	if (nPixelSize == GX_PIXEL_SIZE_BPP8)
	{
		return GX_STATUS_SUCCESS;
	}
	else
	{
		// ��ȡ�豸֧�ֵ����ظ�ʽ��ö�������
		emStatus = GXGetEnumEntryNums(m_hDevice, GX_ENUM_PIXEL_FORMAT, &nEnmuEntry);
		VERIFY_STATUS_RET(emStatus);

		// Ϊ��ȡ�豸֧�ֵ����ظ�ʽö��ֵ׼����Դ
		nBufferSize      = nEnmuEntry * sizeof(GX_ENUM_DESCRIPTION);
		pEnumDescription = new GX_ENUM_DESCRIPTION[nEnmuEntry];

		// ��ȡ֧�ֵ�ö��ֵ
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

		// �����豸֧�ֵ����ظ�ʽ,�������ظ�ʽΪ8bit,
		// ���豸֧�ֵ����ظ�ʽΪMono10��Mono8��������ΪMono8
		for (uint32_t i = 0; i<nEnmuEntry; i++)
		{
			if ((pEnumDescription[i].nValue & GX_PIXEL_8BIT) == GX_PIXEL_8BIT)
			{
				emStatus = GXSetEnum(m_hDevice, GX_ENUM_PIXEL_FORMAT, pEnumDescription[i].nValue);
				break;
			}
		}	

		// �ͷ���Դ
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
\brief  ��ȡ�豸�Ŀ�ߵ�������Ϣ

\return GX_STATUS_SUCCESS:ȫ����ȡ�ɹ�������״̬��:δ�ɹ���ȡȫ��
*/
//----------------------------------------------------------------------------------
GX_STATUS CGxSingleCamMonoDlg::GetDeviceParam()
{
	GX_STATUS emStatus       = GX_STATUS_ERROR;
	bool      bIsImplemented = false;

	// ��ȡͼ���С
	emStatus = GXGetInt(m_hDevice, GX_INT_PAYLOAD_SIZE, &m_nPayLoadSize);
	VERIFY_STATUS_RET(emStatus);

	// ��ȡ���
	emStatus = GXGetInt(m_hDevice, GX_INT_WIDTH, &m_nImageWidth);
	VERIFY_STATUS_RET(emStatus);

	// ��ȡ�߶�
	emStatus = GXGetInt(m_hDevice, GX_INT_HEIGHT, &m_nImageHeight);      
	
	//��ȡ����ɼ�֡��
	/*emStatus = GXGetFloat(m_hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, &m_nAcquisitionFrameRate  );
	CString stringAcquisitionFrame;
	stringAcquisitionFrame.Format("%lf",m_nAcquisitionFrameRate);
	m_ListWords.AddString(stringAcquisitionFrame);
*/
	return emStatus;
}

//----------------------------------------------------------------------------------
/**
\brief  Ϊ����ͼ�����Buffer,Ϊͼ����ʾ׼����Դ

\return true;Ϊͼ����ʾ׼����Դ�ɹ�  false:׼����Դʧ��
*/
//----------------------------------------------------------------------------------
bool  CGxSingleCamMonoDlg::PrepareForShowImg()
{	
	
	//--------------------------------------------------------------------------
	//------------------------ͼ������Buffer����---------------------------------
	//Ϊԭʼͼ�����ݷ���ռ�
	m_pBufferRaw = new BYTE[(size_t)m_nPayLoadSize];
	if (m_pBufferRaw == NULL)
	{
		return false;
	}

	//Ϊ������ת���ͼ�����ݷ���ռ�
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
\brief  ���"����BMPͼ��"CheckBox����Ϣ��Ӧ����

\return ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedCheckSaveImage()
{
	// TODO: Add your control notification handler code here
	m_bIsSaveImg = !m_bIsSaveImg;
}

//----------------------------------------------------------------------------------
/**
\brief  ���"��ʼ�ɼ�"�ؼ���Ӧ����

\return ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnBnClickedBtnStartSnap()
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_ERROR;
	

	//  ����ͼ�񣨵����ʼ�ɼ���ť������һ��ͼ��ֹͣ�ɼ���ʱ����Ҫ�ͷ��ڴ棩
	image = cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);
	curentImage=cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);
	lastImage=cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);
	lastImageProcess=cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);
	curentImageProcess=cvCreateImage(cvSize(m_nImageWidth,m_nImageHeight),IPL_DEPTH_8U,1);

	//�������Ϊ�Դ���
	

	// ��ʼ��BMPͷ��Ϣ������BufferΪͼ��ɼ���׼��
	if (!PrepareForShowImg())
	{
		MessageBox("Ϊͼ����ʾ׼����Դʧ��!");
        return;
	}
	
	//ע��ص�
	emStatus = GXRegisterCaptureCallback(m_hDevice, this, OnFrameCallbackFun);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		UnPrepareForShowImg();
		ShowErrorString(emStatus);
		return;
	}

	//����ʼ�ɼ�����
	emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_START);
	if (emStatus != GX_STATUS_SUCCESS)
	{
		UnPrepareForShowImg();
		ShowErrorString(emStatus);
		return;
	}

	m_bIsSnap = TRUE;

	//// ���½���UI
	UpDateUI();


}

//---------------------------------------------------------------------------------
/**
\brief   �л�"��������"ѡ����Ӧ����

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnCbnSelchangeComboTriggerActive()
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	CComboBox *pBox    = (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_ACTIVE);

	int      nIndex    = pBox->GetCurSel();                   // ��ȡ��ǰѡ��
	int64_t  nEnumVal  = (int64_t)pBox->GetItemData(nIndex);  // ��ȡ��ǰѡ���Ӧ��ö����ֵ
    
	//���õ�ǰѡ��Ĵ������Ե�ֵ
	emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_ACTIVATION, nEnumVal);
    GX_VERIFY(emStatus);
}

//---------------------------------------------------------------------------------
/**
\brief   �л�"����Դ"ѡ����Ӧ����

\return  ��
*/
//----------------------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnCbnSelchangeComboTriggerSource()
{
	// TODO: Add your control notification handler code here
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	CComboBox *pBox    = (CComboBox *)GetDlgItem(IDC_COMBO_TRIGGER_SOURCE);

	int      nIndex    = pBox->GetCurSel();                   // ��ȡ��ǰѡ��
	int64_t  nEnumVal  = (int64_t)pBox->GetItemData(nIndex);  // ��ȡ��ǰѡ���Ӧ��ö����ֵ

	//���õ�ǰѡ��Ĵ���Դ��ֵ
	emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_SOURCE, nEnumVal);
	GX_VERIFY(emStatus);
}

//-----------------------------------------------------------------------
/**
\brief    �����ع�ʱ���Edit��ʧȥ�������Ӧ����

\return   ��
*/
//-----------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnKillfocusEditShutter() 
{
	// TODO: Add your control notification handler code here
	//�жϾ���Ƿ���Ч�������豸���ߺ�رճ�����ֵ��쳣
	if (m_hDevice == NULL)
	{
		return;
	}

	UpdateData();
	GX_STATUS status = GX_STATUS_SUCCESS;

	//�ж�����ֵ�Ƿ����ع�ʱ��ķ�Χ��
	//���������ֵ���ع�ֵ��Ϊ���ֵ
	if (m_dShutterValue > m_stShutterFloatRange.dMax)
	{
		m_dShutterValue = m_stShutterFloatRange.dMax;
	}
	//��С����Сֵ���ع�ֵ��Ϊ��Сֵ
	if (m_dShutterValue < m_stShutterFloatRange.dMin)
	{
		m_dShutterValue = m_stShutterFloatRange.dMin;
	}

	//�����ع�ʱ��
	status = GXSetFloat(m_hDevice,GX_FLOAT_EXPOSURE_TIME,m_dShutterValue);
	GX_VERIFY(status);

	UpdateData(FALSE);
}

//-----------------------------------------------------------------------
/**
\brief   ���������Edit��ʧȥ�������Ӧ����

\return  ��
*/
//-----------------------------------------------------------------------
void CGxSingleCamMonoDlg::OnKillfocusEditGain() 
{
	// TODO: Add your control notification handler code here
	//�жϾ���Ƿ���Ч�������豸���ߺ�رճ�����ֵ��쳣
	if (m_hDevice == NULL)
	{
		return;
	}
	
	UpdateData();
	GX_STATUS status = GX_STATUS_SUCCESS;

	//�ж�����ֵ�Ƿ�������ֵ�ķ�Χ��
	//�������ֵ�������ֵ������ֵ���ó����ֵ
	if (m_dGainValue > m_stGainFloatRange.dMax)
	{
		m_dGainValue = m_stGainFloatRange.dMax;
	}
	
	//�������ֵС����Сֵ�������ֵ���ó���Сֵ
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (m_hDevice == NULL)
	{
		return;
	}

	UpdateData();
	GX_STATUS emStatus = GX_STATUS_ERROR;

	//�ж�����ֵ�Ƿ���֡��ֵ�ķ�Χ��
	//�������ֵ�������ֵ��֡��ֵ���ó����ֵ
	if (m_dFrameRate> 75)
	{
		m_dFrameRate = 75;
	}

	//�������ֵС����Сֵ��֡�ʵ�ֵ���ó���Сֵ
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
\brief   �ַ�������Ҫ�����ع�������Edit����Ӧ�س�������Ϣ
\param   pMsg  ��Ϣ�ṹ��

\return �ɹ�:TRUE   ʧ��:FALSE
*/
//-----------------------------------------------------------------------
BOOL CGxSingleCamMonoDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	CWnd *pWnd = NULL;
	int   nID = 0;          //< �����ȡ�Ŀؼ�ID
	GX_STATUS status = GX_STATUS_SUCCESS;

	//�ж��Ƿ��Ǽ��̻س���Ϣ
	if (WM_KEYDOWN == pMsg->message && VK_RETURN == pMsg->wParam)
	{
		//��ȡ��ǰӵ�����뽹��Ĵ���(�ؼ�)ָ��
		pWnd = GetFocus();

		//��ȡ��������λ�õĿؼ�ID
		nID = pWnd->GetDlgCtrlID();

		//�жϿؼ�ID����
		switch (nID)
		{

		//������ع�EDIT���ID�������ع�ʱ��
		case IDC_EDIT_SHUTTER:

        //���������EDIT���ID����������ֵ
		case IDC_EDIT_GAIN:

		case IDC_EDIT_FrameRate:
			
			//ʧȥ����
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
���ܣ��ڶ�ʱ�������������ͼ����
˵��������ص��������ͣ����ڻص�������ֻ�����ͼ��ɼ�������BYTE�ͱ���m_pBufferRaw�У�
*******************************************/
void CGxSingleCamMonoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CString snFrame, snFrame1, snFrameString;


	switch(nIDEvent)
	{
	case 1:
		{
			nFrame++;

			////ͼ��ֵ��ʱ���������
			critical_section.Lock();
			cvCopy(curentImage, image, NULL); //ͼ��ֵ
			critical_section.Unlock();
			//dstImage��Mat��������,(��IplImage���imageת��ΪMat���dstImage)
			dstImage= cvarrToMat(image);   
			
			CString sXWater, sYWater, sZWater, sXAir, sYAir, sZAir,sPixelX, sPixelY;
			BOOL isProcessed;
			CString strWater,strAir, str,strFrame;

			//////////////ͼ�������///////////////////
	
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

					Vec3f p01_dir;   //��һ�εķ�������
					//float norm_p1 = sqrt( pow(p1[0], 2) + pow(p1[1], 2) + pow(p1[2], 2) );
					//p01_dir[0] = p1[0] / norm_p1;
					//p01_dir[1] = p1[1] / norm_p1;
					//p01_dir[2] = p1[2] / norm_p1;

					float norm_p1 = norm(p1);
					p01_dir = p1 / norm(p1);

					//////1.�ڲ����ϱ����������/////////
					float alpha1 = refractivityAir / refractivityGlass;      //����alpha1
					Vec3f normalVectorGlass = {a,b,c};
					float p01NormalDot = p01_dir[0] * normalVectorGlass[0] + p01_dir[1] * normalVectorGlass[1] + p01_dir[2] * normalVectorGlass[2];
					float beta1 = sqrt(1 - pow(alpha1, 2)*(1 - pow(p01NormalDot, 2))) - alpha1*p01NormalDot;
					
					Vec3f p12;
					Vec3f p12_dir;   //��һ�εķ�������
					p12[0] = alpha1*p01_dir[0] + beta1*normalVectorGlass[0];
					p12[1] = alpha1*p01_dir[1] + beta1*normalVectorGlass[1];
					p12[2] = alpha1*p01_dir[2] + beta1*normalVectorGlass[2];

					//float norm_p2 = sqrt(pow(p12[0], 2) + pow(p12[1], 2) + pow(p12[2], 2));
					//p12_dir[0] = p12[0] / norm_p2;
					//p12_dir[1] = p12[1] / norm_p2;
					//p12_dir[2] = p12[2] / norm_p2;

					float norm_p2 = norm(p12);
					p12_dir = p12 / norm_p2;

					//���ݲ����ϱ��������������Ͳ�����ȣ������·�ﵽ�����±���ĵ�����

					float p12NormalDot = (p12_dir[0] * normalVectorGlass[0] + p12_dir[1] * normalVectorGlass[1] + p12_dir[2] * normalVectorGlass[2]);
					Vec3f p2;
					p2[0] = p1[0] + d*p12_dir[0] / p12NormalDot;
					p2[1] = p1[1] + d*p12_dir[1] / p12NormalDot;
					p2[2] = p1[2] + d*p12_dir[2] / p12NormalDot;
				    
					//////2.�ڲ����±����������/////////
					float alpha2 = refractivityGlass / refractivityWater;      //����alpha2
					float beta2 = sqrt(1 - pow(alpha2, 2)*(1 - pow(p12NormalDot, 2))) - alpha2*p12NormalDot;

					Vec3f p23;
					Vec3f p23_dir;   //�ڶ��εķ�������
					p23[0] = alpha2*p12_dir[0] + beta2*normalVectorGlass[0];
					p23[1] = alpha2*p12_dir[1] + beta2*normalVectorGlass[1];
					p23[2] = alpha2*p12_dir[2] + beta2*normalVectorGlass[2];

					//float norm_p3 = sqrt(pow(p23[0], 2) + pow(p23[1], 2) + pow(p23[2], 2));
					//p23_dir[0] = p23[0] / norm_p3;
					//p23_dir[1] = p23[1] / norm_p3;
					//p23_dir[2] = p23[2] / norm_p3;

					float norm_p3 = norm(p23);
					p23_dir = p23 / norm_p3;

					//////3.������������ˮ�кͿ����е���ά����/////////
					centerLaserPointUnderwater_mm.x = -(p23_dir[0])*(A3*p2[0] + B3*p2[1] + C3*p2[2] + D3) / (A3*p23_dir[0] + B3*p23_dir[1] + C3*p23_dir[2]) + p2[0];
					centerLaserPointUnderwater_mm.y = -(p23_dir[1])*(A3*p2[0] + B3*p2[1] + C3*p2[2] + D3) / (A3*p23_dir[0] + B3*p23_dir[1] + C3*p23_dir[2]) + p2[1];
					centerLaserPointUnderwater_mm.z = -(p23_dir[2])*(A3*p2[0] + B3*p2[1] + C3*p2[2] + D3) / (A3*p23_dir[0] + B3*p23_dir[1] + C3*p23_dir[2]) + p2[2];

					centerLaserPointAir_mm.z = -D1*kx*ky / (C1*kx*ky + A1*ky*(m_CenterLaserPoint[i].x - u0) + B1*kx*(m_CenterLaserPoint[i].y - v0));
					centerLaserPointAir_mm.x = centerLaserPointAir_mm.z*(m_CenterLaserPoint[i].x - u0) / kx;
					centerLaserPointAir_mm.y = centerLaserPointAir_mm.z*(m_CenterLaserPoint[i].y - v0) / ky;

					//////4.�����������ά���걣�浽�ı���/////////
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

			/************ͼ�񱣴洦��************/
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






