
// PhotoSyncDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PhotoSync.h"
#include "PhotoSyncDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CPhotoSyncDlg �Ի���




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


// CPhotoSyncDlg ��Ϣ�������

BOOL CPhotoSyncDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	m_editSrc.SetWindowText(TEXT("c:\\src"));
	m_editDest.SetWindowText(TEXT("c:\\dest"));
	m_editBackup.SetWindowText(TEXT("c:\\backup"));
	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CPhotoSyncDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CPhotoSyncDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CPhotoSyncDlg::OnBnClickedButtonsrc()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	TCHAR szPath[MAX_PATH];     //���ѡ���Ŀ¼·�� 
    CString str;

    ZeroMemory(szPath, sizeof(szPath));   

    BROWSEINFO bi;   
    bi.hwndOwner = m_hWnd;   
    bi.pidlRoot = NULL;   
    bi.pszDisplayName = szPath;   
    bi.lpszTitle = TEXT("��ѡ����Ҫ�����Ŀ¼��");   
    bi.ulFlags = 0;   
    bi.lpfn = NULL;   
    bi.lParam = 0;   
    bi.iImage = 0;   
    //����ѡ��Ŀ¼�Ի���
    LPITEMIDLIST lp = SHBrowseForFolder(&bi);   

    if(lp && SHGetPathFromIDList(lp, szPath))   
    {
        str.Format(TEXT("ѡ���Ŀ¼Ϊ %s"),  szPath);
        //AfxMessageBox(str); 
		m_editSrc.SetWindowText(szPath);
        
    }
    else   
        AfxMessageBox(TEXT("��Ч��Ŀ¼��������ѡ��"));   
}


void CPhotoSyncDlg::OnBnClickedButtondest()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	TCHAR szPath[MAX_PATH];     //���ѡ���Ŀ¼·�� 
    CString str;

    ZeroMemory(szPath, sizeof(szPath));   

    BROWSEINFO bi;   
    bi.hwndOwner = m_hWnd;   
    bi.pidlRoot = NULL;   
    bi.pszDisplayName = szPath;   
    bi.lpszTitle = TEXT("��ѡ����Ҫ�����Ŀ¼��");   
    bi.ulFlags = 0;   
    bi.lpfn = NULL;   
    bi.lParam = 0;   
    bi.iImage = 0;   
    //����ѡ��Ŀ¼�Ի���
    LPITEMIDLIST lp = SHBrowseForFolder(&bi);   

    if(lp && SHGetPathFromIDList(lp, szPath))   
    {
        str.Format(TEXT("ѡ���Ŀ¼Ϊ %s"),  szPath);
        //AfxMessageBox(str); 
		m_editDest.SetWindowText(szPath);
    }
    else   
        AfxMessageBox(TEXT("��Ч��Ŀ¼��������ѡ��"));  
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
	//fprintf(fp,"�ļ��Ĵ���ʱ��      :%d�� %d�� %d�� ����%d, %dʱ %d�� %d��\n", sysTime.wYear,sysTime.wMonth,sysTime.wDay,sysTime.wDayOfWeek,sysTime.wHour+8,sysTime.wMinute,sysTime.wSecond); 
	//destInfo->m_destDir = destPath;
	//destInfo->m_destDir += TEXT("/");
	TCHAR szTmp[MAX_PATH];
	_stprintf(szTmp,TEXT("%s\\%d��\\%04d%02d%02d\\%s"),destPath,sysTime.wYear,sysTime.wYear,sysTime.wMonth,sysTime.wDay,findData->cFileName);
	destInfo->m_destFullName = szTmp;

	_stprintf(szTmp,TEXT("%s\\%d��\\%04d%02d%02d"),destPath,sysTime.wYear,sysTime.wYear,sysTime.wMonth,sysTime.wDay);
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
    {  //Ŀ¼����
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
    //���涨������Կ���ʡ��
    //ֱ��ʹ��return ::CreateDirectory(path, NULL);����
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	totalCopy = 0;
	TCHAR szFind[MAX_PATH];
	m_editSrc.GetWindowText(szFind,MAX_PATH);
	TCHAR szDest[MAX_PATH];
	m_editDest.GetWindowText(szDest,MAX_PATH);
	find(szFind,szDest);

	TCHAR szResult[MAX_PATH];
	_stprintf(szResult,TEXT("��� %d��"),totalCopy);
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
		//strPath������Ҫ��ȡTestĿ¼�µ��ļ��к��ļ�������·����
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
			BrowseCurrentAllFile(strPath); //�ݹ����
		else if(!finder.IsDirectory() && !finder.IsDots())
		{
			//strPaht������Ҫ��ȡ���ļ�·��
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
