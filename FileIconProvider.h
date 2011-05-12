#ifndef FILEICONPROVIDER_H
#define FILEICONPROVIDER_H

#include <QFileIconProvider>

class FileIconProvider : public QFileIconProvider
{
public:
    FileIconProvider(QWidget* widget);

    QIcon icon(const QFileInfo &info) const;

private:
    QWidget* mWidget;
};

#endif // FILEICONPROVIDER_H
