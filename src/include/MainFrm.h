// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
//#include <windows.h>
//#include <psapi.h>
#include <initguid.h>
#include <devpkey.h>
#include <SetupAPI.h>
#pragma comment (lib,"SetupAPI.lib")
//#include <tchar.h>
//#include <stdio.h>
//#include <ATLComTime.h>
//#include <list>
#include <vector>
#include <set>
#include <string>

#include <atlmisc.h>
#include "toolbarHelper.h"


enum
{
	nColumnName,
	nColumnDevType,
	nColumnDriverVendor,
	nColumnDriverVersion,
	nColumnDriverDate,
};

struct CDevice
{
	CString		m_Name;
	CString		m_Type;
	CString		m_Vendor;
	CString		m_Version;
	FILETIME	m_Date = {0};
};
int CALLBACK ListViewCompareProc(LPARAM lparam1, LPARAM lparam2, LPARAM lParamSort);

//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------

class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>, 
	public CUpdateUI<CMainFrame>,
	public CMessageFilter, public CIdleHandler,
	public CToolBarHelper<CMainFrame>

{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	CView m_view;
	CCommandBarCtrl m_CmdBar;
	CComboBox m_ComboType;		//Combobox for device type
	CComboBox m_ComboSearch;

	std::vector<CDevice> m_Devices;
	//---------------------------------------------------------------

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE;

		return m_view.PreTranslateMessage(pMsg);
	}
	//---------------------------------------------------------------

	virtual BOOL OnIdle()
	{
		UIUpdateToolBar();
		return FALSE;
	}
	//---------------------------------------------------------------

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnLVColumnClick)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_CODE_HANDLER(CBN_EDITCHANGE, OnEditChange)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
		CHAIN_MSG_MAP(CToolBarHelper<CMainFrame>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
//---------------------------------------------------------------

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// create command bar window
		HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
		// attach menu
		m_CmdBar.AttachMenu(GetMenu());
		// load command bar images
		m_CmdBar.LoadImages(IDR_MAINFRAME);
		// remove old menu
		SetMenu(NULL);

		HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE_EX);

		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
		AddSimpleReBarBand(hWndCmdBar);
		AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

		AddToolbarButtonText(hWndToolBar, ID_APP_ABOUT, _T("About"));
		AddToolbarButtonText(hWndToolBar, ID_FILE_NEW, _T("Refresh"));
		m_ComboSearch	= CreateToolbarComboBox(hWndToolBar, ID_EDIT_SEARCH	,32,16, CBS_DROPDOWN);
		m_ComboType		= CreateToolbarComboBox(hWndToolBar, ID_TYPE		,16,16, CBS_DROPDOWNLIST);
		m_ComboType.AddString(_T("* All types"));

		//CreateSimpleStatusBar();

		m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);

		UIAddToolBar(hWndToolBar);
		UISetCheck(ID_VIEW_TOOLBAR, 1);
		UISetCheck(ID_VIEW_STATUS_BAR, 1);

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		m_view.AddColumn(_T("Device Name"), nColumnName);
		m_view.AddColumn(_T("Type"), nColumnDevType);
		m_view.AddColumn(_T("Vendor"), nColumnDriverVendor);
		m_view.AddColumn(_T("Version"), nColumnDriverVersion);
		m_view.AddColumn(_T("Date"), nColumnDriverDate);

		EnumerateDevices();

		return 0;
	}
	//---------------------------------------------------------------

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);

		bHandled = FALSE;
		return 1;
	}
	//---------------------------------------------------------------

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}
	//---------------------------------------------------------------

	LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EnumerateDevices();
		return 0;
	}
	//---------------------------------------------------------------

	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static BOOL bVisible = TRUE;	// initially visible
		bVisible = !bVisible;
		CReBarCtrl rebar = m_hWndToolBar;
		int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
		rebar.ShowBand(nBandIndex, bVisible);
		UISetCheck(ID_VIEW_TOOLBAR, bVisible);
		UpdateLayout();
		return 0;
	}
	//---------------------------------------------------------------

	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
		::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
		UpdateLayout();
		return 0;
	}
	//---------------------------------------------------------------

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}


	//---------------------------------------------------------------

	int EnumerateDevices()
	{
		int nItem = -1;
		// this enumerates only portable devices (usb). Not interesting.
		//#include <portabledeviceapi.h>
		//#pragma comment(lib,"PortableDeviceGuids.lib")
		//IPortableDeviceManager *pPortableDeviceManager;
		//HRESULT hr = CoCreateInstance(CLSID_PortableDeviceManager,
		//	NULL,
		//	CLSCTX_INPROC_SERVER,
		//	IID_PPV_ARGS(&pPortableDeviceManager));
		//if (FAILED(hr))	printf("! Failed to CoCreateInstance CLSID_PortableDeviceManager, hr = 0x%lx\n", hr);
		//DWORD cPnPDeviceIDs{ 0 };
		//if (SUCCEEDED(hr))
		//{
		//	hr = pPortableDeviceManager->GetDevices(NULL, &cPnPDeviceIDs);
		//	if (FAILED(hr)) printf("! Failed to get number of devices on the system, hr = 0x%lx\n", hr);
		//}
		
		//this method shows only dlls and syss. Not interesting.
		//#define ARRAY_SIZE 1024
		//LPVOID drivers[ARRAY_SIZE];
		//DWORD cbNeeded;
		//int cDrivers, i;
		//if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers))
		//{
		//	TCHAR szDriver[ARRAY_SIZE];
		//	cDrivers = cbNeeded / sizeof(drivers[0]);
		//	ATLTRACE(TEXT("There are %d drivers:\n"), cDrivers);
		//	for (i = 0; i < cDrivers; i++)
		//		if (GetDeviceDriverBaseName(drivers[i], szDriver, sizeof(szDriver) / sizeof(szDriver[0])))
		//			ATLTRACE(TEXT("%d: %s\n"), i + 1, szDriver);
		//}
		//else
		//{
		//	ATLTRACE(TEXT("EnumDeviceDrivers failed; array size needed is %d\n"), cbNeeded / sizeof(LPVOID));
		//	return 1;
		//}

		// имя устройства, его тип, производителя драйвера для данного устройства, версию и дату драйвера.
		m_Devices.clear();
		SP_CLASSIMAGELIST_DATA ClassImageListData{sizeof(SP_CLASSIMAGELIST_DATA),0};

		if (SetupDiGetClassImageList(&ClassImageListData))
		{
			HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(NULL,NULL,NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
			if (DeviceInfoSet == INVALID_HANDLE_VALUE)
				ATLTRACE("Error creating DeviceInfoSet");
			else
			{
				DWORD indexInInfoSet = -1;
				SP_DEVINFO_DATA DeviceInfoData{ sizeof(SP_DEVINFO_DATA),0};
				DEVPROPTYPE PropertyType = DEVPROP_TYPE_STRING | DEVPROP_TYPEMOD_LIST;
				const DWORD bufSize = 1024;
				byte buf[bufSize] = { 0 };
				DWORD requiredSize;
				while(SetupDiEnumDeviceInfo(DeviceInfoSet, ++indexInInfoSet, &DeviceInfoData))
				{
					bool result(false);
					CDevice Device;
					if (SetupDiGetDeviceProperty(DeviceInfoSet, &DeviceInfoData, &DEVPKEY_NAME,                  &PropertyType, buf, bufSize, &requiredSize, 0))
						result=true, Device.m_Name = (TCHAR*)buf;
					if (SetupDiGetDeviceProperty(DeviceInfoSet, &DeviceInfoData, &DEVPKEY_Device_Class,          &PropertyType, buf, bufSize, &requiredSize, 0))
						result = true, Device.m_Type = (TCHAR*)buf;
					if (SetupDiGetDeviceProperty(DeviceInfoSet, &DeviceInfoData, &DEVPKEY_Device_DriverProvider, &PropertyType, buf, bufSize, &requiredSize, 0))
						result = true, Device.m_Vendor = (TCHAR*)buf;
					if (SetupDiGetDeviceProperty(DeviceInfoSet, &DeviceInfoData, &DEVPKEY_Device_DriverVersion,  &PropertyType, buf, bufSize, &requiredSize, 0))
						result = true, Device.m_Version= (TCHAR*)buf;
					if (SetupDiGetDeviceProperty(DeviceInfoSet, &DeviceInfoData, &DEVPKEY_Device_DriverDate,     &PropertyType, buf, bufSize, &requiredSize, 0))
						result = true, Device.m_Date = *(FILETIME*)buf;
					if (result)
						m_Devices.push_back(Device);
				}
				{
					const auto error = GetLastError();
					if (error == ERROR_NO_MORE_ITEMS)
						ATLTRACE("No more Items");
				}
				SetupDiDestroyDeviceInfoList(DeviceInfoSet);
			}
			SetupDiDestroyClassImageList(&ClassImageListData);
		}
		FillTypesCombo();
		PutDevicesInList();
		return ++nItem;
	}
	//---------------------------------------------------------------

	void PutDevicesInList()
	{
		m_view.DeleteAllItems();                          //clear all items in list before creating the new list

      //retrieving type to add, if any
		int TypeIndex = m_ComboType.GetCurSel();
		CString szType;
		if (TypeIndex>0)
			m_ComboType.GetLBText(TypeIndex,szType);

      //retrieving search pattern to add, if any
		CString szSearch;
		m_ComboSearch.GetWindowText(szSearch);
		bool bSearch = !szSearch.IsEmpty();

		int nItem = -1;//the number of the last added item on the list
		const int bufSize = 254;
		TCHAR buf[bufSize];
		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.stateMask = 0;
		item.iSubItem = 0;
		item.state = 0;

		for (auto &Device : m_Devices)
		{
			if (TypeIndex > 0 && Device.m_Type.Compare(szType) != 0)
					continue;
			if (bSearch)
				if(Device.m_Name.Find(szSearch) < 0)
					continue;

			item.pszText = (LPWSTR)Device.m_Name.GetString();
			item.lParam = (LPARAM)&Device;
			item.iItem = ++nItem;

			m_view.InsertItem(&item);
			m_view.AddItem(nItem, 1, Device.m_Type.GetString());
			m_view.AddItem(nItem, 2, Device.m_Vendor.GetString());
			m_view.AddItem(nItem, 3, Device.m_Version.GetString());
			if (Device.m_Date.dwHighDateTime > 0)
			{
				SYSTEMTIME st;
				if (FileTimeToSystemTime(&Device.m_Date, &st))
				{
					GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, buf, bufSize);
					m_view.AddItem(nItem, 4, buf);
				}
			}
			else m_view.AddItem(nItem, 4, _T("-"));
		}
		m_view.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
		m_view.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
		m_view.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
		m_view.SetColumnWidth(3, LVSCW_AUTOSIZE_USEHEADER);
		m_view.SetColumnWidth(4, LVSCW_AUTOSIZE);

      CSize size = m_view.ApproximateViewRect();
      m_view.SetWindowPos(HWND_NOTOPMOST, 0, 0, size.cx,size.cy, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE);
		
	};
	//---------------------------------------------------------------

	void FillTypesCombo()
	{
		while (m_ComboType.GetCount()>1) m_ComboType.DeleteString(1);//delete all strings

		std::set<CString> types;
		for (auto &Device : m_Devices)
			types.insert(Device.m_Type);

		for (auto s : types)
			m_ComboType.AddString(s);

		m_ComboType.SetCurSel(0);
	}
	//---------------------------------------------------------------

	struct CSortData
	{
		int		m_nColumn=0;
		bool	m_bReverse=false;
	}  m_Sort;

	//---------------------------------------------------------------


	LRESULT OnLVColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		LPNMLISTVIEW lpLV = (LPNMLISTVIEW)pnmh;
		if (m_Sort.m_nColumn == lpLV->iSubItem)
			m_Sort.m_bReverse = !m_Sort.m_bReverse;
		else
			m_Sort.m_nColumn = lpLV->iSubItem, m_Sort.m_bReverse = false;

		ATLASSERT(m_Sort.m_nColumn >= 0 && m_Sort.m_nColumn <= 4);
		m_view.SortItems(ListViewCompareProc, (LPARAM)&m_Sort);
		return 0;
	}
	//---------------------------------------------------------------

	void OnToolBarCombo(HWND hWndCombo, UINT nID, int nSel, LPCTSTR lpszText, DWORD dwItemData)
	{
		PutDevicesInList();
	}

	//---------------------------------------------------------------
	LRESULT OnEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PutDevicesInList();
		return 0;
	}
};
//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------


int CALLBACK ListViewCompareProc(LPARAM lparam1, LPARAM lparam2, LPARAM lParamSort)
{
	ATLASSERT(lParamSort != NULL);

	if (lparam1 == 0)
	{
		if (lparam2 == 0)
			return 0;
		else
			return -1;
	}
	if (lparam2 == 0)
		return -1;

	CDevice* pDevice1 = reinterpret_cast<CDevice*>(lparam1);
	CDevice* pDevice2 = reinterpret_cast<CDevice*>(lparam2);
	CMainFrame::CSortData *sort = reinterpret_cast<CMainFrame::CSortData*>(lParamSort);

	int result = 0;
	switch (sort->m_nColumn)
	{
	case nColumnName			:result = pDevice1->m_Name.CompareNoCase	(pDevice2->m_Name); break;
	case nColumnDevType			:result = pDevice1->m_Type.CompareNoCase	(pDevice2->m_Type); break;
	case nColumnDriverVendor	:result = pDevice1->m_Vendor.CompareNoCase	(pDevice2->m_Vendor); break;
	case nColumnDriverVersion	:result = pDevice1->m_Version.CompareNoCase	(pDevice2->m_Version); break;
	case nColumnDriverDate		:
	{
		long long & d1 = *(long long*)&pDevice1->m_Date;
		long long & d2 = *(long long*)&pDevice2->m_Date;
		if (d1 < d2) result = -1;
		else if (d1 > d2) result = 1;
			else result = 0;
		break;
	}
	}
	;
	//HRESULT hr = 0;
	//if (pSD->bReverseSort)
	//	hr = lplvid1->spParentFolder->CompareIDs(0, lplvid2->lpi, lplvid1->lpi);
	//else
	//	hr = lplvid1->spParentFolder->CompareIDs(0, lplvid1->lpi, lplvid2->lpi);

	//return (int)(short)HRESULT_CODE(hr);
	return sort->m_bReverse ? -result : result;
}
//---------------------------------------------------------------
