#include "MainWindow.hpp"
#include <iostream>
#include "bytesize.hpp"
#include <Bti.hpp>

Glib::RefPtr<Gtk::PopoverMenu> ctxMenu { nullptr };

CellItemFilesystemNode::CellItemFilesystemNode(Glib::ustring entryName, Glib::ustring entrySize, std::filesystem::path entryPath, bool isFolder, std::shared_ptr<Archive::Folder> folderEntry, std::shared_ptr<Archive::File> fileEntry, std::shared_ptr<Disk::Folder> diskFolderEntry, std::shared_ptr<Disk::File> diskFileEntry){
    mEntryName = entryName;
    mEntryPath = entryPath;
    mIsFolder = isFolder;
    if(mIsFolder){
        mDiskFolderEntry = diskFolderEntry;
        mFolderEntry = folderEntry;
        mFileEntry = nullptr;
        mEntrySize = "";
    } else {
        mDiskFileEntry = diskFileEntry;
        mEntrySize = entrySize;
        mFileEntry = fileEntry;
        mFolderEntry = nullptr;
    }
}

CellItemFilesystemNode::CellItemFilesystemNode(Glib::ustring entryName, const std::vector<CellItemFilesystemNode>& children){
    mEntryName = entryName;
    mEntrySize = "";
    mEntryPath = "";
    mIsFolder = false;
    mFolderEntry = nullptr;
    mFileEntry = nullptr;
    mDiskFolderEntry = nullptr;
    mDiskFileEntry = nullptr;
    mChildren = children;
}

void OpenedItem::AddDirectoryNodeDisk(std::vector<CellItemFilesystemNode>& items, std::shared_ptr<Disk::Folder> directory, std::filesystem::path curPath){
    CellItemFilesystemNode node(directory->GetName(), "", curPath / directory->GetName(), true, nullptr, nullptr, directory, nullptr);
    
    for(auto folder : directory->GetSubdirectories()){
        AddDirectoryNodeDisk(node.mChildren, folder, curPath / directory->GetName());
    }

    for(auto file : directory->GetFiles()){
        node.mChildren.push_back(CellItemFilesystemNode(file->GetName(), std::string(bytesize::bytesize(file->GetSize())), curPath / file->GetName(), false, nullptr, nullptr, nullptr, file));
    }

    items.push_back(node);
}

void OpenedItem::AddDirectoryNodeArchive(std::vector<CellItemFilesystemNode>& items, std::shared_ptr<Archive::Folder> directory, std::filesystem::path curPath){
    CellItemFilesystemNode node(directory->GetName(), "", curPath / directory->GetName(), true, directory, nullptr, nullptr, nullptr);
    
    for(auto folder : directory->GetSubdirectories()){
        AddDirectoryNodeArchive(node.mChildren, folder, curPath / directory->GetName());
    }

    for(auto file : directory->GetFiles()){
        node.mChildren.push_back(CellItemFilesystemNode(file->GetName(), std::string(bytesize::bytesize(file->GetSize())), curPath / file->GetName(), false, nullptr, file, nullptr, nullptr));
    }

    items.push_back(node);
}

Glib::RefPtr<Gio::ListModel> OpenedItem::CreateArchiveTreeModel(const Glib::RefPtr<Glib::ObjectBase>& item){
  auto col = std::dynamic_pointer_cast<ModelColumns>(item);
  if (col && col->mChildren.empty())
    // An item without children, i.e. a leaf in the tree. 
    return {};

  auto result = Gio::ListStore<ModelColumns>::create();
  
  const std::vector<CellItemFilesystemNode>& children = col ? col->mChildren : mArchiveItems;
  for (const auto& child : children) result->append(ModelColumns::create(child));
  
  return result;
}

Glib::RefPtr<Gio::ListModel> OpenedItem::CreateDiskTreeModel(const Glib::RefPtr<Glib::ObjectBase>& item){
  auto col = std::dynamic_pointer_cast<ModelColumns>(item);
  if (col && col->mChildren.empty())
    // An item without children, i.e. a leaf in the tree. 
    return {};

  auto result = Gio::ListStore<ModelColumns>::create();
  
  const std::vector<CellItemFilesystemNode>& children = col ? col->mChildren : mDiskItems;
  for (const auto& child : children) result->append(ModelColumns::create(child));
  
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

    auto col = std::dynamic_pointer_cast<ModelColumns>(row->get_item());
    if (!col) return;
    
    auto expander = dynamic_cast<Gtk::TreeExpander*>(list_item->get_child());
    if (!expander) return;

    expander->set_list_row(row);
    
    auto box = dynamic_cast<Gtk::Box*>(expander->get_child());
    if(!box) return;

    auto image = dynamic_cast<Gtk::Image*>(box->get_first_child());
    if(!image) return;

    if(col->mIsFolder){
        image->set_from_icon_name("folder");
    } else {
        if(mIsArchive){
            if(col->mEntryPath.extension() == ".bti"){
                Bti img;
                bStream::CMemoryStream stream(col->mFileEntry->GetData(), col->mFileEntry->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
                
                if(img.Load(&stream)){
                    auto pixbuf = Gdk::Pixbuf::create_from_data(img.GetData(), Gdk::Colorspace::RGB, true, 8, img.mWidth, img.mHeight, img.mWidth*4);
                    float ratio = std::min(static_cast<float>(32) / static_cast<float>(img.mWidth), static_cast<float>(32) / static_cast<float>(img.mHeight));
                    image->set_pixel_size(32);
                    image->set(pixbuf->scale_simple(static_cast<int>(img.mWidth * ratio), static_cast<int>(img.mHeight * ratio), Gdk::InterpType::NEAREST));
                }

            } else if(col->mEntryPath.extension() == ".tpl"){
                Tpl img;
                bStream::CMemoryStream stream(col->mFileEntry->GetData(), col->mFileEntry->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
                
                if(img.Load(&stream)){
                    auto pixbuf = Gdk::Pixbuf::create_from_data(img.GetImage(0)->GetData(), Gdk::Colorspace::RGB, true, 8, img.GetImage(0)->mWidth, img.GetImage(0)->mHeight, img.GetImage(0)->mWidth*4);
                    float ratio = std::min(static_cast<float>(32) / static_cast<float>(img.GetImage(0)->mWidth), static_cast<float>(32) / static_cast<float>(img.GetImage(0)->mHeight));
                    image->set_pixel_size(32);
                    image->set(pixbuf->scale_simple(static_cast<int>(img.GetImage(0)->mWidth * ratio), static_cast<int>(img.GetImage(0)->mHeight * ratio), Gdk::InterpType::NEAREST));
                }
            }
        } else {
            if(col->mEntryPath.extension() == ".bti"){
                Bti img;
                bStream::CMemoryStream stream(col->mDiskFileEntry->GetData(), col->mDiskFileEntry->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
                
                if(img.Load(&stream)){
                    auto pixbuf = Gdk::Pixbuf::create_from_data(img.GetData(), Gdk::Colorspace::RGB, true, 8, img.mWidth, img.mHeight, img.mWidth*4);
                    float ratio = std::min(static_cast<float>(32) / static_cast<float>(img.mWidth), static_cast<float>(32) / static_cast<float>(img.mHeight));
                    image->set_pixel_size(32);
                    image->set(pixbuf->scale_simple(static_cast<int>(img.mWidth * ratio), static_cast<int>(img.mHeight * ratio), Gdk::InterpType::NEAREST));
                }

            } else if(col->mEntryPath.extension() == ".tpl"){
                Tpl img;
                bStream::CMemoryStream stream(col->mDiskFileEntry->GetData(), col->mDiskFileEntry->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
                
                if(img.Load(&stream)){
                    auto pixbuf = Gdk::Pixbuf::create_from_data(img.GetImage(0)->GetData(), Gdk::Colorspace::RGB, true, 8, img.GetImage(0)->mWidth, img.GetImage(0)->mHeight, img.GetImage(0)->mWidth*4);
                    float ratio = std::min(static_cast<float>(32) / static_cast<float>(img.GetImage(0)->mWidth), static_cast<float>(32) / static_cast<float>(img.GetImage(0)->mHeight));
                    image->set_pixel_size(32);
                    image->set(pixbuf->scale_simple(static_cast<int>(img.GetImage(0)->mWidth * ratio), static_cast<int>(img.GetImage(0)->mHeight * ratio), Gdk::InterpType::NEAREST));
                }
            }
        }
    }

    
    auto label = dynamic_cast<Gtk::Label*>(box->get_last_child());
    if (!label) return;
    label->set_text(col->mEntryName);
}

void OpenedItem::OnBindSize(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row) return;

    auto col = std::dynamic_pointer_cast<ModelColumns>(row->get_item());
    if (!col) return;
    
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    if (!label) return;
    label->set_text(col->mEntrySize);
}


OpenedItem::OpenedItem(Gtk::ColumnView* view, std::shared_ptr<Archive::Rarc> arc){
    mView = view;
    mArchive = arc;
    mDisk = nullptr;

    mIsArchive = true;

    AddDirectoryNodeArchive(mArchiveItems, mArchive->GetRoot(), "");

    auto root = CreateArchiveTreeModel();
    mTreeListModel = Gtk::TreeListModel::create(root, sigc::mem_fun(*this, &OpenedItem::CreateArchiveTreeModel), false, true);
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

    AddDirectoryNodeDisk(mDiskItems, mDisk->GetRoot(), "");

    auto root = CreateDiskTreeModel();
    mTreeListModel = Gtk::TreeListModel::create(root, sigc::mem_fun(*this, &OpenedItem::CreateDiskTreeModel), false, true);
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


void MainWindow::TreeClicked(int n_press, double x, double y){
    // add rename?
    mContextMenu.set_menu_model(mTreeContextMenuModel);
    mContextMenu.set_pointing_to(Gdk::Rectangle {(int)x, (int)y, 0, 0});
    mContextMenu.popup();
}

void MainWindow::NotebookRightClicked(int n_press, double x, double y){
    mContextMenu.set_menu_model(mNotebookContextMenuModel);
    mContextMenu.set_pointing_to(Gdk::Rectangle {(int)x, (int)y, 0, 0});
    mContextMenu.popup();
}

void MainWindow::OnNew(){
    std::shared_ptr<Archive::Rarc> arc = Archive::Rarc::Create();
    auto root = Archive::Folder::Create(arc);
    root->SetName("archive");
    arc->SetRoot(root);

    Gtk::ColumnView* columnView = Gtk::make_managed<Gtk::ColumnView>();
    columnView->set_expand();
    
    OpenedItem* scroller = Gtk::make_managed<OpenedItem>(columnView, arc);
    scroller->set_has_frame(false);
    scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    scroller->set_expand();

    auto tab = Gtk::make_managed<ItemTab>("archive", mNotebook, scroller);
    mNotebook->append_page(*scroller, *tab);
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
            columnView->set_expand();

            OpenedItem* scroller = Gtk::make_managed<OpenedItem>(columnView, arc);
            scroller->set_has_frame(false);
            scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
            scroller->set_expand();    
            scroller->add_controller(mTreeClicked);
            
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
        }
    } else if(extension == ".iso" || extension == ".gcm") {
        std::shared_ptr img = Disk::Image::Create();
        if(!img->Load(&stream)){
            // show fail dialog
            mStatus->push(std::format("Failed to open image {} ", file->get_path()));
        } else {
            mStatus->push(std::format("Opened image {} ", file->get_path()));

            Gtk::ColumnView* columnView = Gtk::make_managed<Gtk::ColumnView>();
            columnView->set_expand();

            OpenedItem* scroller = Gtk::make_managed<OpenedItem>(columnView, img);
            scroller->set_has_frame(false);
            scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
            scroller->set_expand();                
            scroller->add_controller(mTreeClicked);

            auto tab = Gtk::make_managed<ItemTab>(file->get_basename(), mNotebook, scroller);
            mNotebook->append_page(*scroller, *tab);
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

void MainWindow::OnOpenSettings(){
    mSettingsDialog->set_transient_for(*this);
    set_modal(mSettingsDialog);
    mSettingsDialog->set_visible(true);
}

void MainWindow::OnQuit(){
    set_visible(false);
}

void MainWindow::OnImport(){

}

void MainWindow::OnDelete(){

}

void MainWindow::OnExtract(){

}

void MainWindow::OnNewFolder(){

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
    
    contextMenuAction->add_action("import", sigc::mem_fun(*this, &MainWindow::OnImport));
    contextMenuAction->add_action("extract", sigc::mem_fun(*this, &MainWindow::OnExtract));
    contextMenuAction->add_action("delete", sigc::mem_fun(*this, &MainWindow::OnDelete));
    contextMenuAction->add_action("newfolder", sigc::mem_fun(*this, &MainWindow::OnNewFolder));
    contextMenuAction->add_action("close", sigc::mem_fun(*this, &MainWindow::OnCloseItem));

    insert_action_group("fs", contextMenuAction);

    mStatus = builder->get_widget<Gtk::Statusbar>("gctoolsStatusBar");
    mNotebook = builder->get_widget<Gtk::Notebook>("openedPages");

    mTreeContextMenuModel = builder->get_object<Gio::Menu>("ctxMenuModel");
    mNotebookContextMenuModel = builder->get_object<Gio::Menu>("ctxNotebookMenuModel");


    mContextMenu.set_flags(Gtk::PopoverMenu::Flags::NESTED);
    mContextMenu.set_has_arrow(false);

    mTreeClicked = Gtk::GestureClick::create();
    mTreeClicked->set_button(GDK_BUTTON_SECONDARY);
    mTreeClicked->signal_pressed().connect(sigc::mem_fun(*this, &MainWindow::TreeClicked));

    mNotebookClicked = Gtk::GestureClick::create();
    mNotebookClicked->set_button(GDK_BUTTON_SECONDARY);
    mNotebookClicked->signal_pressed().connect(sigc::mem_fun(*this, &MainWindow::NotebookRightClicked));
    mNotebook->add_controller(mNotebookClicked);
    
    mSettingsDialog = BuildSettingsDialog();
    
    mSettingsDialog->Builder()->get_widget<Gtk::Button>("applyButton")->signal_clicked().connect([&](){
        mSettingsDialog->set_visible(false);
    });
}

MainWindow::~MainWindow(){
    delete mSettingsDialog;
}

void SettingsDialog::on_message_finish(const Glib::RefPtr<Gio::AsyncResult>& result, const Glib::RefPtr<SettingsDialog>& dialog){

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