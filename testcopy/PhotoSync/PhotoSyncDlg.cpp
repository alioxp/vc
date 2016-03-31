
// PhotoSyncDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PhotoSync.h"
#include "PhotoSyncDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CPhotoSyncDlg 对话框




CPhotoSyncDlg::CPhotoSyncDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPhotoSyncDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPhotoSyncDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDITSRC, m_editSrc);
	DDX_Control(pDX, IDC_EDITDEST, m_editDest);
	DDX_Control(pDX, IDC_EDITBAKLIB, m_editBackup);
}

BEGIN_MESSAGE_MAP(CPhotoSyncDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTONSRC, &CPhotoSyncDlg::OnBnClickedButtonsrc)
	ON_BN_CLICKED(IDC_BUTTONDEST, &CPhotoSyncDlg::OnBnClickedButtondest)
	ON_BN_CLICKED(IDC_BUTTON1, &CPhotoSyncDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTONBACKUP, &CPhotoSyncDlg::OnBnClickedButtonbackup)
END_MESSAGE_MAP()


// CPhotoSyncDlg 消息处理程序

BOOL CPhotoSyncDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	m_editSrc.SetWindowText(TEXT("c:\\src"));
	m_editDest.SetWindowText(TEXT("c:\\dest"));
	m_editBackup.SetWindowText(TEXT("c:\\backup"));
	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPhotoSyncDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPhotoSyncDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPhotoSyncDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CPhotoSyncDlg::OnBnClickedButtonsrc()
{
	// TODO: 在此添加控件通知处理程序代码
	TCHAR szPath[MAX_PATH];     //存放选择的目录路径 
    CString str;

    ZeroMemory(szPath, sizeof(szPath));   

    BROWSEINFO bi;   
    bi.hwndOwner = m_hWnd;   
    bi.pidlRoot = NULL;   
    bi.pszDisplayName = szPath;   
    bi.lpszTitle = TEXT("请选择需要导入的目录：");   
    bi.ulFlags = 0;   
    bi.lpfn = NULL;   
    bi.lParam = 0;   
    bi.iImage = 0;   
    //弹出选择目录对话框
    LPITEMIDLIST lp = SHBrowseForFolder(&bi);   

    if(lp && SHGetPathFromIDList(lp, szPath))   
    {
        str.Format(TEXT("选择的目录为 %s"),  szPath);
        //AfxMessageBox(str); 
		m_editSrc.SetWindowText(szPath);
        
    }
    else   
        AfxMessageBox(TEXT("无效的目录，请重新选择"));   
}


void CPhotoSyncDlg::OnBnClickedButtondest()
{
	// TODO: 在此添加控件通知处理程序代码
	TCHAR szPath[MAX_PATH];     //存放选择的目录路径 
    CString str;

    ZeroMemory(szPath, sizeof(szPath));   

    BROWSEINFO bi;   
    bi.hwndOwner = m_hWnd;   
    bi.pidlRoot = NULL;   
    bi.pszDisplayName = szPath;   
    bi.lpszTitle = TEXT("请选择需要存入的目录：");   
    bi.ulFlags = 0;   
    bi.lpfn = NULL;   
    bi.lParam = 0;   
    bi.iImage = 0;   
    //弹出选择目录对话框
    LPITEMIDLIST lp = SHBrowseForFolder(&bi);   

    if(lp && SHGetPathFromIDList(lp, szPath))   
    {
        str.Format(TEXT("选择的目录为 %s"),  szPath);
        //AfxMessageBox(str); 
		m_editDest.SetWindowText(szPath);
    }
    else   
        AfxMessageBox(TEXT("无效的目录，请重新选择"));  
}

struct DestInfo
{
	CString m_srcFullName;
	CString m_destFullName;
	CString m_destFullDir;
};
int totalCopy = 0;
void convertDestInfo(TCHAR* srcPath,WIN32_FIND_DATA* findData,TCHAR * destPath,DestInfo* destInfo)
{
	SYSTEMTIME sysTime;
	FileTimeToSystemTime(&(findData->ftLastWriteTime),&sysTime);
	//fprintf(fp,"文件的创建时间      :%d年 %d月 %d日 星期%d, %d时 %d分 %d秒\n", sysTime.wYear,sysTime.wMonth,sysTime.wDay,sysTime.wDayOfWeek,sysTime.wHour+8,sysTime.wMinute,sysTime.wSecond); 
	//destInfo->m_destDir = destPath;
	//destInfo->m_destDir += TEXT("/");
	TCHAR szTmp[MAX_PATH];
	_stprintf(szTmp,TEXT("%s\\%d年\\%04d%02d%02d\\%s"),destPath,sysTime.wYear,sysTime.wYear,sysTime.wMonth,sysTime.wDay,findData->cFileName);
	destInfo->m_destFullName = szTmp;

	_stprintf(szTmp,TEXT("%s\\%d年\\%04d%02d%02d"),destPath,sysTime.wYear,sysTime.wYear,sysTime.wMonth,sysTime.wDay);
	destInfo->m_destFullDir = szTmp;

	_stprintf(szTmp,TEXT("%s\\%s"),srcPath,findData->cFileName);
	destInfo->m_srcFullName = szTmp;
}
BOOL DirectoryExist(CString Path)
{
	WIN32_FIND_DATA fd;
	BOOL ret = FALSE;
    HANDLE hFind = FindFirstFile(Path, &fd);
    if ((hFind != INVALID_HANDLE_VALUE) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {  //目录存在
        ret = TRUE;    
    }
	FindClose(hFind);
	return ret;
}
BOOL CreateFolder(CString strPath)
{
    SECURITY_ATTRIBUTES attrib;
    attrib.bInheritHandle = FALSE;
    attrib.lpSecurityDescriptor = NULL;
    attrib.nLength = sizeof(SECURITY_ATTRIBUTES);
    //上面定义的属性可以省略
    //直接使用return ::CreateDirectory(path, NULL);即可
    return ::CreateDirectory(strPath, &attrib);
}
void CreateAllDirectory(CString& Dir)
{
	if(DirectoryExist(Dir))
		return;

	int pos = Dir.ReverseFind(L'\\');
	if(pos == 0)
		return;

	if(pos < 0)
	{
		if(!DirectoryExist(Dir))
		{
			CreateFolder(Dir);
		}
		return;
	}

	if(pos == 0)
		return;

	
	CString parentDir = Dir.Left(pos);
	CreateAllDirectory(parentDir);

	CreateFolder(Dir);
}


void find(TCHAR * srcPath,TCHAR* destPath)
{
	TCHAR szFind[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	_tcscpy(szFind,srcPath);
	_tcscat(szFind,TEXT("/*.*"));
	HANDLE hFind=::FindFirstFile(szFind,&FindFileData);
	if(INVALID_HANDLE_VALUE == hFind)
		return;
	while(TRUE)
	{
		if(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			if(FindFileData.cFileName[0]!='.')
			{
				TCHAR szFile[MAX_PATH];
				_tcscpy(szFile,srcPath);
				_tcscat(szFile,TEXT("/"));
				_tcscat(szFile,FindFileData.cFileName);
				find(szFile,destPath);
			}
		}
		else
		{
			//cout << FindFileData.cFileName;
			DestInfo destInfo;
			convertDestInfo(srcPath,&FindFileData, destPath, &destInfo);
			if(!DirectoryExist(destInfo.m_destFullDir))
			{
				CreateAllDirectory(destInfo.m_destFullDir);
			}
			CopyFile( destInfo.m_srcFullName,destInfo.m_destFullName,FALSE);
			totalCopy++;
		}
		if(!FindNextFile(hFind,&FindFileData))
			break;
	}
	FindClose(hFind);
}

void CPhotoSyncDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	totalCopy = 0;
	TCHAR szFind[MAX_PATH];
	m_editSrc.GetWindowText(szFind,MAX_PATH);
	TCHAR szDest[MAX_PATH];
	m_editDest.GetWindowText(szDest,MAX_PATH);
	find(szFind,szDest);

	TCHAR szResult[MAX_PATH];
	_stprintf(szResult,TEXT("完成 %d！"),totalCopy);
	AfxMessageBox(szResult);
}

void CPhotoSyncDlg::BrowseCurrentDir(CString strDir)
{
	CFileFind finder;
	CString strPath;
	BOOL bWorking = finder.FindFile(strDir);
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		strPath=finder.GetFilePath();
		//strPath就是所要获取Test目录下的文件夹和文件（包括路径）
	}
	finder.Close();
}
void CPhotoSyncDlg::BrowseCurrentAllFile(CString strDir)
{
    if(strDir == _T("")) 
    { 
		return; 
	}
	else
	{
		if(strDir.Right(1) != _T("//"))
			strDir += L"//";
		strDir =strDir+_T("*.*");
	}
	CFileFind finder;
	CString strPath;
	BOOL bWorking = finder.FindFile(strDir);
	while(bWorking)
	{
		bWorking = finder.FindNextFile();
		strPath = finder.GetFilePath();
		if(finder.IsDirectory() && !finder.IsDots())
			BrowseCurrentAllFile(strPath); //递归调用
		else if(!finder.IsDirectory() && !finder.IsDots())
		{
			//strPaht就是所要获取的文件路径
		}
	} 
}


void CPhotoSyncDlg::OnBnClickedButtonbackup()
{
	// TODO: 
	TCHAR szDest[MAX_PATH];
	m_editDest.GetWindowText(szDest,MAX_PATH);
	CString strDest = szDest;
	BrowseCurrentAllFile(strDest);
}
