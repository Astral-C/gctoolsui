#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__
#include <gtkmm.h>
#include <Archive.hpp>
#include <GCM.hpp>
#include <format>
#include <cmath>

class CellItemFilesystemNode {
public:
    CellItemFilesystemNode() = default;
    CellItemFilesystemNode(Glib::ustring entryName, Glib::ustring entrySize, std::filesystem::path entryPath, bool isFolder, std::shared_ptr<Archive::Folder> folderEntry, std::shared_ptr<Archive::File> fileEntry, std::shared_ptr<Disk::Folder> diskFolderEntry, std::shared_ptr<Disk::File> diskFileEntry);
    CellItemFilesystemNode(Glib::ustring entryName, const std::vector<CellItemFilesystemNode>& children);
    CellItemFilesystemNode(const CellItemFilesystemNode& src) = default;
    CellItemFilesystemNode(CellItemFilesystemNode&& src) = default;
    CellItemFilesystemNode& operator=(const CellItemFilesystemNode& src) = default;
    CellItemFilesystemNode& operator=(CellItemFilesystemNode&& src) = default;
    ~CellItemFilesystemNode() = default;

    Glib::ustring mEntryName;
    Glib::ustring mEntrySize;
    std::filesystem::path mEntryPath;
    
    bool mIsFolder;
    std::shared_ptr<Disk::Folder> mDiskFolderEntry;
    std::shared_ptr<Disk::File> mDiskFileEntry;

    std::shared_ptr<Archive::Folder> mFolderEntry;
    std::shared_ptr<Archive::File> mFileEntry;

    std::vector<CellItemFilesystemNode> mChildren;
};

// TODO Make this some sort of template so that both dist and archive can use it without duplication?
class OpenedItem : public Gtk::ScrolledWindow {
public:
    bool mIsArchive { true };
    std::filesystem::path mOpenedPath;
    Gtk::ColumnView* mView { nullptr };
    std::shared_ptr<Disk::Image> mDisk { nullptr };
    std::shared_ptr<Archive::Rarc> mArchive { nullptr };

    Glib::RefPtr<Gtk::TreeStore> mTreeStore;
    Glib::RefPtr<Gtk::SingleSelection> mSelection;
    Glib::RefPtr<Gtk::TreeListModel> mTreeListModel;
    
    std::vector<CellItemFilesystemNode> mDiskItems;
    std::vector<CellItemFilesystemNode> mArchiveItems;

    Compression::Format mCompressionFmt;

    void AddDirectoryNodeArchive(std::vector<CellItemFilesystemNode>& items, std::shared_ptr<Archive::Folder> directory, std::filesystem::path curPath);
    void AddDirectoryNodeDisk(std::vector<CellItemFilesystemNode>& items, std::shared_ptr<Disk::Folder> directory, std::filesystem::path curPath);

    void OnCreateItem(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void OnCreateNoExpander(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void OnCreateIconItem(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void OnBindName(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void OnBindSize(const Glib::RefPtr<Gtk::ListItem>& list_item);

    void Save();

    Glib::RefPtr<Gio::ListModel> CreateArchiveTreeModel(const Glib::RefPtr<Glib::ObjectBase>& item = {});
    Glib::RefPtr<Gio::ListModel> CreateDiskTreeModel(const Glib::RefPtr<Glib::ObjectBase>& item = {});
    
    OpenedItem(Gtk::ColumnView* view, std::shared_ptr<Archive::Rarc> arc);
    OpenedItem(Gtk::ColumnView* view, std::shared_ptr<Disk::Image> img);
};

class ItemTab : public Gtk::Box {
public:
    Gtk::Label mLabel;
    Gtk::Image mIconClose;
    Glib::RefPtr<Gtk::GestureClick> mCloseTabClicked;
    OpenedItem* mPageItem;
    Gtk::Notebook* mNotebook;
    
    void CloseItem(int n_press, double x, double y);

    ItemTab(std::string label, Gtk::Notebook* notebook, OpenedItem* item);
};


class ModelColumns : public Glib::Object {
public:
    Glib::ustring mEntryName;
    Glib::ustring mEntrySize;
    std::filesystem::path mEntryPath;
    
    bool mIsFolder;

    std::shared_ptr<Disk::Folder> mDiskFolderEntry;
    std::shared_ptr<Disk::File> mDiskFileEntry;

    std::shared_ptr<Archive::Folder> mFolderEntry;
    std::shared_ptr<Archive::File> mFileEntry;

    std::vector<CellItemFilesystemNode> mChildren;

    static Glib::RefPtr<ModelColumns> create(const CellItemFilesystemNode& item){
        return Glib::make_refptr_for_instance<ModelColumns>(new ModelColumns(item));
    }
    
protected:
    ModelColumns(const CellItemFilesystemNode& item) : mEntryName(item.mEntryName), mEntrySize(item.mEntrySize), mEntryPath(item.mEntryPath), mIsFolder(item.mIsFolder), mFolderEntry(item.mFolderEntry), mFileEntry(item.mFileEntry), mDiskFolderEntry(item.mDiskFolderEntry), mDiskFileEntry(item.mDiskFileEntry), mChildren(item.mChildren){}
};

class SettingsDialog : public Gtk::Dialog {
public:
    void on_message_finish(const Glib::RefPtr<Gio::AsyncResult>& result, const Glib::RefPtr<SettingsDialog>& dialog);
    SettingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    ~SettingsDialog() override;

    Glib::RefPtr<Gtk::Builder> Builder() { return mBuilder; }

protected:
    Glib::RefPtr<Gtk::Builder> mBuilder;
};

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    ~MainWindow() override;

protected:
    void OnNew();
    void OnOpen();
    void OnSave();
    void OnSaveAs();
    void OnQuit();
    void OnOpenSettings();
    void OpenArchive(Glib::RefPtr<Gio::AsyncResult>& result);

    void OnImport();
    void OnDelete();
    void OnExtract();
    void OnNewFolder();
    void OnCloseItem(){ /* TODO: close notebook page hovered */ }

    void TreeClicked(int n_press, double x, double y);
    void NotebookRightClicked(int n_press, double x, double y);
    Gtk::Statusbar* mStatus;
    Gtk::Notebook* mNotebook;
    Gtk::PopoverMenu mContextMenu;

    Glib::RefPtr<Gio::Menu> mTreeContextMenuModel;
    Glib::RefPtr<Gio::Menu> mNotebookContextMenuModel;

    SettingsDialog* mSettingsDialog;
    Glib::RefPtr<Gtk::GestureClick> mTreeClicked, mNotebookClicked;
    Glib::RefPtr<Gtk::RecentManager> mRecentManager;
    Glib::RefPtr<Gtk::FileDialog> mFileDialog;
    Glib::RefPtr<Gtk::Builder> mBuilder;
};

SettingsDialog* BuildSettingsDialog();
Gtk::ApplicationWindow* BuildWindow();

#endif