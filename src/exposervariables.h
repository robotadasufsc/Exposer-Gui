#include <QMap>
#include <QHash>
#include <QString>
#include <QVariant>

class ExposerVariables: public QObject
{
    Q_OBJECT

private:
    struct varTime
    {
        QVariant var;
        qint64 time;
    };
    // Save variables settings here
    QHash<QString, QList<varTime> > vars;
    struct varInfo
    {
        uint type = -1;
        QString name = QString();
        bool done = false;
        QList<varTime>* QVariantList;
    };

public:
    ExposerVariables(QWidget* parent = nullptr);
    ~ExposerVariables();
    QVariant getLast(uint);
    QVariant getLast(QString);
    void append(QString, QVariant, qint64);
    void append(uint, QVariant, qint64);
    void setType(QString, uint type);
    void setType(uint, uint type);
    int getType(QString);
    int getType(uint);
    int getIndice(QString);
    QString getName(uint);
    void setIndiceName(QString, uint);
    bool isValid(QString);
    bool isValid(uint);
    bool getDone(uint);
    void checkVarInfo(uint);
    int size();
    void print();

    QMap<uint, varInfo> indiceToVarInfo;

    enum {UINT8, UINT16, UINT32, INT8, INT16, INT32, FLOAT, STRING};
};
