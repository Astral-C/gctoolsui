#include "MainWindow.hpp"
#include <iostream>
#include "bytesize.hpp"

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

void OpenedItem::OnCreateIconItem(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto expander = Gtk::make_managed<Gtk::TreeExpander>();
    auto label = Gtk::make_managed<Gtk::Label>();
    label->set_halign(Gtk::Align::START);
    expander->set_child(*label);
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
    
    auto label = dynamic_cast<Gtk::Label*>(expander->get_child());
    if (!label) return;
    label->set_text(col->mEntryName);
}

void OpenedItem::OnBindSize(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row) return;

    auto col = std::dynamic_pointer_cast<ModelColumns>(row->get_item());
    if (!col) return;
    
    auto expander = dynamic_cast<Gtk::TreeExpander*>(list_item->get_child());
    if (!expander) return;
    
    expander->set_list_row(row);
    
    auto label = dynamic_cast<Gtk::Label*>(expander->get_child());
    if (!label) return;
    label->set_text(col->mEntrySize);
}


OpenedItem::OpenedItem(Gtk::ColumnView* view, std::shared_ptr<Archive::Rarc> arc){
    mView = view;
    mArchive = arc;
    
    AddDirectoryNodeArchive(mArchiveItems, mArchive->GetRoot(), "");

    auto root = CreateArchiveTreeModel();
    mTreeListModel = Gtk::TreeListModel::create(root, sigc::mem_fun(*this, &OpenedItem::CreateArchiveTreeModel), false, true);
    mSelection = Gtk::SingleSelection::create(mTreeListModel);

    auto nameItemFactory = Gtk::SignalListItemFactory::create();
    nameItemFactory->signal_setup().connect(sigc::mem_fun(*this, &OpenedItem::OnCreateItem));
    nameItemFactory->signal_bind().connect(sigc::mem_fun(*this, &OpenedItem::OnBindName));

    auto column = Gtk::ColumnViewColumn::create("Name", nameItemFactory);
    column->set_expand(true);
    mView->append_column(column);

    auto sizeItemFactory = Gtk::SignalListItemFactory::create();
    sizeItemFactory->signal_setup().connect(sigc::mem_fun(*this, &OpenedItem::OnCreateItem));
    sizeItemFactory->signal_bind().connect(sigc::mem_fun(*this, &OpenedItem::OnBindSize));

    column = Gtk::ColumnViewColumn::create("Size", sizeItemFactory);
    column->set_fixed_width(128);
    mView->append_column(column);

    mView->set_show_column_separators(true);

    mView->set_model(mSelection);
}

OpenedItem::OpenedItem(Gtk::ColumnView* view, std::shared_ptr<Disk::Image> img){
    mView = view;
    mDisk = img;
    
    AddDirectoryNodeDisk(mDiskItems, mDisk->GetRoot(), "");

    auto root = CreateArchiveTreeModel();
    mTreeListModel = Gtk::TreeListModel::create(root, sigc::mem_fun(*this, &OpenedItem::CreateDiskTreeModel), false, true);
    mSelection = Gtk::SingleSelection::create(mTreeListModel);

    auto nameItemFactory = Gtk::SignalListItemFactory::create();
    nameItemFactory->signal_setup().connect(sigc::mem_fun(*this, &OpenedItem::OnCreateItem));
    nameItemFactory->signal_bind().connect(sigc::mem_fun(*this, &OpenedItem::OnBindName));

    auto column = Gtk::ColumnViewColumn::create("Name", nameItemFactory);
    column->set_expand(true);
    mView->append_column(column);

    auto sizeItemFactory = Gtk::SignalListItemFactory::create();
    sizeItemFactory->signal_setup().connect(sigc::mem_fun(*this, &OpenedItem::OnCreateItem));
    sizeItemFactory->signal_bind().connect(sigc::mem_fun(*this, &OpenedItem::OnBindSize));

    column = Gtk::ColumnViewColumn::create("Size", sizeItemFactory);
    column->set_fixed_width(128);
    mView->append_column(column);

    mView->set_show_column_separators(true);

    mView->set_model(mSelection);
}


void MainWindow::OnNew(){
    std::shared_ptr<Archive::Rarc> arc = Archive::Rarc::Create();
    auto root = Archive::Folder::Create(arc);
    root->SetName("archive");
    arc->SetRoot(root);

    Gtk::ScrolledWindow* scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroller->set_has_frame(false);
    scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    scroller->set_expand();    
    
    Gtk::ColumnView* columnView = Gtk::make_managed<Gtk::ColumnView>();
    columnView->set_expand();
    
    mOpenedItems.push_back(OpenedItem(columnView, arc));
    scroller->set_child(*columnView);

    mNotebook->append_page(*scroller, "archive");
}

void MainWindow::OpenArchive(Glib::RefPtr<Gio::AsyncResult>& result){
    Glib::RefPtr<Gio::File> file;

    try {
        file = mFileDialog->open_finish(result);
    } catch(const Gtk::DialogError& err) {
        return;
    }

    //check extension to see if we should be opening an iso?

    bStream::CFileStream stream(file->get_path(), bStream::Endianess::Big, bStream::OpenMode::In);
    std::shared_ptr arc = Archive::Rarc::Create();
    if(!arc->Load(&stream)){
        // show fail dialog
        mStatus->push(std::format("Failed to open archive {} ", file->get_path()));
    } else {
        mStatus->push(std::format("Opened archive {} ", file->get_path()));

        Gtk::ScrolledWindow* scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
        scroller->set_has_frame(false);
        scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
        scroller->set_expand();    
        
        Gtk::ColumnView* columnView = Gtk::make_managed<Gtk::ColumnView>();
        columnView->set_expand();
        
        mOpenedItems.push_back(OpenedItem(columnView, arc));
        scroller->set_child(*columnView);

        mNotebook->append_page(*scroller, file->get_basename());
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
    //mArchive->S
}

void MainWindow::OnSaveAs(){

}

void MainWindow::OnQuit(){
    set_visible(false);
}

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::ApplicationWindow(cobject), mBuilder(builder){
    auto menuActions = Gio::SimpleActionGroup::create();
    
    menuActions->add_action("new", sigc::mem_fun(*this, &MainWindow::OnNew));
    menuActions->add_action("open", sigc::mem_fun(*this, &MainWindow::OnOpen));
    menuActions->add_action("save", sigc::mem_fun(*this, &MainWindow::OnSave));
    menuActions->add_action("saveas", sigc::mem_fun(*this, &MainWindow::OnSaveAs));
    menuActions->add_action("quit", sigc::mem_fun(*this, &MainWindow::OnQuit));

    insert_action_group("menuactions", menuActions);

    mStatus = builder->get_widget<Gtk::Statusbar>("gctoolsStatusBar");
    mNotebook = builder->get_widget<Gtk::Notebook>("openedPages");
    mNotebook->signal_page_removed().connect(sigc::mem_fun(*this, &MainWindow::PageRemoved));
}

MainWindow::~MainWindow(){

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