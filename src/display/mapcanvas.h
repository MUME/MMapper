/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MAPCANVAS_H
#define MAPCANVAS_H

#include "mmapper2exit.h"
#include "coordinate.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <vector>
#include <set>

class QOpenGLTexture;
class QOpenGLShaderProgram;
class MapData;
class Room;
class InfoMark;
class InfoMarksEditDlg;
class RoomSelection;
class ConnectionSelection;
class ParseEvent;
class CGroup;
class CGroupCharacter;
class RoomRecipient;
class PrespammedPath;
class Coordinate;

class MapCanvas : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    MapCanvas( MapData *mapData, PrespammedPath *prespammedPath, CGroup *groupManager,
               QWidget *parent = 0 );
    ~MapCanvas();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    static float SCROLLFACTOR();
    float getDW() const;
    float getDH() const;

    MapData *getData ()
    {
        return m_data;
    }
    //void receiveRoom(RoomAdmin * admin, AbstractRoom * room);

    enum CanvasMouseMode { CMM_NONE, CMM_MOVE, CMM_SELECT_ROOMS, CMM_SELECT_CONNECTIONS,
                           CMM_CREATE_ROOMS, CMM_CREATE_CONNECTIONS, CMM_CREATE_ONEWAY_CONNECTIONS,
                           CMM_EDIT_INFOMARKS
                         };

    void drawRoom(const Room *room, const std::vector<Room *> &rooms,
                  const std::vector<std::set<RoomRecipient *> > &locks);



public slots:
    void forceMapperToRoom();
    void onInfoMarksEditDlgClose();

    void setCanvasMouseMode(CanvasMouseMode);

    void setHorizontalScroll(int x);
    void setVerticalScroll(int y);
    void setScroll(int x, int y);
    void zoomIn();
    void zoomOut();
    void layerUp();
    void layerDown();

    void clearRoomSelection();
    void clearConnectionSelection();

    //void worldChanged();

    void dataLoaded();
    void moveMarker(const Coordinate &);

signals:
    void onEnsureVisible( qint32 x, qint32 y );
    void onCenter( qint32 x, qint32 y );

    void mapMove(int dx, int dy);
    void setScrollBars(const Coordinate &ulf, const Coordinate &lrb);

    void log( const QString &, const QString & );

    void continuousScroll(qint8, qint8);

    void newRoomSelection(const RoomSelection *);
    void newConnectionSelection(ConnectionSelection *);

    void setCurrentRoom(uint id);
    void roomPositionChanged();

    //for main move/search algorithm
    void charMovedEvent(ParseEvent * );

protected:
    //void closeEvent(QCloseEvent *event);

    void initializeGL();
    void paintGL();
    void drawGroupCharacters();
    void drawCharacter(const Coordinate &c, QColor color);
    void drawRoomDoorName(const Room *sourceRoom, uint sourceDir, const Room *targetRoom,
                          uint targetDir);
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent ( QWheelEvent *event );

    void alphaOverlayTexture(QOpenGLTexture *texture);

    void drawConnection( const Room *leftRoom, const Room *rightRoom,
                         ExitDirection connectionStartDirection, ExitDirection connectionEndDirection, bool oneway,
                         bool inExitFlags = true );
    void drawInfoMark(InfoMark *);
    void drawPathStart(const Coordinate &);
    bool drawPath(const Coordinate &sc, const Coordinate &dc, double &dx, double &dy, double &dz);
    void drawPathEnd(double dx, double dy, double dz);
    void drawFlow( const Room *room, const std::vector<Room *> &rooms, ExitDirection exitDirection);

    // QGLWidget backwards comptability
    void inline qglColor(QColor c)
    {
        glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    }
    void inline qglClearColor(QColor c)
    {
        glClearColor(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    }
    enum FontFormatFlags {FFF_NONE = 0, FFF_ITALICS = 1, FFF_UNDERLINE = 2};
    void renderText(double x, double y, const QString &str, QColor color = Qt::white,
                    uint fontFormatFlags = FFF_NONE, double rotationAngle = 0.0f);

private:
    static QColor m_noFleeColor;

    GLint    m_viewport[4];
    GLdouble m_modelview[16];
    GLdouble m_projection[16];

    QOpenGLContext *m_context;

    QOpenGLTexture *m_terrainTextures[16];
    QOpenGLTexture *m_roadTextures[16];
    QOpenGLTexture *m_loadTextures[16];
    QOpenGLTexture *m_mobTextures[15];
    QOpenGLTexture *m_updateTextures;
    QOpenGLTexture *m_trailTextures[16];

    void moveSelection(const RoomSelection *sel, int dx, int dy);

    void normalizeAngle(int *angle);
    void makeGlLists();

    float m_scaleFactor;
    int m_scrollX, m_scrollY;
    GLdouble m_visibleX1, m_visibleY1;
    GLdouble m_visibleX2, m_visibleY2;

    bool m_mouseRightPressed;
    bool m_mouseLeftPressed;
    bool m_altPressed;
    bool m_ctrlPressed;

    CanvasMouseMode m_canvasMouseMode;

    //mouse selection
    GLdouble m_selX1, m_selY1, m_selX2, m_selY2;
    int m_selLayer1, m_selLayer2;

    GLdouble m_moveX1backup, m_moveY1backup;

    bool        m_selectedArea;
    const RoomSelection *m_roomSelection;

    bool m_roomSelectionMove;
    bool m_roomSelectionMoveWrongPlace;
    int m_roomSelectionMoveX;
    int m_roomSelectionMoveY;

    bool m_infoMarkSelection;

    ConnectionSelection *m_connectionSelection;

    Coordinate m_mapMove;

    MapData *m_data;
    PrespammedPath *m_prespammedPath;
    CGroup *m_groupManager;

    qint16 m_currentLayer;

    GLuint m_wall_north_gllist;
    GLuint m_wall_south_gllist;
    GLuint m_wall_east_gllist;
    GLuint m_wall_west_gllist;
    GLuint m_exit_up_gllist;
    GLuint m_exit_down_gllist;
    GLuint m_exit_up_transparent_gllist;
    GLuint m_exit_down_transparent_gllist;

    GLuint m_door_north_gllist;
    GLuint m_door_south_gllist;
    GLuint m_door_east_gllist;
    GLuint m_door_west_gllist;
    GLuint m_door_up_gllist;
    GLuint m_door_down_gllist;

    GLuint m_room_gllist;
    GLuint m_room_selection_gllist;
    GLuint m_room_selection_inner_gllist;
    GLuint m_character_hint_gllist;
    GLuint m_character_hint_inner_gllist;

    GLuint m_flow_begin_gllist[6];
    GLuint m_flow_end_gllist[6];

    QFont *m_glFont;
    QFontMetrics *m_glFontMetrics;

    InfoMarksEditDlg *m_infoMarksEditDlg;

    int tmpx, tmpy;

    int GLtoMap(double arg);

    GLint UnProject(GLdouble winx, GLdouble winy, GLdouble winz, GLdouble *objx, GLdouble *objy,
                    GLdouble *objz);
    GLint Project(GLdouble objx, GLdouble objy, GLdouble objz, GLdouble *winx, GLdouble *winy,
                  GLdouble *winz);

    void MultMatrixVecd(const GLdouble matrix[16], const GLdouble in[4], GLdouble out[4]);
    void MultMatricesd(const GLdouble a[16], const GLdouble b[16], GLdouble r[16]);
    void MakeIdentityd(GLdouble m[16]);
    int InvertMatrixd(const GLdouble src[16], GLdouble inverse[16]);
};


#endif
