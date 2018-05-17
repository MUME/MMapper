#include "graphicspage.h"
#include "configuration/configuration.h"
#include "ui_graphicspage.h"

#include <QColorDialog>

GraphicsPage::GraphicsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphicsPage)
{
    ui->setupUi(this);

    connect(ui->changeColor, SIGNAL(clicked()), SLOT(changeColorClicked()));
    connect(ui->antialiasingSamplesComboBox, SIGNAL( currentTextChanged(const QString &) ), this,
            SLOT( antialiasingSamplesTextChanged(const QString &) )  );
    connect(ui->trilinearFilteringCheckBox, SIGNAL(stateChanged(int)),
            SLOT(trilinearFilteringStateChanged(int)));
    connect(ui->softwareOpenGLCheckBox, SIGNAL(stateChanged(int)),
            SLOT(softwareOpenGLStateChanged(int)));

    connect(ui->updated, SIGNAL(stateChanged(int)), SLOT(updatedStateChanged(int)));
    connect(ui->drawNotMappedExits, SIGNAL(stateChanged(int)),
            SLOT(drawNotMappedExitsStateChanged(int)));
    connect(ui->drawNoMatchExits, SIGNAL(stateChanged(int)), SLOT(drawNoMatchExitsStateChanged(int)));
    connect(ui->drawDoorNames, SIGNAL(stateChanged(int)), SLOT(drawDoorNamesStateChanged(int)));
    connect(ui->drawUpperLayersTextured, SIGNAL(stateChanged(int)),
            SLOT(drawUpperLayersTexturedStateChanged(int)));

    QPixmap bgPix(16, 16);
    bgPix.fill(Config().m_backgroundColor);
    ui->changeColor->setIcon(QIcon(bgPix));
    QString antiAliasingSamples = QString::number(Config().m_antialiasingSamples);
    int index = ui->antialiasingSamplesComboBox->findText(antiAliasingSamples);
    if (index < 0) {
        index = 0;
    }
    ui->antialiasingSamplesComboBox->setCurrentIndex(index);
    ui->trilinearFilteringCheckBox->setChecked(Config().m_trilinearFiltering);
    ui->softwareOpenGLCheckBox->setChecked(Config().m_softwareOpenGL);

    ui->updated->setChecked( Config().m_showUpdated );
    ui->drawNotMappedExits->setChecked( Config().m_drawNotMappedExits );
    ui->drawNoMatchExits->setChecked( Config().m_drawNoMatchExits );
    ui->drawUpperLayersTextured->setChecked( Config().m_drawUpperLayersTextured );
    ui->drawDoorNames->setChecked( Config().m_drawDoorNames );
}


void GraphicsPage::changeColorClicked()
{
    const QColor newColor = QColorDialog::getColor(Config().m_backgroundColor, this);
    if (newColor.isValid() && newColor != Config().m_backgroundColor) {
        QPixmap bgPix(16, 16);
        bgPix.fill(newColor);
        ui->changeColor->setIcon(QIcon(bgPix));
        Config().m_backgroundColor = newColor;
    }
}

void GraphicsPage::antialiasingSamplesTextChanged(const QString & /*unused*/)
{
    Config().m_antialiasingSamples = ui->antialiasingSamplesComboBox->currentText().toInt();
}

void GraphicsPage::trilinearFilteringStateChanged(int /*unused*/)
{
    Config().m_trilinearFiltering = ui->trilinearFilteringCheckBox->isChecked();
}

void GraphicsPage::softwareOpenGLStateChanged(int /*unused*/)
{
    Config().m_softwareOpenGL = ui->softwareOpenGLCheckBox->isChecked();
}


void GraphicsPage::updatedStateChanged(int /*unused*/)
{
    Config().m_showUpdated = ui->updated->isChecked();
}

void GraphicsPage::drawNotMappedExitsStateChanged(int /*unused*/)
{
    Config().m_drawNotMappedExits = ui->drawNotMappedExits->isChecked();
}

void GraphicsPage::drawNoMatchExitsStateChanged(int /*unused*/)
{
    Config().m_drawNoMatchExits = ui->drawNoMatchExits->isChecked();
}

void GraphicsPage::drawDoorNamesStateChanged(int /*unused*/)
{
    Config().m_drawDoorNames = ui->drawDoorNames->isChecked();
}

void GraphicsPage::drawUpperLayersTexturedStateChanged(int /*unused*/)
{
    Config().m_drawUpperLayersTextured = ui->drawUpperLayersTextured->isChecked();
}
