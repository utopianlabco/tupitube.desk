/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              * 
 *                                                                         *
 *   Developers:                                                           *
 *   2025:                                                                 *
 *    Utopian Lab Development Team                                         *
 *   2010:                                                                 *
 *    Gustav Gonzalez                                                      *
 *   ---                                                                   *
 *   KTooN's versions:                                                     *
 *   2006:                                                                 *
 *    David Cuadrado                                                       *
 *    Jorge Cuadrado                                                       *
 *   2003:                                                                 *
 *    Fernado Roldan                                                       *
 *    Simena Dinas                                                         *
 *                                                                         *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "tupcanvas.h"
#include "timagebutton.h"
#include "tuppendialog.h"
#include "tuponiondialog.h"

#include <QColorDialog>
#include <QIcon>
#include <QBoxLayout>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QBuffer>

TupCanvas::TupCanvas(QWidget *parent, Qt::WindowFlags flags, TupGraphicsScene *gScene,
                   const QPointF centerPoint, const QSize &screenSize, TupProject *work, qreal scaleFactor,
                   int angle, TupBrushManager *manager) : QDialog(parent, flags)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCanvas()]";
    #endif

    setModal(true);
    setWindowTitle("TupiTube Desk");
    setWindowIcon(QIcon(QPixmap(THEME_DIR + "icons/animation_mode.png")));

    hand = Right;
    scene = gScene;
    frameIndex = scene->currentFrameIndex();

    project = work;
    size = work->getDimension();
    currentColor = manager->penColor();
    brushManager = manager;

    screen = QGuiApplication::screens().at(0);

    int iconSize = 50;
    int windowHeight = screen->geometry().height();
    if (windowHeight < 1080)
        iconSize = windowHeight*0.04;

    graphicsView = new TupCanvasView(this, gScene, screenSize, size, work->getCurrentBgColor());
    connect(graphicsView, SIGNAL(rightClick()), this, SIGNAL(rightClick()));
    connect(graphicsView, SIGNAL(zoomIn()), this, SLOT(wakeUpZoomIn()));
    connect(graphicsView, SIGNAL(zoomOut()), this, SLOT(wakeUpZoomOut()));
    connect(graphicsView, SIGNAL(frameBackward()), this, SLOT(oneFrameBack()));
    connect(graphicsView, SIGNAL(frameForward()), this, SLOT(oneFrameForward()));
    connect(graphicsView, SIGNAL(saveRequested()), this, SIGNAL(saveRequested()));
    connect(graphicsView, SIGNAL(undoRequested()), this, SIGNAL(undoRequested()));
    connect(graphicsView, SIGNAL(redoRequested()), this, SIGNAL(redoRequested()));
    connect(graphicsView, SIGNAL(selectToolLaunched()), this, SIGNAL(selectToolLaunched()));

    graphicsView->centerOn(centerPoint);
    graphicsView->scale(scaleFactor, scaleFactor);
    graphicsView->rotate(angle);

    TImageButton *frameBackward = new TImageButton(QPixmap(THEME_DIR + "icons/frame_backward_big.png"), iconSize, this);
    frameBackward->setToolTip(tr("Frame Backward"));
    connect(frameBackward, SIGNAL(clicked()), this, SLOT(oneFrameBack()));

    TImageButton *frameForward = new TImageButton(QPixmap(THEME_DIR + "icons/frame_forward_big.png"), iconSize, this);
    frameForward->setToolTip(tr("Frame Forward"));
    connect(frameForward, SIGNAL(clicked()), this, SLOT(oneFrameForward()));

    TImageButton *pencil = new TImageButton(QPixmap(THEME_DIR + "icons/pencil_big.png"), iconSize, this);
    pencil->setToolTip(tr("Pencil"));
    connect(pencil, SIGNAL(clicked()), this, SLOT(wakeUpPencil()));

    TImageButton *polyline = new TImageButton(QPixmap(THEME_DIR + "icons/polyline_big.png"), iconSize, this);
    polyline->setToolTip(tr("Polyline"));
    connect(polyline, SIGNAL(clicked()), this, SLOT(wakeUpPolyline()));

    TImageButton *rectangle = new TImageButton(QPixmap(THEME_DIR + "icons/square_big.png"), iconSize, this);
    rectangle->setToolTip(tr("Rectangle"));
    connect(rectangle, SIGNAL(clicked()), this, SLOT(wakeUpRectangle()));

    TImageButton *ellipse = new TImageButton(QPixmap(THEME_DIR + "icons/ellipse_big.png"), iconSize, this);
    ellipse->setToolTip(tr("Ellipse"));
    connect(ellipse, SIGNAL(clicked()), this, SLOT(wakeUpEllipse()));

    TImageButton *selection = new TImageButton(QPixmap(THEME_DIR + "icons/selection_big.png"), iconSize, this);
    selection->setToolTip(tr("Selection"));
    connect(selection, SIGNAL(clicked()), this, SLOT(wakeUpSelection()));

    TImageButton *trash = new TImageButton(QPixmap(THEME_DIR + "icons/delete_big.png"), iconSize, this);
    trash->setToolTip(tr("Delete Selection"));
    connect(trash, SIGNAL(clicked()), this, SLOT(wakeUpDeleteSelection()));

    TImageButton *nodes = new TImageButton(QPixmap(THEME_DIR + "icons/nodes_big.png"), iconSize, this);
    nodes->setToolTip(tr("Nodes"));
    connect(nodes, SIGNAL(clicked()), this, SLOT(wakeUpNodes()));

    TImageButton *undo = new TImageButton(QPixmap(THEME_DIR + "icons/undo_big.png"), iconSize, this);
    undo->setToolTip(tr("Undo"));
    connect(undo, SIGNAL(clicked()), this, SLOT(undo()));

    TImageButton *redo = new TImageButton(QPixmap(THEME_DIR + "icons/redo_big.png"), iconSize, this);
    redo->setToolTip(tr("Redo"));
    connect(redo, SIGNAL(clicked()), this, SLOT(redo()));

    TImageButton *zoomIn = new TImageButton(QPixmap(THEME_DIR + "icons/zoom_in_big.png"), iconSize, this);
    zoomIn->setToolTip(tr("Zoom In"));
    connect(zoomIn, SIGNAL(clicked()), this, SLOT(wakeUpZoomIn()));

    TImageButton *zoomOut = new TImageButton(QPixmap(THEME_DIR + "icons/zoom_out_big.png"), iconSize, this);
    zoomOut->setToolTip(tr("Zoom Out"));
    connect(zoomOut, SIGNAL(clicked()), this, SLOT(wakeUpZoomOut()));

    TImageButton *images = new TImageButton(QPixmap(THEME_DIR + "icons/bitmap_big.png"), iconSize, this);
    images->setToolTip(tr("Images"));
    connect(images, SIGNAL(clicked()), this, SLOT(wakeUpLibrary()));

    TImageButton *color = new TImageButton(QPixmap(THEME_DIR + "icons/color_palette_big.png"), iconSize, this);
    color->setToolTip(tr("Color Palette"));
    connect(color, SIGNAL(clicked()), this, SLOT(showColorDialog()));

    TImageButton *size = new TImageButton(QPixmap(THEME_DIR + "icons/pen_properties.png"), iconSize, this);
    size->setToolTip(tr("Pen Size"));
    connect(size, SIGNAL(clicked()), this, SLOT(penDialog()));

    TImageButton *onion = new TImageButton(QPixmap(THEME_DIR + "icons/onion_big.png"), iconSize, this);
    onion->setToolTip(tr("Onion Skin Factor"));
    connect(onion, SIGNAL(clicked()), this, SLOT(onionDialog()));

    TImageButton *close = new TImageButton(QPixmap(THEME_DIR + "icons/close_big.png"), iconSize, this);
    close->setToolTip(tr("Close Full Screen"));
    connect(close, SIGNAL(clicked()), this, SIGNAL(closeHugeCanvas()));

    QBoxLayout *controls = new QBoxLayout(QBoxLayout::TopToBottom);
    controls->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    int spacing = 5;
    if (windowHeight < 1080) {
        spacing = windowHeight*0.015;
        controls->setContentsMargins(5, 5, 5, 5);
    } else { 
        controls->setContentsMargins(3, 10, 3, 3);
    }

    controls->setSpacing(spacing);

    controls->addWidget(frameBackward);
    controls->addWidget(frameForward);
    controls->addWidget(pencil);
    controls->addWidget(polyline);
    controls->addWidget(rectangle);
    controls->addWidget(ellipse);
    controls->addWidget(selection);
    controls->addWidget(trash);
    controls->addWidget(nodes);
    controls->addWidget(undo);
    controls->addWidget(redo);
    controls->addWidget(zoomIn);
    controls->addWidget(zoomOut);
    controls->addWidget(images);
    controls->addWidget(color);
    controls->addWidget(size);
    controls->addWidget(onion);
    controls->addWidget(close);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    if (hand == Right)
        layout->addLayout(controls);

    layout->addWidget(graphicsView);

    if (hand == Left)
        layout->addLayout(controls);

    setLayout(layout);
}

TupCanvas::~TupCanvas()
{
}

void TupCanvas::updateCursor(const QCursor &cursor)
{
    graphicsView->viewport()->setCursor(cursor);
}

void TupCanvas::closeEvent(QCloseEvent *event)
{
    delete graphicsView; 
    event->accept();
}

void TupCanvas::openColorDialog(const QColor &current)
{
    QColor color = QColorDialog::getColor(current, this);
    if (color.isValid()) { 
        currentColor = color;
        emit colorChanged(TColorCell::Contour, color);
    }
}

void TupCanvas::showColorDialog()
{
    QColor color = QColorDialog::getColor(currentColor, this);
    if (color.isValid())
        emit colorChanged(TColorCell::Contour, color);
}

void TupCanvas::penDialog()
{
    TupPenDialog *dialog = new TupPenDialog(brushManager, this);
    connect(dialog, SIGNAL(updatePen(int)), this, SIGNAL(penWidthChangedFromFullScreen(int)));

    QApplication::restoreOverrideCursor();

    dialog->show();
    dialog->move(static_cast<int> ((screen->geometry().width() - dialog->width()) / 2),
                 static_cast<int> ((screen->geometry().height() - dialog->height()) / 2));
}

void TupCanvas::onionDialog()
{
    TupOnionDialog *dialog = new TupOnionDialog(brushManager->penColor(), scene->getOpacity(), this);
    connect(dialog, SIGNAL(updateOpacity(double)), this, SLOT(setOnionOpacity(double)));

    QApplication::restoreOverrideCursor();

    dialog->show();
    dialog->move(static_cast<int> ((screen->geometry().width() - dialog->width()) / 2),
                 static_cast<int> ((screen->geometry().height() - dialog->height()) / 2));
}

void TupCanvas::setOnionOpacity(double opacity)
{
    scene->setOnionFactor(opacity);
    emit onionOpacityChangedFromFullScreen(opacity); 
}

void TupCanvas::oneFrameBack()
{
    if (frameIndex > 0) {
        frameIndex--;
        emit callAction(TAction::Arrows, TAction::Left_Arrow);
    }
}

void TupCanvas::oneFrameForward()
{
    frameIndex++;
    emit callAction(TAction::Arrows, TAction::Right_Arrow);
}

void TupCanvas::wakeUpPencil()
{    
    emit callAction(TAction::BrushesMenu, TAction::Pencil);
}

void TupCanvas::wakeUpPolyline()
{
    emit callAction(TAction::BrushesMenu, TAction::Polyline);
}

void TupCanvas::wakeUpRectangle()
{
    emit callAction(TAction::ShapesMenu, TAction::Rectangle);
}

void TupCanvas::wakeUpEllipse()
{
    emit callAction(TAction::ShapesMenu, TAction::Ellipse);
}

void TupCanvas::wakeUpLibrary()
{
    QString graphicPath = QFileDialog::getOpenFileName (this, tr("Import an image file..."), QDir::homePath(),
                                                       tr("Raster/Vector") + " (*.png *.jpg *.jpeg *.gif *.svg)");
    if (graphicPath.isEmpty())
        return;

    QFile f(graphicPath);
    QFileInfo fileInfo(f);

    QString lowercasePath = graphicPath.toLower();
    if (lowercasePath.endsWith(".svg")) {
        QString tag = fileInfo.fileName();

        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            f.close();
            TupProjectRequest request = TupRequestBuilder::createLibraryRequest(TupProjectRequest::Add, tag,
                                        TupLibraryObject::Svg, TupProject::FRAMES_MODE, data, QString(),
                                        scene->currentSceneIndex(), scene->currentLayerIndex(), scene->currentFrameIndex());
            emit requestTriggered(&request);
        }
    } else {
        QString symName = fileInfo.fileName();

        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            f.close();

            QPixmap *pixmap = new QPixmap(graphicPath);
            int picWidth = pixmap->width();
            int picHeight = pixmap->height();
            int projectWidth = size.width();
            int projectHeight = size.height();

            if (picWidth > projectWidth || picHeight > projectHeight) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Information"));
                msgBox.setIcon(QMessageBox::Question);
                msgBox.setText(tr("Image is bigger than workspace."));
                msgBox.setInformativeText(tr("Do you want to resize it?"));
                msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
                msgBox.setDefaultButton(QMessageBox::Ok);
                msgBox.show();
                msgBox.move(static_cast<int> ((screen->geometry().width() - msgBox.width()) / 2),
                            static_cast<int> ((screen->geometry().height() - msgBox.height()) / 2));

                int answer = msgBox.exec();

                if (answer == QMessageBox::Yes) {
                    pixmap = new QPixmap();
                    QString extension = fileInfo.suffix().toUpper();
                    QByteArray ba = extension.toLatin1();
                    const char* ext = ba.data();
                    if (pixmap->loadFromData(data, ext)) {
                        QPixmap newpix;
                        if (picWidth > projectWidth)
                            newpix = QPixmap(pixmap->scaledToWidth(projectWidth, Qt::SmoothTransformation));
                        else
                            newpix = QPixmap(pixmap->scaledToHeight(projectHeight, Qt::SmoothTransformation));
                        QBuffer buffer(&data);
                        buffer.open(QIODevice::WriteOnly);
                        newpix.save(&buffer, ext);
                    }
                }
           }

           QString tag = symName;

           TupProjectRequest request = TupRequestBuilder::createLibraryRequest(TupProjectRequest::Add, tag,
                                                                               TupLibraryObject::Image, TupProject::FRAMES_MODE, data, QString(),
                                                                               scene->currentSceneIndex(), scene->currentLayerIndex(), scene->currentFrameIndex());
           emit requestTriggered(&request);

           data.clear();
        }
    }
}

void TupCanvas::wakeUpSelection()
{
    emit callAction(TAction::SelectionMenu, TAction::ObjectSelection);
}

void TupCanvas::wakeUpNodes()
{
    emit callAction(TAction::SelectionMenu, TAction::NodesEditor);
}

void TupCanvas::wakeUpDeleteSelection()
{
    emit callAction(TAction::SelectionMenu, TAction::Delete);
}

void TupCanvas::wakeUpZoomIn()
{
    graphicsView->scale(1.3, 1.3);

    emit zoomFactorChangedFromFullScreen(1.3);
}

void TupCanvas::wakeUpZoomOut()
{
    graphicsView->scale(0.7, 0.7);

    emit zoomFactorChangedFromFullScreen(0.7);
}

void TupCanvas::undo()
{
    QAction *undo = kApp->findGlobalAction("undo");
    if (undo) 
        undo->trigger();
}

void TupCanvas::redo()
{
    QAction *redo = kApp->findGlobalAction("redo");
    if (redo) 
        redo->trigger();
}

void TupCanvas::enableRubberBand()
{
    graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
}
