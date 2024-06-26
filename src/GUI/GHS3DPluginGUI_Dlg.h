// Copyright (C) 2007-2024  CEA, EDF
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

// File   : GHS3DPluginGUI_Dlg.h
// Author : Renaud NEDELEC (OpenCascade)

#ifndef GHS3DPLUGINGUI_H
#define GHS3DPLUGINGUI_H

enum {
  OPTION_ID_COLUMN = 0,
  OPTION_TYPE_COLUMN,
  OPTION_NAME_COLUMN = 0,
  OPTION_VALUE_COLUMN,
  NB_COLUMNS,
};

//////////////////////////////////////////
// GHS3DPluginGUI_AdvWidget
//////////////////////////////////////////

#include "ui_GHS3DPluginGUI_AdvWidget_QTD.h"
#include "GHS3DPluginGUI_HypothesisCreator.h"

class GHS3DPLUGINGUI_EXPORT GHS3DPluginGUI_AdvWidget : public QWidget, 
                                            public Ui::GHS3DPluginGUI_AdvWidget_QTD
{
  Q_OBJECT

public:
  GHS3DPluginGUI_AdvWidget( QWidget* = 0, Qt::WindowFlags = 0 );
  ~GHS3DPluginGUI_AdvWidget();

  void AddOption( const char* name_value_type, bool isCustom = false );
  void GetOptionAndValue( QTreeWidgetItem * tblRow, QString& option, QString& value, bool& dflt );
  void EnableAdvancedOptions( bool isMGTetra );
public slots:

  void itemChanged(QTreeWidgetItem * tblRow, int column);
};

#endif
