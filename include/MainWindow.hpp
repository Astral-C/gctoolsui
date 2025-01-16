#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__
#include <gtkmm.h>
#include <Archive.hpp>
#include <GCM.hpp>
#include <format>
#include <cmath>
#include "bytesize.hpp"

class ArchiveFSNode : public Glib::Object {
public:
    Glib::ustring mEntryName;
    Glib::ustring mEntrySize;
    
    bool mIsFolder;

    std::shared_ptr<Archive::Folder> mFolderEntry;
    std::shared_ptr<Archive::File> mFileEntry;

    std::vector<ArchiveFSNode> mChildren;

    Gtk::Label* mLabel;

    static Glib::RefPtr<ArchiveFSNode> create(std::shared_ptr<Archive::Folder> folder){
        return Glib::make_refptr_for_instance<ArchiveFSNode>(new ArchiveFSNode(folder));
    }

    static Glib::RefPtr<ArchiveFSNode> create(std::shared_ptr<Archive::File> file){
        return Glib::make_refptr_for_instance<ArchiveFSNode>(new ArchiveFSNode(file));
    }
    
protected:
    ArchiveFSNode(std::shared_ptr<Archive::Folder> folder){
        mIsFolder = true;
        mFolderEntry = folder;
        mFileEntry = nullptr;

        mEntryName = folder->GetName();
        mEntrySize = "";
    }

    ArchiveFSNode(std::shared_ptr<Archive::File> file){
        mIsFolder = false;
        mFolderEntry = nullptr;
        mFileEntry = file;

        mEntryName = file->GetName();
        mEntrySize = std::string(bytesize::bytesize(file->GetSize()));
    }
};

class DiskFSNode : public Glib::Object {
public:
    Glib::ustring mEntryName;
    Glib::ustring mEntrySize;
    
    bool mIsFolder;

    std::shared_ptr<Disk::Folder> mFolderEntry;
    std::shared_ptr<Disk::File> mFileEntry;

    //std::vector<ArchiveFSNode> mMountedArchive; // perhaps?
    std::vector<DiskFSNode> mChildren;

    Gtk::Label* mLabel;

    static Glib::RefPtr<DiskFSNode> create(std::shared_ptr<Disk::Folder> folder){
        return Glib::make_refptr_for_instance<DiskFSNode>(new DiskFSNode(folder));
    }

    static Glib::RefPtr<DiskFSNode> create(std::shared_ptr<Disk::File> file){
        return Glib::make_refptr_for_instance<DiskFSNode>(new DiskFSNode(file));
    }
    
protected:
    DiskFSNode(std::shared_ptr<Disk::Folder> folder){
        mIsFolder = true;
        mFolderEntry = folder;
        mFileEntry = nullptr;

        mEntryName = folder->GetName();
        mEntrySize = "";
    }

    DiskFSNode(std::shared_ptr<Disk::File> file){
        mIsFolder = false;
        mFolderEntry = nullptr;
        mFileEntry = file;

        mEntryName = file->GetName();
        mEntrySize = std::string(bytesize::bytesize(file->GetSize()));
    }
};

// TODO Make this some sort of template so that both dist and archive can use it without duplication?
class OpenedItem : public Gtk::ScrolledWindow {
public:
    bool mIsArchive { true };
    std::filesystem::path mOpenedPath;
    Gtk::ColumnView* mView { nullptr };
    std::shared_ptr<Disk::Image> mDisk { nullptr };
    std::shared_ptr<Archive::Rarc> mArchive { nullptr };

    guint mSelectedRow;
    Glib::RefPtr<Gtk::TreeStore> mTreeStore;
    Glib::RefPtr<Gtk::SingleSelection> mSelection;
    Glib::RefPtr<Gtk::TreeListModel> mTreeListModel;

    Compression::Format mCompressionFmt;

    void OnCreateItem(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void OnCreateNoExpander(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void OnCreateIconItem(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void OnBindName(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void OnBindSize(const Glib::RefPtr<Gtk::ListItem>& list_item);

    void RebuildModel();

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

class SettingsDialog : public Gtk::Dialog {
public:
    void OnMessageFinish(const Glib::RefPtr<Gio::AsyncResult>& result, const Glib::RefPtr<SettingsDialog>& dialog);
    SettingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    ~SettingsDialog() override;

    Glib::RefPtr<Gtk::Builder> Builder() { return mBuilder; }

protected:
    Glib::RefPtr<Gtk::Builder> mBuilder;
};

class RenameDialog : public Gtk::Dialog {
public:
    void OnMessageFinish(const Glib::RefPtr<Gio::AsyncResult>& result, const Glib::RefPtr<RenameDialog>& dialog);

    RenameDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    ~RenameDialog() override;

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
    void Import(Glib::RefPtr<Gio::AsyncResult>& result);
    void Extract(Glib::RefPtr<Gio::AsyncResult>& result);

    void OnRename();
    void OnImport();
    void OnDelete();
    void OnExtract();
    void OnNewFolder();
    void ActivatedItem(guint pos);

    void TreeClicked(int n_press, double x, double y);
    Gtk::Statusbar* mStatus;
    Gtk::Notebook* mNotebook;
    Gtk::PopoverMenu mContextMenu;
    Glib::RefPtr<Gio::Menu> mTreeContextMenuModel;

    RenameDialog* mRenameDialog;
    SettingsDialog* mSettingsDialog;
    Glib::RefPtr<Gtk::RecentManager> mRecentManager;
    Glib::RefPtr<Gtk::FileDialog> mFileDialog;
    Glib::RefPtr<Gtk::Builder> mBuilder;
};

SettingsDialog* BuildSettingsDialog();
Gtk::ApplicationWindow* BuildWindow();

#endif