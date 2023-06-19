// Copyright (C) 2007-2023  CEA, EDF
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

#include "GHS3DPluginGUI_TreeWidget.h"
#include <QKeyEvent>

namespace
{
  bool isEditable( const QModelIndex& index )
  {
    return index.isValid() &&
      index.flags() & Qt::ItemIsEditable && 
      index.flags() & Qt::ItemIsEnabled &&
      ( !index.data( Qt::UserRole + 1 ).isValid() || index.data( Qt::UserRole + 1 ).toInt() != 0 );
  }
}

GHS3DPluginGUI_TreeWidget::GHS3DPluginGUI_TreeWidget( QWidget* parent )
  : QTreeWidget( parent )
{
}

QModelIndex GHS3DPluginGUI_TreeWidget::moveCursor( CursorAction action, Qt::KeyboardModifiers modifiers )
{
  QModelIndex current = currentIndex();
  int column = current.column();
  if ( action == MoveNext ) {
    if ( column < columnCount()-1 ) {
      QModelIndex next = current.sibling( current.row(), column+1 );
      if ( isEditable( next ) )
        return next;
    }
    else {
      QModelIndex next = current.sibling( current.row()+1, 0 );
      if ( isEditable( next ) )
        return next;
    }
  }
  else if ( action == MovePrevious ) {
    if ( column == 0 ) {
      QModelIndex next = current.sibling( current.row()-1, columnCount()-1 );
      if ( isEditable( next ) )
        return next;
    }
    else {
      QModelIndex next = current.sibling( current.row(), column-1 );
      if ( isEditable( next ) )
        return next;
    }
  }
  return QTreeWidget::moveCursor( action, modifiers );
}

void GHS3DPluginGUI_TreeWidget::keyPressEvent( QKeyEvent* e )
{
  switch ( e->key() ) {
  case Qt::Key_F2:
    {
      QModelIndex index = currentIndex();
      if ( !isEditable( index ) ) {
        for ( int i = 0; i < columnCount(); i++ ) {
          QModelIndex sibling = index.sibling( index.row(), i );
          if ( isEditable( sibling ) ) {
            if ( !edit( sibling, EditKeyPressed, e ) )
              e->ignore();
          }
        }
      }
    }
    break;
  default:
    break;
  }
  QTreeWidget::keyPressEvent( e );
}
