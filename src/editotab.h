#ifndef EDITOTAB_H
#define EDITOTAB_H

#include <QTabWidget>

class EditoTab : public QTabWidget
{
public:
    EditoTab(QWidget *parent = 0);

public slots:
    void closeTab(int);
};

#endif // EDITOTAB_H
