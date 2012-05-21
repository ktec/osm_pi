///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __OSMGUI_H__
#define __OSMGUI_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/slider.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/clrpicker.h>
#include <wx/fontpicker.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/tglbtn.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/notebook.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class OsmCfgDlgDef
///////////////////////////////////////////////////////////////////////////////
class OsmCfgDlgDef : public wxDialog 
{
	private:
	
	protected:
	
	public:
		
		OsmCfgDlgDef( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Osm preferences"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 496,587 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~OsmCfgDlgDef();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class OsmDlgDef
///////////////////////////////////////////////////////////////////////////////
class OsmDlgDef : public wxDialog 
{
	private:
	
	protected:
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOsmProperties( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOsmCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOsmOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxChoice* m_chSurvey;
		
		OsmDlgDef( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("OpenSeaMap"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 700,550 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~OsmDlgDef();
	
};


#endif //__OSMGUI_H__
