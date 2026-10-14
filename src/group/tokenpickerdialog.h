#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialog>
#include <QMap>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

class TokenPickerDialog final : public QDialog
{
private:
    QListWidget *m_tokenList = nullptr;
    QLineEdit *m_filter = nullptr;
    QLabel *m_preview = nullptr;
    QLabel *m_selectedName = nullptr;
    QPushButton *m_selectButton = nullptr;
    QString m_selectedToken;
    QString m_tokenToSelectAfterImport;
    bool m_useDefault = false;

    void updatePreview();
    void populateTokens(const QMap<QString, QString> &availableFiles,
                        const QString &selectToken = QString());

public:
    ~TokenPickerDialog() override;
    explicit TokenPickerDialog(const QMap<QString, QString> &availableFiles,
                               QWidget *parent = nullptr);

    const QString &selectedToken() const { return m_selectedToken; }
    bool useDefault() const { return m_useDefault; }
};
