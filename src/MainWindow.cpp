#include "MainWindow.hpp"
#include <iostream>
#include <Bti.hpp>

void ExtractDir(std::shared_ptr<Archive::Folder> dir){
    std::filesystem::create_directory(dir->GetName());
    
    auto cur = std::filesystem::current_path();
    std::filesystem::current_path(dir->GetName());
    for(auto folder : dir->GetSubdirectories()){
        ExtractDir(folder);
    }

    for(auto file : dir->GetFiles()){
        bStream::CFileStream stream(file->GetName(), bStream::Endianess::Big, bStream::OpenMode::Out);
        stream.writeBytes(file->GetData(), file->GetSize());
    }
    std::filesystem::current_path(cur);
}

void ExtractDir(std::shared_ptr<Disk::Folder> dir){
    std::filesystem::create_directory(dir->GetName());
    
    auto cur = std::filesystem::current_path();
    std::filesystem::current_path(dir->GetName());
    for(auto folder : dir->GetSubdirectories()){
        ExtractDir(folder);
    }

    for(auto file : dir->GetFiles()){
        bStream::CFileStream stream(file->GetName(), bStream::Endianess::Big, bStream::OpenMode::Out);
        stream.writeBytes(file->GetData(), file->GetSize());
    }
    std::filesystem::current_path(cur);
}

Glib::RefPtr<Gio::ListModel> OpenedItem::CreateArchiveTreeModel(const Glib::RefPtr<Glib::ObjectBase>& item){
    auto col = std::dynamic_pointer_cast<ArchiveFSNode>(item);
    if (col && !col->mIsFolder)
        return {};

    auto result = Gio::ListStore<ArchiveFSNode>::create();
    
    if(col){
        for(auto folder : col->mFolderEntry->GetSubdirectories()){
            result->append(ArchiveFSNode::create(folder));
        }

        for(auto file : col->mFolderEntry->GetFiles()){
            result->append(ArchiveFSNode::create(file));
        }
    } else {
        result->append(ArchiveFSNode::create(mArchive->GetRoot()));
    }
    
    return result;
}

Glib::RefPtr<Gio::ListModel> OpenedItem::CreateDiskTreeModel(const Glib::RefPtr<Glib::ObjectBase>& item){
    auto col = std::dynamic_pointer_cast<DiskFSNode>(item);
    if (col && !col->mIsFolder)
        return {};

    auto result = Gio::ListStore<DiskFSNode>::create();
    
    if(col){
        for(auto folder : col->mFolderEntry->GetSubdirectories()){
            result->append(DiskFSNode::create(folder));
        }

        for(auto file : col->mFolderEntry->GetFiles()){
            result->append(DiskFSNode::create(file));
        }
    } else {
        result->append(DiskFSNode::create(mDisk->GetRoot()));
    }
  
  return result;
}

void OpenedItem::OnCreateItem(const Glib::RefPtr<Gtk::ListItem>& list_item){
    // Each ListItem contains a TreeExpander, which contains a Label.
    auto expander = Gtk::make_managed<Gtk::TreeExpander>();
    auto label = Gtk::make_managed<Gtk::Label>();
    label->set_halign(Gtk::Align::START);
    expander->set_child(*label);
    list_item->set_child(*expander);
}

void OpenedItem::OnCreateNoExpander(const Glib::RefPtr<Gtk::ListItem>& list_item){
    // Each ListItem contains a TreeExpander, which contains a Label.
    auto label = Gtk::make_managed<Gtk::Label>();
    label->set_halign(Gtk::Align::START);
    list_item->set_child(*label);
}

void OpenedItem::OnCreateIconItem(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto expander = Gtk::make_managed<Gtk::TreeExpander>();
    auto box = Gtk::make_managed<Gtk::Box>();
    auto image = Gtk::make_managed<Gtk::Image>();
    auto label = Gtk::make_managed<Gtk::Label>();
    image->set_halign(Gtk::Align::START);
    image->set_margin_end(10);
    label->set_halign(Gtk::Align::START);
    box->prepend(*label);
    box->prepend(*image);
    expander->set_child(*box);
    list_item->set_child(*expander);
}

void OpenedItem::OnBindName(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row) return;

    auto expander = dynamic_cast<Gtk::TreeExpander*>(list_item->get_child());
    if (!expander) return;

    expander->set_list_row(row);
        
    auto box = dynamic_cast<Gtk::Box*>(expander->get_child());
    if(!box) return;

    auto image = dynamic_cast<Gtk::Image*>(box->get_first_child());
    if(!image) return;

    if(mIsArchive) {
        auto col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
        if (!col) return;

        if(col->mIsFolder){
            image->set_from_icon_name("folder");
        } else {
            if(std::filesystem::path(col->mEntryName).extension() == ".bti"){
                Bti img;
                bStream::CMemoryStream stream(col->mFileEntry->GetData(), col->mFileEntry->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
                
                if(img.Load(&stream)){
                    auto pixbuf = Gdk::Pixbuf::create_from_data(img.GetData(), Gdk::Colorspace::RGB, true, 8, img.mWidth, img.mHeight, img.mWidth*4);
                    float ratio = std::min(static_cast<float>(32) / static_cast<float>(img.mWidth), static_cast<float>(32) / static_cast<float>(img.mHeight));
                    image->set_pixel_size(32);
                    image->set(pixbuf->scale_simple(static_cast<int>(img.mWidth * ratio), static_cast<int>(img.mHeight * ratio), Gdk::InterpType::NEAREST));
                }
            } else if(std::filesystem::path(col->mEntryName).extension() == ".tpl"){
                Tpl img;
                bStream::CMemoryStream stream(col->mFileEntry->GetData(), col->mFileEntry->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
                
                if(img.Load(&stream)){
                    auto pixbuf = Gdk::Pixbuf::create_from_data(img.GetImage(0)->GetData(), Gdk::Colorspace::RGB, true, 8, img.GetImage(0)->mWidth, img.GetImage(0)->mHeight, img.GetImage(0)->mWidth*4);
                    float ratio = std::min(static_cast<float>(32) / static_cast<float>(img.GetImage(0)->mWidth), static_cast<float>(32) / static_cast<float>(img.GetImage(0)->mHeight));
                    image->set_pixel_size(32);
                    image->set(pixbuf->scale_simple(static_cast<int>(img.GetImage(0)->mWidth * ratio), static_cast<int>(img.GetImage(0)->mHeight * ratio), Gdk::InterpType::NEAREST));
                }
            }
        }

        auto label = dynamic_cast<Gtk::Label*>(box->get_last_child());
        if (!label) return;
        label->set_text(col->mEntryName);
        col->mLabel = label;
    } else {
        auto col = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
        if (!col) return;

        if(col->mIsFolder){
            image->set_from_icon_name("folder");
        } else {
            if(std::filesystem::path(col->mEntryName).extension() == ".bti"){
                Bti img;
                bStream::CMemoryStream stream(col->mFileEntry->GetData(), col->mFileEntry->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
                
                if(img.Load(&stream)){
                    auto pixbuf = Gdk::Pixbuf::create_from_data(img.GetData(), Gdk::Colorspace::RGB, true, 8, img.mWidth, img.mHeight, img.mWidth*4);
                    float ratio = std::min(static_cast<float>(32) / static_cast<float>(img.mWidth), static_cast<float>(32) / static_cast<float>(img.mHeight));
                    image->set_pixel_size(32);
                    image->set(pixbuf->scale_simple(static_cast<int>(img.mWidth * ratio), static_cast<int>(img.mHeight * ratio), Gdk::InterpType::NEAREST));
                }
            } else if(std::filesystem::path(col->mEntryName).extension() == ".tpl"){
                Tpl img;
                bStream::CMemoryStream stream(col->mFileEntry->GetData(), col->mFileEntry->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
                
                if(img.Load(&stream)){
                    auto pixbuf = Gdk::Pixbuf::create_from_data(img.GetImage(0)->GetData(), Gdk::Colorspace::RGB, true, 8, img.GetImage(0)->mWidth, img.GetImage(0)->mHeight, img.GetImage(0)->mWidth*4);
                    float ratio = std::min(static_cast<float>(32) / static_cast<float>(img.GetImage(0)->mWidth), static_cast<float>(32) / static_cast<float>(img.GetImage(0)->mHeight));
                    image->set_pixel_size(32);
                    image->set(pixbuf->scale_simple(static_cast<int>(img.GetImage(0)->mWidth * ratio), static_cast<int>(img.GetImage(0)->mHeight * ratio), Gdk::InterpType::NEAREST));
                }
            }
        }

        auto label = dynamic_cast<Gtk::Label*>(box->get_last_child());
        if (!label) return;
        label->set_text(col->mEntryName);
        col->mLabel = label;
    }   
}

void OpenedItem::OnBindSize(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row) return;

    if(mIsArchive){
        auto col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
        if (!col) return;
        
        auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
        if (!label) return;
        label->set_text(col->mEntrySize);
    } else {
        auto col = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
        if (!col) return;
        
        auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
        if (!label) return;
        label->set_text(col->mEntrySize);
    }
}


OpenedItem::OpenedItem(Gtk::ColumnView* view, std::shared_ptr<Archive::Rarc> arc){
    mView = view;
    mArchive = arc;
    mDisk = nullptr;

    mIsArchive = true;

    auto root = CreateArchiveTreeModel();
    mTreeListModel = Gtk::TreeListModel::create(root, sigc::mem_fun(*this, &OpenedItem::CreateArchiveTreeModel), false, false);
    mSelection = Gtk::SingleSelection::create(mTreeListModel);

    auto nameItemFactory = Gtk::SignalListItemFactory::create();
    nameItemFactory->signal_setup().connect(sigc::mem_fun(*this, &OpenedItem::OnCreateIconItem));
    nameItemFactory->signal_bind().connect(sigc::mem_fun(*this, &OpenedItem::OnBindName));

    auto column = Gtk::ColumnViewColumn::create("Name", nameItemFactory);
    column->set_expand(true);
    mView->append_column(column);

    auto sizeItemFactory = Gtk::SignalListItemFactory::create();
    sizeItemFactory->signal_setup().connect(sigc::mem_fun(*this, &OpenedItem::OnCreateNoExpander));
    sizeItemFactory->signal_bind().connect(sigc::mem_fun(*this, &OpenedItem::OnBindSize));

    column = Gtk::ColumnViewColumn::create("Size", sizeItemFactory);
    column->set_fixed_width(128);
    mView->append_column(column);

    mView->set_show_column_separators(true);

    mView->set_model(mSelection);
    set_child(*mView);
}

OpenedItem::OpenedItem(Gtk::ColumnView* view, std::shared_ptr<Disk::Image> img){
    mView = view;
    mDisk = img;
    mArchive = nullptr;

    mIsArchive = false;

    auto root = CreateDiskTreeModel();
    mTreeListModel = Gtk::TreeListModel::create(root, sigc::mem_fun(*this, &OpenedItem::CreateDiskTreeModel), false, false);
    mSelection = Gtk::SingleSelection::create(mTreeListModel);

    auto nameItemFactory = Gtk::SignalListItemFactory::create();
    nameItemFactory->signal_setup().connect(sigc::mem_fun(*this, &OpenedItem::OnCreateIconItem));
    nameItemFactory->signal_bind().connect(sigc::mem_fun(*this, &OpenedItem::OnBindName));

    auto column = Gtk::ColumnViewColumn::create("Name", nameItemFactory);
    column->set_expand(true);
    mView->append_column(column);

    auto sizeItemFactory = Gtk::SignalListItemFactory::create();
    sizeItemFactory->signal_setup().connect(sigc::mem_fun(*this, &OpenedItem::OnCreateNoExpander));
    sizeItemFactory->signal_bind().connect(sigc::mem_fun(*this, &OpenedItem::OnBindSize));

    column = Gtk::ColumnViewColumn::create("Size", sizeItemFactory);
    column->set_fixed_width(128);
    mView->append_column(column);

    mView->set_reorderable(false);
    mView->set_show_column_separators(true);

    mView->set_model(mSelection);
    set_child(*mView);
}

void OpenedItem::Save(){
    if(mArchive){
        mArchive->SaveToFile(mOpenedPath, mCompressionFmt, 7, true);
    } else if(mDisk){
        mDisk->SaveToFile(mOpenedPath);
    }
}

ItemTab::ItemTab(std::string label, Gtk::Notebook* notebook, OpenedItem* item) {
    mLabel.set_text(label);
    mIconClose.set_from_icon_name("window-close-symbolic");
    mIconClose.set_margin_start(5);

    mCloseTabClicked = Gtk::GestureClick::create();
    mCloseTabClicked->set_button(GDK_BUTTON_PRIMARY);
    mCloseTabClicked->signal_pressed().connect(sigc::mem_fun(*this, &ItemTab::CloseItem));
    mIconClose.add_controller(mCloseTabClicked);

    mPageItem = item;
    mNotebook = notebook;

    prepend(mIconClose);
    prepend(mLabel);
}

void ItemTab::CloseItem(int n_press, double x, double y){
    mNotebook->remove_page(*mPageItem);
}

void MainWindow::ActivatedItem(guint pos){
    if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
        Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
        Glib::RefPtr<Gdk::Seat> seat = display->get_default_seat();
        Glib::RefPtr<Gdk::Device> pointer = seat->get_pointer();
        dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()))->mSelectedRow = pos;
        
        double x, y;
        pointer->get_surface_at_position(x, y);
    
        mContextMenu.set_pointing_to(Gdk::Rectangle {(int)x, (int)y, 0, 0});
        mContextMenu.popup();
    }
}

void MainWindow::OnNew(){
    std::shared_ptr<Archive::Rarc> arc = Archive::Rarc::Create();
    auto root = Archive::Folder::Create(arc);
    root->SetName("archive");
    arc->SetRoot(root);

    Gtk::ColumnView* columnView = Gtk::make_managed<Gtk::ColumnView>();
    columnView->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::ActivatedItem));
    columnView->set_expand();
    
    OpenedItem* scroller = Gtk::make_managed<OpenedItem>(columnView, arc);
    scroller->set_has_frame(false);
    scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    scroller->set_expand();

    auto tab = Gtk::make_managed<ItemTab>("archive", mNotebook, scroller);
    mNotebook->append_page(*scroller, *tab);
    mNotebook->set_tab_reorderable(*scroller);
}

void MainWindow::OpenArchive(Glib::RefPtr<Gio::AsyncResult>& result){
    Glib::RefPtr<Gio::File> file;

    try {
        file = mFileDialog->open_finish(result);
    } catch(const Gtk::DialogError& err) {
        return;
    }

    //check extension to see if we should be opening an iso?
    std::string extension = std::filesystem::path(file->get_path()).extension();
    bStream::CFileStream stream(file->get_path(), bStream::Endianess::Big, bStream::OpenMode::In);
    
    if(extension == ".arc" || extension == ".rarc" || extension == ".szp" || extension == ".szs"){
        std::shared_ptr arc = Archive::Rarc::Create();
        if(!arc->Load(&stream)){
            // show fail dialog
            mStatus->push(std::format("Failed to open archive {} ", file->get_path()));
        } else {
            mStatus->push(std::format("Opened archive {} ", file->get_path()));

            Gtk::ColumnView* columnView = Gtk::make_managed<Gtk::ColumnView>();
            columnView->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::ActivatedItem));
            columnView->set_expand();

            OpenedItem* scroller = Gtk::make_managed<OpenedItem>(columnView, arc);
            scroller->set_has_frame(false);
            scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
            scroller->set_expand();    
            
            scroller->mOpenedPath = file->get_path();

            uint32_t magic = stream.peekUInt32(0);
            if(magic == 1499560496){
                scroller->mCompressionFmt = Compression::Format::YAZ0;
            } else if(magic == 1499560240){
                scroller->mCompressionFmt = Compression::Format::YAY0;
            } else {
                scroller->mCompressionFmt = Compression::Format::None;
            }

            auto tab = Gtk::make_managed<ItemTab>(file->get_basename(), mNotebook, scroller);
            mNotebook->append_page(*scroller, *tab);
            mNotebook->set_tab_reorderable(*scroller);
        }
    } else if(extension == ".iso" || extension == ".gcm") {
        std::shared_ptr img = Disk::Image::Create();
        if(!img->Load(&stream)){
            // show fail dialog
            mStatus->push(std::format("Failed to open image {} ", file->get_path()));
        } else {
            mStatus->push(std::format("Opened image {} ", file->get_path()));

            Gtk::ColumnView* columnView = Gtk::make_managed<Gtk::ColumnView>();
            columnView->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::ActivatedItem));
            columnView->set_expand();

            OpenedItem* scroller = Gtk::make_managed<OpenedItem>(columnView, img);
            scroller->set_has_frame(false);
            scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
            scroller->set_expand();

            auto tab = Gtk::make_managed<ItemTab>(file->get_basename(), mNotebook, scroller);
            mNotebook->append_page(*scroller, *tab);
            mNotebook->set_tab_reorderable(*scroller);
        }
    }

}

void MainWindow::OnOpen(){
    if(!mFileDialog){
        mFileDialog = Gtk::FileDialog::create();
        mFileDialog->set_modal(true);
    }

    mFileDialog->open(*this, sigc::mem_fun(*this, &MainWindow::OpenArchive));

}

void MainWindow::OnSave(){
    int page = mNotebook->get_current_page();
    dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(page))->Save();
}

void MainWindow::OnSaveAs(){

}

void MainWindow::OnRename(){
    if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
        OpenedItem* opened = dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()));

        auto row = opened->mTreeListModel->get_row(opened->mSelectedRow);
        if(row->get_item() == nullptr) return;

        std::string name;
        if(opened->mIsArchive){
            auto col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
            if(col->mIsFolder){
                name = col->mFolderEntry->GetName();
            } else {
                name = col->mFileEntry->GetName();
            }
        } else {
            auto col = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
            if(col->mIsFolder){
                name = col->mFolderEntry->GetName();
            } else {
                name = col->mFileEntry->GetName();
            }
        }

        mBuilder->get_widget<Gtk::Text>("nameBox")->get_buffer()->set_text(name);
        
        mRenameDialog->set_transient_for(*this);
        mRenameDialog->set_modal(true);
        mRenameDialog->set_visible(true);
    }
}

void MainWindow::OnOpenSettings(){
    mSettingsDialog->set_transient_for(*this);
    mSettingsDialog->set_modal(true);
    mSettingsDialog->set_visible(true);
}

void MainWindow::OnQuit(){
    set_visible(false);
}

void MainWindow::Import(Glib::RefPtr<Gio::AsyncResult>& result){
    Glib::RefPtr<Gio::File> file;

    try {
        file = mFileDialog->open_finish(result);
    } catch(const Gtk::DialogError& err) {
        return;
    }
    
    bStream::CFileStream stream(file->get_path(), bStream::Endianess::Big, bStream::OpenMode::In);
    std::size_t size = stream.getSize();

    uint8_t* data = new uint8_t[size];
    stream.readBytesTo(data, size);

    if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
        OpenedItem* opened = dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()));

        auto row = opened->mTreeListModel->get_row(opened->mSelectedRow);
        if(opened->mIsArchive){
            auto col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
            if(!col->mIsFolder){
                col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_parent()->get_item());
            } 
            
            std::shared_ptr<Archive::File> newFile = Archive::File::Create();
            newFile->SetName(file->get_basename());
            newFile->SetData(data, size);
            col->mFolderEntry->AddFile(newFile);
        } else {
            auto col = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
            if(!col->mIsFolder){
                col = std::dynamic_pointer_cast<DiskFSNode>(row->get_parent()->get_item());
            } 
            
            std::shared_ptr<Disk::File> newFile = Disk::File::Create();
            newFile->SetName(file->get_basename());
            newFile->SetData(data, size);
            col->mFolderEntry->AddFile(newFile);
        }

        if(row->get_expanded()){
            row->set_expanded(false);
            row->set_expanded(true);    
        }
        row->set_expanded(true);

    }

    delete data;
}

void MainWindow::OnImport(){
    if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
        OpenedItem* opened = dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()));

        auto row = opened->mTreeListModel->get_row(opened->mSelectedRow);
        if(row->get_item() == nullptr) return;

        if(!mFileDialog){
            mFileDialog = Gtk::FileDialog::create();
            mFileDialog->set_modal(true);
        }
        
        mFileDialog->open(*this, sigc::mem_fun(*this, &MainWindow::Import));
    }
}

void MainWindow::OnDelete(){
    if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
        OpenedItem* opened = dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()));

        auto row = opened->mTreeListModel->get_row(opened->mSelectedRow);
        if(row->get_item() == nullptr) return;

        auto parent_row = row->get_parent();
        if(parent_row->get_item() == nullptr) return;
        
        if(opened->mIsArchive){
            auto parent = std::dynamic_pointer_cast<ArchiveFSNode>(parent_row->get_item());
            auto child = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
            if(!parent->mIsFolder) return;

            if(child->mIsFolder){
                auto iter = std::find(parent->mFolderEntry->GetSubdirectories().begin(), parent->mFolderEntry->GetSubdirectories().end(), child->mFolderEntry);
                if(iter != parent->mFolderEntry->GetSubdirectories().end()){
                    parent->mFolderEntry->GetSubdirectories().erase(iter);
                }
            } else {
                auto iter = std::find(parent->mFolderEntry->GetFiles().begin(), parent->mFolderEntry->GetFiles().end(), child->mFileEntry);
                if(iter != parent->mFolderEntry->GetFiles().end()){
                    parent->mFolderEntry->GetFiles().erase(iter);
                }
            }
        } else {
            auto parent = std::dynamic_pointer_cast<DiskFSNode>(parent_row->get_item());
            auto child = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
            if(!parent->mIsFolder) return;

            if(child->mIsFolder){
                auto iter = std::find(parent->mFolderEntry->GetSubdirectories().begin(), parent->mFolderEntry->GetSubdirectories().end(), child->mFolderEntry);
                if(iter != parent->mFolderEntry->GetSubdirectories().end()){
                    parent->mFolderEntry->GetSubdirectories().erase(iter);
                }
            } else {
                auto iter = std::find(parent->mFolderEntry->GetFiles().begin(), parent->mFolderEntry->GetFiles().end(), child->mFileEntry);
                if(iter != parent->mFolderEntry->GetFiles().end()){
                    parent->mFolderEntry->GetFiles().erase(iter);
                }
            }
        }
        auto parent = row->get_parent();
        if(parent != nullptr){
            parent->set_expanded(false);
            parent->set_expanded(true);    
        }
    }
}

// dont really like this
void MainWindow::Extract(Glib::RefPtr<Gio::AsyncResult>& result){
    Glib::RefPtr<Gio::File> file;
    if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
        OpenedItem* opened = dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()));

        auto row = opened->mTreeListModel->get_row(opened->mSelectedRow);
        if(opened->mIsArchive){
            auto col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
            if(!col->mIsFolder){                
                try {
                    file = mFileDialog->save_finish(result);
                } catch(const Gtk::DialogError& err) {
                    return;
                }

                bStream::CFileStream stream(file->get_path(), bStream::Endianess::Big, bStream::OpenMode::Out);
                stream.writeBytes(col->mFileEntry->GetData(), col->mFileEntry->GetSize());
            } else {
                try {
                    file = mFileDialog->select_folder_finish(result);
                } catch(const Gtk::DialogError& err) {
                    return;
                }

                auto cur = std::filesystem::current_path();
                std::filesystem::current_path(std::filesystem::path(file->get_path()));
                ExtractDir(col->mFolderEntry);
                std::filesystem::current_path(cur);
            }
        } else {
            auto col = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
            if(!col->mIsFolder){
                try {
                    file = mFileDialog->save_finish(result);
                } catch(const Gtk::DialogError& err) {
                    return;
                }

                bStream::CFileStream stream(file->get_path(), bStream::Endianess::Big, bStream::OpenMode::Out);
                stream.writeBytes(col->mFileEntry->GetData(), col->mFileEntry->GetSize());
            } else {
                try {
                    file = mFileDialog->select_folder_finish(result);
                } catch(const Gtk::DialogError& err) {
                    return;
                }

                auto cur = std::filesystem::current_path();
                std::filesystem::current_path(std::filesystem::path(file->get_path()));
                ExtractDir(col->mFolderEntry);
                std::filesystem::current_path(cur);
            }
        }

    }
}

void MainWindow::OnExtract(){
    if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
        OpenedItem* opened = dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()));

        auto row = opened->mTreeListModel->get_row(opened->mSelectedRow);
        if(row->get_item() == nullptr) return;

        if(!mFileDialog){
            mFileDialog = Gtk::FileDialog::create();
            mFileDialog->set_modal(true);
        }
        
        if(opened->mIsArchive){
            auto col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
            if(!col->mIsFolder){
                mFileDialog->set_initial_name(col->mEntryName);
                mFileDialog->save(*this, sigc::mem_fun(*this, &MainWindow::Extract));
            } else {
                mFileDialog->select_folder(*this, sigc::mem_fun(*this, &MainWindow::Extract));
            }
        } else {
            auto col = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
            if(!col->mIsFolder){
                mFileDialog->set_initial_name(col->mEntryName);
                mFileDialog->save(*this, sigc::mem_fun(*this, &MainWindow::Extract));
            } else {
                mFileDialog->select_folder(*this, sigc::mem_fun(*this, &MainWindow::Extract));
            }
        }
    }
}

void MainWindow::OnNewFolder(){
    if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
        OpenedItem* opened = dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()));

        auto row = opened->mTreeListModel->get_row(opened->mSelectedRow);
        if(row->get_item() == nullptr) return;
        
        if(opened->mIsArchive){
            auto col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
            if(col->mIsFolder){
                std::shared_ptr<Archive::Folder> newFolder = Archive::Folder::Create(col->mFolderEntry->GetArchive().lock());
                newFolder->SetName("New Folder");
                col->mFolderEntry->AddSubdirectory(newFolder);
            }
        } else {
            auto col = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
            if(col->mIsFolder){
                std::shared_ptr<Disk::Folder> newFolder = Disk::Folder::Create(col->mFolderEntry->GetDisk());
                newFolder->SetName("New Folder");
                col->mFolderEntry->AddSubdirectory(newFolder);            
            }
        }
        if(row->get_expanded()){
            row->set_expanded(false);
            row->set_expanded(true);    
        }
        row->set_expanded(true);
    }
}


MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::ApplicationWindow(cobject), mBuilder(builder){
    auto menuActions = Gio::SimpleActionGroup::create();
    
    menuActions->add_action("new", sigc::mem_fun(*this, &MainWindow::OnNew));
    menuActions->add_action("open", sigc::mem_fun(*this, &MainWindow::OnOpen));
    menuActions->add_action("save", sigc::mem_fun(*this, &MainWindow::OnSave));
    menuActions->add_action("saveas", sigc::mem_fun(*this, &MainWindow::OnSaveAs));
    menuActions->add_action("quit", sigc::mem_fun(*this, &MainWindow::OnQuit));

    menuActions->add_action("settings", sigc::mem_fun(*this, &MainWindow::OnOpenSettings));

    insert_action_group("menuactions", menuActions);

    auto contextMenuAction = Gio::SimpleActionGroup::create();
    
    contextMenuAction->add_action("rename", sigc::mem_fun(*this, &MainWindow::OnRename));
    contextMenuAction->add_action("import", sigc::mem_fun(*this, &MainWindow::OnImport));
    contextMenuAction->add_action("extract", sigc::mem_fun(*this, &MainWindow::OnExtract));
    contextMenuAction->add_action("delete", sigc::mem_fun(*this, &MainWindow::OnDelete));
    contextMenuAction->add_action("newfolder", sigc::mem_fun(*this, &MainWindow::OnNewFolder));

    insert_action_group("fs", contextMenuAction);

    mStatus = builder->get_widget<Gtk::Statusbar>("gctoolsStatusBar");
    mNotebook = builder->get_widget<Gtk::Notebook>("openedPages");

    mTreeContextMenuModel = builder->get_object<Gio::Menu>("ctxMenuModel");
    mContextMenu.set_parent(*this);
    mContextMenu.set_menu_model(mTreeContextMenuModel);
    mContextMenu.set_flags(Gtk::PopoverMenu::Flags::NESTED);
    mContextMenu.set_has_arrow(false);

    mRenameDialog = Gtk::Builder::get_widget_derived<RenameDialog>(builder, "renameDialog");
    mBuilder->get_widget<Gtk::Button>("applyNameBtn")->signal_clicked().connect([&](){
        std::string newName = mBuilder->get_widget<Gtk::Text>("nameBox")->get_buffer()->get_text();

        if(mNotebook->get_nth_page(mNotebook->get_current_page()) != nullptr){
            OpenedItem* opened = dynamic_cast<OpenedItem*>(mNotebook->get_nth_page(mNotebook->get_current_page()));

            auto row = opened->mTreeListModel->get_row(opened->mSelectedRow);
            if(row->get_item() == nullptr) return;
            
            if(opened->mIsArchive){
                auto col = std::dynamic_pointer_cast<ArchiveFSNode>(row->get_item());
                col->mLabel->set_text(newName);
                col->mEntryName = newName;
                if(col->mIsFolder){
                    col->mFolderEntry->SetName(newName);
                } else {
                    col->mFileEntry->SetName(newName);
                }
            } else {
                auto col = std::dynamic_pointer_cast<DiskFSNode>(row->get_item());
                col->mLabel->set_text(newName);
                col->mEntryName = newName;
                if(col->mIsFolder){
                    col->mFolderEntry->SetName(newName);
                } else {
                    col->mFileEntry->SetName(newName);
                }
            }
            
        }

        mRenameDialog->set_visible(false);
    });

    mSettingsDialog = BuildSettingsDialog();
    
    mSettingsDialog->Builder()->get_widget<Gtk::Button>("applyButton")->signal_clicked().connect([&](){
        mSettingsDialog->set_visible(false);
    });
}

MainWindow::~MainWindow(){
    delete mSettingsDialog;
}

void RenameDialog::OnMessageFinish(const Glib::RefPtr<Gio::AsyncResult>& result, const Glib::RefPtr<RenameDialog>& dialog){

}

RenameDialog::RenameDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(cobject), mBuilder(builder){
}

RenameDialog::~RenameDialog(){
}

void SettingsDialog::OnMessageFinish(const Glib::RefPtr<Gio::AsyncResult>& result, const Glib::RefPtr<SettingsDialog>& dialog){

}

SettingsDialog::SettingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(cobject), mBuilder(builder){
}

SettingsDialog::~SettingsDialog(){
}

SettingsDialog* BuildSettingsDialog(){
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_file("settings.ui");
    } catch (const Glib::Error& error) {
        std::cout << "Error loading settings.ui: " << error.what() << std::endl;
        return nullptr;
    }

    auto window = Gtk::Builder::get_widget_derived<SettingsDialog>(builder, "settingsDialog");
    if (!window){
        std::cout << "Could not get 'window' from the builder." << std::endl;
        return nullptr;
    }
    
    return window;
}

Gtk::ApplicationWindow* BuildWindow(){
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_file("gctools.ui");
    } catch (const Glib::Error& error) {
        std::cout << "Error loading gctools.ui: " << error.what() << std::endl;
        return nullptr;
    }

    auto window = Gtk::Builder::get_widget_derived<MainWindow>(builder, "mainWin");
    if (!window){
        std::cout << "Could not get 'window' from the builder." << std::endl;
        return nullptr;
    }
    
    return window;
}