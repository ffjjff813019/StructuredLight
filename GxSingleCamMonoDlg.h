// GxSingleCamMonoDlg.h : header file
//

#if !defined(AFX_GxSingleCamMonoDLG_H__D8D963C2_3BE2_44EC_B0EB_FCE016DDD6FA__INCLUDED_)
#define AFX_GxSingleCamMonoDLG_H__D8D963C2_3BE2_44EC_B0EB_FCE016DDD6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///< ������ʾ�����궨��
#define  GX_VERIFY(emStatus) \
	                        if (emStatus != GX_STATUS_SUCCESS)\
							{\
							ShowErrorString(emStatus); \
							return ;\
							 } 
///< �жϷ���ֵ�궨��
#define VERIFY_STATUS_RET(emStatus) \
	                              if (emStatus != GX_STATUS_SUCCESS) \
								   {\
								   return emStatus;\
								   }\

/////////////////////////////////////////////////////////////////////////////
// CGxSingleCamMonoDlg dialog
//���ͷ�ļ�
#include "GxIAPI.h"
#include "DxImageProc.h"
//ͼ����ʾ���������ļ�
#include "CvvImage.h"
//OpenCVͷ�ļ�
#include <opencv2/opencv.hpp>  
#include <math.h>
using namespace cv;
#include <vector>
#include "afxwin.h"


//ʹ��ͬ����
#include "afxmt.h"

class CGxSingleCamMonoDlg : public CDialog
{
// Construction
public:
	CGxSingleCamMonoDlg(CWnd* pParent = NULL);	// standard constructor
	~CGxSingleCamMonoDlg();//��������
public:
	/// ��ʼ�����:����Ĭ�ϲ���
	GX_STATUS InitDevice();
	/// ������������ݸ�ʽΪ8bit
	GX_STATUS SetPixelFormat8bit();
	/// ��ȡ�豸�Ŀ��ߵ�������Ϣ
	GX_STATUS GetDeviceParam();
    /// ��ȡ�豸����,��ʼ������ؼ�
	void InitUI();
	/// ��ʼ��UI�������
	void InitUIParam(GX_DEV_HANDLE hDevice);
    /// ˢ��UI����
	void UpDateUI();
	///  �ͷ�Ϊͼ����ʾ׼����Դ
	void UnPrepareForShowImg();
	/// ��ʾͼ��
	void DrawImg(Mat,UINT);
	/// ����ƽ������У��
	void LaserPlaneRefraction();
    ///��������ͼ��ʵʱ����
	BOOL ProcessLaserStripeImage(Mat);
    //�������Ƹ���Ȥ����ȷ��
	BOOL ROIDetermination(Mat&,Mat&);         
	//�����ݶȷ���ȡ��������������
	void LaserCenterLineGradient(Mat,vector<Point2f> &CenterLaserPoint);
	 //��ͼ���ֵ���Ժ����ûҶ����ķ���ȡ��������������
	void LaserCenterLineCentroidBw (Mat,vector<Point2f> &CenterLaserPoint);
	//ֱ�Ӷ�ԭʼͼ�����ûҶ����ķ���ȡ��������������
	void LaserCenterLineCentroid(Mat,vector<Point2f> &CenterLaserPoint); 
	//����opencv����任������ȡ��������ֱ��
	bool FindLinesHough(vector<Vec2f> &linsInterest,vector<Point2f> &CenterLaserPoint);
	//�õ����ո���Ȥ��ֱ��
	void GetInterestLine(vector<Vec2f> &linsInterest,vector<Point2f> &CenterLaserPoint);
	//���ݸ����ĵ㣬������С�������ֱ��
	Vec2f LineFitting(vector<Point2f> &fittingPoint);
	//Ostu����Ӧ��ȡ��ֵ
	double getThreshVal_Otsu_8u( const Mat& _src);
	/// ����ͼ��
	void SaveImage();
	
	/// ��ȡGxIAPI��������,������������ʾ�Ի���
	void ShowErrorString(GX_STATUS emErrorStatus);

	/// ��ʼ��ö������UI����
	void InitEnumUI(GX_FEATURE_ID_CMD emFeatureID, CComboBox *pComboBox, bool bIsImplement);
	
	/// ��ʼ���������ؿؼ�
	void InitGainUI();
	
	/// ��ʼ���ع���ؿؼ�
	void InitShutterUI();

	/// ��ʼ������ģʽ��ؿؼ�
	void InitTriggerModeUI();

	/// ��ʼ������Դ��ؿؼ�
	void InitTriggerSourceUI();

	/// ��ʼ������������ؿؼ�
	void InitTriggerActivationUI();

	/// �ص�����
	static void __stdcall OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM* pFrame);

	///  Ϊͼ����ʾ׼����Դ
	bool PrepareForShowImg();



public:
	CFile file;
	IplImage * image;
	IplImage * lastImage;
	IplImage * curentImage;
	IplImage * lastImageProcess;
	IplImage * curentImageProcess;
	Mat dstImage;                         //ͼ�����õ���Mat����
	BOOL    isFirstFrame;                  //�ɼ������Ƿ�Ϊ��һ֡ͼ��
	BOOL    isFisnshSecondFrame;           //�Ƿ���ɵڶ�֡�Ĳɼ�

	vector<Point2f> m_CenterLaserPoint;    //�����������������㼯��
	Point2f m_FeaturePoint;                //ͼ������ϵ������������
	Point3f centerLaserPointAir_mm;             //�����������������������ϵ�µ�����
	Point3f centerLaserPointUnderwater_mm;      //��������������������

//	bool featureLinesFlag;
public:
	GX_DEV_HANDLE           m_hDevice;            ///< �豸���	
	int64_t                 m_nPayLoadSize;       ///< ͼ���С
	int64_t                 m_nImageHeight;       ///< ԭʼͼ���
    int64_t                 m_nImageWidth;        ///< ԭʼͼ���	
	int64_t                 m_nTriggerMode;       ///< ����ģʽ

	double                 m_nAcquisitionFrameRate;   ///���õĲɼ�֡��
	double                 m_nCurrentAcquisitionFrameRate; ///��ǰ�Ĳɼ�֡��
	GX_FLOAT_RANGE          m_stShutterFloatRange;///< �����ع�ʱ��ķ�Χ
	GX_FLOAT_RANGE          m_stGainFloatRange;   ///< ��������ֵ�ķ�Χ

	BYTE                   *m_pBufferRaw;        ///< ͼ��ԭʼ����
	BYTE                   *m_pImageBuffer;      ///< ���淭ת���ͼ��������ʾ��
	char                    m_chBmpBuf[2048];    ///< BIMTAPINFO �洢��������m_pBmpInfo��ָ��˻�����
	BOOL                    m_bIsSaveImg;        ///< ��ʶ�Ƿ񱣴�ͼ��
	BOOL                    m_bDevOpened;        ///< ��ʶ�Ƿ��Ѵ��豸
	BOOL                    m_bIsSnap;           ///< ��ʶ�Ƿ�ʼ�ɼ�

	bool                    m_bTriggerMode;      ///< �Ƿ�֧�ִ���ģʽ
	bool                    m_bTriggerActive;    ///< �Ƿ�֧�ִ�������
	bool                    m_bTriggerSource;    ///< �Ƿ�֧�ִ���Դ 

 //   CWnd                    *m_pWnd;             ///< ��ʾͼ�񴰿�(�ؼ�)ָ��
//	HDC                     m_hDC;               ///< ����ͼ��DC���

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
