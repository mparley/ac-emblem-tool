#include "ImagePanel.h"

#include <wx/graphics.h>
#include <wx/dcbuffer.h>

ImagePanel::ImagePanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
  : wxWindow(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE)
{
  this->SetBackgroundStyle(wxBG_STYLE_PAINT);

  this->Bind(wxEVT_PAINT, &ImagePanel::OnPaint, this);
  // this->Bind(wxEVT_SIZE, &ImagePanel::OnSize, this);
}

void ImagePanel::LoadFromFile(wxString filename, wxBitmapType format) {
  image.LoadFile(filename, format);
  width = image.GetSize().GetWidth();
  height = image.GetSize().GetHeight();
  bimage = wxBitmap(image.Scale(width*3, height*3, wxIMAGE_QUALITY_NEAREST));
}

void ImagePanel::SetImage(wxImage emblem) {
  image = emblem;
  width = image.GetSize().GetWidth();
  height = image.GetSize().GetHeight();
  bimage = wxBitmap(image.Scale(width*3, height*3, wxIMAGE_QUALITY_NEAREST));
  Refresh();
}

void ImagePanel::OnPaint(wxPaintEvent& event) {
  wxClientDC dc(this);
  dc.Clear();

  if (image.IsOk()) {
    int fw, fh;
    dc.GetSize(&fw, &fh);

    int smaller = fw < fh ? fw : fh;
    bimage = wxBitmap(image.Scale(smaller, smaller, wxIMAGE_QUALITY_NEAREST));

    int x = fw / 2 - (bimage.GetSize().GetWidth() / 2);
    int y = fh / 2 - (bimage.GetSize().GetHeight() / 2);
    dc.DrawBitmap(bimage, x, y, false);
  }
}

void ImagePanel::OnSize(wxSizeEvent& event) {
  Refresh();
  event.Skip();
}

wxImage ImagePanel::Image() {
  return image;
}