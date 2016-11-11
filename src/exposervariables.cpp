#include <QDebug>
#include "exposervariables.h"

ExposerVariables::ExposerVariables(QWidget *parent)
{

}

QVariant ExposerVariables::getLast(uint i)
{
    return getLast(getName(i));
}

QVariant ExposerVariables::getLast(QString name)
{
    return vars[name].isEmpty() ? QVariant() : vars[name].last().var;
}

void ExposerVariables::append(QString name, QVariant data, qint64 time)
{
    varTime a;
    a.var = data;
    a.time = time;
    vars[name].append(a);
}

void ExposerVariables::append(uint i, QVariant data, qint64 time)
{
    append(getName(i), data, time);
}

void ExposerVariables::setType(QString name, uint type)
{
    setType(getIndice(name), type);
}

void ExposerVariables::setType(uint i, uint type)
{
    indiceToVarInfo[i].type = type;
    checkVarInfo(i);
}

int ExposerVariables::getType(QString name)
{
    if (vars.contains(name))
        return indiceToVarInfo[getIndice(name)].type;
    return -1;
}

int ExposerVariables::getType(uint i)
{
    QString name = getName(i);
    return getType(name);
}

int ExposerVariables::getIndice(QString name)
{
    uint i = 0;
    for (const auto &indice: indiceToVarInfo)
    {
        if (indice.name == name)
            return i;
        i++;
    }
    return -1;
}

QString ExposerVariables::getName(uint i)
{
    return indiceToVarInfo[i].name;
}

void ExposerVariables::setIndiceName(QString name, uint i)
{
    indiceToVarInfo[i].name = name;
    checkVarInfo(i);
}

int ExposerVariables::size()
{
    return indiceToVarInfo.size();
}

bool ExposerVariables::isValid(QString name)
{
    bool valid = vars.contains(name);
    valid = valid && (getType(name) != -1);
    valid = valid && (getIndice(name) != -1);
    return valid;
}

bool ExposerVariables::isValid(uint i)
{
    QString name = getName(i);
    return isValid(name);
}

bool ExposerVariables::getDone(uint i)
{
    return indiceToVarInfo[i].done;
}

void ExposerVariables::checkVarInfo(uint i)
{

    if ((getDone(i) == true) && (isValid(i) == true))
        return;

    indiceToVarInfo[i].done = true;
    indiceToVarInfo[i].QVariantList = &vars[getName(i)];
}

void ExposerVariables::print()
{
    qDebug() << "indiceToVarInfoSize " << indiceToVarInfo.size();
    for (const auto &var: indiceToVarInfo)
    {
        qDebug() << "Var name " << var.name;
        qDebug() << "Var indice " << getIndice(var.name);
        qDebug() << "Var type " << var.type;
    }

    //qDebug() << vars;
}

ExposerVariables::~ExposerVariables()
{

}
