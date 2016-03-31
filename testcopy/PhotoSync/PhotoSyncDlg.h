
// PhotoSyncDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CPhotoSyncDlg 对话框
class CPhotoSyncDlg : public CDialogEx
{
// 构造
public:
	CPhotoSyncDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_PHOTOSYNC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	void BrowseCurrentAllFile(CString strDir);
	void BrowseCurrentDir(CString strDir);
	// 生成的消息映射函数
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
