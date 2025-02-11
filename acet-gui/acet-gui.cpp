#include "ImagePanel.h"
#include "acet.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <wx/filepicker.h>
#include <wx/image.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/quantize.h>
#include <wx/wx.h>

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

  void BackupSave();
  wxImage ExtractHelper(wxString filename);
  void InjectHelper(wxString savename, wxString imagename);

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

MainFrame::MainFrame() 
  : wxFrame(nullptr, wxID_ANY, "ACET Gui")
{
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
  // auto injectButton = new wxButton(mainPanel, wxID_ANY, "Inject Image");
  // auto extractButton = new wxButton(mainPanel, wxID_ANY, "Extract Image");

  sizer->Add(savePicker, 0, wxEXPAND | wxALL, 10);
  sizer->Add(imagePanel, 1, wxEXPAND);
  // bottomSizer->AddStretchSpacer(1);
  // bottomSizer->Add(extractButton, 0, wxEXPAND | wxRIGHT, 10);
  // bottomSizer->Add(injectButton, 0, wxEXPAND | wxRIGHT, 10);
  // sizer->Add(bottomSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);

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

wxImage MainFrame::ExtractHelper(wxString filename) {
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

  auto imageRGBA = ExtractImage(buffer);

  auto emblem = wxImage(128, 128, false);
  emblem.InitAlpha();
  emblem.SetType(wxBITMAP_TYPE_PNG);

  for (int i = 0; i < kNumPixels * 4; i += 4) {
    int pixelNumber = i / 4;
    int y = pixelNumber / 128;
    int x = pixelNumber % 128;
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

  int colors = 256;

  if (!image.HasMask()) {
    if (!image.HasAlpha()) {
      image.InitAlpha();
      colors = 255;
    }
    image.ConvertAlphaToMask();
  }

  if (image.GetWidth() > 128 || image.GetHeight() > 128)
    image.Rescale(128, 128);

  if (image.CountColours(colors+1) > colors) {
    wxImage quantized;
    wxPalette *pal = NULL;
    if (wxQuantize::Quantize(image, quantized, &pal, colors, 0, wxQUANTIZE_FILL_DESTINATION_IMAGE))
      image = quantized;
  }

  BackupSave();

  std::vector<uint8_t> save = ReadSaveFile((std::string)savename);
  std::vector<uint8_t> inImage;

  for (int y = 0; y < 128; y++)
    for (int x = 0; x < 128; x++) {
      if (image.IsTransparent(x,y)) {
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

  InjectImage(save, inImage);

  std::ofstream outfile(std::string(savename), std::ios::binary);
  outfile.write((char *)save.data(), save.size());
  outfile.close();
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
  auto emblem = ExtractHelper(savefile);

  savePicker->SetPath(savefile);
  imagePanel->SetImage(emblem);
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
  auto emblem = ExtractHelper(savefile);
  imagePanel->SetImage(emblem);
}

void MainFrame::OnFilesDropped(wxDropFilesEvent &event) {
  wxString imagename, savename;
  auto filenames = event.GetFiles();

  if (!savePicker->GetPath().IsEmpty())
    savename = savePicker->GetPath();

  for (int i = 0; i < event.GetNumberOfFiles(); i++) {
    if (wxImage::CanRead(filenames[i])) {
      imagename = filenames[i];
    } else if (FindPaletteOffset((std::string)filenames[i])) {
      savename = filenames[i];
    }
  }

  if (savename.IsEmpty()) {
    wxMessageBox("Couldn't find a possible emblem save in dropped files");
    return;
  }

  savePicker->SetPath(savename);
  auto emblem = ExtractHelper(savename);
  imagePanel->SetImage(emblem);

  if (!imagename.IsEmpty()) {
    InjectHelper(savename, imagename);
  }

  emblem = ExtractHelper(savename);
  imagePanel->SetImage(emblem);
}
