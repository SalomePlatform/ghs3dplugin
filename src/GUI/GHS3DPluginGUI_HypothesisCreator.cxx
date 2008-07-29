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

#include <SUIT_MessageBox.h>
#include <SUIT_Session.h>
#include <SUIT_FileDlg.h>
#include <SUIT_ResourceMgr.h>
#include <SalomeApp_Tools.h>

#include <QLabel>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QTabWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QFileInfo>

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
  QFrame* fr = new QFrame( 0 );
  QVBoxLayout* lay = new QVBoxLayout( fr );
  lay->setMargin( 5 );
  lay->setSpacing( 0 );

  // tab
  QTabWidget* tab = new QTabWidget( fr );
  tab->setTabShape( QTabWidget::Rounded );
  tab->setTabPosition( QTabWidget::North );
  lay->addWidget( tab );

  // basic parameters
  myStdGroup = new QWidget();
  QGridLayout* aStdLayout = new QGridLayout( myStdGroup );
  aStdLayout->setSpacing( 6 );
  aStdLayout->setMargin( 11 );

  int row = 0;
  myName = 0;
  if( isCreation() )
  {
    aStdLayout->addWidget( new QLabel( tr( "SMESH_NAME" ), myStdGroup ), row, 0, 1, 1 );
    myName = new QLineEdit( myStdGroup );
    aStdLayout->addWidget( myName, row++, 1, 1, 1 );
  }

  myToMeshHolesCheck = new QCheckBox( tr( "GHS3D_TO_MESH_HOLES" ), myStdGroup );
  aStdLayout->addWidget( myToMeshHolesCheck, row++, 0, 1, 2 );

  aStdLayout->addWidget( new QLabel( tr( "GHS3D_OPTIMIZATIOL_LEVEL" ), myStdGroup ), row, 0 );
  myOptimizationLevelCombo = new QComboBox( myStdGroup );
  aStdLayout->addWidget( myOptimizationLevelCombo, row++, 1, 1, 1 );

  QStringList types;
  types << tr( "LEVEL_NONE" ) << tr( "LEVEL_LIGHT" ) << tr( "LEVEL_MEDIUM" ) << tr( "LEVEL_STRONG" );
  myOptimizationLevelCombo->addItems( types );

  aStdLayout->setRowStretch( row, 5 );

  // advanced parameters
  myAdvGroup = new QWidget();
  QGridLayout* anAdvLayout = new QGridLayout( myAdvGroup );
  anAdvLayout->setSpacing( 6 );
  anAdvLayout->setMargin( 11 );
  
  myMaximumMemoryCheck = new QCheckBox( tr( "MAX_MEMORY_SIZE" ), myAdvGroup );
  myMaximumMemorySpin = new QSpinBox( myAdvGroup );
  myMaximumMemorySpin->setMinimum( 1 );
  myMaximumMemorySpin->setMaximum( maxAvailableMemory() );
  myMaximumMemorySpin->setSingleStep( 10 );
  QLabel* aMegabyteLabel = new QLabel( tr( "MEGABYTE" ), myAdvGroup );

  myInitialMemoryCheck = new QCheckBox( tr( "INIT_MEMORY_SIZE" ), myAdvGroup );
  myInitialMemorySpin = new QSpinBox( myAdvGroup );
  myInitialMemorySpin->setMinimum( 1 );
  myInitialMemorySpin->setMaximum( maxAvailableMemory() );
  myInitialMemorySpin->setSingleStep( 10 );
  QLabel* aMegabyteLabel2 = new QLabel( tr( "MEGABYTE" ), myAdvGroup );

  QLabel* aWorkinDirLabel = new QLabel( tr( "WORKING_DIR" ), myAdvGroup );
  myWorkingDir = new QLineEdit( myAdvGroup );
  //myWorkingDir->setReadOnly( true );
  QPushButton* dirBtn = new QPushButton( tr( "SELECT_DIR" ), myAdvGroup );
  dirBtn->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  
  myKeepFiles = new QCheckBox( tr( "KEEP_WORKING_FILES" ), myAdvGroup );

  QLabel* aVerboseLevelLabel = new QLabel( tr( "VERBOSE_LEVEL" ), myAdvGroup );
  myVerboseLevelSpin = new QSpinBox( myAdvGroup );
  myVerboseLevelSpin->setMinimum( 0 );
  myVerboseLevelSpin->setMaximum( 10 );
  myVerboseLevelSpin->setSingleStep( 1 );

  myToCreateNewNodesCheck = new QCheckBox( tr( "TO_ADD_NODES" ), myAdvGroup );
  
  myBoundaryRecoveryCheck = new QCheckBox( tr( "RECOVERY_VERSION" ), myAdvGroup );

  QLabel* aTextOptionLabel = new QLabel( tr( "TEXT_OPTION" ), myAdvGroup );
  myTextOption = new QLineEdit( myAdvGroup );

  anAdvLayout->addWidget( myMaximumMemoryCheck,    0, 0, 1, 1 );
  anAdvLayout->addWidget( myMaximumMemorySpin,     0, 1, 1, 1 );
  anAdvLayout->addWidget( aMegabyteLabel,          0, 2, 1, 1 );
  anAdvLayout->addWidget( myInitialMemoryCheck,    1, 0, 1, 1 );
  anAdvLayout->addWidget( myInitialMemorySpin,     1, 1, 1, 1 );
  anAdvLayout->addWidget( aMegabyteLabel2,         1, 2, 1, 1 );
  anAdvLayout->addWidget( aWorkinDirLabel,         2, 0, 1, 1 );
  anAdvLayout->addWidget( myWorkingDir,            2, 1, 1, 2 );
  anAdvLayout->addWidget( dirBtn,                  2, 3, 1, 1 );
  anAdvLayout->addWidget( myKeepFiles,             3, 0, 1, 4 );
  anAdvLayout->addWidget( aVerboseLevelLabel,      4, 0, 1, 1 );
  anAdvLayout->addWidget( myVerboseLevelSpin,      4, 1, 1, 1 );
  anAdvLayout->addWidget( myToCreateNewNodesCheck, 5, 0, 1, 4 );
  anAdvLayout->addWidget( myBoundaryRecoveryCheck, 6, 0, 1, 4 );
  anAdvLayout->addWidget( aTextOptionLabel,        7, 0, 1, 1 );
  anAdvLayout->addWidget( myTextOption,            7, 1, 1, 2 );

  // add tabs
  tab->insertTab( STD_TAB, myStdGroup, tr( "SMESH_ARGUMENTS" ) );
  tab->insertTab( ADV_TAB, myAdvGroup, tr( "GHS3D_ADV_ARGS" ) );
  tab->setCurrentIndex( STD_TAB );

  // connections
  connect( myMaximumMemoryCheck,    SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myInitialMemoryCheck,    SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myBoundaryRecoveryCheck, SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( dirBtn,                  SIGNAL( clicked() ),       this, SLOT( onDirBtnClicked() ) );
  
  return fr;
}

void GHS3DPluginGUI_HypothesisCreator::onDirBtnClicked()
{
  QString dir = SUIT_FileDlg::getExistingDirectory( dlg(), myWorkingDir->text(), QString() );
  if ( !dir.isEmpty() )
    myWorkingDir->setText( dir );
}

void GHS3DPluginGUI_HypothesisCreator::updateWidgets()
{
  myMaximumMemorySpin->setEnabled( myMaximumMemoryCheck->isChecked() );
  myInitialMemoryCheck->setEnabled( !myBoundaryRecoveryCheck->isChecked() );
  myInitialMemorySpin->setEnabled( myInitialMemoryCheck->isChecked() && !myBoundaryRecoveryCheck->isChecked() );
  myOptimizationLevelCombo->setEnabled( !myBoundaryRecoveryCheck->isChecked() );
}

bool GHS3DPluginGUI_HypothesisCreator::checkParams() const
{
  if ( !QFileInfo( myWorkingDir->text().trimmed() ).isWritable() ) {
    SUIT_MessageBox::warning( dlg(),
			      tr( "SMESH_WRN_WARNING" ),
			      tr( "GHS3D_PERMISSION_DENIED" ) );
    return false;
  }
  return true;
}

void GHS3DPluginGUI_HypothesisCreator::retrieveParams() const
{
  GHS3DHypothesisData data;
  readParamsFromHypo( data );

  if ( myName )
    myName->setText( data.myName );
  
  myToMeshHolesCheck      ->setChecked    ( data.myToMeshHoles );
  myOptimizationLevelCombo->setCurrentIndex( data.myOptimizationLevel );
  myMaximumMemoryCheck    ->setChecked    ( data.myMaximumMemory > 0 );
  myMaximumMemorySpin     ->setValue      ( qMax( data.myMaximumMemory,
						  myMaximumMemorySpin->minimum() ));
  myInitialMemoryCheck    ->setChecked    ( data.myInitialMemory > 0 );
  myInitialMemorySpin     ->setValue      ( qMax( data.myInitialMemory,
						  myInitialMemorySpin->minimum() ));
  myWorkingDir            ->setText       ( data.myWorkingDir );
  myKeepFiles             ->setChecked    ( data.myKeepFiles );
  myVerboseLevelSpin      ->setValue      ( data.myVerboseLevel );
  myToCreateNewNodesCheck ->setChecked    ( data.myToCreateNewNodes );
  myBoundaryRecoveryCheck ->setChecked    ( data.myBoundaryRecovery );
  myTextOption            ->setText       ( data.myTextOption );
  
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  that->updateWidgets();
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
      SMESH::SetName( SMESH::FindSObject( h ), h_data.myName.toLatin1().constData() );

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
      h->SetWorkingDirectory ( h_data.myWorkingDir.toLatin1().constData() );
    if ( h->GetVerboseLevel() != h_data.myVerboseLevel )
      h->SetVerboseLevel     ( h_data.myVerboseLevel );
    if ( h->GetToCreateNewNodes() != h_data.myToCreateNewNodes )
      h->SetToCreateNewNodes( h_data.myToCreateNewNodes );
    if ( h->GetToUseBoundaryRecoveryVersion() != h_data.myBoundaryRecovery )
      h->SetToUseBoundaryRecoveryVersion( h_data.myBoundaryRecovery );
    if ( h->GetTextOption() != h_data.myTextOption )
      h->SetTextOption       ( h_data.myTextOption.toLatin1().constData() );
  }
  catch ( const SALOME::SALOME_Exception& ex )
  {
    SalomeApp_Tools::QtCatchCorbaException( ex );
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
  h_data.myOptimizationLevel = myOptimizationLevelCombo->currentIndex();
  h_data.myKeepFiles         = myKeepFiles->isChecked();
  h_data.myWorkingDir        = myWorkingDir->text().trimmed();
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
  return SUIT_Session::session()->resourceMgr()->loadPixmap( "GHS3DPlugin", tr( "ICON_DLG_GHS3D_PARAMETERS" ) );
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
    if ( aHypType == "GHS3D_Parameters" )
      return new GHS3DPluginGUI_HypothesisCreator( aHypType );
    return 0;
  }
}
