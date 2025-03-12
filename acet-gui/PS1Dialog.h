#pragma once
#include "acet.h"
#include <cctype>
#include <string>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

class PS1Dialog : public wxDialog {
private:
  wxChoice *blockChooser;
  wxSpinCtrl *slotChooser;

public:
  PS1Dialog(const wxString& savename)
    : wxDialog(NULL, wxID_ANY, _("PS1 MEMORY CARD"))
  {
    blockChooser = new wxChoice(this, wxID_ANY);
    slotChooser = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition,
      wxDefaultSize, wxSP_ARROW_KEYS, 0, 6);

    auto choices = ListPS1MCBlocks((std::string)savename);
    for (int i = 1; i < choices.size(); i++) {
      std::string choice = std::to_string(i) + " - ";
      choice += (!choices[i].empty()) ? choices[i] : "Empty Block";
      blockChooser->Append(choice);
    }
    blockChooser->SetSelection(0);

    auto choiceSizer = new wxBoxSizer(wxHORIZONTAL);
    choiceSizer->Add(blockChooser, 0, wxEXPAND | wxRIGHT | wxLEFT, 10);
    choiceSizer->Add(slotChooser, 0, wxEXPAND | wxRIGHT, 10);

    auto mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(choiceSizer, 0, wxALL, 10);
    mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL, 10);

    SetSizerAndFit(mainSizer);
    Center();
  }

  int Slot() const { return slotChooser->GetValue(); }

  int Block() const { return blockChooser->GetSelection() + 1; }

  bool IsMOA() const { return blockChooser->GetStringSelection().Contains("BASLUS-01030Z"); }
};
