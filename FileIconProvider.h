#ifndef FILEICONPROVIDER_H
#define FILEICONPROVIDER_H

#include <QFileIconProvider>

class FileIconProvider : public QFileIconProvider
{
public:
    FileIconProvider();

    QIcon icon(const QFileInfo &info) const;
};

#endif // FILEICONPROVIDER_H
