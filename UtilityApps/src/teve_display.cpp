//==========================================================================
//  AIDA Detector description implementation 
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : M.Frank
//
//==========================================================================

// Framework include files
#include "DD4hep/Factories.h"
#include "DD4hep/Detector.h"
#include "DDRec/SurfaceHelper.h"

#include "EvNavHandler.h"
#include "MultiView.h"

#include "run_plugin.h"
#include "TRint.h"

#include "TROOT.h"
#include "TEveGeoNode.h"
#include "TEveBrowser.h"
#include "TGNumberEntry.h"
#include "TGButton.h"
#include "TGLabel.h"
#include "TStyle.h"
#include "TGComboBox.h"
#include "TEveManager.h"
#include "TSystem.h"
#include "TGLViewer.h"
#include "TEveViewer.h"
#include "TGLPerspectiveCamera.h"
#include "TGLCamera.h"
#include "TEveStraightLineSet.h"
#include "TSysEvtHandler.h"
#include <TEveScene.h>
#include <TEveProjectionManager.h>
#include <TEveProjectionAxes.h>
#include <TEveWindow.h>

#include "TGeoManager.h"
#include "TGLClip.h"
#include "TMap.h"
#include "TObjString.h"
#include "TGeoShape.h"
#include "TGLScenePad.h"


using namespace dd4hep ;
using namespace rec ;
using namespace detail ;


//=====================================================================================
// function declarations: 
void next_event();
void make_gui();

TEveStraightLineSet* getSurfaces(int col=kRed, const SurfaceType& type=SurfaceType(), TString name="Surfaces" ) ;
TEveStraightLineSet* getSurfaceVectors(bool addO=true, bool addU= true, bool addV=true, bool addN=true,TString name="SurfaceVectors",int color=kGreen) ;

//=====================================================================================

static long teve_display(Detector& description, int argc, char** argv)  {
  int level = 4, visopt = 0, help = 0;

  for( int i=0; i<argc; ++i )  {
    if ( strncmp(argv[i],"-visopt",4)    == 0 ) visopt = ::atol(argv[++i]);
    else if ( strncmp(argv[i],"-level", 4)    == 0 ) level  = ::atol(argv[++i]);
    else help = 1;
  }
  if ( help )   {
    std::cout <<
      "Usage: teveDisplay -arg [-arg]                                                    \n\n"
      "     Invoke TEve display using the factory mechanism.                             \n\n"
      "     -level    <number> Visualization level  [TGeoManager::SetVisLevel]  Default: 4 \n"
      "     -visopt   <number> Visualization option [TGeoManager::SetVisOption] Default: 0 \n"
      "     -help              Print this help output" << std::endl << std::flush;
    ::exit(EINVAL);
  }
  
  TGeoManager* mgr = &description.manager();
  mgr->SetNsegments(100); // Increase the visualization resolution.
  TEveManager::Create();

  TEveGeoTopNode* tn = new TEveGeoTopNode(mgr, mgr->GetTopNode());
  // option 0 in TEve seems to correspond to option 1 in TGeo ( used in geoDisplay ...)
  tn->SetVisOption(visopt) ;
  tn->SetVisLevel(level);

  // // ---- try to set transparency - does not seem to work ...
  // TGeoNode* node1 = gGeoManager->GetTopNode();
  // int k_nodes = node1->GetNdaughters();
  // for(int i=0; i<k_nodes; i++) {
  //   TGeoNode * CurrentNode = node1->GetDaughter(i);
  //   CurrentNode->GetMedium()->GetMaterial()->SetTransparency(80);
  //   // TEveGeoNode CurrentNode( node1->GetDaughter(i) ) ;
  //   // CurrentNode.SetMainTransparency( 80 ) ;
  // }
  // this code seems to have no effect either ...
  // tn->CSCApplyMainTransparencyToAllChildren() ;
  // tn->SetMainTransparency( 80 ) ;


  gEve->AddGlobalElement( tn );

  TEveElement* surfaces = getSurfaces(  kRed, SurfaceType( SurfaceType::Sensitive ), "SensitiveSurfaces" ) ;
  TEveElement* helperSurfaces = getSurfaces(  kGray, SurfaceType( SurfaceType::Helper ),"HelperSurfaces" ) ;
  TEveElement* surfaceVectors = getSurfaceVectors(1,0,0,1,"SurfaceVectorsN",kGreen) ;

  gEve->AddGlobalElement( surfaces ) ;
  gEve->AddGlobalElement( helperSurfaces ) ;
  gEve->AddGlobalElement( surfaceVectors ) ;
  
  
  TEveElement* surfaceVectors_u = getSurfaceVectors(0,1,0,0,"SurfaceVectorsU",kMagenta) ;
  
  gEve->AddGlobalElement( surfaceVectors_u ) ;

  TEveElement* surfaceVectors_v = getSurfaceVectors(0,0,1,0,"SurfaceVectorsV",kBlack) ;
  
  gEve->AddGlobalElement( surfaceVectors_v ) ;

  TGLViewer *v = gEve->GetDefaultGLViewer();
  // v->GetClipSet()->SetClipType(TGLClip::kClipPlane);
  //  v->ColorSet().Background().SetColor(kMagenta+4);
  v->ColorSet().Background().SetColor(kWhite);
  v->SetGuideState(TGLUtil::kAxesEdge, kTRUE, kFALSE, 0);
  v->RefreshPadEditor(v);
  //  v->CurrentCamera().RotateRad(-1.2, 0.5);

  gEve->GetGlobalScene()->GetGLScene()->SetSelectable(kFALSE) ; //change for debugging (kTRUE);


  MultiView::instance()->ImportGeomRPhi( surfaces );
  MultiView::instance()->ImportGeomRhoZ( surfaces ) ;
  MultiView::instance()->ImportGeomRPhi( helperSurfaces );
  MultiView::instance()->ImportGeomRhoZ( helperSurfaces ) ;

  make_gui();
  next_event();
  gEve->FullRedraw3D(kTRUE);
  return 1;
}
DECLARE_APPLY(DD4hepTEveDisplay,teve_display)


//=====================================================================================================================

int main(int argc,char** argv)  {
  std::vector<const char*> av;
  std::string level, visopt, opt;
  bool help = false;
  for( int i=0; i<argc; ++i )   {
    if ( i==1 && argv[i][0] != '-' ) av.emplace_back("-input");
    else if ( strncmp(argv[i],"-help",4)      == 0 ) help = true, av.emplace_back(argv[i]);
    else if ( strncmp(argv[i],"-visopt",4)    == 0 ) visopt   = argv[++i];
    else if ( strncmp(argv[i],"-level", 4)    == 0 ) level    = argv[++i];
    else if ( strncmp(argv[i],"-option",4)    == 0 ) opt      = argv[++i];
    else av.emplace_back(argv[i]);
  }
  av.emplace_back("-interactive");
  av.emplace_back("-plugin");
  av.emplace_back("DD4hepTEveDisplay");
  if ( help              ) av.emplace_back("-help");
  if ( !opt.empty()      ) av.emplace_back("-opt"),      av.emplace_back(opt.c_str());
  if ( !level.empty()    ) av.emplace_back("-level"),    av.emplace_back(level.c_str());
  if ( !visopt.empty()   ) av.emplace_back("-visopt"),   av.emplace_back(visopt.c_str());
  return dd4hep::execute::main_plugins("DD4hepTEveDisplay", av.size(), (char**)&av[0]);
}

//=====================================================================================================================
TEveStraightLineSet* getSurfaceVectors(bool addO, bool addU, bool addV, bool addN, TString name,int color) {
  TEveStraightLineSet* ls = new TEveStraightLineSet(name);
  Detector& description = Detector::getInstance();
  DetElement world = description.world() ;

  // create a list of all surfaces in the detector:
  SurfaceHelper surfMan(  world ) ;

  const SurfaceList& sL = surfMan.surfaceList() ;

  for( SurfaceList::const_iterator it = sL.begin() ; it != sL.end() ; ++it ){

    ISurface* surf = *it ;
    const Vector3D& u = surf->u() ;
    const Vector3D& v = surf->v() ;
    const Vector3D& n = surf->normal() ;
    const Vector3D& o = surf->origin() ;

    Vector3D ou = o + u ;
    Vector3D ov = o + v ;
    Vector3D on = o + n ;
 
    if (addU) ls->AddLine( o.x(), o.y(), o.z(), ou.x() , ou.y() , ou.z()  );
    
//     TEveStraightLineSet::Marker_t *m = ls->AddMarker(id,1.);
    
    if (addV) ls->AddLine( o.x(), o.y(), o.z(), ov.x() , ov.y() , ov.z()  );
    if (addN) ls->AddLine( o.x(), o.y(), o.z(), on.x() , on.y() , on.z()  );
    if (addO) ls->AddMarker(  o.x(), o.y(), o.z() );
  }
  ls->SetLineColor( color ) ;
  ls->SetMarkerColor( kBlue ) ;
  ls->SetMarkerSize(1);
  ls->SetMarkerStyle(4);
  
  //  gEve->AddElement(ls);

  return ls;
}
//=====================================================================================
TEveStraightLineSet* getSurfaces(int col, const SurfaceType& type, TString name) {

  TEveStraightLineSet* ls = new TEveStraightLineSet(name);

  Detector& description = Detector::getInstance();

  DetElement world = description.world() ;

  // create a list of all surfaces in the detector:
  SurfaceHelper surfMan(  world ) ;

  const SurfaceList& sL = surfMan.surfaceList() ;

//    std::cout << " getSurfaces() for "<<name<<" - #surfaces : " << sL.size() << std::endl ;

  for( SurfaceList::const_iterator it = sL.begin() ; it != sL.end() ; ++it ){

    Surface* surf = dynamic_cast< Surface*> ( *it ) ;

    if( ! surf ) 
      continue ;
    
    if( ! ( surf->type().isVisible() && ( surf->type().isSimilar( type ) ) ) ) 
      continue ;

//     std::cout << " **** drawSurfaces() : empty lines vector for surface " << *surf << std::endl ;

    const std::vector< std::pair<Vector3D,Vector3D> > lines = surf->getLines() ;

    if( lines.empty() )
      std::cout << " **** drawSurfaces() : empty lines vector for surface " << *surf << std::endl ;

    unsigned nL = lines.size() ;

    for( unsigned i=0 ; i<nL ; ++i){

      //      std::cout << " **** drawSurfaces() : draw line for surface " <<   lines[i].first << " - " <<  lines[i].second  << std::endl ;

      ls->AddLine( lines[i].first.x(),  lines[i].first.y(),  lines[i].first.z(), 
                   lines[i].second.x(), lines[i].second.y(), lines[i].second.z() ) ;
    }
    
    ls->SetLineColor( col ) ;
    ls->SetMarkerColor( col ) ;
    ls->SetMarkerSize(.1);
    ls->SetMarkerStyle(4);

  }

  return ls;
}

//=====================================================================================

void make_gui() {

  // Create minimal GUI for event navigation.

  TEveBrowser* browser = gEve->GetBrowser();
  browser->StartEmbedding(TRootBrowser::kLeft);

  TGMainFrame* frmMain = new TGMainFrame(gClient->GetRoot(), 1000, 600);
  frmMain->SetWindowName("dd4hep GUI");
  frmMain->SetCleanup(kDeepCleanup);

  TGHorizontalFrame* hf = new TGHorizontalFrame(frmMain);
  {
      
#if ROOT_VERSION_CODE >= ROOT_VERSION(6,9,2)
    TString icondir( Form("%s/", TROOT::GetIconPath().Data()) );
#else
    TString icondir( Form("%s/icons/", gSystem->Getenv("ROOTSYS")) );
#endif
    TGPictureButton* b = 0;
    EvNavHandler    *fh = new EvNavHandler;

    b = new TGPictureButton(hf, gClient->GetPicture(icondir+"GoBack.gif"));
    b->SetEnabled(kFALSE);
    b->SetToolTipText("Go to previous event - not supported.");
    hf->AddFrame(b);
    b->Connect("Clicked()", "EvNavHandler", fh, "Bck()");

    b = new TGPictureButton(hf, gClient->GetPicture(icondir+"GoForward.gif"));
    b->SetToolTipText("Generate new event.");
    hf->AddFrame(b);
    b->Connect("Clicked()", "EvNavHandler", fh, "Fwd()");
  }
  frmMain->AddFrame(hf);

  frmMain->MapSubwindows();
  frmMain->Resize();
  frmMain->MapWindow();

  browser->StopEmbedding();
  browser->SetTabTitle("Event Control", 0);
}
//=====================================================================================

