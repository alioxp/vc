
// PhotoSyncDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CPhotoSyncDlg �Ի���
class CPhotoSyncDlg : public CDialogEx
{
// ����
public:
	CPhotoSyncDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_PHOTOSYNC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	void BrowseCurrentAllFile(CString strDir);
	void BrowseCurrentDir(CString strDir);
	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonsrc();
	afx_msg void OnBnClickedButtondest();
	afx_msg void OnBnClickedButton1();
	CEdit m_editSrc;
	CEdit m_editDest;
	afx_msg void OnEnChangeEditdest2();
	afx_msg void OnBnClickedButtonbackup();
	CEdit m_editBackup;
};
