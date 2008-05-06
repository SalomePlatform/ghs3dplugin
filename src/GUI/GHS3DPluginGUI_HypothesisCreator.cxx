//  GHS3DPlugin GUI: GUI for plugged-in mesher GHS3DPlugin
//
//  Copyright (C) 2003  CEA
// 
//  This library is free software; you can redistribute it and/or 
//  modify it under the terms of the GNU Lesser General Public 
//  License as published by the Free Software Foundation; either 
//  version 2.1 of the License. 
// 
//  This library is distributed in the hope that it will be useful, 
//  but WITHOUT ANY WARRANTY; without even the implied warranty of 
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
//  Lesser General Public License for more details. 
// 
//  You should have received a copy of the GNU Lesser General Public 
//  License along with this library; if not, write to the Free Software 
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
// 
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//
//
//  File   : GHS3DPluginGUI_HypothesisCreator.cxx
//  Author : Michael Zorin
//  Module : GHS3DPlugin
//  $Header: 

#include "GHS3DPluginGUI_HypothesisCreator.h"

#include <SMESHGUI_Utils.h>
#include <SMESHGUI_HypothesesUtils.h>

#include CORBA_SERVER_HEADER(GHS3DPlugin_Algorithm)

#include <QtxComboBox.h>
#include <SUIT_MessageBox.h>
#include <SUIT_Session.h>
#include <SUIT_FileDlg.h>
#include <SalomeApp_Tools.h>

#include <qlabel.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qpixmap.h>
#include <qtabbar.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
//#include <qapplication.h>

enum {
  STD_TAB = 0,
  ADV_TAB
};

namespace {

#ifndef WIN32
#include <sys/sysinfo.h>
#endif

  int maxAvailableMemory()
  {
#ifndef WIN32
    struct sysinfo si;
    int err = sysinfo( &si );
    if ( err == 0 ) {
      int totMB =
        si.totalram * si.mem_unit / 1024 / 1024 +
        si.totalswap * si.mem_unit / 1024 / 1024 ;
      return (int) ( 0.7 * totMB );
    }
#endif
    return 100000;
  }
}

GHS3DPluginGUI_HypothesisCreator::GHS3DPluginGUI_HypothesisCreator( const QString& theHypType )
: SMESHGUI_GenericHypothesisCreator( theHypType )
{
}

GHS3DPluginGUI_HypothesisCreator::~GHS3DPluginGUI_HypothesisCreator()
{
}

QFrame* GHS3DPluginGUI_HypothesisCreator::buildFrame()
{
  QFrame* fr = new QFrame( 0, "myframe" );
  QVBoxLayout* lay = new QVBoxLayout( fr, 5, 0 );

  // tab
  QTabBar* tab = new QTabBar( fr, "QTabBar");
  tab->setShape( QTabBar::RoundedAbove );
  tab->insertTab( new QTab( tr( "SMESH_ARGUMENTS")), STD_TAB);
  tab->insertTab( new QTab( tr( "GHS3D_ADV_ARGS")), ADV_TAB);
  lay->addWidget( tab );

  // basic parameters
  myStdGroup = new QGroupBox( 2, Qt::Horizontal, fr, "myStdGroup" );
  myStdGroup->layout()->setSpacing( 6 );
  myStdGroup->layout()->setMargin( 11 );
  lay->addWidget( myStdGroup );

  myName = 0;
  if( isCreation() )
  {
    new QLabel( tr( "SMESH_NAME" ), myStdGroup );
    myName = new QLineEdit( myStdGroup );
  }

  myToMeshHolesCheck = new QCheckBox( tr( "GHS3D_TO_MESH_HOLES" ), myStdGroup );
  myStdGroup->addSpace(0);

  new QLabel( tr( "GHS3D_OPTIMIZATIOL_LEVEL" ), myStdGroup );
  myOptimizationLevelCombo = new QtxComboBox( myStdGroup );
  QStringList types;
  types.append( QObject::tr( "LEVEL_NONE" ) );
  types.append( QObject::tr( "LEVEL_LIGHT" ) );
  types.append( QObject::tr( "LEVEL_MEDIUM" ) );
  types.append( QObject::tr( "LEVEL_STRONG" ) );
  myOptimizationLevelCombo->insertStringList( types );

  // advanced parameters
  myAdvGroup = new QGroupBox( 3, Qt::Horizontal, fr, "myAdvGroup" );
  lay->addWidget( myAdvGroup );
  QFrame* anAdvFrame = new QFrame(myAdvGroup, "anAdvFrame");
  QGridLayout* anAdvLayout = new QGridLayout(anAdvFrame, 8, 3);
  anAdvLayout->setSpacing(6);
  anAdvLayout->setAutoAdd(false);
  
  myMaximumMemoryCheck = new QCheckBox( tr("MAX_MEMORY_SIZE"), anAdvFrame );

  myMaximumMemorySpin = new QSpinBox( anAdvFrame );
  myMaximumMemorySpin->setMinValue( 1 );
  myMaximumMemorySpin->setMaxValue( maxAvailableMemory() );
  myMaximumMemorySpin->setLineStep( 10 );
  QLabel* aMegabyteLabel = new QLabel( tr("MEGABYTE"), anAdvFrame);

  myInitialMemoryCheck = new QCheckBox( tr("INIT_MEMORY_SIZE"), anAdvFrame );

  myInitialMemorySpin = new QSpinBox( anAdvFrame );
  myInitialMemorySpin->setMinValue( 1 );
  myInitialMemorySpin->setMaxValue( maxAvailableMemory() );
  myInitialMemorySpin->setLineStep( 10 );
  QLabel* aMegabyteLabel2 = new QLabel( tr("MEGABYTE"), anAdvFrame);

  QLabel* aWorkinDirLabel = new QLabel( tr( "WORKING_DIR" ), anAdvFrame );
  QPushButton* dirBtn = new QPushButton( tr( "SELECT_DIR"), anAdvFrame, "dirBtn");
  myWorkingDir = new QLineEdit( anAdvFrame, "myWorkingDir");
  myWorkingDir->setReadOnly( true );
  
  myKeepFiles = new QCheckBox( tr( "KEEP_WORKING_FILES" ), anAdvFrame );

  QLabel* aVerboseLevelLabel = new QLabel( tr("VERBOSE_LEVEL"), anAdvFrame);
  myVerboseLevelSpin = new QSpinBox( anAdvFrame );
  myVerboseLevelSpin->setMinValue( 0 );
  myVerboseLevelSpin->setMaxValue( 10 );
  myVerboseLevelSpin->setLineStep( 1 );

  myToCreateNewNodesCheck = new QCheckBox( tr( "TO_ADD_NODES" ), anAdvFrame );
  
  myBoundaryRecoveryCheck = new QCheckBox( tr( "RECOVERY_VERSION" ), anAdvFrame );

  QLabel* aTextOptionLabel = new QLabel( tr( "TEXT_OPTION" ), anAdvFrame );
  myTextOption = new QLineEdit( anAdvFrame, "myTextOption");


  anAdvLayout->addWidget(myMaximumMemoryCheck, 0, 0);
  anAdvLayout->addWidget(myMaximumMemorySpin,  0, 1);
  anAdvLayout->addWidget(aMegabyteLabel,       0, 2);

  anAdvLayout->addWidget(myInitialMemoryCheck, 1, 0);
  anAdvLayout->addWidget(myInitialMemorySpin,  1, 1);
  anAdvLayout->addWidget(aMegabyteLabel2,       1, 2);

  anAdvLayout->addWidget(aWorkinDirLabel,      2, 0);
  anAdvLayout->addWidget(dirBtn,               2, 1);
  anAdvLayout->addWidget(myWorkingDir,         2, 2);

  anAdvLayout->addWidget(myKeepFiles,          3, 0);
  
  anAdvLayout->addWidget(aVerboseLevelLabel,   4, 0);
  anAdvLayout->addWidget(myVerboseLevelSpin,   4, 1);
  
  anAdvLayout->addWidget(myToCreateNewNodesCheck, 5, 0);

  anAdvLayout->addMultiCellWidget(myBoundaryRecoveryCheck, 6, 6, 0, 1);

  anAdvLayout->addWidget(aTextOptionLabel,     7, 0);
  anAdvLayout->addMultiCellWidget(myTextOption,7, 7, 1, 2);
  

  connect( tab,                  SIGNAL( selected(int) ), this, SLOT( onTabSelected(int) ) );
  connect( myMaximumMemoryCheck, SIGNAL( toggled(bool) ), this, SLOT( onCheckToggled(bool) ));
  connect( myInitialMemoryCheck, SIGNAL( toggled(bool) ), this, SLOT( onCheckToggled(bool) ));
  connect( myBoundaryRecoveryCheck,SIGNAL(toggled(bool)), this, SLOT( onCheckToggled(bool) ));
  connect( dirBtn,               SIGNAL( clicked() ),     this, SLOT( onDirBtnClicked() ) );
  
  return fr;
}

void GHS3DPluginGUI_HypothesisCreator::onTabSelected(int tab)
{
  if ( tab == STD_TAB ) {
    myAdvGroup->hide();
    myStdGroup->show();
  }
  else {
    myStdGroup->hide();
    myAdvGroup->show();
  }
//   qApp->processEvents();
  dlg()->adjustSize();
}

void GHS3DPluginGUI_HypothesisCreator::onCheckToggled(bool on)
{
  const QObject * aSender = sender();
  if ( aSender == myMaximumMemoryCheck )
    myMaximumMemorySpin->setEnabled( on );

  else if ( aSender == myInitialMemoryCheck )
    myInitialMemorySpin->setEnabled( on );

  else if ( aSender == myBoundaryRecoveryCheck ) {
    myOptimizationLevelCombo->setEnabled( !on );
    myInitialMemorySpin->setEnabled( myInitialMemoryCheck->isChecked() && !on );
    myInitialMemoryCheck->setEnabled( !on );
    myOptimizationLevelCombo->setEnabled( !on );
  }
 
}

void GHS3DPluginGUI_HypothesisCreator::onDirBtnClicked()
{
  QString dir = SUIT_FileDlg::getExistingDirectory(dlg(),QString::null, QString::null );
  if ( dir )
    myWorkingDir->setText( dir );
}


bool GHS3DPluginGUI_HypothesisCreator::checkParams() const
{
  if ( !QFileInfo( myWorkingDir->text() ).isWritable() ) {
    SUIT_MessageBox::warn1(dlg(),
                           QObject::tr("SMESH_WRN_WARNING"),
                           QObject::tr("GHS3D_PERMISSION_DENIED"),
                           QObject::tr("SMESH_BUT_OK"));
    return false;
  }
  return true;
}

void GHS3DPluginGUI_HypothesisCreator::retrieveParams() const
{
  GHS3DHypothesisData data;
  readParamsFromHypo( data );

  if( myName )
    myName->setText( data.myName );
  
  myToMeshHolesCheck      ->setChecked    ( data.myToMeshHoles );
  myOptimizationLevelCombo->setCurrentItem( data.myOptimizationLevel );
  myMaximumMemoryCheck    ->setChecked    ( data.myMaximumMemory > 0 );
  myMaximumMemorySpin     ->setValue      ( std::max( data.myMaximumMemory,
                                                      myMaximumMemorySpin->minValue() ));
  myInitialMemoryCheck    ->setChecked    ( data.myInitialMemory > 0 );
  myInitialMemorySpin     ->setValue      ( std::max( data.myInitialMemory,
                                                      myInitialMemorySpin->minValue() ));
  myWorkingDir            ->setText       ( data.myWorkingDir );
  myKeepFiles             ->setChecked    ( data.myKeepFiles );
  myVerboseLevelSpin      ->setValue      ( data.myVerboseLevel );
  myToCreateNewNodesCheck ->setChecked    ( data.myToCreateNewNodes );
  myBoundaryRecoveryCheck ->setChecked    ( data.myBoundaryRecovery );
  myTextOption            ->setText       ( data.myTextOption );

  myOptimizationLevelCombo->setEnabled    ( !data.myBoundaryRecovery );
  myMaximumMemorySpin     ->setEnabled    ( myMaximumMemoryCheck->isChecked() );
  myInitialMemorySpin     ->setEnabled    ( myInitialMemoryCheck->isChecked() &&
                                            !data.myBoundaryRecovery );
  myInitialMemoryCheck    ->setEnabled    (!data.myBoundaryRecovery );
  myOptimizationLevelCombo->setEnabled    (!data.myBoundaryRecovery );
}

QString GHS3DPluginGUI_HypothesisCreator::storeParams() const
{
  GHS3DHypothesisData data;
  readParamsFromWidgets( data );
  storeParamsToHypo( data );
  
  QString valStr = "";

  if ( !data.myBoundaryRecovery )
    valStr = "-c " + QString::number( !data.myToMeshHoles );

  if ( data.myOptimizationLevel >= 0 && data.myOptimizationLevel < 4 && !data.myBoundaryRecovery) {
    char* level[] = { "none" , "light" , "standard" , "strong" };
    valStr += " -o ";
    valStr += level[ data.myOptimizationLevel ];
  }
  if ( data.myMaximumMemory > 0 ) {
    valStr += " -m ";
    valStr += QString::number( data.myMaximumMemory );
  }
  if ( data.myInitialMemory > 0 && !data.myBoundaryRecovery ) {
    valStr += " -M ";
    valStr += QString::number( data.myInitialMemory );
  }
  valStr += " -v ";
  valStr += QString::number( data.myVerboseLevel );

  if ( !data.myToCreateNewNodes )
    valStr += " -p0";

  if ( data.myBoundaryRecovery )
    valStr += " -C";

  valStr += " ";
  valStr += data.myTextOption;

  return valStr;
}

bool GHS3DPluginGUI_HypothesisCreator::readParamsFromHypo( GHS3DHypothesisData& h_data ) const
{
  GHS3DPlugin::GHS3DPlugin_Hypothesis_var h =
    GHS3DPlugin::GHS3DPlugin_Hypothesis::_narrow( initParamsHypothesis() );

  HypothesisData* data = SMESH::GetHypothesisData( hypType() );
  h_data.myName = isCreation() && data ? hypName() : "";

  h_data.myToMeshHoles       = h->GetToMeshHoles();
  h_data.myMaximumMemory     = h->GetMaximumMemory();
  h_data.myInitialMemory     = h->GetInitialMemory();
  h_data.myInitialMemory     = h->GetInitialMemory();
  h_data.myOptimizationLevel = h->GetOptimizationLevel();
  h_data.myKeepFiles         = h->GetKeepFiles();
  h_data.myWorkingDir        = h->GetWorkingDirectory();
  h_data.myVerboseLevel      = h->GetVerboseLevel();
  h_data.myToCreateNewNodes  = h->GetToCreateNewNodes();
  h_data.myBoundaryRecovery  = h->GetToUseBoundaryRecoveryVersion();
  h_data.myTextOption        = h->GetTextOption();
  
  return true;
}

bool GHS3DPluginGUI_HypothesisCreator::storeParamsToHypo( const GHS3DHypothesisData& h_data ) const
{
  GHS3DPlugin::GHS3DPlugin_Hypothesis_var h =
    GHS3DPlugin::GHS3DPlugin_Hypothesis::_narrow( hypothesis() );

  bool ok = true;
  try
  {
    if( isCreation() )
      SMESH::SetName( SMESH::FindSObject( h ), h_data.myName.latin1() );

    if ( h->GetToMeshHoles() != h_data.myToMeshHoles ) // avoid duplication of DumpPython commands
      h->SetToMeshHoles      ( h_data.myToMeshHoles       );
    if ( h->GetMaximumMemory() != h_data.myMaximumMemory )
      h->SetMaximumMemory    ( h_data.myMaximumMemory     );
    if ( h->GetInitialMemory() != h_data.myInitialMemory )
      h->SetInitialMemory    ( h_data.myInitialMemory     );
    if ( h->GetInitialMemory() != h_data.myInitialMemory )
      h->SetInitialMemory    ( h_data.myInitialMemory     );
    if ( h->GetOptimizationLevel() != h_data.myOptimizationLevel )
      h->SetOptimizationLevel( h_data.myOptimizationLevel );
    if ( h->GetKeepFiles() != h_data.myKeepFiles )
      h->SetKeepFiles        ( h_data.myKeepFiles         );
    if ( h->GetWorkingDirectory() != h_data.myWorkingDir )
      h->SetWorkingDirectory ( h_data.myWorkingDir        );
    if ( h->GetVerboseLevel() != h_data.myVerboseLevel )
      h->SetVerboseLevel     ( h_data.myVerboseLevel );
    if ( h->GetToCreateNewNodes() != h_data.myToCreateNewNodes )
      h->SetToCreateNewNodes( h_data.myToCreateNewNodes );
    if ( h->GetToUseBoundaryRecoveryVersion() != h_data.myBoundaryRecovery )
      h->SetToUseBoundaryRecoveryVersion( h_data.myBoundaryRecovery );
    if ( h->GetTextOption() != h_data.myTextOption )
      h->SetTextOption       ( h_data.myTextOption );
  }
  catch(const SALOME::SALOME_Exception& ex)
  {
    SalomeApp_Tools::QtCatchCorbaException(ex);
    ok = false;
  }
  return ok;
}

bool GHS3DPluginGUI_HypothesisCreator::readParamsFromWidgets( GHS3DHypothesisData& h_data ) const
{
  h_data.myName              = myName ? myName->text() : "";
  h_data.myToMeshHoles       = myToMeshHolesCheck->isChecked();
  h_data.myMaximumMemory     = myMaximumMemoryCheck->isChecked() ? myMaximumMemorySpin->value() : -1;
  h_data.myInitialMemory     = myInitialMemoryCheck->isChecked() ? myInitialMemorySpin->value() : -1;
  h_data.myOptimizationLevel = myOptimizationLevelCombo->currentItem();
  h_data.myKeepFiles         = myKeepFiles->isChecked();
  h_data.myWorkingDir        = myWorkingDir->text();
  h_data.myVerboseLevel      = myVerboseLevelSpin->value();
  h_data.myToCreateNewNodes  = myToCreateNewNodesCheck->isChecked();
  h_data.myBoundaryRecovery  = myBoundaryRecoveryCheck->isChecked();
  h_data.myTextOption        = myTextOption->text();

  return true;
}

QString GHS3DPluginGUI_HypothesisCreator::caption() const
{
  return tr( "GHS3D_TITLE" );
}

QPixmap GHS3DPluginGUI_HypothesisCreator::icon() const
{
  QString hypIconName = tr( QString("ICON_DLG_GHS3D_PARAMETERS" ));
  return SUIT_Session::session()->resourceMgr()->loadPixmap( "GHS3DPlugin", hypIconName );
}

QString GHS3DPluginGUI_HypothesisCreator::type() const
{
  return tr( "GHS3D_HYPOTHESIS" );
}

QString GHS3DPluginGUI_HypothesisCreator::helpPage() const
{
  return "ghs3d_hypo_page.html";
}

//=============================================================================
/*! GetHypothesisCreator
 *
 */
//=============================================================================
extern "C"
{
  GHS3DPLUGIN_EXPORT
  SMESHGUI_GenericHypothesisCreator* GetHypothesisCreator( const QString& aHypType )
  {
    if( aHypType=="GHS3D_Parameters" )
      return new GHS3DPluginGUI_HypothesisCreator( aHypType );
    return 0;
  }
}