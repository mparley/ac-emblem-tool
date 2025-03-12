#include "ImagePanel.h"
#include "PS1Dialog.h"
#include "acet.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <wx/filepicker.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/layout.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/quantize.h>
#include <wx/wx.h>
#include <wx/spinctrl.h>

class ACETGui : public wxApp {
public:
  bool OnInit() override;
  bool OnExceptionInMainLoop() override;
};

wxIMPLEMENT_APP(ACETGui);

class MainFrame : public wxFrame {
public:
  MainFrame();

private:
  ImagePanel *imagePanel;
  wxFilePickerCtrl *savePicker;
  uint8_t ps1MemBlock;
  uint8_t ps1EmbSlot;
  bool ps1IsMOA;

  void BackupSave();
  wxImage ExtractHelper(wxString filename);
  void InjectHelper(wxString savename, wxString imagename);
  void OpenSaveHelper(wxString savename);

  void OnMenuOpenSave(wxCommandEvent &event);
  void OnMenuExit(wxCommandEvent &event);
  void OnMenuAbout(wxCommandEvent &event);
  void OnMenuExtract(wxCommandEvent &event);
  void OnMenuInject(wxCommandEvent &event);
  void OnSavePicked(wxFileDirPickerEvent &event);
  void OnFilesDropped(wxDropFilesEvent &event);
};

bool ACETGui::OnInit() {
  wxInitAllImageHandlers();
  wxLog::SetVerbose(true);
  MainFrame *frame = new MainFrame();
  frame->Show(true);
  return true;
}

bool ACETGui::OnExceptionInMainLoop() {
  wxString error;
  try {
    throw; // Rethrow the current exception.
  } catch (const std::exception &e) {
    error = e.what();
  } catch (...) {
    error = "Unknown error.";
  }

  wxLogError(
      "Unexpected exception has occurred!\n %s\n The program will terminate.",
      error);

  // Exit the main loop and thus terminate the program.
  return false;
}

MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "ACET Gui")
{
  ps1MemBlock = 1;
  ps1EmbSlot = 0;
  ps1IsMOA = false;

  wxMenu *menuFile = new wxMenu;
  DragAcceptFiles(true);

  menuFile->Append(wxID_OPEN, "&Open Save...\tCtrl-O", "Open AC emblem save file to edit");
  auto menuExtractID = menuFile->Append(wxID_ANY, "&Extract Image As...\tCtrl-E",
    "Save current image")->GetId();
  auto menuInjectID = menuFile->Append(wxID_ANY, "&Inject Image...\tCtrl-I",
    "Inject an image into the current save file")->GetId();
  
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu *menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  Bind(wxEVT_MENU, &MainFrame::OnMenuOpenSave, this, wxID_OPEN);
  Bind(wxEVT_MENU, &MainFrame::OnMenuExtract, this, menuExtractID);
  Bind(wxEVT_MENU, &MainFrame::OnMenuInject, this, menuInjectID);
  Bind(wxEVT_MENU, &MainFrame::OnMenuAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, &MainFrame::OnMenuExit, this, wxID_EXIT);

  CreateStatusBar();

  auto mainPanel = new wxPanel(this);
  auto sizer = new wxBoxSizer(wxVERTICAL);
  auto bottomSizer = new wxBoxSizer(wxHORIZONTAL);

  savePicker = new wxFilePickerCtrl(mainPanel, wxID_ANY);
  imagePanel = new ImagePanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(128, 128));

  sizer->Add(savePicker, 0, wxEXPAND | wxALL, 10);
  sizer->Add(imagePanel, 1, wxEXPAND);

  Bind(wxEVT_FILEPICKER_CHANGED, &MainFrame::OnSavePicked, this, savePicker->GetId());
  Bind(wxEVT_DROP_FILES, &MainFrame::OnFilesDropped, this, wxID_ANY);

  mainPanel->SetSizer(sizer);
  imagePanel->SetMinSize(wxSize(384, 384));
  sizer->SetSizeHints(this);
  // SetClientSize(384, 440);
}

void MainFrame::BackupSave() {
  if (savePicker->GetPath().IsEmpty()) {
    wxLogError("BackupSave: No savefile to backup");
    return;
  }

  auto backup = wxGetCwd() + "/acet-backup";

  if (!wxDirExists(backup) && !wxMkdir(backup)) {
    wxLogError("BackupSave: Couldn't make acet-backup");
    return;
  }

  backup += wxString::Format(wxT("/%ld"), wxGetLocalTime());

  if (!wxDirExists(backup) && !wxMkdir(backup)) {
    wxLogError("BackupSave: Couldn't make " + backup);
    return;
  }

  if (!wxCopyFile(savePicker->GetPath(),
    backup + "/" + savePicker->GetFileName().GetName()))
  {
    wxLogError("BackupSave: Couldn't copy save file");
    return;
  }
}

void MainFrame::OpenSaveHelper(wxString savename) {
  if (isPS1Save((std::string)savename)) {
    PS1Dialog ps1Dlg(savename);
    if (ps1Dlg.ShowModal() == wxID_CANCEL)
      return;

    ps1EmbSlot = ps1Dlg.Slot();
    ps1MemBlock = ps1Dlg.Block();
    ps1IsMOA = ps1Dlg.IsMOA();
  }

  savePicker->SetPath(savename);
  auto emblem = ExtractHelper(savename);
  imagePanel->SetImage(emblem);
}

wxImage MainFrame::ExtractHelper(wxString filename) {
  std::vector<uint8_t> imageRGBA;
  int width = 128;
  int height = 128;

  if (isPS1Save((std::string)filename)) {
    width = 64;
    height = 64;
    imageRGBA = ExtractImageAC1((std::string)filename, ps1IsMOA, ps1MemBlock, ps1EmbSlot);
  } else {
    std::ifstream infile((std::string)filename, std::ios::binary | std::ios::ate);
    if (!infile) {
      wxLogError("ExtractHelper: Failed to load save file");
      return wxNullImage;
    }

    size_t save_size = infile.tellg();
    std::vector<uint8_t> buffer(save_size, 0);

    infile.seekg(0, infile.beg);
    infile.read((char *)buffer.data(), save_size);
    infile.close();

    imageRGBA = ExtractImage(buffer);
  }

  auto emblem = wxImage(width, height, false);
  emblem.InitAlpha();
  emblem.SetType(wxBITMAP_TYPE_PNG);

  for (int i = 0; i < width * height * 4; i += 4) {
    int pixelNumber = i / 4;
    int y = pixelNumber / width;
    int x = pixelNumber % width;
    emblem.SetRGB(x, y, imageRGBA[i], imageRGBA[i + 1], imageRGBA[i + 2]);
    emblem.SetAlpha(x, y, imageRGBA[i + 3]);
  }

  if (!emblem.IsOk()) {
    wxLogError("ExtactionHelper: Failed to load emblem");
    return emblem;
  }

  return emblem;
}

void MainFrame::InjectHelper(wxString savename, wxString imagename) {
  auto image = wxImage(imagename);
  if (!image.IsOk()) {
    wxLogError("OnMenuInject: Image not ok");
    return;
  }

  BackupSave();

  int colors = 256;
  int width = 128;
  int height = 128;
  bool ps1 = false;

  if (isPS1Save((std::string)savename)) {
    colors = 16;
    width = 64;
    height = 64;
    ps1 = true;
  }

  if (!image.HasMask()) {
    if (!image.HasAlpha()) {
      image.InitAlpha();
      colors--;
    }
    image.ConvertAlphaToMask();
  }

  if (image.GetWidth() != width || image.GetHeight() != height)
    image.Rescale(width, height);

  /*image.SaveFile(imagename + ".intermediate.png");*/

  wxImage masked = image;
  if (image.CountColours(colors+1) > colors) {
    wxPalette *pal = NULL;
    wxImage quantized;
    if (wxQuantize::Quantize(image, quantized, &pal, colors, 0, wxQUANTIZE_FILL_DESTINATION_IMAGE))
      image = quantized;
  }

  if (image.HasMask()) 
    masked = image;

  /*wxString message = "HasAlpha:";*/
  /*message += (image.HasAlpha()) ? "True" : "False";*/
  /*message += " HasMask:";*/
  /*message += (image.HasMask()) ? "True" : "False";*/
  /*message += " Colors:" + std::to_string(image.CountColours());*/
  /*GetStatusBar()->PushStatusText(message);*/

  std::vector<uint8_t> inImage;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (masked.IsTransparent(x,y)) {
        inImage.push_back(0x00);
        inImage.push_back(0x00);
        inImage.push_back(0x00);
        inImage.push_back(0x00);
      } else {
        inImage.push_back(image.GetRed(x, y));
        inImage.push_back(image.GetGreen(x, y));
        inImage.push_back(image.GetBlue(x, y));
        // auto alpha = image.IsTransparent(x,y) ? 0x00 : 0xFF;
        inImage.push_back(0xFF);
      }
    }
  }

  if (ps1) {
    InjectImageAC1((std::string)savename, inImage, ps1IsMOA, ps1MemBlock, ps1EmbSlot);
  } else {
    std::vector<uint8_t> save = ReadSaveFile((std::string)savename);
    InjectImage(save, inImage);

    std::ofstream outfile(std::string(savename), std::ios::binary);
    outfile.write((char *)save.data(), save.size());
    outfile.close();
  }
  /*GetStatusBar()->PushStatusText("Injected [" + imagename + "]!");*/
}

void MainFrame::OnMenuExit(wxCommandEvent &event) {
  Close(true);
}

void MainFrame::OnMenuAbout(wxCommandEvent &event) {
  std::string message = "This a tool for injecting and extracting images from PS2 Armored Core emblem saves.\n";
  message += "\nBASIC USAGE:\n\n";
  message += "1. Save your emblem in-game as an Emblem Save\n";
  message += "2. Get the save on your computer somehow (PCSX2 users can skip this step)\n";
  message += "3. Convert it to a RAW save file using mymc or PCSX2's memcard folder converter.\n";
  message += "\t - It should be called something like BASLUS-20014E00 for example\n";
  message += "\t - BASLUS-20014 is the ps2 game code\n";
  message += "\t - E00 means emblem save 00\n";
  message += "\t - The only exception is last raven saves which are named data0, data1, etc. in an EMB folder\n";
  message += "4. Drag and drop, browse button, or File > Open Save... will load the emblem save and display the image\n";
  message += "5. File > Extract Image As... will extract the emblem to a file\n";
  message += "6. Drag and drop, or File > Inject Image... will format and inject the image into the save\n";
  message += "\t - A backup of the current save will be made in an acet-backup folder where the exe is (so probably don't put the exe in Program Files)\n";
  message += "\t - Basic image formats like PNG, BMP, GIF, JPEG are supported\n";

  wxMessageBox(
    // "This a tool for injecting and extracting images from PS2 Armored Core emblem saves.",
    message,
    "About ACET-GUI", wxOK | wxICON_INFORMATION
  );
}

void MainFrame::OnMenuOpenSave(wxCommandEvent &event) {
  // wxFileDialog dialog(this, "Open AC2 Emblem Save File", "", "", "AC2SAVEFILES (BASLUS-21200E*)|BASLUS-21200E*",
  //     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  wxFileDialog dialog(this);

  if (dialog.ShowModal() == wxID_CANCEL)
    return;

  auto savefile = dialog.GetPath();
  OpenSaveHelper(savefile);
}

void MainFrame::OnMenuExtract(wxCommandEvent &event) {
  wxFileDialog dialog(this, "Save current image", "", "",
    "PNG image (*.png)|*.png|GIF image (*.gif)|*.gif|BMP image (*.bmp)|*.bmp|JPEG image (*.jpg)|*.jpg",
    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (dialog.ShowModal() == wxID_CANCEL)
    return;

  wxImage image = imagePanel->Image();
  image.SaveFile(dialog.GetPath());
}

void MainFrame::OnMenuInject(wxCommandEvent &event) {
  if (savePicker->GetPath().IsEmpty()) {
    wxLogError("OnMenuInect: No savefile selected");
    return;
  }

  wxFileDialog dialog(this);
  if (dialog.ShowModal() == wxID_CANCEL)
    return;

  InjectHelper(savePicker->GetPath(), dialog.GetPath());
  auto emblem = ExtractHelper(savePicker->GetPath());
  imagePanel->SetImage(emblem);
}

void MainFrame::OnSavePicked(wxFileDirPickerEvent &event) {
  auto savefile = event.GetPath();
  OpenSaveHelper(savefile);
}

void MainFrame::OnFilesDropped(wxDropFilesEvent &event) {
  std::string imagename, savename;
  auto filenames = event.GetFiles();
  bool newSave = false;

  if (!savePicker->GetPath().IsEmpty())
    savename = savePicker->GetPath();

  for (int i = 0; i < event.GetNumberOfFiles(); i++) {
    std::string filename = (std::string) filenames[i];
    if (wxImage::CanRead(filename)) {
      imagename = filenames[i];
    } else if (isPS1Save(filename) || FindPaletteOffset(filename)) {
      savename = filenames[i];
      newSave = true;
    }
  }

  if (savename.empty()) {
    wxMessageBox("Couldn't find a possible emblem save in dropped files");
    return;
  }

  if (newSave) {
    OpenSaveHelper(savename);
  }

  if (!imagename.empty()) {
    InjectHelper(savename, imagename);
    auto emblem = ExtractHelper(savename);
    imagePanel->SetImage(emblem);
  }
}
