// PoolListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "PoolMonEx.h"
#include "PoolListCtrl.h"

#pragma comment(lib, "ntdll")

// CPoolListCtrl

IMPLEMENT_DYNAMIC(CPoolListCtrl, CMFCListCtrl)

CPoolListCtrl::CPoolListCtrl()
{
	m_pPoolInfo = (SYSTEM_POOLTAG_INFORMATION*)::malloc(m_PoolInfoSize);
}

CPoolListCtrl::~CPoolListCtrl()
{
	::free(m_pPoolInfo);
}


BEGIN_MESSAGE_MAP(CPoolListCtrl, CMFCListCtrl)
	ON_WM_CREATE()
	ON_NOTIFY(HDN_ITEMCLICKA, 0, &CPoolListCtrl::OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, &CPoolListCtrl::OnHdnItemclick)
	ON_WM_TIMER()
	ON_COMMAND(ID_VIEW_REFRESH, &CPoolListCtrl::OnViewRefresh)
	ON_COMMAND_RANGE(ID_REFRESHINTERVAL_5SECONDS, ID_REFRESHINTERVAL_5SECONDS + 3, OnIntervalChange)
	ON_UPDATE_COMMAND_UI_RANGE(ID_REFRESHINTERVAL_5SECONDS, ID_REFRESHINTERVAL_5SECONDS + 3, OnIntervalChangeUpdateUI)
	ON_COMMAND(ID_VIEW_DYNAMICSORT, &CPoolListCtrl::OnViewDynamicsort)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DYNAMICSORT, &CPoolListCtrl::OnUpdateViewDynamicsort)
END_MESSAGE_MAP()


int CPoolListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Images.Create(16, 16, ILC_COLOR32, 4, 2);
	m_Images.Add(AfxGetApp()->LoadIconW(IDI_PAGED));
	m_Images.Add(AfxGetApp()->LoadIconW(IDI_NONPAGED));

	SetImageList(&m_Images, LVSIL_SMALL);

	//EnableMarkSortedColumn(FALSE);

	CFont font;
	font.CreatePointFont(100, L"Consolas");

	LOGFONT lf;
	GetFont()->GetLogFont(&lf);

	SetFont(&font);
	font.Detach();

	font.DeleteObject();
	font.CreateFontIndirectW(&lf);
	GetHeaderCtrl().SetFont(&font);
	font.Detach();

	InsertColumn(0, L"Tag", LVCFMT_CENTER, 80);
	InsertColumn(1, L"Type", LVCFMT_CENTER, 80);
	InsertColumn(2, L"Allocs", LVCFMT_RIGHT, 100);
	InsertColumn(3, L"Frees", LVCFMT_RIGHT, 100);
	InsertColumn(4, L"Diff", LVCFMT_RIGHT, 80);
	InsertColumn(5, L"Bytes", LVCFMT_RIGHT, 100);
	InsertColumn(6, L"KBytes", LVCFMT_RIGHT, 100);
	InsertColumn(7, L"B / Alloc", LVCFMT_RIGHT, 80);

	BuildPoolData();

	SetTimer(1, m_Interval, nullptr);

	return 0;
}

void CPoolListCtrl::BuildPoolData()
{
	CWaitCursor wait;

	ULONG size;
	auto status = ::NtQuerySystemInformation(SystemInformationClass::SystemPoolTagInformation, m_pPoolInfo, m_PoolInfoSize, &size);
	if (status == 0) {
		char tag[5] = { 0 };
		int count = m_pPoolInfo->Count;
		m_Tags.clear();

		for (auto i = 0; i < count; i++) {
			const auto& item = m_pPoolInfo->TagInfo[i];
			::CopyMemory(tag, &item.Tag, 4);

			if (item.NonPagedAllocs > 0) {
				AddItem(i, item, tag, true);
			}
			if (item.PagedAllocs > 0) {
				AddItem(i, item, tag, false);
			}

		}
	}
}

void CPoolListCtrl::UpdateItem(int n, const SYSTEM_POOLTAG& item, bool nonpaged)
{
	SetItemText(n, ColumnType::PoolType, nonpaged ? L"Non Paged" : L"Paged");
	CString value;
	value.Format(L"%u", nonpaged ? item.NonPagedAllocs : item.PagedAllocs);
	SetItemText(n, ColumnType::Allocs, value);

	value.Format(L"%u", nonpaged ? item.NonPagedFrees : item.PagedFrees);
	SetItemText(n, ColumnType::Frees, value);

	int diff;
	value.Format(L"%u", nonpaged ? diff = item.NonPagedAllocs - item.NonPagedFrees : diff = item.PagedAllocs - item.PagedFrees);
	SetItemText(n, ColumnType::Diff, value);

	auto used = nonpaged ? item.NonPagedUsed : item.PagedUsed;
	value.Format(L"%u", used);
	SetItemText(n, ColumnType::Usage, value);

	value.Format(L"%u KB", (used + 512) >> 10);
	SetItemText(n, ColumnType::UsageKB, value);

	value.Format(L"%u", diff == 0 ? 0 : used / diff);
	SetItemText(n, ColumnType::PerAlloc, value);
}

void CPoolListCtrl::AddColor(int row, int col, COLORREF color)
{
	m_CellColors.insert(std::make_pair(std::make_pair(row, col), color));
}

bool CPoolListCtrl::ProcessChanges(int row, const SYSTEM_POOLTAG & newTag, const SYSTEM_POOLTAG & oldTag, bool nonpaged)
{
	if (::memcmp(&newTag, &oldTag, sizeof(newTag)) == 0)
		return false;

	const COLORREF PlusColor = Green, MinusColor = Red;

	if (nonpaged) {
		if (newTag.NonPagedAllocs > oldTag.NonPagedAllocs)
			AddColor(row, ColumnType::Allocs, PlusColor);
		if (newTag.NonPagedFrees > oldTag.NonPagedFrees)
			AddColor(row, ColumnType::Frees, PlusColor);
		if (newTag.NonPagedUsed > oldTag.NonPagedUsed) {
			AddColor(row, ColumnType::Usage, PlusColor);
			AddColor(row, ColumnType::UsageKB, PlusColor);
		}
		else if (newTag.NonPagedUsed < oldTag.NonPagedUsed) {
			AddColor(row, ColumnType::Usage, MinusColor);
			AddColor(row, ColumnType::UsageKB, MinusColor);
		}
		if (newTag.NonPagedAllocs - newTag.NonPagedFrees > oldTag.NonPagedAllocs - oldTag.NonPagedFrees)
			AddColor(row, ColumnType::Diff, PlusColor);
		else if(newTag.NonPagedAllocs - newTag.NonPagedFrees < oldTag.NonPagedAllocs - oldTag.NonPagedFrees)
			AddColor(row, ColumnType::Diff, MinusColor);
	}
	else {
		if (newTag.PagedAllocs > oldTag.PagedAllocs)
			AddColor(row, ColumnType::Allocs, PlusColor);
		if (newTag.PagedFrees > oldTag.PagedFrees)
			AddColor(row, ColumnType::Frees, PlusColor);
		if (newTag.PagedUsed > oldTag.PagedUsed) {
			AddColor(row, ColumnType::Usage, PlusColor);
			AddColor(row, ColumnType::UsageKB, PlusColor);
		}
		else if (newTag.PagedUsed < oldTag.PagedUsed) {
			AddColor(row, ColumnType::Usage, MinusColor);
			AddColor(row, ColumnType::UsageKB, MinusColor);
		}
		if (newTag.PagedAllocs - newTag.PagedFrees > oldTag.PagedAllocs - oldTag.PagedFrees)
			AddColor(row, ColumnType::Diff, PlusColor);
		else if (newTag.PagedAllocs - newTag.PagedFrees < oldTag.PagedAllocs - oldTag.PagedFrees)
			AddColor(row, ColumnType::Diff, MinusColor);
	}
	return true;
}

int CPoolListCtrl::AddItem(int index, const SYSTEM_POOLTAG& item, PCSTR tag, bool nonpaged)
{
	int n = InsertItem(index, CString(tag), nonpaged ? 1 : 0);

	UpdateItem(n, item, nonpaged);

	ULONG key = nonpaged ? (item.TagUlong | NonPagedBit) : item.TagUlong;

	m_Tags.insert(std::make_pair(key, item));

	SetItemData(n, key);

	return n;
}

int CPoolListCtrl::OnCompareItems(LPARAM lParam1, LPARAM lParam2, int iColumn)
{
	ASSERT(m_Tags.find(lParam1) != m_Tags.end());
	ASSERT(m_Tags.find(lParam2) != m_Tags.end());

	const auto& item1 = m_Tags[lParam1];
	const auto& item2 = m_Tags[lParam2];

	auto nonPaged1 = ((ULONG)lParam1) & NonPagedBit;
	auto nonPaged2 = ((ULONG)lParam2) & NonPagedBit;

	long long order;

	switch (iColumn) {
	case ColumnType::TagName:
		order = (long long)ReverseTag(item1.TagUlong) - ReverseTag(item2.TagUlong);
		break;

	case ColumnType::PoolType:
		if (nonPaged1 == nonPaged2)
			return 0;

		order = nonPaged1 ? -1 : 1;
		break;

	case ColumnType::Allocs:
		order = (long long)(nonPaged1 ? item1.NonPagedAllocs : item1.PagedAllocs) - (nonPaged2 ? item2.NonPagedAllocs : item2.PagedAllocs);
		break;

	case ColumnType::Frees:
		order = (long long)(nonPaged1 ? item1.NonPagedFrees : item1.PagedFrees) - (nonPaged2 ? item2.NonPagedFrees : item2.PagedFrees);
		break;

	case ColumnType::Diff:
	{
		auto diff1 = (nonPaged1 ? item1.NonPagedAllocs : item1.PagedAllocs) - (nonPaged1 ? item1.NonPagedFrees : item1.PagedFrees);
		auto diff2 = (nonPaged2 ? item2.NonPagedAllocs : item2.PagedAllocs) - (nonPaged2 ? item2.NonPagedFrees : item2.PagedFrees);
		order = (long long)(diff1) - diff2;
		break;
	}

	case ColumnType::Usage:
	case ColumnType::UsageKB:
		order = (long long)(nonPaged1 ? item1.NonPagedUsed : item1.PagedUsed) - (nonPaged2 ? item2.NonPagedUsed : item2.PagedUsed);
		break;

	case ColumnType::PerAlloc:
		auto allocs1 = (nonPaged1 ? item1.NonPagedAllocs : item1.PagedAllocs);
		auto allocs2 = (nonPaged2 ? item2.NonPagedAllocs : item2.PagedAllocs);
		auto frees1 = (nonPaged1 ? item1.NonPagedFrees : item1.PagedFrees);
		auto frees2 = (nonPaged2 ? item2.NonPagedFrees : item2.PagedFrees);
		auto used1 = (nonPaged1 ? item1.NonPagedUsed : item1.PagedUsed);
		auto used2 = (nonPaged2 ? item2.NonPagedUsed : item2.PagedUsed);

		auto diff1 = allocs1 - frees1, diff2 = allocs2 - frees2;

		order = (long long)(diff1 ? used1 / diff1 : 0) - (diff2 ? used2 / diff2 : 0);
		break;
	}

	if (order == 0)
		return 0;

	return order > 0 ? 1 : -1;
}

ULONG CPoolListCtrl::ReverseTag(ULONG tagUlong)
{
	BYTE* tag = (BYTE*)&tagUlong;
	std::swap(tag[0], tag[3]);
	std::swap(tag[1], tag[2]);
	return tagUlong;
}

void CPoolListCtrl::OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);

	if (m_SortColumn != phdr->iItem) {
		m_SortColumn = phdr->iItem;
		m_Ascending = true;
	}
	else {
		m_Ascending = !m_Ascending;
	}

	Sort(m_SortColumn, m_Ascending ? TRUE : FALSE);

	*pResult = 0;
}

void CPoolListCtrl::RefreshPoolData()
{
	CWaitCursor wait;

	ULONG size;
	auto status = ::NtQuerySystemInformation(SystemInformationClass::SystemPoolTagInformation, m_pPoolInfo, m_PoolInfoSize, &size);
	if (status == 0) {
		char tag[5] = { 0 };
		int count = m_pPoolInfo->Count;
		LVFINDINFO lvfi = { LVFI_PARAM };

		int row;
		CRect rc;
		m_Update = true;
		for (const auto& cell : m_CellColors) {
			row = cell.first.first;
			RedrawItems(row, row);
		}
		m_Update = false;

		LockWindowUpdate();

		m_CellColors.clear();

		for (auto i = 0; i < count; i++) {
			const auto& item = m_pPoolInfo->TagInfo[i];
			::CopyMemory(tag, &item.Tag, 4);

			if (item.NonPagedAllocs > 0) {
				// find the tag (if exists)
				auto it = m_Tags.find(item.TagUlong | NonPagedBit);
				if (it == m_Tags.end())
					AddItem(GetItemCount(), item, tag, true);
				else {
					// find existing item
					lvfi.lParam = item.TagUlong | NonPagedBit;
					int n = FindItem(&lvfi);
					ASSERT(n >= 0);
					if (ProcessChanges(n, item, it->second, true)) {
						UpdateItem(n, item, true);
						m_Tags[lvfi.lParam] = item;
					}
				}
			}
			if (item.PagedAllocs > 0) {
				auto it = m_Tags.find(item.TagUlong);
				if (it == m_Tags.end())
					AddItem(GetItemCount(), item, tag, false);
				else {
					// find existing item
					lvfi.lParam = item.TagUlong;
					int n = FindItem(&lvfi);
					ASSERT(n >= 0);
					if (ProcessChanges(n, item, it->second, false)) {
						UpdateItem(n, item, false);
						m_Tags[item.TagUlong] = item;
					}
				}
			}
		}
		if (m_DynamicSort && m_SortColumn >= 0)
			Sort(m_SortColumn, m_Ascending);

		UnlockWindowUpdate();

	}
}

void CPoolListCtrl::Pause(bool pause)
{
	if (pause)
		KillTimer(1);
	else
		SetTimer(1, m_Interval, nullptr);
	m_Paused = pause;
}

void CPoolListCtrl::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		RefreshPoolData();
		TRACE(L"refresh: %d\n", ::GetTickCount());
	}
}

void CPoolListCtrl::OnViewRefresh()
{
	RefreshPoolData();
}

COLORREF CPoolListCtrl::OnGetCellBkColor(int row, int col)
{
	if (m_Update)
		return GetBkColor();

	auto it = m_CellColors.find(std::make_pair(row, col));
	return it == m_CellColors.end() ? GetBkColor() : it->second;
}

void CPoolListCtrl::OnIntervalChange(UINT id)
{
	m_IntervalIndex = id - ID_REFRESHINTERVAL_5SECONDS;

	const int intervals[] = { 5000, 10000, 30 * 1000, 60 * 1000 };

	if (!m_Paused)
		SetTimer(1, intervals[m_IntervalIndex], nullptr);
}

void CPoolListCtrl::OnIntervalChangeUpdateUI(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(pCmdUI->m_nID - ID_REFRESHINTERVAL_5SECONDS == m_IntervalIndex);
}


void CPoolListCtrl::OnViewDynamicsort()
{
	m_DynamicSort = !m_DynamicSort;
}


void CPoolListCtrl::OnUpdateViewDynamicsort(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_DynamicSort);
}
