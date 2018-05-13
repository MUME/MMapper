#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include <QWidget>
#include "ui_graphicspage.h"

namespace Ui {
class GraphicsPage;
}

class GraphicsPage : public QWidget
{
    Q_OBJECT

public:
    explicit GraphicsPage(QWidget *parent = nullptr);

signals:

public slots:
    void changeColorClicked();
    void antialiasingSamplesTextChanged(const QString &);
    void trilinearFilteringStateChanged(int);
    void softwareOpenGLStateChanged(int);

    void updatedStateChanged(int);
    void drawNotMappedExitsStateChanged(int);
    void drawNoMatchExitsStateChanged(int);
    void drawDoorNamesStateChanged(int);
    void drawUpperLayersTexturedStateChanged(int);

private:
    Ui::GraphicsPage *ui;
};

#endif // GRAPHICSPAGE_H
