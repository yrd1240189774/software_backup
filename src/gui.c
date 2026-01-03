// 包含头文件
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <shlobj.h>
#include <objbase.h>
#include "../include/main.h"

// 确保使用ANSI版本的API
#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#define ID_TAB_CONTROL 1001
#define ID_BACKUP_BUTTON 1002
#define ID_SELECT_PATH_BUTTON 1003
#define ID_REFRESH_BUTTON 1004
#define ID_DELETE_BUTTON 1005
#define ID_RESTORE_BUTTON 1006
#define ID_SET_BACKUP_PATH_BUTTON 1007
#define ID_SCHEDULE_BUTTON 1008
#define ID_FILTER_TABS 1009
#define ID_SELECT_TARGET_PATH_BUTTON 1010
#define ID_SELECT_RESTORE_PATH_BUTTON 1011

// 备份页面控件ID
#define ID_COMPRESS_CHECK 2001
#define ID_COMPRESS_HUFFMAN 2002
#define ID_COMPRESS_LZ77 2003
#define ID_ENCRYPT_CHECK 2004
#define ID_ENCRYPT_AES 2005
#define ID_ENCRYPT_KEY_EDIT 2006

// 筛选页面控件ID
#define ID_FILTER_REGULAR_FILE 3001
#define ID_FILTER_DIRECTORY 3002
#define ID_FILTER_SHORTCUT 3003
#define ID_FILTER_HIDDEN 3004
#define ID_FILTER_SYSTEM 3005
#define ID_FILTER_ALL 3006
#define ID_CANCEL_SCHEDULE_BUTTON 1011

// 函数声明
void update_schedule_status();
void update_cleanup_status();

// 全局变量
HWND hTab;
HWND hBackupPage;
HWND hManagePage;
HWND hSettingsPage;
HWND hSourcePathEdit;
HWND hTargetPathEdit;
HWND hBackupPathEdit; // 备份路径编辑框句柄（管理页面）
HWND hBackupList;
HWND hBackupFileEdit; // 备份文件编辑框句柄（管理页面）
HWND hRestorePathEdit; // 还原路径编辑框句柄（管理页面）
HFONT g_hFont; // 全局字体句柄
HWND hFilterTabs;
HWND hScheduleStatus; // 定时备份状态显示控件句柄
HWND hCleanupStatus; // 定时淘汰状态显示控件句柄
HWND hStartBackupBtn; // 启用定时备份按钮句柄
HWND hStopBackupBtn; // 禁用定时备份按钮句柄
HWND hStartCleanupBtn; // 启用定时淘汰按钮句柄
HWND hStopCleanupBtn; // 禁用定时淘汰按钮句柄
WNDPROC g_originalBackupPageProc; // 备份页面原来的窗口过程
WNDPROC g_originalManagePageProc; // 管理页面原来的窗口过程
WNDPROC g_originalSettingsPageProc; // 设置页面原来的窗口过程
int g_debug_mode = 0; // 调试模式标志

// 子页面句柄
HWND hFileTypePage;
HWND hTimePage;
HWND hSizePage;
HWND hExcludeUserPage;
HWND hExcludeFilenamePage;
HWND hExcludeDirPage;

// 备份页面控件句柄
HWND hPackZip;
HWND hPackTar;
HWND hCompressCheck;
HWND hCompressHuffman;
HWND hCompressLZ77;
HWND hEncryptCheck;
HWND hEncryptAES;
HWND hEncryptKeyEdit;
HWND hOverwriteCheck;

// 定时淘汰全局变量
// 定时淘汰是否正在运行，定义在cleanup.c中

// 字体设置回调函数
BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
{
    // 为子控件设置字体
    SendMessage(hwndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE; // 继续枚举
}

// 备份选项结构体
BackupOptions g_backup_options = {
    .source_path = "./test_source",
    .target_path = "./backup",
    .file_types = FILE_TYPE_REGULAR | FILE_TYPE_DIRECTORY,
    .pack_algorithm = PACK_ALGORITHM_TAR,
    .compress_algorithm = COMPRESS_ALGORITHM_NONE,
    .encrypt_enable = 0,
    .encrypt_algorithm = ENCRYPT_ALGORITHM_NONE,
    .overwrite = 1, // 默认勾选覆盖选项
    .size_range.min_size = 0,
    .size_range.max_size = 0xFFFFFFFF,
    // 初始化排除选项
    .exclude_usergroup.enable_user = 0,
    .exclude_usergroup.enable_group = 0,
    .exclude_usergroup.user_count = 0,
    .exclude_usergroup.group_count = 0,
    .exclude_filename.pattern[0] = '\0',
    .exclude_directory.count = 0,
    // 初始化时间范围
    .create_time_range.enable = 0,
    .modify_time_range.enable = 0,
    .access_time_range.enable = 0
};

// 密码对话框全局变量
char* g_password_buffer = NULL;
int g_password_max_len = 0;

// 声明函数
LRESULT CALLBACK BackupPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ManagePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SettingsPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateBackupPage(HWND hwnd);
void CreateManagePage(HWND hwnd);
void CreateSettingsPage(HWND hwnd);
void CreateFilterTabs(HWND hwnd);
void CreateFileTypePage(HWND hwnd);
void CreateTimePage(HWND hwnd);
void CreateSizePage(HWND hwnd);
void CreateExcludeUserPage(HWND hwnd);
void CreateExcludeFilenamePage(HWND hwnd);
void CreateExcludeDirPage(HWND hwnd);
void ShowFilterPage(int page_index);
BOOL ShowPasswordDialog(HWND hwnd, char* password, int max_len);
INT_PTR CALLBACK PasswordDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void LoadBackupFiles();

// 备份页面窗口过程
LRESULT CALLBACK BackupPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        // 处理备份页面上的控件消息
        switch (LOWORD(wParam))
        {
        case ID_SELECT_PATH_BUTTON:
        {
            // 选择文件夹对话框
            BROWSEINFO bi = { 0 };
            bi.hwndOwner = hwnd;
            bi.pszDisplayName = NULL;
            bi.lpszTitle = "请选择要备份的文件夹";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            // 显示文件夹选择对话框
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != NULL)
            {
                // 获取选择的文件夹路径
                char szPath[MAX_PATH] = { 0 };
                if (SHGetPathFromIDList(pidl, szPath))
                {
                    SetWindowText(hSourcePathEdit, szPath);
                }
                // 释放内存
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        case ID_COMPRESS_CHECK:
        {
            // 压缩复选框状态变化
            BOOL checked = SendMessage(hCompressCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            
            if (checked)
            {
                // 勾选：启用压缩算法选项
                EnableWindow(hCompressHuffman, TRUE);
                EnableWindow(hCompressLZ77, TRUE);
            }
            else
            {
                // 取消勾选：禁用压缩算法选项并清除选择状态
                EnableWindow(hCompressHuffman, FALSE);
                EnableWindow(hCompressLZ77, FALSE);
                // 清除选择状态
                SendMessage(hCompressHuffman, BM_SETCHECK, BST_UNCHECKED, 0);
                SendMessage(hCompressLZ77, BM_SETCHECK, BST_UNCHECKED, 0);
            }
            return 0;
        }
        case ID_ENCRYPT_CHECK:
        {
            // 加密复选框状态变化
            BOOL checked = SendMessage(hEncryptCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            
            if (checked)
            {
                // 勾选：启用加密算法选项和密钥输入框
                EnableWindow(hEncryptAES, TRUE);
                EnableWindow(hEncryptKeyEdit, TRUE);
            }
            else
            {
                // 取消勾选：禁用加密算法选项和密钥输入框，并清除选择状态
                EnableWindow(hEncryptAES, FALSE);
                EnableWindow(hEncryptKeyEdit, FALSE);
                // 清除选择状态
                SendMessage(hEncryptAES, BM_SETCHECK, BST_UNCHECKED, 0);
                // 清除密钥输入框内容
                SetWindowText(hEncryptKeyEdit, "");
            }
            return 0;
        }
        default:
            {
                // 将未处理的WM_COMMAND消息转发给主窗口处理
                // 先获取主窗口句柄（通过获取祖先窗口）
                HWND hMainWindow = GetAncestor(hwnd, GA_ROOT);
                if (hMainWindow)
                {
                    return SendMessage(hMainWindow, uMsg, wParam, lParam);
                }
                return 0;
            }
        }
    }
    case WM_NOTIFY:
    {
        NMHDR *pnmh = (NMHDR *)lParam;
        if (pnmh->hwndFrom == hBackupList && pnmh->code == LVN_ITEMCHANGED)
        {
            NMLISTVIEW *pnmlv = (NMLISTVIEW *)lParam;
            if (pnmlv->uChanged & LVIF_STATE && (pnmlv->uNewState & LVIS_SELECTED))
            {
                // 获取选中的文件名
                char backup_file[MAX_PATH] = {0};
                ListView_GetItemText(hBackupList, pnmlv->iItem, 0, backup_file, sizeof(backup_file));
                // 设置到备份文件编辑框
                SetWindowText(hBackupFileEdit, backup_file);
            }
            return 0;
        }
        
        // 将WM_NOTIFY消息转发给父窗口（主窗口）处理
        HWND hParent = GetParent(hwnd);
        if (hParent)
        {
            // 使用SendMessage将WM_NOTIFY消息转发给父窗口，并返回结果
            return SendMessage(hParent, uMsg, wParam, lParam);
        }
        return 0;
    }
    default:
        // 其他消息调用原来的窗口过程处理
        return CallWindowProc(g_originalBackupPageProc, hwnd, uMsg, wParam, lParam);
    }
}

// 管理页面窗口过程
LRESULT CALLBACK ManagePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        // 处理管理页面上的控件消息
        switch (LOWORD(wParam))
        {
        case ID_SELECT_PATH_BUTTON:
        {
            // 选择文件夹对话框
            BROWSEINFO bi = { 0 };
            bi.hwndOwner = hwnd;
            bi.pszDisplayName = NULL;
            bi.lpszTitle = "请选择备份目录";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            // 显示文件夹选择对话框
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != NULL)
            {
                // 获取选择的文件夹路径
                char szPath[MAX_PATH] = { 0 };
                if (SHGetPathFromIDList(pidl, szPath))
                {
                    // 更新备份路径编辑框
                    SetWindowText(hBackupPathEdit, szPath);
                    // 加载该目录下的备份文件
                    LoadBackupFiles();
                }
                // 释放内存
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        case ID_DELETE_BUTTON:
        {
            // 删除选中的备份
            MessageBox(hwnd, "删除功能待实现", "提示", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        case ID_REFRESH_BUTTON:
        {
            // 刷新备份列表
            LoadBackupFiles();
            return 0;
        }
        case ID_RESTORE_BUTTON:
        {
            // 还原选中的备份
            // 获取选中的项
            int selected = ListView_GetNextItem(hBackupList, -1, LVNI_SELECTED);
            if (selected == -1)
            {
                MessageBox(hwnd, "请先选择要还原的备份文件", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            
            // 获取选中的文件名
            char backup_file[MAX_PATH] = {0};
            ListView_GetItemText(hBackupList, selected, 0, backup_file, sizeof(backup_file));
            
            // 获取备份路径
            char backup_path[MAX_PATH] = {0};
            GetWindowText(hBackupPathEdit, backup_path, sizeof(backup_path));
            if (backup_path[0] == '\0') {
                strcpy(backup_path, ".\backup");
            }
            
            // 构建完整的备份文件路径
            char full_backup_path[MAX_PATH] = {0};
            sprintf(full_backup_path, "%s\\%s", backup_path, backup_file);
            
            // 获取还原路径
            char restore_path[MAX_PATH] = {0};
            GetWindowText(hRestorePathEdit, restore_path, sizeof(restore_path));
            if (restore_path[0] == '\0') {
                // 显示文件夹选择对话框
                BROWSEINFO bi = { 0 };
                bi.hwndOwner = hwnd;
                bi.pszDisplayName = NULL;
                bi.lpszTitle = "请选择还原目标路径";
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                
                LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
                if (pidl == NULL) {
                    // 用户取消了选择
                    return 0;
                }
                
                if (!SHGetPathFromIDList(pidl, restore_path)) {
                    CoTaskMemFree(pidl);
                    MessageBox(hwnd, "获取还原路径失败", "错误", MB_OK | MB_ICONERROR);
                    return 0;
                }
                CoTaskMemFree(pidl);
                
                // 更新还原路径编辑框
                SetWindowText(hRestorePathEdit, restore_path);
            }
            
            // 收集还原选项
            RestoreOptions restore_options = {0};
            
            // 设置备份文件路径
            strcpy(restore_options.backup_file, full_backup_path);
            
            // 设置还原目标路径
            strcpy(restore_options.target_path, restore_path);
            
            // 检查是否启用加密
            BOOL encrypt_enable = SendMessage(GetDlgItem(hwnd, 1020), BM_GETCHECK, 0, 0) == BST_CHECKED;
            restore_options.encrypt_enable = encrypt_enable;
            
            if (encrypt_enable) {
                // 获取加密算法
                BOOL aes_checked = SendMessage(GetDlgItem(hwnd, 1021), BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (aes_checked) {
                    restore_options.encrypt_algorithm = ENCRYPT_ALGORITHM_AES;
                } else {
                    restore_options.encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
                }
                
                // 获取密钥
                GetWindowText(GetDlgItem(hwnd, 1022), restore_options.encrypt_key, sizeof(restore_options.encrypt_key));
            } else {
                restore_options.encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
                restore_options.encrypt_key[0] = '\0';
            }
            
            // 显示还原信息
            char msg[512] = {0};
            if (encrypt_enable) {
                sprintf(msg, "开始还原加密备份：%s\n还原到：%s\n加密算法：%s\n密钥：%s", 
                       full_backup_path, restore_path, 
                       (restore_options.encrypt_algorithm == ENCRYPT_ALGORITHM_AES) ? "AES" : "None", 
                       restore_options.encrypt_key);
            } else {
                sprintf(msg, "开始还原备份：%s\n还原到：%s", full_backup_path, restore_path);
            }
            MessageBox(hwnd, msg, "还原信息", MB_OK | MB_ICONINFORMATION);
            
            // 调用真正的还原函数
            BackupResult restore_result = restore_data(&restore_options);
            
            // 显示还原结果
            char result_msg[512] = {0};
            sprintf(result_msg, "=== 还原结果 ===\n结果代码：%d\n结果含义：", restore_result);
            
            switch (restore_result)
            {
            case BACKUP_SUCCESS:
                strcat(result_msg, "还原成功！");
                MessageBox(hwnd, result_msg, "还原成功", MB_OK | MB_ICONINFORMATION);
                break;
            case BACKUP_ERROR_PARAM:
                strcat(result_msg, "还原失败：参数错误");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_PATH:
                strcat(result_msg, "还原失败：路径错误");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_FILE:
                strcat(result_msg, "还原失败：文件操作错误");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_MEMORY:
                strcat(result_msg, "还原失败：内存分配错误");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_COMPRESS:
                strcat(result_msg, "还原失败：压缩/解压错误");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_ENCRYPT:
                strcat(result_msg, "还原失败：加密/解密错误");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_PACK:
                strcat(result_msg, "还原失败：打包/解包错误");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_NO_FILES:
                strcat(result_msg, "还原失败：没有找到要还原的文件");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            default:
                strcat(result_msg, "还原失败：未知错误");
                MessageBox(hwnd, result_msg, "还原失败", MB_OK | MB_ICONERROR);
                break;
            }
            return 0;
        }
        case ID_SET_BACKUP_PATH_BUTTON:
        {
            // 设置备份路径，使用文件夹选择对话框
            BROWSEINFO bi = { 0 };
            bi.hwndOwner = hwnd;
            bi.pszDisplayName = NULL;
            bi.lpszTitle = "请选择备份路径";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            // 显示文件夹选择对话框
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != NULL)
            {
                // 获取选择的文件夹路径
                char szPath[MAX_PATH] = { 0 };
                if (SHGetPathFromIDList(pidl, szPath))
                {
                    // 更新编辑框内容
                    SetWindowText(hBackupPathEdit, szPath);
                    // 刷新备份列表
                    LoadBackupFiles();
                }
                // 释放内存
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        case ID_SELECT_RESTORE_PATH_BUTTON:
        {
            // 选择还原路径，使用文件夹选择对话框
            BROWSEINFO bi = { 0 };
            bi.hwndOwner = hwnd;
            bi.pszDisplayName = NULL;
            bi.lpszTitle = "请选择还原目标路径";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            // 显示文件夹选择对话框
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != NULL)
            {
                // 获取选择的文件夹路径
                char szPath[MAX_PATH] = { 0 };
                if (SHGetPathFromIDList(pidl, szPath))
                {
                    // 更新还原路径编辑框内容
                    SetWindowText(hRestorePathEdit, szPath);
                }
                // 释放内存
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        case 1020: // 加密备份复选框
        {
            // 加密复选框状态变化
            BOOL checked = SendMessage(GetDlgItem(hwnd, 1020), BM_GETCHECK, 0, 0) == BST_CHECKED;
            
            if (checked)
            {
                // 勾选：启用加密算法选项和密钥输入框
                EnableWindow(GetDlgItem(hwnd, 1021), TRUE);
                EnableWindow(GetDlgItem(hwnd, 1022), TRUE);
            }
            else
            {
                // 取消勾选：禁用加密算法选项和密钥输入框，并清除选择状态
                EnableWindow(GetDlgItem(hwnd, 1021), FALSE);
                EnableWindow(GetDlgItem(hwnd, 1022), FALSE);
                // 清除选择状态
                SendMessage(GetDlgItem(hwnd, 1021), BM_SETCHECK, BST_UNCHECKED, 0);
                // 清除密钥输入框内容
                SetWindowText(GetDlgItem(hwnd, 1022), "");
            }
            return 0;
        }
        default:
        {
            // 转发未处理的WM_COMMAND消息给主窗口
            HWND hMainWindow = GetAncestor(hwnd, GA_ROOT);
            if (hMainWindow)
            {
                return SendMessage(hMainWindow, uMsg, wParam, lParam);
            }
            return 0;
        }
        }
    }
    case WM_NOTIFY:
    {
        NMHDR *pnmh = (NMHDR *)lParam;
        if (pnmh->hwndFrom == hBackupList && pnmh->code == LVN_ITEMCHANGED)
        {
            NMLISTVIEW *pnmlv = (NMLISTVIEW *)lParam;
            if (pnmlv->uChanged & LVIF_STATE && (pnmlv->uNewState & LVIS_SELECTED))
            {
                // 获取选中的文件名并显示在编辑框中
                char backup_file[MAX_PATH] = {0};
                ListView_GetItemText(hBackupList, pnmlv->iItem, 0, backup_file, sizeof(backup_file));
                SetWindowText(hBackupFileEdit, backup_file);
            }
            return 0;
        }
        // 将其他WM_NOTIFY消息转发给父窗口（主窗口）处理
        HWND hParent = GetParent(hwnd);
        if (hParent)
        {
            // 使用SendMessage将WM_NOTIFY消息转发给父窗口，并返回结果
            return SendMessage(hParent, uMsg, wParam, lParam);
        }
        return 0;
    }
    default:
        // 其他消息调用原来的窗口过程处理
        return CallWindowProc(g_originalManagePageProc, hwnd, uMsg, wParam, lParam);
    }
}

// 设置页面窗口过程
LRESULT CALLBACK SettingsPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        // 处理设置页面上的控件消息
        switch (LOWORD(wParam))
        {
        case ID_SET_BACKUP_PATH_BUTTON:
        {
            // 设置备份路径，使用文件夹选择对话框
            BROWSEINFO bi = { 0 };
            bi.hwndOwner = hwnd;
            bi.pszDisplayName = NULL;
            bi.lpszTitle = "请选择备份路径";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            // 显示文件夹选择对话框
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != NULL)
            {
                // 获取选择的文件夹路径
                char szPath[MAX_PATH] = { 0 };
                if (SHGetPathFromIDList(pidl, szPath))
                {
                    // 更新编辑框内容
                    SetWindowText(hBackupPathEdit, szPath);
                }
                // 释放内存
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        case 1012: // 源路径浏览按钮
        {
            BROWSEINFO bi = { 0 };
            bi.hwndOwner = hwnd;
            bi.pszDisplayName = NULL;
            bi.lpszTitle = "请选择备份源路径";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != NULL)
            {
                char szPath[MAX_PATH] = { 0 };
                if (SHGetPathFromIDList(pidl, szPath))
                {
                    // 更新编辑框内容
                    HWND hScheduleSourceEdit = GetDlgItem(hwnd, 0);
                    SetWindowText(hScheduleSourceEdit, szPath);
                }
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        case 1013: // 目标路径浏览按钮
        {
            BROWSEINFO bi = { 0 };
            bi.hwndOwner = hwnd;
            bi.pszDisplayName = NULL;
            bi.lpszTitle = "请选择备份目标路径";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != NULL)
            {
                char szPath[MAX_PATH] = { 0 };
                if (SHGetPathFromIDList(pidl, szPath))
                {
                    // 更新编辑框内容
                    HWND hScheduleTargetEdit = GetDlgItem(hwnd, 0);
                    SetWindowText(hScheduleTargetEdit, szPath);
                }
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        case ID_SCHEDULE_BUTTON:
        {
            // 收集定时备份设置
            char interval_hours_str[10] = {0};
            char interval_minutes_str[10] = {0};
            char max_backups_str[10] = {0};
            char source_path[256] = {0};
            char target_path[256] = {0};
            char encrypt_key[64] = {0};
            
            // 获取UI控件句柄
            HWND hIntervalHoursEdit = GetDlgItem(hwnd, 0);
            HWND hIntervalMinutesEdit = GetDlgItem(hwnd, 0);
            HWND hMaxBackupsEdit = GetDlgItem(hwnd, 0);
            HWND hScheduleSourceEdit = GetDlgItem(hwnd, 0);
            HWND hScheduleTargetEdit = GetDlgItem(hwnd, 0);
            
            // 获取打包算法控件
            HWND hSchedulePackContainer = GetDlgItem(hwnd, 0);
            HWND hSchedulePackZip = GetWindow(hSchedulePackContainer, GW_CHILD);
            HWND hSchedulePackTar = GetNextWindow(hSchedulePackZip, GW_HWNDNEXT);
            
            // 获取压缩算法控件
            HWND hScheduleCompressContainer = GetDlgItem(hwnd, 0);
            HWND hScheduleCompressNone = GetWindow(hScheduleCompressContainer, GW_CHILD);
            HWND hScheduleCompressHaff = GetNextWindow(hScheduleCompressNone, GW_HWNDNEXT);
            HWND hScheduleCompressLz77 = GetNextWindow(hScheduleCompressHaff, GW_HWNDNEXT);
            
            // 获取加密选项控件
            HWND hScheduleEncryptCheck = GetDlgItem(hwnd, 0);
            HWND hScheduleEncryptKeyEdit = GetDlgItem(hwnd, 0);
            
            // 获取用户输入的路径
            GetWindowText(hScheduleSourceEdit, source_path, sizeof(source_path));
            GetWindowText(hScheduleTargetEdit, target_path, sizeof(target_path));
            
            // 将相对路径转换为绝对路径
            char source_abs[256];
            char target_abs[256];
            
            GetFullPathName(source_path, sizeof(source_abs), source_abs, NULL);
            GetFullPathName(target_path, sizeof(target_abs), target_abs, NULL);
            
            strcpy(source_path, source_abs);
            strcpy(target_path, target_abs);
            
            // 获取用户输入的时间间隔和保留备份数
            GetWindowText(hIntervalHoursEdit, interval_hours_str, sizeof(interval_hours_str));
            GetWindowText(hIntervalMinutesEdit, interval_minutes_str, sizeof(interval_minutes_str));
            GetWindowText(hMaxBackupsEdit, max_backups_str, sizeof(max_backups_str));
            
            // 转换为数值
            int interval_hours = atoi(interval_hours_str);
            int interval_minutes = atoi(interval_minutes_str);
            int max_backup_count = atoi(max_backups_str);
            
            // 设置默认值（如果用户未输入）
            if (source_path[0] == '\0') {
                strcpy(source_path, "./test_source");
            }
            if (target_path[0] == '\0') {
                strcpy(target_path, "./backup");
            }
            if (interval_hours < 0) {
                interval_hours = 0;
            }
            if (interval_minutes <= 0) {
                interval_minutes = 60;
            }
            if (max_backup_count <= 0) {
                max_backup_count = 5;
            }
            
            int pack_algorithm = PACK_ALGORITHM_TAR;
            
            // 获取打包算法
            if (SendMessage(hSchedulePackZip, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                pack_algorithm = PACK_ALGORITHM_ZIP;
            } else if (SendMessage(hSchedulePackTar, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                pack_algorithm = PACK_ALGORITHM_TAR;
            }
            
            // 获取压缩算法
            int compress_algorithm = COMPRESS_ALGORITHM_NONE;
            if (SendMessage(hScheduleCompressHaff, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                compress_algorithm = COMPRESS_ALGORITHM_HAFF;
            } else if (SendMessage(hScheduleCompressLz77, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                compress_algorithm = COMPRESS_ALGORITHM_LZ77;
            }
            
            // 获取覆盖选项
            int overwrite = 0;
            HWND hScheduleOverwriteCheck = GetDlgItem(hwnd, 0);
            if (hScheduleOverwriteCheck) {
                // 获取覆盖复选框的句柄（通过遍历查找）
                HWND hChild = GetWindow(hwnd, GW_CHILD);
                while (hChild) {
                    char class_name[256] = {0};
                    char window_text[256] = {0};
                    GetClassName(hChild, class_name, sizeof(class_name));
                    GetWindowText(hChild, window_text, sizeof(window_text));
                    if (strcmp(class_name, "Button") == 0 && strcmp(window_text, "覆盖现有备份文件") == 0) {
                        hScheduleOverwriteCheck = hChild;
                        break;
                    }
                    hChild = GetNextWindow(hChild, GW_HWNDNEXT);
                }
                
                if (SendMessage(hScheduleOverwriteCheck, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    overwrite = 1;
                }
            } else {
                // 如果找不到控件，默认覆盖
                overwrite = 1;
            }
            
            // 获取加密选项
            int encrypt_enable = 0;
            if (SendMessage(hScheduleEncryptCheck, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                encrypt_enable = 1;
                GetWindowText(hScheduleEncryptKeyEdit, encrypt_key, sizeof(encrypt_key));
            }
            
            // 创建定时备份选项
            ScheduledBackupOptions schedule_options;
            memset(&schedule_options, 0, sizeof(schedule_options));
            
            // 设置备份选项
            strcpy(schedule_options.backup_options.source_path, source_path);
            strcpy(schedule_options.backup_options.target_path, target_path);
            schedule_options.backup_options.file_types = FILE_TYPE_REGULAR | FILE_TYPE_DIRECTORY;
            schedule_options.backup_options.pack_algorithm = pack_algorithm;
            schedule_options.backup_options.compress_algorithm = compress_algorithm;
            schedule_options.backup_options.encrypt_enable = encrypt_enable;
            schedule_options.backup_options.encrypt_algorithm = ENCRYPT_ALGORITHM_AES;
            schedule_options.backup_options.overwrite = overwrite;
            if (encrypt_enable) {
                strncpy(schedule_options.backup_options.encrypt_key, encrypt_key, sizeof(schedule_options.backup_options.encrypt_key) - 1);
            }
            schedule_options.backup_options.size_range.min_size = 0;
            schedule_options.backup_options.size_range.max_size = 0xFFFFFFFF;
            
            // 设置定时配置
            schedule_options.schedule_config.enable = 1;
            schedule_options.schedule_config.interval_hours = interval_hours;
            schedule_options.schedule_config.interval_minutes = interval_minutes;
            schedule_options.schedule_config.last_run_time = time(NULL);
            schedule_options.schedule_config.next_run_time = time(NULL) + interval_hours * 3600 + interval_minutes * 60;
            
            // 设置清理配置
            schedule_options.cleanup_config.enable = 1;
            schedule_options.cleanup_config.max_backup_count = max_backup_count;
            
            // 在新线程中启动定时备份
            DWORD thread_id;
            HANDLE hThread = CreateThread(
                NULL,                   // 默认安全属性
                0,                      // 默认栈大小
                (LPTHREAD_START_ROUTINE)schedule_backup,
                &schedule_options,      // 线程参数
                0,                      // 立即运行
                &thread_id              // 返回线程ID
            );
            
            if (hThread) {
                MessageBox(hwnd, "定时备份已启用", "提示", MB_OK | MB_ICONINFORMATION);
                CloseHandle(hThread); // 关闭线程句柄，不等待线程结束
                // 更新状态显示
                update_schedule_status();
            } else {
                MessageBox(hwnd, "启动定时备份失败", "错误", MB_OK | MB_ICONERROR);
            }
            
            return 0;
        }
        case ID_CANCEL_SCHEDULE_BUTTON:
        {
            // 取消定时备份
            cancel_schedule_backup();
            MessageBox(hwnd, "定时备份已取消", "提示", MB_OK | MB_ICONINFORMATION);
            // 更新状态显示
            update_schedule_status();
            return 0;
        }
        case 1014: // 启用定时淘汰按钮
        {
            // 收集定时淘汰设置
            char keep_days_str[10] = {0};
            char max_backups_str[10] = {0};
            char backup_path[256] = {0};
            
            // 获取UI控件句柄
            HWND hKeepDaysEdit = GetDlgItem(hwnd, 0);
            HWND hMaxBackupsEdit = GetDlgItem(hwnd, 0);
            
            // 查找保留天数和最大备份数编辑框
            HWND hChild = GetWindow(hwnd, GW_CHILD);
            while (hChild)
            {
                char class_name[256] = {0};
                char window_text[256] = {0};
                GetClassName(hChild, class_name, sizeof(class_name));
                GetWindowText(hChild, window_text, sizeof(window_text));
                
                // 查找父控件的保留天数标签
                if (strcmp(class_name, "Static") == 0 && strcmp(window_text, "保留天数：") == 0)
                {
                    // 找到保留天数标签，下一个兄弟控件就是编辑框
                    hKeepDaysEdit = GetNextWindow(hChild, GW_HWNDNEXT);
                }
                // 查找父控件的最大备份数标签
                if (strcmp(class_name, "Static") == 0 && strcmp(window_text, "保留备份数：") == 0)
                {
                    // 找到保留备份数标签，下一个兄弟控件就是编辑框
                    hMaxBackupsEdit = GetNextWindow(hChild, GW_HWNDNEXT);
                }
                
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
            
            // 获取备份路径
            GetWindowText(hBackupPathEdit, backup_path, sizeof(backup_path));
            
            // 获取保留天数和最大备份数
            GetWindowText(hKeepDaysEdit, keep_days_str, sizeof(keep_days_str));
            GetWindowText(hMaxBackupsEdit, max_backups_str, sizeof(max_backups_str));
            
            // 转换为数值
            int keep_days = atoi(keep_days_str);
            int max_backups = atoi(max_backups_str);
            
            // 设置默认值
            if (keep_days <= 0)
            {
                keep_days = 7;
            }
            if (max_backups <= 0)
            {
                max_backups = 5;
            }
            if (backup_path[0] == '\0')
            {
                strcpy(backup_path, "./backup");
            }
            
            // 创建清理配置
            CleanupConfig cleanup_config;
            memset(&cleanup_config, 0, sizeof(cleanup_config));
            cleanup_config.enable = 1;
            cleanup_config.keep_days = keep_days;
            cleanup_config.max_backup_count = max_backups;
            
            // 在新线程中启动定时淘汰
            DWORD thread_id;
            HANDLE hThread = CreateThread(
                NULL,                   // 默认安全属性
                0,                      // 默认栈大小
                (LPTHREAD_START_ROUTINE)start_cleanup_service,
                &cleanup_config,        // 线程参数
                0,                      // 立即运行
                &thread_id              // 返回线程ID
            );
            
            if (hThread)
            {
                MessageBox(hwnd, "定时淘汰已启用", "提示", MB_OK | MB_ICONINFORMATION);
                CloseHandle(hThread); // 关闭线程句柄，不等待线程结束
                // 更新状态显示
                update_cleanup_status();
            } else {
                MessageBox(hwnd, "启动定时淘汰失败", "错误", MB_OK | MB_ICONERROR);
            }
            
            return 0;
        }
        case 1015: // 禁用定时淘汰按钮
        {
            // 取消定时淘汰
            stop_cleanup_service();
            MessageBox(hwnd, "定时淘汰已取消", "提示", MB_OK | MB_ICONINFORMATION);
            // 更新状态显示
            update_cleanup_status();
            return 0;
        }
        default:
        {
            // 转发未处理的WM_COMMAND消息给主窗口
            HWND hMainWindow = GetAncestor(hwnd, GA_ROOT);
            if (hMainWindow)
            {
                return SendMessage(hMainWindow, uMsg, wParam, lParam);
            }
            return 0;
        }
        }
    }
    case WM_NOTIFY:
    {
        // 将WM_NOTIFY消息转发给父窗口（主窗口）处理
        HWND hParent = GetParent(hwnd);
        if (hParent)
        {
            // 使用SendMessage将WM_NOTIFY消息转发给父窗口，并返回结果
            return SendMessage(hParent, uMsg, wParam, lParam);
        }
        return 0;
    }
    default:
        // 其他消息调用原来的窗口过程处理
        return CallWindowProc(g_originalSettingsPageProc, hwnd, uMsg, wParam, lParam);
    }
}

// 主窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        // 创建微软雅黑字体，进一步增大字体大小，提高可读性
        LOGFONT lf;
        memset(&lf, 0, sizeof(LOGFONT));
        lf.lfHeight = 18; // 进一步增大字体大小到18像素，使文字更加清晰
        lf.lfWidth = 0;
        lf.lfEscapement = 0;
        lf.lfOrientation = 0;
        lf.lfWeight = FW_NORMAL;
        lf.lfItalic = FALSE;
        lf.lfUnderline = FALSE;
        lf.lfStrikeOut = FALSE;
        lf.lfCharSet = GB2312_CHARSET;
        lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf.lfQuality = CLEARTYPE_QUALITY; // 使用ClearType抗锯齿，使字体更清晰
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
        strcpy(lf.lfFaceName, "Microsoft YaHei"); // 使用微软雅黑字体，更现代美观

        // 保存到全局变量
        g_hFont = CreateFontIndirect(&lf);

        // 创建标签控件，使用更现代的样式
        hTab = CreateWindowEx(WS_EX_CLIENTEDGE,
            WC_TABCONTROL,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_FLATBUTTONS | TCS_FOCUSONBUTTONDOWN,
            10,
            10,
            850,
            550,
            hwnd,
            (HMENU)ID_TAB_CONTROL,
            GetModuleHandle(NULL),
            NULL);

        // 设置标签控件的字体
        SendMessage(hTab, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessage(hTab, WM_FONTCHANGE, 0, 0);

        // 添加标签页
        TCITEM tie;
        tie.mask = TCIF_TEXT;
        
        char* tab_texts[] = {"备份", "管理备份数据", "备份设置"};
        for (int i = 0; i < 3; i++) {
            tie.pszText = tab_texts[i];
            TabCtrl_InsertItem(hTab, i, &tie);
        }

        // 创建页面，将主标签控件作为父窗口
        CreateBackupPage(hTab);
        CreateManagePage(hTab);
        CreateSettingsPage(hTab);

        // 为所有页面及其子控件设置字体
        SendMessage(hTab, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        EnumChildWindows(hTab, EnumChildProc, (LPARAM)g_hFont);
        
        SendMessage(hBackupPage, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        EnumChildWindows(hBackupPage, EnumChildProc, (LPARAM)g_hFont);
        
        SendMessage(hManagePage, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        EnumChildWindows(hManagePage, EnumChildProc, (LPARAM)g_hFont);
        
        SendMessage(hSettingsPage, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        EnumChildWindows(hSettingsPage, EnumChildProc, (LPARAM)g_hFont);

        // 显示第一个页面
        ShowWindow(hBackupPage, SW_SHOW);
        ShowWindow(hManagePage, SW_HIDE);
        ShowWindow(hSettingsPage, SW_HIDE);

        return 0;
    }
    case WM_NOTIFY:
    {
        NMHDR *pnmh = (NMHDR *)lParam;
        
        // 调试：检查WM_NOTIFY消息
        char debug_msg[256];
        sprintf(debug_msg, "WM_NOTIFY received: hwndFrom=%p, code=%d", pnmh->hwndFrom, pnmh->code);
        // MessageBox(NULL, debug_msg, "Debug", MB_OK);
        
        if (pnmh->code == TCN_SELCHANGE)
        {
            // 调试：检查TCN_SELCHANGE消息
            sprintf(debug_msg, "TCN_SELCHANGE received: hwndFrom=%p", pnmh->hwndFrom);
            // MessageBox(NULL, debug_msg, "Debug", MB_OK);
            
            if (pnmh->hwndFrom == hTab)
            {
                // 主标签页切换
                int sel = TabCtrl_GetCurSel(hTab);
                ShowWindow(hBackupPage, sel == 0 ? SW_SHOW : SW_HIDE);
                ShowWindow(hManagePage, sel == 1 ? SW_SHOW : SW_HIDE);
                ShowWindow(hSettingsPage, sel == 2 ? SW_SHOW : SW_HIDE);
                
                // 切换到管理页面时自动加载备份文件列表
                if (sel == 1) {
                    LoadBackupFiles();
                }
            }
            else if (pnmh->hwndFrom == hFilterTabs)
            {
                // 筛选子标签页切换
                int sel = TabCtrl_GetCurSel(hFilterTabs);
                sprintf(debug_msg, "Filter tab change: sel=%d", sel);
                // MessageBox(NULL, debug_msg, "Debug", MB_OK);
                ShowFilterPage(sel);
            }
        }
        return 0;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_BACKUP_BUTTON:
        {
            // 执行备份
            
            // 1. 收集源路径
            GetWindowText(hSourcePathEdit, g_backup_options.source_path, sizeof(g_backup_options.source_path));
            
            // 2. 使用设置页面的全局备份路径作为目标路径
            GetWindowText(hBackupPathEdit, g_backup_options.target_path, sizeof(g_backup_options.target_path));
            
            // 3. 将相对路径转换为绝对路径
            char source_abs[256];
            char target_abs[256];
            
            GetFullPathName(g_backup_options.source_path, sizeof(source_abs), source_abs, NULL);
            GetFullPathName(g_backup_options.target_path, sizeof(target_abs), target_abs, NULL);
            
            strcpy(g_backup_options.source_path, source_abs);
            strcpy(g_backup_options.target_path, target_abs);
            
            // 3. 收集打包算法
            // 检查哪个打包算法被选中
            BOOL zip_checked = SendMessage(hPackZip, BM_GETCHECK, 0, 0) == BST_CHECKED;
            BOOL tar_checked = SendMessage(hPackTar, BM_GETCHECK, 0, 0) == BST_CHECKED;
            
            if (zip_checked)
            {
                g_backup_options.pack_algorithm = PACK_ALGORITHM_ZIP;
            }
            else if (tar_checked)
            {
                g_backup_options.pack_algorithm = PACK_ALGORITHM_TAR;
            }
            else
            {
                // 默认使用tar算法
                g_backup_options.pack_algorithm = PACK_ALGORITHM_TAR;
            }
            
            // 4. 收集压缩选项
            BOOL compress_checked = SendMessage(hCompressCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            if (compress_checked)
            {
                // 检查哪个压缩算法被选中
                BOOL haff_checked = SendMessage(hCompressHuffman, BM_GETCHECK, 0, 0) == BST_CHECKED;
                BOOL lz77_checked = SendMessage(hCompressLZ77, BM_GETCHECK, 0, 0) == BST_CHECKED;
                
                if (haff_checked)
                {
                    g_backup_options.compress_algorithm = COMPRESS_ALGORITHM_HAFF;
                }
                else if (lz77_checked)
                {
                    g_backup_options.compress_algorithm = COMPRESS_ALGORITHM_LZ77;
                }
                else
                {
                    // 默认不压缩
                    g_backup_options.compress_algorithm = COMPRESS_ALGORITHM_NONE;
                }
            }
            else
            {
                g_backup_options.compress_algorithm = COMPRESS_ALGORITHM_NONE;
            }
            
            // 5. 收集覆盖选项
            BOOL overwrite_checked = SendMessage(hOverwriteCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            g_backup_options.overwrite = overwrite_checked ? 1 : 0;
            
            // 6. 收集加密选项
            BOOL encrypt_checked = SendMessage(hEncryptCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            g_backup_options.encrypt_enable = encrypt_checked;
            
            if (encrypt_checked)
            {
                // 检查加密算法
                BOOL aes_checked = SendMessage(hEncryptAES, BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (aes_checked)
                {
                    g_backup_options.encrypt_algorithm = ENCRYPT_ALGORITHM_AES;
                }
                else
                {
                    g_backup_options.encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
                }
                
                // 收集密钥
                GetWindowText(hEncryptKeyEdit, g_backup_options.encrypt_key, sizeof(g_backup_options.encrypt_key));
            }
            else
            {
                g_backup_options.encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
                g_backup_options.encrypt_key[0] = '\0';
            }
            
            // 6. 收集筛选选项
            // 6.1 收集文件类型筛选选项
            int file_types = 0;
            
            // 检查是否勾选了"所有文件"
            if (SendMessage(GetDlgItem(hFileTypePage, ID_FILTER_ALL), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                // 勾选了所有文件，设置所有文件类型
                file_types = FILE_TYPE_ALL;
            } else {
                // 基础文件类型选择
                if (SendMessage(GetDlgItem(hFileTypePage, ID_FILTER_REGULAR_FILE), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    file_types |= FILE_TYPE_REGULAR;
                }
                if (SendMessage(GetDlgItem(hFileTypePage, ID_FILTER_DIRECTORY), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    file_types |= FILE_TYPE_DIRECTORY;
                }
                if (SendMessage(GetDlgItem(hFileTypePage, ID_FILTER_SHORTCUT), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    file_types |= FILE_TYPE_SYMLINK;
                }
                // 特殊文件属性选择
                if (SendMessage(GetDlgItem(hFileTypePage, ID_FILTER_HIDDEN), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    file_types |= FILE_TYPE_HIDDEN;
                }
                if (SendMessage(GetDlgItem(hFileTypePage, ID_FILTER_SYSTEM), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    file_types |= FILE_TYPE_SYSTEM;
                }
            }
            
            // 更新全局备份选项的文件类型
            g_backup_options.file_types = file_types;
            
            // 6.2 调用备份函数
            
            // 调试：显示收集到的参数，使用更大的缓冲区
            char debug_msg[1024];
            sprintf(debug_msg, "=== 备份调试信息 ===\n" 
                   "1. 备份选项：\n" 
                   "   源路径：%s\n" 
                   "   目标路径：%s\n" 
                   "   打包算法：%d (0=tar, 1=zip)\n" 
                   "   压缩算法：%d (0=none, 1=haff, 2=lz77)\n" 
                   "   加密：%d (0=否, 1=是)\n" 
                   "   加密算法：%d (0=none, 1=aes, 2=des)\n" 
                   "   文件类型：%d\n" 
                   "   最小大小：%llu 字节\n" 
                   "   最大大小：%llu 字节\n" 
                   "2. 排除选项：\n" 
                   "   排除用户：%d\n" 
                   "   排除组：%d\n" 
                   "   用户数量：%d\n" 
                   "   组数量：%d\n" 
                   "   排除文件名：%s\n" 
                   "   排除目录数量：%d\n" 
                   "3. 时间范围：\n" 
                   "   创建时间：%d\n" 
                   "   修改时间：%d\n" 
                   "   访问时间：%d\n",
                   g_backup_options.source_path,
                   g_backup_options.target_path,
                   g_backup_options.pack_algorithm,
                   g_backup_options.compress_algorithm,
                   g_backup_options.encrypt_enable,
                   g_backup_options.encrypt_algorithm,
                   g_backup_options.file_types,
                   (unsigned long long)g_backup_options.size_range.min_size,
                   (unsigned long long)g_backup_options.size_range.max_size,
                   g_backup_options.exclude_usergroup.enable_user,
                   g_backup_options.exclude_usergroup.enable_group,
                   g_backup_options.exclude_usergroup.user_count,
                   g_backup_options.exclude_usergroup.group_count,
                   g_backup_options.exclude_filename.pattern,
                   g_backup_options.exclude_directory.count,
                   g_backup_options.create_time_range.enable,
                   g_backup_options.modify_time_range.enable,
                   g_backup_options.access_time_range.enable);
            MessageBox(hwnd, debug_msg, "备份调试信息", MB_OK | MB_ICONINFORMATION);
            
            // 调试：验证路径是否存在
            DWORD attrs = GetFileAttributes(g_backup_options.source_path);
            char path_msg[512];
            if (attrs == INVALID_FILE_ATTRIBUTES) {
                sprintf(path_msg, "错误：源路径不存在或无法访问！\n" 
                       "路径：%s\n" 
                       "错误码：%u\n" 
                       "错误信息：%s\n", 
                       g_backup_options.source_path, 
                       GetLastError(), 
                       "无法访问该路径");
                MessageBox(hwnd, path_msg, "路径错误", MB_OK | MB_ICONERROR);
                return 0;
            } else {
                sprintf(path_msg, "路径验证成功！\n" 
                       "源路径：%s\n" 
                       "属性：%u (0x%X)\n" 
                       "是否目录：%s\n", 
                       g_backup_options.source_path, 
                       attrs, attrs, 
                       (attrs & FILE_ATTRIBUTE_DIRECTORY) ? "是" : "否");
                MessageBox(hwnd, path_msg, "路径验证", MB_OK | MB_ICONINFORMATION);
            }
            
            // 确保目标目录存在
            if (!CreateDirectory(g_backup_options.target_path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
                sprintf(path_msg, "错误：无法创建目标目录！\n" 
                       "路径：%s\n" 
                       "错误码：%u\n", 
                       g_backup_options.target_path, 
                       GetLastError());
                MessageBox(hwnd, path_msg, "目录创建错误", MB_OK | MB_ICONERROR);
                return 0;
            } else {
                sprintf(path_msg, "目标目录验证成功！\n" 
                       "路径：%s\n" 
                       "状态：%s\n", 
                       g_backup_options.target_path, 
                       (GetLastError() == ERROR_ALREADY_EXISTS) ? "目录已存在" : "目录创建成功");
                MessageBox(hwnd, path_msg, "目录验证", MB_OK | MB_ICONINFORMATION);
            }
            
            // 调用备份函数
            MessageBox(hwnd, "开始执行备份...", "提示", MB_OK | MB_ICONINFORMATION);
            
            BackupResult result = backup_data(&g_backup_options);
            
            // 调试：显示备份结果
            char result_msg[512];
            sprintf(result_msg, "=== 备份结果 ===\n" 
                   "结果代码：%d\n" 
                   "结果含义：", 
                   result);
            
            switch (result)
            {
            case BACKUP_SUCCESS:
                strcat(result_msg, "备份成功！");
                MessageBox(hwnd, result_msg, "备份成功", MB_OK | MB_ICONINFORMATION);
                
                // 检查备份目录内容
                WIN32_FIND_DATA findData;
                HANDLE hFind = FindFirstFile(".\\backup\\*", &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    char backup_files[1024] = "=== 备份文件列表 ===\n";
                    int file_count = 0;
                    do {
                        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            char file_info[256];
                            sprintf(file_info, "%s\t%llu 字节\n", 
                                   findData.cFileName, 
                                   (long long)findData.nFileSizeHigh << 32 | findData.nFileSizeLow);
                            strcat(backup_files, file_info);
                            file_count++;
                        }
                    } while (FindNextFile(hFind, &findData));
                    FindClose(hFind);
                    
                    char count_info[64];
                    sprintf(count_info, "\n共找到 %d 个文件\n", file_count);
                    strcat(backup_files, count_info);
                    
                    MessageBox(hwnd, backup_files, "备份文件", MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBox(hwnd, "备份目录为空！", "警告", MB_OK | MB_ICONWARNING);
                }
                break;
            case BACKUP_ERROR_PARAM:
                strcat(result_msg, "备份失败：参数错误");
                MessageBox(hwnd, result_msg, "备份失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_PATH:
                strcat(result_msg, "备份失败：路径错误");
                MessageBox(hwnd, result_msg, "备份失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_MEMORY:
                strcat(result_msg, "备份失败：内存分配错误");
                MessageBox(hwnd, result_msg, "备份失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_NO_FILES:
                strcat(result_msg, "备份失败：没有找到要备份的文件");
                MessageBox(hwnd, result_msg, "备份失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_PACK:
                strcat(result_msg, "备份失败：打包失败");
                MessageBox(hwnd, result_msg, "备份失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_COMPRESS:
                strcat(result_msg, "备份失败：压缩失败");
                MessageBox(hwnd, result_msg, "备份失败", MB_OK | MB_ICONERROR);
                break;
            case BACKUP_ERROR_ENCRYPT:
                strcat(result_msg, "备份失败：加密失败");
                MessageBox(hwnd, result_msg, "备份失败", MB_OK | MB_ICONERROR);
                break;
            default:
                sprintf(result_msg + strlen(result_msg), "备份失败，未知错误码: %d", result);
                MessageBox(hwnd, result_msg, "备份失败", MB_OK | MB_ICONERROR);
                break;
            }
            return 0;
        }
        case ID_REFRESH_BUTTON:
        {
            // 刷新备份列表
            LoadBackupFiles();
            return 0;
        }
        case ID_DELETE_BUTTON:
        {
            // 删除选中的备份
            // 这里需要实现删除逻辑
            return 0;
        }
        case ID_RESTORE_BUTTON:
        {
            // 还原选中的备份
            char backup_file[256] = {0};
            char restore_path[256] = {0};
            char password[64] = {0};
            
            // 检查备份是否加密（这里需要根据备份文件的元数据判断）
            int is_encrypted = 1; // 假设备份是加密的
            
            if (is_encrypted)
            {
                // 显示密码输入对话框
                if (ShowPasswordDialog(hwnd, password, sizeof(password)))
                {
                    // 用户输入了密码，执行还原
                    MessageBox(hwnd, "还原功能待实现", "提示", MB_OK | MB_ICONINFORMATION);
                }
            }
            else
            {
                // 备份未加密，直接执行还原
                MessageBox(hwnd, "还原功能待实现", "提示", MB_OK | MB_ICONINFORMATION);
            }
            return 0;
        }
        }
        return 0;
    }
    case WM_SIZE:
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        MoveWindow(hTab, 10, 10, rc.right - 20, rc.bottom - 20, TRUE);
        MoveWindow(hBackupPage, 20, 50, rc.right - 40, rc.bottom - 80, TRUE);
        MoveWindow(hManagePage, 20, 50, rc.right - 40, rc.bottom - 80, TRUE);
        MoveWindow(hSettingsPage, 20, 50, rc.right - 40, rc.bottom - 80, TRUE);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// 创建备份页面
void CreateBackupPage(HWND hwnd)
{
    // 创建备份页面作为普通窗口
    hBackupPage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        20,
        50,
        780,
        440,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    // 子类化备份页面，为其设置新的窗口过程以处理WM_NOTIFY消息
    g_originalBackupPageProc = (WNDPROC)SetWindowLongPtr(hBackupPage, GWLP_WNDPROC, (LONG_PTR)BackupPageProc);

    // 数据路径标签和编辑框
    CreateWindow(WC_STATIC,
        "数据路径：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        10,
        80,
        25,
        hBackupPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    hSourcePathEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        ".\\test_source",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        100,
        10,
        500,
        25,
        hBackupPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 选择路径按钮，使用更现代的样式
    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_BUTTON,
        "选择源路径",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER,
        620,
        10,
        120, // 增加按钮宽度
        30, // 增加按钮高度
        hBackupPage,
        (HMENU)ID_SELECT_PATH_BUTTON,
        GetModuleHandle(NULL),
        NULL);

    // 创建筛选标签页
    CreateFilterTabs(hBackupPage);

    // 覆盖选项
    hOverwriteCheck = CreateWindow(WC_BUTTON,
        "覆盖现有备份文件",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10,
        200,
        150,
        25,
        hBackupPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    // 默认勾选覆盖选项
    SendMessage(hOverwriteCheck, BM_SETCHECK, BST_CHECKED, 0);

    // 打包算法标签
    CreateWindow(WC_STATIC,
        "打包算法",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        230,
        80,
        25,
        hBackupPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 创建打包算法单选按钮容器
    HWND hPackContainer = CreateWindow(WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        100,
        230,
        220,
        25,
        hBackupPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 打包算法单选按钮
    hPackZip = CreateWindow(WC_BUTTON,
        "zip",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        0,
        0,
        100,
        25,
        hPackContainer,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    hPackTar = CreateWindow(WC_BUTTON,
        "tar",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        120,
        0,
        100,
        25,
        hPackContainer,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    // 默认选择tar算法
    SendMessage(hPackTar, BM_SETCHECK, BST_CHECKED, 0);

    // 压缩选项
    hCompressCheck = CreateWindow(WC_BUTTON,
        "压缩",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10,
        260,
        60,
        25,
        hBackupPage,
        (HMENU)ID_COMPRESS_CHECK,
        GetModuleHandle(NULL),
        NULL);

    // 创建压缩算法单选按钮容器
    HWND hCompressContainer = CreateWindow(WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        100,
        260,
        220,
        25,
        hBackupPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    hCompressHuffman = CreateWindow(WC_BUTTON,
        "Haff",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_DISABLED,
        0,
        0,
        100,
        25,
        hCompressContainer,
        (HMENU)ID_COMPRESS_HUFFMAN,
        GetModuleHandle(NULL),
        NULL);

    hCompressLZ77 = CreateWindow(WC_BUTTON,
        "LZ77",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_DISABLED,
        120,
        0,
        100,
        25,
        hCompressContainer,
        (HMENU)ID_COMPRESS_LZ77,
        GetModuleHandle(NULL),
        NULL);

    // 加密选项
    hEncryptCheck = CreateWindow(WC_BUTTON,
        "加密",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10,
        290,
        60,
        25,
        hBackupPage,
        (HMENU)ID_ENCRYPT_CHECK,
        GetModuleHandle(NULL),
        NULL);

    // 创建加密算法单选按钮容器
    HWND hEncryptContainer = CreateWindow(WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        100,
        290,
        220,
        25,
        hBackupPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    hEncryptAES = CreateWindow(WC_BUTTON,
        "AES",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_DISABLED,
        0,
        0,
        100,
        25,
        hEncryptContainer,
        (HMENU)ID_ENCRYPT_AES,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_STATIC,
        "key",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        340,
        290,
        30,
        25,
        hBackupPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    hEncryptKeyEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_DISABLED,
        380,
        290,
        300,
        25,
        hBackupPage,
        (HMENU)ID_ENCRYPT_KEY_EDIT,
        GetModuleHandle(NULL),
        NULL);

    // 备份按钮，使用更突出的样式
    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_BUTTON,
        "备份",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER,
        650, // 调整位置，使其更居中
        350,
        150, // 增加按钮宽度，使其更突出
        40, // 增加按钮高度，使其更突出
        hBackupPage,
        (HMENU)ID_BACKUP_BUTTON,
        GetModuleHandle(NULL),
        NULL);
}

// 创建管理备份数据页面
void CreateManagePage(HWND hwnd)
{
    hManagePage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        20,
        50,
        780,
        440,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    // 子类化管理页面，为其设置新的窗口过程以处理WM_COMMAND消息
    g_originalManagePageProc = (WNDPROC)SetWindowLongPtr(hManagePage, GWLP_WNDPROC, (LONG_PTR)ManagePageProc);
    
    // 确保hBackupPathEdit已经被初始化
    if (hBackupPathEdit == NULL) {
        // 如果hBackupPathEdit还没有被创建，创建一个隐藏的编辑框
        hBackupPathEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
            WC_EDIT,
            ".\backup", // 使用相对路径，更通用
            WS_CHILD | ES_AUTOHSCROLL, // 不显示
            0,
            0,
            0,
            0,
            hManagePage, // 使用当前页面作为父窗口
            NULL,
            GetModuleHandle(NULL),
            NULL);
    }

    // 左侧按钮组
    CreateWindow(WC_BUTTON,
        "选择文件",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10,
        10,
        120,
        30,
        hManagePage,
        (HMENU)ID_SELECT_PATH_BUTTON,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_BUTTON,
        "删除",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10,
        50,
        120,
        30,
        hManagePage,
        (HMENU)ID_DELETE_BUTTON,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_BUTTON,
        "刷新",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10,
        90,
        120,
        30,
        hManagePage,
        (HMENU)ID_REFRESH_BUTTON,
        GetModuleHandle(NULL),
        NULL);

    // 右侧备份列表
    hBackupList = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        140,
        10,
        630,
        250,
        hManagePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 添加列表视图列
    LVCOLUMN lvCol;
    lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvCol.pszText = "文件名";
    lvCol.cx = 150;
    ListView_InsertColumn(hBackupList, 0, &lvCol);

    lvCol.pszText = "文件大小";
    lvCol.cx = 100;
    ListView_InsertColumn(hBackupList, 1, &lvCol);

    lvCol.pszText = "备份时间";
    lvCol.cx = 150;
    ListView_InsertColumn(hBackupList, 2, &lvCol);

    lvCol.pszText = "状态";
    lvCol.cx = 100;
    ListView_InsertColumn(hBackupList, 3, &lvCol);

    // 备份文件标签和编辑框
    CreateWindow(WC_STATIC,
        "备份文件：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        280,
        80,
        25,
        hManagePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    hBackupFileEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        100,
        280,
        670,
        25,
        hManagePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 还原路径标签和编辑框
    CreateWindow(WC_STATIC,
        "还原路径：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        320,
        80,
        25,
        hManagePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    hRestorePathEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        100,
        320,
        570,
        25,
        hManagePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 选择还原路径按钮
    CreateWindow(WC_BUTTON,
        "选择路径",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        680,
        320,
        100,
        25,
        hManagePage,
        (HMENU)ID_SELECT_RESTORE_PATH_BUTTON,
        GetModuleHandle(NULL),
        NULL);

    // 还原选项区域
    // 加密选项复选框
    CreateWindow(WC_BUTTON,
        "加密备份",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10,
        360,
        80,
        25,
        hManagePage,
        (HMENU)1020, // 加密选项复选框ID
        GetModuleHandle(NULL),
        NULL);

    // AES加密算法选项
    CreateWindow(WC_BUTTON,
        "AES加密",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        100,
        360,
        80,
        25,
        hManagePage,
        (HMENU)1021, // AES加密算法ID
        GetModuleHandle(NULL),
        NULL);

    // 密钥输入框标签
    CreateWindow(WC_STATIC,
        "密钥：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        200,
        360,
        40,
        25,
        hManagePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 密钥输入框
    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        240,
        360,
        300,
        25,
        hManagePage,
        (HMENU)1022, // 密钥输入框ID
        GetModuleHandle(NULL),
        NULL);

    // 还原按钮
    CreateWindow(WC_BUTTON,
        "还原",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        680,
        360,
        100,
        30,
        hManagePage,
        (HMENU)ID_RESTORE_BUTTON,
        GetModuleHandle(NULL),
        NULL);
    
    // 初始化时加载备份文件列表
    LoadBackupFiles();
}

// 更新定时备份状态显示
void update_schedule_status() {
    if (hScheduleStatus) {
        if (g_schedule_is_running) {
            // 设置为运行状态：绿色文本
            SetWindowText(hScheduleStatus, "运行中");
            HDC hDC = GetDC(hScheduleStatus);
            SetTextColor(hDC, RGB(0, 255, 0));
            ReleaseDC(hScheduleStatus, hDC);
            // 更新按钮状态：运行中时，启用禁用按钮，禁用启用按钮
            if (hStartBackupBtn) EnableWindow(hStartBackupBtn, FALSE);
            if (hStopBackupBtn) EnableWindow(hStopBackupBtn, TRUE);
        } else {
            // 设置为未运行状态：红色文本
            SetWindowText(hScheduleStatus, "未运行");
            HDC hDC = GetDC(hScheduleStatus);
            SetTextColor(hDC, RGB(255, 0, 0));
            ReleaseDC(hScheduleStatus, hDC);
            // 更新按钮状态：未运行时，启用启用按钮，禁用禁用按钮
            if (hStartBackupBtn) EnableWindow(hStartBackupBtn, TRUE);
            if (hStopBackupBtn) EnableWindow(hStopBackupBtn, FALSE);
        }
    }
}

// 更新定时淘汰状态显示
void update_cleanup_status() {
    if (hCleanupStatus) {
        if (g_cleanup_is_running) {
            // 设置为运行状态：绿色文本
            SetWindowText(hCleanupStatus, "运行中");
            HDC hDC = GetDC(hCleanupStatus);
            SetTextColor(hDC, RGB(0, 255, 0));
            ReleaseDC(hCleanupStatus, hDC);
            // 更新按钮状态：运行中时，启用禁用按钮，禁用启用按钮
            if (hStartCleanupBtn) EnableWindow(hStartCleanupBtn, FALSE);
            if (hStopCleanupBtn) EnableWindow(hStopCleanupBtn, TRUE);
        } else {
            // 设置为未运行状态：红色文本
            SetWindowText(hCleanupStatus, "未运行");
            HDC hDC = GetDC(hCleanupStatus);
            SetTextColor(hDC, RGB(255, 0, 0));
            ReleaseDC(hCleanupStatus, hDC);
            // 更新按钮状态：未运行时，启用启用按钮，禁用禁用按钮
            if (hStartCleanupBtn) EnableWindow(hStartCleanupBtn, TRUE);
            if (hStopCleanupBtn) EnableWindow(hStopCleanupBtn, FALSE);
        }
    }
}

// 创建设置页面
void CreateSettingsPage(HWND hwnd)
{
    hSettingsPage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        20,
        20,
        780,
        440,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    // 子类化设置页面，为其设置新的窗口过程以处理WM_COMMAND消息
    g_originalSettingsPageProc = (WNDPROC)SetWindowLongPtr(hSettingsPage, GWLP_WNDPROC, (LONG_PTR)SettingsPageProc);

    // 数据备份路径标签和编辑框
    CreateWindow(WC_STATIC,
        "数据备份路径：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        30,
        120,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    hBackupPathEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        ".\\backup", // 使用相对路径，更通用
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        140,
        30,
        500,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 设置备份路径按钮
    CreateWindow(WC_BUTTON,
        "选择路径",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        650,
        30,
        100,
        25,
        hSettingsPage,
        (HMENU)ID_SET_BACKUP_PATH_BUTTON,
        GetModuleHandle(NULL),
        NULL);

    // 状态显示行
    // 定时备份状态显示
    CreateWindow(WC_STATIC,
        "定时备份状态：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        70,
        120,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 创建定时备份状态显示控件
    hScheduleStatus = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        "未运行",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        140,
        70,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    // 设置定时备份状态显示控件的背景色和文本颜色
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
    SendMessage(hScheduleStatus, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hScheduleStatus), (LPARAM)hBrush);
    DeleteObject(hBrush);
    
    // 设置定时备份状态控件文本颜色为红色（未运行状态）
    HDC hDC = GetDC(hScheduleStatus);
    SetTextColor(hDC, RGB(255, 0, 0));
    ReleaseDC(hScheduleStatus, hDC);

    // 定时淘汰状态显示
    CreateWindow(WC_STATIC,
        "定时淘汰状态：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        260,
        70,
        120,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 创建定时淘汰状态显示控件
    hCleanupStatus = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        "未运行",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        390,
        70,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    // 设置定时淘汰状态显示控件的背景色和文本颜色
    hBrush = CreateSolidBrush(RGB(255, 255, 255));
    SendMessage(hCleanupStatus, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hCleanupStatus), (LPARAM)hBrush);
    DeleteObject(hBrush);
    
    // 设置定时淘汰状态控件文本颜色为红色（未运行状态）
    hDC = GetDC(hCleanupStatus);
    SetTextColor(hDC, RGB(255, 0, 0));
    ReleaseDC(hCleanupStatus, hDC);

    // 增加留白，调整定时备份设置的位置
    // 定时备份设置
    CreateWindow(WC_STATIC,
        "定时备份设置：",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        10,
        130,
        120,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 备份间隔（小时）
    CreateWindow(WC_STATIC,
        "间隔（小时）：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        170,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hIntervalHoursEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "0",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
        120,
        170,
        60,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 备份间隔（分钟）
    CreateWindow(WC_STATIC,
        "间隔（分钟）：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        200,
        170,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hIntervalMinutesEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "60",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
        310,
        170,
        60,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 源路径
    CreateWindow(WC_STATIC,
        "源路径：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        210,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hScheduleSourceEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "./test_source",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        120,
        210,
        400,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 源路径浏览按钮
    CreateWindow(WC_BUTTON,
        "浏览",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        530,
        210,
        70,
        25,
        hSettingsPage,
        (HMENU)1012, // 临时ID，后续需要添加到宏定义中
        GetModuleHandle(NULL),
        NULL);

    // 目标路径
    CreateWindow(WC_STATIC,
        "目标路径：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        250,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hScheduleTargetEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "./backup",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        120,
        250,
        400,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 目标路径浏览按钮
    CreateWindow(WC_BUTTON,
        "浏览",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        530,
        250,
        70,
        25,
        hSettingsPage,
        (HMENU)1013, // 临时ID，后续需要添加到宏定义中
        GetModuleHandle(NULL),
        NULL);

    // 打包算法
    CreateWindow(WC_STATIC,
        "打包算法：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        290,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 创建打包算法单选按钮容器
    HWND hSchedulePackContainer = CreateWindow(WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        120,
        290,
        220,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hSchedulePackZip = CreateWindow(WC_BUTTON,
        "zip",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        0,
        0,
        100,
        25,
        hSchedulePackContainer,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hSchedulePackTar = CreateWindow(WC_BUTTON,
        "tar",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        120,
        0,
        100,
        25,
        hSchedulePackContainer,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    // 默认选择tar算法
    SendMessage(hSchedulePackTar, BM_SETCHECK, BST_CHECKED, 0);

    // 压缩算法
    CreateWindow(WC_STATIC,
        "压缩算法：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        330,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 创建压缩算法单选按钮容器
    HWND hScheduleCompressContainer = CreateWindow(WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        120,
        330,
        400,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hScheduleCompressNone = CreateWindow(WC_BUTTON,
        "无压缩",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        0,
        0,
        100,
        25,
        hScheduleCompressContainer,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hScheduleCompressHaff = CreateWindow(WC_BUTTON,
        "哈夫曼",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        120,
        0,
        100,
        25,
        hScheduleCompressContainer,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hScheduleCompressLz77 = CreateWindow(WC_BUTTON,
        "lz77",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        240,
        0,
        100,
        25,
        hScheduleCompressContainer,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    // 默认选择无压缩
    SendMessage(hScheduleCompressNone, BM_SETCHECK, BST_CHECKED, 0);

    // 覆盖选项
    CreateWindow(WC_STATIC,
        "覆盖选项：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        370,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 创建覆盖复选框
    HWND hScheduleOverwriteCheck = CreateWindow(WC_BUTTON,
        "覆盖现有备份文件",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        120,
        370,
        150,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    // 默认勾选覆盖选项
    SendMessage(hScheduleOverwriteCheck, BM_SETCHECK, BST_CHECKED, 0);

    // 增加留白，调整定时淘汰设置的位置
    // 定时淘汰设置（移到最下方）
    CreateWindow(WC_STATIC,
        "定时淘汰设置：",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        10,
        410,
        120,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);



    // 保留天数
    CreateWindow(WC_STATIC,
        "保留天数：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        450,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hKeepDaysEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "7",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
        120,
        450,
        60,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 保留备份数
    CreateWindow(WC_STATIC,
        "保留备份数：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        200,
        450,
        100,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    HWND hMaxBackupsEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "5",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
        310,
        450,
        60,
        25,
        hSettingsPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 按钮排版：第一行：启用/禁用定时备份；第二行：启用/禁用定时淘汰
    // 启用定时备份按钮（下沿与保留天数框下沿对齐）
    hStartBackupBtn = CreateWindow(WC_BUTTON,
        "启用定时备份",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        550,
        445,
        120,
        30,
        hSettingsPage,
        (HMENU)ID_SCHEDULE_BUTTON,
        GetModuleHandle(NULL),
        NULL);

    // 禁用定时备份按钮（下沿与保留天数框下沿对齐）
    hStopBackupBtn = CreateWindow(WC_BUTTON,
        "禁用定时备份",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        680,
        445,
        120,
        30,
        hSettingsPage,
        (HMENU)ID_CANCEL_SCHEDULE_BUTTON,
        GetModuleHandle(NULL),
        NULL);

    // 启用定时淘汰按钮（第二行，下沿与保留天数框下沿对齐）
    hStartCleanupBtn = CreateWindow(WC_BUTTON,
        "启用定时淘汰",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        550,
        485,
        120,
        30,
        hSettingsPage,
        (HMENU)1014, // 临时ID，用于定时淘汰启动
        GetModuleHandle(NULL),
        NULL);

    // 禁用定时淘汰按钮（第二行，下沿与保留天数框下沿对齐）
    hStopCleanupBtn = CreateWindow(WC_BUTTON,
        "禁用定时淘汰",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        680,
        485,
        120,
        30,
        hSettingsPage,
        (HMENU)1015, // 临时ID，用于定时淘汰禁用
        GetModuleHandle(NULL),
        NULL);

    // 初始化按钮状态
    EnableWindow(hStopBackupBtn, FALSE); // 初始状态下禁用按钮不可点击
    EnableWindow(hStopCleanupBtn, FALSE); // 初始状态下禁用按钮不可点击
}

// 创建筛选标签页
void CreateFilterTabs(HWND hwnd)
{
    // 创建筛选标签页控件
    hFilterTabs = CreateWindowEx(0,
        WC_TABCONTROL,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS,
        10,
        50,
        750,
        130,
        hwnd,
        (HMENU)ID_FILTER_TABS,
        GetModuleHandle(NULL),
        NULL);

    // 设置筛选标签控件的字体
    SendMessage(hFilterTabs, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(hFilterTabs, WM_FONTCHANGE, 0, 0);

    // 添加筛选标签页
    TCITEM tie;
    tie.mask = TCIF_TEXT;
    
    char* filter_texts[] = {"文件类型", "时间", "文件大小", "排除用户（组）", "排除文件名称", "排除目录"};
    for (int i = 0; i < 6; i++) {
        tie.pszText = filter_texts[i];
        TabCtrl_InsertItem(hFilterTabs, i, &tie);
    }

    // 创建各个筛选子页面，将筛选标签控件作为父窗口
    CreateFileTypePage(hFilterTabs);
    CreateTimePage(hFilterTabs);
    CreateSizePage(hFilterTabs);
    CreateExcludeUserPage(hFilterTabs);
    CreateExcludeFilenamePage(hFilterTabs);
    CreateExcludeDirPage(hFilterTabs);

    // 显示第一个筛选页面
    ShowFilterPage(0);
}

// 创建文件类型筛选页面
void CreateFileTypePage(HWND hwnd)
{
    hFileTypePage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        5,
        30,
        740,
        90,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 文件类型选项（Windows版本）
    HWND hRegularFileCheck = CreateWindow(WC_BUTTON,
        "普通文件",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10,
        10,
        100,
        25,
        hFileTypePage,
        (HMENU)ID_FILTER_REGULAR_FILE,
        GetModuleHandle(NULL),
        NULL);
    // 默认勾选
    SendMessage(hRegularFileCheck, BM_SETCHECK, BST_CHECKED, 0);

    HWND hDirectoryCheck = CreateWindow(WC_BUTTON,
        "目录文件",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        120,
        10,
        100,
        25,
        hFileTypePage,
        (HMENU)ID_FILTER_DIRECTORY,
        GetModuleHandle(NULL),
        NULL);
    // 默认勾选
    SendMessage(hDirectoryCheck, BM_SETCHECK, BST_CHECKED, 0);

    CreateWindow(WC_BUTTON,
        "快捷方式",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        230,
        10,
        100,
        25,
        hFileTypePage,
        (HMENU)ID_FILTER_SHORTCUT,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_BUTTON,
        "隐藏文件",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        340,
        10,
        100,
        25,
        hFileTypePage,
        (HMENU)ID_FILTER_HIDDEN,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_BUTTON,
        "系统文件",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        450,
        10,
        100,
        25,
        hFileTypePage,
        (HMENU)ID_FILTER_SYSTEM,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_BUTTON,
        "所有文件",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        560,
        10,
        120,
        25,
        hFileTypePage,
        (HMENU)ID_FILTER_ALL,
        GetModuleHandle(NULL),
        NULL);
}

// 创建时间筛选页面
void CreateTimePage(HWND hwnd)
{
    hTimePage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        5,
        30,
        740,
        90,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 创建时间复选框和编辑框
    CreateWindow(WC_BUTTON,
        "创建时间",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10,
        10,
        100,
        25,
        hTimePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "2000/01/01 00:00",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        120,
        10,
        120,
        25,
        hTimePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_STATIC,
        "-",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        250,
        10,
        20,
        25,
        hTimePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "2030/12/31 23:59",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        280,
        10,
        120,
        25,
        hTimePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_BUTTON,
        "修改时间",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10,
        45,
        100,
        25,
        hTimePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "2000/01/01 00:00",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        120,
        45,
        120,
        25,
        hTimePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_STATIC,
        "-",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        250,
        45,
        20,
        25,
        hTimePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "2030/12/31 23:59",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        280,
        45,
        120,
        25,
        hTimePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
}

// 创建文件大小筛选页面
void CreateSizePage(HWND hwnd)
{
    hSizePage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        5,
        30,
        740,
        90,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 文件大小选项
    CreateWindow(WC_STATIC,
        "尺寸",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        35,
        50,
        25,
        hSizePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "0",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
        70,
        35,
        100,
        25,
        hSizePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_STATIC,
        "bytes -",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        180,
        35,
        60,
        25,
        hSizePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "1000000",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
        250,
        35,
        100,
        25,
        hSizePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_STATIC,
        "bytes",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        360,
        35,
        50,
        25,
        hSizePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
}

// 创建排除用户（组）筛选页面
void CreateExcludeUserPage(HWND hwnd)
{
    hExcludeUserPage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        5,
        30,
        740,
        90,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 排除用户选项
    CreateWindow(WC_BUTTON,
        "排除用户",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10,
        10,
        100,
        25,
        hExcludeUserPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_BUTTON,
        "排除用户组",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        370,
        10,
        120,
        25,
        hExcludeUserPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 用户列表
    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTBOX,
        NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        10,
        45,
        300,
        30,
        hExcludeUserPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 用户组列表
    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTBOX,
        NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        370,
        45,
        300,
        30,
        hExcludeUserPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
}

// 创建排除文件名称筛选页面
void CreateExcludeFilenamePage(HWND hwnd)
{
    hExcludeFilenamePage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        5,
        30,
        740,
        90,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 文件名称标签
    CreateWindow(WC_STATIC,
        "文件名称",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        10,
        80,
        25,
        hExcludeFilenamePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 正则表达式输入框
    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_EDIT,
        "^.*\\.doc$",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        100,
        10,
        500,
        25,
        hExcludeFilenamePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 排除列表
    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTBOX,
        NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        10,
        45,
        600,
        30,
        hExcludeFilenamePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 添加按钮
    CreateWindow(WC_BUTTON,
        "添加",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        620,
        45,
        80,
        25,
        hExcludeFilenamePage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
}

// 创建排除目录筛选页面
void CreateExcludeDirPage(HWND hwnd)
{
    hExcludeDirPage = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_STATIC,
        NULL,
        WS_CHILD | WS_VISIBLE,
        5,
        30,
        740,
        90,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    // 排除目录选项
    CreateWindow(WC_STATIC,
        "排除",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10,
        10,
        50,
        25,
        hExcludeDirPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTBOX,
        NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        10,
        45,
        600,
        30,
        hExcludeDirPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    CreateWindow(WC_BUTTON,
        "添加",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        620,
        45,
        80,
        25,
        hExcludeDirPage,
        NULL,
        GetModuleHandle(NULL),
        NULL);
}

// 显示筛选页面
void ShowFilterPage(int page_index)
{
    ShowWindow(hFileTypePage, page_index == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(hTimePage, page_index == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSizePage, page_index == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(hExcludeUserPage, page_index == 3 ? SW_SHOW : SW_HIDE);
    ShowWindow(hExcludeFilenamePage, page_index == 4 ? SW_SHOW : SW_HIDE);
    ShowWindow(hExcludeDirPage, page_index == 5 ? SW_SHOW : SW_HIDE);
}

// 密码输入对话框过程
INT_PTR CALLBACK PasswordDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // 初始化对话框
        SetWindowText(hwnd, "密码输入");
        CreateWindow(WC_STATIC,
            "请输入密码",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20,
            20,
            100,
            25,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        
        // 创建密码输入框
        HWND hPasswordEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
            WC_EDIT,
            "",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD,
            20,
            50,
            200,
            25,
            hwnd,
            (HMENU)1001,
            GetModuleHandle(NULL),
            NULL);
        
        // 创建取消按钮
        CreateWindow(WC_BUTTON,
            "取消",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50,
            90,
            80,
            30,
            hwnd,
            (HMENU)IDCANCEL,
            GetModuleHandle(NULL),
            NULL);
        
        // 创建OK按钮
        CreateWindow(WC_BUTTON,
            "确定",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            160,
            90,
            80,
            30,
            hwnd,
            (HMENU)IDOK,
            GetModuleHandle(NULL),
            NULL);
        
        // 设置焦点到密码输入框
        SetFocus(hPasswordEdit);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            // 获取密码
            HWND hPasswordEdit = GetDlgItem(hwnd, 1001);
            char password_a[256];
            GetWindowText(hPasswordEdit, password_a, 256);
            // 直接复制到全局缓冲区
            strncpy(g_password_buffer, password_a, g_password_max_len);
            g_password_buffer[g_password_max_len - 1] = '\0'; // 确保字符串结束
            EndDialog(hwnd, IDOK);
            return (INT_PTR)TRUE;
        }
        case IDCANCEL:
        {
            EndDialog(hwnd, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        }
        return (INT_PTR)FALSE;
    }
    default:
        return (INT_PTR)FALSE;
    }
}

// 显示密码输入对话框
BOOL ShowPasswordDialog(HWND hwnd, char* password, int max_len)
{
    g_password_buffer = password;
    g_password_max_len = max_len;
    
    // 创建模态对话框
    int result = DialogBoxParam(GetModuleHandle(NULL), NULL, hwnd, PasswordDialogProc, 0);
    
    return (result == IDOK);
}

// 加载备份文件列表
void LoadBackupFiles()
{
    // 清空当前列表
    ListView_DeleteAllItems(hBackupList);
    
    // 获取备份路径
    char backup_path[MAX_PATH] = {0};
    GetWindowText(hBackupPathEdit, backup_path, sizeof(backup_path));
    
    // 如果备份路径为空，使用默认路径
    if (backup_path[0] == '\0') {
        strcpy(backup_path, ".\backup");
    }
    
    // 构建搜索路径
    char search_path[MAX_PATH] = {0};
    sprintf(search_path, "%s\\*", backup_path);
    
    // 查找第一个文件
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(search_path, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        int item_index = 0;
        do {
            // 跳过目录和特殊文件
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
                strcmp(findData.cFileName, ".") != 0 && 
                strcmp(findData.cFileName, "..") != 0) {
                
                // 添加到列表视图
                LVITEM lvItem;
                lvItem.mask = LVIF_TEXT;
                lvItem.iItem = item_index;
                lvItem.iSubItem = 0;
                lvItem.pszText = findData.cFileName;
                ListView_InsertItem(hBackupList, &lvItem);
                
                // 设置文件大小
                char size_str[32] = {0};
                sprintf(size_str, "%llu", (long long)findData.nFileSizeHigh << 32 | findData.nFileSizeLow);
                ListView_SetItemText(hBackupList, item_index, 1, size_str);
                
                // 设置修改时间
                char time_str[32] = {0};
                SYSTEMTIME sysTime;
                FileTimeToSystemTime(&findData.ftLastWriteTime, &sysTime);
                sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", 
                    sysTime.wYear, sysTime.wMonth, sysTime.wDay,
                    sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
                ListView_SetItemText(hBackupList, item_index, 2, time_str);
                
                // 设置状态
                ListView_SetItemText(hBackupList, item_index, 3, "可用");
                
                item_index++;
            }
        } while (FindNextFile(hFind, &findData));
        
        FindClose(hFind);
    }
}

// 主GUI函数
int GUI_Init(int debug_mode)
{
    g_debug_mode = debug_mode;
    // 注册窗口类，使用WNDCLASSEX结构体，支持小图标
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "BackupGUI";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); // 使用按钮面背景色，更现代
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 使用标准应用图标
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION); // 使用标准应用小图标

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "窗口类注册失败！", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 调整窗口大小，使其更适合显示内容
    HWND hwnd = CreateWindow(
        "BackupGUI",
        "数据备份工具",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900, // 增加宽度，使界面更宽敞
        650, // 增加高度，使界面更宽敞
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, "窗口创建失败！", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// 主函数入口
#ifdef GUI_MAIN
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return GUI_Init();
}
#endif