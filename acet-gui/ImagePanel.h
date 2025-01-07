#pragma once
#include <wx/wx.h>

class ImagePanel : public wxWindow {

  wxImage image;
  wxBitmap bimage;
  int width, height;

public:
  ImagePanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size);
  void LoadFromFile(wxString filename, wxBitmapType format);
  void SetImage(wxImage image);
  void OnPaint(wxPaintEvent& event);
  void OnSize(wxSizeEvent& event);
  wxImage Image();
};
