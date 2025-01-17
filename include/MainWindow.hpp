#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__
#include <gtkmm.h>
#include <Archive.hpp>
#include <GCM.hpp>
#include <format>
#include <cmath>
#include "bytesize.hpp"

// This is kind of dumb but it makes things more readalbe in the main UI code so, keeping it
template<typename T>
concept Folder = requires(T t){
    { t.GetName() } -> std::same_as<std::string>;
};


template<typename T>
concept File = requires(T t){
    { t.GetName() } -> std::same_as<std::string>;
    { t.GetSize() } -> std::convertible_to<std::size_t>;
    { t.GetData() } -> std::same_as<uint8_t*>;
};

template<Folder T, File U>
class FSNode : public Glib::Object {
public:
    Glib::ustring mEntryName;
    Glib::ustring mEntrySize;
    
    bool mIsFolder;

    std::shared_ptr<T> mFolderEntry;
    std::shared_ptr<U> mFileEntry;

    std::vector<FSNode> mChildren;

    Gtk::Label* mLabel;

    static Glib::RefPtr<FSNode> create(std::shared_ptr<T> folder){
        return Glib::make_refptr_for_instance<FSNode>(new FSNode<T, U>(folder));
    }

    static Glib::RefPtr<FSNode> create(std::shared_ptr<U> file){
        return Glib::make_refptr_for_instance<FSNode>(new FSNode<T, U>(file));
    }
    
protected:
    FSNode(std::shared_ptr<T> folder){
        mIsFolder = true;
        mFolderEntry = folder;
        mFileEntry = nullptr;

        mEntryName = folder->GetName();
        mEntrySize = "";
    }

    FSNode(std::shared_ptr<U> file){
        mIsFolder = false;
        mFolderEntry = nullptr;
        mFileEntry = file;

        mEntryName = file->GetName();
        mEntrySize = std::string(bytesize::bytesize(file->GetSize()));
    }
};

struct NodeAccessor {
    Glib::RefPtr<FSNode<Disk::Folder, Disk::File>> mDisk { nullptr };
    Glib::RefPtr<FSNode<Archive::Folder, Archive::File>> mArc { nullptr };

    void Delete(NodeAccessor child){
        if(mArc != nullptr){
            if(child.IsFolder()){
                auto iter = std::find(mArc->mFolderEntry->GetSubdirectories().begin(), mArc->mFolderEntry->GetSubdirectories().end(), child.mArc->mFolderEntry);
                if(iter != mArc->mFolderEntry->GetSubdirectories().end()){
                    mArc->mFolderEntry->GetSubdirectories().erase(iter);
                }
            } else {
                auto iter = std::find(mArc->mFolderEntry->GetFiles().begin(), mArc->mFolderEntry->GetFiles().end(), child.mArc->mFileEntry);
                if(iter != mArc->mFolderEntry->GetFiles().end()){
                    mArc->mFolderEntry->GetFiles().erase(iter);
                }
            }
        } else {
            if(child.IsFolder()){
                auto iter = std::find(mDisk->mFolderEntry->GetSubdirectories().begin(), mDisk->mFolderEntry->GetSubdirectories().end(), child.mDisk->mFolderEntry);
                if(iter != mDisk->mFolderEntry->GetSubdirectories().end()){
                    mDisk->mFolderEntry->GetSubdirectories().erase(iter);
                }
            } else {
                auto iter = std::find(mDisk->mFolderEntry->GetFiles().begin(), mDisk->mFolderEntry->GetFiles().end(), child.mDisk->mFileEntry);
                if(iter != mDisk->mFolderEntry->GetFiles().end()){
                    mDisk->mFolderEntry->GetFiles().erase(iter);
                }
            }            
        }
    }

    void AddSubdirectory(std::string name){
        if(mArc != nullptr){
            std::shared_ptr<Archive::Folder> newFolder = Archive::Folder::Create(mArc->mFolderEntry->GetArchive().lock());
            newFolder->SetName("New Folder");
            mArc->mFolderEntry->AddSubdirectory(newFolder);
        } else {
            std::shared_ptr<Disk::Folder> newFolder = Disk::Folder::Create(mDisk->mFolderEntry->GetDisk());
            newFolder->SetName("New Folder");
            mDisk->mFolderEntry->AddSubdirectory(newFolder);
        }
    }

    void AddFile(std::string name, uint8_t* data, std::size_t size){
        if(mArc != nullptr){
            std::shared_ptr<Archive::File> newFile = Archive::File::Create();
            newFile->SetName(name);
            newFile->SetData(data, size);
            mArc->mFolderEntry->AddFile(newFile);
        } else {
            std::shared_ptr<Disk::File> newFile = Disk::File::Create();
            newFile->SetName(name);
            newFile->SetData(data, size);
            mDisk->mFolderEntry->AddFile(newFile);
        }
    }

    Gtk::Label* GetLabel(){
        if(mArc != nullptr){
            return mArc->mLabel;
        } else {
            return mDisk->mLabel;
        }        
    }

    void SetLabel(Gtk::Label* l){
        if(mArc != nullptr){
            mArc->mLabel = l;
        } else {
            mDisk->mLabel = l;
        }
    }

    bool& IsFolder(){
        if(mArc != nullptr){
            return mArc->mIsFolder;
        } else {
            return mDisk->mIsFolder;
        }
    }

    Glib::ustring& GetName() {
        if(mArc != nullptr){
            return mArc->mEntryName;
        } else {
            return mDisk->mEntryName;
        }
    }

    void SetName(std::string s) {
        if(mArc != nullptr){
            mArc->mEntryName = s;
            if(mArc->mIsFolder){
                mArc->mFolderEntry->SetName(s);
            } else {
                mArc->mFileEntry->SetName(s);
            }
        } else {
            mDisk->mEntryName = s;
            if(mDisk->mIsFolder){
                mDisk->mFolderEntry->SetName(s);
            } else {
                mDisk->mFileEntry->SetName(s);
            }
        }
    }

    uint8_t* GetData(){
        if(mArc != nullptr){
            return mArc->mFileEntry->GetData();
        } else {
            return mDisk->mFileEntry->GetData();
        }
    }

    void SetData(uint8_t* data, uint32_t size){
        if(mArc != nullptr){
            mArc->mFileEntry->SetData(data, size);
        } else {
            mDisk->mFileEntry->SetData(data, size);
        }
    }

    std::size_t GetSize() {
        if(mArc != nullptr){
            return mArc->mFileEntry->GetSize();
        } else {
            return mDisk->mFileEntry->GetSize();
        }
    }

    Glib::ustring& GetSizeStr() {
        if(mArc != nullptr){
            return mArc->mEntrySize;
        } else {
            return mDisk->mEntrySize;
        }
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

    Gtk::PopoverMenu* mContextMenu;
    Glib::RefPtr<Gtk::TreeListRow> mSelectedRow;
    Glib::RefPtr<Gtk::TreeStore> mTreeStore;
    Glib::RefPtr<Gtk::SingleSelection> mSelection;
    Glib::RefPtr<Gtk::TreeListModel> mTreeListModel;

    Compression::Format mCompressionFmt { Compression::Format::None };

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