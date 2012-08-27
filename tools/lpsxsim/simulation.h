// Author(s): Ruud Koolen
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef LPSXSIM_SIMULATION_H
#define LPSXSIM_SIMULATION_H

#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QWaitCondition>

#include "mcrl2/data/rewriter.h"
#include "mcrl2/lps/simulation.h"


class Simulation : public QObject
{
  Q_OBJECT

  public:
    typedef QStringList State;
    struct Transition
    {
      State destination;
      QString action;
    };
    struct TracePosition
    {
      State state;
      QList<Transition> transitions;
      size_t transitionNumber;
    };
    typedef QList<TracePosition> Trace;

  public:
    Simulation(QString filename, mcrl2::data::rewrite_strategy strategy);
    ~Simulation() { delete m_simulation; }
    bool initialized() const { return m_initialized; }
    const QStringList &parameters() const { return m_parameters; }
    Trace trace() { QMutexLocker locker(&m_traceMutex); return m_trace; }

  private slots:
    void init();
    void updateTrace(unsigned int firstChangedState);

  private:
    State renderState(const mcrl2::lps::state &state);

  public slots:
    void reset(unsigned int stateNumber) { m_simulation->truncate(stateNumber); updateTrace(stateNumber); }
    void select(unsigned int transitionNumber) { m_simulation->select(transitionNumber); updateTrace(m_trace.size() - 1); }
    void enable_tau_prioritization(bool enable, QString action = "ctau") { m_simulation->enable_tau_prioritization(enable, action.toStdString()); updateTrace(0); }
    void load(QString filename);
    void save(QString filename);

  signals:
    void traceChanged(unsigned int firstChangedState);

  private:
    QString m_filename;
    mcrl2::data::rewrite_strategy m_strategy;
    bool m_initialized;

    mcrl2::lps::simulation *m_simulation;
    QStringList m_parameters;
    Trace m_trace;
    QMutex m_traceMutex;
};

#endif